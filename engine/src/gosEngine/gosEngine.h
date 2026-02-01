#ifndef _gosEngine_h_
#define _gosEngine_h_
#include "gosEngineEnumAndDefine.h"
#include "gosEngine_vtxBufferMan.h"
#include "gosEngine_idxBufferMan.h"
#include "gosEngine_scene.h"
#include "../gos/logger/gosLoggerStdout.h"
#include "../gosAsset2/gosAsset2.h"
#include "line3d/gosEngine_rend_line3d.h"



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
            const gos::input::MouseStatus *mouseStatus;
            const gos::input::sButtonModifier *btnModifier;
        };

    public:
        gos::GPU                *gpu;
        gos::input::Context     *inputCtx;

    public:
                            Engine();
                            ~Engine()                                   { unsetup(); }

        bool                setup (u32 mainWin_w, u32 mainWin_h, const char *mainWin_title);
        void                unsetup();

        bool                asset_rebuildAll();
        bool                asset_build();

            /* update:  ritorna false se la mainwin e' stata chiusa */
        bool                update();
        bool                inputEvent_getNext (InputEvent *out);
        input::eMouseMode   getMouseMode() const;
        void			    setMouseMode (input::eMouseMode mode);
        
        
        //=============================
        void            toggleFullscreen()                              { gpu->toggleFullscreen(); }
        void            toggleVSync();


        //=======================
        void            utils__quick_and_dirty__create_GPUSHape_and_stageIt_to_VB_IB (const gos::Shape *shape, ENGGPUShape *out_handle); 


        //============================= vtxBuffer
        bool            vtxBuffer_create (u32 sizeInByte, eMemAccessMode mode, ENGVtxBuffer *out_handle);
        void            release (ENGVtxBuffer &handle);
        bool            get (ENGVtxBuffer handle, const engine::ResVtxBuffer **out)                               { return handleList_vtxBuffer.queryInfo(handle, out); }

        //============================= idxBuffer
        bool            idxBuffer_create (u32 sizeInByte, eMemAccessMode mode, ENGIdxBuffer *out_handle);
        void            release (ENGIdxBuffer &handle);
        bool            get (ENGIdxBuffer handle, const engine::ResIdxBuffer **out)                               { return handleList_idxBuffer.queryInfo(handle, out); }

        //============================= vtxshader
        bool            vtxshader_createFromAsset (const char *uid_runtimeName, ENGVtxShader *out_handle, engine::eLoadMode loadMode = engine::eLoadMode::onDemand);
        bool            vtxshader_createFromFile (const char *filename, const char *mainFnName, ENGVtxShader *out_handle);
        bool            vtxshader_createFromMemory (const void *bufferIN, u32 bufferSize, const char *mainFnName, ENGVtxShader *out_handle);
        void            release (ENGVtxShader &handle)                                                            	{ priv_asset_release(handle, resHandler_vtxShader); }
        bool            get (ENGVtxShader handle, const engine::ResShader **out, u64 timeout_msec = 0)            	{ return priv_resource_get_and_schedule_load_if_needed (handle, resHandler_vtxShader, out, timeout_msec); }

        //============================= pxlshader
        bool            pxlshader_createFromAsset (const char *uid_runtimeName, ENGPxlShader *out_handle, engine::eLoadMode loadMode = engine::eLoadMode::onDemand);
        bool            pxlshader_createFromFile (const char *filename, const char *mainFnName, ENGPxlShader *out_handle);
        bool            pxlshader_createFromMemory (const void *bufferIN, u32 bufferSize, const char *mainFnName, ENGPxlShader *out_handle);
        void            release (ENGPxlShader &handle)                                                            	{ priv_asset_release(handle, resHandler_pxlShader); }
        bool            get (ENGPxlShader handle, const engine::ResShader **out, u64 timeout_msec = 0)            	{ return priv_resource_get_and_schedule_load_if_needed (handle, resHandler_pxlShader, out, timeout_msec); }

        //============================= shape
        bool            shape_createFromAsset (const char *uid_runtimeName, ENGShape *out_handle, engine::eLoadMode loadMode = engine::eLoadMode::onDemand);
        bool            shape_create (const VtxLayout &vtxLayout, u32 numVtx, u32 numIdx, ENGShape *out_handle);
        void            release (ENGShape &handle)																	{ priv_asset_release(handle, resHandler_shape); }
        bool            get (ENGShape handle, const engine::ResShape **out, u64 timeout_msec = 0)					{ return priv_resource_get_and_schedule_load_if_needed (handle, resHandler_shape, out, timeout_msec); }

		//============================= skeleton
        bool            skeleton_createFromAsset (const char *uid_runtimeName, ENGSkeleton *out_handle, engine::eLoadMode loadMode = engine::eLoadMode::onDemand);
        bool            skeleton_create (const u8 *buffer, u32 sizeof_buffer, ENGSkeleton *out_handle);
        void            release (ENGSkeleton &handle)																{ priv_asset_release(handle, resHandler_skeleton); }
        bool            get (ENGSkeleton handle, const engine::ResSkeleton **out, u64 timeout_msec = 0)        		{ return priv_resource_get_and_schedule_load_if_needed (handle, resHandler_skeleton, out, timeout_msec); }

		//============================= model3d
        bool            model_createFromAsset (const char *uid_runtimeName, ENGModel3d *out_handle, engine::eLoadMode loadMode = engine::eLoadMode::onDemand);
        bool            model_create (u16 num_shape, u16 num_material, u16 num_meshes, ENGModel3d *out_handle);
        void            release (ENGModel3d &handle)																{ priv_asset_release(handle, resHandler_model3d); }
        bool            get (ENGModel3d handle, const engine::ResModel3d **out, u64 timeout_msec = 0)        		{ return priv_resource_get_and_schedule_load_if_needed (handle, resHandler_model3d, out, timeout_msec); }
		
		//============================= model instance
        bool            modelinst_create (ENGModel3d handle_model, ENGModel3dInst *out_handle);
        void            release (ENGModel3dInst &handle)															{ priv_asset_release(handle, resHandler_model3dInst); }
		bool            get (ENGModel3dInst handle, const engine::ResModel3dInst **out, u64 timeout_msec = 0)		{ return priv_resource_get_and_schedule_load_if_needed (handle, resHandler_model3dInst, out, timeout_msec); }

        //============================= GPUShape
		/* 	Le GPUShape create hanno gia' gli handler VB/IB settati correttamente anche se i vtx/idx NON sono ancora stati copiati nei buffer (lo devi fare te).
			Le GPUShape create a partire da una ENGShape sono mappate internamente in modo che una successiva chiamata a GPUShape_create(ENGShape, ENGGPUShape) ritorni
			la ENGGPUShape senza ricrearla (se esisteva gia').
		*/
		bool            GPUShape_create (ENGShape handle_shape, ENGGPUShape *out_handle);
        bool            GPUShape_create (const gos::Shape *shape, ENGGPUShape *out_handle);
        void            release (ENGGPUShape &handle);
        bool            get (ENGGPUShape handle, const engine::ResGPUShape **out)                                  { return handleList_GPUShape.queryInfo(handle, out); }
		bool            get (ENGShape handle, const engine::ResGPUShape **out);
        
        //============================= texture2D
        bool            texture2D_createFromAsset (const char *uid_runtimeName, ENGTexture *out_handle, engine::eLoadMode loadMode = engine::eLoadMode::onDemand);
        bool            texture2D_create (u16 dimx, u16 dimy, u8 nMipMap, eImageFormat fmt, eMemAccessMode memAccessMode, const void *srcDATA, ENGTexture *out_handle, gpu::StageHelper &stageHelper);
        bool            texture2D_create (const gos::Image *im, u8 srcTextureNum, eMemAccessMode memAccessMode, ENGTexture *out_handle, gpu::StageHelper &stageHelper);
        void            release (ENGTexture &handle)                                                                { priv_asset_release(handle, resHandler_texture); }
        bool            get (ENGTexture handle, const engine::ResTexture **out, u64 timeout_msec = 0)               { return priv_resource_get_and_schedule_load_if_needed (handle, resHandler_texture, out, timeout_msec); }

        //============================= pipeline
        bool            pipeline_createFromAsset (const char *uid_runtimeName, ENGPipeline *out_handle, engine::eLoadMode loadMode = engine::eLoadMode::onDemand);
        void            release (ENGPipeline &handle)                                                              { priv_asset_release(handle, resHandler_pipeline); }
        bool            get (ENGPipeline handle, const engine::ResPipeline **out, u64 timeout_msec = 0)            { return priv_resource_get_and_schedule_load_if_needed (handle, resHandler_pipeline, out, timeout_msec); }




    private:
        typedef FastHashMap<asset2::UID, u32> HashListOfLoadedUID;


    private:
        //====================================================
        template<class HANDLE_TYPE, class HANDLE_STRUCT>
        class HList
        {
        public:
            void setup (gos::Allocator *allocator)          { list.setup(allocator); thread::mutexCreate (&mutex); }
            void unsetup()                                  { thread::mutexDestroy(mutex); list.unsetup(); }

            void lock()                                     { thread::mutexLock(mutex); }
            void unlock()                                   { thread::mutexUnlock(mutex); }

            HANDLE_STRUCT*  reserveTS (HANDLE_TYPE *out)
            { 
                lock();
                HANDLE_STRUCT *s = list.reserve(out);
                if (NULL != s)
                {
                    s->reset();
                    s->brh.refCount = 1;
                }
                unlock();
                return s;
            }

            bool            releaseTS (HANDLE_TYPE &handle, HANDLE_STRUCT *out)
            { 
                bool ret = false;
                lock();     
                
                HANDLE_STRUCT *pt = NULL;
                list.fromHandleToPointer (handle, &pt);
                if (NULL != pt)
                {
                    pt->brh.refCount--;
                    if (pt->brh.refCount <= 0)
                    {
                        ret = true;
                        memcpy (out, pt, sizeof(HANDLE_STRUCT));
                        list.release(handle);
                    }
                }
                unlock();
                return ret;
            }

            i32             incRefCount (HANDLE_TYPE handle)
            { 
                lock();     
                HANDLE_STRUCT *pt = NULL;
                list.fromHandleToPointer (handle, &pt);
                if (NULL != pt)
                    pt->brh.refCount++;
                unlock();
                assert (NULL != pt);
                return pt->brh.refCount;
            }

            bool		    fromHandleToPointer (HANDLE_TYPE h, HANDLE_STRUCT* *out) const          { return list.fromHandleToPointer (h, out); }
            const HANDLE_STRUCT* getInfo (HANDLE_TYPE handle) const                                 { HANDLE_STRUCT *ret = NULL; list.fromHandleToPointer (handle, &ret); return ret; }
            bool            queryInfo (HANDLE_TYPE h, const HANDLE_STRUCT* *out) const              { *out = getInfo(h); return (NULL != out); }
            
            

        private:
            gos::Mutex  mutex;
            HandleList<HANDLE_TYPE, HANDLE_STRUCT> list;
        };


        //====================================================
        class BaseResourceHandler
        {
        public:
                            BaseResourceHandler()   { }
            virtual         ~BaseResourceHandler()  { }

            virtual bool    handle_get_or_create_from_asset (Engine *engine, asset2::UID uid, engine::eLoadMode loadMode, u32 *out_handleAsU32) = 0;
            virtual void    resource_schedule_load (Engine *engine, u32 handle_asU32) = 0;
            virtual void    resource_release (Engine *engine, u32 handle_asU32) = 0;
        };

        //====================================================
        template<class HANDLE_TYPE, class HANDLE_STRUCT>
        class ResouceHandler : public BaseResourceHandler
        {
        public:
                            ResouceHandler ()           { }
            virtual         ~ResouceHandler ()          { }


            void            setup (gos::Allocator *allocator, eAssetType assetTypeIN)               { handle_list.setup(allocator); assetType=assetTypeIN; }
            void            unsetup()                                                               { handle_list.unsetup(); }
            
            HANDLE_STRUCT*  reserveTS (HANDLE_TYPE *out)                                            { return handle_list.reserveTS(out); }
            bool            releaseTS (HANDLE_TYPE &handle, HANDLE_STRUCT *out)                     { return handle_list.releaseTS(handle, out); }
            void            incRefCount (HANDLE_TYPE handle)                                        { handle_list.incRefCount(); }
            bool		    fromHandleToPointer (HANDLE_TYPE h, HANDLE_STRUCT* *out) const          { return handle_list.fromHandleToPointer (h, out); }

            eAssetType      getAssetType() const                                                    { return assetType; }
            const HANDLE_STRUCT* getInfo (HANDLE_TYPE handle) const                                 { HANDLE_STRUCT *ret = NULL; handle_list.fromHandleToPointer (handle, &ret); return ret; }

            bool            handle_get_or_create_from_asset (Engine *engine, asset2::UID uid, engine::eLoadMode loadMode, u32 *out_handleAsU32)
                            {
                                HANDLE_TYPE handle;
                                if (!engine->priv_resource_get_or_create_handle_from_asset (uid, *this, &handle))
                                {
                                    logger::err ("Engine::resource_createFromAsset => unable to match UID and handle\n");
                                    return false;
                                }

                                if (engine::eLoadMode::asap == loadMode)
                                {
                                    const HANDLE_STRUCT *res;
                                    engine->priv_resource_get_and_schedule_load_if_needed (handle, *this, &res, 0);
                                }

                                *out_handleAsU32 = handle.viewAsU32();
                                return true;
                            }
            void            resource_schedule_load (Engine *engine, u32 handle_asU32)
            {
                HANDLE_TYPE handle;
                handle.setFromU32 (handle_asU32);

                HANDLE_STRUCT *res;
                if (!handle_list.fromHandleToPointer (handle, &res))
                {
                    DBGBREAK;
                    return;
                }
                assert (res->brh.uid.isAnAsset());

                //schedulo un load solo se serve
                if (engine::eResStatus::notLoaded != res->brh.status)
                    return;
                res->brh.status = engine::eResStatus::loading;

                engine->priv_send_load_msg_to_LoaderThread (res);
            }
            void            resource_release (Engine *engine, u32 handle_asU32)
            {
                HANDLE_TYPE handle;
                handle.setFromU32 (handle_asU32);
                engine->priv_asset_release (handle, *this);
            }

        public:
            HList<HANDLE_TYPE,HANDLE_STRUCT>    handle_list;

        private:
            eAssetType      assetType;
        };



    public:
                       /***
                         * @brief   internal__from_asset_to_handle
                         *          Se <uid> e' stato gia' associato ad un handle di una risorsa (tramite una delle
                         *          xx_createFromAsset()), allora ritorna true e mette in <out__handleID> l'u32
						 * 			che rappresenta l'handle a cui UID e' associato
                         */	
        bool            internal__from_asset_to_handle (asset2::UID uid, u32 *out__handleID) const					{ return listof_knownUID.find (uid, out__handleID); }

                       /***
                         * @brief   internal__get_raw_data
                         *          ritorna false se <handle> e' invalido
                         *          ritorna true altrimenti e <out> punta alla risorsa
                         * 
                         *          Per ogni <HANDLE_TYPE> e' necessario aggiungere un "if constexper" perche'
                         *          bisogna indicare quale resHandler utilizzare per recuperare la risorsa
                         */
                        template<class HANDLE_TYPE, class RESOURCE_TYPE>
        bool            internal__get_raw_data (HANDLE_TYPE handle, RESOURCE_TYPE **out_resPt)
                        {
                            if constexpr (std::is_same<HANDLE_TYPE, ENGVtxShader>::value)
								return resHandler_vtxShader.fromHandleToPointer(handle, out_resPt);
                            else if constexpr (std::is_same<HANDLE_TYPE, ENGPxlShader>::value)
								return resHandler_pxlShader.fromHandleToPointer(handle, out_resPt);
                            else if constexpr (std::is_same<HANDLE_TYPE, ENGShape>::value)
								return resHandler_shape.fromHandleToPointer(handle, out_resPt);
                            else if constexpr (std::is_same<HANDLE_TYPE, ENGSkeleton>::value)
								return resHandler_skeleton.fromHandleToPointer(handle, out_resPt);
                            else
                            {
                                DBGBREAK;
                                return false;
                            }
                        }

                       /***
                         * @brief   internal__from_asset_to_raw_data
                         *          Combina <internal__from_asset_to_handle()> e <internal__get_raw_data()>
						 * 			per ritornare l'handle e il pt ai dati della risorsa a partire da un assetUID
                         */
                        template<class HANDLE_TYPE, class RESOURCE_TYPE>
        bool            internal__from_asset_to_raw_data (asset2::UID uid, HANDLE_TYPE *out_handle, RESOURCE_TYPE **out_resPt)
                        {
							u32 handle_as_u32;
							if (!internal__from_asset_to_handle(uid, &handle_as_u32))
								return false;

							out_handle->setFromU32(handle_as_u32);
							return internal__get_raw_data (*out_handle, out_resPt);
                        }
			


    private:
                        		template<class HANDLE_TYPE, class HANDLE_STRUCT>
        void            		priv_setup_resource_handler (eAssetType assetType, ResouceHandler<HANDLE_TYPE, HANDLE_STRUCT> *res_handler)
                        {
                            res_handler->setup (allocator, assetType);

                            const u32 index = (u32)assetType;
                            assert (index <= (u32)eAssetType::__NUM);
                            resHandler_list[index] = res_handler;
                        }
        void            		priv_flushLoaderThreadMsg();
        bool            		asset_bind (asset2::UID uid, u32 handle_asU32);
		engine::ResGPUShape* 	priv_GPUShape_create (const gos::Shape *shape, ENGGPUShape *out_handle);

        BaseResourceHandler*    priv_get_baseResourceHandler (eAssetType assetType) const
                        { 
                            const u32 index = (u32)assetType;
                            assert (index <= (u32)eAssetType::__NUM);
                            assert (NULL != resHandler_list[index]);
                            return resHandler_list[index];
                        }

    private:
                        /***
                         * @brief   priv_resource_get_and_schedule_load_if_needed
                         *          ritorna true se <out> e' gia' in stato eResStatus::ready
                         *          ritorna false altrimenti (e ne schedula il load se possibile)
                         *          <out> viene fillata in ogni caso, e vale NULL solo se l'<handle> e' invalido
                         */
                        template<class HANDLE_TYPE, class HANDLE_STRUCT>
        bool            priv_resource_get_and_schedule_load_if_needed (HANDLE_TYPE handle, ResouceHandler<HANDLE_TYPE,HANDLE_STRUCT> &res_handler, const HANDLE_STRUCT **out, u64 timeout_msec) 
                        {
                            *out = res_handler.getInfo(handle);
                            if (NULL == (*out) ) return false;
                            if (engine::eResStatus::ready == (*out)->brh.status) return true;
                            if (engine::eResStatus::notLoaded == (*out)->brh.status)
                            {
                                //dato che l'asset e' notLoaded, schedulo il suo load (e degli asset da cui dipende)
                                asset2::UID uid;
                                uid = (*out)->brh.uid;
                                asset_logger->log (eTextColor::darkGreen, "asset::  [%s] %016" PRIX64 " load requested\n", asset2::enumToString(uid.getAssetType()), uid._uid);
                                asset_logger->incIndent();
                                {
                                    //gestisco le dipendenze di questo asset. Se lui dipende da altri, deve loadare pure quelli, prima di me stesso
                                    u8 memblock[1024];
                                    asset2::FastUIDList fastUIDList;
                                    fastUIDList.setupWithBase (memblock, sizeof(memblock), gos::getScrapAllocator());

                                    asset2::asset_get_runtime_dependecies_list (asset_ctx, uid, false, &fastUIDList);
                                    for (u32 i=0; i<fastUIDList.getNElem(); i++)
                                    {
                                        const asset2::UID uid_child = fastUIDList(i);
                                        u32 handleAsU32;
                                        if (internal__from_asset_to_handle (uid_child, &handleAsU32))
                                        {
                                            BaseResourceHandler *base_resHandler = priv_get_baseResourceHandler(uid_child.getAssetType());
                                            base_resHandler->resource_schedule_load (this, handleAsU32);
                                        }
                                        else
                                        {
                                            //se arriviamo qui, vuol dire che un asset da cui dipendo non esiste il che non 
                                            //dovrebbe essere possibile perche' durante asset_bind() di me stesso, ho certamente creato
                                            //anche quelli che dipendono da me
                                            DBGBREAK;
                                        }
                                    }
                                }
                                asset_logger->decIndent();                                 
                                
                                //schedolo il load di me stesso
                                res_handler.resource_schedule_load (this, handle.viewAsU32());

                                if (0 == timeout_msec)
                                    return false;
                            }


                            //se richiesto, attendo per un po' nella speranza di vedere l'asset "ready"
                            if (timeout_msec > 0 && engine::eResStatus::error != (*out)->brh.status)
                            {
                                u64 time_to_exit_msec = gos::getTimeSinceStart_msec() + timeout_msec;
                                while (gos::getTimeSinceStart_msec() < time_to_exit_msec)
                                {
                                    priv_flushLoaderThreadMsg();
                                    if (engine::eResStatus::ready == (*out)->brh.status) return true;
                                    if (engine::eResStatus::error == (*out)->brh.status) return false;
                                }
                            }
                            return false;
                        }

                        /***
                         * @brief   priv_resource_get_or_create_handle_from_asset
                         *          Crea una associazione tra <uid> e <handle> e filla <out_handle> con le opportune informazione
                         *          Se la <uid> e' un asset gia noto, allora recupera il relativo <handle> e ne incrementa il recCount
                         */
                        template<class HANDLE_TYPE, class HANDLE_STRUCT>
        bool            priv_resource_get_or_create_handle_from_asset (asset2::UID uid, ResouceHandler<HANDLE_TYPE,HANDLE_STRUCT> &res_handler, HANDLE_TYPE *out_handle)
                        {
                            //vediamo se UID e' gia stato associato ad un handle
                            u32 handleAsU32;
                            if (internal__from_asset_to_handle (uid, &handleAsU32))
                            {
                                out_handle->setFromU32(handleAsU32);
                                const i32 refcount = res_handler.handle_list.incRefCount (*out_handle);
                                asset_logger->log (eTextColor::darkGreen, "asset::  [%s] %016" PRIX64 " refcount=%d\n", asset2::enumToString(uid.getAssetType()), uid._uid, refcount);
                                return true;
                            }

                            //pare di no, devo creare un nuovo handle...
                            HANDLE_STRUCT *res = res_handler.handle_list.reserveTS(out_handle);
                            if (NULL == res)
                            {
                                logger::err ("Engine::resource_createFromAsset() => can't create handle\n");
                                return false;
                            }

                            //..bindarlo all'asset uid
                            res->brh.uid = uid;
                            res->brh.status = engine::eResStatus::notLoaded;
                            asset_bind (uid, out_handle->viewAsU32());
                            return true;
                        }                        


                        /***
                         * @brief   priv_asset_release
                         *          Decrementa il refCount di handle e, se arriva a 0, fa il free della risorsa
                         */
                        template<class HANDLE_TYPE, class HANDLE_STRUCT>
        void            priv_asset_release (HANDLE_TYPE &handle, ResouceHandler<HANDLE_TYPE,HANDLE_STRUCT> &res_handler)
                        {
                            HANDLE_STRUCT res;
                            if (res_handler.releaseTS (handle, &res))
                            {
                                //il ref-count e' a zero, devo distruggere la risorsa

                                //distruggo me stesso
                                res.data.destroy(allocator, gpu);

                                //se la risorsa e' stata caricata tramite un asset, devo rilasciare gli eventuali asset dipendenti                                   
                                if (res.brh.uid.isValid())
                                {
                                    asset_logger->log (eTextColor::darkGreen, "asset::  [%s] %016" PRIX64 " released\n", asset2::enumToString(res.brh.uid.getAssetType()), res.brh.uid._uid);

                                    //unbindo l'asset
                                    listof_knownUID.remove(res.brh.uid);

                                    //gestisco le dipendenze di questo asset. Se lui dipende da altri, deve rilasciare anche gli altri
                                    u8 memblock[1024];
                                    asset2::FastUIDList fastUIDList;
                                    fastUIDList.setupWithBase (memblock, sizeof(memblock), gos::getScrapAllocator());

                                    asset_logger->incIndent();
                                    asset2::asset_get_runtime_dependecies_list (asset_ctx, res.brh.uid, false, &fastUIDList);
                                    for (u32 i=0; i<fastUIDList.getNElem(); i++)
                                    {
                                        const asset2::UID uid_child = fastUIDList(i);
                                        u32 handleAsU32;
                                        if (internal__from_asset_to_handle (uid_child, &handleAsU32))
                                        {
                                            BaseResourceHandler *base_resHandler = priv_get_baseResourceHandler(uid_child.getAssetType());
                                            base_resHandler->resource_release (this, handleAsU32);
                                        }
                                    }
                                    asset_logger->decIndent();      
                                }
                            }
                            
                            handle.setInvalid();
                        }

        void            priv_send_load_msg_to_LoaderThread (void *res)
                        {
							engine::BaseResHandle *brh = (engine::BaseResHandle*)res;
                            asset_logger->log (eTextColor::darkGreen, "asset::  [%s] %016" PRIX64 " loading\n", asset2::enumToString(brh->uid.getAssetType()), brh->uid._uid);
                            thread::pushMsg (msgq_1W, MSG_FOR_LOADER_THREAD__LOAD, 0, res);
                        }



    private:
        gos::Allocator                              *allocator;
        bool                                        bQuitEngine;
        input::ResolvedEvtList                      evtList;
        engine::VtxBufferMan                        vtxBufferMan;
        engine::IdxBufferMan                        idxBufferMan;

        gos::Logger                                 *asset_logger;
        asset2::DBContext                           asset_ctx;
        HashListOfLoadedUID			                listof_knownUID;	//mappa asset2::uid ad u32 che e' l'handle della risorsa nell'engine

        BaseResourceHandler                                 *resHandler_list[(u32)eAssetType::__NUM + 1];
        HList<ENGVtxBuffer, engine::ResVtxBuffer>           handleList_vtxBuffer;
        HList<ENGIdxBuffer, engine::ResIdxBuffer>           handleList_idxBuffer;
        HList<ENGGPUShape, engine::ResGPUShape>             handleList_GPUShape;
		FastHashMap<ENGShape, ENGGPUShape>					map_of_shape_to_gpushape;
        ResouceHandler<ENGShape, engine::ResShape>          resHandler_shape;
        ResouceHandler<ENGTexture, engine::ResTexture>      resHandler_texture;
        ResouceHandler<ENGPipeline, engine::ResPipeline>    resHandler_pipeline;
        ResouceHandler<ENGVtxShader, engine::ResShader>     resHandler_vtxShader;
        ResouceHandler<ENGPxlShader, engine::ResShader>     resHandler_pxlShader;
		ResouceHandler<ENGSkeleton, engine::ResSkeleton>	resHandler_skeleton;
		ResouceHandler<ENGModel3d, engine::ResModel3d>		resHandler_model3d;
		ResouceHandler<ENGModel3dInst, engine::ResModel3dInst> resHandler_model3dInst;



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

