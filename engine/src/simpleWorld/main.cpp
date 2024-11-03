#include "TheApp.h"
#include "../gosImage/gosImageBuilder.h"

using namespace gos;


//**********************************
void importTexture (u16 w, u16 h, const char *fname)
{
    gos::Image im;
    gos::image::Builder builder;

    builder.begin (gos::getScrapAllocator(), &im)
        .beginTexture2D (gos::eImageFormat::U8_RGBA_sRGB, w, h, 1)
        .setMipMapDataFromFile (0, fname)
        .endTexture2D()
    .end();
    if (builder.anyError())
        gos::logger::err ("importAssets => can't build image %s'\n", fname);
    else
    {
        char name[128];
        gos::fs::extractFileNameWithoutExt(fname, name, sizeof(name));

        char s[1024];
        sprintf_s (s, sizeof(s), "texture/%s.gosimage", name);
        image::save (im, s);
    }

    image::free (gos::getScrapAllocator(), im);
}

//**********************************
void importAssetes ()
{
    importTexture (1024, 1024, "texture/checker_color_1k.jpg");
    importTexture (512, 512, "texture/stonetiles_003.png");
}

//**********************************
void foreverLoop (gos::GPU *gpu)
{
    TheApp app;

    app.setup (gpu);
    app.run();
}


//******************************** 
int main()
{
    gos::sGOSInit init;
    init.memory_setDefaultForGame();

    init.setLogMode (gos::sGOSInit::eLogMode::both_console_and_file);
    if (!gos::init (init, "simpleWorld"))
        return -1;

    if (!gos::input::init())
        return -2;

    GOSWinHandle mainWin;
    if (!gos::input::window_create (1024, 768, gos::getAppName(), &mainWin))
        return -3;
        
    gos::GPU gpu;
    if (!gpu.init (mainWin, false))
        return -4;

    //importAssetes ();
    foreverLoop (&gpu);
    
    gpu.deinit();
    gos::input::window_destroy (mainWin);
    gos::input::deinit();
    gos::deinit();
    return 0;
}