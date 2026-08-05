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

        bool                setup_renderPipe();

            /* update:  ritorna false se la mainwin e' stata chiusa */
        bool                            update();
        bool                            inputEvent_getNext (InputEvent *out);
        const input::MouseStatus*       inputEvent_getMouseStatus() const;
        const input::sButtonModifier*   inputEvent_getBtnModifier() const;

        input::eMouseMode   getMouseMode() const;
        void			    setMouseMode (input::eMouseMode mode);
        
        //=============================
        void            toggleFullscreen()                              { gpu->toggleFullscreen(); }
        void            toggleVSync();


        //============================= vtxBuffer
        bool            vtxBuffer_create (u32 sizeInByte, eMemAccessMode mode, ENGVtxBuffer *out_handle);
        void            release (ENGVtxBuffer &handle)																{ res_release(handle.res_handle); handle.res_handle.setInvalid(); }
        bool            get (ENGVtxBuffer handle, const res::VtxBuffer **out)										{ return res_getOrScheduleLoadT(handle, out, 0); }
		void            internal__vtxBuffer_on_afterCreate (void *res);
		void            internal__vtxBuffer_on_destroy (void *res);

        //============================= idxBuffer
        bool            idxBuffer_create (u32 sizeInByte, eMemAccessMode mode, ENGIdxBuffer *out_handle);
        void            release (ENGIdxBuffer &handle)																{ res_release(handle.res_handle); handle.res_handle.setInvalid(); }
        bool            get (ENGIdxBuffer handle, const res::IdxBuffer **out)										{ return res_getOrScheduleLoadT(handle, out, 0); }
		void 			internal__idxBuffer_on_afterCreate (void *res);
		void            internal__idxBuffer_on_destroy (void *res);

        //============================= vtxshader
        bool            vtxshader_createFromAsset (const char *uid_runtimeName, ENGVtxShader *out_handle, res::eLoadMode loadMode = res::eLoadMode::onDemand)	{ return res_createFromAssetT (uid_runtimeName, out_handle, loadMode); }
        bool            vtxshader_createFromFile (const char *filename, const char *mainFnName, ENGVtxShader *out_handle);
        bool            vtxshader_createFromMemory (const void *bufferIN, u32 bufferSize, const char *mainFnName, ENGVtxShader *out_handle);
        void            release (ENGVtxShader &handle)                                                            	{ res_release(handle.res_handle); handle.res_handle.setInvalid(); }
        bool            get (ENGVtxShader handle, const res::Shader **out, u64 timeout_msec = 0)            		{ return res_getOrScheduleLoadT(handle, out, timeout_msec); }
		void 			internal__vtxshader_on_afterCreate (void *res);
		void            internal__vtxshader_on_destroy (void *res);

        //============================= pxlshader
        bool            pxlshader_createFromAsset (const char *uid_runtimeName, ENGPxlShader *out_handle, res::eLoadMode loadMode = res::eLoadMode::onDemand)	{ return res_createFromAssetT (uid_runtimeName, out_handle, loadMode); }
        bool            pxlshader_createFromFile (const char *filename, const char *mainFnName, ENGPxlShader *out_handle);
        bool            pxlshader_createFromMemory (const void *bufferIN, u32 bufferSize, const char *mainFnName, ENGPxlShader *out_handle);
        void            release (ENGPxlShader &handle)                                                            	{ res_release(handle.res_handle); handle.res_handle.setInvalid(); }
        bool            get (ENGPxlShader handle, const res::Shader **out, u64 timeout_msec = 0)            		{ return res_getOrScheduleLoadT(handle, out, timeout_msec); }
		void 			internal__pxlshader_on_afterCreate (void *res);
		void            internal__pxlshader_on_destroy (void *res);

        //============================= pipeline
        bool            pipeline_createFromAsset (const char *uid_runtimeName, ENGPipeline *out_handle, res::eLoadMode loadMode = res::eLoadMode::onDemand)	{ return res_createFromAssetT (uid_runtimeName, out_handle, loadMode); }
        bool            pipeline_create (const gpu::Pipeline_def &rpd, ENGPipeline *out_handle);
        void            release (ENGPipeline &handle)                                                              	{ res_release(handle.res_handle); handle.res_handle.setInvalid(); }
        bool            get (ENGPipeline handle, const res::Pipeline **out, u64 timeout_msec = 0)            		{ return res_getOrScheduleLoadT(handle, out, timeout_msec); }
		void 			internal__pipeline_on_afterCreate (void *res);
		void            internal__pipeline_on_destroy (void *res);

        //============================= texture2D
        bool            texture2D_createFromAsset (const char *uid_runtimeName, ENGTexture *out_handle, res::eLoadMode loadMode = res::eLoadMode::onDemand)	{ return res_createFromAssetT (uid_runtimeName, out_handle, loadMode); }
        bool            texture2D_create (u16 dimx, u16 dimy, u8 nMipMap, eImageFormat fmt, eMemAccessMode memAccessMode, const void *srcDATA, ENGTexture *out_handle, gpu::StageHelper &stageHelper);
        bool            texture2D_create (const gos::Image *im, u8 srcTextureNum, eMemAccessMode memAccessMode, ENGTexture *out_handle, gpu::StageHelper &stageHelper);
        void            release (ENGTexture &handle)                                                                { res_release(handle.res_handle); handle.res_handle.setInvalid(); }
        bool            get (ENGTexture handle, const res::Texture2d **out, u64 timeout_msec = 0)               	{ return res_getOrScheduleLoadT(handle, out, timeout_msec); }
        bool            reload (ENGTexture handle)                                                                  { return res_reload (handle.res_handle); }
		void 			internal__texture2D_on_afterCreate (void *res);
		void            internal__texture2D_on_destroy (void *res);
        void            internal__texture2D_on_afterLoad(void *res);
        void            internal__texture2D_on_unload (void *resIN);

		bool            get_texture_bianca (const res::Texture2d **out)               								{ return res_getOrScheduleLoadT(handle_texture_bianca, out, 0); }

        //============================= shape
        bool            shape_createFromAsset (const char *uid_runtimeName, ENGShape *out_handle, res::eLoadMode loadMode = res::eLoadMode::onDemand)		{ return res_createFromAssetT (uid_runtimeName, out_handle, loadMode); }
        bool            shape_create (const VtxLayout &vtxLayout, u32 numVtx, u32 numIdx, ENGShape *out_handle);
        void			release (ENGShape &handle)																	{ res_release(handle.res_handle); handle.res_handle.setInvalid(); }
        bool            get (ENGShape handle, const res::Shape **out, u64 timeout_msec = 0)							{ return res_getOrScheduleLoadT(handle, out, timeout_msec); }
		void 			internal__shape_on_afterCreate (void *res);
		void            internal__shape_on_destroy (void *res);

        //============================= GPUShape
		bool            GPUShape_create (ENGShape handle_shape, gpu::StageHelper &stageHelper, ENGGPUShape *out_handle);
        bool            GPUShape_create (const gos::Shape *shape, gpu::StageHelper &stageHelper, ENGGPUShape *out_handle);
        void            release (ENGGPUShape &handle)																{ res_release(handle.res_handle); handle.res_handle.setInvalid(); }
        bool            get (ENGGPUShape handle, const res::GPUShape **out)                                  		{ return res_getOrScheduleLoadT(handle, out, 0); }
		bool            get (ENGShape handle, const res::GPUShape **out);
		void 			internal__GPUShape_on_afterCreate (void *res);
		void            internal__GPUShape_on_destroy (void *res);
        

		//============================= skeleton
        bool            skeleton_createFromAsset (const char *uid_runtimeName, ENGSkeleton *out_handle, res::eLoadMode loadMode = res::eLoadMode::onDemand)	{ return res_createFromAssetT (uid_runtimeName, out_handle, loadMode); }
        bool            skeleton_createFromMemory (const u8 *buffer, u32 sizeof_buffer, ENGSkeleton *out_handle);
		bool            skeleton_create (const Skeleton &sk, ENGSkeleton *out_handle);
        void            release (ENGSkeleton &handle)																{ res_release(handle.res_handle); handle.res_handle.setInvalid(); }
        bool            get (ENGSkeleton handle, const res::Skeleton **out, u64 timeout_msec = 0)        			{ return res_getOrScheduleLoadT(handle, out, timeout_msec); }
		void 			internal__skeleton_on_afterCreate (void *res);
		void            internal__skeleton_on_destroy (void *res);

        //============================= material
        bool            materialPBR_createFromAsset (const char *uid_runtimeName, ENGMaterialPBR *out_handle, res::eLoadMode loadMode = res::eLoadMode::onDemand)		{ return res_createFromAssetT (uid_runtimeName, out_handle, loadMode); }
						//TODO: l'idea e' che dopo "create" devo poter ottenere un pt al materiale per poterlo modificare
		bool            materialPBR_create (ENGMaterialPBR *out_handle);
        void			release (ENGMaterialPBR &handle)															{ res_release(handle.res_handle); handle.res_handle.setInvalid(); }
        bool            get (ENGMaterialPBR handle, const res::MaterialPBR **out, u64 timeout_msec = 0)				{ return res_getOrScheduleLoadT(handle, out, timeout_msec); }
		void 			internal__materialPBR_on_afterCreate (void *res);
		void            internal__materialPBR_on_destroy (void *res);
        bool 			internal__materialPBR_update_renderer_binding (ENGMaterialPBR handle, u8 renderer_uid, u32 data);


		//============================= model3d
        bool            model_createFromAsset (const char *uid_runtimeName, ENGModel3d *out_handle, res::eLoadMode loadMode = res::eLoadMode::onDemand)	{ return res_createFromAssetT (uid_runtimeName, out_handle, loadMode); }
        gos::Model*		model_create (ENGSkeleton handle_skeleton, u16 num_shape, u16 num_material, u16 num_meshes, ENGModel3d *out_handle);
        void            release (ENGModel3d &handle)																{ res_release(handle.res_handle); handle.res_handle.setInvalid(); }
        bool            get (ENGModel3d handle, const res::Model3d **out, u64 timeout_msec = 0)        				{ return res_getOrScheduleLoadT(handle, out, timeout_msec); }
		void 			internal__model_on_afterCreate (void *res);
		void            internal__model_on_destroy (void *res);
		
		//============================= model instance
        bool            modelinst_create (ENGModel3d handle_model, ENGModel3dInst *out_handle);
        void            release (ENGModel3dInst &handle)															{ res_release(handle.res_handle); handle.res_handle.setInvalid(); }
        bool            get (ENGModel3dInst handle, const res::Model3dInst **out)                           		{ return res_getOrScheduleLoadT(handle, out, 0); }
		void            modelinst_applyTransform (ENGModel3dInst handle, const mat4x4f &matW);
		void 			internal__modelinst_on_afterCreate (void *res);
		void            internal__modelinst_on_destroy (void *res);


    private:
        typedef FastHashMap<asset2::UID, u32> HashListOfLoadedUID;

	private:
		void 			priv_flushLoaderThreadMsg();
        void 			priv_reload_resource();
		bool 			priv_GPUShape_create (const gos::Shape *shape, gpu::StageHelper &stageHelper, res::GPUShape *res);
		void			priv_modelinst_applyTransform_ric (const gos::Bone *model_listof_bones, gos::Bone *listof_bones, u32 boneIndex, const mat4x4f &parent_matW) const;

        void            priv_texture2D__add_to_mega_array (res::Texture2d *res, u32 desired_index=u32MAX);
        void            priv_texture2D__remove_from_mega_array (res::Texture2d *res);
        bool            priv_texture2D_create_ex (u16 dimx, u16 dimy, u8 nMipMap, eImageFormat fmt, eMemAccessMode memAccessMode, const void *srcDATA, ENGTexture *out_handle, gpu::StageHelper &stageHelper, u32 desired_texture_index);

		void 			res_printInfo (const void *res) const;
		res::Descr*		res_createHandle (res::eType res_type, res::Handle *out_handle);
        res::Descr*		res_getOrCreateHandleFromAsset (const char *uid_runtimeName, res::Handle *out_handle, bool *out_bWasNew);
		res::Descr*		res_getOrCreateHandleFromAsset (asset2::UID uid, res::Handle *out_handle, bool *out_bWasNew);
		void 			res_bindEvents (res::Handle handle, res::Descr *res);
        res::Descr*		res_getDescriptor (res::Handle handle);
        void            res_release (res::Handle handle);
		void            res_release (res::Descr *res);
        bool            res_reload (res::Handle handle);
        void            res_do_destroy (res::Descr *res);
		bool 			res_getOrScheduleLoad (res::Handle handle, const res::Descr **out, u64 timeout_msec = 0);
		
		bool 			res_assetUID_to_resUID (asset2::UID uid, res::eType *out_res_type) const;
		res::HandleChain*	res_newHandleChain ();
		void 			res_freeHandleChain (res::HandleChain *p);
		void 			res_addChild (res::Descr *padre_res, res::Descr *child_res);

						template<class HANDLE, class RESOURCE>
		bool			res_getOrScheduleLoadT (HANDLE handle, const RESOURCE **out, u64 timeout_msec)
						{
							const res::Descr *res;
							const bool ret = res_getOrScheduleLoad(handle.res_handle, &res, timeout_msec);
							(*out) = reinterpret_cast<const RESOURCE*>(res);
							return ret;
						}

						template<class HANDLE>
		bool 			res_createFromAssetT (const char *uid_runtimeName, HANDLE *out_handle, res::eLoadMode loadMode)
						{
							bool bWasNew;
							res::Descr *res = res_getOrCreateHandleFromAsset (uid_runtimeName, &out_handle->res_handle, &bWasNew);
							if (NULL == res)
								return false;
							
							//schedula load se richiesto
							if (res::eLoadMode::asap == loadMode)
							{
								const res::Descr *descr;
								res_getOrScheduleLoad (out_handle->res_handle, &descr);
							}
							return true;
						}

	public:
						template<class RESOURCE>
		bool 			internal__getResFromUID (asset2::UID uid, RESOURCE **out)
						{
							u32 handle_asU32;
							if (!listof_knownUID.find (uid, &handle_asU32))
								return false;
							
							res::Handle handle;
							handle.setFromU32(handle_asU32);
							(*out) = (RESOURCE*)res_getDescriptor (handle);
							return true;
						}

						template<class RESOURCE, class HANDLE>
		bool 			internal__getResFromUID (asset2::UID uid, RESOURCE **out, HANDLE *out_handle)
						{
							assert (NULL != out_handle);
							u32 handle_asU32;
							if (!listof_knownUID.find (uid, &handle_asU32))
								return false;
							
							out_handle->res_handle.setFromU32(handle_asU32);
							(*out) = (RESOURCE*)res_getDescriptor (out_handle->res_handle);
							return true;
						}
						
						template<class RESOURCE_PADRE, class RESOURCE_FIGLIO>
		void 			internal__resAddChild (RESOURCE_PADRE *padre, RESOURCE_FIGLIO *figlio)
						{
							res_addChild (&padre->_descr, &figlio->_descr);
						}

						template<class RESOURCE_PADRE, class HANDLE_FIGLIO>
		void 			internal__resAddChild (RESOURCE_PADRE *padre, HANDLE_FIGLIO handle_figlio)
						{
							res::Descr *figlio = res_getDescriptor(handle_figlio.res_handle);
							assert (NULL != figlio);
							res_addChild (&padre->_descr, figlio);
						}						


    private:
        struct sUnloadInfo
        {
            res::Handle res_handle;
            u32         timer_msec;
        };

    private:
        gos::Allocator                              *allocator;
        bool                                        bQuitEngine;
        input::ResolvedEvtList                      evtList;
        engine::VtxBufferMan                        vtxBufferMan;
        engine::IdxBufferMan                        idxBufferMan;

        gos::Logger                                 *asset_logger;
        asset2::DBContext                           asset_ctx;
        HashListOfLoadedUID			                listof_knownUID;	//mappa asset2::uid ad u32 che e' l'handle della risorsa nell'engine

		res::Manager 								resManager;
		FastHashMap<ENGShape, ENGGPUShape>			map_of_shape_to_gpushape;
		gos::ObjectPool<res::HandleChain>			resHandleChainPool;
        gos::FastArray<sUnloadInfo>                 list_of_res_to_be_reloaded;
        ENGTexture		                            handle_texture_bianca;

		
		



    //================= loader thread stuff ============
private:
        static constexpr u8         LOADER_THREAD__NUM_MAX_MESSAGES_TO_READ = 32;

        static constexpr u32		MSG_FOR_LOADER_THREAD__DIE		        = 0xff;
        static constexpr u32		MSG_FOR_LOADER_THREAD__LOAD	            = 0x01;

        static constexpr u32		MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_OK 	= 0x01;
        static constexpr u32		MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_KO 	= 0x02;

        struct sLoaderThreadInitParams
        {
            gos::Event		    hEvent_started;
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

