#include "Renderer1.h"
#include "../gosShape/gosShapeImport.h"

using namespace gos;



//********************************
Renderer1::Renderer1()
{
    gpu = NULL;
}

//********************************
Renderer1::~Renderer1()
{
    if (NULL != gpu)
    {
        gpu->deleteResource (hPipeline);
        gpu->deleteResource (hVtxShader);
        gpu->deleteResource (hFragShader);


        gpu->deleteResource (descriptorMaterial.descr.instance);
        gpu->deleteResource (descriptorMaterial.descr.layout);
        gpu->deleteResource (descriptorMaterial.ssboHandle);

        materialList.unsetup();

        shapeList.unsetup();
        instanceList.unsetup();

        gpu = NULL;
    }
}

//********************************
bool Renderer1::setup (ThePipeline *thePipelineIN)
{
    thePipeline = thePipelineIN;
    localAllocator = thePipeline->localAllocator;
    gpu = thePipeline->gpu;
    if (!priv_setupVulkan())
        return false;

    materialList.setup (localAllocator, NUM_MAX_MATERIAL);
    shapeList.setup (localAllocator, 8192);
    instanceList.setup (localAllocator, 1024);

    return true;
}

//********************************
bool Renderer1::priv_setupVulkan()
{
    if (!priv_createDescriptorMaterial())
        return false;


    if (!priv_createPipeline())
    {
        gos::logger::err ("Renderer1::setup() => can't create pipeline\n");
        return false;
    }

    return true;
}

//********************************
bool Renderer1::priv_createDescriptorMaterial()
{
    //Creo il descriptorSet layout 2
    if (!gpu->descrSetLayout_create (&descriptorMaterial.descr.layout)
        .add_dynamicStorageBuffer (VK_SHADER_STAGE_FRAGMENT_BIT) //set 2, binding 0
        .end())
    {
        gos::logger::err ("Renderer1::priv_createDescriptorMaterial() => can't create descriptor set 2\n");
        return false;
    }       
    
    if (!thePipeline->createDescriptorInstance (&descriptorMaterial.descr))
    {
        gos::logger::err ("Renderer1::priv_createDescriptorMaterial() => can't create descriptorSet instance 2\n");
        return false;
    }
    
    //creo un buffer per SSBO
    if (!gpu->storageBuffer_create (SIZEOF_ONE_ELEMENT_IN_MATERIAL_SSBO * 1024, eVIBufferMode::shared_cpuW_manualSync, &descriptorMaterial.ssboHandle))
    {
        gos::logger::err ("Renderer1::priv_createDescriptorMaterial() => GPU::storageBuffer_create\n");
        return false;
    }

    //bind del buffer al descrittore
    gos::gpu::DescrSetInstanceWriter descrWriter;

    descrWriter.begin (gpu, descriptorMaterial.descr.instance)
        .bindDynamicStorageBuffer (0, descriptorMaterial.ssboHandle, SIZEOF_ONE_ELEMENT_IN_MATERIAL_SSBO)
        .end();

    return true;
}


//********************************
bool Renderer1::priv_createPipeline()
{
    //carico gli shader
    if (!gpu->vtxshader_createFromFile ("@shader/phong.vert.spv", "main", &hVtxShader))
    {
        gos::logger::err ("Renderer1::priv_createPipeline() => can't create vert shader\n");
        return false;
    }

    if (!gpu->fragshader_createFromFile ("@shader/phong.frag.spv", "main", &hFragShader))
    {
        gos::logger::err ("Renderer1::priv_createPipeline() => can't create frag shader\n");
        return false;
    }    

    //creo la pipeline
    thePipeline->createPipeline (&hPipeline)
        .addShader (hVtxShader)
        .addShader (hFragShader)
        .descriptor_add (descriptorMaterial.descr.layout)
        .pushConstant_add (VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(gos::mat4x4f), &pc_objWorldPos)
        //.setWireframe(true)
    .end ();
    if (hPipeline.isInvalid())
    {
        gos::logger::err ("Renderer1::priv_createPipeline() => can't create pipeline\n");
        return false;
    }

    return true;
}

//************************************
bool Renderer1::recordCommandBuffer (gpu::CmdBufferWriter &cw, gos::geom::Camera3 *cam)
{
    //aggiorno un materiale
    static u64 debug_nextTimeChangeColor_msec = 0;
    static f32 debug_red = 1.0f;
    static f32 debug_redInc = 0.01f;
    if (gos::getTimeSinceStart_msec() > debug_nextTimeChangeColor_msec)
    {
        const u32 INDEX_OF_MATERIAL_TO_CHANGE = 0;
        debug_nextTimeChangeColor_msec = gos::getTimeSinceStart_msec() + 10;
        Material *m;
        materialList.get (INDEX_OF_MATERIAL_TO_CHANGE, &m);
        m->colorDiffuse.y = debug_red;
        
        debug_red += debug_redInc;
        if (debug_red <0 || debug_red > 1.0f)
            debug_redInc = -debug_redInc;

        gpu::sMappedBuffer mapped;
        gpu->map (descriptorMaterial.ssboHandle, SIZEOF_ONE_ELEMENT_IN_MATERIAL_SSBO*INDEX_OF_MATERIAL_TO_CHANGE, sizeof(Material), &mapped);
        memcpy (mapped.host_pt, m, sizeof(Material));
        gpu->buffer_manualSync (&mapped, 1);
        gpu->buffer_unmap(mapped);

    }


    
    //rendering
    cw.bindPipeline (hPipeline)
        .renderPass_begin (thePipeline->hRenderLayout, thePipeline->hFrameBuffer)
            .bindDescriptorSet (thePipeline->descriptorBase_get()->instance, 0)
            .bindDescriptorSet (thePipeline->descriptorScene_get()->instance, 1);

    const u32 n = instanceList.getNElem();
    for (u32 i=0; i<n; i++)
    {
        gos::mat4x4f objW;
        instanceList(i).worldPos.getMatrix4x4(&objW);

        //const Material *material;
        //materialList.get (instanceList(i).indexOf_material, &material);
        const u16 materialIndex = instanceList(i).indexOf_material;


        const tpp::sBoundShapeInfo *shapeInfo;
        shapeList.get (instanceList(i).indexOf_shape, &shapeInfo);

        
        cw  .bindVtxBuffer(shapeInfo->hVtxBuffer)
            .bindIdxBufferU16(shapeInfo->hIdxBuffer)
            .bindDescriptorSet (descriptorMaterial.descr.instance, 2, materialIndex * SIZEOF_ONE_ELEMENT_IN_MATERIAL_SSBO)
            .pushConstant (pc_objWorldPos, &objW, sizeof(objW))
            .drawIndexed (shapeInfo->numIdx, 1, shapeInfo->startIdx, shapeInfo->startVtx, 0);
            
    }
    cw.renderPass_end();


    return true;
}


//************************************
bool Renderer1::material_create (const GPUTextureHandle &hDiffuseTex, const gos::vec3f &diffuseCol, u16 *out_index)
{
    assert (sizeof(Material) <= SIZEOF_ONE_ELEMENT_IN_MATERIAL_SSBO);

    Material material;
    material.colorDiffuse = vec4f(diffuseCol, 0);

    u16 indexOfTexture;
    thePipeline->decriptorBase_addTextureIfNotExitst(hDiffuseTex, &indexOfTexture);
    material.indexOf_texDiffuse = indexOfTexture;


    u16 index;
    if (!materialList.add (material, &index))
        return false;
    *out_index= index;

    
    //copio il materiale nell'array dei materiali
    const u32 offset = SIZEOF_ONE_ELEMENT_IN_MATERIAL_SSBO * index;
    gpu::sMappedBuffer map;
    gpu->map (descriptorMaterial.ssboHandle, offset, sizeof(Material), &map);
    memcpy (map.host_pt, &material, sizeof(Material));

    gpu->buffer_manualSync (&map, 1);
    gpu->buffer_unmap (map);
    
    return true;
}

//************************************
bool Renderer1::shape_add (const tpp::sBoundShapeInfo &shape, u16 *out_index)
{
    if (!shapeList.add (shape, out_index))
    {
        gos::logger::err ("Renderer1::shape_add () => can't add shape\n");
        return false;
    }

    return true;    
}

//************************************
bool Renderer1::instance_add (u16 indexOf_shape, u16 indexOf_material, const gos::geom::Pos3 &worldPos)
{
    const u32 n = instanceList.getNElem();
    instanceList[n].indexOf_material = indexOf_material;
    instanceList[n].indexOf_shape = indexOf_shape;
    instanceList[n].worldPos = worldPos;

   return true;
}
