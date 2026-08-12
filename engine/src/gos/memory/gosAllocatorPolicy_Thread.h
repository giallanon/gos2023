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
                AllocPolicy_Thread_Safe ()						{ gos::thread::mutex_create(&mutex); }
                ~AllocPolicy_Thread_Safe ()						{ gos::thread::mutex_destroy(mutex); }

        void	lock ()	const									{ gos::thread::mutex_lock(mutex); }
        void	unlock () const									{ gos::thread::mutex_unlock(mutex); }
        bool	isThreadSafe () const							{ return true; }

    private:
        mutable gos::Mutex mutex;
    };

} //namespace gos
#endif //_gosAllocatorPolicy_Thread_h_