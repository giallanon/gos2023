#ifndef _VulkanApp_h_
#define _VulkanApp_h_
#include "gosGPU.h"
#include "dataTypes/gosTimer.h"


/*************************************************
 *  VulkanApp
 */
class VulkanApp
{
public:
    
                VulkanApp()                                 { gpu = NULL; }
    virtual     ~VulkanApp()                                { }


    bool        init (gos::GPU *gpu, const char *title);
    void        run()                                       { virtual_onRun(); }
    void        cleanup()                                   { virtual_onCleanup(); }

    void        toggleFullscreen()                          { gpu->toggleFullscreen(); }
    void        toggleVSync();


protected:
    gos::GPU    *gpu;


protected:
    virtual bool    virtual_onInit() = 0;    
    virtual void    virtual_onRun() = 0;
    virtual void    virtual_onCleanup() = 0;


};

#endif //_VulkanApp_h_