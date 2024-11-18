#ifndef _TheApp_h_
#define _TheApp_h_
#include "../gosGPU/gosGPU.h"
#include "../gosGeom/gosGeomCamera3.h"
#include "../gosGPU/utils/gosFreeMovement.h"


/**
 * @brief TheApp
 *  
 */
class TheApp
{
public:
    TheApp();
    ~TheApp();

    bool    setup (gos::GPU *gpu);
    void    unsetup();
    void    run();

private:
    void    priv_toggleVSync();
    void    priv_handleInput();
    void    priv_doCPUStuff ();
    bool    priv_buildScene1();
    bool    priv_buildScene2();

private:
    gos::GPU            *gpu;
    bool                bQuitApp;
    gos::input::Context inputCtx;
    gos::geom::Camera3  cam;
    gos::FreeMovement   movement;
};



#endif //_TheApp_h_
