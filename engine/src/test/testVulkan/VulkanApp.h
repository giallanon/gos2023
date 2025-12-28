#ifndef _VulkanApp_h_
#define _VulkanApp_h_
#include "gosGPU.h"
#include "gosAsset2Builder.h"
#include "../gosGeom/gosGeomCamera3.h"
#include "../gosShape/gosShape.h"
#include "../gosGPU/utils/gosFreeMovement.h"
#include "../gosGPU/utils/gosFPSMovement.h"


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
    gos::input::Context     inputCtx;
    bool                    bQuitApp;
    
protected:
    void            handleInput();
    virtual bool    virtual_onInit() = 0;    
    virtual void    virtual_explain() = 0;
    virtual void    virtual_onRun() = 0;
    virtual void    virtual_onCleanup() = 0;
    virtual void    virtual_onInputEvent (  UNUSED_PARAM(u32 event32),
                                            UNUSED_PARAM(i16 value), 
                                            UNUSED_PARAM(const gos::input::MouseStatus &mouseStatus), 
                                            UNUSED_PARAM(const gos::input::sButtonModifier &btnModifier))   { }

};

#endif //_VulkanApp_h_