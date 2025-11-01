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
    gos::Allocator *allocator = material_buffer.getAllocator();
    renderableList.unsetup();
    texture_array.unsetup();
    matrix_buffer.unsetup();
    matrix_bitmask.unsetup (allocator);
    material_buffer.unsetup();
    material_bitmask.unsetup (allocator);
    

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

    renderableList.setup (allocator, 32768);

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

    //creo gli oggetti che poi dovro' bindare ai descrittori
    texture_array.setup(allocator, NUM_MAX_TEXTURE);
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

        //UBO "scene"
        gpu->uniformBuffer_create (sizeof(SceneData), eMemAccessMode::shared_cpuW_autoSync, &handle_ubo_scene);

        //SBO matrici
        matrix_buffer.setup (allocator, NUM_MAX_MATRIX, gpu->limits_get_minStorageBufferOffsetAlignment());
        matrix_bitmask.setup (allocator, NUM_MAX_MATRIX);
        matrix_bitmask.zero();
        matrix_wasUpdated = 1;
        matrix_default.identity();
        gpu->storageBuffer_create (matrix_buffer.getRealSizeAllocated(), eMemAccessMode::shared_cpuW_autoSync, &handle_sbo_matrixList);

        //SBO materialList
        material_buffer.setup (allocator, NUM_MAX_MATERIAL, gpu->limits_get_minStorageBufferOffsetAlignment());
        material_bitmask.setup (allocator, NUM_MAX_MATERIAL);
        material_bitmask.zero();
        material_wasUpdated = 1;
        material_default.texture_index = 0;
        material_default.diffuse_col.set (1.0f, 1.0f, 1.0f);
        gpu->storageBuffer_create (material_buffer.getRealSizeAllocated(), eMemAccessMode::shared_cpuW_autoSync, &handle_sbo_materiaList);
    }

    //attendo che la pipe sia stata caricata perche' mi servono le definizioni dei descrittori
    const asset::Asset_pipe *pipe;
    engine->assetHub->getAssetWithTimeout (assHandle_pipe, &pipe, 5000);
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
			dsw.begin (gpu, handle_descrSet2)
				.bindStorageBuffer (0, handle_sbo_matrixList, 0)
				.bindStorageBuffer (1, handle_sbo_materiaList, 0)
				.end();
        }
    }


    return true;
}

//**********************************
u32 Renderer1::texture_addIfNotExitst (GPUTextureHandle texHandle)
{ 
    bool bWasNew;
    const u32 texture_index = texture_array.addIfNotExitst(texHandle, &bWasNew);
    if (bWasNew)
    {
        gos::gpu::DescrSetInstanceWriter dsw;
        dsw.begin (gpu, handle_descrSet0)
            .bindTextureInArray (1, texHandle, texture_index)
            .end();
    }

    return texture_index;
}

//**********************************
u32 Renderer1::material_create (u32 texture_index, const vec3f diffuse_col)
{
    u32 material_index;
    if (!material_bitmask.findAndSetFirstFreeBit(&material_index))
    {
        DBGBREAK;
        return u32MAX;
    }
    
    //creo il nuovo materiale
    Material *m = material_buffer.getElem(material_index);
    m->texture_index = texture_index;
    m->diffuse_col = diffuse_col;
    
    //mi segno che l'array dei materiali e' da aggiornare su GPU
    material_wasUpdated = 1;
    
    return material_index;
}

//**********************************
void Renderer1::material_delete (u32 material_index)
{
    material_bitmask.clear (material_index);
}

//**********************************
Renderer1::Material* Renderer1::material_getForUpdate (u32 material_index)
{
    if (material_bitmask.isBitSet(material_index))
    {
        //mi segno che l'array dei materiali e' da aggiornare su GPU
        material_wasUpdated = 1;        
        return material_buffer.getElem(material_index);
    }
    DBGBREAK;
    return &material_default;
}

//**********************************
const Renderer1::Material* Renderer1::material_query (u32 material_index) const
{
    if (material_bitmask.isBitSet(material_index))
        return material_buffer.queryElem(material_index);

    DBGBREAK;
    return &material_default;
}



//**********************************
u32 Renderer1::matrix_create ()                                         { mat4x4f m; m.identity(); return matrix_create(m); }
u32 Renderer1::matrix_create (const mat4x4f &mIN)
{
    u32 matrix_index;
    if (!matrix_bitmask.findAndSetFirstFreeBit(&matrix_index))
    {
        DBGBREAK;
        return u32MAX;
    }
    
    //creo il nuovo materiale
    mat4x4f *m = matrix_buffer.getElem(matrix_index);
    *m = mIN;
    
    //mi segno che l'array delle matrici e' da aggiornare su GPU
    matrix_wasUpdated = 1;
    
    return matrix_index;
}

//**********************************
void Renderer1::matrix_delete (u32 matrix_index)
{
    matrix_bitmask.clear (matrix_index);
}

//**********************************
mat4x4f* Renderer1::matrix_getForUpdate (u32 matrix_index)
{
    if (matrix_bitmask.isBitSet(matrix_index))
    {
        //mi segno che l'array dei materiali e' da aggiornare su GPU
        matrix_wasUpdated = 1;        
        return matrix_buffer.getElem(matrix_index);
    }
    DBGBREAK;
    return &matrix_default;
}

//**********************************
void Renderer1::matrix_update (u32 matrix_index, const mat4x4f &mIN)
{
    mat4x4f *m = matrix_getForUpdate (matrix_index);
    *m = mIN;
}

//**********************************
const mat4x4f* Renderer1::matrix_query (u32 matrix_index) const
{
    if (matrix_bitmask.isBitSet(matrix_index))
        return matrix_buffer.queryElem(matrix_index);

    DBGBREAK;
    return &matrix_default;
}



//**********************************
void Renderer1::begin (gos::geom::Camera3 *cam)
{
    renderableList.reset();

    //aggiorno UBO descrittore scena
	scene.matVP = cam->getMatVP();
	scene.lightDir = vec4f (cam->pos.getAsseZ(), 0);
	scene.lightDir.normalize();
	gpu->writeAndSync (handle_ubo_scene, 0, &scene, sizeof(scene));
}

//**********************************
void Renderer1::add (const ENGShape shape, u32 matrixIndex, u32 materialIndex)
{
    renderableList.append (Renderable{ 
        .shape = shape,
        .matrixIndex = matrixIndex,
        .materialIndex = materialIndex });
}

//**********************************
void Renderer1::end (gos::gpu::pipe2::CmdBufferWriter2 &cw)
{
    const asset::Asset_pipe *pipe;
    if (!engine->assetHub->getAsset (assHandle_pipe, &pipe))
    {
        return;
    }

    //devo aggiornare SBO dei materiali?
    if (material_wasUpdated)
    {
        material_wasUpdated = 0;

        //TODO: non c'e' bisogno di uppare l'intero material_buffer tutte le volte, idealmente basta uppare solo
        //gli elementi che sono stati modificati
        gpu->writeAndSync (handle_sbo_materiaList, 0, material_buffer.getBuffer(), material_buffer.getRealSizeAllocated());
    }

    //devo aggiornare SBO delle matrici
    if (matrix_wasUpdated)
    {
        matrix_wasUpdated = 0;

        //TODO: non c'e' bisogno di uppare l'intero buffer tutte le volte, idealmente basta uppare solo
        //gli elementi che sono stati modificati
        gpu->writeAndSync (handle_sbo_matrixList, 0, matrix_buffer.getBuffer(), matrix_buffer.getRealSizeAllocated());
    }    



	cw  .imageTransition (handle_rt0, eImageLayout::undefined, eImageLayout::color_attachment_optimal)
		.imageTransition (handle_zbuffer, eImageLayout::undefined, eImageLayout::depth_attachment_optimal);

	auto &renderer = cw.beginRender();
    renderer.withRenderArea (handle_rt0)
            .withRT (handle_rt0, eAttachmentLoadOp::clear, eAttachmentStoreOp::dont_care, gos::ColorHDR(0, 0.0f, 0.1f))
            .withZB (handle_zbuffer, eAttachmentLoadOp::clear, eAttachmentStoreOp::dont_care)
			.bindPipeline (pipe->handle_pipe)
			.bindDescriptorSet (handle_descrSet0, 0)
			.bindDescriptorSet (handle_descrSet1, 1)
			.bindDescriptorSet (handle_descrSet2, 2);

    //render delle shape
    const u32 n = renderableList.getNElem();
    for (u32 i=0; i<n; i++)
    {
        const Renderable &r = renderableList.queryElem(i);
        const engine::Shape *info_shape = engine->shape_getInfo (r.shape);
        
        renderer.bindVtxBuffer(info_shape->vbHandle)
                .bindIdxBufferU16(info_shape->ibHandle)
                .pushConstant(0, &r.matrixIndex, sizeof(u32))	//matrix index
                .pushConstant(1, &r.materialIndex, sizeof(u32))	//material index
                .drawIndexed (info_shape->numIndices, 1, info_shape->indexStart, info_shape->vtxStart, 0);

    }


    renderer.endRender();
}
