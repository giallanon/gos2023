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
    void        virtual_onInputEvent (u32 actionID, i16 value, const gos::input::MouseStatus &mouseStatus, const gos::input::sButtonModifier &btnModifier);

private:
    struct sPerInstanceData
    {
        gos::vec3f  pos;
        gos::vec3f  color;
        gos::vec3f  scale;

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

        u16     addVtx (const gos::vec3f &p);
        void    line (u16 v0, u16 v1);

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
        gos::FastArray<sVertex>     vtxList;
        gos::FastArray<u16>         idxList;
        bool                        bNeedUpdate;
        gos::vec3f                  curColor;
        sUniformBufferObject        ubo;

        GPUVtxBufferHandle      hVtxBuffer;
        GPUIdxBufferHandle      hIdxBuffer;
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


    /**********************************
     * MarchingSquare
     */
    class MarchingSquare
    {
    public:
        static u8      computeSquareMask (const World &world, u32 x, u32 y);    

    public:
        enum class ePos : u8
        {
            a = 0,
            b = 1,
            c = 2,
            d = 3
        };

        struct sEdge
        {
            u16 i0;
            u16 i1;
        };

    public:
        void    algo1 (const World &world, Line &line);
        void    algo2 (const World &world, Line &line);
        void    algo3 (const World &world, Line &line);

    private:
        class VtxHelper2
        {

        public:
                    VtxHelper2 (gos::Allocator *allocator, const World &world);
                    ~VtxHelper2 ();

            sEdge   addEdge (u16 worldX, u16 worldY, ePos pos1, ePos pos2);
        
        public:
            gos::FastArray<gos::vec3f>  vtxList;
            gos::FastArray<sEdge>       edgeList;

        private:
            u16     priv_vtxAddIfNeeded (u16 worldX, u16 worldY, ePos pos);
            u32     priv_calc (u16 worldX, u16 worldY, ePos pos, gos:: vec3f *out_v) const;
            
        private:
            gos::Allocator *localAllocator;
            u32     worldDimX;
            u32     worldDimY;
            f32     *coordX;
            f32     *coordZ;
            u16     *existingVtx;
            

        };

    private:
        void    priv_renderLine (Line &line, gos::FastArray<sEdge> &edgeList) const;
        void    priv_moveEdge (gos::FastArray<sEdge> *from, gos::FastArray<sEdge> *to, u32 i) const;
        u32     priv_findEdgeWithVtx (const gos::FastArray<sEdge> *list, const sEdge *edge) const;
        void    priv_coloredLine (Line &line, const VtxHelper2 &helper, gos::FastArray<sEdge> &edgeList) const;
        void    priv_smoothLine (Line &line, const VtxHelper2 &helper, gos::FastArray<sEdge> &edgeList) const;
           
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
    void        priv_setSphere_ON_OFF (i16 mouseX, i16 mouseY, bool b);
    void        priv_drawGrid ();
    void        priv_runMarchingSquare();
    void        priv_createSfera();
    bool        priv_loadSfera();

private:
    static const u32     NUM_MAX_VERTEX = 1024;
    static const u32     NUM_MAX_INDEX =  4096;
    static const u32     NUM_INSTANCES = 2;

private:
    gos::geom::Camera3      cam;
    gos::FreeMovement       movement;
    gos::Shape              myShape;
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