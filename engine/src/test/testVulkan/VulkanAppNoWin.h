#ifndef _VulkanAppNoWin_h_
#define _VulkanAppNoWin_h_
#include "gosGPU.h"
#include "gosAsset2Builder.h"
#include "../gosGeom/gosGeomCamera3.h"
#include "../gosShape/gosShape.h"
#include "../gosGameUtils/ctrl/gosCtrlFreeMove.h"


/*************************************************
 *  VulkanAppNoWin
 */
class VulkanAppNoWin
{
public:
    
                VulkanAppNoWin();
    virtual     ~VulkanAppNoWin()                           { }


    bool        init (gos::GPU *gpu);
    void        run()                                       { bQuitApp=false; virtual_onRun(); }
    void        cleanup()                                   { virtual_onCleanup(); }


protected:
    gos::GPU    *gpu;
    bool        bQuitApp;
    
protected:
    virtual bool    virtual_onInit() = 0;    
    virtual void    virtual_explain() = 0;
    virtual void    virtual_onRun() = 0;
    virtual void    virtual_onCleanup() = 0;
};

#endif //_VulkanAppNoWin_h_