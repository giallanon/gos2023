#ifndef _VKExample1_h_
#define _VKExample1_h_
#include "gosGPU.h"
#include "Pipeline1.h"

/************************************
 *  VulkanExample
 */
class VulkanExample1
{
public:
    
                VulkanExample1();

    bool        init (gos::GPU *gpu);
    void        mainLoop();
    void        cleanup();

    void        toggleFullscreen()                          { gpu->toggleFullscreen(); }

    

private:
    bool                            recordCommandBuffer(u32 imageIndex, VkCommandBuffer &in_out_commandBuffer);
    void                            mainLoop_waitEveryFrame();

private:
    gos::GPU        *gpu;
    
    Pipeline1       pipe1;
};


#endif //_VKExample1_h_