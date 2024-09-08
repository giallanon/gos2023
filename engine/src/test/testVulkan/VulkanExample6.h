#ifndef _VulkanExample6_h_
#define _VulkanExample6_h_
#include "VulkanApp.h"

/************************************
 *  VulkanExample6
 */
class VulkanExample6 : public VulkanApp
{
public:
    
                VulkanExample6();

    bool        virtual_onInit ();
    void        virtual_explain();
    void        virtual_onRun();
    void        virtual_onCleanup();

private:
    struct Vertex 
    {
        gos::vec3f  pos;
        gos::vec2f  tutv0;
        gos::vec3f  normal;
    };

    struct sUniformBufferObject 
    {
        //glm::mat4 world;
        gos::mat4x4f    camView;
        gos::mat4x4f    camProj;
        gos::vec4f      lightDir;
        gos::mat4x4f    objWorld;
    };


private:
    bool        createVertexIndexStageBuffer();
    bool        recordCommandBuffer (GPUCmdBufferHandle &cmdBufferHandle);
    void        doCPUStuff ();
    void        mainLoop();
    void        virtual_onInputEvent (u32 event32, i16 value, const gos::input::MouseStatus &mouseStatus, const gos::input::sButtonModifier &btnModifier);

private:
    gos::FastArray<gos::shape::Shape> shapeList;
    sUniformBufferObject    ubo;
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
};


#endif //_VulkanExample6_h_