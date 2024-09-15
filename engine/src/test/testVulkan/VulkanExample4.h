#ifndef _VulkanExample4_h_
#define _VulkanExample4_h_
#include "VulkanApp.h"


/************************************
 *  VulkanExample4
 */
class VulkanExample4 : public VulkanApp
{
public:
    
                VulkanExample4();

    bool        virtual_onInit ();
    void        virtual_explain();
    void        virtual_onRun();
    void        virtual_onCleanup();

private:
    struct Vertex 
    {
        gos::vec3f  pos;
        gos::vec3f  colorRGB;
        gos::vec3f  normal;
        gos::vec2f  tutv0;

        void set (f32 x, f32 y, f32 z, f32 r, f32 g, f32 b)    { pos.set(x,y,z); colorRGB.set(r,g,b); }
    };

    struct sUniformBufferObject 
    {
        //glm::mat4 world;
        gos::mat4x4f    camView;
        gos::mat4x4f    camProj;
        gos::vec4f      lightDir;
        gos::mat4x4f    objWorld;
    };


    struct sAnimation
    {
        u64 nextTimeRotate_msec;
        f32 rotation_grad;
        f32 zPos;
        f32 zInc;

        void reset () { nextTimeRotate_msec = 0; rotation_grad = 0; zPos = 0; zInc = 0.1f; }
    };

private:
    bool        createVertexIndexStageBuffer();
    bool        recordCommandBuffer (GPUCmdBufferHandle &cmdBufferHandle);
    void        doCPUStuff ();
    void        mainLoop();
    void        virtual_onInputEvent (u32 event32, i16 value, const gos::input::MouseStatus &mouseStatus, const gos::input::sButtonModifier &btnModifier);

private:
    gos::shape::Shape       myShape;
    sUniformBufferObject    ubo;
    sAnimation              anim;
    gos::geom::Camera3      cam;
    gos::FPSMovement        movement;


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
    GPUTextureHandle        texHandle;
    GPUSamplerHandle        samplerHandle;
};


#endif //_VulkanExample4_h_