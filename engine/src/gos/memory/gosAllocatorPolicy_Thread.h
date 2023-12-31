#ifndef _gosAllocatorPolicy_Thread_h_
#define _gosAllocatorPolicy_Thread_h_
#include "../gos.h"

namespace gos
{
    /***********************************************************************
     * AllocPolicy_Thread_Unsafe
     */
    class AllocPolicy_Thread_Unsafe
    {
    public:
                AllocPolicy_Thread_Unsafe ()					{ }

        void	lock ()	const									{ }
        void	unlock () const									{ }
        bool	isThreadSafe () const							{ return false; }
    };


    /***********************************************************************
     * AllocPolicy_Thread_Safe
     */
    class AllocPolicy_Thread_Safe
    {
    public:
                AllocPolicy_Thread_Safe ()						{ gos::thread::mutexCreate(&mutex); }
                ~AllocPolicy_Thread_Safe ()						{ gos::thread::mutexDestroy(mutex); }

        void	lock ()	const									{ gos::thread::mutexLock(mutex); }
        void	unlock () const									{ gos::thread::mutexUnlock(mutex); }
        bool	isThreadSafe () const							{ return true; }

    private:
        mutable gos::Mutex mutex;
    };

} //namespace gos
#endif //_gosAllocatorPolicy_Thread_h_