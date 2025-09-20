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
    bool        recordCommandBuffer (GPUCmdBufferHandle &cmdBufferHandle, VkImage swapChainImage);
    bool        do_recordCommandBuffer (gos::gpu::pipe2::CmdBufferWriter2 &cw, VkImage swapChainImage);

private:
    gos::asset::Hub         theHub;
    gos::asset::Handle      assetPipe1;
    gos::asset::Handle      assetPipe2;
    GPUVtxBufferHandle      vtxBufferHandle;
    GPUIdxBufferHandle      idxBufferHandle;
    GPURenderTargetHandle   rt1;
    u32                     vtxoffset2, idxoffset2;

};


#endif //_VulkanExample7_h_