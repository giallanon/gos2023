#ifndef _TheApp_h_
#define _TheApp_h_
#include "gosGPU.h"
#include "../gosShape/gosShape.h"
#include "../gosShape/gosShapeImport.h"
#include "../gosGeom/gosGeomCamera3.h"
#include "../gosGPU/utils/gosFreeMovement.h"
#include "Renderer1.h"
#include "terrain/MapRenderer.h"
#include "LineRenderer/LineRenderer.h"

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

    ThePipeline         thePipeline;
    Renderer1           renderer;
    MapRenderer         mapRenderer;
    LineRenderer        lineRenderer;
};



#endif //_TheApp_h_
