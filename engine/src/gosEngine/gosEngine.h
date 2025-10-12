#ifndef _gosEngine_h_
#define _gosEngine_h_
#include "gosEngineEnumAndDefine.h"
#include "gosEngine_vtxBufferMan.h"
#include "gosEngine_idxBufferMan.h"

namespace gos
{
    /****************
     * @brief   Engine
     * 
     * 
     */
    class Engine
    {
    public:
                                    Engine();
                                    ~Engine()                                   { unsetup(); }

        bool                        setup (u32 mainWin_w, u32 mainWin_h, const char *mainWin_title);
        void                        unsetup();

            /* update:  ritorna false se la mainwin e' stata chiusa */
        bool                        update();
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
        bool                        shape_create (const gos::Shape *shape, ENGShape *out_handle);
        void                        shape_release (ENGShape &handle);
        const engine::Shape*        shape_getInfo (const ENGShape handle) const                               { return shapeHandleList.getInfo(handle); }
        void                        priv_shape_delete (engine::Shape *info);

    public:
        gos::GPU                *gpu;
        gos::input::Context     *inputCtx;
        gos::asset::Hub         *assetHub;

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
        void    priv_handleInput();

    private:
        gos::Allocator          *allocator;
        bool                    bQuitEngine;
        
        HList<ENGVtxBuffer, engine::VtxBuffer>  vtxBufferHandleList;
        HList<ENGIdxBuffer, engine::IdxBuffer>  idxBufferHandleList;
        HList<ENGShape, engine::Shape>          shapeHandleList;


        engine::VtxBufferMan    vtxBufferMan;
        engine::IdxBufferMan    idxBufferMan;

    }; //class Engine
} //namespace gos


#endif //_gosEngine_h_

