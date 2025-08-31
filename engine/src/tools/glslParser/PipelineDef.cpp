#include "PipelineParser.h"
#include "gosUtils.h"
#include "gosMagicUID.h"
#include "../gosShape/gosShape.h"


using namespace gos;


//***********************************
void PipelineDef::setDefault()
{
    memset (this, 0, sizeof(PipelineDef));

    outputRT_fmt = eImageFormat::_SAME_AS_CURRENT_SWAPCHAIN;
    outputRT_finalLayout = eImageLayout::color_attachment_optimal;
    outputRT_loadOp = eAttachmentLoadOp::clear;
    outputRT_storeOp = eAttachmentStoreOp::store;
    outputRT_clearCol_ARGB = 0xFF000000;
    
    outputDepth_fmt = eImageFormat::_SAME_AS_CURRENT_SWAPCHAIN;
    outputDepth_finalLayout = eImageLayout::depth_attachment_optimal;
    outputDepth_loadOp = eAttachmentLoadOp::clear;
    outputDepth_storeOp = eAttachmentStoreOp::store;
    outputDepth_zClearValue = 1;
    outputDepth_stencilClearValue = 0;

    zbuffer_enabled = true;
    zbuffer_write = true;
    zbuffer_cmpFn = eZFunc::LESS;

    stencil_enabled = false;
    stencil_cmpFn = eStencilFunc::NEVER;

    cullMode = eCullMode::CCW;
    drawPrimitive = eDrawPrimitive::trisList;

    vtxLayout.reset();
}

//***********************************
u32 PipelineDef::serialize (u8 *buffer, u32 sizeof_buffer) const
{
    assert (NULL != buffer);
    const u32 byteNeeded =  sizeof(u32)
                        + NAME_MAX_SIZE
                        + 4 * sizeof(u8) + sizeof(u32)
                        + 4 * sizeof(u8) + sizeof(f32) + sizeof(f32)
                        + 3 * sizeof(u8)
                        + 2 * sizeof(u8)
                        + 2 * sizeof(u8)
                        + shape::serialize(vtxLayout, NULL, 0);

    if (NULL == buffer)
        return byteNeeded;
    if (sizeof_buffer < byteNeeded)
        return 0;

    u32 ct = 0;
    ct += utils::bufferWriteU32 (&buffer[ct], TEMP_MAGIC);
    
    memcpy (&buffer[ct], name, NAME_MAX_SIZE);
    ct += NAME_MAX_SIZE;

    ct += utils::bufferWriteU8 (&buffer[ct], static_cast<u8>(outputRT_fmt));
    ct += utils::bufferWriteU8 (&buffer[ct], static_cast<u8>(outputRT_finalLayout));
    ct += utils::bufferWriteU8 (&buffer[ct], static_cast<u8>(outputRT_loadOp));
    ct += utils::bufferWriteU8 (&buffer[ct], static_cast<u8>(outputRT_storeOp));
    ct += utils::bufferWriteU32 (&buffer[ct], outputRT_clearCol_ARGB);


    ct += utils::bufferWriteU8 (&buffer[ct], static_cast<u8>(outputDepth_fmt));
    ct += utils::bufferWriteU8 (&buffer[ct], static_cast<u8>(outputDepth_finalLayout));
    ct += utils::bufferWriteU8 (&buffer[ct], static_cast<u8>(outputDepth_loadOp));
    ct += utils::bufferWriteU8 (&buffer[ct], static_cast<u8>(outputDepth_storeOp));       
    ct += utils::bufferWriteF32 (&buffer[ct], outputDepth_zClearValue);
    ct += utils::bufferWriteF32 (&buffer[ct], outputDepth_stencilClearValue);


    ct += utils::bufferWriteU8 (&buffer[ct], static_cast<u8>(zbuffer_enabled));
    ct += utils::bufferWriteU8 (&buffer[ct], static_cast<u8>(zbuffer_write));
    ct += utils::bufferWriteU8 (&buffer[ct], static_cast<u8>(zbuffer_cmpFn));


    ct += utils::bufferWriteU8 (&buffer[ct], static_cast<u8>(stencil_enabled));
    ct += utils::bufferWriteU8 (&buffer[ct], static_cast<u8>(stencil_cmpFn));


    ct += utils::bufferWriteU8 (&buffer[ct], static_cast<u8>(cullMode));
    ct += utils::bufferWriteU8 (&buffer[ct], static_cast<u8>(drawPrimitive));

    ct += shape::serialize(vtxLayout, &buffer[ct], sizeof_buffer - ct);

    assert (ct == byteNeeded);
    return byteNeeded;
}

//***********************************
u32 PipelineDef::deserialize (const u8 *buffer, u32 sizeof_buffer)
{
    assert (NULL != buffer);
    const u32 MIN_byteNeeded =  sizeof(u32)
                                + NAME_MAX_SIZE
                                + 4 * sizeof(u8) + sizeof(u32)
                                + 4 * sizeof(u8) + sizeof(f32) + sizeof(f32)
                                + 3 * sizeof(u8)
                                + 2 * sizeof(u8)
                                + 2 * sizeof(u8);

    if (sizeof_buffer < MIN_byteNeeded)
        return 0;

    u32 ct = 0;
    const u32 magic = utils::bufferReadU32 (&buffer[ct]);
    ct += 4;

    if (!magic::signatureMatch(magic, TEMP_MAGIC))
    {
        DBGBREAK;
        return 0;
    }
    if (!magic::versionMatch(magic, TEMP_MAGIC))
    {
        DBGBREAK;
        return 0;
    }

    memcpy (name, &buffer[ct], NAME_MAX_SIZE);
    ct += NAME_MAX_SIZE;    


#define READ_U8(TYPE, VAR)  VAR = static_cast<TYPE>(buffer[ct++]);

    READ_U8( eImageFormat, outputRT_fmt)
    READ_U8( eImageLayout, outputRT_finalLayout)
    READ_U8( eAttachmentLoadOp, outputRT_loadOp)
    READ_U8( eAttachmentStoreOp, outputRT_storeOp)
    outputRT_clearCol_ARGB = utils::bufferReadU32 (&buffer[ct]); ct+=4;

    READ_U8( eImageFormat, outputDepth_fmt)
    READ_U8( eImageLayout, outputDepth_finalLayout)
    READ_U8( eAttachmentLoadOp, outputDepth_loadOp)
    READ_U8( eAttachmentStoreOp, outputDepth_storeOp)
    outputDepth_zClearValue = utils::bufferReadF32 (&buffer[ct]); ct+=4;
    outputDepth_stencilClearValue = utils::bufferReadF32 (&buffer[ct]); ct+=4;

    READ_U8( bool, zbuffer_enabled)
    READ_U8( bool, zbuffer_write)
    READ_U8( eZFunc, zbuffer_cmpFn)

    READ_U8( bool, stencil_enabled)
    READ_U8( eStencilFunc, stencil_cmpFn)

    READ_U8( eCullMode, cullMode)
    READ_U8( eDrawPrimitive, drawPrimitive)

    assert (ct == MIN_byteNeeded);

    u32 n = shape::deserialize (&buffer[ct], sizeof_buffer - ct, &vtxLayout);
    if (0 == n)
        return 0;
    ct += n;
    
    return ct;
}
