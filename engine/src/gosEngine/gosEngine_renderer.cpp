#include "gosEngine_renderer.h"
#include "gosEngine.h"
#include <algorithm>

using namespace gos;
using namespace gos::engine;


#define USE_QSORT

//**********************************
Renderer1::Renderer1()
{
    engine = NULL;
    material_buffer = NULL;
}

//**********************************
void Renderer1::unsetup()
{
    gos::Allocator *allocator = renderableList.getAllocator();
    renderableList.unsetup();
    texture_array.unsetup();
    gpu->buffer_unmap (matrix_buffer);
    GOSFREE(allocator, material_buffer);
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

#ifdef USE_QSORT
        gpu->deleteResource(handle_sbo_instanceData);
#endif

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
#ifdef USE_QSORT
    if (!engine->assetHub->getHandle ("pipe3", &assHandle_pipe, true))
        return false;    
#else    
    if (!engine->assetHub->getHandle ("pipe2", &assHandle_pipe, true))
        return false;    
#endif


    renderableList.setup (allocator, NUM_MAX_MATRIX);

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
        matrix_sizeof_buffer = NUM_MAX_MATRIX * sizeof(mat4x4f);
        matrix_default.identity();
        gpu->storageBuffer_create (matrix_sizeof_buffer, eMemAccessMode::shared_cpuW_manualSync, &handle_sbo_matrixList);
        gpu->map (handle_sbo_matrixList, 0, u32MAX, &matrix_buffer);

        //SBO materialList
        material_sizeof_buffer = NUM_MAX_MATERIAL * sizeof(Material);
        material_buffer = (Material*) GOSALIGNEDALLOC(allocator, material_sizeof_buffer, gpu->limits_get_minStorageBufferOffsetAlignment());
        material_bitmask.setup (allocator, NUM_MAX_MATERIAL);
        material_bitmask.zero();
        material_wasUpdated = 1;
        material_default.texture_index = 0;
        material_default.diffuse_col.set (1.0f, 1.0f, 1.0f);
        gpu->storageBuffer_create (material_sizeof_buffer, eMemAccessMode::shared_cpuW_autoSync, &handle_sbo_materiaList);

#ifdef USE_QSORT
        ///SBO instance data
        //instance_sizeof_buffer = NUM_MAX_MATRIX * sizeof(InstanceData);
        instance_sizeof_buffer = NUM_MAX_MATRIX * sizeof(Renderable);
        gpu->storageBuffer_create (instance_sizeof_buffer, eMemAccessMode::shared_cpuW_manualSync, &handle_sbo_instanceData);
        gpu->map (handle_sbo_instanceData, 0, u32MAX, &instance_buffer);        
#endif        
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
                
#ifdef USE_QSORT
                .bindStorageBuffer (2, handle_sbo_instanceData)
#endif                
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
void Renderer1::begin (gos::geom::Camera3 *cam)
{
    renderableList.reset();
    matrix_nextIndex = 0;

    //aggiorno UBO descrittore scena
	scene.matVP = cam->getMatVP();
	scene.lightDir = vec4f (cam->pos.getAsseZ(), 0);
	scene.lightDir.normalize();
	gpu->writeAndSync (handle_ubo_scene, 0, &scene, sizeof(scene));
}

//**********************************
void Renderer1::add (const ENGShape shape, const mat4x4f &m, u32 materialIndex)
{
    assert (matrix_nextIndex < NUM_MAX_MATRIX);
    const u32 matrixIndex = matrix_nextIndex++;

    u8 *p = reinterpret_cast<u8*>(matrix_buffer.host_pt);
    memcpy (&p[sizeof(mat4x4f) * matrixIndex], m._getValuesPtConst(), sizeof(mat4x4f));

    renderableList.append (Renderable{ 
        .shape = shape,
        .matrixIndex = matrixIndex,
        .materialIndex = materialIndex });
}

//**********************************
void Renderer1::end (gos::gpu::pipe2::CmdBufferWriter2 &cw)
{
    const u32 nRenderable = renderableList.getNElem();
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

    


#ifdef USE_QSORT
    //sort
    Renderable *pRenderableList = renderableList._getTypedPointer();
    std::sort (pRenderableList, pRenderableList + nRenderable, [](const Renderable &r1, const Renderable &r2){
        return (r1.materialIndex < r2.materialIndex);
    });

    //memcpio nel SSBO  (TODO: non serve avere <renderableList>, posso usare direttamente SSBO per storare i renderabili
    //                         cosi' evito di fare questo mmcpy
    memcpy (instance_buffer.host_pt, pRenderableList, sizeof(Renderable) *nRenderable);
    gpu->buffer_manualSync_cpuWrite (instance_buffer, 0, u32MAX);
#endif


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
#ifdef USE_QSORT
    u32 cur_index = 0;
    u32 first_instance_index = 0;
    while (cur_index < nRenderable)
    {
        ENGShape cur_shape = pRenderableList[cur_index++].shape;

        //conto quante shape identiche a cur_shape ci sono
        u32 numInstances = 1;
        while (cur_index < nRenderable)
        {
            if (pRenderableList[cur_index].shape == cur_shape)
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
#else    
    for (u32 i=0; i<nRenderable; i++)
    {
        const Renderable &r = renderableList.queryElem(i);
        const engine::Shape *info_shape = engine->shape_getInfo (r.shape);
        
        renderer
            //.bindVtxBuffer(info_shape->vbHandle)
            //.bindIdxBufferU16(info_shape->ibHandle)
            .bindVtxIdxBuffer (info_shape->vbHandle, 0, info_shape->ibHandle, 0)
            .pushConstant(0, &r.matrixIndex, sizeof(u32))	//matrix index
            .pushConstant(1, &r.materialIndex, sizeof(u32))	//material index
            .drawIndexed (info_shape->numIndices, 1, info_shape->indexStart, info_shape->vtxStart, 0)
            ;
    }
#endif


    renderer.endRender();
}
