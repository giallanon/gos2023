#include "gosEngine_renderer.h"
#include "gosEngine.h"
#include <algorithm>

using namespace gos;
using namespace gos::engine;



//**********************************
Renderer1::Renderer1()
{
    engine = NULL;
    material_buffer = NULL;
    pRenderableList = NULL;
    nRenderable = 0;
}

//**********************************
void Renderer1::unsetup()
{
    texture_array.unsetup();
    gpu->buffer_unmap (matrix_buffer);
    GOSFREE(localAllocator, material_buffer);
    GOSFREE(localAllocator, pRenderableList);
    material_bitmask.unsetup (localAllocator);
    

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
        gpu->deleteResource(handle_sbo_instanceData);
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
    localAllocator = allocator;
    engine = engineIN;
    gpu = engine->gpu;

    //load degli assets
    if (!engine->assetHub->getHandle ("pipe3", &assHandle_pipe, true))
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

    //creo gli oggetti che poi dovro' bindare ai descrittori
    texture_array.setup(localAllocator, NUM_MAX_TEXTURE);
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
        matrix_sizeof_buffer = NUM_MAX_MATRIX * sizeof(mat4x4f);
        matrix_default.identity();
        gpu->storageBuffer_create (matrix_sizeof_buffer, eMemAccessMode::shared_cpuW_manualSync, &handle_sbo_matrixList);
        gpu->map (handle_sbo_matrixList, 0, u32MAX, &matrix_buffer);

        //SBO materialList
        material_sizeof_buffer = NUM_MAX_MATERIAL * sizeof(Material);
        material_buffer = (Material*) GOSALIGNEDALLOC(localAllocator, material_sizeof_buffer, gpu->limits_get_minStorageBufferOffsetAlignment());
        material_bitmask.setup (localAllocator, NUM_MAX_MATERIAL);
        material_bitmask.zero();
        material_wasUpdated = 1;
        material_default.texture_index = 0;
        material_default.diffuse_col.set (1.0f, 1.0f, 1.0f);
        gpu->storageBuffer_create (material_sizeof_buffer, eMemAccessMode::shared_cpuW_autoSync, &handle_sbo_materiaList);

        ///SBO instance data
        //instance_sizeof_buffer = NUM_MAX_MATRIX * sizeof(InstanceData);
        instance_sizeof_buffer = NUM_MAX_MATRIX * sizeof(u64);
        gpu->storageBuffer_create (instance_sizeof_buffer, eMemAccessMode::shared_cpuW_manualSync, &handle_sbo_instanceData);
        gpu->map (handle_sbo_instanceData, 0, u32MAX, &instance_buffer);
        pRenderableList = GOSALLOCT(u64*, localAllocator, sizeof(u64) * NUM_MAX_MATRIX);
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
                .bindStorageBuffer (2, handle_sbo_instanceData)
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
    material_buffer[material_index].texture_index = texture_index;
    material_buffer[material_index].diffuse_col = diffuse_col;
    
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
        return &material_buffer[material_index];
    }
    DBGBREAK;
    return &material_default;
}

//**********************************
const Renderer1::Material* Renderer1::material_query (u32 material_index) const
{
    if (material_bitmask.isBitSet(material_index))
        return &material_buffer[material_index];

    DBGBREAK;
    return &material_default;
}



//**********************************
u64 Renderer1::priv_pack_renderable (ENGShape shape, u32 material_index, u32 matrix_index) const
{
    u64 ret = shape.viewAsU32();
    ret <<= 32;

    //14bit per material_index (16.384)
    ret |= (u64)(material_index & 0x3FFF) << 18;

    //18bit per matrix_index (262.144)
    ret |= (u64)(matrix_index & 0x3FFFF);

    return ret;
}

//**********************************
void Renderer1::priv_unpack_renderable (u64 packed, ENGShape *out_shape, u32 *out_material_index, u32 *out_matrix_index) const
{
    out_shape->setFromU32 ( (u32)(packed >> 32) );
    *out_material_index = (u32) ((packed >> 18) & 0x3FFF);
    *out_matrix_index = (u32)(packed & 0x3FFFF);
}

//**********************************
void Renderer1::begin (gos::geom::Camera3 *cam)
{
    matrix_nextIndex = 0;
    nRenderable = 0;

    //aggiorno UBO descrittore scena
	scene.matVP = cam->getMatVP();
	scene.lightDir = vec4f (cam->pos.getAsseZ(), 0);
	scene.lightDir.normalize();
	gpu->writeAndSync (handle_ubo_scene, 0, &scene, sizeof(scene));
}

//**********************************
void Renderer1::add (const ENGShape shape, const mat4x4f &m, u32 material_index)
{
    if (matrix_nextIndex >= NUM_MAX_MATRIX)
        return;
    const u32 matrix_index = matrix_nextIndex++;

    u8 *p = reinterpret_cast<u8*>(matrix_buffer.host_pt);
    memcpy (&p[sizeof(mat4x4f) * matrix_index], m._getValuesPtConst(), sizeof(mat4x4f));

    //u64 *pRenderableList = static_cast<u64*>(instance_buffer.host_pt);
    pRenderableList[nRenderable++] = priv_pack_renderable (shape, material_index, matrix_index);
}

//**********************************
void Renderer1::end (gos::gpu::pipe2::CmdBufferWriter2 &cw)
{
    if (0 == nRenderable)
        return;

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
        gpu->writeAndSync (handle_sbo_materiaList, 0, material_buffer, material_sizeof_buffer);
    }

    //devo aggiornare SBO delle matrici
    if (matrix_nextIndex)
    {
        gpu->buffer_manualSync_cpuWrite (matrix_buffer, 0, sizeof(mat4x4f) * matrix_nextIndex);
    }    

    

    //sort
    std::sort (pRenderableList, &pRenderableList[nRenderable]);

    //memcpio nel SSBO
    memcpy (instance_buffer.host_pt, pRenderableList, sizeof(u64) *nRenderable);
    gpu->buffer_manualSync_cpuWrite (instance_buffer, 0, u32MAX);




    //command
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
    u32 cur_index = 0;
    u32 first_instance_index = 0;
    while (cur_index < nRenderable)
    {
        ENGShape cur_shape;
        u32 material_index;
        u32 matrix_index;
        priv_unpack_renderable (pRenderableList[cur_index], &cur_shape, &material_index, &matrix_index);
        cur_index++;

        //conto quante shape identiche a cur_shape ci sono
        u32 numInstances = 1;
        while (cur_index < nRenderable)
        {
            ENGShape shape;
            priv_unpack_renderable (pRenderableList[cur_index], &shape, &material_index, &matrix_index);
            if (cur_shape == shape)
            {
                cur_index++;
                numInstances++;
            }
            else
                break;
        }


        const engine::Shape *cur_shape_info = engine->shape_getInfo (cur_shape);
        renderer
            .bindVtxIdxBuffer (cur_shape_info->vbHandle, 0, cur_shape_info->ibHandle, 0)
            .drawIndexed (cur_shape_info->numIndices, numInstances, cur_shape_info->indexStart, cur_shape_info->vtxStart, first_instance_index)
            ;

        first_instance_index += numInstances;
    }

    renderer.endRender();
}
