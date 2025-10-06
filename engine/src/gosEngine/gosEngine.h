#ifndef _gosEngine_h_
#define _gosEngine_h_
#include "gosGPU.h"
#include "gosInput.h"
#include "gosAssetHub.h"

namespace gos
{
    /****************
     * @brief   Engine
     * 
     * 
     */
    class Engine
    {
    public:
                Engine();
                ~Engine();

        bool    setup (u32 mainWin_w, u32 mainWin_h, const char *mainWin_title);

        bool    run();
        void    toggleFullscreen()                          { gpu->toggleFullscreen(); }
        void    toggleVSync();

    public:
        gos::GPU                *gpu;
        gos::input::Context     *inputCtx;
        gos::asset::Hub         *assetHub;

    private:
        void    priv_handleInput();

    private:
        bool                    bQuitEngine;

    }; //class Engine
} //namespace gos


#endif //_gosEngine_h_

