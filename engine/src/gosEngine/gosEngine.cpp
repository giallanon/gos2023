#include "gosEngine.h"
#include "../gos/memory/gosAllocatorHeap.h"

using namespace gos;

typedef gos::AllocatorHeap<gos::AllocPolicy_Track_simple, gos::AllocPolicy_Thread_Safe>		GOSENGINEMemAllocatorTS;


//******************************** 
Engine::Engine()
{
    allocator = NULL;
    gpu = NULL;
    inputCtx = NULL;
    assetHub = NULL;
    bQuitEngine = false;
}

//******************************** 
void Engine::unsetup()
{
    bQuitEngine = true;
    if (NULL == gpu || NULL == allocator)
        return;

    //resource manager
    vtxBufferMan.unsetup();
    idxBufferMan.unsetup();
    worldMatrixBufferMan.unsetup();
    
    //handle lists
    vtxBufferHandleList.unsetup();
    idxBufferHandleList.unsetup();
    shapeHandleList.unsetup();


    //win & gpu
    GOSWinHandle mainWin = gpu->getWindow();
    
    GOSDELETE(gos::getSysHeapAllocator(), assetHub);
    assetHub = NULL;

    gpu->deinit();
    GOSDELETE(gos::getSysHeapAllocator(), gpu);
    gpu = NULL;

    GOSDELETE(gos::getSysHeapAllocator(), inputCtx);
    inputCtx = NULL;

    gos::input::window_destroy (mainWin);
    gos::input::deinit();


    //engine allocator
    GOSDELETE(gos::getSysHeapAllocator(), allocator);
    allocator = NULL;
}

//******************************** 
bool Engine::setup (u32 mainWin_w, u32 mainWin_h, const char *mainWin_title)
{
    if (!gos::input::init())
    {
        logger::err ("Engine::setup() => input::init failed\n");
        return false;
    }

    //main win
    GOSWinHandle mainWin;
    if (!gos::input::window_create (1024, 768, gos::getAppName(), &mainWin))
    {
        logger::err ("Engine::setup() => input::window_create failed\n");
        return false;
    }
    gos::input::window_setTitle (mainWin, mainWin_title);

    //GPU
    gpu = GOSNEW(gos::getSysHeapAllocator(), gos::GPU)();
    if (!gpu->init (mainWin, false))
    {
        logger::err ("Engine::setup() => gpu->init() failed\n");
        return false;
    }

    //assetHub
    assetHub = GOSNEW(gos::getSysHeapAllocator(), asset::Hub)();
    assetHub->setup ("@w/data", gpu);

    //creo l'input context di default
    inputCtx = GOSNEW(gos::getSysHeapAllocator(), input::Context)("global");
    {
        
        inputCtx->action_add ("app_terminate")
            .action_add ("toggle_fullscreen")
            .action_add ("toggle_vsync")
            .action_add ("show_all_actions")

            .action_add ("toggle_mouse_mode")
            .action_add ("move_forward")
            .action_add ("move_backward")
            .action_add ("strafe_left")
            .action_add ("strafe_right")
            .action_add ("strafe_up")
            .action_add ("strafe_down")
            .action_add ("rotateX")
            .action_add ("rotateY");


        inputCtx->action_bindToBtn ("app_terminate", input::eOrigin::keyboard, GLFW_KEY_Q, input::eButtonStatus::pressed, input::sButtonModifier(input::eButtonModifier::LCTRL));
        inputCtx->action_bindToBtn ("app_terminate", input::eOrigin::window, GOS_BUTTON_WINDOW_CLOSE, input::eButtonStatus::pressed);
        inputCtx->action_bindToBtn ("toggle_fullscreen", input::eOrigin::keyboard, GLFW_KEY_ENTER, input::eButtonStatus::pressed, input::sButtonModifier(input::eButtonModifier::LALT));
        inputCtx->action_bindToBtn ("toggle_vsync", input::eOrigin::keyboard, GLFW_KEY_BACKSPACE, input::eButtonStatus::pressed, input::sButtonModifier(input::eButtonModifier::LALT));
        inputCtx->action_bindToBtn ("show_all_actions", input::eOrigin::keyboard, GLFW_KEY_F1, input::eButtonStatus::pressed, input::sButtonModifier(input::eButtonModifier::LCTRL, input::eButtonModifier::LSHIFT));
        
        inputCtx->action_bindToBtn ("toggle_mouse_mode", input::eOrigin::keyboard, GLFW_KEY_TAB, input::eButtonStatus::pressed);
        inputCtx->action_bindToBtn ("move_forward", input::eOrigin::keyboard, GLFW_KEY_W, input::eButtonStatus::both);
        inputCtx->action_bindToBtn ("move_backward", input::eOrigin::keyboard, GLFW_KEY_S, input::eButtonStatus::both);
        inputCtx->action_bindToBtn ("strafe_left", input::eOrigin::keyboard, GLFW_KEY_A, input::eButtonStatus::both);
        inputCtx->action_bindToBtn ("strafe_right", input::eOrigin::keyboard, GLFW_KEY_D, input::eButtonStatus::both);
        inputCtx->action_bindToBtn ("strafe_up", input::eOrigin::keyboard, GLFW_KEY_Q, input::eButtonStatus::both);
        inputCtx->action_bindToBtn ("strafe_down", input::eOrigin::keyboard, GLFW_KEY_Z, input::eButtonStatus::both);

        inputCtx->action_bindToAxleREL ("rotateX",  input::eOrigin::mouse, input::eAxle::y, input::eAxleDirection::both);
        inputCtx->action_bindToAxleREL ("rotateY",  input::eOrigin::mouse, input::eAxle::x, input::eAxleDirection::both);    
    }


    //Creo un allocatore dedicato per la GPU
    GOSENGINEMemAllocatorTS *engAllocator = GOSNEW(gos::getSysHeapAllocator(), GOSENGINEMemAllocatorTS)("ENG");
    engAllocator->setup (1024 * 1024 * 128); //128MB
    this->allocator = engAllocator;

    //handle list
    vtxBufferHandleList.setup (allocator);
    idxBufferHandleList.setup (allocator);
    shapeHandleList.setup (allocator);

    //resource manager
    vtxBufferMan.setup (allocator, gpu);
    idxBufferMan.setup (allocator, gpu);
    worldMatrixBufferMan.setup (allocator, NUM_MAX_WMATRIX);
    return true;
}

//************************************
void Engine::toggleVSync()
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


//******************************** 
bool Engine::update()
{
    if (bQuitEngine)
        return false;

    //input
    gos::input::pollEvents();
    input::resolveEvents (gpu->getWindow(), inputCtx, &evtList);
    return true;
}

//******************************** 
bool Engine::inputEvent_getNext (InputEvent *out)
{
    while (1)
    {
        out->actionID = evtList.nextActionID(&out->value);
        if (0 == out->actionID)
            return false;

        switch (out->actionID)
        {
        default:
            return true;
            break;

        case COMPILE_TIME_STR_CRC32("show_all_actions"):
            inputCtx->logAllMappedInput();
            break;

        case COMPILE_TIME_STR_CRC32("app_terminate"):
            bQuitEngine = true;
            break;

        case COMPILE_TIME_STR_CRC32("toggle_fullscreen"):
            this->toggleFullscreen();
            break;

        case COMPILE_TIME_STR_CRC32("toggle_vsync"):
            this->toggleVSync();
            break;

        case COMPILE_TIME_STR_CRC32("toggle_mouse_mode"):
            input::window_toggleMouseMode(gpu->getWindow());
            break;
        }
    }
}


//******************************** 
bool Engine::vtxBuffer_create (u32 sizeInByte, eMemAccessMode mode, ENGVtxBuffer *out_handle)
{
    GPUVtxBufferHandle gpuResourceHandle;
    if (!gpu->vertexBuffer_create (sizeInByte, mode, &gpuResourceHandle))
    {
        return false;
    }

    assert (NULL != out_handle);
    engine::VtxBuffer *s = vtxBufferHandleList.reserveTS(out_handle);
    if (NULL == s)
    {
        logger::err ("Engine::vtxBuffer_create() => can't create handle\n");
        return false;
    }

    s->vbHandle = gpuResourceHandle;
    return true;
}

void Engine::vtxBuffer_release (ENGVtxBuffer &handle)
{
    engine::VtxBuffer info;
    if (vtxBufferHandleList.releaseTS (handle, &info))
        priv_vtxBuffer_delete (&info);
    handle.setInvalid();
}

void Engine::priv_vtxBuffer_delete (engine::VtxBuffer *info)
{
    gpu->deleteResource (info->vbHandle);
}

//******************************** 
bool Engine::idxBuffer_create (u32 sizeInByte, eMemAccessMode mode, ENGIdxBuffer *out_handle)
{
    GPUIdxBufferHandle gpuResourceHandle;
    if (!gpu->indexBuffer_create (sizeInByte, mode, &gpuResourceHandle))
    {
        return false;
    }

    assert (NULL != out_handle);
    engine::IdxBuffer *s = idxBufferHandleList.reserveTS(out_handle);
    if (NULL == s)
    {
        logger::err ("Engine::idxBuffer_create() => can't create handle\n");
        return false;
    }

    s->ibHandle = gpuResourceHandle;
    return true;
}

void Engine::idxBuffer_release (ENGIdxBuffer &handle)
{
    engine::IdxBuffer info;
    if (idxBufferHandleList.releaseTS (handle, &info))
        priv_idxBuffer_delete (&info);
    handle.setInvalid();
}

void Engine::priv_idxBuffer_delete (engine::IdxBuffer *info)
{
    gpu->deleteResource (info->ibHandle);
}


//******************************** 
bool Engine::shape_create (const gos::Shape *shape, ENGShape *out_handle)
{
    assert (NULL != out_handle);
    engine::Shape *s = shapeHandleList.reserveTS(out_handle);
    if (NULL == s)
    {
        logger::err ("Engine::shape_create() => can't create handle\n");
        return false;
    }

    
    u32 byteNeeded;

    //vtxbuffer
    if (shape->numVtx)
    {
        const u32 sizeOfAVertex = gos::shape::calcSizeOfAVertex(shape->vtxLayout);
        byteNeeded = sizeOfAVertex * shape->numVtx;
        vtxBufferMan.reserve (byteNeeded, &s->alloc_vtxbuf_offset, &s->alloc_vtxbuf_size, &s->vbHandle);
        s->vtxStart = s->alloc_vtxbuf_offset / sizeOfAVertex;
        s->numVertex = shape->numVtx;
    }

    //idxBuffer    
    if (shape->numIdx)
    {
        const u32 sizeOfAnIndex = sizeof(u16);
        byteNeeded = sizeOfAnIndex * shape->numIdx;
        idxBufferMan.reserve (byteNeeded, &s->alloc_idxbuf_offset, &s->alloc_idxbuf_size, &s->ibHandle);
        s->indexStart = s->alloc_idxbuf_offset / sizeOfAnIndex;
        s->numIndices = shape->numIdx;
    }
    return true;
}

void Engine::shape_release (ENGShape &handle)
{
    engine::Shape info;
    if (shapeHandleList.releaseTS (handle, &info))
        priv_shape_delete (&info);
    handle.setInvalid();
}

void Engine::priv_shape_delete (engine::Shape *info)
{
    if (info->numVertex)
        vtxBufferMan.release (info->vbHandle, info->alloc_vtxbuf_offset, info->alloc_vtxbuf_size);

    if (info->numIndices)
        idxBufferMan.release (info->ibHandle, info->alloc_idxbuf_offset, info->alloc_idxbuf_size);
}