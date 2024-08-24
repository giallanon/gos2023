#ifndef _VulkanApp_h_
#define _VulkanApp_h_
#include "gosGPU.h"
#include "../gosGeom/gosGeomCamera3.h"
#include "../gosGeom/gosGeomShapes.h"
#include "FPSMegaTimer.h"
#include "GPUMainLoop.h"
#include "FPSMovement.h"
#include "FreeMovement.h"

/*************************************************
 *  VulkanApp
 */
class VulkanApp
{
public:
    
                VulkanApp();
    virtual     ~VulkanApp()                                { }


    bool        init (gos::GPU *gpu, const char *title);
    void        run()                                       { bQuitApp=false; virtual_onRun(); }
    void        cleanup()                                   { virtual_onCleanup(); }

    void        toggleFullscreen()                          { gpu->toggleFullscreen(); }
    void        toggleVSync();


protected:
    gos::GPU                *gpu;
    bool                    bQuitApp;
    FPSMegaTimer            fpsMegaTimer;
    gos::input::Mapper      inputMap;

protected:
    void            handleInput();
    virtual bool    virtual_onInit() = 0;    
    virtual void    virtual_explain() = 0;
    virtual void    virtual_onRun() = 0;
    virtual void    virtual_onCleanup() = 0;
    virtual void    virtual_onInputEvent (u32 event32, i16 value)   { }
};

#endif //_VulkanApp_h_