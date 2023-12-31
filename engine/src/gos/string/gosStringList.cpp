#include "gosStringList.h"
#include "../gosString.h"

using namespace gos;

//***************************************
void StringList::add (const char *m)
{
    const u32 n = static_cast<u32>(strlen(m)) +1;
    priv_doAdd (m, n);
}

//***************************************
void StringList::add (const u8 *m)
{
    const u32 n = gos::string::utf8::lengthInByte(m) +1;
    priv_doAdd (m, n);
}

//***************************************
void StringList::priv_doAdd (const void *m, u32 sizeInByte)
{
    if (sizeInByte == 1)
        return;
    if (cursor + sizeInByte >= buffer.getTotalSizeAllocated())
        buffer.growIncremental (512);

    buffer.write (m, cursor, sizeInByte, false);
    cursor += sizeInByte;
    count++;
}

//***************************************
const char* StringList::nextAsChar (u32 *iter) const 
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

//***************************************
const u8* StringList::nextAsU8 (u32 *iter) const 
{
    u32 offset = ((*iter) & 0x0000FFFF);
    u32 stringNum = (((*iter) & 0xFFFF0000) >> 16);
    if (stringNum >= count)
        return NULL;
    
    const u8 *ret = buffer._getPointer(offset);

    stringNum++;
    const u32 n = string::utf8::lengthInByte(ret);
    offset += (n+1);
    (*iter) = offset | (stringNum<<16);
    return ret;
}
