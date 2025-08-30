#include "gosStringIncludeDetector.h"
#include "../gos.h"

using namespace gos;
using namespace gos::string;


//************************************ 
IncludeDetector::IncludeDetector()
{
    list.setup (gos::getSysHeapAllocator(), 128);
}

//************************************ 
bool IncludeDetector::getResultByIndex (u32 index, u32 *out_startAtByte, u32 *out_len) const
{
    assert (NULL != out_startAtByte);
    assert (NULL != out_len);
    if (index < list.getNElem())
    {
        *out_startAtByte = list(index).startAt;
        *out_len = list(index).len;
        return true;
    }
    return false;
}

//************************************ 
bool IncludeDetector::getResultAsString (const u8 *bufferSRC, u32 index, char *out, u32 sizeof_out) const
{
    assert (NULL != bufferSRC);
    assert (NULL != out);
    assert (sizeof_out > 1);

    if (index < list.getNElem())
    {
        u32 len = list(index).len;
        if (len >= sizeof_out-1)
            len = sizeof_out-1;
        memcpy (out, &bufferSRC[list(index).startAt], len);
        out[len] = 0;
        return true;
    }

    return false;
}

//************************************ 
bool IncludeDetector::getResultAsString (const char *bufferSRC, u32 index, char *out, u32 sizeof_out) const
{
    assert (NULL != bufferSRC);
    assert (NULL != out);
    assert (sizeof_out > 1);

    if (index < list.getNElem())
    {
        u32 len = list(index).len;
        if (len >= sizeof_out-1)
            len = sizeof_out-1;
        memcpy (out, &bufferSRC[list(index).startAt], len);
        out[len] = 0;
        return true;
    }

    return false;
}


//************************************ 
u32 IncludeDetector::parse (gos::string::utf8::Iter &srcIN)
{
    list.reset();

    utf8::Iter result;

    while (false == srcIN.getCurChar().isEOF())
    {
        srcIN.toNextValidChar();
        if (utf8::extractCPPComment (srcIN, &result, NULL))
            continue;

        srcIN.toNextValidChar();

        u32 src_offset = srcIN.getCursorPos();
        utf8::extractLine (srcIN, &result);
        if (0 != result.getBytesLeft())
        {
            if (utf8::find (result, "#include"))
            {
                result.advanceNumByte (8);
                result.toNextValidChar();
                if (result.getCurChar() == '"')
                {
                    src_offset += result.getCursorPos();

                    utf8::Iter result2;
                    if (utf8::extractValue (result, &result2))
                    {
                        sElem elem;
                        elem.startAt = src_offset + 1 + result2.getCursorPos();
                        elem.len = result2.getBytesLeft();
                        list.append (elem);
                    }
                }
            }
        }
    }

    return list.getNElem();
}
