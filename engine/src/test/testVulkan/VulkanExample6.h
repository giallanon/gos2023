#ifndef _VulkanExample6_h_
#define _VulkanExample6_h_
#include "VulkanApp.h"
#include "gosAsset2Hub.h"


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
    bool        recordCommandBuffer (GPUCmdBufferHandle &cmdBufferHandle, VkImage swapChainImage);
    void        doCPUStuff ();
    void        mainLoop();
    void        virtual_onInputEvent (u32 event32, i16 value, const gos::input::MouseStatus &mouseStatus, const gos::input::sButtonModifier &btnModifier);

    bool        priv_loadModel();
    bool        priv_recordCommandBuffer_v2 (gos::gpu::CmdBufferWriter2 &cw, VkImage swapChainImage, const gos::asset2::Asset_pipe *pipe);

private:
    gos::FastArray<gos::Shape> shapeList;
    sUniformBufferObject    ubo;
    gos::geom::Camera3      cam;
    gos::FPSMovement        movement;

    GPUZBufferHandle   zbufferHandle;
    GPUVtxBufferHandle      vtxBufferHandle;
    GPUIdxBufferHandle      idxBufferHandle;
    gos::gpu::StageHelper	stageHelper;

    GPUDescrPoolHandle      descrPoolHandle;
    GPUDescrSetInstanceHandle descrSetInstancerHandle;
    GPUUniformBufferHandle  uboHandle;
    
    u32                     nextTimeSwapRT_msec;
    GPURenderTargetHandle   rtToShow;
    GPURenderTargetHandle   rt1;
    GPURenderTargetHandle   rt2;
    GPURenderTargetHandle   rt3;

    gos::asset2::Hub        theHub;
    gos::asset2::Handle     assetPipe;
    gos::asset2::Handle     assetPipe2;
};


#endif //_VulkanExample6_h_