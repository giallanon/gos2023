#include "VulkanExample5.h"
#include "../gosGeom/gosGeomCamera3.h"


using namespace gos;

//*******************************************************
VulkanExample5::World::World (gos::GPU *gpuIN)
{
    localAllocator = gos::getSysHeapAllocator();
    gpu = gpuIN;
    map2 = NULL;
    dimx = dimy = 0;
}

VulkanExample5::World::~World()
{
    priv_freeMap();
    gpu->deleteResource (hVBInstance);
}

void VulkanExample5::World::priv_freeMap()
{
    if (NULL != map2)
    {
        GOSFREE(localAllocator, map2);
        map2 = NULL;
        dimx = dimy = 0;
    }
}

void VulkanExample5::World::setup (u32 gridSizeX, u32 gridSizeY)
{
    priv_freeMap();
    dimx = gridSizeX;
    dimy = gridSizeY;
    map2 = GOSALLOCT(sMapElem*, localAllocator, dimx*dimy * sizeof(sMapElem));
    
    for (u32 ct=0; ct<dimx*dimy; ct++)
    {
        map2[ct].bIsON = false;
        map2[ct].sx = 0.2f + gos::random01() * 0.6f;
        map2[ct].sz = 0.2f + gos::random01() * 0.6f;
    }
}

void VulkanExample5::World::set_scaleX (u32 x, u32 y, f32 sx)
{
    if (x >= dimx || y >= dimy)
        return;

    bNeedUpdate = true;
    const u32 ct = y*dimy+x;    
    map2[ct].sx = sx;
}

void VulkanExample5::World::set_scaleZ (u32 x, u32 y, f32 sz)
{
    if (x >= dimx || y >= dimy)
        return;

    bNeedUpdate = true;
    const u32 ct = y*dimy+x;    
    map2[ct].sz = sz;
}

void VulkanExample5::World::set_scaleXZ (u32 x, u32 y, f32 sx, f32 sz)
{
    if (x >= dimx || y >= dimy)
        return;

    bNeedUpdate = true;
    const u32 ct = y*dimy+x;    
    map2[ct].sx = sx;
    map2[ct].sz = sz;
}

void VulkanExample5::World::inc_scaleX (u32 x, u32 y)
{
    if (x >= dimx || y >= dimy)
        return;

    bNeedUpdate = true;
    const u32 ct = y*dimy+x;
    if (map2[ct].sx < 0.48)
        map2[ct].sx += 0.1f;

}

void VulkanExample5::World::dec_scaleX (u32 x, u32 y)
{
    if (x >= dimx || y >= dimy)
        return;

    bNeedUpdate = true;
    const u32 ct = y*dimy+x;
    if (map2[ct].sx > 0.12)
        map2[ct].sx -= 0.1f;
}

void VulkanExample5::World::inc_scaleZ (u32 x, u32 y)
{
    if (x >= dimx || y >= dimy)
        return;

    bNeedUpdate = true;
    const u32 ct = y*dimy+x;
    if (map2[ct].sz < 0.5)
        map2[ct].sz += 0.1f;
}

void VulkanExample5::World::dec_scaleZ (u32 x, u32 y)
{
    if (x >= dimx || y >= dimy)
        return;

    bNeedUpdate = true;
    const u32 ct = y*dimy+x;
    if (map2[ct].sz > 0.1)
        map2[ct].sz -= 0.1f;
}

void VulkanExample5::World::set_ON_OFF (u32 x, u32 y, bool b)
{
    if (x >= dimx || y >= dimy)
        return;

    bNeedUpdate = true;
    const u32 ct = y*dimy+x;
    if (b)
        map2[ct].bIsON = true;
    else
        map2[ct].bIsON = false;
}

void VulkanExample5::World::toggle (u32 x, u32 y)
{
    if (x >= dimx || y >= dimy)
        return;

    bNeedUpdate = true;
    const u32 ct = y*dimx+x;
    if (map2[ct].bIsON)
        map2[ct].bIsON = false;
    else
        map2[ct].bIsON = true;
}

bool VulkanExample5::World::mouseToGrid (const GPU *gpu, gos::geom::Camera3 &cam, i16 mx, i16 my, u16 *out_x, u16 *out_y) const
{
    const gos::vec2f mouseXY(mx, my);
    gos::vec3f dir;
    cam.unproject (gpu->swapChain_getWidth(), gpu->swapChain_getHeight(), &mouseXY, &dir, 1);

    //cam.o.z + dir.z * t = 0
    const f32 t = -cam.pos.o.y / dir.y;
    const gos::vec3f p = cam.pos.o + dir*t;
    //printf ("POINT (%d,%d) to 3d: %.2f %.2f %.2f\n", inputMap.resolve_getMouseX(), inputMap.resolve_getMouseY(), pp.x, pp.y, pp.z);


    const i16 x = (i16) (floorf(p.x + 0.5f) + (f32)dimx/2);
    const i16 y = (i16) ((f32)dimy/2 - floorf(p.z + 0.5f));
    if (x >=0 && x < (i16)dimx)
    {
        *out_x = (u16)x;
        *out_y = (u16)y;
        return true;
    }
    return false;
}

gos::vec3f VulkanExample5::World::getPos3D (u32 x, u32 y) const
{
    gos::vec3f v ( 
        ((i32)x - (i32)(dimx/2)) * SPACE,
        0,
        ((i32)(dimy/2) - (i32)y)* SPACE);
    return v;
}

void VulkanExample5::World::updateInstanceVB (GPUStgBufferHandle hStgBuffer)
{
    if (!bNeedUpdate)
        return;
    bNeedUpdate = false;
    gpu->deleteResource (hVBInstance);

    const u32 numInstances = dimx*dimy;
    //vtx buffer (stream 1)
    if (!gpu->vertexBuffer_create (sizeof(sPerInstanceData) * numInstances, eVIBufferMode::onGPU, &hVBInstance))
    {
        gos::logger::err ("VulkanExample5::World::updateInstanceVB() => gpu->vertexBuffer_create() failed\n");
        return;
    }    

    sPerInstanceData *perInstanceData = GOSALLOCT(sPerInstanceData*, gos::getScrapAllocator(), sizeof(sPerInstanceData) * numInstances);
    u32 ct = 0;
    const f32 startX = -((dimx/2) * SPACE);
    f32 zz = (dimy/2) * SPACE;

    for (u32 y=0; y<dimy; y++)
    {
        f32 xx = startX;
        for (u32 x=0; x<dimx; x++)
        {
            perInstanceData[ct].pos.set (xx, 0, zz);
            if (map2[ct].bIsON)
                perInstanceData[ct].color.set (0,1,0);
            else
                perInstanceData[ct].color.set (1,0,0);

            perInstanceData[ct].scale.set (map2[ct].sx, 0.1f, map2[ct].sz);
            ct++;
            xx += SPACE;
        }
        zz -= SPACE;
    }

    if (!gpu->stagingBuffer_uploadToGPUBuffer (hStgBuffer, perInstanceData, hVBInstance, 0, sizeof(sPerInstanceData) * numInstances))
    {
        gos::logger::err ("VulkanExample5::World::updateInstanceVB() => can't upload to VtxBuffer\n");
    }

    GOSFREE(gos::getScrapAllocator(), perInstanceData);
}

