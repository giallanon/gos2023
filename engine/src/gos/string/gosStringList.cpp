#include "gosStringList.h"
#include "../gosString.h"
#include "../gos.h"
#include "../gosUtils.h"

using namespace gos;

//***************************************
u32 StringList::serialize_calcSizeNeeded() const
{
    return 8 + cursor;
}

//***************************************
u32 StringList::serialize_toMemory (u8 *mem, u32 sizeof_mem) const
{
    const u32 needed = serialize_calcSizeNeeded();

    if (sizeof_mem < needed)
    {
        DBGBREAK;
        return 0;
    }

    u32 ct = 0;
    ct += gos::utils::bufferWriteU32 (&mem[ct], cursor);
    ct += gos::utils::bufferWriteU32 (&mem[ct], count);

    if (cursor)
    {
        memcpy (&mem[ct], buffer._getPointer(0), cursor);
        ct += cursor;
    }

    assert (ct == needed);
    return needed;
}

//***************************************
u32 StringList::deserialize_fromMemory (gos::Allocator *allocatorIN, const u8 *mem, u32 sizeof_mem)
{
    unsetup();

    if (sizeof_mem < 8)
    {
        DBGBREAK;
        return 0;
    }

    u32 ct = 0;
    u32 read_cursor, read_count;
    ct += gos::utils::bufferReadU32 (&mem[ct], &read_cursor);
    ct += gos::utils::bufferReadU32 (&mem[ct], &read_count);

    if (sizeof_mem < ct + read_cursor)
    {
        DBGBREAK;
        return 0;
    }

    setup (allocatorIN, 8 + read_cursor);
    cursor = read_cursor;
    count = read_count;
    if (cursor)
    {
        memcpy (buffer._getPointer(0), &mem[ct], cursor);
        ct += cursor;
    }

    assert (ct == serialize_calcSizeNeeded());
    return ct;
}

//***************************************
void StringList::clone_from (gos::Allocator *allocatorIN, const StringList &src)
{
    unsetup();

    setup (allocatorIN, src.getUsedMemSize());
    cursor = src.cursor;
    count = src.count;
    buffer.copyFrom (src.buffer, 0, src.getUsedMemSize(), 0);
}

//***************************************
u32 StringList::add (const char *m)
{
    const u32 n = static_cast<u32>(strlen(m)) +1;
    return priv_doAdd (m, n);
}

//***************************************
u32 StringList::priv_doAdd (const void *m, u32 sizeInByte)
{
    if (sizeInByte == 1)
        return u32MAX;
    if (cursor + sizeInByte >= buffer.getTotalSizeAllocated())
        buffer.growIncremental (512);

    const u32 ret = cursor;
    buffer.write (m, cursor, sizeInByte, false);
    cursor += sizeInByte;
    count++;

    return ret;
}

//***************************************
const char* StringList::getStringAtOffset (u32 offset) const
{
    if (offset >= cursor)
        return NULL;
    return reinterpret_cast<const char*>(buffer._getPointer(offset));
}

//***************************************
const char* StringList::next (u32 *iter) const 
{
    u32 offset = ((*iter) & 0x0000FFFF);
    u32 stringNum = (((*iter) & 0xFFFF0000) >> 16);
    if (stringNum >= count)
        return NULL;
    
    const char *ret = reinterpret_cast<const char*>(buffer._getPointer(offset));

    stringNum++;
    const u32 n = static_cast<u32>(strlen(ret));
    offset += (n+1);
    (*iter) = offset | (stringNum<<16);
    return ret;
}
