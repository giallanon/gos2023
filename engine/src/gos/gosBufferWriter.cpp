#include "gosBufferWriter.h"
#include "gosUtils.h"


using namespace gos;

//*********************************** 
void BufferW_interface::prot_setup (void *bufferIN, u32 sizeof_buffer, eEndianess endianessIN)
{
    buffer = reinterpret_cast<u8*>(bufferIN);
    bufferSize = sizeof_buffer;
    endianess = endianessIN;
    cursor = maxWritePos = 0;
}

//*********************************** 
bool BufferW_interface::prot_moveCursorTo (u32 absOffset)
{
    bool ret = true;
    if (absOffset <= bufferSize)
        cursor = absOffset;
    else
    {
        cursor = bufferSize;
        ret = false;
    }
        
    return ret;
}

//*********************************** 
bool BufferW_interface::writePadUntilMultiplo4()
{
    u32 i = cursor & 4;
    if (0 == i)
        return true;
    i = 4-i;
    while (i--)
    {
        if (!writeU8 (0))
            return false;
    }
    return true;
}

//*********************************** 
bool BufferW_interface::writePadUntilMultiplo8()
{
    u32 i = cursor & 8;
    if (0 == i)
        return true;
    i = 8-i;
    while (i--)
    {
        if (!writeU8 (0))
            return false;
    }
    return true;
}


//*********************************** 
bool BufferW_interface::priv_writeAt (u32 offset, const void *src, u32 howManyByte, bool bMoveCursor)
{
    const u32 finalOffset = offset + howManyByte;
    if (finalOffset > bufferSize)
    {
        u8 *newBuffer = NULL;
        u32 newSize = 0;
        if (!virt_growUpTo(finalOffset, &newBuffer, &newSize))
            return false;
        buffer = newBuffer;
        bufferSize = newSize;        
    }

    memcpy (&buffer[offset], src, howManyByte);

    if (finalOffset > maxWritePos)
        maxWritePos = finalOffset;
    if (bMoveCursor)
        cursor = finalOffset;
    return true;
}

//*********************************** 
bool BufferW_interface::priv_writeU8At (u32 offset, u8 data, bool bMoveCursor)
{
    return priv_writeAt (offset, &data, 1, bMoveCursor);
}

//*********************************** 
bool BufferW_interface::priv_writeU16At (u32 offset, u16 data, bool bMoveCursor)
{
    u8 temp[4];
    
    if (eEndianess::big == endianess)
        gos::utils::bufferWriteU16 (temp, data);
    else
        gos::utils::bufferWriteU16_LSB_MSB (temp, data);
    
    return priv_writeAt (offset, temp, 2, bMoveCursor);
}

//*********************************** 
bool BufferW_interface::priv_writeU32At (u32 offset, u32 data, bool bMoveCursor)
{
    u8 temp[4];
    
    if (eEndianess::big == endianess)
        gos::utils::bufferWriteU32 (temp, data);
    else
        gos::utils::bufferWriteU32_LSB_MSB (temp, data);
    
    return priv_writeAt (offset, temp, 4, bMoveCursor);
}

//*********************************** 
bool BufferW_interface::priv_writeU64At (u32 offset, u64 data, bool bMoveCursor)
{
    u8 temp[8];
    
    if (eEndianess::big == endianess)
        gos::utils::bufferWriteU64 (temp, data);
    else
        gos::utils::bufferWriteU64_LSB_MSB (temp, data);
    
    return priv_writeAt (offset, temp, 8, bMoveCursor);
}
