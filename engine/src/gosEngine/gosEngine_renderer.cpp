#include "gosEngine_renderer.h"
#include "gosEngine.h"

using namespace gos;
using namespace gos::engine;


//**********************************
Renderer::Renderer()
{
    engine = NULL;
}

//**********************************
void Renderer::priv_free()
{
    if (NULL != gpu)
    {
        gpu->deleteResource(handle_zbuffer);
        gpu->deleteResource(handle_rt0);
        //gpu->deleteResource(handle_samplers[0]);
        //gpu->deleteResource(handle_samplers[1]);
        gpu->deleteResource(handle_ubo_scene);
        gpu->deleteResource(handle_sbo_materialList);
        gpu->deleteResource(handle_descrSet0);
        gpu->deleteResource(handle_descrSet1);
        gpu->deleteResource(handle_descrSet2);
        gpu->deleteResource(handle_descrPool);
    }

    gpu = NULL;
    engine = NULL;
}

//**********************************
bool Renderer::setup (gos::Engine *engineIN)
{
    engine = engineIN;
    gpu = engine->gpu;

    //load degli assets
    if (!engine->assetHub->getHandle ("GOSENG_pipe_phong", &assHandle_pipe, true))
        return false;
    if (!engine->assetHub->getHandle ("GOSENG_tex_checker", &assHandle_tex_checker, true))
        return false;
    
    //risorse di rendering
    {
        //rt0
        if (!gpu->renderTarget_create ("0-", "0-", eImageFormat::U8_RGBA, &handle_rt0))
            return false;

        //zbuffer
        if (!gpu->zbuffer_create ("0-", "0-", eImageFormat::_DEPTH_BEST, &handle_zbuffer))
        {
            gos::logger::err ("Renderer::setup() => GPU::zbuffer_create\n");
            return false;
        }

        //creo un descriptor pool
        gpu->descrPool_createNew (&handle_descrPool)
            .setMaxNumDescriptorSet(4)
            .addPool_uniformBuffer(1)
            .addPool_storageBuffer(1)
            .addPool_sampler(2)
            .addPool_texture(1)
            .end();
        if (handle_descrPool.isInvalid())
        {
            gos::logger::err ("Renderer::setup() => can't create descriptor pool\n");
            return false;
        }
    }


    //attendo che la pipe sia stata caricata perche' mi servono le definizioni dei descrittori
    const asset::Asset_pipe *pipe;
    engine->assetHub->getAssetWithTimeout (assHandle_pipe, 5000, &pipe);
    {
        const asset::Asset_tex2D *tex2d_checker;
        engine->assetHub->getAssetWithTimeout (assHandle_tex_checker, 5000, &tex2d_checker);

        //alloco una istanza dei descriptor-set
        gos::gpu::DescrSetInstanceWriter dsw;

        //descriptor set 0
        if (!gpu->descrSetInstance_create (handle_descrPool, pipe->handle_pipe, 0, &handle_descrSet0))
        {
            gos::logger::err ("Renderer::setup() => can't create an instance of descriptorSet_0\n");
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
                .bindTextureInArray  (1, tex2d_checker->handle_texture, 0)  //bindo in textureList[0] la texture "checker"
                .end();
        }
        

        //descriptor set 1
        if (!gpu->descrSetInstance_create (handle_descrPool, pipe->handle_pipe, 1, &handle_descrSet1))
        {
            gos::logger::err ("Renderer::setup() => can't create an instance of descriptorSet_0\n");
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
            gos::logger::err ("Renderer::setup() => can't create an instance of descriptorSet_0\n");
            return false;
        }
        else
        {
    
            gpu->storageBuffer_create (SIZEOF_ONE_ELEMENT_IN_MATERIAL_SSBO * NUM_MAX_MATERIAL, eMemAccessMode::shared_cpuW_manualSync, &handle_sbo_materialList);
            dsw.begin (gpu, handle_descrSet2)
                .bindDynamicStorageBuffer (0, handle_sbo_materialList, SIZEOF_ONE_ELEMENT_IN_MATERIAL_SSBO)
                .end();
        }
    }


    return true;
}

//**********************************
void Renderer::begin (gos::geom::Camera3 *cam)
{}

//**********************************
void Renderer::add (const ENGShape shape, const ENGMatrixW worldPos)
{}

//**********************************
void Renderer::end()
{}
