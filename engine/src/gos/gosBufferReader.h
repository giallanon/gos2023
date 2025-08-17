#ifndef _gosBufferReader_h_
#define _gosBufferReader_h_
#include "gosEnumAndDefine.h"
#include "gosUtils.h"

namespace gos
{
    /**
     * @brief BufferR_interface
     * classe template di comodo che espone i metodi per la lettura di un buffer
     * Questa classe e' usata sia nella classe BufferR che nella class BuffeW per
     * fornire ad entrambe i metori di lettura
     * 
     * Non e' intesa per uso in solitaria
     */
    template<class T>
    class BufferR_interface
    {
    public:
                //read: avanzano il cursore
        bool    readAndCopyHere (void *dst, u32 howManyByte)            { if (readAtAndCopyHere (cursor, dst, howManyByte)) { cursor += howManyByte; return true; } return false; }
        u8      readU8  ()                                              { u8 ret; if (priv_readU8At(cursor, &ret)) { cursor++; return ret; } DBGBREAK; return 0; }
        u16     readU16 ()                                              { u16 ret; if (priv_readU16At(cursor, &ret)) { cursor += sizeof(ret); return ret; } DBGBREAK; return 0; }
        u32     readU32 ()                                              { u32 ret; if (priv_readU32At(cursor, &ret)) { cursor += sizeof(ret); return ret; } DBGBREAK; return 0; }

                //readAt: legge nella posizione <offset> lasciando inalterato <cursor>
        bool    readAtAndCopyHere (u32 offset, void *dst, u32 howManyByte) const    { const u32 finalOffset = offset + howManyByte; if (finalOffset > bufferSize) return false; memcpy (dst, &buffer[offset], howManyByte); return true; }
        u8      readU8At  (u32 offset) const                                        { u8 ret; if (priv_readU8At(offset, &ret)) return ret; DBGBREAK; return 0; }
        u16     readU16At (u32 offset) const                                        { u16 ret; if (priv_readU16At(offset, &ret)) return ret; DBGBREAK; return 0; }
        u32     readU32At (u32 offset) const                                        { u32 ret; if (priv_readU32At(offset, &ret)) return ret; DBGBREAK; return 0; }

    protected:
                BufferR_interface()                                     { }
                ~BufferR_interface()                                    { }

    protected:
        eEndianess  endianess;
        T           buffer;
        u32         bufferSize;
        u32         cursor;                

    private:
        bool    priv_readU8At  (u32 offset, u8 *out) const
                {
                    assert (NULL != out);
                    if (offset >= bufferSize)
                        return false;
                    *out = buffer[offset];
                    return true;
                }

        bool    priv_readU16At (u32 offset, u16 *out) const
                {
                    assert (NULL != out);
                    const u32 finalOffset = offset + sizeof(u16);
                    if (finalOffset > bufferSize)
                        return false;

                    if (eEndianess::big == endianess)
                        *out = gos::utils::bufferReadU16 (&buffer[offset]);
                    else
                        *out = gos::utils::bufferReadU16_LSB_MSB (&buffer[offset]);
                    return true;
                }

        bool    priv_readU32At (u32 offset, u32 *out) const
                {
                    assert (NULL != out);
                    const u32 finalOffset = offset + sizeof(u32);
                    if (finalOffset > bufferSize)
                        return false;

                    if (eEndianess::big == endianess)
                        *out = gos::utils::bufferReadU32 (&buffer[offset]);
                    else
                        *out = gos::utils::bufferReadU32_LSB_MSB (&buffer[offset]);
                    return true;
                }
    };

    /**
     * @brief BufferR
     * Classe di comodo che prende un generico buffer ed espone metodi di lettura sullo
     * stesso
     */
    class BufferR : public BufferR_interface<const u8*>
    {
    public:
                BufferR()                                               { setup (NULL,0); }
                ~BufferR()                                              { }

        void    setup (const void *bufferIN, u32 sizeof_buffer, eEndianess endianessIN = eEndianess::big);

        u32     tell() const                                            { return cursor; }        
        bool    advanceCursor (i32 howManyByte)                         { return moveCursorTo ( (u32) ((i32)cursor + howManyByte) ); }
        bool    moveCursorTo (u32 absOffset);    

        const u8*   getPointer (u32 absOffset) const                    { return &buffer[absOffset]; }    
    };
} //namespace gos
#endif //_gosBufferReader_h_