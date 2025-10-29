#include "gosEngine_renderer.h"
#include "gosEngine.h"

using namespace gos;
using namespace gos::engine;


//**********************************
Renderer1::Renderer1()
{
    engine = NULL;
}

//**********************************
void Renderer1::unsetup()
{
    matrixBuffer.unsetup();
    materialBuffer.unsetup();

    if (NULL != gpu)
    {
        engine->assetHub->unload (assHandle_pipe);
        
        
        gpu->deleteResource(handle_zbuffer);
        gpu->deleteResource(handle_rt0);
        //gpu->deleteResource(handle_samplers[0]);
        //gpu->deleteResource(handle_samplers[1]);
        gpu->deleteResource(handle_ubo_scene);
        gpu->deleteResource(handle_sbo_matrixList);
        gpu->deleteResource(handle_sbo_materiaList);
        gpu->deleteResource(handle_descrSet0);
        gpu->deleteResource(handle_descrSet1);
        gpu->deleteResource(handle_descrSet2);
        gpu->deleteResource(handle_descrPool);
    }

    gpu = NULL;
    engine = NULL;
}

//**********************************
bool Renderer1::setup (gos::Allocator *allocator, gos::Engine *engineIN)
{
    engine = engineIN;
    gpu = engine->gpu;

    //load degli assets
    if (!engine->assetHub->getHandle ("pipe2", &assHandle_pipe, true))
        return false;    


    //risorse di rendering
    {
        //rt0
        if (!gpu->renderTarget_create ("0-", "0-", eImageFormat::U8_RGBA, &handle_rt0))
            return false;

        //zbuffer
        if (!gpu->zbuffer_create ("0-", "0-", eImageFormat::_DEPTH_BEST, &handle_zbuffer))
        {
            gos::logger::err ("Renderer1::setup() => GPU::zbuffer_create\n");
            return false;
        }

        //creo un descriptor pool
        gpu->descrPool_createNew (&handle_descrPool)
            .setMaxNumDescriptorSet(4)
            .addPool_uniformBuffer(1)
            .addPool_storageBuffer(2)
            .addPool_sampler(2)
            .addPool_texture(NUM_MAX_TEXTURE)
            .end();
        if (handle_descrPool.isInvalid())
        {
            gos::logger::err ("Renderer1::setup() => can't create descriptor pool\n");
            return false;
        }
    }


    //attendo che la pipe sia stata caricata perche' mi servono le definizioni dei descrittori
    const asset::Asset_pipe *pipe;
    engine->assetHub->getAssetWithTimeout (assHandle_pipe, 5000, &pipe);
    {
        //alloco una istanza dei descriptor-set
        gos::gpu::DescrSetInstanceWriter dsw;

        //descriptor set 0
        if (!gpu->descrSetInstance_create (handle_descrPool, pipe->handle_pipe, 0, &handle_descrSet0))
        {
            gos::logger::err ("Renderer1::setup() => can't create an instance of descriptorSet_0\n");
            return false;
        }
        else
        {
            gpu::SamplerDesc desc;

            //sampler2d: bilinear filtering
            desc.reset();
            gpu->sampler_create (desc, &handle_samplers[0]);

            //sampler2d: point filtering
            desc.reset();
            desc.minFilter = desc.magFilter = eSamplerFilter::point;
            desc.mipFilter = eSamplerMipFilter::nearest;
            desc.bAnisotropic = false;
            gpu->sampler_create (desc, &handle_samplers[1]);

            dsw.begin (gpu, handle_descrSet0)
                .bindSamplerInArray  (0, handle_samplers[0], 0)             //bindo in samplerList[0] il sampler "bilinear"
                .bindSamplerInArray  (0, handle_samplers[1], 1)             //bindo in samplerList[1] il sampler "point"
                .end();
        }
        

        //descriptor set 1
        if (!gpu->descrSetInstance_create (handle_descrPool, pipe->handle_pipe, 1, &handle_descrSet1))
        {
            gos::logger::err ("Renderer1::setup() => can't create an instance of descriptorSet_0\n");
            return false;
        }
        else
        {
            gpu->uniformBuffer_create (sizeof(SceneData), eMemAccessMode::shared_cpuW_autoSync, &handle_ubo_scene);
            dsw.begin (gpu, handle_descrSet1)
                .bindUniformBuffer (0, handle_ubo_scene, 0)
                .end();
        }


        //descriptor set 2        
        if (!gpu->descrSetInstance_create (handle_descrPool, pipe->handle_pipe, 2, &handle_descrSet2))
        {
            gos::logger::err ("Renderer1::setup() => can't create an instance of descriptorSet_0\n");
            return false;
        }
        else
        {
			//SBO matrici
            matrixBuffer.setup (allocator, NUM_MAX_MATRIX, gpu->limits_get_minStorageBufferOffsetAlignment());
        	gpu->storageBuffer_create (matrixBuffer.getRealSizeAllocated(), eMemAccessMode::shared_cpuW_autoSync, &handle_sbo_matrixList);

			//SBO materialList
            materialBuffer.setup (allocator, NUM_MAX_MATERIAL, gpu->limits_get_minStorageBufferOffsetAlignment());
        	gpu->storageBuffer_create (materialBuffer.getRealSizeAllocated(), eMemAccessMode::shared_cpuW_autoSync, &handle_sbo_materiaList);

			dsw.begin (gpu, handle_descrSet2)
				.bindStorageBuffer (0, handle_sbo_matrixList, 0)
				.bindStorageBuffer (1, handle_sbo_materiaList, 0)
				.end();
        }
    }


    return true;
}

//**********************************
void Renderer1::begin (gos::geom::Camera3 *cam)
{
    //aggiorno UBO descrittore scena
	scene.matVP = cam->getMatVP();
	scene.lightDir = vec4f (cam->pos.getAsseZ(), 0);
	scene.lightDir.normalize();
	gpu->writeAndSync (handle_ubo_scene, 0, &scene, sizeof(scene));
}



//**********************************
void Renderer1::end (gos::gpu::pipe2::CmdBufferWriter2 &cw)
{
    const asset::Asset_pipe *pipe;
    if (!engine->assetHub->getAsset (assHandle_pipe, &pipe))
        return;

	cw
		.imageTransition (handle_rt0, eImageLayout::undefined, eImageLayout::color_attachment_optimal)
		.imageTransition (handle_zbuffer, eImageLayout::undefined, eImageLayout::depth_attachment_optimal);

	auto &r = cw.beginRender();
			r.withRenderArea (handle_rt0)
			.withRT (handle_rt0, eAttachmentLoadOp::clear, eAttachmentStoreOp::dont_care, gos::ColorHDR(0, 0.0f, 0.1f))
			.withZB (handle_zbuffer, eAttachmentLoadOp::clear, eAttachmentStoreOp::dont_care)
			.bindPipeline (pipe->handle_pipe)
			.bindDescriptorSet (handle_descrSet0, 0)
			.bindDescriptorSet (handle_descrSet1, 1)
			.bindDescriptorSet (handle_descrSet2, 2);


            /*render delle shape
			.bindVtxBuffer(info_shape->vbHandle)
			.bindIdxBufferU16(info_shape->ibHandle);
			for (u32 i = 0; i < 4; i++)
			{
				r.pushConstant(0, &i, sizeof(u32));	//matrix index
				r.pushConstant(1, &i, sizeof(u32));	//material index
				r.drawIndexed (info_shape->numIndices, 1, info_shape->indexStart, info_shape->vtxStart, 0);
			}
            */

    r.endRender();
}
