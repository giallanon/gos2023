#include "gosGPUFramebuffer_def.h"
#include "../gos/gosMagicUID.h"

using namespace gos;
using namespace gos::gpu;

//***********************************************
u32 gpu::serialize (const Framebuffer_def &def, u8 *buffer, u32 sizeof_buffer)
{
    const u32 byteNeeded =  sizeof(u32)
                            + sizeof(u32)
                            + def.numAttachment * 5;
    if (NULL == buffer)
        return byteNeeded;
    if (sizeof_buffer < byteNeeded)
        return 0;

    u32 ct = 0;
    ct += utils::bufferWriteU32 (&buffer[ct], GOS_MAGIC__GPU_FRAME_BUFFER_DEF);
    ct += utils::bufferWriteU32 (&buffer[ct], def.numAttachment);
    for (u32 i=0; i<def.numAttachment; i++)
    {
        buffer[ct++] = static_cast<u8>(def.attachment[i].fmt);
        buffer[ct++] = static_cast<u8>(def.attachment[i].initialLayout);
        buffer[ct++] = static_cast<u8>(def.attachment[i].finalLayout);
        buffer[ct++] = static_cast<u8>(def.attachment[i].loadOp);
        buffer[ct++] = static_cast<u8>(def.attachment[i].storeOp);
    }

    assert (ct == byteNeeded);
    return byteNeeded;
}

//***********************************************
u32 gpu::deserialize (const u8 *buffer, u32 sizeof_buffer, Framebuffer_def *out)
{
    assert (NULL != buffer);
    assert (NULL != out);

    out->reset();
    if (sizeof_buffer < 8)
        return 0;

    u32 ct = 0;        
    const u32 magic = utils::bufferReadU32 (&buffer[ct]);
    ct+=4;
    if (!magic::signatureMatch(magic, GOS_MAGIC__GPU_FRAME_BUFFER_DEF))
    {
        DBGBREAK;
        return 0;
    }
    if (!magic::versionMatch(magic, GOS_MAGIC__GPU_FRAME_BUFFER_DEF))
    {
        DBGBREAK;
        return 0;
    }

    out->numAttachment = utils::bufferReadU32 (&buffer[ct]);
    ct+=4;

    const u32 byteNeeded =  sizeof(u32) + sizeof(u32) + out->numAttachment * 5;
    if (sizeof_buffer < byteNeeded)
        return 0;

    for (u32 i=0; i<out->numAttachment; i++)
    {
        out->attachment[i].fmt = static_cast<eImageFormat>(buffer[ct++]);
        out->attachment[i].initialLayout = static_cast<eImageLayout>(buffer[ct++]);
        out->attachment[i].finalLayout = static_cast<eImageLayout>(buffer[ct++]);
        out->attachment[i].loadOp = static_cast<eAttachmentLoadOp>(buffer[ct++]);
        out->attachment[i].storeOp = static_cast<eAttachmentStoreOp>(buffer[ct++]);
    }

    assert (ct == byteNeeded);
    return byteNeeded;
}
