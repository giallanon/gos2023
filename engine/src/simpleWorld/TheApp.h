#ifndef _TheApp_h_
#define _TheApp_h_
#include "gosGPU.h"
#include "../gosShape/gosShape.h"
#include "../gosShape/gosShapeImport.h"
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
    void    unsetup();
    void    run();


private:
    struct sVertex
    {
        gos::vec3f  pos;
        gos::vec3f  norm;
        gos::vec2f  tutv0;
    };

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

    VBIBSTBuffer        vbibstBuffer;
    ThePipeline         thePipeline;
    Renderer1           renderer;
};



#endif //_TheApp_h_
