#ifndef _gosEngine_h_
#define _gosEngine_h_
#include "gosEngineEnumAndDefine.h"
#include "gosEngine_vtxBufferMan.h"
#include "gosEngine_idxBufferMan.h"
#include "gosEngine_scene.h"
#include "renderPipe/gosEngineRenderPipe.h"
#include "../gos/logger/gosLoggerStdout.h"
#include "../gos/gosObjectPool.h"
#include "../gosAsset2/gosAsset2.h"
#include "../gos/gosThreadMsgQ.h"
#include "../gos/memory/gosAllocatorHeap.h"
#include "res2/loaders/Loader.h"

namespace gos
{
	/****************
	 * @brief   Engine
	 * 
	 *          <assetHub>   viene creato durante setup() e punta alla directory "data"v
	 */
	class Engine
	{
	public:
		struct InputEvent
		{
			u32 actionID;
			i16 value;
		};

	public:
		gos::GPU                *gpu;
		gos::input::Context     *inputCtx;
		engine::RenderPipe		renderPipe;

	public:
							Engine();
							~Engine()                                   { unsetup(); }

		bool                setup (u32 mainWin_w, u32 mainWin_h, const char *mainWin_title);
		void                unsetup();

		bool                asset_rebuildAll();
		bool                asset_build();
		void 				asset_hotreload (asset2::UID assetUID);

		bool                setup_renderPipe();

			/* update:  ritorna false se la mainwin e' stata chiusa */
		bool                            update();
		u64 							get_frame_num() const 						{ return frame_num; }
		bool                            inputEvent_getNext (InputEvent *out);
		const input::MouseStatus*       inputEvent_getMouseStatus() const;
		const input::sButtonModifier*   inputEvent_getBtnModifier() const;
		input::eButtonStatus			inputEvent_getBtnStatus() const;

		input::eMouseMode   getMouseMode() const;
		void			    setMouseMode (input::eMouseMode mode);
		
		//=============================
		void            toggleFullscreen()                              			{ gpu->toggleFullscreen(); }
		void            toggleVSync();


		//============================= vtxBuffer
		bool            vtxBuffer_create (u32 sizeInByte, eMemAccessMode mode, VtxBufferH *out_handle);
		void            release (VtxBufferH &handle)															{ res2__release(handle); }
		bool            get (VtxBufferH handle, const VtxBufferData **out)										{ return res2__get_or_schedule_loadT<res2::VtxBuffer>(handle, out, 0); }
		void            res2__vtxBufferRes_callback_on_destroy (res2::ResDescr *descr);

		//============================= idxBuffer
		bool            idxBuffer_create (u32 sizeInByte, eMemAccessMode mode, IdxBufferH *out_handle);
		void            release (IdxBufferH handle)																{ res2__release(handle); }
		bool            get (IdxBufferH handle, const IdxBufferData **out)										{ return res2__get_or_schedule_loadT<res2::IdxBuffer>(handle, out, 0); }
		void            res2__idxBufferRes_callback_on_destroy (res2::ResDescr *descr);

		//============================= texture2D
		bool            texture2D_createFromAsset (const char *runtimeName, TextureH *out_handle, res2::eLoadMode loadMode = res2::eLoadMode::onDemand);
		bool            texture2D_create (u16 dimx, u16 dimy, u8 nMipMap, eImageFormat fmt, eMemAccessMode memAccessMode, const void *srcDATA, TextureH *out_handle, gpu::StageHelper &stageHelper);
		bool            texture2D_create (const gos::Image *im, u8 srcTextureNum, eMemAccessMode memAccessMode, TextureH *out_handle, gpu::StageHelper &stageHelper);
		void            release (TextureH &handle)																			{ res2__release(handle); }
		bool            get (TextureH handle, const TextureData **out, u32 timeout_msec = 0)								{ return res2__get_or_schedule_loadT<res2::Texture2d>(handle, out, timeout_msec); }
		eRes2LoadResult	res2__textureRes_callback_on_load (const res2::CallbackData_onLoad *callback_data);
		void            res2__textureRes_callback_on_destroy (res2::ResDescr *descr);
		void            res2__textureRes_callback_on_hot_reload (res2::ResDescr *asset_descr, asset2::UID new_signatureUID)	{ res2__callback_on_hot_reloadT<res2::Texture2d>(asset_descr, new_signatureUID); }

		bool            get_texture_bianca (const TextureData **out)														{ return get(handle_texture_bianca, out, 0); }
		TextureH		get_handle_texture_bianca()																			{ return handle_texture_bianca; }

		//============================= vtxshader
		bool            vtxshader_createFromAsset (const char *runtimeName, VtxShaderH *out_handle, res2::eLoadMode loadMode = res2::eLoadMode::onDemand);
		bool            vtxshader_createFromFile (const char *filename, const char *mainFnName, VtxShaderH *out_handle);
		bool            vtxshader_createFromMemory (const void *bufferIN, u32 bufferSize, const char *mainFnName, VtxShaderH *out_handle);
		void            release (VtxShaderH &handle)																			{ res2__release(handle); }
		bool            get (VtxShaderH handle, const VtxShaderData **out, u32 timeout_msec = 0)								{ return res2__get_or_schedule_loadT<res2::VtxShader>(handle, out, timeout_msec); }
		eRes2LoadResult	res2__vtxShaderRes_callback_on_load (const res2::CallbackData_onLoad *callback_data);
		void            res2__vtxShaderRes_callback_on_destroy (res2::ResDescr *descr);
		void            res2__vtxShaderRes_callback_on_hot_reload (res2::ResDescr *asset_descr, asset2::UID new_signatureUID)	{ res2__callback_on_hot_reloadT<res2::VtxShader>(asset_descr, new_signatureUID); }

		//============================= pxlshader
		bool            pxlshader_createFromAsset (const char *runtimeName, PxlShaderH *out_handle, res2::eLoadMode loadMode = res2::eLoadMode::onDemand);
		bool            pxlshader_createFromFile (const char *filename, const char *mainFnName, PxlShaderH *out_handle);
		bool            pxlshader_createFromMemory (const void *bufferIN, u32 bufferSize, const char *mainFnName, PxlShaderH *out_handle);
		void            release (PxlShaderH &handle)																			{ res2__release(handle); }
		bool            get (PxlShaderH handle, const PxlShaderData **out, u32 timeout_msec = 0)								{ return res2__get_or_schedule_loadT<res2::PxlShader>(handle, out, timeout_msec); }
		eRes2LoadResult	res2__pxlShaderRes_callback_on_load (const res2::CallbackData_onLoad *callback_data);
		void            res2__pxlShaderRes_callback_on_destroy (res2::ResDescr *descr);
		void            res2__pxlShaderRes_callback_on_hot_reload (res2::ResDescr *asset_descr, asset2::UID new_signatureUID)	{ res2__callback_on_hot_reloadT<res2::PxlShader>(asset_descr, new_signatureUID); }

		//============================= pipeline
		bool            pipeline_createFromAsset (const char *runtimeName, PipelineH *out_handle, res2::eLoadMode loadMode = res2::eLoadMode::onDemand);
		bool            pipeline_create (const gpu::Pipeline_def &rpd, PipelineH *out_handle);
		void            release (PipelineH &handle)																				{ res2__release(handle); }
		bool            get (PipelineH handle, const PipelineData **out, u32 timeout_msec = 0)									{ return res2__get_or_schedule_loadT<res2::Pipeline>(handle, out, timeout_msec); }
		eRes2LoadResult	res2__pipelineRes_callback_on_load (const res2::CallbackData_onLoad *callback_data);
		void            res2__pipelineRes_callback_on_destroy (res2::ResDescr *descr);
		void            res2__pipelineRes_callback_on_hot_reload (res2::ResDescr *asset_descr, asset2::UID new_signatureUID)	{ res2__callback_on_hot_reloadT<res2::Pipeline>(asset_descr, new_signatureUID); }

		//============================= GPUShape
		bool            GPUShape_createFromAsset (const char *runtimeName, GPUShapeH *out_handle, res2::eLoadMode loadMode = res2::eLoadMode::onDemand);
		bool            GPUShape_create (const gos::Shape *shape, gpu::StageHelper &stageHelper, GPUShapeH *out_handle);
		void            release (GPUShapeH &handle)																				{ res2__release(handle); }
		bool            get (GPUShapeH handle, const GPUShapeData **out, u32 timeout_msec = 0)									{ return res2__get_or_schedule_loadT<res2::GPUShape>(handle, out, timeout_msec); }
		eRes2LoadResult	res2__gpushapeRes_callback_on_load (const res2::CallbackData_onLoad *callback_data);
		void            res2__gpushapeRes_callback_on_destroy (res2::ResDescr *descr);
		void            res2__gpushapeRes_callback_on_hot_reload (res2::ResDescr *asset_descr, asset2::UID new_signatureUID)	{ res2__callback_on_hot_reloadT<res2::GPUShape>(asset_descr, new_signatureUID); }

		//============================= material
		bool            materialPBR_createFromAsset (const char *runtimeName, MaterialPBRH *out_handle, res2::eLoadMode loadMode = res2::eLoadMode::onDemand);
		bool            materialPBR_create (MaterialPBRH *out_handle);
		void			release (MaterialPBRH &handle)																			{ res2__release(handle); }
		bool            get (MaterialPBRH handle, const MaterialPBRData **out, u32 timeout_msec = 0)							{ return res2__get_or_schedule_loadT<res2::MaterialPBR>(handle, out, timeout_msec); }
		eRes2LoadResult	res2__materialPBR_callback_on_load (const res2::CallbackData_onLoad *callback_data);
		void            res2__materialPBR_callback_on_destroy (res2::ResDescr *descr);
		void            res2__materialPBR_callback_on_hot_reload (res2::ResDescr *asset_descr, asset2::UID new_signatureUID)	{ res2__callback_on_hot_reloadT<res2::MaterialPBR>(asset_descr, new_signatureUID); }

		bool 			get_for_update (MaterialPBRH handle, MaterialPBRDataUpd *out, u32 timeout_msec = 0);
		bool 			materialPBR_update_renderer_binding (MaterialPBRDataUpd *data, u8 renderer_uid, u32 user_data);
		void            materialPBR_update_texture_diffuse (MaterialPBRDataUpd *data, TextureH tex_handle);
		void            materialPBR_update_texture_normal (MaterialPBRDataUpd *data, TextureH tex_handle);

		//============================= skeleton
		bool            skeleton_createFromAsset (const char *runtimeName, SkeletonH *out_handle, res2::eLoadMode loadMode = res2::eLoadMode::onDemand);
		bool            skeleton_createFromMemory (const u8 *buffer, u32 sizeof_buffer, SkeletonH *out_handle);
		bool            skeleton_create (const Skeleton &sk, SkeletonH *out_handle);
		void			release (SkeletonH &handle)																				{ res2__release(handle); }
		bool            get (SkeletonH handle, const SkeletonData **out, u32 timeout_msec = 0)									{ return res2__get_or_schedule_loadT<res2::Skeleton>(handle, out, timeout_msec); }
		eRes2LoadResult	res2__skeletonRes_callback_on_load (const res2::CallbackData_onLoad *callback_data);
		void            res2__skeletonRes_callback_on_destroy (res2::ResDescr *descr);
		void            res2__skeletonRes_callback_on_hot_reload (res2::ResDescr *asset_descr, asset2::UID new_signatureUID)	{ res2__callback_on_hot_reloadT<res2::Skeleton>(asset_descr, new_signatureUID); }

		//============================= model3d
		bool            model_createFromAsset (const char *runtimeName, ModelH *out_handle, res2::eLoadMode loadMode = res2::eLoadMode::onDemand);
		bool			model_create (SkeletonH handle_skeleton, u16 num_shape, u16 num_material, u16 num_meshes, ModelH *out_handle);
		void			release (ModelH &handle)																				{ res2__release(handle); }
		bool            get (ModelH handle, const ModelData **out, u32 timeout_msec = 0)										{ return res2__get_or_schedule_loadT<res2::Model3d>(handle, out, timeout_msec); }
		eRes2LoadResult	res2__modelRes_callback_on_load (const res2::CallbackData_onLoad *callback_data);
		void            res2__modelRes_callback_on_destroy (res2::ResDescr *descr);
		void            res2__modelRes_callback_on_hot_reload (res2::ResDescr *asset_descr, asset2::UID new_signatureUID)		{ res2__callback_on_hot_reloadT<res2::Model3d>(asset_descr, new_signatureUID); }

		//============================= shape
		/*bool            shape_createFromAsset (const char *runtimeName, ENGShape *out_handle, res2::eLoadMode loadMode = res2::eLoadMode::onDemand)		{ return proxyRes__createFromRuntimeNameT (runtimeName, out_handle, loadMode); }
		bool            shape_create (const VtxLayout &vtxLayout, u32 numVtx, u32 numIdx, ENGShape *out_handle);
		void			release (ENGShape handle)																	{ proxyRes__releaseT(handle); }
		bool            get (ENGShape handle, const res::Shape **out, u32 timeout_msec = 0)							{ return proxyRes__getT (handle, out, timeout_msec); }
		bool            hotreload (ENGShape handle)																	{ return proxyRes__hotReloadT(handle); }
		void 			internal__shape_on_afterCreate (void *res);
		void            internal__shape_on_destroy (void *res);
		void			internal__shape_on_unload (void *resIN);
		*/


		
		/*============================= model instance
		bool            modelinst_create (ENGModel3d handle_model, ENGModel3dInst *out_handle);
		void            release (ENGModel3dInst &handle)															{ res__release(handle.res_handle); handle.res_handle.setInvalid(); }
		bool            get (ENGModel3dInst handle, const res::Model3dInst **out, u32 timeout_msec = 0)				{ return res__getOrScheduleLoadT(handle, out, timeout_msec); }
		void            modelinst_applyTransform (ENGModel3dInst handle, const mat4x4f &matW);
		void 			modelinst_getWMatrix (ENGModel3dInst handle, mat4x4f *out_matW);
		void 			internal__modelinst_on_afterCreate (void *res);
		bool 			internal__modelinst_on_loadCallback(void *callback_data);
		void            internal__modelinst_on_destroy (void *res);
		void            internal__modelinst_on_unload (void *resIN);
		*/

	private:
		void			priv_modelinst_applyTransform_ric (const gos::Bone *model_listof_bones, gos::Bone *listof_bones, u32 boneIndex, const mat4x4f &parent_matW) const;
		bool 			priv_materialPBR_bind_texture (res2::ResDescr *res_materialPRB, asset2::UID texture_uid, TextureData **out_data);
		void 			priv__materialPBR_update_texture (MaterialPBRDataUpd *dataIN, TextureH tex_handle, res2::Texture2d::Data **target);

		void            priv_texture2D__add_to_mega_array (TextureData *data, u32 desired_index=u32MAX);
		void            priv_texture2D__remove_from_mega_array (TextureData *data);
		bool            priv_texture2D_create_ex (u16 dimx, u16 dimy, u8 nMipMap, eImageFormat fmt, eMemAccessMode memAccessMode, const void *srcDATA, TextureH *out_handle, gpu::StageHelper &stageHelper, u32 desired_texture_index);

		void 			priv_gpushape_build (const gos::Shape *shape, gpu::StageHelper &stageHelper, GPUShapeData *data);

	private:
		gos::Allocator				*allocator;
		bool						bQuitEngine;
		u64							frame_num;		//incrementato ad ogni chiamata di update()
		input::ResolvedEvtList		evtList;
		engine::VtxBufferMan		vtxBufferMan;
		engine::IdxBufferMan		idxBufferMan;
		gpu::StageHelper			stageHelper;

		gos::Logger					*asset_logger;
		asset2::DBContext			asset_ctx;

		TextureH					handle_texture_bianca;
		

	//============= res 2 =============
	private:
		typedef gos::FastHashMap<asset2::UID, res2::ResHandle>	Res2_Resource_vs_signatureUID_HashMap;
		typedef eRes2LoadResult (Engine::*FN_res2__on_file_loaded) (const res2::CallbackData_onLoad *data);
		typedef void (Engine::*FN_res2__on_destroy) (res2::ResDescr *descr);
		typedef void (Engine::*FN_res2__on_hot_reload) (res2::ResDescr *asset_descr, asset2::UID new_res_signatureUID);

	private:		
		void 			res2__flushLoaderThreadMsg();
		void 			res2__register_resource (res2::eResType type, FN_res2__on_file_loaded on_file_loaded, FN_res2__on_destroy on_destroy, FN_res2__on_hot_reload on_hot_reload);
		eRes2LoadResult	res2__callbak_on_file_loaded (res2::CallbackData_onLoad *data);
		void 			res2__callbak_on_destroy (res2::ResDescr *descr);
		void 			res2__callbak_on_hot_reload (res2::ResDescr *asset_descr, asset2::UID new_res_signatureUID);
		bool 			res2__get_or_schedule_load (res2::ResDescr *descr, u32 timeout_msec);
		void 			res2__release (res2::ResHandle handle);
		res2::ResDescr*	res2__get_descriptor (res2::ResHandle handle)	{ return res2MemManager.get(handle); }

						template<typename RES2_TYPE, typename ASSET = typename RES2_TYPE::Asset>
		ASSET*			res2__get_or_create_asset_from_rtname (const char *debug_called_from, const char *rtname, res2::eLoadMode loadMode)
						{
							asset2::UID signatureUID;
							asset2::UID assetUID;
							if (!asset2::asset_rtname_exists (asset_ctx, rtname, &assetUID, NULL, &signatureUID))
							{
								logger::err ("%s(%s) => invalid runtime name\n", debug_called_from, rtname);
								return NULL;
							}

							return res2__get_or_create_asset_from_UIDs<RES2_TYPE>(debug_called_from, assetUID, signatureUID, loadMode);
						}
						
						template<typename RES2_TYPE, typename ASSET = typename RES2_TYPE::Asset>
		ASSET*			res2__get_or_create_asset_from_assetUID (const char *debug_called_from, asset2::UID assetUID, res2::eLoadMode loadMode)
						{
							assert (assetUID.isAnAsset());
							asset2::UID signatureUID;
							if (!asset2::asset_get_info (asset_ctx, assetUID, NULL, &signatureUID))
							{
								logger::err ("%s => assetUID does not match with a signatureUID\n", debug_called_from);
								return NULL;
							}
							return res2__get_or_create_asset_from_UIDs<RES2_TYPE>(debug_called_from, assetUID, signatureUID, loadMode);
						}

						template<typename RES2_TYPE, typename ASSET = typename RES2_TYPE::Asset, typename RESOURCE = typename RES2_TYPE::Resource>
		ASSET*			res2__get_or_create_asset_from_UIDs (const char *debug_called_from, asset2::UID assetUID, asset2::UID signatureUID, res2::eLoadMode loadMode)
						{
							//using RESOURCE = typename RES2_TYPE::Resource;
							//using ASSET = typename RES2_TYPE::Asset;

							assert (assetUID.isAnAsset());
							assert (signatureUID.isASignature());

							//se l'assetUID esiste gia', lo ritorno
							res2::ResHandle hh;
							if (res2_known_assetUID_HashMap.find(assetUID, &hh))
							{
								res2::ResDescr *descr = res2__get_descriptor(hh);
								res2::inc_refCount (descr);
								return reinterpret_cast<ASSET*>( descr );
							}

							//altrimenti lo creo
							ASSET *asset = res2MemManager.reserveT<ASSET> (&hh);
							if (NULL == asset)
							{
								logger::err ("%s => can't create handle\n", debug_called_from);
								return NULL;
							}
							asset->base.UID = assetUID;
								
							//asset e' ready di default visto che dipendde unicamente dai suoi figli
							res2::change_state (&asset->base, res2::eStatus::ready, "created");
							res2_known_assetUID_HashMap.insertIfNotExists (assetUID, hh);	

							asset_logger->inc_indent();
							{
								//questo asset ha come figlio la risorsa <signatureUID>
								RESOURCE *child_res = res2__get_or_create_from_signatureUID<RES2_TYPE> (signatureUID);
								assert (NULL != child_res);
								res2::add_child (&asset->base, &child_res->base);

								//punto il campo asset->res_data_pt alla risorsa->data
								asset->res_data_pt = &child_res->data;

							}
							asset_logger->dec_indent();

							//se richiesto, schedulo il load
							if (res2::eLoadMode::asap == loadMode)
								res2__get_or_schedule_load (&asset->base, 0);

							return asset;
						}
						
						template<class RES2_TYPE>
		res2::ResDescr*	res2__find_child_with_type_and_data (const res2::ResDescr *descr, const void *data_ptr)
						{
							using RESOURCE = typename RES2_TYPE::Resource;
							using ASSET = typename RES2_TYPE::Asset;

							res2::ResHandleChain *p = descr->list_figli;
							while (p)
							{
								if (RES2_TYPE::get_type() == p->descr->get_type())
								{
									if (p->descr->isAnAsset())
									{
										ASSET *asset = reinterpret_cast<ASSET*> ( p->descr );
										if (data_ptr == asset->res_data_pt)
											return &asset->base;
									}
									else
									{
										RESOURCE *res = reinterpret_cast<RESOURCE*> (p->descr);
										if (data_ptr == &res->data)
											return &res->base;
									}
								}
								p = p->next;
							}
							return NULL;
						}

						template<class RES2_TYPE, class DATA=decltype(RES2_TYPE::Data)>
		bool 			res2__get_or_schedule_loadT (res2::ResHandle handle, DATA **out, u32 timeout_msec)
						{
							using RESOURCE = typename RES2_TYPE::Resource;
							using ASSET = typename RES2_TYPE::Asset;

							//dall'handle recupero il descrittore
							res2::ResDescr *descr = res2MemManager.get (handle);
							if (NULL == descr)
								return false;

							if (!res2__get_or_schedule_load (descr, timeout_msec))
								return false;

							//dal descrittore deduco se stiamo puntando ad una risorsa o un asset
							if (descr->UID.isAnAsset())
							{
								ASSET *asset = reinterpret_cast<ASSET*> (descr);
								*out = asset->res_data_pt;
							}
							else
							{
								RESOURCE *res = reinterpret_cast<RESOURCE*> (descr);
								*out = &res->data;
							}
							return true;
						}

						template<typename RES2_TYPE, typename RESOURCE = typename RES2_TYPE::Resource>
		RESOURCE*		res2__get_or_create_from_signatureUID (asset2::UID signatureUID)
						{
							//using RESOURCE = typename RES2_TYPE::Resource;
							assert (signatureUID.isValid() && signatureUID.isASignature());

							//se la signatureUID esiste gia'...
							res2::ResHandle hh;
							if (res2_known_signatureUID_HashMap.find(signatureUID, &hh))
							{
								res2::ResDescr *descr = res2__get_descriptor(hh);
								res2::inc_refCount (descr);
								return reinterpret_cast<RESOURCE*>( descr );
							}

							//altrimenti la creo
							RESOURCE *res = res2MemManager.reserveT<RESOURCE> (&hh);
							if (NULL == res)
							{
								logger::err ("Engine::res2__get_or_create_from_signatureUID() => can't create handle\n");
								return NULL;
							}
							
							res->base.UID = signatureUID;
							res2::change_state (&res->base, res2::eStatus::unloaded, "created");
							res2_known_signatureUID_HashMap.insertIfNotExists (signatureUID, hh);
							return res;
						}		
	
						template<typename RES2_TYPE>
		void 			res2__callback_on_hot_reloadT (res2::ResDescr *asset_descr, asset2::UID new_signatureUID)
						{
							using RESOURCE = typename RES2_TYPE::Resource;
							using ASSET = typename RES2_TYPE::Asset;

							//questa viene chiamata solo sugli asset
							assert (asset_descr->UID.isValid() && asset_descr->UID.isAnAsset());
							assert (new_signatureUID.isValid() && new_signatureUID.isASignature());

							RESOURCE *new_child_res = res2__get_or_create_from_signatureUID<RESOURCE> (new_signatureUID);
							assert (NULL != new_child_res);
							res2::add_child (asset_descr, &new_child_res->base);

							//punto il campo asset->res_data_pt alla risorsa->data
							ASSET *asset = reinterpret_cast<ASSET*>( asset_descr );
							asset->res_data_pt = &new_child_res->data;
						}

	private:
		res2::MemManager		res2MemManager;
		Res2_Resource_vs_signatureUID_HashMap	res2_known_signatureUID_HashMap;
		Res2_Resource_vs_signatureUID_HashMap	res2_known_assetUID_HashMap;
		FN_res2__on_file_loaded	res2FNList_onFileLoaded[(u8)res2::eResType::NUM_MAX];
		FN_res2__on_destroy		res2FNList_onDestroy[(u8)res2::eResType::NUM_MAX];
		FN_res2__on_hot_reload	res2FNList_onHotReload[(u8)res2::eResType::NUM_MAX];

		//================= res2  loader thread stuff ============
	private:
		struct sLoaderThreadInitParams
		{
			gos::Signal		    hEvent_started;
			HThreadMsgR		    msgqR;
			HThreadMsgW		    msgqW;
			gos::Logger		    *logger;
			gos::GPU            *gpu;
			asset2::DBContext   *ctx;
			gos::Allocator		*engine_allocator;
			Engine				*engine;
		};
		
	private:
		static constexpr u32		RES2__MSG_FOR_LOADER_THREAD__DIE		        	= 0xff;
		static constexpr u32		RES2__MSG_FOR_LOADER_THREAD__LOAD					= 0x10;
		static constexpr u32		RES2__MSG_FOR_LOADER_THREAD__LOAD_CONTINUED			= 0x11;
		static constexpr u32		RES2__MSG_FOR_LOADER_THREAD__LOAD_FINISHED			= 0x12;

	public:
		static constexpr u32		RES2__MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_OK 				= 0x01;
		static constexpr u32		RES2__MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_OK_WANT_CALLBACK = 0x02;
		static constexpr u32		RES2__MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_KO 				= 0x03;

	private:		
		static i16	        res2__LoaderThread_mainFN (void *params);
		GOSThreadHandle 	res2__hThreadLoader;
		HThreadMsgR     	res2__msgq_1R;
		HThreadMsgW     	res2__msgq_1W;
		HThreadMsgR     	res2__msgq_2R;
		HThreadMsgW     	res2__msgq_2W;           


	}; //class Engine
} //namespace gos


#endif //_gosEngine_h_

