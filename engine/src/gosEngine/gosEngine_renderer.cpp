#include "gosEngine_renderer.h"
#include "gosEngine.h"
#include <algorithm>

using namespace gos;
using namespace gos::engine;

//**********************************
void RendererCommon::unsetup()
{
    texture_array.unsetup();

    if (NULL == engine)
        return;

    engine->release(handle_pipeline);
        
    engine->gpu->deleteResource(handle_zbuffer);
    engine->gpu->deleteResource(handle_rt0);
    //gpu->deleteResource(handle_samplers[0]);
    //gpu->deleteResource(handle_samplers[1]);
    engine->gpu->deleteResource(handle_descrSet0);

    engine->gpu->deleteResource(handle_descrPool);

    engine = NULL;
}

//**********************************
bool RendererCommon::setup (gos::Allocator *allocator, gos::Engine *engineIN, const char *pipeline_asset_name)
{
    engine = engineIN;
    gos::GPU *gpu = engineIN->gpu;

    //load degli assets
    if (!engine->pipeline_createFromAsset (pipeline_asset_name, &handle_pipeline, res::eLoadMode::asap))
        return false; 

    //risorse di rendering
    {
        //rt0
        if (!gpu->renderTarget_create ("0-", "0-", eImageFormat::U8_RGBA, &handle_rt0))
            return false;

        //zbuffer
        if (!gpu->zbuffer_create ("0-", "0-", eImageFormat::_DEPTH_BEST, &handle_zbuffer))
        {
            gos::logger::err ("RendererCommon::setup() => GPU::zbuffer_create\n");
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
            gos::logger::err ("RendererCommon::setup() => can't create descriptor pool\n");
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


    }

    //attendo che la pipe sia stata caricata perche' mi servono le definizioni dei descrittori
    const res::Pipeline *res_pipeline;
    if (engine->get (handle_pipeline, &res_pipeline, 5000))
    {
        //alloco una istanza dei descriptor-set
        gos::gpu::DescrSetInstanceWriter dsw;

        //descriptor set 0
        if (!gpu->descrSetInstance_create (handle_descrPool, res_pipeline->pipeHandle, 0, &handle_descrSet0))
        {
            gos::logger::err ("RendererCommon::setup() => can't create an instance of descriptorSet_0\n");
            return false;
        }
        else
        {
            dsw.begin (gpu, handle_descrSet0)
                .bindSamplerInArray  (0, handle_samplers[0], 0)             //bindo in samplerList[0] il sampler "bilinear"
                .bindSamplerInArray  (0, handle_samplers[1], 1)             //bindo in samplerList[1] il sampler "point"
                .end();
        }
    }

    return true;
}

//**********************************
u32 RendererCommon::texture_addIfNotExitst (GPUTextureHandle texHandle)
{ 
    bool bWasNew;
    const u32 texture_index = texture_array.addIfNotExitst(texHandle, &bWasNew);
    if (bWasNew)
    {
        gos::gpu::DescrSetInstanceWriter dsw;
        dsw.begin (engine->gpu, handle_descrSet0)
            .bindTextureInArray (1, texHandle, texture_index)
            .end();
    }

    return texture_index;
}