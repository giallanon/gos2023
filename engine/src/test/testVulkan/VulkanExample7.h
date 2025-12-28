#ifndef _VulkanExample7_h_
#define _VulkanExample7_h_
#include "VulkanAppNoWin.h"
#include "gosAsset2Hub.h"
#include "../gosImage/gosImageBufferRGBA.h"


/************************************
 *  VulkanExample7
 * 
  */
class VulkanExample7 : public VulkanAppNoWin
{
public:
    
                VulkanExample7();

    bool        virtual_onInit ();
    void        virtual_onRun();
    void        virtual_onCleanup();
    void        virtual_explain();

private:    
    bool        sampleImage1 (GPUCmdBufferHandle &cmdBufferHandle, u32 dstW, u32 dstH);
    bool        sampleImage2 (GPUCmdBufferHandle &cmdBufferHandle, u32 dstW, u32 dstH);
    void        save (const gos::gpu::sMappedImage &src, u32 srcW, u32 srcH);

private:
    gos::asset2::Hub        theHub;
    gos::asset2::Handle     assetPipe1;
    gos::asset2::Handle     assetPipe2;
    GPUVtxBufferHandle      vtxBufferHandle;
    GPUIdxBufferHandle      idxBufferHandle;
    GPURenderTargetHandle   rt1;
    GPURenderTargetHandle   rtReadback;

    GPUTextureHandle        texHandle;
    GPUSamplerHandle        samplerHandle;
    GPUDescrPoolHandle      descrPoolHandle;
    GPUDescrSetInstanceHandle descrSetInstanceHandle;

    GPUViewportHandle       viewportHandle;
    u32                     tex_width;
    u32                     tex_height;
    u32                     rt_width;
    u32                     rt_height;

    bool bUsePipe1;


};


#endif //_VulkanExample7_h_