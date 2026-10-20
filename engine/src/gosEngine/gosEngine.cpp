#include "gosEngine.h"
#include "../gosAsset2/gosAsset2Builder.h"
#include "../gosAsset2/assetFile/gosAssetFile_materialPBR.h"

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


	//loader thread
	thread::pushMsg (res2__msgq_1W, RES2__MSG_FOR_LOADER_THREAD__DIE, 0);
	thread::wait_end (res2__hThreadLoader);
	for (u8 i=0; i<4; i++)
	{
		res2__flushLoaderThreadMsg();
	}
	thread::deleteMsgQ (res2__msgq_1R, res2__msgq_1W);
	thread::deleteMsgQ (res2__msgq_2R, res2__msgq_2W);


	//renderPipe
	release(handle_texture_bianca);
	renderPipe.priv_unsetup();

	//manager vari
	vtxBufferMan.unsetup();
	idxBufferMan.unsetup();
	stageHelper.unsetup();

	//res2 unsetup
	{
		res2MemManager.unsetup();
		res2_known_signatureUID_HashMap.unsetup();
		res2_known_assetUID_HashMap.unsetup();
		res2::unsetup();
	}


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
	

	//res2 setup
	//creo 2 code, una che uso per mandare msg da this al thread e una per mandare
	//msg dal thread a this
	sLoaderThreadInitParams res2__thread_params;
	{
		res2::setup (allocator, asset_logger);
		res2MemManager.setup (this->allocator);

		res2_known_signatureUID_HashMap.setup (allocator, 8192);
		res2_known_assetUID_HashMap.setup (allocator, 8192);

		memset (res2FNList_onFileLoaded, 0, sizeof(res2FNList_onFileLoaded));
		memset (res2FNList_onDestroy, 0, sizeof(res2FNList_onDestroy));
		memset (res2FNList_onHotReload, 0, sizeof(res2FNList_onHotReload));

		res2__register_resource (res2::eResType::vtx_buffer, NULL, &Engine::res2__vtxBufferRes_callback_on_destroy, NULL);
		res2__register_resource (res2::eResType::idx_buffer, NULL, &Engine::res2__idxBufferRes_callback_on_destroy, NULL);
		res2__register_resource (res2::eResType::texture_2d, &Engine::res2__textureRes_callback_on_load,
									&Engine::res2__textureRes_callback_on_destroy, 
									&Engine::res2__textureRes_callback_on_hot_reload);
		res2__register_resource (res2::eResType::materialPBR, &Engine::res2__materialPBR_callback_on_load,
									&Engine::res2__materialPBR_callback_on_destroy, 
									&Engine::res2__materialPBR_callback_on_hot_reload);	
		res2__register_resource (res2::eResType::vtx_shader, &Engine::res2__vtxShaderRes_callback_on_load,
									&Engine::res2__vtxShaderRes_callback_on_destroy,
									&Engine::res2__vtxShaderRes_callback_on_hot_reload);
		res2__register_resource (res2::eResType::pxl_shader, &Engine::res2__pxlShaderRes_callback_on_load,
									&Engine::res2__pxlShaderRes_callback_on_destroy,
									&Engine::res2__pxlShaderRes_callback_on_hot_reload);
		res2__register_resource (res2::eResType::pipeline, &Engine::res2__pipelineRes_callback_on_load,
									&Engine::res2__pipelineRes_callback_on_destroy,
									&Engine::res2__pipelineRes_callback_on_hot_reload);
		res2__register_resource (res2::eResType::gpu_shape, &Engine::res2__gpushapeRes_callback_on_load,
									&Engine::res2__gpushapeRes_callback_on_destroy,
									&Engine::res2__gpushapeRes_callback_on_hot_reload);
		res2__register_resource (res2::eResType::skeleton, &Engine::res2__skeletonRes_callback_on_load,
									&Engine::res2__skeletonRes_callback_on_destroy,
									&Engine::res2__skeletonRes_callback_on_hot_reload);
		res2__register_resource (res2::eResType::model_3d, &Engine::res2__modelRes_callback_on_load,
									&Engine::res2__modelRes_callback_on_destroy,
									&Engine::res2__modelRes_callback_on_hot_reload);


		//thread loader
		thread::createMsgQ (&res2__msgq_1R, &res2__msgq_1W);
		thread::createMsgQ (&res2__msgq_2R, &res2__msgq_2W);

		res2__thread_params.msgqR = res2__msgq_1R;
		res2__thread_params.msgqW = res2__msgq_2W;
		res2__thread_params.logger = asset_logger;
		res2__thread_params.gpu = gpu;
		res2__thread_params.ctx = &asset_ctx;
		res2__thread_params.engine_allocator = this->allocator;
		res2__thread_params.engine = this;
		thread::signal_create (&res2__thread_params.hEvent_started);		

		eThreadError err = thread::create (&res2__hThreadLoader, Engine::res2__LoaderThread_mainFN, &res2__thread_params);
		if (err != eThreadError::none)
		{
			logger::err ("Engine::setup() => error creating thread loader: errcode=%d\n", static_cast<int>(err));
			return false;
		}
		
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
			.action_add ("rotateY")
			.action_add ("zoom_in")
			.action_add ("zoom_out");
			


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
		inputCtx->action_bindToAxleREL ("zoom_in", input::eOrigin::mouse, input::eAxle::z, input::eAxleDirection::both);
		inputCtx->action_bindToAxleREL ("zoom_out", input::eOrigin::mouse, input::eAxle::z, input::eAxleDirection::both);

	}


	
	//resource manager
	vtxBufferMan.setup (allocator, gpu);
	idxBufferMan.setup (allocator, gpu);


	//attendo che il loader-thread abbia segnalato che e' partito
	if (!thread::signal_wait (res2__thread_params.hEvent_started, 20000))
	{
		logger::err ("Engine::setup() => error waiting for loader thread to start\n");
		return false;
	}
	thread::signal_destroy (res2__thread_params.hEvent_started);



	handle_texture_bianca.set_invalid();
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
	res2__flushLoaderThreadMsg();

	//input
	gos::input::pollEvents();
	input::resolveEvents (gpu->getWindow(), inputCtx, &evtList);
	return true;
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
input::eButtonStatus Engine::inputEvent_getBtnStatus() const
{
	return evtList.getBtnStatus();
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
void Engine::asset_hotreload (asset2::UID assetUID)
{
	res2::ResHandle asset_handle;
	if (!res2_known_assetUID_HashMap.find (assetUID, &asset_handle))
		return;

	asset_logger->log ("Hot reload of asset UID %016" PRIX64 "\n", assetUID);
	asset_logger->inc_indent();
	{
		//faccio il release della risorsa associata all'asset_handle
		res2::ResDescr *asset_descr = res2__get_descriptor(asset_handle);
		assert (NULL != asset_descr->list_figli->descr); //deve avere 1 figlio
		assert (NULL == asset_descr->list_figli->next); //deve avere 1 e 1 solo figlio in quanto e' un asset
		res2::ResDescr *child_res_descr = asset_descr->list_figli->descr;

		//rimuovo il figlio dalla mia lista di figli e poi ne faccio il free
		res2::remove_child (asset_descr, child_res_descr);
		res2__release (child_res_descr->res_handle);


		//recupero info sulla nuova signatureUID figlia di assetUID
		asset2::UID new_signatureUID;
		if (asset2::asset_get_info (asset_ctx, assetUID, NULL, &new_signatureUID))
		{
			//questo asset ha come figlio la risorsa <new_signatureUID>
			res2__callbak_on_hot_reload (asset_descr, new_signatureUID);
		}
		else
		{
			//a quanto pare l'asset non e' + associato ad alcuna risorsa, non dovrebbe succedere
			//...o forse puo' succedere se ho eliminato del tutto l'asset dai file gosassetdj
			DBGBREAK;
		}
	}
	asset_logger->dec_indent();

}

/**************************************************************** 
 * VTX BUFFER
 *****************************************************************/
bool Engine::vtxBuffer_create (u32 sizeInByte, eMemAccessMode mode, VtxBufferH *out_handle)
{
	res2::VtxBuffer::Resource *res = res2MemManager.reserveT<res2::VtxBuffer::Resource> (out_handle);
	if (NULL == res)
	{
		logger::err ("Engine::vtxBuffer_create() => can't create handle\n");
		return false;
	}
	
	if (!gpu->vertexBuffer_create (sizeInByte, mode, &res->data.vbHandle))
	{
		logger::err ("Engine::vtxBuffer_create() => can't create VB\n");
		return false;
	}	

	res2::change_state (&res->base, res2::eStatus::ready, "created");
	return true;
}

void Engine::res2__vtxBufferRes_callback_on_destroy (res2::ResDescr *descr)
{
	res2::VtxBuffer::Resource *res = reinterpret_cast<res2::VtxBuffer::Resource*>( descr );
	gpu->deleteResource(res->data.vbHandle);
}

/**************************************************************** 
 * IDX BUFFER
 *****************************************************************/
bool Engine::idxBuffer_create (u32 sizeInByte, eMemAccessMode mode, IdxBufferH *out_handle)
{
	res2::IdxBuffer::Resource *res = res2MemManager.reserveT<res2::IdxBuffer::Resource> (out_handle);
	if (NULL == res)
	{
		logger::err ("Engine::idxBuffer_create() => can't create handle\n");
		return false;
	}
	
	if (!gpu->indexBuffer_create (sizeInByte, mode, &res->data.ibHandle))
	{
		logger::err ("Engine::idxBuffer_create() => can't create VB\n");
		return false;
	}	

	res2::change_state (&res->base, res2::eStatus::ready, "created");
	return true;
}

void Engine::res2__idxBufferRes_callback_on_destroy (res2::ResDescr *descr)
{
	res2::IdxBuffer::Resource *res = reinterpret_cast<res2::IdxBuffer::Resource*>( descr );
	gpu->deleteResource(res->data.ibHandle);
}

/**************************************************************** 
 * TEXTURE 2D
 *****************************************************************/
bool Engine::texture2D_createFromAsset (const char *rtname, TextureH *out_handle, res2::eLoadMode loadMode)
{
	res2::Texture2d::Asset *asset = res2__get_or_create_asset_from_rtname<res2::Texture2d> ("Engine::texture2D_createFromAsset", rtname, loadMode);
	if (NULL == asset)
	{
		out_handle->set_invalid();
		return false;
	}
	out_handle->setFromU32 (asset->base.res_handle.viewAsU32());
	return true;
}

bool Engine::texture2D_create (u16 dimx, u16 dimy, u8 nMipMap, eImageFormat fmt, eMemAccessMode memAccessMode, const void *srcDATA, TextureH *out_handle, gpu::StageHelper &stageHelper)
{
	if (priv_texture2D_create_ex (dimx, dimy, nMipMap, fmt, memAccessMode, srcDATA, out_handle, stageHelper, u32MAX))
		return true;
	logger::err ("Engine::texture2D_create() => can't create handle\n");
	return false;
}

bool Engine::priv_texture2D_create_ex (u16 dimx, u16 dimy, u8 nMipMap, eImageFormat fmt, eMemAccessMode memAccessMode, const void *srcDATA, TextureH *out_handle, gpu::StageHelper &stageHelper, u32 desired_texture_index)
{
	res2::Texture2d::Resource *res = res2MemManager.reserveT<res2::Texture2d::Resource> (out_handle);
	if (NULL == res)
	{
		logger::err ("Engine::priv_texture2D_create_ex() => can't create handle\n");
		return false;
	}
	
	if (!gpu->texture_create2D (dimx, dimy, nMipMap, fmt, memAccessMode, srcDATA, &res->data.texHandle, stageHelper))
	{
		logger::err ("Engine::priv_texture2D_create_ex() => can't create texture\n");
		return false;
	}

	priv_texture2D__add_to_mega_array (&res->data, desired_texture_index);
	res2::change_state (&res->base, res2::eStatus::ready, "created");
	return true;
}

bool Engine::texture2D_create (const gos::Image *im, u8 srcTextureNum, eMemAccessMode memAccessMode, TextureH *out_handle, gpu::StageHelper &stageHelper)
{
	res2::Texture2d::Resource *res = res2MemManager.reserveT<res2::Texture2d::Resource> (out_handle);
	if (NULL == res)
	{
		logger::err ("Engine::priv_texture2D_create_ex() => can't create handle\n");
		return false;
	}
	
	if (!gpu->texture_create2D (im, srcTextureNum, memAccessMode, &res->data.texHandle, stageHelper))
	{
		logger::err ("Engine::priv_texture2D_create_ex() => can't create texture\n");
		return false;
	}

	priv_texture2D__add_to_mega_array (&res->data);
	res2::change_state (&res->base, res2::eStatus::ready, "created");
	return true;
}

eRes2LoadResult Engine::res2__textureRes_callback_on_load (const res2::CallbackData_onLoad *callback_data)
{
	//questa viene chiamata solo sulle risorse bindate ad una signature
	assert (callback_data->descr->UID.isASignatureOfType(eAssetType::tex2D));

	res2::Texture2d::Resource *res = reinterpret_cast<res2::Texture2d::Resource*>( callback_data->descr );
	gos::Image image;
	image.p = callback_data->user_data_pt;	

	if (gpu->texture_create2D (&image, 0, eMemAccessMode::onGPU, &res->data.texHandle, stageHelper))
	{
		priv_texture2D__add_to_mega_array(&res->data);
		return eRes2LoadResult::success;
	}

	logger::err ("Engine::res2__textureRes_callback_on_load() => can't create texture\n");
	return eRes2LoadResult::fail;
}

void Engine::res2__textureRes_callback_on_destroy (res2::ResDescr *descr)
{
	//questa viene chiamata solo sulle risorse, non sugli asset
	assert (descr->UID.isInvalid() ||  descr->UID.isASignatureOfType(eAssetType::tex2D));

	res2::Texture2d::Resource *res = reinterpret_cast<res2::Texture2d::Resource*>( descr );
	priv_texture2D__remove_from_mega_array (&res->data);
	
	if (res->data.texHandle.isValid())
	{
		gpu->deleteResource (res->data.texHandle);
		res->data.reset();
	}
}

void Engine::priv_texture2D__add_to_mega_array (TextureData *data, u32 desired_index)
{
	assert (u32MAX == data->index);
	if (u32MAX != desired_index)
	{
		renderPipe.internal__texture_add_reserved (data->texHandle, desired_index);
		data->index = desired_index;
	}
	else
		data->index = renderPipe.internal__texture_add_if_dont_exists (data->texHandle);

	asset_logger->log ("priv_texture2D__add_to_mega_array (tex-index=%d)\n", data->index);
}

void Engine::priv_texture2D__remove_from_mega_array (TextureData *data)
{
	//asset_logger->log ("priv_texture2D__remove_from_mega_array (tex-index=%d)\n", data->index);

	renderPipe.internal__texture_remove (data->texHandle);
	data->index = u32MAX;
}

/**************************************************************** 
 * MATERIAL PBR
 *****************************************************************/
bool Engine::materialPBR_createFromAsset (const char *rtname, MaterialPBRH *out_handle, res2::eLoadMode loadMode)
{
	res2::MaterialPBR::Asset *asset = res2__get_or_create_asset_from_rtname<res2::MaterialPBR> ("Engine::materialPBR_createFromAsset", rtname, loadMode);
	if (NULL == asset)
	{
		out_handle->set_invalid();
		return false;
	}
	out_handle->setFromU32 (asset->base.res_handle.viewAsU32());
	return true;

}

bool Engine::materialPBR_create (MaterialPBRH *out_handle)
{
	res2::MaterialPBR::Resource *res = res2MemManager.reserveT<res2::MaterialPBR::Resource> (out_handle);
	if (NULL == res)
	{
		logger::err ("Engine::materialPBR_create() => can't create handle\n");
		return false;
	}
	res2::change_state (&res->base, res2::eStatus::ready, "created");
	asset_logger->inc_indent();
	{
		//gli addo le texture di default
		res2::Texture2d::Resource *res_tex_bianca = res2MemManager.getT<res2::Texture2d::Resource>(handle_texture_bianca);
		assert (NULL != res_tex_bianca);
		
		res2::add_child (&res->base, &res_tex_bianca->base);
		res2::inc_refCount (&res_tex_bianca->base);
		res->data.diffuse_texture = &res_tex_bianca->data;

		res2::add_child (&res->base, &res_tex_bianca->base);
		res2::inc_refCount (&res_tex_bianca->base);
		res->data.normal_texture = &res_tex_bianca->data;
	}
	asset_logger->dec_indent();
	return true;
}

bool Engine::get_for_update (MaterialPBRH handle, MaterialPBRDataUpd *out, u32 timeout_msec)
{ 
	assert (NULL != out);
	res2::ResDescr *descr = res2MemManager.get (handle);
	if (descr->isAnAsset())
	{
		//gli asset hanno un solo figlio, che e' la risorsa vera e propria
		assert (NULL == descr->list_figli->next);		
		descr = descr->list_figli->descr;
		handle.setFromU32 (descr->res_handle.viewAsU32());
	}

	if (!res2__get_or_schedule_load (descr, timeout_msec))
		return false;

	res2::MaterialPBR::Resource *res = reinterpret_cast<res2::MaterialPBR::Resource*> (descr);
	res->data.update_ct++;
	out->data = &res->data;
	out->base = descr;

	//ritorno sempre un pt ad una risorsa
	assert ( !out->base->isAnAsset()  );
	return true;	
}

void Engine::priv__materialPBR_update_texture (MaterialPBRDataUpd *dataIN, TextureH tex_handle, res2::Texture2d::Data **target)
{
	assert (NULL != target);
	assert (NULL != dataIN);
	assert (NULL != dataIN->base);
	assert (!dataIN->base->isAnAsset());
	
	//recupero la nuova risorsa 
	res2::ResDescr *new_descr = res2MemManager.get (tex_handle);
	if (NULL == new_descr)
	{
		DBGBREAK;
		return;
	}
	assert (res2::eResType::texture_2d == new_descr->get_type());


	//sono una risorsa
	assert (!dataIN->base->isAnAsset());
	res2::ResDescr *descr = dataIN->base;
	res2::MaterialPBR::Resource *res = reinterpret_cast<res2::MaterialPBR::Resource*>( descr );
	
	//cerco il mio figlio che al momento "owna" il parametro <target> che voglio cambiare
	if (NULL != (*target))
	{
		res2::ResDescr *child_descr = res2__find_child_with_type_and_data<res2::Texture2d> (descr, (*target));
		if (NULL == child_descr)
		{
			//no ho trovato il figlio il cui <data> sia == a res->data.diffuse_texture
			DBGBREAK;
			return;
		}
		if (child_descr->res_handle.viewAsU32() == tex_handle.viewAsU32())
			return;

		//rimuovo il figlio dalla mia lista di figli e ne faccio il release
		res2::remove_child (&res->base, child_descr);
		res2__release(child_descr->res_handle);
	}

	//addo la nuova risorsa alla lista dei miei figli
	res2::add_child (&res->base, new_descr);
	res2::inc_refCount(new_descr);

	if (new_descr->isAnAsset())
	{
		res2::Texture2d::Asset *new_asset = reinterpret_cast<res2::Texture2d::Asset*>( new_descr );
		(*target) = new_asset->res_data_pt;
	}
	else
	{
		res2::Texture2d::Resource *new_res = reinterpret_cast<res2::Texture2d::Resource*>( new_descr );
		(*target) = &new_res->data;
	}
}

void Engine::materialPBR_update_texture_diffuse (MaterialPBRDataUpd *dataIN, TextureH tex_handle)
{
	assert (!dataIN->base->isAnAsset());
	res2::ResDescr *descr = dataIN->base;
	res2::MaterialPBR::Resource *res = reinterpret_cast<res2::MaterialPBR::Resource*>( descr );
	priv__materialPBR_update_texture (dataIN, tex_handle, &res->data.diffuse_texture);
}

void Engine::materialPBR_update_texture_normal (MaterialPBRDataUpd *dataIN, TextureH tex_handle)
{
	assert (!dataIN->base->isAnAsset());
	res2::ResDescr *descr = dataIN->base;
	res2::MaterialPBR::Resource *res = reinterpret_cast<res2::MaterialPBR::Resource*>( descr );
	priv__materialPBR_update_texture (dataIN, tex_handle, &res->data.normal_texture);
}

bool Engine::materialPBR_update_renderer_binding (MaterialPBRDataUpd *dataIN, u8 renderer_uid, u32 user_data)
{
	assert (NULL != dataIN);
	assert (NULL != dataIN->base);
	assert (!dataIN->base->UID.isAnAsset());
	
	assert (renderer_uid < res2::MaterialPBR::Data::NUM_MAX_RENDERER);
	dataIN->data->renderer_bindings[renderer_uid] = user_data;
	return true;
}

bool Engine::priv_materialPBR_bind_texture (res2::ResDescr *res_materialPRB, asset2::UID texture_uid, TextureData **out_data)
{
	res2::ResDescr *tex_descr = NULL;
	if (texture_uid.isInvalid())
	{
		res2::Texture2d::Resource *res_tex_bianca = res2MemManager.getT<res2::Texture2d::Resource>(handle_texture_bianca);
		assert (NULL != res_tex_bianca);
		
		tex_descr = &res_tex_bianca->base;
		(*out_data) = &res_tex_bianca->data;
		res2::inc_refCount (tex_descr);
	}
	else if (texture_uid.isASignature())
	{
		assert (texture_uid.isASignatureOfType(eAssetType::tex2D));
		res2::Texture2d::Resource *tex_res = res2__get_or_create_from_signatureUID<res2::Texture2d> (texture_uid);
		assert (NULL != tex_res);
		
		tex_descr = &tex_res->base;
		(*out_data) = &tex_res->data;
	}
	else if (texture_uid.isAnAsset())
	{
		assert (texture_uid.isAnAssetOfType(eAssetType::tex2D));
		res2::Texture2d::Asset *tex_asset = res2__get_or_create_asset_from_assetUID<res2::Texture2d>  ("Engine::priv_materialPBR_bind_texture", texture_uid, res2::eLoadMode::asap);
		assert (NULL != tex_asset);
		tex_descr = &tex_asset->base;
		(*out_data) = tex_asset->res_data_pt;
	}
	else
	{
		DBGBREAK;
		return false;
	}

	assert (NULL != tex_descr);
	res2::add_child (res_materialPRB, tex_descr);
	return true;
}

eRes2LoadResult Engine::res2__materialPBR_callback_on_load (const res2::CallbackData_onLoad *callback_data)
{
	asset_logger->log ("res2__materialPBR_callback_on_load\n");
	asset_logger->inc_indent();

	//questa viene chiamata solo sulle risorse bindate ad una signature
	assert (callback_data->descr->UID.isASignatureOfType(eAssetType::materialPBR));

	res2::MaterialPBR::Resource *res = reinterpret_cast<res2::MaterialPBR::Resource*>( callback_data->descr );
	const u32 fsize = callback_data->user_1;

	asset2::MaterialPBR mat;
	if (0 == asset2::MaterialPBR::deserialize (callback_data->user_data_pt, fsize, &mat))
	{
		logger::err ("Engine::res2__materialPBR_callback_on_load() => error parsing asset file\n");
		return eRes2LoadResult::fail;
	}

	asset_logger->inc_indent();
	{
		//diffuse col
		res->data.diffuse_col_HDR_RGBA.set (mat.diffuse_col_RGBA_HDR[0], mat.diffuse_col_RGBA_HDR[1], mat.diffuse_col_RGBA_HDR[2], mat.diffuse_col_RGBA_HDR[3]);

		// MaterialPBRDataUpd upd;	
		// get_for_update(res->base.res_handle);
		// materialPBR_update_texture_diffuse (&upd, 
		
		//diffuse texture
		priv_materialPBR_bind_texture (&res->base, mat.diffuse_texture_uid, &res->data.diffuse_texture);

		//normal texture
		priv_materialPBR_bind_texture (&res->base, mat.normal_texture_uid, &res->data.normal_texture);
	}
	asset_logger->dec_indent();

	
	asset_logger->dec_indent();
	return eRes2LoadResult::success;
}

void Engine::res2__materialPBR_callback_on_destroy (res2::ResDescr *descr)
{
	//questa viene chiamata solo sulle risorse, non sugli asset
	assert (descr->UID.isInvalid() ||  descr->UID.isASignatureOfType(eAssetType::materialPBR));

	//non devo fare nulla di che dato che sui miei figli viene fatto automaticamente il release
	//TODO: forse devo informare i "renderer" del fatto che sono morto?	
}

/**************************************************************** 
 * VTX SHADER
 *****************************************************************/
bool Engine::vtxshader_createFromAsset (const char *rtname, VtxShaderH *out_handle, res2::eLoadMode loadMode)
{
	res2::VtxShader::Asset *asset = res2__get_or_create_asset_from_rtname<res2::VtxShader> ("Engine::vtxshader_createFromAsset", rtname, loadMode);
	if (NULL == asset)
	{
		out_handle->set_invalid();
		return false;
	}
	out_handle->setFromU32 (asset->base.res_handle.viewAsU32());
	return true;
}

bool Engine::vtxshader_createFromFile (const char *filename, const char *mainFnName, VtxShaderH *out_handle)
{
	res2::VtxShader::Resource *res = res2MemManager.reserveT<res2::VtxShader::Resource> (out_handle);
	if (NULL == res)
	{
		logger::err ("Engine::vtxshader_createFromFile() => can't create handle\n");
		return false;
	}
	
	if (!gpu->vtxshader_createFromFile (filename, mainFnName, &res->data.vtxshader_handle))
	{
		logger::err ("Engine::vtxshader_createFromFile() => can't create shader\n");
		return false;
	}

	res2::change_state (&res->base, res2::eStatus::ready, "created");
	return true;
}

bool Engine::vtxshader_createFromMemory (const void *bufferIN, u32 bufferSize, const char *mainFnName, VtxShaderH *out_handle)
{
	res2::VtxShader::Resource *res = res2MemManager.reserveT<res2::VtxShader::Resource> (out_handle);
	if (NULL == res)
	{
		logger::err ("Engine::vtxshader_createFromMemory() => can't create handle\n");
		return false;
	}
	
	if (!gpu->vtxshader_createFromMemory (bufferIN, bufferSize, mainFnName, &res->data.vtxshader_handle))
	{
		logger::err ("Engine::vtxshader_createFromMemory() => can't create shader\n");
		return false;
	}

	res2::change_state (&res->base, res2::eStatus::ready, "created");
	return true;
}

eRes2LoadResult Engine::res2__vtxShaderRes_callback_on_load (const res2::CallbackData_onLoad *callback_data)
{
	//questa viene chiamata solo sulle risorse bindate ad una signature
	assert (callback_data->descr->UID.isASignatureOfType(eAssetType::vtx_shader));

	res2::VtxShader::Resource *res = reinterpret_cast<res2::VtxShader::Resource*>( callback_data->descr );
	res->data.vtxshader_handle.setFromU32 (callback_data->user_1);
	return eRes2LoadResult::success;
}

void Engine::res2__vtxShaderRes_callback_on_destroy (res2::ResDescr *descr)
{
	//questa viene chiamata solo sulle risorse, non sugli asset
	assert (descr->UID.isInvalid() ||  descr->UID.isASignatureOfType(eAssetType::vtx_shader));

	res2::VtxShader::Resource *res = reinterpret_cast<res2::VtxShader::Resource*>( descr );
	if (res->data.vtxshader_handle.isValid())
	{
		gpu->deleteResource (res->data.vtxshader_handle);
		res->data.reset();
	}
}

/**************************************************************** 
 * PXL SHADER
 *****************************************************************/
bool Engine::pxlshader_createFromAsset (const char *rtname, PxlShaderH *out_handle, res2::eLoadMode loadMode)
{
	res2::PxlShader::Asset *asset = res2__get_or_create_asset_from_rtname<res2::PxlShader> ("Engine::pxlshader_createFromAsset", rtname, loadMode);
	if (NULL == asset)
	{
		out_handle->set_invalid();
		return false;
	}
	out_handle->setFromU32 (asset->base.res_handle.viewAsU32());
	return true;
}

bool Engine::pxlshader_createFromFile (const char *filename, const char *mainFnName, PxlShaderH *out_handle)
{
	res2::PxlShader::Resource *res = res2MemManager.reserveT<res2::PxlShader::Resource> (out_handle);
	if (NULL == res)
	{
		logger::err ("Engine::pxlshader_createFromFile() => can't create handle\n");
		return false;
	}
	
	if (!gpu->pxlshader_createFromFile (filename, mainFnName, &res->data.pxlshader_handle))
	{
		logger::err ("Engine::pxlshader_createFromFile() => can't create shader\n");
		return false;
	}

	res2::change_state (&res->base, res2::eStatus::ready, "created");
	return true;
}

bool Engine::pxlshader_createFromMemory (const void *bufferIN, u32 bufferSize, const char *mainFnName, PxlShaderH *out_handle)
{
	res2::PxlShader::Resource *res = res2MemManager.reserveT<res2::PxlShader::Resource> (out_handle);
	if (NULL == res)
	{
		logger::err ("Engine::pxlshader_createFromMemory() => can't create handle\n");
		return false;
	}
	
	if (!gpu->pxlshader_createFromMemory (bufferIN, bufferSize, mainFnName, &res->data.pxlshader_handle))
	{
		logger::err ("Engine::pxlshader_createFromMemory() => can't create shader\n");
		return false;
	}

	res2::change_state (&res->base, res2::eStatus::ready, "created");
	return true;
}

eRes2LoadResult Engine::res2__pxlShaderRes_callback_on_load (const res2::CallbackData_onLoad *callback_data)
{
	//questa viene chiamata solo sulle risorse bindate ad una signature
	assert (callback_data->descr->UID.isASignatureOfType(eAssetType::pxl_shader));

	res2::PxlShader::Resource *res = reinterpret_cast<res2::PxlShader::Resource*>( callback_data->descr );
	res->data.pxlshader_handle.setFromU32 (callback_data->user_1);
	return eRes2LoadResult::success;
}

void Engine::res2__pxlShaderRes_callback_on_destroy (res2::ResDescr *descr)
{
	//questa viene chiamata solo sulle risorse, non sugli asset
	assert (descr->UID.isInvalid() ||  descr->UID.isASignatureOfType(eAssetType::pxl_shader));

	res2::PxlShader::Resource *res = reinterpret_cast<res2::PxlShader::Resource*>( descr );
	if (res->data.pxlshader_handle.isValid())
	{
		gpu->deleteResource (res->data.pxlshader_handle);
		res->data.reset();
	}
}

/**************************************************************** 
 * PIPELINE
 *****************************************************************/
bool Engine::pipeline_createFromAsset (const char *rtname, PipelineH *out_handle, res2::eLoadMode loadMode)
{
	res2::Pipeline::Asset *asset = res2__get_or_create_asset_from_rtname<res2::Pipeline> ("Engine::pipeline_createFromAsset", rtname, loadMode);
	if (NULL == asset)
	{
		out_handle->set_invalid();
		return false;
	}
	out_handle->setFromU32 (asset->base.res_handle.viewAsU32());
	return true;
}

bool Engine::pipeline_create (const gpu::Pipeline_def &rpd, PipelineH *out_handle)
{
	res2::Pipeline::Resource *res = res2MemManager.reserveT<res2::Pipeline::Resource> (out_handle);
	if (NULL == res)
	{
		logger::err ("Engine::pipeline_create() => can't create handle\n");
		return false;
	}
	
	if (!gpu->pipeline_createNew (rpd, &res->data.pipe_handle))
	{
		logger::err ("Engine::pipeline_create() => can't create shader\n");
		return false;
	}

	res2::change_state (&res->base, res2::eStatus::ready, "created");
	return true;
}

eRes2LoadResult Engine::res2__pipelineRes_callback_on_load (const res2::CallbackData_onLoad *callback_data)
{
	//questa viene chiamata solo sulle risorse bindate ad una signature
	assert (callback_data->descr->UID.isASignatureOfType(eAssetType::pipe));

	res2::Pipeline::Resource *res_pipeline = reinterpret_cast<res2::Pipeline::Resource*>( callback_data->descr );

	//il caricamento avviene in + step, palleggiando tra engine e il loader-thread
	const u32 current_step = callback_data->user_2;
	switch (current_step)
	{
	default:
		DBGBREAK;
		return eRes2LoadResult::fail;

	case 0:
		{
			const u32 fsize = callback_data->user_1;
			
			gos::BufferR reader;
			reader.setup (callback_data->user_data_pt, fsize);
            
			//skip del <magic> che e' gia' stato verificato dal loader
			reader.readU32();

			//devo recuperare (o creare) l'asset del vtx shader
			{
				asset2::UID assetUID;
				assetUID._uid = reader.readU64 ();
				res2::VtxShader::Asset *asset = res2__get_or_create_asset_from_assetUID<res2::VtxShader> ("Engine::res2__pipelineRes_callback_on_load", assetUID, res2::eLoadMode::asap);

				res2::add_child (&res_pipeline->base, &asset->base);
			}

			//devo recuperare (o creare) l'asset del pxl shader
			{
				asset2::UID assetUID;
				assetUID._uid = reader.readU64 ();
				res2::PxlShader::Asset *asset = res2__get_or_create_asset_from_assetUID<res2::PxlShader> ("Engine::res2__pipelineRes_callback_on_load", assetUID, res2::eLoadMode::asap);

				res2::add_child (&res_pipeline->base, &asset->base);
			}

			//dico al loader che c'e' ancora del lavoro da fare
			return eRes2LoadResult::pending;
		}
		break;

	case 1:
		//a questo punto gli shader sono stati caricati.
		//Chiamo di nuovo il loader che si occupa di creare la pipe
		return eRes2LoadResult::pending;

	case 2:
		res_pipeline->data.pipe_handle.setFromU32 (callback_data->user_1);
		return eRes2LoadResult::success;
	}

	//qui non ci dovremmo mai arrivare
	DBGBREAK;
	return eRes2LoadResult::fail;
}

void Engine::res2__pipelineRes_callback_on_destroy (res2::ResDescr *descr)
{
	//questa viene chiamata solo sulle risorse, non sugli asset
	assert (descr->UID.isInvalid() ||  descr->UID.isASignatureOfType(eAssetType::pipe));

	res2::Pipeline::Resource *res = reinterpret_cast<res2::Pipeline::Resource*>( descr );
	if (res->data.pipe_handle.isValid())
	{
		gpu->deleteResource (res->data.pipe_handle);
		res->data.reset();
	}
}

/**************************************************************** 
 * GPU SHAPE
 *****************************************************************/
bool Engine::GPUShape_createFromAsset (const char *rtname, GPUShapeH *out_handle, res2::eLoadMode loadMode)
{
	res2::GPUShape::Asset *asset = res2__get_or_create_asset_from_rtname<res2::GPUShape> ("Engine::GPUShape_createFromAsset", rtname, loadMode);
	if (NULL == asset)
	{
		out_handle->set_invalid();
		return false;
	}
	out_handle->setFromU32 (asset->base.res_handle.viewAsU32());
	return true;
}

bool Engine::GPUShape_create (const gos::Shape *shape, gpu::StageHelper &stageHelper, GPUShapeH *out_handle)
{
	res2::GPUShape::Resource *res = res2MemManager.reserveT<res2::GPUShape::Resource> (out_handle);
	if (NULL == res)
	{
		logger::err ("Engine::GPUShape_create() => can't create handle\n");
		return false;
	}

	priv_gpushape_build (shape, stageHelper, &res->data);

	res2::change_state (&res->base, res2::eStatus::ready, "created");
	return true;
}

void Engine::priv_gpushape_build (const gos::Shape *shape, gpu::StageHelper &stageHelper, GPUShapeData *data)
{
	u32 byteNeeded;

	//vtxbuffer
	if (shape->numVtx)
	{
		const u32 sizeOfAVertex = gos::shape::calcSizeOfAVertex(shape->vtxLayout);
		byteNeeded = sizeOfAVertex * shape->numVtx;
		vtxBufferMan.reserve (byteNeeded, &data->alloc_vtxbuf_offset, &data->alloc_vtxbuf_size, &data->vbHandle);
		data->vtxStart = data->alloc_vtxbuf_offset / sizeOfAVertex;
		data->numVertex = shape->numVtx;
	}

	//idxBuffer    
	if (shape->numIdx)
	{
		const u32 sizeOfAnIndex = sizeof(u16);
		byteNeeded = sizeOfAnIndex * shape->numIdx;
		idxBufferMan.reserve (byteNeeded, &data->alloc_idxbuf_offset, &data->alloc_idxbuf_size, &data->ibHandle);
		data->indexStart = data->alloc_idxbuf_offset / sizeOfAnIndex;
		data->numIndices = shape->numIdx;
	}

	stageHelper.begin()
		.mem_to_buffer (shape->vtxBuffer, shape->numVtx * shape::calcSizeOfAVertex(shape->vtxLayout), data->vbHandle, data->alloc_vtxbuf_offset)
		.mem_to_buffer (shape->idxBuffer, shape->numIdx * sizeof(u16), data->ibHandle, data->alloc_idxbuf_offset)
		.submit();
}

eRes2LoadResult Engine::res2__gpushapeRes_callback_on_load (const res2::CallbackData_onLoad *callback_data)
{
	//questa viene chiamata solo sulle risorse bindate ad una signature
	assert (callback_data->descr->UID.isASignatureOfType(eAssetType::shape));

	res2::GPUShape::Resource *res = reinterpret_cast<res2::GPUShape::Resource*>( callback_data->descr );
	const u32 fsize = callback_data->user_1;
	u8 *file_buffer = (u8*)callback_data->user_data_pt;

	gos::Shape shape;
	if (!shape::deserialize (file_buffer, fsize, gos::getScrapAllocator(), &shape))
	{
		logger::err ("res2__gpushapeRes_callback_on_load() => error deserializing shape\n");
		return eRes2LoadResult::fail;
	}

	priv_gpushape_build (&shape, stageHelper, &res->data);
	shape::shapeFree (gos::getScrapAllocator(), &shape);
	return eRes2LoadResult::success;
}

void Engine::res2__gpushapeRes_callback_on_destroy (res2::ResDescr *descr)
{
	//questa viene chiamata solo sulle risorse, non sugli asset
	assert (descr->UID.isInvalid() ||  descr->UID.isASignatureOfType(eAssetType::shape));

	res2::GPUShape::Resource *res = reinterpret_cast<res2::GPUShape::Resource*>( descr );
	GPUShapeData *data = &res->data;
	{
		if (data->numVertex)
			vtxBufferMan.release (data->vbHandle, data->alloc_vtxbuf_offset, data->alloc_vtxbuf_size);

		if (data->numIndices)
			idxBufferMan.release (data->ibHandle, data->alloc_idxbuf_offset, data->alloc_idxbuf_size);
	}
	res->data.reset();
}


/**************************************************************** 
 * SKELETON
 *****************************************************************/
bool Engine::skeleton_createFromAsset (const char *rtname, SkeletonH *out_handle, res2::eLoadMode loadMode)
{
	res2::Skeleton::Asset *asset = res2__get_or_create_asset_from_rtname<res2::Skeleton> ("Engine::skeleton_createFromAsset", rtname, loadMode);
	if (NULL == asset)
	{
		out_handle->set_invalid();
		return false;
	}
	out_handle->setFromU32 (asset->base.res_handle.viewAsU32());
	return true;
}

bool Engine::skeleton_create (const Skeleton &sk, SkeletonH *out_handle)
{
	if (skeleton_createFromMemory (sk.blob, skeleton::get_blob_size(sk), out_handle))
		return true;

	logger::err ("Engine::skeleton_create() => error creating skeleton\n");
	return false;
}

bool Engine::skeleton_createFromMemory (const u8 *buffer, u32 sizeof_buffer, SkeletonH *out_handle)
{
	res2::Skeleton::Resource *res = res2MemManager.reserveT<res2::Skeleton::Resource> (out_handle);
	if (NULL == res)
	{
		logger::err ("Engine::skeleton_createFromMemory() => can't create handle\n");
		return false;
	}

	const u32 n = skeleton::deserialize (buffer, sizeof_buffer, allocator, &res->data.skeleton);
	if (0 == n)
	{
		logger::err ("Engine::skeleton_createFromMemory() => error during deserialization\n");
		return false;
	}

	res2::change_state (&res->base, res2::eStatus::ready, "created");
	return true;
}

eRes2LoadResult Engine::res2__skeletonRes_callback_on_load (const res2::CallbackData_onLoad *callback_data)
{
	//questa viene chiamata solo sulle risorse bindate ad una signature
	assert (callback_data->descr->UID.isASignatureOfType(eAssetType::skeleton));

	res2::Skeleton::Resource *res = reinterpret_cast<res2::Skeleton::Resource*>( callback_data->descr );
	const u32 fsize = callback_data->user_1;
	u8 *file_buffer = (u8*)callback_data->user_data_pt;

	if (!skeleton::deserialize (file_buffer, fsize, this->allocator, &res->data.skeleton))
	{
		logger::err ("res2__skeletonRes_callback_on_load() => error deserializing shape\n");
		return eRes2LoadResult::fail;
	}

	return eRes2LoadResult::success;
}

void Engine::res2__skeletonRes_callback_on_destroy (res2::ResDescr *descr)
{
	//questa viene chiamata solo sulle risorse, non sugli asset
	assert (descr->UID.isInvalid() ||  descr->UID.isASignatureOfType(eAssetType::skeleton));

	res2::Skeleton::Resource *res = reinterpret_cast<res2::Skeleton::Resource*>( descr );
	skeleton::free (res->data.skeleton);
	res->data.reset();
}


/**************************************************************** 
 * MODEL 3d
 *****************************************************************/
bool Engine::model_createFromAsset (const char *rtname, ModelH *out_handle, res2::eLoadMode loadMode)
{
	res2::Model3d::Asset *asset = res2__get_or_create_asset_from_rtname<res2::Model3d> ("Engine::model_createFromAsset", rtname, loadMode);
	if (NULL == asset)
	{
		out_handle->set_invalid();
		return false;
	}
	out_handle->setFromU32 (asset->base.res_handle.viewAsU32());
	return true;
}

bool Engine::model_create (SkeletonH handle_skeleton, u16 num_shape, u16 num_material, u16 num_meshes, ModelH *out_handle)
{
	res2::Model3d::Resource *res = res2MemManager.reserveT<res2::Model3d::Resource> (out_handle);
	if (NULL == res)
	{
		logger::err ("Engine::model_create() => can't create handle\n");
		return false;
	}

	if (!model::alloc (allocator, num_shape, num_material, num_meshes, &res->data.model))
	{
		logger::err ("Engine::model_create() => can't allocate model\n");
		return NULL;
	}

	model::set_skeleton(res->data.model, handle_skeleton);

	res2::change_state (&res->base, res2::eStatus::ready, "created");
	return true;
}

eRes2LoadResult	Engine::res2__modelRes_callback_on_load (const res2::CallbackData_onLoad *callback_data)
{
	//questa viene chiamata solo sulle risorse bindate ad una signature
	assert (callback_data->descr->UID.isASignatureOfType(eAssetType::model3d));

	res2::Model3d::Resource *res_model = reinterpret_cast<res2::Model3d::Resource*>( callback_data->descr );

	const u32 fsize = callback_data->user_1;
	u8 *file_buffer = (u8*)callback_data->user_data_pt;

	gos::BufferR reader;
	reader.setup (file_buffer, fsize);
	
	//skip del <magic> che e' gia' stato verificato dal loader
	reader.readU32();
	
	
	const u32 num_shape = reader.readU32 ();
	const u32 num_material = reader.readU32 ();
	const u32 num_meshes = reader.readU32 ();
	
	//skeleton asset
	{
		asset2::UID assetUID;
		assetUID._uid = reader.readU64 ();
		res2::Skeleton::Asset *asset = res2__get_or_create_asset_from_assetUID<res2::Skeleton> ("Engine::res2__modelRes_callback_on_load", assetUID, res2::eLoadMode::asap);
		res2::add_child (&res_model->base, &asset->base);
	}

	if (!model::alloc (this->allocator, (u16)num_shape, (u16)num_material, (u16)num_meshes, &res_model->data.model))
	{
		logger::err ("Engine::res2__modelRes_callback_on_load => can't alloc model\n");
		return eRes2LoadResult::fail;
	}

	//shape
	for (u32 i=0; i<num_shape; i++)
	{
		asset2::UID assetUID;
		assetUID._uid = reader.readU64();
		res2::GPUShape::Asset *asset = res2__get_or_create_asset_from_assetUID<res2::GPUShape> ("Engine::res2__modelRes_callback_on_load", assetUID, res2::eLoadMode::asap);
		res2::add_child (&res_model->base, &asset->base);

		GPUShapeH asset_handle;
		asset_handle.setFromU32 ( asset->base.res_handle.viewAsU32() );
		model::set_gpushape (res_model->data.model, i, asset_handle);
	}
	
	//material
	for (u32 i=0; i<num_material; i++)
	{
		asset2::UID assetUID;
		assetUID._uid = reader.readU64();
		res2::MaterialPBR::Asset *asset = res2__get_or_create_asset_from_assetUID<res2::MaterialPBR> ("Engine::res2__modelRes_callback_on_load", assetUID, res2::eLoadMode::asap);
		res2::add_child (&res_model->base, &asset->base);

		MaterialPBRH asset_handle;
		asset_handle.setFromU32 ( asset->base.res_handle.viewAsU32() );
		model::set_material (res_model->data.model, i, asset_handle);
	}

	//meshes
	for (u32 i=0; i<num_meshes; i++)
	{
		const u32 shape_index = reader.readU32();
		const u32 bone_index = reader.readU32();
		const u32 material_index = reader.readU32();
		const char *mesh_name = NULL;
		
		model::set_mesh  (res_model->data.model, i, (u16)shape_index, (u16)bone_index, (u16)material_index, mesh_name);
	}
		

	return eRes2LoadResult::success;
}

void Engine::res2__modelRes_callback_on_destroy (res2::ResDescr *descr)
{
	//questa viene chiamata solo sulle risorse, non sugli asset
	assert (descr->UID.isInvalid() ||  descr->UID.isASignatureOfType(eAssetType::model3d));

	res2::Model3d::Resource *res = reinterpret_cast<res2::Model3d::Resource*>( descr );
	model::free (res->data.model);
	res->data.reset();
}



/**************************************************************** 
 * MODEL INSTANCE
 *****************************************************************
bool Engine::modelinst_create (ENGModel3d handle_modelSRC, ENGModel3dInst *out_handle)
{
	//recupero le info sulla modelSRC,
	//res::Descr *res_modelSRC = res__getDescriptor(handle_modelSRC.res_handle);
	res::Descr *res_modelSRC = proxyRes__getDescriptorT(handle_modelSRC);
	if (NULL == res_modelSRC)
	{
		logger::err ("Engine::modelinst_create() => invalid handle_modelSRC\n");
		return false;
	}

	//creo istanza
	asset2::UID signatureUID;
	signatureUID.setInvalid();
	res::Model3dInst *res = (res::Model3dInst*)res__createHandle (res::eType::model_instance, res::eStatus::notLoaded, signatureUID, &out_handle->res_handle);
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
	return false;
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


void Engine::modelinst_getWMatrix (ENGModel3dInst handle, mat4x4f *out_matW)
{
	assert (NULL != out_matW);
	res::Model3dInst *res = (res::Model3dInst*)res__getDescriptor(handle.res_handle);
	if (NULL != res)
		*out_matW = res->matW;
	else
	{
		DBGBREAK;
	}
}

void Engine::modelinst_applyTransform (ENGModel3dInst handle, const mat4x4f &matW)
{
	res::Model3dInst *res = (res::Model3dInst*)res__getDescriptor(handle.res_handle);
	if (NULL == res)
		return;
	res->matW = matW;

	//se sono gia' in stato ready, applico la matW al mio modello, altrimenti questa cosa
	//verra' fatta non appena divento ready
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
*/




/**************************************************************** 
 * SHAPE
 ****************************************************************
bool Engine::shape_create (const VtxLayout &vtxLayout, u32 numVtx, u32 numIdx, ENGShape *out_handle)
{
	asset2::UID invalid_UID;
	invalid_UID.setInvalid();

	ProxyRes *proxyRes = proxyResInfo.reserve (invalid_UID, &out_handle->index);

	res::Shape *res = (res::Shape*)res__createHandle(res::eType::shape, res::eStatus::ready, invalid_UID, &proxyRes->res_handle);
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
*/
