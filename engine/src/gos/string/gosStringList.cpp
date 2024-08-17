#include "gosStringList.h"
#include "../gosString.h"

using namespace gos;

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
