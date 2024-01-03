#include "gosThreadMsgQ.h"
#include "gosFIFO.h"
#include "gosUtils.h"
#include "memory/gosAllocatorHeap.h"

using namespace gos;

//data struct per un thread
struct sThreadInfo
{
    platform::OSThread      	osThreadHandle;
	GOSThread					gosHandle;
	GOS_ThreadMainFunction		fn;
	void 						*userParam;
};

//handle list per i thread
typedef gos::HandleList<GOSThread,sThreadInfo> GOSThreadHandleList;

//FIFO dei msg di una msgQ
typedef gos::FIFO<thread::sMsg>  GOSThreadMsgFIFO;

//data struct per una msgQ
struct sThreadMsgQ
{
    GOSThreadMsgFIFO    *fifo;
    gos::Event          hEvent;
    gos::Mutex          cs;
};

//handle list per le msgQ
typedef gos::HandleList<HThreadMsgHandle, sThreadMsgQ> GOSThreadMsgHandleArray;

//allocatore 
typedef gos::AllocatorHeap<gos::AllocPolicy_Track_simple, gos::AllocPolicy_Thread_Safe>		GOSThreadMemAllocatorTS;


struct sGlob
{
    GOSThreadMemAllocatorTS     *allocator;
    gos::Mutex                  cs;
    GOSThreadMsgHandleArray     msgHandleList;
	GOSThreadHandleList	        threadHandleList;
};
static sGlob   gosThreadGlob;


//**************************************************************
bool thread::internal_init()
{
    //Creo un allocatore dedicato
    gosThreadGlob.allocator = GOSNEW(gos::getSysHeapAllocator(), GOSThreadMemAllocatorTS)("Thread");
    gosThreadGlob.allocator->setup (1024 * 1024 * 1); //1MB

    gosThreadGlob.msgHandleList.setup (gosThreadGlob.allocator);
	gosThreadGlob.threadHandleList.setup (gosThreadGlob.allocator);
    
    gos::thread::mutexCreate (&gosThreadGlob.cs);

    return true;
}

//**************************************************************
void thread::internal_deinit()
{
    gosThreadGlob.msgHandleList.unsetup();
	gosThreadGlob.threadHandleList.unsetup();
    gos::thread::mutexDestroy (gosThreadGlob.cs);

    GOSDELETE(gos::getSysHeapAllocator(), gosThreadGlob.allocator);
    gosThreadGlob.allocator = NULL;
}


//************************************************************************
i16 GOS_threadFunctionWrapper (void *userParam)
{
    //recupero l'handle del thread
    GOSThread *_pt_to_handle = reinterpret_cast<GOSThread*>(userParam);
    GOSThread handle = *_pt_to_handle;

    //recupero le info sul thread
    sThreadInfo *info = NULL;
    gosThreadGlob.threadHandleList.fromHandleToPointer (handle, &info);
    if (NULL == info)
    {
        DBGBREAK;
        return i16MIN;
    }

    //main fn
    i16 retCode = (*info->fn)(info->userParam);
	
    //invalido il thread handle e libero le risorse
	gosThreadGlob.threadHandleList.release (handle);

	return retCode;
}


//************************************************************************
eThreadError gos::thread::create (GOSThread *out_hThread, GOS_ThreadMainFunction threadFunction, void *userParam, u16 stackSizeInKb)
{
    //riservo un handle
	sThreadInfo *info = gosThreadGlob.threadHandleList.reserve (out_hThread);
	if (NULL == info)
		return eThreadError::tooMany;
	info->gosHandle = *out_hThread;
	info->fn = threadFunction;
	info->userParam = userParam;
	
    //creo il thread
    eThreadError err = platform::createThread (&info->osThreadHandle, gosThreadGlob.allocator, GOS_threadFunctionWrapper, stackSizeInKb, out_hThread);
    if (eThreadError::none != err)
	{
		gosThreadGlob.threadHandleList.release (*out_hThread);
		out_hThread->setInvalid();
	}

	return err;
}


//************************************************************************
void gos::thread::waitEnd (GOSThread &hThread)
{
	sThreadInfo *info;
	if (gosThreadGlob.threadHandleList.fromHandleToPointer (hThread, &info))
	    platform::waitThreadEnd (info->osThreadHandle);
}




//**************************************************************
sThreadMsgQ* thread_HTreadMsgHandle_to_pointer (const HThreadMsgHandle &h)
{
	sThreadMsgQ *s = NULL;
	if (gosThreadGlob.msgHandleList.fromHandleToPointer(h, &s))
		return s;
	return NULL;
}

//**************************************************************
bool thread::createMsgQ (HThreadMsgR *out_handleR, HThreadMsgW *out_handleW)
{
    assert (NULL != out_handleR);
    assert (NULL != out_handleW);
    out_handleR->hRead.setInvalid();
    out_handleW->hWrite.setInvalid();

    MUTEX_LOCK(gosThreadGlob.cs)
        sThreadMsgQ *s = NULL;
        HThreadMsgHandle msgHandle;
        s = gosThreadGlob.msgHandleList.reserve (&msgHandle);
    MUTEX_UNLOCK(gosThreadGlob.cs)

    if (NULL == s)
    {
        DBGBREAK;
        return false;
    }

    s->fifo = GOSNEW(gosThreadGlob.allocator, GOSThreadMsgFIFO) ();
	s->fifo->setup(gosThreadGlob.allocator);
	gos::thread::eventCreate (&s->hEvent);
    gos::thread::mutexCreate (&s->cs);

    out_handleR->hRead = msgHandle;
    out_handleW->hWrite = msgHandle;
    return true;
}

//**************************************************************
void thread::deleteMsgQ (HThreadMsgR &handleR, UNUSED_PARAM(HThreadMsgW &handleW))
{
    sThreadMsgQ *s = thread_HTreadMsgHandle_to_pointer(handleR.hRead);
    if (NULL == s)
        return;

    MUTEX_LOCK (gosThreadGlob.cs);
	{
		MUTEX_LOCK (s->cs);
		{
			thread::sMsg msg;
			while ( s->fifo->pop(&msg) )
				thread::deleteMsg (msg);

			s->fifo->unsetup();
			GOSDELETE( gosThreadGlob.allocator, s->fifo );
		}
		MUTEX_UNLOCK(s->cs);
		
        gos::thread::mutexDestroy(s->cs);
		gos::thread::eventDestroy (s->hEvent);
        gosThreadGlob.msgHandleList.release (handleR.hRead);
    }
    MUTEX_UNLOCK (gosThreadGlob.cs);
}

//**************************************************************
u32 thread::calcSizeNeededToSerializeMsg (const sMsg &msg)
{
    return sizeof(msg.what) + sizeof(msg.paramU32) + sizeof(msg.bufferSize) + msg.bufferSize;
}

//**************************************************************
u32 thread::serializeMsg (const sMsg &msg, u8 *out_buffer, u32 sizeof_out_buffer)
{
    const u32 bytesNeeded = calcSizeNeededToSerializeMsg(msg);
    assert (bytesNeeded < 0xffff);
    if (sizeof_out_buffer < bytesNeeded)
    {
        DBGBREAK;
        return 0;
    }

    u32 ct = 0;
    gos::utils::bufferWriteU16 (&out_buffer[ct], msg.what);
    ct += 2;

    gos::utils::bufferWriteU32 (&out_buffer[ct], msg.paramU32);
    ct += 4;

    gos::utils::bufferWriteU32 (&out_buffer[ct], msg.bufferSize);
    ct += 4;

    if (msg.bufferSize)
    {
        memcpy (&out_buffer[ct], msg.buffer, msg.bufferSize);
        ct += msg.bufferSize;
    }

    assert (ct == bytesNeeded);
    return ct;
}

//**************************************************************
u32 thread::deserializMsg (const u8 *buffer, u16 *out_what, u32 *out_paramU32, u32 *out_bufferSize, const u8 **out_bufferPt)
{
    u32 ct = 0;

    *out_what = gos::utils::bufferReadU16 (&buffer[ct]);
    ct += 2;

    *out_paramU32 = gos::utils::bufferReadU32 (&buffer[ct]);
    ct += 4;

    *out_bufferSize = gos::utils::bufferReadU32 (&buffer[ct]);
    ct += 4;

    if (0 == *out_bufferSize)
        *out_bufferPt = NULL;
    else
    {
        *out_bufferPt = &buffer[ct];
        ct += *out_bufferSize;
    }

    return ct;
}


//**************************************************************
void thread::pushMsg (const HThreadMsgW &h, u16 what, u32 paramU32, const void *src, u32 sizeInBytes)
{
    sThreadMsgQ *s = thread_HTreadMsgHandle_to_pointer(h.hWrite);

    if (NULL == s)
        return;

    thread::sMsg msg;
    memset (&msg, 0x00, sizeof(msg));
    msg.what = what;
    msg.paramU32 = paramU32;
    if (src && sizeInBytes>0)
    {
        msg.bufferSize = sizeInBytes;
        msg.buffer = GOSALLOC(gosThreadGlob.allocator, sizeInBytes);
        memcpy (msg.buffer, src, sizeInBytes);
    }
    else
    {
        msg.buffer = NULL;
        msg.bufferSize = 0;
    }

    MUTEX_LOCK (s->cs);
        s->fifo->push(msg);
		gos::thread::eventFire (s->hEvent);
    MUTEX_UNLOCK (s->cs);
}

//**************************************************************
void thread::pushMsg2Buffer (const HThreadMsgW &h, u16 what, u32 paramU32, const void *src1, u32 sizeInBytes1, const void *src2, u32 sizeInBytes2)
{
    sThreadMsgQ *s = thread_HTreadMsgHandle_to_pointer(h.hWrite);

    if (NULL == s)
        return;

    thread::sMsg msg;
    memset (&msg, 0x00, sizeof(msg));
    msg.what = what;
    msg.paramU32 = paramU32;
    msg.buffer = NULL;
    msg.bufferSize = 0;

    if (sizeInBytes1 && NULL != src1)
    {
        msg.bufferSize += sizeInBytes1;
        if (sizeInBytes2 && NULL != src2)
            msg.bufferSize += sizeInBytes2;
        
        msg.buffer = GOSALLOC(gosThreadGlob.allocator, msg.bufferSize);
        memcpy (msg.buffer, src1, sizeInBytes1);
        if (NULL != src2)
        {
            u8 *p = (u8*)msg.buffer;
            memcpy (&p[sizeInBytes1], src2, sizeInBytes2);
        }
    }

    MUTEX_LOCK (s->cs);
        s->fifo->push(msg);
		gos::thread::eventFire(s->hEvent);
    MUTEX_UNLOCK (s->cs);
}

//**************************************************************
bool thread::getMsgQEvent (const HThreadMsgR &h, gos::Event *out_hEvent)
{
    sThreadMsgQ *s = thread_HTreadMsgHandle_to_pointer(h.hRead);

    if (NULL == s)
        return false;
    *out_hEvent = s->hEvent;
    return true;
}

//**************************************************************
bool thread::popMsg (const HThreadMsgR &h, thread::sMsg *out_msg)
{
    sThreadMsgQ *s = thread_HTreadMsgHandle_to_pointer(h.hRead);
    if (NULL == s)
        return false;

    MUTEX_LOCK (s->cs);
        bool ret = s->fifo->pop(out_msg);
    MUTEX_UNLOCK (s->cs);

    return ret;
}

//**************************************************************
void thread::deleteMsg (const sMsg &msg)
{
    if (msg.bufferSize > 0 && msg.buffer != NULL)
    {
        GOSFREE(gosThreadGlob.allocator, msg.buffer);
    }
}




