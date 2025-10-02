#ifndef _VulkanExample5_h_
#define _VulkanExample5_h_
#include "VulkanApp.h"
#include "VkEx5MarchingSquare.h"
#include "MarchingSquare.h"
#include "SimpleLineRenderer.h"

/************************************
 *  VulkanExample5
 */
class VulkanExample5 : public VulkanApp
{
public:
    
                VulkanExample5();

protected:
    bool        virtual_onInit ();
    void        virtual_explain();
    void        virtual_onRun();
    void        virtual_onCleanup();
    void        virtual_onInputEvent (u32 actionID, i16 value, const gos::input::MouseStatus &mouseStatus, const gos::input::sButtonModifier &btnModifier);

private:
    struct sPerInstanceData
    {
        gos::vec3f  pos;
        gos::vec3f  color;
        gos::vec3f  scale;

        void set (f32 x, f32 y, f32 z, f32 r, f32 g, f32 b)     { pos.set(x,y,z); color.set(r,g,b); }
    };    

    /**********************************
     * world
     */
    class World : public VkEx5MarchingSquare::Map
    {
    public:
                World (gos::GPU *gpu);
                ~World();

        void    setup (u32 gridSizeX, u32 gridSizeY);
        void    set_ON_OFF (u32 x, u32 y, bool b=true);
        void    toggle (u32 x, u32 y);
        void    set_scaleX (u32 x, u32 y, f32 sx);
        void    set_scaleZ (u32 x, u32 y, f32 sz);
        void    set_scaleXZ (u32 x, u32 y, f32 sx, f32 sz);
        void    inc_scaleX (u32 x, u32 y);
        void    dec_scaleX (u32 x, u32 y);
        void    inc_scaleZ (u32 x, u32 y);
        void    dec_scaleZ (u32 x, u32 y);
        void    updateInstanceVB (GPUStgBufferHandle hStgBuffer);

        bool    mouseToGrid (const gos::GPU *gpu, gos::geom::Camera3 &cam, i16 mx, i16 my, u16 *out_x, u16 *out_y) const;
        u32     getNumInstances() const                                     { return dimx*dimy; }
        bool    needUpdate() const                                          { return bNeedUpdate; }
        bool    isON (u32 x, u32 y) const                                   { assert (x<dimx && y<dimy); return map2[y*dimx+x].bIsON; }
        u32     getDimX() const                                             { return dimx; }
        u32     getDimY() const                                             { return dimy; }
        f32     getScaleX(u32 x, u32 y) const                               { assert (x<dimx && y<dimy); return map2[y*dimx+x].sx; }
        f32     getScaleZ(u32 x, u32 y) const                               { assert (x<dimx && y<dimy); return map2[y*dimx+x].sz; }
        gos::vec3f    getPos3D(u32 x, u32 y) const;

    public:
        GPUVtxBufferHandle  hVBInstance;
        
    public:
        static constexpr f32 SPACE = 1.0f;

    private:
        struct sMapElem
        {
            bool    bIsON;
            f32     sx;
            f32     sz;
        };

    private:
        void    priv_freeMap();

    private:
        gos::Allocator      *localAllocator;
        gos::GPU            *gpu;
        sMapElem            *map2;
        u32                 dimx;
        u32                 dimy;
        bool                bNeedUpdate;
    };

 
private:
    struct Vertex 
    {
        gos::vec3f  pos;
        gos::vec3f  norm;
    };


    struct sUniformBufferObject 
    {
        //glm::mat4 world;
        gos::mat4x4f camView;
        gos::mat4x4f camProj;
        gos::vec4f  lightDir;
    };

    struct sMSQ2
    {
        GPUVtxBufferHandle      vtxBufferHandle;
        GPUIdxBufferHandle      idxBufferHandle;
        u32                     numVtx;
        u32                     numIdx;
    };

private:
    void        priv_doCPUStuff();
    bool        recordCommandBuffer (GPUCmdBufferHandle &cmdBufferHandle, gos::gpu::SwapchainImg &swapChainImage);
    void        priv_setSphere_ON_OFF (i16 mouseX, i16 mouseY, bool b);
    void        priv_drawGrid ();
    void        priv_runMarchingSquare();
    void        priv_createSfera();
    bool        priv_loadSfera();
    void        priv_freeMSQ2();

private:
    static const u32     NUM_MAX_VERTEX = 1024;
    static const u32     NUM_MAX_INDEX =  4096;
    static const u32     NUM_INSTANCES = 2;

private:
    gos::geom::Camera3      cam;
    gos::FreeMovement       movement;
    gos::Shape              myShape;
    World                   *world;
    SimpleLineRenderer      *line;

    sUniformBufferObject    ubo;

    GPUVtxBufferHandle      vtxBufferHandle;
    GPUIdxBufferHandle      idxBufferHandle;
    GPUStgBufferHandle      stgBufferHandle;

    GPUZBufferHandle       zbufferHandle;
    GPUPipelineHandle           pipelineHandle;
    GPUShaderHandle             vtxShaderHandle;
    GPUShaderHandle             fragShaderHandle;

    GPUDescrPoolHandle          descrPoolHandle;
    GPUDescrSetInstanceHandle   descrSetInstancerHandle;
    GPUUniformBufferHandle      uboHandle;


    sMSQ2                   gpuMSQ2;

};


#endif //_VulkanExample5_h_