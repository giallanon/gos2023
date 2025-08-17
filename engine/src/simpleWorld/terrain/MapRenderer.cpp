#include "MapRenderer.h"
#include "../gosShape/gosShapeVtxArrayWriter.h"

using namespace gos;

//********************************
MapRenderer::MapRenderer()
{
    gpu = NULL;
}

//********************************
MapRenderer::~MapRenderer()
{
    if (NULL != gpu)
    {
        gpu->deleteResource (descriptorPerInstance.descr.instance);
        gpu->deleteResource (descriptorPerInstance.descr.layout);
        gpu->deleteResource (descriptorPerInstance.ssboHandle);
        
        gpu->deleteResource (hPipeline);
        gpu->deleteResource (hVtxShader);
        gpu->deleteResource (hFragShader);

        thePipeline->unsetup();
        gpu = NULL;
    }

    if (localAllocator)
    {
        boundShapePerimetroList.unsetup();
        localAllocator = NULL;
    }
}

//********************************
bool MapRenderer::setup (ThePipeline *thePipelineIN, const char *mapFile)
{
    thePipeline = thePipelineIN;
    localAllocator = thePipeline->localAllocator;
    gpu = thePipeline->gpu;
    if (!priv_createPipeline())
        return false;

    boundShapePerimetroList.setup (localAllocator, 32);
    boundShapeFullQuad.numIdx = 0;

/*
#define HMAP_SET(x,y,val)   hMap[x +y*HMAP_DIMX] = val;
    const u32 HMAP_DIMX = 8;
    const u32 HMAP_DIMY = 8;
    u8 hMap[HMAP_DIMX * HMAP_DIMY];
    memset (hMap, 0, HMAP_DIMX * HMAP_DIMY);
    HMAP_SET(1,1,100); HMAP_SET(2,1,100); HMAP_SET(3,1,100); HMAP_SET(4,1,100); HMAP_SET(5,1,100);
    HMAP_SET(1,2,100); HMAP_SET(2,2,100); HMAP_SET(3,2,100); HMAP_SET(4,2,100); HMAP_SET(5,2,100);
    HMAP_SET(1,3,100); HMAP_SET(2,3,100); HMAP_SET(3,3,101); HMAP_SET(4,3,100); HMAP_SET(5,3,100);
    HMAP_SET(1,4,100); HMAP_SET(2,4,100); HMAP_SET(3,4,100); HMAP_SET(4,4,100); HMAP_SET(5,4,100);
    HMAP_SET(1,5,100); HMAP_SET(2,5,100); HMAP_SET(3,5,100); HMAP_SET(4,5,100); HMAP_SET(5,5,100);
*/


    //carico la mappa
    const f32 SPESSORE = 0.5f;
    const f32 WORLD_START_Y = 0.0f;
    TheMap map;
    
    //if (!map.loadFromSingleChannellHeightmap(hMap, HMAP_DIMX, HMAP_DIMY, 100, 2, WORLD_START_Y, SPESSORE))
    if (!map.loadTGA(mapFile, 64, 6, WORLD_START_Y, SPESSORE))
    {
        gos::logger::err ("MapRenderer::setup() => can't load map file\n");
        return false;
    }


    MarchingSquare::VertexList3 tempVtxList (gos::getScrapAllocator(), map.getDimX()*map.getDimY());
    gos::FastArray<u16> tempIdxList (gos::getScrapAllocator(), map.getDimX()*map.getDimY() * 3);

    //full quad mesh
    {
        MarchingSquare::buildFullQuadMesh (SPESSORE, tempVtxList, tempIdxList);

        gos::Shape shape;
        priv_createAShape (tempVtxList, tempIdxList, &shape);

        //bindo a vb/ib
        if (!thePipeline->shape_uploadToVBIB (&shape, &boundShapeFullQuad))
            gos::logger::err ("MapRenderer::setup() => can't bind shape to VB(IB\n");

        gos::shape::shapeFree (gos::getScrapAllocator(), &shape);    
    }



    //preparo le shape
    for (u8 i=0;i<map.getNumLayer(); i++)
    {
        TheMap::LayerView view;
        map.queryLayer (i, &view);
        priv_buildALevel (view, tempVtxList, tempIdxList);
    }

    return true;
}

//********************************
void MapRenderer::priv_buildALevel (TheMap::LayerView &mapView, MarchingSquare::VertexList3 &tempVtxList, gos::FastArray<u16> &tempIdxList)
{
    const u32 shapeNum = boundShapePerimetroList.getNElem();
    assert (shapeNum < PER_INSTANCE_SSBO__NUM_MAX_ELEM);


    MarchingSquare msq;
    msq.run (gos::getScrapAllocator(), mapView);
    if (0 == msq.getNumPerimetri())
        return;
    msq.buildMesh (mapView.getSpessore(), tempVtxList, tempIdxList);

    gos::Shape shape;
    priv_createAShape (tempVtxList, tempIdxList, &shape);
    
    //bindo a vb/ib
    if (!thePipeline->shape_uploadToVBIB (&shape, &boundShapePerimetroList[shapeNum].bondShape))
        gos::logger::err ("MapRenderer::setup() => can't bind shape to VB(IB\n");
    gos::shape::shapeFree (gos::getScrapAllocator(), &shape);    


    //nell'instance buffer ci metto 1 record per il perimetro e N record per i quad pieni
    u32 index = 0;
    if (0 != shapeNum)
    {
        index = boundShapePerimetroList(shapeNum-1).indexStartInPerInstanceArray;
        index += 1;
        index += boundShapePerimetroList(shapeNum-1).numFullQuad;
    }
    boundShapePerimetroList[shapeNum].numFullQuad = msq.getNumQuadPieni();
    boundShapePerimetroList[shapeNum].indexStartInPerInstanceArray = index;


    //colore del layer
    gos::ColorHDR col (mapView.getColorRGBA());
    col.sRGBToLinear();
    


    //preparare l'instance buffer
    const f32 WORLD_SCALE = 1.0f;
    const f32 WORLD_AO = 1.0f;
    const u32 numInstanceData = 1+ msq.getNumQuadPieni();
    sPerInstanceData *perInstanceData = GOSALLOCT(sPerInstanceData*, gos::getScrapAllocator(), sizeof(sPerInstanceData) * numInstanceData);
    perInstanceData[0].worldPosAndScale.set (0, mapView.getWorldY(), 0,  WORLD_SCALE);
    perInstanceData[0].colorAndAO.set (col.col.r, col.col.g, col.col.b, WORLD_AO);


    vec3f worldTopLeft;
    worldTopLeft.x = 0;
    worldTopLeft.y = 0;
    worldTopLeft.z = (mapView.getDimY() -1)* WORLD_SCALE;

    for (u32 i=0; i<msq.getNumQuadPieni(); i++)
    {
        const gos::vec2u16 p = msq.getPosQuadPieno(i);
        const f32 AO = mapView.calcAO (p.x, p.y);
        perInstanceData[i+1].colorAndAO.set (col.col.r, col.col.g, col.col.b,  AO);

        vec3f pos = worldTopLeft;
        pos.x =  WORLD_SCALE * (p.x);
        pos.y = mapView.getWorldY();
        pos.z -= WORLD_SCALE * (p.y+1);
        perInstanceData[i+1].worldPosAndScale.set (pos.x, pos.y, pos.z, WORLD_SCALE);
    }

    gpu->writeAndSync (descriptorPerInstance.ssboHandle, index*sizeof(sPerInstanceData), perInstanceData, numInstanceData * sizeof(sPerInstanceData));
    GOSFREE(gos::getScrapAllocator(), perInstanceData);
}

//********************************
void MapRenderer::priv_createAShape (MarchingSquare::VertexList3 &tempVtxList, gos::FastArray<u16> &tempIdxList, gos::Shape *out_shape) const
{
    out_shape->reset();
    gos::shape::shapeAlloc (gos::getScrapAllocator(), thePipeline->vtxLayout, tempVtxList.getNElem(), tempIdxList.getNElem(), out_shape);

    //fillo i vtx
    gos::shape::VtxArrayWriter writer;
    gos::shape::VtxArrayWriter::Elem<vec3f> vtx;
    gos::shape::VtxArrayWriter::Elem<vec3f> norm;
    gos::shape::VtxArrayWriter::Elem<vec2f> tex;

    writer.setup (out_shape);
    writer.getPos3 (&vtx);
    writer.getNorm3 (&norm);
    writer.getTexCoord (&tex, 0);

    for (u32 i=0; i<tempVtxList.getNElem(); i++)
    {
        vtx() = tempVtxList(i).pos;
        vtx.next();

        norm() = tempVtxList(i).norm;
        norm.next();

        tex().set (0,0);
        tex.next();
    }

    //fillo idx
    memcpy (out_shape->idxBuffer, tempIdxList._queryPointer(), sizeof(u16) * tempIdxList.getNElem());
}

//********************************
bool MapRenderer::priv_createDescriptorPerInstance()
{
    //Creo il descriptorSet layout 2
    if (!gpu->descrSetLayout_create (&descriptorPerInstance.descr.layout)
        .add_storageBuffer (VK_SHADER_STAGE_VERTEX_BIT) //set 2, binding 0
        .end())
    {
        gos::logger::err ("MapRenderer::priv_createDescriptorPerInstance() => can't create descriptor set 2\n");
        return false;
    }       
    
    if (!thePipeline->createDescriptorInstance (&descriptorPerInstance.descr))
    {
        gos::logger::err ("MapRenderer::priv_createDescriptorPerInstance() => can't create descriptorSet instance 2\n");
        return false;
    }
    
    //creo un buffer per SSBO
    if (!gpu->storageBuffer_create (PER_INSTANCE_SSBO__SIZEOF_ONE_ELEMENT * PER_INSTANCE_SSBO__NUM_MAX_ELEM, eVIBufferMode::shared_cpuW_autoSync, &descriptorPerInstance.ssboHandle))
    {
        gos::logger::err ("MapRenderer::priv_createDescriptorPerInstance() => GPU::storageBuffer_create\n");
        return false;
    }

    //bind del buffer al descrittore
    gos::gpu::DescrSetInstanceWriter descrWriter;

    descrWriter.begin (gpu, descriptorPerInstance.descr.instance)
        //.bindDynamicStorageBuffer (0, descriptorPerInstance.ssboHandle, PER_INSTANCE_SSBO__SIZEOF_ONE_ELEMENT)
        .bindStorageBuffer (0, descriptorPerInstance.ssboHandle)
        .end();

    return true;
}

//********************************
bool MapRenderer::priv_createPipeline()
{
    if (!priv_createDescriptorPerInstance())
        return false;

    //carico gli shader
    if (!gpu->vtxshader_createFromFile ("@shader/mapRenderer.vert.spv", "main", &hVtxShader))
    {
        gos::logger::err ("MapRenderer::priv_createPipeline() => can't create vert shader\n");
        return false;
    }

    if (!gpu->fragshader_createFromFile ("@shader/mapRenderer.frag.spv", "main", &hFragShader))
    {
        gos::logger::err ("MapRenderer::priv_createPipeline() => can't create frag shader\n");
        return false;
    }    

    //creo la pipeline
    //gpu->pipeline_createNew (thePipeline->hRenderLayoutClearBuffer, &hPipeline)
    thePipeline->createPipeline (&hPipeline)
        .addShader (hVtxShader)
        .addShader (hFragShader)
        .descriptor_add (descriptorPerInstance.descr.layout)
    .end ();
    if (hPipeline.isInvalid())
    {
        gos::logger::err ("MapRenderer::priv_createPipeline() => can't create pipeline\n");
        return false;
    }

    return true;
}

//************************************
bool MapRenderer::recordCommandBuffer (gpu::CmdBufferWriter &cw, gos::geom::Camera3 *cam)
{
    //rendering
    cw.bindPipeline (hPipeline)
        .renderPass_begin (thePipeline->hRenderLayout, thePipeline->hFrameBuffer)
            .bindDescriptorSet (thePipeline->descriptorBase_get()->instance, 0)
            .bindDescriptorSet (thePipeline->descriptorScene_get()->instance, 1)
            .bindDescriptorSet (descriptorPerInstance.descr.instance, 2);


    //perimetro
    u32 instanceIndex = 0;
    for (u32 i=0; i<boundShapePerimetroList.getNElem(); i++)
    {
        const sInfoPerLevel *info = &boundShapePerimetroList(i);
        cw  .bindVtxBuffer(info->bondShape.hVtxBuffer)
            .bindIdxBufferU16(info->bondShape.hIdxBuffer);


        cw.drawIndexed (info->bondShape.numIdx, 1, info->bondShape.startIdx, info->bondShape.startVtx, instanceIndex);
        ++instanceIndex;

        if (0 != info->numFullQuad)
        {
            cw.drawIndexed (boundShapeFullQuad.numIdx, info->numFullQuad, boundShapeFullQuad.startIdx, boundShapeFullQuad.startVtx, instanceIndex);
            instanceIndex += info->numFullQuad;
        }
    }       
    cw.renderPass_end();


    return true;
}

