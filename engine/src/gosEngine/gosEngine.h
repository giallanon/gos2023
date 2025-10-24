#ifndef _gosEngine_h_
#define _gosEngine_h_
#include "gosEngineEnumAndDefine.h"
#include "gosEngine_vtxBufferMan.h"
#include "gosEngine_idxBufferMan.h"
#include "gosEngine_fixedSizeBufferTracker.h"

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
        gos::asset::Hub         *assetHub;

    public:
                                    Engine();
                                    ~Engine()                                   { unsetup(); }

        bool                        setup (u32 mainWin_w, u32 mainWin_h, const char *mainWin_title);
        void                        unsetup();

            /* update:  ritorna false se la mainwin e' stata chiusa */
        bool                        update();
        bool                        inputEvent_getNext (InputEvent *out);
        
        
        
        //=============================
        void                        toggleFullscreen()                          { gpu->toggleFullscreen(); }
        void                        toggleVSync();

        //=============================
        bool                        vtxBuffer_create (u32 sizeInByte, eMemAccessMode mode, ENGVtxBuffer *out_handle);
        void                        vtxBuffer_release (ENGVtxBuffer &handle);
        const engine::VtxBuffer*    vtxBuffer_getInfo (const ENGVtxBuffer handle) const                           { return vtxBufferHandleList.getInfo(handle); }
        void                        priv_vtxBuffer_delete (engine::VtxBuffer *info);

        //=============================
        bool                        idxBuffer_create (u32 sizeInByte, eMemAccessMode mode, ENGIdxBuffer *out_handle);
        void                        idxBuffer_release (ENGIdxBuffer &handle);
        const engine::IdxBuffer*    idxBuffer_getInfo (const ENGIdxBuffer handle) const                           { return idxBufferHandleList.getInfo(handle); }
        void                        priv_idxBuffer_delete (engine::IdxBuffer *info);

        //=============================
                                    //crea una shape e le riserva spazio in VB/IB che sono gestiti dall'engine.
                                    //La engine::shape ritornata ha gia' gli handler VB/IB settati correttamente anche se i vtx/idx
                                    //NON sono ancora stati copiati nei buffer (lo devi fare te)
        bool                        shape_create (const gos::Shape *shape, ENGShape *out_handle);
        void                        shape_release (ENGShape &handle);
        const engine::Shape*        shape_getInfo (const ENGShape handle) const                                 { return shapeHandleList.getInfo(handle); }
        void                        priv_shape_delete (engine::Shape *info);

        //=============================
        
        gos::Allocator*             getAllocator() const                                                        { return allocator; }

    private:
        static constexpr u32    NUM_MAX_WMATRIX = 0xFFFF;

    private:
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
                    s->refCount = 1;
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
                    pt->refCount--;
                    if (pt->refCount <= 0)
                    {
                        ret = true;
                        memcpy (out, pt, sizeof(HANDLE_STRUCT));
                        list.release(handle);
                    }
                }
                unlock();
                return ret;
            }

            bool		    fromHandleToPointer (const HANDLE_TYPE &h, HANDLE_STRUCT* *out) const   { return list.fromHandleToPointer (h, out); }
            const HANDLE_STRUCT* getInfo (const HANDLE_TYPE handle) const                           { HANDLE_STRUCT *ret = NULL; list.fromHandleToPointer (handle, &ret); return ret; }

        private:
            gos::Mutex  mutex;
            HandleList<HANDLE_TYPE, HANDLE_STRUCT> list;
        };


    private:
        gos::Allocator          *allocator;
        bool                    bQuitEngine;
        input::ResolvedEvtList  evtList;
        HList<ENGVtxBuffer, engine::VtxBuffer>  vtxBufferHandleList;
        HList<ENGIdxBuffer, engine::IdxBuffer>  idxBufferHandleList;
        HList<ENGShape, engine::Shape>          shapeHandleList;


        engine::VtxBufferMan            vtxBufferMan;
        engine::IdxBufferMan            idxBufferMan;
        engine::FixedSizeBufferTracker  worldMatrixBufferMan;

    }; //class Engine
} //namespace gos


#endif //_gosEngine_h_

