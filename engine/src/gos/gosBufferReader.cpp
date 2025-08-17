#include "gosBufferReader.h"

using namespace gos;

//*****************************************
void BufferR::setup (const void *bufferIN, u32 sizeof_buffer, eEndianess endianessIN)
{
    buffer = reinterpret_cast<const u8*>(bufferIN);
    bufferSize = sizeof_buffer;
    endianess = endianessIN;
    cursor = 0;
}        

//*****************************************
bool BufferR::moveCursorTo (u32 absOffset)
{
    if (absOffset <= bufferSize)
    {
        cursor = absOffset;
        return true;
    }
    cursor = bufferSize;
    return false;
}  