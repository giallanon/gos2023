#ifndef _gosEngine_h_
#define _gosEngine_h_
#include "gosEngineEnumAndDefine.h"
#include "gosEngine_vtxBufferMan.h"
#include "gosEngine_idxBufferMan.h"
#include "gosEngine_scene.h"
#include "renderPipe/gosEngineRenderPipe.h"
#include "res/gosEngineRes.h"
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
		bool            texture2D_createFromAsset (const char *runtimeName, TextureH *out_handle, res::eLoadMode loadMode = res::eLoadMode::onDemand);
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
		bool            vtxshader_createFromAsset (const char *runtimeName, VtxShaderH *out_handle, res::eLoadMode loadMode = res::eLoadMode::onDemand);
		bool            vtxshader_createFromFile (const char *filename, const char *mainFnName, VtxShaderH *out_handle);
		bool            vtxshader_createFromMemory (const void *bufferIN, u32 bufferSize, const char *mainFnName, VtxShaderH *out_handle);
		void            release (VtxShaderH &handle)																			{ res2__release(handle); }
		bool            get (VtxShaderH handle, const VtxShaderData **out, u32 timeout_msec = 0)								{ return res2__get_or_schedule_loadT<res2::VtxShader>(handle, out, timeout_msec); }
		eRes2LoadResult	res2__vtxShaderRes_callback_on_load (const res2::CallbackData_onLoad *callback_data);
		void            res2__vtxShaderRes_callback_on_destroy (res2::ResDescr *descr);
		void            res2__vtxShaderRes_callback_on_hot_reload (res2::ResDescr *asset_descr, asset2::UID new_signatureUID)	{ res2__callback_on_hot_reloadT<res2::VtxShader>(asset_descr, new_signatureUID); }

		//============================= pxlshader
		bool            pxlshader_createFromAsset (const char *runtimeName, PxlShaderH *out_handle, res::eLoadMode loadMode = res::eLoadMode::onDemand);
		bool            pxlshader_createFromFile (const char *filename, const char *mainFnName, PxlShaderH *out_handle);
		bool            pxlshader_createFromMemory (const void *bufferIN, u32 bufferSize, const char *mainFnName, PxlShaderH *out_handle);
		void            release (PxlShaderH &handle)																			{ res2__release(handle); }
		bool            get (PxlShaderH handle, const PxlShaderData **out, u32 timeout_msec = 0)								{ return res2__get_or_schedule_loadT<res2::PxlShader>(handle, out, timeout_msec); }
		eRes2LoadResult	res2__pxlShaderRes_callback_on_load (const res2::CallbackData_onLoad *callback_data);
		void            res2__pxlShaderRes_callback_on_destroy (res2::ResDescr *descr);
		void            res2__pxlShaderRes_callback_on_hot_reload (res2::ResDescr *asset_descr, asset2::UID new_signatureUID)	{ res2__callback_on_hot_reloadT<res2::PxlShader>(asset_descr, new_signatureUID); }

		//============================= pipeline
		bool            pipeline_createFromAsset (const char *runtimeName, PipelineH *out_handle, res::eLoadMode loadMode = res::eLoadMode::onDemand);
		bool            pipeline_create (const gpu::Pipeline_def &rpd, PipelineH *out_handle);
		void            release (PipelineH &handle)																				{ res2__release(handle); }
		bool            get (PipelineH handle, const PipelineData **out, u32 timeout_msec = 0)									{ return res2__get_or_schedule_loadT<res2::Pipeline>(handle, out, timeout_msec); }
		eRes2LoadResult	res2__pipelineRes_callback_on_load (const res2::CallbackData_onLoad *callback_data);
		void            res2__pipelineRes_callback_on_destroy (res2::ResDescr *descr);
		void            res2__pipelineRes_callback_on_hot_reload (res2::ResDescr *asset_descr, asset2::UID new_signatureUID)	{ res2__callback_on_hot_reloadT<res2::Pipeline>(asset_descr, new_signatureUID); }

		//============================= GPUShape
		bool            GPUShape_createFromAsset (const char *runtimeName, GPUShapeH *out_handle, res::eLoadMode loadMode = res::eLoadMode::onDemand);
		bool            GPUShape_create (const gos::Shape *shape, gpu::StageHelper &stageHelper, GPUShapeH *out_handle);
		void            release (GPUShapeH &handle)																				{ res2__release(handle); }
		bool            get (GPUShapeH handle, const GPUShapeData **out, u32 timeout_msec = 0)									{ return res2__get_or_schedule_loadT<res2::GPUShape>(handle, out, timeout_msec); }
		eRes2LoadResult	res2__gpushapeRes_callback_on_load (const res2::CallbackData_onLoad *callback_data);
		void            res2__gpushapeRes_callback_on_destroy (res2::ResDescr *descr);
		void            res2__gpushapeRes_callback_on_hot_reload (res2::ResDescr *asset_descr, asset2::UID new_signatureUID)	{ res2__callback_on_hot_reloadT<res2::GPUShape>(asset_descr, new_signatureUID); }

		//============================= shape
		bool            shape_createFromAsset (const char *runtimeName, ENGShape *out_handle, res::eLoadMode loadMode = res::eLoadMode::onDemand)		{ return proxyRes__createFromRuntimeNameT (runtimeName, out_handle, loadMode); }
		bool            shape_create (const VtxLayout &vtxLayout, u32 numVtx, u32 numIdx, ENGShape *out_handle);
		void			release (ENGShape handle)																	{ proxyRes__releaseT(handle); }
		bool            get (ENGShape handle, const res::Shape **out, u32 timeout_msec = 0)							{ return proxyRes__getT (handle, out, timeout_msec); }
		bool            hotreload (ENGShape handle)																	{ return proxyRes__hotReloadT(handle); }
		void 			internal__shape_on_afterCreate (void *res);
		void            internal__shape_on_destroy (void *res);
		void			internal__shape_on_unload (void *resIN);

		//============================= material
		bool            materialPBR_createFromAsset (const char *runtimeName, MaterialPBRH *out_handle, res::eLoadMode loadMode = res::eLoadMode::onDemand);
						//TODO: l'idea e' che dopo "create" devo poter ottenere un pt al materiale per poterlo modificare
		bool            materialPBR_create (MaterialPBRH *out_handle);
		void			release (MaterialPBRH &handle)																{ res2__release(handle); }
		bool            get (MaterialPBRH handle, const MaterialPBRData **out, u32 timeout_msec = 0)				{ return res2__get_or_schedule_loadT<res2::MaterialPBR>(handle, out, timeout_msec); }
		eRes2LoadResult	res2__materialPBR_callback_on_load (const res2::CallbackData_onLoad *callback_data);
		void            res2__materialPBR_callback_on_destroy (res2::ResDescr *descr);
		void            res2__materialPBR_callback_on_hot_reload (res2::ResDescr *asset_descr, asset2::UID new_signatureUID)	{ res2__callback_on_hot_reloadT<res2::MaterialPBR>(asset_descr, new_signatureUID); }

		bool 			get_for_update (MaterialPBRH handle, MaterialPBRDataUpd *out, u32 timeout_msec = 0);
		bool 			materialPBR_update_renderer_binding (MaterialPBRDataUpd *data, u8 renderer_uid, u32 user_data);
		void            materialPBR_update_texture_diffuse (MaterialPBRDataUpd *data, TextureH tex_handle);
		void            materialPBR_update_texture_normal (MaterialPBRDataUpd *data, TextureH tex_handle);
		


		








		//============================= skeleton
		bool            skeleton_createFromAsset (const char *runtimeName, ENGSkeleton *out_handle, res::eLoadMode loadMode = res::eLoadMode::onDemand)	{ return proxyRes__createFromRuntimeNameT (runtimeName, out_handle, loadMode); }
		bool            skeleton_createFromMemory (const u8 *buffer, u32 sizeof_buffer, ENGSkeleton *out_handle);
		bool            skeleton_create (const Skeleton &sk, ENGSkeleton *out_handle);
		void            release (ENGSkeleton handle)																{ proxyRes__releaseT(handle); }
		bool            get (ENGSkeleton handle, const res::Skeleton **out, u32 timeout_msec = 0)        			{ return proxyRes__getT (handle, out, timeout_msec); }
		bool            hotreload (ENGSkeleton handle)																{ return proxyRes__hotReloadT(handle); }
		void 			internal__skeleton_on_afterCreate (void *res);
		void            internal__skeleton_on_destroy (void *res);
		void            internal__skeleton_on_unload (void *resIN);



		//============================= model3d
		bool            model_createFromAsset (const char *runtimeName, ENGModel3d *out_handle, res::eLoadMode loadMode = res::eLoadMode::onDemand)	{ return proxyRes__createFromRuntimeNameT (runtimeName, out_handle, loadMode); }
		gos::Model*		model_create (ENGSkeleton handle_skeleton, u16 num_shape, u16 num_material, u16 num_meshes, ENGModel3d *out_handle);
		void            release (ENGModel3d handle)																	{ proxyRes__releaseT(handle); }
		bool            get (ENGModel3d handle, const res::Model3d **out, u32 timeout_msec = 0)        				{ return proxyRes__getT (handle, out, timeout_msec); }
		bool            hotreload (ENGModel3d handle)                                                               { return proxyRes__hotReloadT(handle); }
		void 			internal__model_on_afterCreate (void *res);
		bool 			internal__model_on_loadCallback(void *callback_data);
		void            internal__model_on_destroy (void *res);
		void            internal__model_on_unload (void *resIN);
		
		//============================= model instance
		bool            modelinst_create (ENGModel3d handle_model, ENGModel3dInst *out_handle);
		void            release (ENGModel3dInst &handle)															{ res__release(handle.res_handle); handle.res_handle.setInvalid(); }
		bool            get (ENGModel3dInst handle, const res::Model3dInst **out, u32 timeout_msec = 0)				{ return res__getOrScheduleLoadT(handle, out, timeout_msec); }
		void            modelinst_applyTransform (ENGModel3dInst handle, const mat4x4f &matW);
		void 			modelinst_getWMatrix (ENGModel3dInst handle, mat4x4f *out_matW);
		void 			internal__modelinst_on_afterCreate (void *res);
		bool 			internal__modelinst_on_loadCallback(void *callback_data);
		void            internal__modelinst_on_destroy (void *res);
		void            internal__modelinst_on_unload (void *resIN);

	public:
						template<class RESOURCE_PADRE, class RESOURCE_FIGLIO>
		void 			internal__resAddChild (RESOURCE_PADRE *padre, RESOURCE_FIGLIO *figlio)
						{
							res__addChild (&padre->_descr, &figlio->_descr);
						}

						template<class RESOURCE_PADRE, class HANDLE_FIGLIO>
		void 			internal__resAddChild (RESOURCE_PADRE *padre, HANDLE_FIGLIO handle_figlio)
						{
							res::Descr *figlio = res__getDescriptor(handle_figlio.res_handle);
							assert (NULL != figlio);
							res__addChild (&padre->_descr, figlio);
						}						

						template<class RESOURCE>
		bool 			internal__getProxyResFromAssetUID (asset2::UID assetUID, RESOURCE **out)
						{
							assert (assetUID.isAnAsset());

							//se l'assetUID e' gia' stato mappato, ritorno il suo proxy handle
							u32 index;
							if (!proxyResInfo.get_index_from_assetUID (assetUID, &index))
								return false;
							
							const ProxyRes *proxyRes = proxyResInfo.query(index);
							if (NULL == proxyRes)
								return false;

							(*out) = (RESOURCE*)res__getDescriptor (proxyRes->res_handle);
							return true;
						}	

						template<class RESOURCE, class PROXYRES_HANDLE>
		bool 			internal__getProxyResFromAssetUID (asset2::UID assetUID, RESOURCE **out, PROXYRES_HANDLE *out_handle)
						{
							assert (NULL != out_handle);
							assert (assetUID.isAnAsset());
							out_handle->setInvalid();

							//se l'assetUID e' gia' stato mappato, ritorno il suo proxy handle
							if (!proxyResInfo.get_index_from_assetUID (assetUID, &out_handle->index))
								return false;
							
							const ProxyRes *proxyRes = proxyResInfo.query(out_handle->index);
							if (NULL == proxyRes)
								return false;

							(*out) = (RESOURCE*)res__getDescriptor (proxyRes->res_handle);
							return true;
						}		

	private:
		typedef FastHashMap<asset2::UID, u32> HashListOfLoadedUID;
		
		struct sUnloadInfo
		{
			res::Handle res_handle;
			u32         timer_msec;
		};

		class ProxyResInfo
		{
		private:
			static constexpr u32 NUM_MAX_PROXY_HANDLE = 65536;

		public:
							ProxyResInfo()							{ num=0; list=NULL; localAllocator=NULL; }
							~ProxyResInfo()							{ unsetup(); }
			void 			setup (gos::Allocator *allocator)		{ assert(NULL==localAllocator); localAllocator=allocator; num=0; list = GOSALLOCT(ProxyRes*, localAllocator, NUM_MAX_PROXY_HANDLE); map_of_assetUID_to_proxyIndex.setup(localAllocator, 8192); }
			void 			unsetup()								{ if (NULL != list) { GOSFREE(localAllocator,list); list=NULL; num=0; localAllocator=NULL; map_of_assetUID_to_proxyIndex.unsetup(); } }

			ProxyRes*		reserve (const asset2::UID assetUID, u32 *out_index)
			{ 
				if (num >= NUM_MAX_PROXY_HANDLE)
				{
					DBGBREAK;
					return NULL;
				}
				
				*out_index = num++;
				ProxyRes *ret = &list[*out_index]; 
				ret->assetUID = assetUID;
				ret->res_handle.setInvalid();

				if (assetUID.isValid())
					map_of_assetUID_to_proxyIndex.insertIfNotExists (assetUID, *out_index);
				return ret;
			}
			const ProxyRes*	query (u32 index) const															{ if (index >= num) return NULL; return &list[index]; }
			ProxyRes*		get (u32 index)																	{ if (index >= num) return NULL; return &list[index]; }
			bool			get_index_from_assetUID (const asset2::UID assetUID, u32 *out_index) const		{ return map_of_assetUID_to_proxyIndex.find(assetUID, out_index); }

		private:
			u32				num;
			ProxyRes		*list;
			gos::FastHashMap<asset2::UID, u32> map_of_assetUID_to_proxyIndex;
			gos::Allocator	*localAllocator;
			
		};

	private:
		void 			priv_flushLoaderThreadMsg();
		void 			priv_handle_res_hotreload();
		void			priv_modelinst_applyTransform_ric (const gos::Bone *model_listof_bones, gos::Bone *listof_bones, u32 boneIndex, const mat4x4f &parent_matW) const;
		bool 			priv_materialPBR_bind_texture (res2::ResDescr *res_materialPRB, asset2::UID texture_uid, TextureData **out_data);
		void 			priv__materialPBR_update_texture (MaterialPBRDataUpd *dataIN, TextureH tex_handle, res2::Texture2d::Data **target);

		void            priv_texture2D__add_to_mega_array (TextureData *data, u32 desired_index=u32MAX);
		void            priv_texture2D__remove_from_mega_array (TextureData *data);
		bool            priv_texture2D_create_ex (u16 dimx, u16 dimy, u8 nMipMap, eImageFormat fmt, eMemAccessMode memAccessMode, const void *srcDATA, TextureH *out_handle, gpu::StageHelper &stageHelper, u32 desired_texture_index);

		void 			priv_gpushape_build (const gos::Shape *shape, gpu::StageHelper &stageHelper, GPUShapeData *data);

		void 			res__printInfo (const void *res, const char *debug_info) const;
		void            res__set_status (res::Descr *res, res::eStatus new_status);
		void            res__on_children_become_ready (res::Descr *resPadre);
		void            res__on_children_become_notready (res::Descr *resPadre);

		res::Descr*		res__createHandle (res::eType res_type, res::eStatus status, asset2::UID signatureUID, res::Handle *out_handle);
		res::Descr*		res__getOrCreateHandleFromRuntimeName (const char *runtimeName, res::eLoadMode loadMode, res::Handle *out_handle);
		res::Descr*		res__getOrCreateHandleFromSignatureUID (asset2::UID signatureUID, res::Handle *out_handle);
		void 			res__bindEvents (res::Handle handle, res::Descr *res);
		res::Descr*		res__getDescriptor (res::Handle handle);
		bool            res__release (res::Handle handle);
		bool            res__release (res::Descr *res);
		bool            res__hotreload (res::Handle handle);
		void            res__do_destroy (res::Descr *res);
		bool 			res__getOrScheduleLoad (res::Handle handle, const res::Descr **out, u32 timeout_msec = 0);
		bool 			res__scheduleLoadIfNeeded (res::Descr *res, u64 timeout_msec);
		
		bool 			res__signatureUID_to_resType (asset2::UID signatureUID, res::eType *out_res_type) const;
		res::HandleChain*	res__newHandleChain ();
		void 			res__freeHandleChain (res::HandleChain *p);
		void 			res__addChild (res::Descr *padre_res, res::Descr *child_res);

						template<class HANDLE, class RESOURCE>
		bool			res__getOrScheduleLoadT (HANDLE handle, const RESOURCE **out, u32 timeout_msec)
						{
							const res::Descr *res;
							const bool ret = res__getOrScheduleLoad(handle.res_handle, &res, timeout_msec);
							(*out) = reinterpret_cast<const RESOURCE*>(res);
							return ret;
						}


						template<class PROXYRES_HANDLE>
		res::Descr*		proxyRes__getDescriptorT (PROXYRES_HANDLE handle)
						{
							const ProxyRes *proxyRes = proxyResInfo.query(handle.index);
							if (NULL == proxyRes) return NULL;
							return res__getDescriptor(proxyRes->res_handle);
						}

						template<class PROXYRES_HANDLE>
		bool 			proxyRes__createFromRuntimeNameT (const char *rtname, PROXYRES_HANDLE *out_handle, res::eLoadMode loadMode)
						{
							assert (NULL != out_handle);
							out_handle->setInvalid();

							asset2::UID signatureUID;
							asset2::UID assetUID;
							if (!asset2::asset_rtname_exists (asset_ctx, rtname, &assetUID, NULL, &signatureUID))
							{
								logger::err ("Engine::priv_proxyRes_createFromRuntimeName(%s) => invalid runtime name\n", rtname);
								return false;
							}

							//se l'assetUID e' gia' stato mappato, ritorno il suo proxy handle
							if (proxyResInfo.get_index_from_assetUID (assetUID, &out_handle->index))
								return true;
							
							//altrimenti lo creo
							ProxyRes *proxyRes = proxyResInfo.reserve (assetUID, &out_handle->index);
							return res__getOrCreateHandleFromRuntimeName  (rtname, loadMode, &proxyRes->res_handle); 
						}	

						template<class PROXYRES_HANDLE>
		void 			proxyRes__releaseT (const PROXYRES_HANDLE handle)
						{ 
							ProxyRes *proxyRes = proxyResInfo.get(handle.index);
							if (NULL != proxyRes)
							{
								res__release(proxyRes->res_handle);
								proxyRes->res_handle.setInvalid(); 
							}
						}

						template<class PROXYRES_HANDLE, class RESOURCE>
		bool 			proxyRes__getT (const PROXYRES_HANDLE handle, RESOURCE **out_res, u32 timeout_msec)
						{ 
							const ProxyRes *proxyRes = proxyResInfo.query(handle.index);
							if (NULL == proxyRes)
							{
								DBGBREAK;
								return false;
							}

							const res::Descr *res;
							const bool ret = res__getOrScheduleLoad (proxyRes->res_handle, &res, timeout_msec);
							(*out_res) = reinterpret_cast<const RESOURCE*>(res);
							return ret;	
						}
												
						template<class PROXYRES_HANDLE>
		bool 			proxyRes__hotReloadT (PROXYRES_HANDLE handle)
						{
							ProxyRes *proxyRes = proxyResInfo.get(handle.index);
							if (NULL == proxyRes)
							{
								DBGBREAK;
								return false;
							}

							return res__hotreload (proxyRes->res_handle); 
						}



	private:
		gos::Allocator                              *allocator;
		bool                                        bQuitEngine;
		u64											frame_num;		//incrementato ad ogni chiamata di update()
		input::ResolvedEvtList                      evtList;
		engine::VtxBufferMan                        vtxBufferMan;
		engine::IdxBufferMan                        idxBufferMan;
		gpu::StageHelper							stageHelper;

		gos::Logger                                 *asset_logger;
		asset2::DBContext                           asset_ctx;
		HashListOfLoadedUID			                listof_known_signatureUID;	//mappa signatureUID to u32 che e' l'handle della risorsa nell'engine

		res::Manager 								resManager;
		FastHashMap<ENGShape, ENGGPUShape>			map_of_shape_to_gpushape;
		gos::ObjectPool<res::HandleChain>			resHandleChainPool;
		gos::FastArray<sUnloadInfo>                 list_of_res_to_be_hotreloaded;
		TextureH		                            handle_texture_bianca;
		ProxyResInfo								proxyResInfo;
		

	//================= loader thread stuff ============
	private:
		static constexpr u32		MSG_FOR_LOADER_THREAD__DIE		        = 0xff;
		static constexpr u32		MSG_FOR_LOADER_THREAD__LOAD	            = 0x01;
		static constexpr u32		MSG_FOR_LOADER_THREAD__LOAD_CONTINUE	= 0x02;

	public:
		static constexpr u32		MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_OK 	= 0x01;
		static constexpr u32		MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_KO 	= 0x02;
		static constexpr u32		MSG_FROM_LOADER_THREAD__ON_LOAD_CALLBACK		= 0x03;

		struct res2__loaderThreadAnswer_onResFileLoaded
		{
			res2::ResDescr	*descr;
			u64 			file_size;
			const void 		*file_buffer;
		};

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
		
		static i16	        LoaderThread_mainFN (void *params);
		GOSThreadHandle 	hThreadLoader;
		HThreadMsgR     	msgq_1R;
		HThreadMsgW     	msgq_1W;
		HThreadMsgR     	msgq_2R;
		HThreadMsgW     	msgq_2W;           


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

		//				template<class RES2_TYPE>
		//typename RES2_TYPE::Asset*	res2__get_or_create_asset_from_rtname (const char *debug_called_from, const char *rtname, res::eLoadMode loadMode)
						template<typename RES2_TYPE, typename ASSET = typename RES2_TYPE::Asset>
		ASSET*			res2__get_or_create_asset_from_rtname (const char *debug_called_from, const char *rtname, res::eLoadMode loadMode)
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
						
		//				template<class RES2_TYPE>
		//typename RES2_TYPE::Asset*	res2__get_or_create_asset_from_assetUID (const char *debug_called_from, asset2::UID assetUID, res::eLoadMode loadMode)
						template<typename RES2_TYPE, typename ASSET = typename RES2_TYPE::Asset>
		ASSET*			res2__get_or_create_asset_from_assetUID (const char *debug_called_from, asset2::UID assetUID, res::eLoadMode loadMode)
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

		//				template<class RES2_TYPE>
		//typename RES2_TYPE::Asset*	res2__get_or_create_asset_from_UIDs (const char *debug_called_from, asset2::UID assetUID, asset2::UID signatureUID, res::eLoadMode loadMode)
						template<typename RES2_TYPE, typename ASSET = typename RES2_TYPE::Asset, typename RESOURCE = typename RES2_TYPE::Resource>
		ASSET*			res2__get_or_create_asset_from_UIDs (const char *debug_called_from, asset2::UID assetUID, asset2::UID signatureUID, res::eLoadMode loadMode)
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
							if (res::eLoadMode::asap == loadMode)
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

		//				template<typename RES2_TYPE>
		//typename RES2_TYPE::Resource* res2__get_or_create_from_signatureUID (asset2::UID signatureUID)
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

