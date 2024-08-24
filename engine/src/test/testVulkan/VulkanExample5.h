#ifndef _VulkanExample5_h_
#define _VulkanExample5_h_
#include "VulkanApp.h"


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
    void        virtual_onInputEvent (u32 actionID, i16 value);

private:
    struct sPerInstanceData
    {
        gos::vec3f  pos;
        gos::vec3f  color;

        void set (f32 x, f32 y, f32 z, f32 r, f32 g, f32 b)     { pos.set(x,y,z); color.set(r,g,b); }
    };    

   /*************************************
     * Line
     */
    class Line
    {
    public:
                Line ();
                ~Line();

        bool    setup(gos::GPU *gpu, GPUDescrPoolHandle &descrPoolHandle);
        void    begin();
        void    setColor (const gos::vec3f &color);
        void    addLine (const gos::vec3f &p1, const gos::vec3f &p2);
        void    end();

        bool    recordCommandBuffer (gos::gpu::CmdBufferWriter &cw, GPUStgBufferHandle hStgBuffer, gos::geom::Camera3 &cam);

    private:
        struct sVertex
        {
            gos::vec3f  pos;
            gos::vec3f  col;
        };

        struct sUniformBufferObject 
        {
            gos::mat4x4f camView;
            gos::mat4x4f camProj;
        };

    private:
        gos::GPU                    *gpu;
        gos::Allocator              *localAllocator;
        gos::FastArray<sVertex>     list;
        bool                        bNeedUpdate;
        gos::vec3f                  curColor;
        sUniformBufferObject        ubo;

        GPUVtxBufferHandle      hVtxBuffer;
        GPUShaderHandle         hVtxShader;
        GPUShaderHandle         hFragShader;
        GPUPipelineHandle       hPipeline;
        GPUUniformBufferHandle  hUBO;
        GPUDescrSetLayoutHandle hDescrSetLayout;
        GPUDescrSetInstanceHandle hDescrSetInstance;
        GPURenderLayoutHandle   hRenderLayout;
        GPUFrameBufferHandle    hFrameBuffer;
    };

    /**********************************
     * world
     */
    class World
    {
    public:
                World (gos::GPU *gpu);
                ~World();

        void    setup (u32 gridSizeX, u32 gridSizeY);
        void    set (u32 x, u32 y, bool b=true);
        void    toggle (u32 x, u32 y);
        void    set (const gos::vec3f &p, bool b);
        void    updateInstanceVB (GPUStgBufferHandle hStgBuffer);
        
        void    computeAndDrawPerimeter (Line &line);

        u32     getNumInstances() const                                     { return dimx*dimy; }
        bool    needUpdate() const                                          { return bNeedUpdate; }
        u8      get (u32 x, u32 y) const                                    { assert (x<dimx && y<dimy); return map[y*dimx+x]; }

    public:
        GPUVtxBufferHandle  hVBInstance;
        
    private:
        static constexpr f32 SPACE = 1.0f;
    private:
        void    priv_freeMap();

    private:
        gos::Allocator      *localAllocator;
        gos::GPU            *gpu;
        u8                  *map;
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

private:
    void        priv_doCPUStuff();
    void        priv_mainLoop();
    bool        priv_recordCommandBuffer (gos::gpu::CmdBufferWriter &cw);
    void        priv_setSphere (const gos::vec2f &mouseXY, bool b);

private:
    static const u32     NUM_MAX_VERTEX = 1024;
    static const u32     NUM_MAX_INDEX =  4096;
    static const u32     NUM_INSTANCES = 2;

private:
    gos::geom::Camera3      cam;
    FreeMovement            movement;
    Vertex                  vertexList[NUM_MAX_VERTEX];
    u16                     indexList[NUM_MAX_INDEX];
    gos::shape::Info        sphereInfo;
    World                   *world;
    Line                    *line;

    sUniformBufferObject    ubo;

    GPUVtxBufferHandle      vtxBufferHandle;
    GPUIdxBufferHandle      idxBufferHandle;
    GPUStgBufferHandle      stgBufferHandle;

    GPUPipelineHandle       pipelineHandle;
    GPUShaderHandle         vtxShaderHandle;
    GPUShaderHandle         fragShaderHandle;
    GPURenderLayoutHandle   renderLayoutHandle;
    GPUFrameBufferHandle    frameBufferHandle;

    GPUDescrPoolHandle      descrPoolHandle;
    GPUDescrSetLayoutHandle descrSetLayoutHandle;
    GPUDescrSetInstanceHandle descrSetInstancerHandle;
    GPUUniformBufferHandle  uboHandle;
};


#endif //_VulkanExample5_h_