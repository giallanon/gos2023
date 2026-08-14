#include "gosEngine.h"
#include "../gosAsset2/gosAsset2Builder.h"
#include "loaders/gosEngineLoaders.h"

using namespace gos;

typedef gos::AllocatorHeap<gos::AllocPolicy_Track_simple, gos::AllocPolicy_Thread_Safe>		GOSENGINEMemAllocatorTS;
//typedef gos::AllocatorHeap<gos::AllocPolicy_Track_hard, gos::AllocPolicy_Thread_Safe>		GOSENGINEMemAllocatorTS;

#define GOS_ENGINE__ASSET_HUB_PATH "@w/assets"

//******************************** 
Engine::Engine()
{
	allocator = NULL;
	gpu = NULL;
	inputCtx = NULL;
	bQuitEngine = false;
	asset_logger = NULL;
	frame_num = 0;
}

//******************************** 
void Engine::unsetup()
{
	bQuitEngine = true;
	if (NULL == gpu || NULL == allocator)
		return;

	logger::log ("\n\n***************** engine unsetup begin *********************\n");
	if (NULL != asset_logger)
		asset_logger->log ("\n\n***************** engine unsetup begin *********************\n");


	//release di tutte le risorse che erano in scheduling per l'hot-reload
	for (u32 i=0; i<list_of_res_to_be_hotreloaded.getNElem(); i++)
	{
		res::Descr *res = res__getDescriptor (list_of_res_to_be_hotreloaded(i).res_handle);
		res__release (res);
	}
	list_of_res_to_be_hotreloaded.unsetup();

	//chiedo al thread di morire e aspetto che termini
	thread::pushMsg (msgq_1W, MSG_FOR_LOADER_THREAD__DIE, 0);
	thread::wait_end (hThreadLoader);

	for (u8 i=0; i<4; i++)
	{
		priv_flushLoaderThreadMsg();
	}
	thread::deleteMsgQ (msgq_1R, msgq_1W);
	thread::deleteMsgQ (msgq_2R, msgq_2W);




	//renderPipe
	release(handle_texture_bianca);
	renderPipe.priv_unsetup();

	//resource manager
	resManager.unsetup();
	resHandleChainPool.unsetup();
	vtxBufferMan.unsetup();
	idxBufferMan.unsetup();
	listof_knownUID.unsetup();

	
	//handle lists
	map_of_shape_to_gpushape.unsetup();
	stageHelper.unsetup();

	asset2::dbcontext_close (asset_ctx);

	//win & gpu
	GOSWinHandle mainWin = gpu->getWindow();
	
	gpu->deinit();
	GOSDELETE(gos::getSysHeapAllocator(), gpu);
	gpu = NULL;

	GOSDELETE(gos::getSysHeapAllocator(), inputCtx);
	inputCtx = NULL;

	gos::input::window_destroy (mainWin);
	gos::input::deinit();

	GOSDELETE(allocator, asset_logger);

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
	if (!gos::input::window_create (mainWin_w, mainWin_h, gos::getAppName(), &mainWin))
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

	//Creo un allocatore dedicato
	GOSENGINEMemAllocatorTS *engAllocator = GOSNEW(gos::getSysHeapAllocator(), GOSENGINEMemAllocatorTS)("ENG");
	engAllocator->setup (1024 * 1024 * 128); //128MB
	this->allocator = engAllocator;

	list_of_res_to_be_hotreloaded.setup (engAllocator, 1024);

	stageHelper.setup (gpu, 8192*8192);

	//asset
	{
		gos::LoggerStdout *ll = GOSNEW(allocator,gos::LoggerStdout)();
		ll->enableStdouLogging();
		ll->enableFileLogging("@w/log_res", true);
		asset_logger = ll;
		if (!asset2::dbcontext_open (GOS_ENGINE__ASSET_HUB_PATH, true, &asset_ctx))
		{
			logger::err ("Engine::setup() => can't open asset contex in %s\n", GOS_ENGINE__ASSET_HUB_PATH);
			return false;
		}
	}    
	

	//loader thread
	//creo 2 code, una che uso per mandare msg da this al thread e una per mandare
	//msg dal thread a this
	thread::createMsgQ (&msgq_1R, &msgq_1W);
	thread::createMsgQ (&msgq_2R, &msgq_2W);

	sLoaderThreadInitParams params;
	params.msgqR = msgq_1R;
	params.msgqW = msgq_2W;
	params.logger = asset_logger;
	params.gpu = gpu;
	params.ctx = &asset_ctx;
	params.engine_allocator = this->allocator;
	params.engine = this;
	thread::signal_create (&params.hEvent_started);

	eThreadError err = thread::create (&hThreadLoader, Engine::LoaderThread_mainFN, &params);
	if (err != eThreadError::none)
	{
		logger::err ("Engine::setup() => error creating thread: errcode=%d\n", static_cast<int>(err));
		return false;
	}



	//creo l'input context di default
	inputCtx = GOSNEW(gos::getSysHeapAllocator(), input::Context)("global");
	{
		
		inputCtx->action_add ("app_terminate")
			.action_add ("toggle_fullscreen")
			.action_add ("toggle_vsync")
			.action_add ("show_all_actions")

			.action_add ("toggle_mouse_mode")
			.action_add ("mouse_move_x")
			.action_add ("mouse_move_y")

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
		inputCtx->action_bindToAxleABS ("mouse_move_x", input::eOrigin::mouse, input::eAxle::x);
		inputCtx->action_bindToAxleABS ("mouse_move_y", input::eOrigin::mouse, input::eAxle::y);

	}


	//handle list
	resManager.setup (allocator);
	resManager.addResType<res::VtxBuffer> (res::eType::vtx_buffer, 128, 8);	//8 pagine da 128 = 1024
	resManager.addResType<res::IdxBuffer> (res::eType::idx_buffer, 128, 8);
	resManager.addResType<res::Shader>	  (res::eType::vtx_shader, 128, 8);
	resManager.addResType<res::Shader>	  (res::eType::pxl_shader, 128, 8);
	resManager.addResType<res::Pipeline>  (res::eType::pipeline, 128, 8);
	resManager.addResType<res::Texture2d> (res::eType::texture_2d, 1024, 64);			//64 page da 1024 = 65.536
	resManager.addResType<res::Shape> 	  (res::eType::shape, 1024, 128);				//128 page da 1024 = 131.072
	resManager.addResType<res::GPUShape>  (res::eType::gpu_shape, 4096, 64);			//64 page da 4096 = 262.144
	resManager.addResType<res::Skeleton>  (res::eType::skeleton, 1024, 64);				//64 page da 1024 = 65.536
	resManager.addResType<res::Model3d>   (res::eType::model_3d, 4096, 64);				//64 page da 4096 = 262.144
	resManager.addResType<res::Model3dInst> (res::eType::model_instance, 8192, 256);	//256 page da 8192 = 2.097.152
	resManager.addResType<res::MaterialPBR>  (res::eType::materialPBR, 1024, 64);		//64 page da 1024 = 65.536

	map_of_shape_to_gpushape.setup (allocator, 8192);
	resHandleChainPool.setup (allocator, 8192);
	
	//resource manager
	vtxBufferMan.setup (allocator, gpu);
	idxBufferMan.setup (allocator, gpu);
	listof_knownUID.setup (allocator, 8192);


	//attendo che il loader-thread abbia segnalato che e' partito
	if (!thread::signal_wait (params.hEvent_started, 20000))
	{
		logger::err ("Engine::setup() => error waiting for thread to start\n");
		return false;
	}
	thread::signal_destroy (params.hEvent_started);

	handle_texture_bianca.setInvalid();
	return true;
}

//******************************** 
bool Engine::setup_renderPipe()
{
	if (!renderPipe.priv_setup (allocator, this))
	{
		logger::err ("Engine::setup_renderPipe() => error creating renderPipe\n");
		return false;
	}

	gpu::StageHelper stageHelper;
	stageHelper.setup (gpu, 1024*1024);

	//creo la "tex_bianca"
	u16 dimx = 64;
	u16 dimy = 64;
	u8 *srcDATA = GOSALLOCT(u8*, gos::getScrapAllocator(), dimx*dimy*sizeof(u32));
	memset (srcDATA, 0xFF, dimx*dimy*sizeof(u32));
	priv_texture2D_create_ex (dimx, dimy, 1, eImageFormat::U8_RGBA, eMemAccessMode::onGPU, srcDATA, &handle_texture_bianca, stageHelper, engine::RenderPipe::SPECIAL_TEXTURE__BIANCA);
	GOSFREE(gos::getScrapAllocator(), srcDATA);

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
	frame_num++;

	if (bQuitEngine)
		return false;

	//loader-thread
	priv_flushLoaderThreadMsg();

	//hot-reload delle risorse
	priv_handle_res_hotreload();

	//input
	gos::input::pollEvents();
	input::resolveEvents (gpu->getWindow(), inputCtx, &evtList);
	return true;
}

//******************************** 
void Engine::priv_handle_res_hotreload()
{
	const u64 timenow_msec = gos::getTimeSinceStart_msec();
	u32 N = list_of_res_to_be_hotreloaded.getNElem();
	for (u32 i = 0; i < N; i++)
	{
		if (timenow_msec < list_of_res_to_be_hotreloaded(i).timer_msec)
		{
			//sto aspettando "un po'" prima di fare il free
			continue;
		}

		const sUnloadInfo info = list_of_res_to_be_hotreloaded(i);
		list_of_res_to_be_hotreloaded.remove (i);
		N--;
		i--;
		
		res::Descr *res = res__getDescriptor (info.res_handle);
		
		assert (NULL != res);
		assert (res::eStatus::hot_reload == res->_status);

		//unload della risorsa
		if (NULL != res->on_unload)
			(this->*res->on_unload) (res);

		//la porto in stato "not loaded"
		res__set_status (res, res::eStatus::notLoaded);

		//faccio il release dato che ho incrementato il ref-count durante la chiamata a "hotreload()"
		//Se a seguito del mio release la risorsa e' stata eliminata, non sto a schedulare il load
		if (!res__release (res))
		{
			//schedulo il reload
			const res::Descr *descr;
			res__getOrScheduleLoad (info.res_handle, &descr);
		}
	}
}

//******************************** 
void Engine::priv_flushLoaderThreadMsg()
{
	const u32 nMsg = thread::popMultipleMsg(msgq_2R, loaderMsgList, LOADER_THREAD__NUM_MAX_MESSAGES_TO_READ);
	for (u32 i=0; i<nMsg; i++)
	{
		switch (loaderMsgList[i].what)
		{
		default:
			DBGBREAK;
			break;
		
		case MSG_FROM_LOADER_THREAD__ON_LOAD_CALLBACK:
			//il loader thread mi segnala che ha caricato con successo i file necessari
			//ma ora ha bisogno che io faccia qualcosa e che poi scheduli il continuo dell'operazione di
			//load
			{
				engine::loaders::CallbackData *callback_data = (engine::loaders::CallbackData*)loaderMsgList[i].buffer;
				assert (NULL != callback_data->res->on_loadCallback);

				res::Descr *res = callback_data->res;
				u32 anyError = 0;
				if (! (this->*res->on_loadCallback)(callback_data))
					anyError = 1;

				//informo il thread che sono pronto a proseguire
				thread::pushMsg_on_top (msgq_1W, MSG_FOR_LOADER_THREAD__LOAD_CONTINUE, anyError, callback_data, sizeof(engine::loaders::CallbackData));
			}
			break;

		case MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_OK:
		case MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_KO:
			//il thread mi segnala che una risorsa e' stata caricata
			{
				const bool bLoadOK = (MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_OK == loaderMsgList[i].what);

				res::Descr *res = (res::Descr*)loaderMsgList[i].buffer;
				if (bLoadOK)
				{
					//se esiste, chiamo l'handler
					if (NULL != res->on_afterLoad)
						(this->*res->on_afterLoad)(res);

					if (0 == res->_num_child_not_ready)
						res__set_status (res, res::eStatus::ready);
					else
						res__set_status (res, res::eStatus::loaded);
				}
				else
					res__set_status (res, res::eStatus::error);

				//chi ha chiamato il load, ha anche incrementato il refCount..ora lo decremoento
				res__release(res);

			}
			break;
		}

		thread::deleteMsg (loaderMsgList[i]);
	}
}

//******************************** 
input::eMouseMode Engine::getMouseMode() const
{
	return input::window_getMouseMode(gpu->getWindow());
}

//******************************** 
void Engine::setMouseMode (input::eMouseMode mode)
{
	input::window_setMouseMode (gpu->getWindow(), mode);
}

//******************************** 
const input::MouseStatus* Engine::inputEvent_getMouseStatus() const
{
	return &evtList.getMouseStatus();
}

//******************************** 
const input::sButtonModifier* Engine::inputEvent_getBtnModifier() const
{
	return &evtList.getBtnModifier();
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
			logger::log (eTextColor::white, "engine:: toggle mouse mode\n");
			input::window_toggleMouseMode(gpu->getWindow());
			break;
		}
	}
}

//******************************** 
bool Engine::asset_rebuildAll()
{
	asset2::dbcontext_close (asset_ctx);
	gos::asset2::Builder builder (gpu);
	const bool ret = builder.rebuild_all (GOS_ENGINE__ASSET_HUB_PATH, true);
	asset2::dbcontext_open (GOS_ENGINE__ASSET_HUB_PATH, true, &asset_ctx);
	return ret;
}

//******************************** 
bool Engine::asset_build()
{
	asset2::dbcontext_close (asset_ctx);
	gos::asset2::Builder builder (gpu);
	const bool ret = builder.build (GOS_ENGINE__ASSET_HUB_PATH, true);
	asset2::dbcontext_open (GOS_ENGINE__ASSET_HUB_PATH, true, &asset_ctx);
	return ret;
}

//******************************** 
void Engine::asset_hotreload (asset2::UID uid)
{
	u32 handle_asU32;
	if (listof_knownUID.find (uid, &handle_asU32))
	{
		res::Handle res_handle;
		res_handle.setFromU32 (handle_asU32);
		res__hotreload (res_handle);
	}
}

/**************************************************************** 
 * VTX BUFFER
 *****************************************************************/
bool Engine::vtxBuffer_create (u32 sizeInByte, eMemAccessMode mode, ENGVtxBuffer *out_handle)
{
	asset2::UID uid;
	uid.setInvalid();
	res::VtxBuffer *res = (res::VtxBuffer*)res__createHandle(res::eType::vtx_buffer, res::eStatus::ready, uid, &out_handle->res_handle);
	if (NULL == res)
	{
		logger::err ("Engine::vtxBuffer_create() => can't create handle\n");
		return false;
	}

	return gpu->vertexBuffer_create (sizeInByte, mode, &res->vbHandle);
}

void Engine::internal__vtxBuffer_on_afterCreate (void *resIN)
{
	//asset_logger->log ("internal__vtxBuffer_on_afterCreate\n");
	res::VtxBuffer *res = (res::VtxBuffer*)resIN;
	res->vbHandle.setInvalid();
}

void Engine::internal__vtxBuffer_on_destroy (void *resIN)
{
	//asset_logger->log ("internal__vtxBuffer_on_destroy\n");
	res::VtxBuffer *res = (res::VtxBuffer*)resIN;
	gpu->deleteResource (res->vbHandle);
}


/**************************************************************** 
 * IDX BUFFER
 *****************************************************************/
bool Engine::idxBuffer_create (u32 sizeInByte, eMemAccessMode mode, ENGIdxBuffer *out_handle)
{
	asset2::UID uid;
	uid.setInvalid();
	res::IdxBuffer *res = (res::IdxBuffer*)res__createHandle(res::eType::idx_buffer, res::eStatus::ready, uid, &out_handle->res_handle);
	if (NULL == res)
	{
		logger::err ("Engine::idxBuffer_create() => can't create handle\n");
		return false;
	}
	
	return gpu->indexBuffer_create (sizeInByte, mode, &res->ibHandle);
}

void Engine::internal__idxBuffer_on_afterCreate (void *resIN)
{
	asset_logger->log ("internal__idxBuffer_on_afterCreate\n");
	res::IdxBuffer *res = (res::IdxBuffer*)resIN;
	res->ibHandle.setInvalid();
}

void Engine::internal__idxBuffer_on_destroy (void *resIN)
{
	//asset_logger->log ("internal__idxBuffer_on_destroy\n");
	res::IdxBuffer *res = (res::IdxBuffer*)resIN;
	gpu->deleteResource (res->ibHandle);
}


/**************************************************************** 
 * VTX SHADER
 *****************************************************************/
bool Engine::vtxshader_createFromFile (const char *filename, const char *mainFnName, ENGVtxShader *out_handle)
{
	asset2::UID uid;
	uid.setInvalid();
	res::Shader *res = (res::Shader*)res__createHandle(res::eType::vtx_shader, res::eStatus::ready, uid, &out_handle->res_handle);
	if (NULL == res)
	{
		logger::err ("Engine::vtxshader_createFromFile() => can't create handle\n");
		return false;
	}

	return gpu->vtxshader_createFromFile (filename, mainFnName, &res->shaderHandle);
}

bool Engine::vtxshader_createFromMemory (const void *bufferIN, u32 bufferSize, const char *mainFnName, ENGVtxShader *out_handle)
{
	asset2::UID uid;
	uid.setInvalid();
	res::Shader *res = (res::Shader*)res__createHandle(res::eType::vtx_shader, res::eStatus::ready, uid, &out_handle->res_handle);
	if (NULL == res)
	{
		logger::err ("Engine::vtxshader_createFromMemory() => can't create handle\n");
		return false;
	}

	return gpu->vtxshader_createFromMemory (bufferIN, bufferSize, mainFnName, &res->shaderHandle);
}

void Engine::internal__vtxshader_on_afterCreate (void *resIN)
{
	//asset_logger->log ("internal__vtxshader_on_afterCreate\n");
	res::Shader *res = (res::Shader*)resIN;
	res->shaderHandle.setInvalid();
}

void Engine::internal__vtxshader_on_destroy (void *resIN)
{
	//asset_logger->log ("internal__vtxshader_on_destroy\n");
	res::Shader *res = (res::Shader*)resIN;
	gpu->deleteResource (res->shaderHandle);
}

void Engine::internal__vtxshader_on_unload (void *resIN)
{
	//asset_logger->log ("internal__vtxshader_on_unload\n");
	res::Shader *res = (res::Shader*)resIN;
	gpu->deleteResource (res->shaderHandle);
	res->shaderHandle.setInvalid();
}

/**************************************************************** 
 * PXL SHADER
 *****************************************************************/
bool Engine::pxlshader_createFromFile (const char *filename, const char *mainFnName, ENGPxlShader *out_handle)
{
	asset2::UID uid;
	uid.setInvalid();
	res::Shader *res = (res::Shader*)res__createHandle(res::eType::pxl_shader, res::eStatus::ready, uid, &out_handle->res_handle);
	if (NULL == res)
	{
		logger::err ("Engine::pxlshader_createFromFile() => can't create handle\n");
		return false;
	}

	return gpu->pxlshader_createFromFile (filename, mainFnName, &res->shaderHandle);
}

bool Engine::pxlshader_createFromMemory (const void *bufferIN, u32 bufferSize, const char *mainFnName, ENGPxlShader *out_handle)
{
	asset2::UID uid;
	uid.setInvalid();
	res::Shader *res = (res::Shader*)res__createHandle(res::eType::pxl_shader, res::eStatus::ready, uid, &out_handle->res_handle);
	if (NULL == res)
	{
		logger::err ("Engine::pxlshader_createFromMemory() => can't create handle\n");
		return false;
	}

	return gpu->pxlshader_createFromMemory (bufferIN, bufferSize, mainFnName, &res->shaderHandle);
}

void Engine::internal__pxlshader_on_afterCreate (void *resIN)
{
	//asset_logger->log ("internal__pxlshader_on_afterCreate\n");
	res::Shader *res = (res::Shader*)resIN;
	res->shaderHandle.setInvalid();
}

void Engine::internal__pxlshader_on_destroy (void *resIN)
{
	//asset_logger->log ("internal__pxlshader_on_destroy\n");
	res::Shader *res = (res::Shader*)resIN;
	gpu->deleteResource (res->shaderHandle);
}

void Engine::internal__pxlshader_on_unload (void *resIN)
{
	//asset_logger->log ("internal__pxlshader_on_unload\n");
	res::Shader *res = (res::Shader*)resIN;
	gpu->deleteResource (res->shaderHandle);
	res->shaderHandle.setInvalid();
}


/**************************************************************** 
 * PIPELINE
 *****************************************************************/
bool Engine::pipeline_create (const gpu::Pipeline_def &def, ENGPipeline *out_handle)
{
	asset2::UID uid;
	uid.setInvalid();
	res::Pipeline *res = (res::Pipeline*)res__createHandle(res::eType::pipeline, res::eStatus::ready, uid, &out_handle->res_handle);
	if (NULL == res)
	{
		logger::err ("Engine::pipeline_create() => can't create handle\n");
		return false;
	}

	return gpu->pipeline_createNew (def, &res->pipeHandle); 
}

void Engine::internal__pipeline_on_afterCreate (void *resIN)
{
	//asset_logger->log ("internal__pipeline_on_afterCreate\n");
	res::Pipeline *res = (res::Pipeline*)resIN;
	res->pipeHandle.setInvalid();
}

 void Engine::internal__pipeline_on_destroy (void *resIN)
{
	//asset_logger->log ("internal__pipeline_on_destroy\n");
	res::Pipeline *res = (res::Pipeline*)resIN;
	gpu->deleteResource (res->pipeHandle);
}

void Engine::internal__pipeline_on_unload (void *resIN)
{
	//asset_logger->log ("internal__pipeline_on_unload\n");
	res::Pipeline *res = (res::Pipeline*)resIN;
	gpu->deleteResource (res->pipeHandle);
	res->pipeHandle.setInvalid();
}


/**************************************************************** 
 * TEXTURE 2D
 *****************************************************************/
bool Engine::texture2D_create (u16 dimx, u16 dimy, u8 nMipMap, eImageFormat fmt, eMemAccessMode memAccessMode, const void *srcDATA, ENGTexture *out_handle, gpu::StageHelper &stageHelper)
{
	if (priv_texture2D_create_ex (dimx, dimy, nMipMap, fmt, memAccessMode, srcDATA, out_handle, stageHelper, u32MAX))
		return true;
	logger::err ("Engine::texture2D_create() => can't create handle\n");
	return false;
}

bool Engine::priv_texture2D_create_ex (u16 dimx, u16 dimy, u8 nMipMap, eImageFormat fmt, eMemAccessMode memAccessMode, const void *srcDATA, ENGTexture *out_handle, gpu::StageHelper &stageHelper, u32 desired_texture_index)
{
	asset2::UID uid;
	uid.setInvalid();
	res::Texture2d *res = (res::Texture2d*)res__createHandle(res::eType::texture_2d, res::eStatus::ready, uid, &out_handle->res_handle);
	if (NULL == res)
		return false;

	if (gpu->texture_create2D (dimx, dimy, nMipMap, fmt, memAccessMode, srcDATA, &res->texHandle, stageHelper))
	{
		priv_texture2D__add_to_mega_array (res, desired_texture_index);
		return true;
	}
	return false;
}

bool Engine::texture2D_create (const gos::Image *im, u8 srcTextureNum, eMemAccessMode memAccessMode, ENGTexture *out_handle, gpu::StageHelper &stageHelper)
{
	asset2::UID uid;
	uid.setInvalid();
	res::Texture2d *res = (res::Texture2d*)res__createHandle(res::eType::texture_2d, res::eStatus::ready, uid, &out_handle->res_handle);
	if (NULL == res)
	{
		logger::err ("Engine::texture2D_create() => can't create handle\n");
		return false;
	}

	if (gpu->texture_create2D (im, srcTextureNum, memAccessMode, &res->texHandle, stageHelper))
	{
		priv_texture2D__add_to_mega_array (res);
		return true;
	}

	DBGBREAK;
	return false;
}

void Engine::internal__texture2D_on_afterCreate (void *resIN)
{
	//questa fn viene chiamata subito dopo res_createHandle()

	//asset_logger->log ("internal__texture2D_on_afterCreate\n");
	res::Texture2d *res = (res::Texture2d*)resIN;
	res->texHandle.setInvalid();
	res->index = u32MAX;
}

void Engine::internal__texture2D_on_destroy (void *resIN)
{
	//chiamata da res_destroy() solo se la res e' in stato ready

	res::Texture2d *res = (res::Texture2d*)resIN;
	//asset_logger->log ("internal__texture2D_on_destroy (tex-index=%d)\n", res->index);

	assert (res->texHandle.isValid());
	priv_texture2D__remove_from_mega_array (res);
	gpu->deleteResource (res->texHandle);    
}

void Engine::internal__texture2D_on_afterLoad (void *resIN)
{
	//chiamata da this dopo che la risorsa e' stata creata e schedulata per il caricamento da disco.
	//Una volta che il caricamento e' terminato con successo, questo handler viene invocato

	res::Texture2d *res = (res::Texture2d*)resIN;
	//asset_logger->log ("internal__texture2D_on_afterLoad (tex-index=%d)\n", res->index);

	assert (res->texHandle.isValid());
	priv_texture2D__add_to_mega_array(res);
}

void Engine::internal__texture2D_on_unload (void *resIN)
{
	res::Texture2d *res = (res::Texture2d*)resIN;
	//asset_logger->log ("internal__texture2D_on_unload (tex-index=%d)\n", res->index);

	
	priv_texture2D__remove_from_mega_array (res);
	if (res->texHandle.isValid())
	{
		gpu->deleteResource (res->texHandle);
		res->texHandle.setInvalid();
	}
}

bool Engine::internal__texture2D_loadCallback (void *callback_dataIN)
{
	engine::loaders::CallbackData *callback_data = (engine::loaders::CallbackData*)callback_dataIN;
	res::Texture2d *res = reinterpret_cast<res::Texture2d*> (callback_data->res);
	
	gos::Image image;
	image.p = callback_data->user_data_pt;

	//asset_logger->log ("internal__texture2D_loadCallback [%08X]\n", res->_descr.handle.viewAsU32());
	return gpu->texture_create2D (&image, 0, eMemAccessMode::onGPU, &res->texHandle, stageHelper);
}

void Engine::priv_texture2D__add_to_mega_array (res::Texture2d *res, u32 desired_index)
{
	assert (u32MAX == res->index);
	if (u32MAX != desired_index)
		res->index = renderPipe.internal__texture_add_reserved (res->texHandle, desired_index);
	else
		res->index = renderPipe.internal__texture_add_if_dont_exists (res->texHandle);

	//asset_logger->log ("priv_texture2D__add_to_mega_array (tex-index=%d)\n", res->index);
}

void Engine::priv_texture2D__remove_from_mega_array (res::Texture2d *res)
{
	//asset_logger->log ("priv_texture2D__remove_from_mega_array (tex-index=%d)\n", res->index);

	renderPipe.internal__texture_remove (res->texHandle);
	res->index = u32MAX;
}

/**************************************************************** 
 * SHAPE
 *****************************************************************/
bool Engine::shape_create (const VtxLayout &vtxLayout, u32 numVtx, u32 numIdx, ENGShape *out_handle)
{
	asset2::UID uid;
	uid.setInvalid();
	res::Shape *res = (res::Shape*)res__createHandle(res::eType::shape, res::eStatus::ready, uid, &out_handle->res_handle);
	if (NULL == res)
	{
		logger::err ("Engine::shape_create() => can't create handle\n");
		return false;
	}

	if (!shape::shapeAlloc (allocator, vtxLayout, numVtx, numIdx, &res->shape))
	{
		logger::err ("Engine::shape_create() => error during shapeAlloc\n");
		return false;
	}

	return true;
}

void Engine::internal__shape_on_afterCreate (void *resIN)
{
	//asset_logger->log ("internal__shape_on_afterCreate\n");
	res::Shape *res = (res::Shape*)resIN;
	res->shape.reset();
}

void Engine::internal__shape_on_destroy (void *resIN)
{
	//asset_logger->log ("internal__shape_on_destroy\n");
	res::Shape *res = (res::Shape*)resIN;
	shape::shapeFree (allocator, &res->shape);
}

void Engine::internal__shape_on_unload (void *resIN)
{
	//asset_logger->log ("internal__shape_on_unload\n");
	res::Shape *res = (res::Shape*)resIN;
	shape::shapeFree (allocator, &res->shape);
	res->shape.reset();
}



/**************************************************************** 
 * GPU SHAPE
 *****************************************************************/
bool Engine::GPUShape_create (ENGShape handle_shapeSRC, ENGGPUShape *out_handle)
{
	//recupero le info sulla shaperSRC
	res::Descr *res_shapeSRC = res__getDescriptor(handle_shapeSRC.res_handle);
	if (NULL == res_shapeSRC)
	{
		logger::err ("Engine::GPUShape_create() => src shape does not exists\n");
		return false;
	}

	//Se io sono gia' stata creata, allora sono certamente uno dei padri di handle_shapeSRC.
	//Se trovo un padre di tipo GPUShape, sono io
	{
		res::HandleChain *p = res_shapeSRC->padri;
		while (p)
		{
			if ((u32)res::eType::gpu_shape == p->res->handle.get_value_TYPE())
			{
				out_handle->setFromU32 (p->res->handle.viewAsU32());
				p->res->refCount++;
				res__printInfo(p->res, "refcount++");
				return true;
			}
			p = p->next;
		}
	}

	//devo creare una nuova GPUShape che diventa padre di handle_shapeSRC
	//Se handle_shapeSRC e' ready o loaded, allora gpu_shape e' a sua volta ready, altrimenti vuol
	//dire che handle_shapeSRC non e' stata ancora caricata e quindi devo posticipare la creazione della gpu_shape
	asset2::UID uid;
	uid.setInvalid();
	res::GPUShape *res = (res::GPUShape*)res__createHandle(res::eType::gpu_shape, res::eStatus::notLoaded, uid, &out_handle->res_handle);
	if (NULL == res)
	{
		logger::err ("Engine::GPUShape_create() => can't create handle\n");
		return false;
	}
	res->handle_shape = handle_shapeSRC;

	//io divento padre di handle_shapeSRC
	res__addChild (&res->_descr, res_shapeSRC);
	res_shapeSRC->refCount++;
	res__printInfo(res_shapeSRC, "refcount++");

	return true;
}

bool Engine::GPUShape_create (const gos::Shape *shape, gpu::StageHelper &stageHelper, ENGGPUShape *out_handle)
{
	asset2::UID uid;
	uid.setInvalid();
	res::GPUShape *res = (res::GPUShape*)res__createHandle(res::eType::gpu_shape, res::eStatus::ready, uid, &out_handle->res_handle);
	if (NULL == res)
	{
		logger::err ("Engine::GPUShape_create() => can't create handle\n");
		return false;
	}

	return priv_GPUShape_create (shape, stageHelper, res);;
}

bool Engine::priv_GPUShape_create (const gos::Shape *shape, gpu::StageHelper &stageHelper, res::GPUShape *res)
{
	assert (NULL != res);
	u32 byteNeeded;

	//vtxbuffer
	if (shape->numVtx)
	{
		const u32 sizeOfAVertex = gos::shape::calcSizeOfAVertex(shape->vtxLayout);
		byteNeeded = sizeOfAVertex * shape->numVtx;
		vtxBufferMan.reserve (byteNeeded, &res->alloc_vtxbuf_offset, &res->alloc_vtxbuf_size, &res->vbHandle);
		res->vtxStart = res->alloc_vtxbuf_offset / sizeOfAVertex;
		res->numVertex = shape->numVtx;
	}

	//idxBuffer    
	if (shape->numIdx)
	{
		const u32 sizeOfAnIndex = sizeof(u16);
		byteNeeded = sizeOfAnIndex * shape->numIdx;
		idxBufferMan.reserve (byteNeeded, &res->alloc_idxbuf_offset, &res->alloc_idxbuf_size, &res->ibHandle);
		res->indexStart = res->alloc_idxbuf_offset / sizeOfAnIndex;
		res->numIndices = shape->numIdx;
	}

	stageHelper.begin()
		.mem_to_buffer (shape->vtxBuffer, shape->numVtx * shape::calcSizeOfAVertex(shape->vtxLayout), res->vbHandle, res->alloc_vtxbuf_offset)
		.mem_to_buffer (shape->idxBuffer, shape->numIdx * sizeof(u16), res->ibHandle, res->alloc_idxbuf_offset)
		.submit();
	return true;
}

void Engine::release (ENGGPUShape &handle)
{
	res::Descr *res = res__getDescriptor(handle.res_handle);
	if (NULL != res)
	{
		gos::ENGShape handle_shape = ((res::GPUShape*)res)->handle_shape;
		if (res__release(res))
		{
			if (handle_shape.isValid())
				map_of_shape_to_gpushape.remove (handle_shape);
		}
	}
	handle.res_handle.setInvalid(); 
		
}

void Engine::internal__GPUShape_reset (res::GPUShape *res)
{
	res->handle_shape.setInvalid();
	res->vbHandle.setInvalid();
	res->ibHandle.setInvalid();
	res->numIndices = 0;
	res->numVertex=0;
	res->alloc_vtxbuf_offset = 0;
	res->alloc_vtxbuf_size = 0;
	res->alloc_idxbuf_offset = 0;
	res->alloc_idxbuf_size = 0;	
}

void Engine::internal__GPUShape_on_afterCreate (void *resIN)
{
	res::GPUShape *res = (res::GPUShape*)resIN;
	internal__GPUShape_reset(res);
	//asset_logger->log ("internal__GPUShape_on_afterCreate [handle=%08X]\n", res->_descr.handle.viewAsU32());
}

void Engine::internal__GPUShape_on_destroy (void *resIN)
{
	res::GPUShape *res = (res::GPUShape*)resIN;

	//asset_logger->log ("internal__GPUShape_on_destroy [handle=%08X]\n", res->_descr.handle.viewAsU32());


	if (res->numVertex)
		vtxBufferMan.release (res->vbHandle, res->alloc_vtxbuf_offset, res->alloc_vtxbuf_size);

	if (res->numIndices)
		idxBufferMan.release (res->ibHandle, res->alloc_idxbuf_offset, res->alloc_idxbuf_size);

	//unmappo la coppia <shape, gpu_shape>
	if (res->handle_shape.isValid())
		map_of_shape_to_gpushape.remove (res->handle_shape);
}

void Engine::internal__GPUShape_on_unload (void *resIN)
{
	res::GPUShape *res = (res::GPUShape*)resIN;

	asset_logger->log ("internal__GPUShape_on_unload [handle=%08X]\n", res->_descr.handle.viewAsU32());

	if (res->numVertex)
		vtxBufferMan.release (res->vbHandle, res->alloc_vtxbuf_offset, res->alloc_vtxbuf_size);

	if (res->numIndices)
		idxBufferMan.release (res->ibHandle, res->alloc_idxbuf_offset, res->alloc_idxbuf_size);

	internal__GPUShape_reset(res);
}

bool Engine::internal__GPUShape_on_loadCallback(void *callback_dataIN)
{
	engine::loaders::CallbackData *callback_data = (engine::loaders::CallbackData*)callback_dataIN;
	res::GPUShape *res = reinterpret_cast<res::GPUShape*>(callback_data->res);

	//asset_logger->log ("internal__GPUShape_on_loadCallback [%08X]\n", res->_descr.handle.viewAsU32());

	//mio figlio deve essere una Shape
	assert (NULL != res->_descr.figli);
	const res::Shape *res_shape = (res::Shape*)res->_descr.figli->res;

	bool ret = priv_GPUShape_create (&res_shape->shape, stageHelper, res);
	return ret;
}

/**************************************************************** 
 * SKELETON
 *****************************************************************/
bool Engine::skeleton_create (const Skeleton &sk, ENGSkeleton *out_handle)
{
	if (skeleton_createFromMemory (sk.blob, skeleton::get_blob_size(sk), out_handle))
		return true;

	logger::err ("Engine::skeleton_create() => error creating skeleton\n");
	return false;
}

bool Engine::skeleton_createFromMemory (const u8 *buffer, u32 sizeof_buffer, ENGSkeleton *out_handle)
{
	asset2::UID uid;
	uid.setInvalid();
	res::Skeleton *res = (res::Skeleton*)res__createHandle(res::eType::skeleton, res::eStatus::ready, uid, &out_handle->res_handle);
	if (NULL == res)
	{
		logger::err ("Engine::skeleton_createFromMemory() => can't create handle\n");
		return false;
	}

	const u32 n = skeleton::deserialize (buffer, sizeof_buffer, allocator, &res->skeleton);
	if (0 == n)
	{
		logger::err ("Engine::skeleton_createFromMemory() => error during shapeAlloc\n");
		return false;
	}
	return true;
}

void Engine::internal__skeleton_on_afterCreate (void *resIN)
{
	//asset_logger->log ("internal__skeleton_on_afterCreate\n");
	res::Skeleton *res = (res::Skeleton*)resIN;
	res->skeleton.reset();
}

void Engine::internal__skeleton_on_destroy (void *resIN)
{
	//asset_logger->log ("internal__skeleton_on_destroy\n");
	res::Skeleton *res = (res::Skeleton*)resIN;
	skeleton::free (res->skeleton);
}

void Engine::internal__skeleton_on_unload (void *resIN)
{
	//asset_logger->log ("internal__skeleton_on_unload\n");
	internal__skeleton_on_destroy(resIN);

	res::Skeleton *res = (res::Skeleton*)resIN;
	res->skeleton.reset();
}


/**************************************************************** 
 * MODEL 3d
 *****************************************************************/
gos::Model*	Engine::model_create (ENGSkeleton handle_skeleton, u16 num_shape, u16 num_material, u16 num_meshes, ENGModel3d *out_handle)
{
	asset2::UID uid;
	uid.setInvalid();
	res::Model3d *res = (res::Model3d*)res__createHandle(res::eType::model_3d, res::eStatus::ready, uid, &out_handle->res_handle);
	if (NULL == res)
	{
		logger::err ("Engine::model_create() => can't create handle\n");
		return NULL;
	}

	if (!model::alloc (allocator, num_shape, num_material, num_meshes, &res->model))
	{
		logger::err ("Engine::model_create() => can't allocate model\n");
		return NULL;
	}

	model::set_skeleton(res->model, handle_skeleton);
	return &res->model;
}

void Engine::internal__model_on_afterCreate (void *resIN)
{
	//asset_logger->log ("internal__model_on_afterCreate\n");
	res::Model3d *res = (res::Model3d*)resIN;
	res->model.reset();
}

void Engine::internal__model_on_destroy (void *resIN)
{
	//asset_logger->log ("internal__model_on_destroy\n");
	res::Model3d *res = (res::Model3d*)resIN;
	model::free(res->model);
}

void Engine::internal__model_on_unload (void *resIN)
{
	//asset_logger->log ("internal__model_on_unload\n");
	res::Model3d *res = (res::Model3d*)resIN;
	model::free(res->model);
	res->model.reset();
}

bool Engine::internal__model_on_loadCallback (void *callback_dataIN)
{
	engine::loaders::CallbackData *callback_data = (engine::loaders::CallbackData*)callback_dataIN;
	const u8 *buffer = reinterpret_cast<const u8*>(callback_data->user_data_pt);
	const u32 fsize = (u32)callback_data->user_data_1;
	res::Model3d *res_model = reinterpret_cast<res::Model3d*>(callback_data->res);

	asset_logger->log ("internal__model_on_loadCallback [%08X]\n", res_model->_descr.handle.viewAsU32());
	asset_logger->inc_indent();

	gos::BufferR reader;
	reader.setup (buffer, fsize);
	reader.readU32();	//skip magica
	reader.readU64();	//skip skeleton uid

	bool ret = true;
	const u32 num_shapes = reader.readU32();
	const u32 start_of_list_of_shape_uid = reader.tell();
	reader.moveCursorTo(start_of_list_of_shape_uid);
	for (u32 i=0; i<num_shapes; i++)
	{
		asset2::UID uid_shape;
		uid_shape._uid = reader.readU64();

		ENGShape handle_shape;
		res::Shape *res_shape;
		if (!internal__getResFromUID(uid_shape, &res_shape, &handle_shape))
		{
			logger::err ("engine::internal__model_on_loadCallback() => can't find shape for UID %016" PRIX64 "\n", uid_shape._uid);
			ret = false;
			break;
		}

		//creo la GPUshape (se non esise gia')
		//handle_gpuShape avra' refCount==1 se la gpushape e' stata creata ora, altrimenti 
		//il suo refCount viene incrementato di 1 direttamente da GPUShape_create()
		//Non c'e bisogno quindi che io incrementi il refCount per significare che possiedo questa gpushape
		ENGGPUShape handle_gpuShape;
		if (!GPUShape_create (handle_shape, &handle_gpuShape))
		{
			logger::err ("engine::internal__model_on_loadCallback() => error creating GPUShape from shape %016" PRIX64 "\n", uid_shape._uid);
			return false;
		}
		model::set_gpushape (res_model->model, i, handle_gpuShape);

		//GPU shape diventa figlia di model
		res::Descr *res = res__getDescriptor(handle_gpuShape.res_handle);
		assert (NULL != res);
		res__addChild (&res_model->_descr, res);

		//carico la GPUShape
		res__scheduleLoadIfNeeded (res, 0);
	}

	asset_logger->dec_indent();
	return ret;
}



/**************************************************************** 
 * MODEL INSTANCE
 *****************************************************************/
bool Engine::modelinst_create (ENGModel3d handle_modelSRC, ENGModel3dInst *out_handle)
{
	//recupero le info sulla modelSRC,
	res::Descr *res_modelSRC = res__getDescriptor(handle_modelSRC.res_handle);
	if (NULL == res_modelSRC)
	{
		logger::err ("Engine::modelinst_create() => invalid handle_modelSRC\n");
		return false;
	}

	//creo istanza
	asset2::UID uid;
	uid.setInvalid();
	res::Model3dInst *res = (res::Model3dInst*)res__createHandle (res::eType::model_instance, res::eStatus::notLoaded, uid, &out_handle->res_handle);
	if (NULL == res)
	{
		logger::err ("Engine::modelinst_create() => can't create handle\n");
		return false;
	}

	//modelSRC diventa mio figlio e io divento uno dei suoi padri
	asset_logger->inc_indent();
		res__addChild (&res->_descr, res_modelSRC);
		res_modelSRC->refCount++;
		res__printInfo(res_modelSRC, "refcount++");
	asset_logger->dec_indent();

	return true;
}

void Engine::internal__modelinst_on_afterCreate (void *resIN)
{
	//asset_logger->log ("internal__modelinst_on_afterCreate\n");
	res::Model3dInst *res = (res::Model3dInst*)resIN;
	res->minst.reset();
	res->matW.identity();
}

void Engine::internal__modelinst_on_destroy (void *resIN)
{
	//asset_logger->log ("internal__modelinst_on_destroy\n");
	res::Model3dInst *res = (res::Model3dInst*)resIN;
	res->minst.free();
}

void Engine::internal__modelinst_on_unload (void *resIN)
{
	res::Model3dInst *res = (res::Model3dInst*)resIN;
	res->minst.free();
	res->minst.reset();
}

bool Engine::internal__modelinst_on_loadCallback (void *callback_dataIN)
{
	engine::loaders::CallbackData *callback_data = (engine::loaders::CallbackData*)callback_dataIN;
	res::Model3dInst *res = reinterpret_cast<res::Model3dInst*>(callback_data->res);

	bool ret = false;
	asset_logger->log ("internal__modelinst_on_loadCallback [%08X]\n", res->_descr.handle.viewAsU32());
	asset_logger->inc_indent();
	{
		//mio figlio e' un Model
		assert (NULL != res->_descr.figli);
		const res::Model3d *res_model = reinterpret_cast<const res::Model3d*> (res->_descr.figli->res);
		

		//il model deve essere "ready" altrimenti non posso proseguire
		if (res::eStatus::ready == res_model->_descr._status)
		{
			model::Reader mr (&res_model->model);

			//recupero lo skeleton
			const res::Skeleton *res_skeleton;
			if (!get (mr.skeleton_get_handle(), &res_skeleton))
			{
				//lo skeleton deve esistere ed essere "loaded"
				asset_logger->err ("internal__modelinst_on_loadCallback => invalid skeleton handle\n");
				ret = false;
			}
			else
			{
				skeleton::Reader sr(&res_skeleton->skeleton);


				ModelInstance *mi = &res->minst;
				mi->allocator = this->allocator;

				mi->num_bones = sr.bone_get_num();
				mi->model_listof_bones = sr.bone_get_by_index(0);

				mi->num_meshes = mr.mesh_get_num();
				mi->listof_meshes = mr.mesh_get_by_index(0);
				
				mi->num_gpushapes = mr.gpushape_get_num();
				mi->listof_gpushapes = mr.gpushape_get_pt_to_list();

				mi->listof_bones = GOSALLOCT(Bone*, allocator, sizeof(Bone) * mi->num_bones);	
				memcpy (mi->listof_bones, mi->model_listof_bones, sizeof(Bone) * mi->num_bones);

				mi->num_materials = mr.material_get_num();
				mi->listof_materials = mr.material_get_pt_to_list();

				priv_modelinst_applyTransform_ric (mi->model_listof_bones, mi->listof_bones, 0, res->matW);

				ret = true;
			}
		}
		else
		{
			if (res::eStatus::error == res_model->_descr._status)
			{
				//C'e' poco da fare..
				DBGBREAK;
				ret = false;
			}
			else
			{
				//devo rischedulare questo load in attesa che Model diventi ready
				callback_data->reschedule_load_at_time_msec = gos::getTimeSinceStart_msec() + 100;
				ret = true;
			}
		}
	}
	asset_logger->dec_indent();
	return ret;
}


void Engine::modelinst_applyTransform (ENGModel3dInst handle, const mat4x4f &matW)
{
	res::Model3dInst *res = (res::Model3dInst*)res__getDescriptor(handle.res_handle);
	if (NULL == res)
		return;
	res->matW = matW;
	if (res::eStatus::ready == res->_descr._status)
	{
		priv_modelinst_applyTransform_ric (res->minst.model_listof_bones, res->minst.listof_bones, 0, matW);
	}
}

void Engine::priv_modelinst_applyTransform_ric (const gos::Bone *model__listof_bones, gos::Bone *instance__listof_bones, u32 boneIndex, const mat4x4f &parent_matW) const
{
	Bone *instance_bone = &instance__listof_bones[boneIndex];
	instance_bone->matrix = parent_matW * model__listof_bones[boneIndex].matrix;
	
	u32 childrenIndex = instance_bone->firstChildIndex;
	while (0xFF != childrenIndex)
	{
		priv_modelinst_applyTransform_ric (model__listof_bones, instance__listof_bones, childrenIndex, instance_bone->matrix);
		childrenIndex = instance__listof_bones[childrenIndex].sigblinIndex;
	}
}




/**************************************************************** 
 * MATERIAL PBR
 *****************************************************************/
bool Engine::materialPBR_create (ENGMaterialPBR *out_handle)
{
	asset2::UID uid;
	uid.setInvalid();
	res::MaterialPBR *res = (res::MaterialPBR*)res__createHandle(res::eType::materialPBR, res::eStatus::ready, uid, &out_handle->res_handle);
	if (NULL == res)
	{
		logger::err ("Engine::materialPBR_create() => can't create handle\n");
		return false;
	}

	return true;
}

bool Engine::internal__materialPBR_update_renderer_binding (ENGMaterialPBR handle, u8 renderer_uid, u32 data)
{
	res::MaterialPBR *res = (res::MaterialPBR*)res__getDescriptor(handle.res_handle);
	if (NULL == res)
		return false;
	if (res::eStatus::ready == res->_descr._status)
	{
		assert (renderer_uid < res::MaterialPBR::NUM_MAX_RENDERER);
		res->renderer_bindings[renderer_uid] = data;
		return true;
	}

	return false;
}

void Engine::internal__materialPBR_on_afterCreate (void *resIN)
{
	//asset_logger->log ("materialPBR_on_afterCreate\n");
	res::MaterialPBR *res = (res::MaterialPBR*)resIN;
	
	memset (res->renderer_bindings, 0xff, sizeof(res->renderer_bindings));
	res->set_default_material_params();
}

void Engine::internal__materialPBR_on_destroy (void *resIN)
{
	//asset_logger->log ("internal__materialPBR_on_destroy\n");
	//res::MaterialPBR *res = (res::MaterialPBR*)resIN;

	//TODO: segnalare alle renderPipe che il materiale e' morto

}






