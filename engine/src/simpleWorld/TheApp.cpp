#include "TheApp.h"

using namespace gos;


//********************************
TheApp::TheApp()
{
    gpu = NULL;
}

//********************************
TheApp::~TheApp()
{
    unsetup();
}

//********************************
void TheApp::unsetup()
{
    if (NULL == gpu)
        return;

    gpu = NULL;
}

//********************************
bool TheApp::setup (gos::GPU *gpuIN)
{
    gpu = gpuIN;

    //fs alias
    fs::addAlias ("@shader", "shader/compiled", eAliasPathMode::relativeToAppFolder);

    //input mapping
    inputCtx.action_add ("quit")
        .action_add ("toggleFullscreen")
        .action_add ("toggleVSync")
        .action_add ("show_all_actions")

        .action_add ("toggle_mouse_mode")
        .action_add ("move_forward")
        .action_add ("move_backward")
        .action_add ("strafe_left")
        .action_add ("strafe_right")
        .action_add ("strafe_up")
        .action_add ("strafe_down")
        .action_add ("rotateX")
        .action_add ("rotateY")
        .action_add("speedUP")
        .action_add("speedDOWN")
        .action_add("printCAMPos");


    inputCtx.action_bindToBtn ("quit", input::eOrigin::keyboard, GLFW_KEY_Q, input::eButtonStatus::pressed, input::sButtonModifier(input::eButtonModifier::LCTRL));
    inputCtx.action_bindToBtn ("quit", input::eOrigin::window, GOS_BUTTON_WINDOW_CLOSE, input::eButtonStatus::pressed);
    inputCtx.action_bindToBtn ("toggleFullscreen", input::eOrigin::keyboard, GLFW_KEY_ENTER, input::eButtonStatus::pressed, input::sButtonModifier(input::eButtonModifier::LALT));
    inputCtx.action_bindToBtn ("toggleVSync", input::eOrigin::keyboard, GLFW_KEY_BACKSPACE, input::eButtonStatus::pressed, input::sButtonModifier(input::eButtonModifier::LALT));
    inputCtx.action_bindToBtn ("show_all_actions", input::eOrigin::keyboard, GLFW_KEY_F1, input::eButtonStatus::pressed, input::sButtonModifier(input::eButtonModifier::LCTRL, input::eButtonModifier::LSHIFT));

    inputCtx.action_bindToBtn ("toggle_mouse_mode", input::eOrigin::keyboard, GLFW_KEY_TAB, input::eButtonStatus::pressed);
    inputCtx.action_bindToBtn ("move_forward", input::eOrigin::keyboard, GLFW_KEY_W, input::eButtonStatus::both);
    inputCtx.action_bindToBtn ("move_backward", input::eOrigin::keyboard, GLFW_KEY_S, input::eButtonStatus::both);
    inputCtx.action_bindToBtn ("strafe_left", input::eOrigin::keyboard, GLFW_KEY_A, input::eButtonStatus::both);
    inputCtx.action_bindToBtn ("strafe_right", input::eOrigin::keyboard, GLFW_KEY_D, input::eButtonStatus::both);
    inputCtx.action_bindToBtn ("strafe_up", input::eOrigin::keyboard, GLFW_KEY_Q, input::eButtonStatus::both);
    inputCtx.action_bindToBtn ("strafe_down", input::eOrigin::keyboard, GLFW_KEY_Z, input::eButtonStatus::both);

    inputCtx.action_bindToAxleREL ("rotateX",  input::eOrigin::mouse, input::eAxle::y, input::eAxleDirection::both);
    inputCtx.action_bindToAxleREL ("rotateY",  input::eOrigin::mouse, input::eAxle::x, input::eAxleDirection::both);

    inputCtx.action_bindToBtn ("speedUP", input::eOrigin::keyboard, GLFW_KEY_KP_ADD, input::eButtonStatus::pressed);
    inputCtx.action_bindToBtn ("speedDOWN", input::eOrigin::keyboard, GLFW_KEY_KP_SUBTRACT, input::eButtonStatus::pressed);
    inputCtx.action_bindToBtn ("printCAMPos", input::eOrigin::keyboard, GLFW_KEY_KP_ENTER, input::eButtonStatus::pressed);

    //creo la pipeline
    if (!thePipeline.setup (gpu))
        return false;

    //aggiungo un renderer
    if (!renderer.setup(&thePipeline))
        return false;
    
    //map renderer
    if (!mapRenderer.setup (&thePipeline, "assets/map256.tga"))
        return false;

    if (!lineRenderer.setup (&thePipeline))
        return false;

    //movement.setLinearSpeed (15);
    return true;
}


//************************************
void TheApp::priv_toggleVSync()
{ 
    if (gpu->vsync_isEnabled())
    {
        gpu->vsync_enable (false);
        gos::logger::log (eTextColor::yellow, "VSYNC: off\n");
    }
    else
    {
        gpu->vsync_enable (true);
        gos::logger::log (eTextColor::yellow, "VSYNC: on\n");
    }
}

//************************************
void TheApp::priv_handleInput()
{
    gos::input::pollEvents();

    input::ResolvedEvtList evtList;
    input::resolveEvents (gpu->getWindow(), &inputCtx, &evtList);

    i16 value;
    while (1)
    {
        const u32 actionID = evtList.nextActionID(&value);
        if (0 == actionID)
            break;
        switch (actionID)
        {
        default:
            break;

        case COMPILE_TIME_STR_CRC32("quit"):
            bQuitApp = true;
            break;

        case COMPILE_TIME_STR_CRC32("toggleFullscreen"):
            gpu->toggleFullscreen();
            break;

        case COMPILE_TIME_STR_CRC32("toggleVSync"):
            this->priv_toggleVSync();
            break;

        case COMPILE_TIME_STR_CRC32("show_all_actions"):
            inputCtx.logAllMappedInput();
            break;

        case COMPILE_TIME_STR_CRC32("toggle_mouse_mode"):
            input::window_toggleMouseMode(gpu->getWindow());
            break;

        case COMPILE_TIME_STR_CRC32("move_forward"):           movement.moveForward ((value == 1));break;
        case COMPILE_TIME_STR_CRC32("move_backward"):          movement.moveBackward ((value == 1));    break;
        case COMPILE_TIME_STR_CRC32("strafe_left"):            movement.strafeLeft ((value == 1));    break;
        case COMPILE_TIME_STR_CRC32("strafe_right"):           movement.strafeRight ((value == 1));    break;
        case COMPILE_TIME_STR_CRC32("strafe_up"):              movement.strafeUp ((value == 1));    break;
        case COMPILE_TIME_STR_CRC32("strafe_down"):            movement.strafeDown ((value == 1));    break;
        case COMPILE_TIME_STR_CRC32("rotateY"):                movement.rotateY ((value<0)); break;
        case COMPILE_TIME_STR_CRC32("rotateX"):                movement.rotateX ((value<0)); break;

        case COMPILE_TIME_STR_CRC32("mouse_move"):
        case COMPILE_TIME_STR_CRC32("LMB"):
        case COMPILE_TIME_STR_CRC32("RMB"):
            break;

        case COMPILE_TIME_STR_CRC32("printCAMPos"):
            printf ("camera pos (%.2f, %.2f, %.2f)\n", cam.pos.o.x, cam.pos.o.y, cam.pos.o.z);
            break;

        case COMPILE_TIME_STR_CRC32("speedUP"):
            {
                f32 speed = movement.getLinearSpeed();
                speed += 4.0f;
                movement.setLinearSpeed(speed);
                printf ("movement speed set to %.1f\n", speed);
            }
            break;

        case COMPILE_TIME_STR_CRC32("speedDOWN"):
            {
                f32 speed = movement.getLinearSpeed();
                speed -= 4.0f;
                if (speed < 4)
                    speed = 4;
                movement.setLinearSpeed(speed);
                printf ("movement speed set to %.1f\n", speed);
            }
            break;            
        }
    }
}    

//**********************************
void TheApp::priv_doCPUStuff ()
{
    priv_handleInput();

    //gestione del movimento
    const u64 timeNow_msec = gos::getTimeSinceStart_msec();
    movement.update(timeNow_msec);
    cam.markUpdated();
}

//********************************
#include "../gosShape/gosShapePrefabs.h"
bool TheApp::priv_buildScene1()
{
    //carico delle texture
    GPUTextureHandle hTex_checker;
    GPUTextureHandle hTex_stone003;
    {    
        gos::Image im;
        image::load (gos::getScrapAllocator(), "texture/checker_color_1k.gosimage", &im);
        gpu->texture_create2D (&im, 0, &hTex_checker);
        image::free (gos::getScrapAllocator(), im);

        image::load (gos::getScrapAllocator(), "texture/stonetiles_003.gosimage", &im);
        gpu->texture_create2D (&im, 0, &hTex_stone003);
        image::free (gos::getScrapAllocator(), im);
    }


    //creo una shape e la bindo a VB/IB
    tpp::sBoundShapeInfo uploadedShapeCubo1;
    {
        gos::Shape sh;
        sh.reset();
        const f32 lato = 1;
        gos::shape::buildCube24 (vec3f(0,0,0), vec3f(lato, lato, lato), thePipeline.vtxLayout, gos::getSysHeapAllocator(), &sh);
        if (!thePipeline.shape_uploadToVBIB (&sh, &uploadedShapeCubo1))
        {
            gos::logger::err ("TheApp::priv_buildScene1() => can't upload to VtxBuffer\n");
            return false;
        }
        shape::shapeFree (gos::getSysHeapAllocator(), &sh);
    }

    //creo il pavimento e lo bindo a VB/IB
    tpp::sBoundShapeInfo uploadedShapePlane1;
    {
        gos::Shape sh;
        sh.reset();
        gos::shape::buildCube24 (vec3f(0,0,0), vec3f(40, 0.1f, 40), thePipeline.vtxLayout, gos::getSysHeapAllocator(), &sh);

        if (!thePipeline.shape_uploadToVBIB (&sh, &uploadedShapePlane1))
        {
            gos::logger::err ("TheApp::priv_buildScene1() => can't upload to VtxBuffer\n");
            return false;
        }
        shape::shapeFree (gos::getSysHeapAllocator(), &sh);
    }    


    //creo dei materiali
    u16 material1, material2, material3, material4;
    renderer.material_create (hTex_checker, gos::vec3f(1,0,0), &material1);
    renderer.material_create (hTex_stone003, gos::vec3f(0,1,0), &material2);
    renderer.material_create (hTex_stone003, gos::vec3f(1,1,1), &material3);
    renderer.material_create (hTex_checker, gos::vec3f(1,1,1), &material4);

    //aggiungo la shape al renderer
    u16 shapeCubo1;         renderer.shape_add (uploadedShapeCubo1, &shapeCubo1);
    u16 shapePavimento;     renderer.shape_add (uploadedShapePlane1, &shapePavimento);

    //creo un po' di istanze
    renderer.instance_add (shapeCubo1, material1, geom::Pos3(-1,0.5f,0));
    renderer.instance_add (shapeCubo1, material2, geom::Pos3(-2,1.5f,0));
    renderer.instance_add (shapeCubo1, material3, geom::Pos3(-1,2.5f,0));
    renderer.instance_add (shapeCubo1, material1, geom::Pos3(-2,3.5f,0));

    /*renderer.instance_add (shapeCubo1, material1, geom::Pos3( 1,0.5f,0));
    renderer.instance_add (shapeCubo1, material2, geom::Pos3( 2,1.5f,0));
    renderer.instance_add (shapeCubo1, material3, geom::Pos3( 1,2.5f,0));
    renderer.instance_add (shapeCubo1, material1, geom::Pos3( 2,3.5f,0));
    */
    renderer.instance_add (shapePavimento, material4, geom::Pos3(0,-0.01f,0));

    return true;
}


//********************************
bool TheApp::priv_buildScene2()
{
    //carico delle texture
    GPUTextureHandle hTex_stone003;
    {    
        gos::Image im;
        image::load (gos::getScrapAllocator(), "texture/stonetiles_003.gosimage", &im);
        gpu->texture_create2D (&im, 0, &hTex_stone003);
        image::free (gos::getScrapAllocator(), im);
    }

    //creo dei materiali
    u16 material1;
    renderer.material_create (hTex_stone003, gos::vec3f(1,1,1), &material1);


    //shape
    gos::ShapeList shapeList;
    shapeList.setup (gos::getScrapAllocator(), 16 * 1024);
    if (!gos::shape::importFrom_glTF ("/home/giallanon/Desktop/info/Blender/modelli/models_from_glTF_repo/Sponza/glTF/Sponza.glb", thePipeline.vtxLayout, gos::getSysHeapAllocator(), shapeList)) 
        return false;
    
    
    //instances
    const u32 n = shapeList.getNElem();
    for (u32 i=0; i<n; i++)
    {
        tpp::sBoundShapeInfo uploadedShapeInfo;
        if (!thePipeline.shape_uploadToVBIB (&shapeList(i), &uploadedShapeInfo))
        {
            gos::logger::err ("TheApp::priv_buildScene2() => can't upload to VtxBuffer\n");
            return false;
        }

        u16 shapeIndex;
        if (!renderer.shape_add (uploadedShapeInfo, &shapeIndex))
        {
            gos::logger::err ("TheApp::priv_buildScene2() => can't shape_add()\n");
            return false;
        }
        
        renderer.instance_add (shapeIndex, material1, geom::Pos3(0,0,0));
    }
    

    //free delle shape
    for (u32 i=0; i<shapeList.getNElem(); i++)
    {
        shape::shapeFree (gos::getSysHeapAllocator(), &shapeList[i]);
    }
    shapeList.unsetup();
    
    return true;
}

//********************************
void TheApp::run()
{
    if (!priv_buildScene1())
    //if (!priv_buildScene2())
    {
        gos::logger::err ("TheApp::run() => cant build scene\n");
        return;
    }


    //posizione inziale della camera
    cam.setPerspectiveFovLH (gpu->swapChain_calcAspectRatio(),  math::gradToRad(45), 0.1f, 850.0f);
    cam.pos.identity(); cam.pos.warp (0, 1.8f, -10);
    cam.markUpdated();
    movement.bind (&cam.pos);



    //command buffer 
    GPUCmdBufferHandle  cmdBufferHandle;
    gpu->cmdBuffer_create (eGPUQueueType::gfx, &cmdBufferHandle);
    gos::gpu::CmdBufferWriter cw;



    //main loop
    gpu::MainLoop gpuLoop;
    gpuLoop.setup (gpu);
    bQuitApp = false;
    while (bQuitApp == false)
    {
        //cpu stuff & input handling
        gpuLoop.stat_onCPUFrameBegin();
        priv_doCPUStuff();
        gpuLoop.stat_onCPUFrameEnd();

        gpuLoop.stat_printReport();


        gpuLoop.run ();
        if (gpuLoop.swapchainRecreated())
            cam.changeAspectRatioPerspectiveFovLH (gpu->swapChain_calcAspectRatio());
        if (gpuLoop.canSubmitGFXJob())
        {
            cw.begin (gpu, cmdBufferHandle);

            
            ThePipeline::Context ctx;
            ctx.cam = &cam;
            ctx.cw = &cw;
            if (thePipeline.beginFrame (ctx))
            {
                //mapRenderer.recordCommandBuffer(cw, &cam);
                renderer.recordCommandBuffer(cw, &cam);
                lineRenderer.recordCommandBuffer(cw, &cam);

                thePipeline.endFrame(ctx);

                
                gpuLoop.submitGFXJob (cmdBufferHandle);
            }
        }

    }
    
    //aspetto che GPU abbia finito tutto cio' che ha in coda
    gpu->waitIdle();

    //free
    gpu->deleteResource (cmdBufferHandle);
    gpuLoop.unsetup();
}
