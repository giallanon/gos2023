#ifndef _VulkanExample6_h_
#define _VulkanExample6_h_
#include "VulkanApp.h"
#include "gosAssetHub.h"


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
    bool        priv_recordCommandBuffer_v2 (gos::gpu::pipe2::CmdBufferWriter2 &cw, VkImage swapChainImage, const gos::asset::Asset_pipe *pipe);

private:
    gos::FastArray<gos::Shape> shapeList;
    sUniformBufferObject    ubo;
    gos::geom::Camera3      cam;
    gos::FPSMovement        movement;


    GPUVtxBufferHandle      vtxBufferHandle;
    GPUIdxBufferHandle      idxBufferHandle;
    GPUStgBufferHandle      stgBufferHandle;

    GPUDescrPoolHandle      descrPoolHandle;
    GPUDescrSetInstanceHandle descrSetInstancerHandle;
    GPUUniformBufferHandle  uboHandle;
    
    u32                     nextTimeSwapRT_msec;
    GPURenderTargetHandle   rtToShow;
    GPURenderTargetHandle   rt1;
    GPURenderTargetHandle   rt2;
    GPURenderTargetHandle   rt3;

    gos::asset::Hub         theHub;
    gos::asset::Handle      assetPipe;
    gos::asset::Handle      assetPipe2;
};


#endif //_VulkanExample6_h_