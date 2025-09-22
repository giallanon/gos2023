#ifndef _VulkanExample7_h_
#define _VulkanExample7_h_
#include "VulkanApp.h"
#include "gosAssetHub.h"


/************************************
 *  VulkanExample7
 * 
  */
class VulkanExample7 : public VulkanApp
{
public:
    
                VulkanExample7()                    { }

    bool        virtual_onInit ();
    void        virtual_onRun();
    void        virtual_onCleanup();
    void        virtual_explain();

private:    
    void        doCPUStuff ();
    bool        recordCommandBuffer (GPUCmdBufferHandle &cmdBufferHandle, gos::gpu::SwapchainImg &swapchainImg);
    bool        do_recordCommandBuffer (gos::gpu::pipe2::CmdBufferWriter2 &cw, gos::gpu::SwapchainImg &swapchainImg);

private:
    gos::asset::Hub         theHub;
    gos::asset::Handle      assetPipe1;
    GPUVtxBufferHandle      vtxBufferHandle;
    GPUIdxBufferHandle      idxBufferHandle;
    GPURenderTargetHandle   rt1;

    GPUTextureHandle        texHandle;
    GPUSamplerHandle        samplerHandle;
    GPUDescrPoolHandle      descrPoolHandle;
    GPUDescrSetInstanceHandle descrSetInstanceHandle;


};


#endif //_VulkanExample7_h_