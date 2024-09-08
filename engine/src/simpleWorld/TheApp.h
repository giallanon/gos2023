#ifndef _TheApp_h_
#define _TheApp_h_
#include "gosGPU.h"
#include "../gosShape/gosShape.h"
#include "../gosGeom/gosGeomCamera3.h"
#include "../gosGPU/utils/gosFreeMovement.h"
#include "Renderer1.h"


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
    void    run();

private:
    void    priv_toggleVSync();
    void    priv_handleInput();
    void    priv_doCPUStuff ();

private:
    gos::GPU            *gpu;
    bool                bQuitApp;
    gos::input::Context inputCtx;
    gos::geom::Camera3  cam;
    gos::FreeMovement   movement;

    Renderer1           renderer;
};



#endif //_TheApp_h_
