#ifndef _gosBufferWriter_h_
#define _gosBufferWriter_h_
#include "gosBufferReader.h"
#include "gosBufferLinear.h"

namespace gos
{
    /**
     * @brief BufferW_interface
     * Classe di comodo che prende un generico buffer ed espone metodi di lettura e scrittura sullo
     * stesso.
     * 
     * I metodi di lettura derivano dal template BufferR_interface<>, vedi anche gosBufferReader.h
     * 
     * BufferW_interface non e' intesa per uso standalone, vedi sotto le classi BufferW_fixed e BufferW_linear
     */    
    class BufferW_interface: public BufferR_interface<u8*>
    {
    public:
        void    reset()                                                 { cursor = maxWritePos = 0; }

                //ritorna la posizione + alta utilizzata all'iterno del buffer
        u32     getMaxWritePos() const                                  { return maxWritePos; }

                //write: scrivono il dato e muovono il cursore. Ritornano false se non e' stato
                //possibile scrivere
        bool    write (const void *src, u32 howManyByte)                { return priv_writeAt    (cursor, src, howManyByte, true); }
        bool    writeU8  (u8 data)                                      { return priv_writeU8At  (cursor, data, true); }
        bool    writeBool(bool data)                                    { const u8 b = data ? 1: 0; return priv_writeU8At (cursor, b, true); }
        bool    writeU16 (u16 data)                                     { return priv_writeU16At (cursor, data, true); }
        bool    writeU32 (u32 data)                                     { return priv_writeU32At (cursor, data, true); }
        bool    writeU64 (u64 data)                                     { return priv_writeU64At (cursor, data, true); }
		bool    writeF32 (f32 data)                                     { return priv_writeF32At (cursor, data, true); }

				//avanza il cursore di <howManyByte> ridimensionando il buffer se necessario e ritorna il pt
				//alla zona di memoria appena riservata (ci puoi memcpyare dentro)
		u8*		reserveSpace (u32 howManyByte);

                //write: scrivono il dato alla posizione <offset> e NON muovono il cursore. Ritornano false se non e' stato
                //possibile scrivere
        bool    writeAt (u32 offset, const void *src, u32 howManyByte)  { return priv_writeAt    (offset, src, howManyByte, false); }
        bool    writeU8At  (u32 offset, u8 data)                        { return priv_writeU8At  (offset, data, false); }
        bool    writeBoolAt(bool data)                                  { const u8 b = data ? 1: 0; return priv_writeU8At (cursor, b, true); }
        bool    writeU16At (u32 offset, u16 data)                       { return priv_writeU16At (offset, data, false); }
        bool    writeU32At (u32 offset, u32 data)                       { return priv_writeU32At (offset, data, false); }
        bool    writeU64At (u32 offset, u64 data)                       { return priv_writeU64At (offset, data, false); }
		bool    writeF32At (u32 offset, f32 data)                       { return priv_writeF32At (cursor, data, false); }

                //inseriscono degli 0 fino a che <cursor> non diventa un multiplo di 4 o 8
        bool    writePadUntilMultiplo4();
        bool    writePadUntilMultiplo8();

                //read: avanzano il cursore
                //vedi BufferReader

    protected:
                BufferW_interface()                                               { }
                ~BufferW_interface()                                              { }

        void    prot_setup (void *buffer, u32 sizeof_buffer, eEndianess endianess = eEndianess::big);

        u32     prot_tell() const                                            { return cursor; }        
        bool    prot_advanceCursor (i32 howManyByte)                         { return prot_moveCursorTo ( (u32) ((i32)cursor + howManyByte) ); }
        bool    prot_moveCursorTo (u32 absOffset);

        virtual bool    virt_growUpTo(u32 newSize, u8 **out_newBuffer, u32 *out_newBufferSize) = 0;

    private:
        bool    priv_writeAt    (u32 offset, const void *src, u32 howManyByte, bool bMoveCursor);
        bool    priv_writeU8At  (u32 offset, u8 data, bool bMoveCursor);
        bool    priv_writeU16At (u32 offset, u16 data, bool bMoveCursor);
        bool    priv_writeU32At (u32 offset, u32 data, bool bMoveCursor);
        bool    priv_writeU64At (u32 offset, u64 data, bool bMoveCursor);
		bool    priv_writeF32At (u32 offset, f32 data, bool bMoveCursor)		{ return priv_writeAt (offset, &data, sizeof(f32), bMoveCursor);}

    private:
        u32     maxWritePos;
    };


    /**
     * @brief BufferW_fixedSize
     * 
     * Utilizza un generico buffer immutabile ed espone metodi per la lettura/scrittura
     */
    class BufferW_fixedSize : public BufferW_interface
    {
    public:
                BufferW_fixedSize()                                     { setup (NULL,0); }
                ~BufferW_fixedSize()                                    { }

        void    setup (void *bufferIN, u32 sizeof_buffer, eEndianess endianessIN = eEndianess::big) { BufferW_interface::prot_setup (bufferIN, sizeof_buffer, endianessIN); }

        u32     tell() const                                            { return BufferW_interface::prot_tell(); }        
        bool    advanceCursor (i32 howManyByte)                         { return BufferW_interface::prot_advanceCursor (howManyByte); }
        bool    moveCursorTo (u32 absOffset)                            { return BufferW_interface::prot_moveCursorTo (absOffset); }

    protected:
        bool    virt_growUpTo(u32 newSize, u8 **out_newBuffer, u32 *out_newBufferSize)  { return false; }
    };

    /**
     * @brief BufferW_linear
     * 
     * Utilizza un bufferLinear che puo' crescere, ed espone metodi per la lettura/scrittura
     */
    class BufferW_linear : public BufferW_interface
    {
    public:
                BufferW_linear()                                        { }
                ~BufferW_linear()                                       { unsetup(); }

		void	setupWithBase (void *startingBlock, u32 sizeOfStartingBlock, Allocator *backingallocator, eEndianess endianessIN = eEndianess::big)
        {
            bufLinear.setupWithBase (startingBlock, sizeOfStartingBlock, backingallocator);
            BufferW_interface::prot_setup (bufLinear._getPointer(0), bufLinear.getTotalSizeAllocated(), endianessIN);
        }

		void	setup (Allocator *backingallocator, u32 preallocNumBytes, eEndianess endianessIN = eEndianess::big)
        {
            bufLinear.setup (backingallocator, preallocNumBytes);
            BufferW_interface::prot_setup (bufLinear._getPointer(0), bufLinear.getTotalSizeAllocated(), endianessIN);
        }    

        void    unsetup()                                               { bufLinear.unsetup(); }

        u32     tell() const                                            { return BufferW_interface::prot_tell(); }        
        bool    advanceCursor (i32 howManyByte)                         { return BufferW_interface::prot_advanceCursor (howManyByte); }
        bool    moveCursorTo (u32 absOffset)                            { return BufferW_interface::prot_moveCursorTo (absOffset); }

        const u8*   getPointer (u32 absOffset) const                    { return bufLinear._getPointer(absOffset); }
        
    protected:
        bool    virt_growUpTo(u32 newSize, u8 **out_newBuffer, u32 *out_newBufferSize)
        { 
            bufLinear.growUpTo (newSize);
            *out_newBuffer = bufLinear._getPointer(0);
            *out_newBufferSize = bufLinear.getTotalSizeAllocated();
            return true;
        }

    private:
        BufferLinear bufLinear;
    };    
} //namespace gos



#endif //_gosBufferWriter_h_
