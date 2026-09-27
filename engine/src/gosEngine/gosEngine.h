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
		bool            vtxBuffer_create (u32 sizeInByte, eMemAccessMode mode, ENGVtxBuffer2 *out_handle);
		void            release (ENGVtxBuffer2 handle)																{ proxyRes__releaseT(handle); }
		bool            get (ENGVtxBuffer2 handle, const res::VtxBuffer **out)										{ return proxyRes__getT(handle, out, 0); }
		void            internal__vtxBuffer_on_afterCreate (void *res);
		void            internal__vtxBuffer_on_destroy (void *res);

		//============================= idxBuffer
		bool            idxBuffer_create (u32 sizeInByte, eMemAccessMode mode, ENGIdxBuffer2 *out_handle);
		void            release (ENGIdxBuffer2 handle)																{ proxyRes__releaseT(handle); }
		bool            get (ENGIdxBuffer2 handle, const res::IdxBuffer **out)										{ return proxyRes__getT(handle, out, 0); }
		void 			internal__idxBuffer_on_afterCreate (void *res);
		void            internal__idxBuffer_on_destroy (void *res);

		//============================= vtxshader
		bool            vtxshader_createFromAsset (const char *runtimeName, ENGVtxShader2 *out_handle, res::eLoadMode loadMode = res::eLoadMode::onDemand)	{ return proxyRes__createFromRuntimeNameT (runtimeName, out_handle, loadMode); }
		bool            vtxshader_createFromFile (const char *filename, const char *mainFnName, ENGVtxShader2 *out_handle);
		bool            vtxshader_createFromMemory (const void *bufferIN, u32 bufferSize, const char *mainFnName, ENGVtxShader2 *out_handle);
		void            release (ENGVtxShader2 handle)                                                            	{ proxyRes__releaseT(handle); }
		bool            get (ENGVtxShader2 handle, const res::Shader **out, u64 timeout_msec = 0)            		{ return proxyRes__getT(handle, out, timeout_msec); }
		bool            hotreload (ENGVtxShader2 handle)															{ return proxyRes__hotReloadT(handle); }
		void 			internal__vtxshader_on_afterCreate (void *res);
		void            internal__vtxshader_on_destroy (void *res);
		void            internal__vtxshader_on_unload (void *resIN);

		//============================= pxlshader
		bool            pxlshader_createFromAsset (const char *runtimeName, ENGPxlShader2 *out_handle, res::eLoadMode loadMode = res::eLoadMode::onDemand)	{ return proxyRes__createFromRuntimeNameT (runtimeName, out_handle, loadMode); }
		bool            pxlshader_createFromFile (const char *filename, const char *mainFnName, ENGPxlShader2 *out_handle);
		bool            pxlshader_createFromMemory (const void *bufferIN, u32 bufferSize, const char *mainFnName, ENGPxlShader2 *out_handle);
		void            release (ENGPxlShader2 handle)                                                            	{ proxyRes__releaseT(handle); }
		bool            get (ENGPxlShader2 handle, const res::Shader **out, u64 timeout_msec = 0)            		{ return proxyRes__getT(handle, out, timeout_msec); }
		bool            hotreload (ENGPxlShader2 handle)															{ return proxyRes__hotReloadT(handle); }
		void 			internal__pxlshader_on_afterCreate (void *res);
		void            internal__pxlshader_on_destroy (void *res);
		void			internal__pxlshader_on_unload (void *resIN);

		//============================= pipeline
		bool            pipeline_createFromAsset (const char *runtimeName, ENGPipeline *out_handle, res::eLoadMode loadMode = res::eLoadMode::onDemand)	{ return proxyRes__createFromRuntimeNameT (runtimeName, out_handle, loadMode); }
		bool            pipeline_create (const gpu::Pipeline_def &rpd, ENGPipeline *out_handle);
		void            release (ENGPipeline handle)                                                              	{ proxyRes__releaseT(handle); }
		bool            get (ENGPipeline handle, const res::Pipeline **out, u64 timeout_msec = 0)            		{ return proxyRes__getT(handle, out, timeout_msec); }
		bool            hotreload (ENGPipeline handle)																{ return proxyRes__hotReloadT(handle); }
		void 			internal__pipeline_on_afterCreate (void *res);
		void            internal__pipeline_on_destroy (void *res);
		void			internal__pipeline_on_unload (void *resIN);

		//============================= texture2D
		bool            texture2D_createFromAsset (const char *runtimeName, ENGTexture2 *out_handle, res::eLoadMode loadMode = res::eLoadMode::onDemand)	{ return proxyRes__createFromRuntimeNameT (runtimeName, out_handle, loadMode); }
		bool            texture2D_create (u16 dimx, u16 dimy, u8 nMipMap, eImageFormat fmt, eMemAccessMode memAccessMode, const void *srcDATA, ENGTexture2 *out_handle, gpu::StageHelper &stageHelper);
		bool            texture2D_create (const gos::Image *im, u8 srcTextureNum, eMemAccessMode memAccessMode, ENGTexture2 *out_handle, gpu::StageHelper &stageHelper);
		void            release (ENGTexture2 &handle)																{ proxyRes__releaseT(handle); }
		bool            get (ENGTexture2 handle, const res::Texture2d **out, u64 timeout_msec = 0)					{ return proxyRes__getT (handle, out, timeout_msec); }
		bool            hotreload (ENGTexture2 handle)																{ return proxyRes__hotReloadT(handle); }
		void 			internal__texture2D_on_afterCreate (void *res);
		bool 			internal__texture2D_loadCallback(void *callback_data);
		void            internal__texture2D_on_destroy (void *res);
		void            internal__texture2D_on_afterLoad(void *res);
		void            internal__texture2D_on_unload (void *resIN);

		bool            get_texture_bianca (const res::Texture2d **out)               								{ return get(handle_texture_bianca, out, 0); }

		//============================= shape
		bool            shape_createFromAsset (const char *runtimeName, ENGShape *out_handle, res::eLoadMode loadMode = res::eLoadMode::onDemand)		{ return res__getOrCreateHandleFromRuntimeName  (runtimeName, loadMode, &out_handle->res_handle); }
		bool            shape_create (const VtxLayout &vtxLayout, u32 numVtx, u32 numIdx, ENGShape *out_handle);
		void			release (ENGShape &handle)																	{ res__release(handle.res_handle); handle.res_handle.setInvalid(); }
		bool            get (ENGShape handle, const res::Shape **out, u64 timeout_msec = 0)							{ return res__getOrScheduleLoadT(handle, out, timeout_msec); }
		bool            hotreload (ENGShape handle)																	{ return res__hotreload (handle.res_handle); }
		void 			internal__shape_on_afterCreate (void *res);
		void            internal__shape_on_destroy (void *res);
		void			internal__shape_on_unload (void *resIN);

		//============================= GPUShape
		bool            GPUShape_create (ENGShape handle_shape, ENGGPUShape *out_handle);
		bool            GPUShape_create (const gos::Shape *shape, gpu::StageHelper &stageHelper, ENGGPUShape *out_handle);
		void            release (ENGGPUShape &handle);
		bool            get (ENGGPUShape handle, const res::GPUShape **out)                                  		{ return res__getOrScheduleLoadT(handle, out, 0); }
		bool            get (ENGShape handle, const res::GPUShape **out);
		void 			internal__GPUShape_reset (res::GPUShape *res);
		void 			internal__GPUShape_on_afterCreate (void *res);
		bool 			internal__GPUShape_on_loadCallback(void *callback_data);
		void            internal__GPUShape_on_destroy (void *res);
		void			internal__GPUShape_on_unload (void *resIN);
		

		//============================= skeleton
		bool            skeleton_createFromAsset (const char *runtimeName, ENGSkeleton *out_handle, res::eLoadMode loadMode = res::eLoadMode::onDemand)	{ return res__getOrCreateHandleFromRuntimeName  (runtimeName, loadMode, &out_handle->res_handle); }
		bool            skeleton_createFromMemory (const u8 *buffer, u32 sizeof_buffer, ENGSkeleton *out_handle);
		bool            skeleton_create (const Skeleton &sk, ENGSkeleton *out_handle);
		void            release (ENGSkeleton &handle)																{ res__release(handle.res_handle); handle.res_handle.setInvalid(); }
		bool            get (ENGSkeleton handle, const res::Skeleton **out, u64 timeout_msec = 0)        			{ return res__getOrScheduleLoadT(handle, out, timeout_msec); }
		bool            hotreload (ENGSkeleton handle)																{ return res__hotreload (handle.res_handle); }
		void 			internal__skeleton_on_afterCreate (void *res);
		void            internal__skeleton_on_destroy (void *res);
		void            internal__skeleton_on_unload (void *resIN);

		//============================= material
		bool            materialPBR_createFromAsset (const char *runtimeName, ENGMaterialPBR *out_handle, res::eLoadMode loadMode = res::eLoadMode::onDemand)		{ return res__getOrCreateHandleFromRuntimeName  (runtimeName, loadMode, &out_handle->res_handle); }
						//TODO: l'idea e' che dopo "create" devo poter ottenere un pt al materiale per poterlo modificare
		bool            materialPBR_create (ENGMaterialPBR *out_handle);
		void			release (ENGMaterialPBR &handle)															{ res__release(handle.res_handle); handle.res_handle.setInvalid(); }
		bool            get (ENGMaterialPBR handle, const res::MaterialPBR **out, u64 timeout_msec = 0)				{ return res__getOrScheduleLoadT(handle, out, timeout_msec); }
		void 			internal__materialPBR_on_afterCreate (void *res);
		void            internal__materialPBR_on_destroy (void *res);
		bool 			internal__materialPBR_update_renderer_binding (ENGMaterialPBR handle, u8 renderer_uid, u32 data);


		//============================= model3d
		bool            model_createFromAsset (const char *runtimeName, ENGModel3d *out_handle, res::eLoadMode loadMode = res::eLoadMode::onDemand)	{ return res__getOrCreateHandleFromRuntimeName  (runtimeName, loadMode, &out_handle->res_handle); }
		gos::Model*		model_create (ENGSkeleton handle_skeleton, u16 num_shape, u16 num_material, u16 num_meshes, ENGModel3d *out_handle);
		void            release (ENGModel3d &handle)																{ res__release(handle.res_handle); handle.res_handle.setInvalid(); }
		bool            get (ENGModel3d handle, const res::Model3d **out, u64 timeout_msec = 0)        				{ return res__getOrScheduleLoadT(handle, out, timeout_msec); }
		bool            hotreload (ENGModel3d handle)                                                               { return res__hotreload (handle.res_handle); }
		void 			internal__model_on_afterCreate (void *res);
		bool 			internal__model_on_loadCallback(void *callback_data);
		void            internal__model_on_destroy (void *res);
		void            internal__model_on_unload (void *resIN);
		
		//============================= model instance
		bool            modelinst_create (ENGModel3d handle_model, ENGModel3dInst *out_handle);
		void            release (ENGModel3dInst &handle)															{ res__release(handle.res_handle); handle.res_handle.setInvalid(); }
		bool            get (ENGModel3dInst handle, const res::Model3dInst **out, u64 timeout_msec = 0)				{ return res__getOrScheduleLoadT(handle, out, timeout_msec); }
		void            modelinst_applyTransform (ENGModel3dInst handle, const mat4x4f &matW);
		void 			modelinst_getWMatrix (ENGModel3dInst handle, mat4x4f *out_matW);
		void 			internal__modelinst_on_afterCreate (void *res);
		bool 			internal__modelinst_on_loadCallback(void *callback_data);
		void            internal__modelinst_on_destroy (void *res);
		void            internal__modelinst_on_unload (void *resIN);

	public:
						template<class RESOURCE>
		bool 			internal__getResFromSignatureUID (asset2::UID signatureUID, RESOURCE **out)
						{
							u32 handle_asU32;
							if (!listof_known_signatureUID.find (signatureUID, &handle_asU32))
								return false;
							
							res::Handle handle;
							handle.setFromU32(handle_asU32);
							(*out) = (RESOURCE*)res__getDescriptor (handle);
							return true;
						}

						template<class RESOURCE, class HANDLE>
		bool 			internal__getResFromSignatureUID (asset2::UID signatureUID, RESOURCE **out, HANDLE *out_handle)
						{
							assert (NULL != out_handle);
							u32 handle_asU32;
							if (!listof_known_signatureUID.find (signatureUID, &handle_asU32))
								return false;
							
							out_handle->res_handle.setFromU32(handle_asU32);
							(*out) = (RESOURCE*)res__getDescriptor (out_handle->res_handle);
							return true;
						}
						
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
		bool 			priv_GPUShape_create (const gos::Shape *shape, gpu::StageHelper &stageHelper, res::GPUShape *res);
		void			priv_modelinst_applyTransform_ric (const gos::Bone *model_listof_bones, gos::Bone *listof_bones, u32 boneIndex, const mat4x4f &parent_matW) const;

		void            priv_texture2D__add_to_mega_array (res::Texture2d *res, u32 desired_index=u32MAX);
		void            priv_texture2D__remove_from_mega_array (res::Texture2d *res);
		bool            priv_texture2D_create_ex (u16 dimx, u16 dimy, u8 nMipMap, eImageFormat fmt, eMemAccessMode memAccessMode, const void *srcDATA, ENGTexture2 *out_handle, gpu::StageHelper &stageHelper, u32 desired_texture_index);

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
		bool 			res__getOrScheduleLoad (res::Handle handle, const res::Descr **out, u64 timeout_msec = 0);
		bool 			res__scheduleLoadIfNeeded (res::Descr *res, u64 timeout_msec);
		
		bool 			res__signatureUID_to_resType (asset2::UID signatureUID, res::eType *out_res_type) const;
		res::HandleChain*	res__newHandleChain ();
		void 			res__freeHandleChain (res::HandleChain *p);
		void 			res__addChild (res::Descr *padre_res, res::Descr *child_res);

						template<class HANDLE, class RESOURCE>
		bool			res__getOrScheduleLoadT (HANDLE handle, const RESOURCE **out, u64 timeout_msec)
						{
							const res::Descr *res;
							const bool ret = res__getOrScheduleLoad(handle.res_handle, &res, timeout_msec);
							(*out) = reinterpret_cast<const RESOURCE*>(res);
							return ret;
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
		bool 			proxyRes__getT (const PROXYRES_HANDLE handle, RESOURCE **out_res, u64 timeout_msec)
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
		ENGTexture2		                            handle_texture_bianca;
		ProxyResInfo								proxyResInfo;
		
		



	//================= loader thread stuff ============
private:
		static constexpr u8         LOADER_THREAD__NUM_MAX_MESSAGES_TO_READ = 32;

		static constexpr u32		MSG_FOR_LOADER_THREAD__DIE		        = 0xff;
		static constexpr u32		MSG_FOR_LOADER_THREAD__LOAD	            = 0x01;
		static constexpr u32		MSG_FOR_LOADER_THREAD__LOAD_CONTINUE	= 0x02;

public:
		static constexpr u32		MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_OK 	= 0x01;
		static constexpr u32		MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_KO 	= 0x02;
		static constexpr u32		MSG_FROM_LOADER_THREAD__ON_LOAD_CALLBACK		= 0x03;

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
		
	
		thread::sMsg        loaderMsgList[LOADER_THREAD__NUM_MAX_MESSAGES_TO_READ];
		GOSThreadHandle 	hThreadLoader;
		HThreadMsgR     	msgq_1R;
		HThreadMsgW     	msgq_1W;
		HThreadMsgR     	msgq_2R;
		HThreadMsgW     	msgq_2W;           

	}; //class Engine
} //namespace gos


#endif //_gosEngine_h_

