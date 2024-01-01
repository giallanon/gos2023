#include "gosThreadMsgQ.h"
#include "gosFIFO.h"
#include "gosUtils.h"

using namespace gos;

typedef gos::FIFO<thread::sMsg>  GOSThreadMsgFIFO;

struct sThreadMsgQ
{
    GOSThreadMsgFIFO    *fifo;
    gos::Event          hEvent;
    gos::Mutex          cs;
};

typedef gos::HandleList<HThreadMsgHandle, sThreadMsgQ> GOSThreadMsgHandleArray;

struct sGlob
{
    GOSThreadMsgHandleArray     msgHandleList;
    gos::Allocator              *allocator;
    gos::Mutex                  cs;
};
static sGlob   gosThreadGlob;


//**************************************************************
bool thread::internal_init()
{
    gosThreadGlob.allocator = gos::getSysHeapAllocator();
    gosThreadGlob.msgHandleList.setup (gosThreadGlob.allocator);
    gos::thread::mutexCreate (&gosThreadGlob.cs);
    return true;
}

//**************************************************************
void thread::internal_deinit()
{
    gosThreadGlob.msgHandleList.unsetup();
    gos::thread::mutexDestroy (gosThreadGlob.cs);
}

//**************************************************************
sThreadMsgQ* thread_fromHandleToPointer (const HThreadMsgHandle &h)
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

    sThreadMsgQ *s = NULL;
    HThreadMsgHandle msgHandle;
    gos::thread::mutexLock (gosThreadGlob.cs);
    {
        s = gosThreadGlob.msgHandleList.reserve (&msgHandle);
    }
    gos::thread::mutexUnlock (gosThreadGlob.cs);

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
    sThreadMsgQ *s = thread_fromHandleToPointer(handleR.hRead);
    if (NULL == s)
        return;

    gos::thread::mutexLock(gosThreadGlob.cs);
	{
		gos::thread::mutexLock (s->cs);
		{
			thread::sMsg msg;
			while ( s->fifo->pop(&msg) )
				thread::deleteMsg (msg);

			s->fifo->unsetup();
			GOSDELETE( gosThreadGlob.allocator, s->fifo );
		}
		gos::thread::mutexUnlock(s->cs);
		gos::thread::mutexDestroy(s->cs);

		gos::thread::eventDestroy (s->hEvent);

        gosThreadGlob.msgHandleList.release (handleR.hRead);
    }
    gos::thread::mutexUnlock (gosThreadGlob.cs);
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
    sThreadMsgQ *s = thread_fromHandleToPointer(h.hWrite);

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

    gos::thread::mutexLock (s->cs);
        s->fifo->push(msg);
		gos::thread::eventFire (s->hEvent);
    gos::thread::mutexUnlock (s->cs);
}

//**************************************************************
void thread::pushMsg2Buffer (const HThreadMsgW &h, u16 what, u32 paramU32, const void *src1, u32 sizeInBytes1, const void *src2, u32 sizeInBytes2)
{
    sThreadMsgQ *s = thread_fromHandleToPointer(h.hWrite);

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

    gos::thread::mutexLock (s->cs);
        s->fifo->push(msg);
		gos::thread::eventFire(s->hEvent);
    gos::thread::mutexUnlock (s->cs);
}

//**************************************************************
bool thread::getMsgQEvent (const HThreadMsgR &h, gos::Event *out_hEvent)
{
    sThreadMsgQ *s = thread_fromHandleToPointer(h.hRead);

    if (NULL == s)
        return false;
    *out_hEvent = s->hEvent;
    return true;
}

//**************************************************************
bool thread::popMsg (const HThreadMsgR &h, thread::sMsg *out_msg)
{
    sThreadMsgQ *s = thread_fromHandleToPointer(h.hRead);
    if (NULL == s)
        return false;

    gos::thread::mutexLock (s->cs);
        bool ret = s->fifo->pop(out_msg);
    gos::thread::mutexUnlock (s->cs);

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




