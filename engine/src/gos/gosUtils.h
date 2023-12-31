#ifndef _gosUtils_h_
#define _gosUtils_h_
#include "gosEnumAndDefine.h"

namespace gos
{
    namespace utils
    {
        u8	    getFormatSize (eDataFormat f);

        /**********************************************************
         * Manipolazione di BIT 
         * 
         * Settano/resettano il bit in posizione [pos].
         * [pos] == 0  => LSBit
        */        
        void    bitZERO  (void *dst, u32 sizeof_dst);
        void    bitSET (void *dst, u32 sizeof_dst, u32 pos);
        void    bitCLEAR (void *dst, u32 sizeof_dst, u32 pos);
        bool    isBitSET (const void *dst, u32 sizeof_dst, u32 pos);

        //versioni ottimizzare per u32
        void    bitZERO  (u32 *dst);
        void    bitSET (u32 *dst, u32 pos);
        void    bitCLEAR (u32 *dst, u32 pos);
        bool    isBitSET (const u32 *dst, u32 pos);

        //versioni ottimizzare per u16
        void    bitZERO  (u16 *dst);
        void    bitSET (u16 *dst, u32 pos);
        void    bitCLEAR (u16 *dst, u32 pos);
        bool    isBitSET (const u16 *dst, u32 pos);

        //versioni ottimizzare per u8
        void    bitZERO  (u8 *dst);
        void    bitSET (u8 *dst, u32 pos);
        void    bitCLEAR (u8 *dst, u32 pos);
        bool    isBitSET (const u8 *dst, u32 pos);

        /**********************************************************
         * Manipolazione di BYTE
         * 
         * setta/resetta il byte posizione [pos].
         * [pos] == 0  => LSB
        */        
        void    byteSET (u32 *dst, u8 value, u32 pos);
        u8      byteGET (const u32 *dst, u32 pos);        



        /**********************************************************
         * Buffer read/write
         * 
         */
		u8	        bufferWriteU64(u8 *buffer, u64 val);
        u8	        bufferWriteU64_LSB_MSB (u8 *buffer, u64 val);
		u8	        bufferWriteI64(u8 *buffer, i64 val);
        u8	        bufferWriteI64_LSB_MSB (u8 *buffer, i64 val);

        inline u8	bufferWriteU32(u8 *buffer, u32 val)			                { buffer[0] = (u8)((val & 0xFF000000) >> 24); buffer[1] = (u8)((val & 0x00FF0000) >> 16); buffer[2] = (u8)((val & 0x0000FF00) >> 8); buffer[3] = (u8)(val & 0x000000FF); return 4; }
        inline u8	bufferWriteU32_LSB_MSB (u8 *buffer, u32 val)                { buffer[3] = (u8)((val & 0xFF000000) >> 24); buffer[2] = (u8)((val & 0x00FF0000) >> 16); buffer[1] = (u8)((val & 0x0000FF00) >> 8); buffer[0] = (u8)(val & 0x000000FF); return 4; }
        inline u8	bufferWriteI32(u8 *buffer, i32 val)			                { buffer[0] = (u8)((val & 0xFF000000) >> 24); buffer[1] = (u8)((val & 0x00FF0000) >> 16); buffer[2] = (u8)((val & 0x0000FF00) >> 8); buffer[3] = (u8)(val & 0x000000FF); return 4; }
        inline u8	bufferWriteI32_LSB_MSB (u8 *buffer, i32 val)                { buffer[3] = (u8)((val & 0xFF000000) >> 24); buffer[2] = (u8)((val & 0x00FF0000) >> 16); buffer[1] = (u8)((val & 0x0000FF00) >> 8); buffer[0] = (u8)(val & 0x000000FF); return 4; }
		
		inline u8	bufferWriteU24(u8 *buffer, u32 val)			                { buffer[0] = (u8)((val & 0x00FF0000) >> 16); buffer[1] = (u8)((val & 0x0000FF00) >> 8); buffer[2] = (u8)(val & 0x000000FF); return 3; }
        inline u8	bufferWriteU24_LSB_MSB (u8 *buffer, u32 val)                { buffer[2] = (u8)((val & 0x00FF0000) >> 16); buffer[1] = (u8)((val & 0x0000FF00) >> 8); buffer[0] = (u8)(val & 0x000000FF); return 3; }

        inline u8	bufferWriteU16(u8 *buffer, u16 val)			                { buffer[0] = (u8)((val & 0xFF00) >> 8); buffer[1] = (u8)(val & 0x00FF); return 2; }
		inline u8	bufferWriteU16_LSB_MSB(u8 *buffer, u16 val)                 { buffer[1] = (u8)((val & 0xFF00) >> 8); buffer[0] = (u8)(val & 0x00FF); return 2; }
        inline u8	bufferWriteI16(u8 *buffer, i16 val)			                { buffer[0] = (u8)((val & 0xFF00) >> 8); buffer[1] = (u8)(val & 0x00FF); return 2; }
		inline u8	bufferWriteI16_LSB_MSB(u8 *buffer, i16 val)                 { buffer[1] = (u8)((val & 0xFF00) >> 8); buffer[0] = (u8)(val & 0x00FF); return 2; }
		
        u8          bufferWriteF32(u8 *buffer, f32 val);


		u64	        bufferReadU64(const u8 *buffer);
        u64	        bufferReadU64_LSB_MSB (const u8 *buffer);
        inline i64	bufferReadI64(const u8 *buffer)				                { return static_cast<i64>(bufferReadU64(buffer)); }
		inline i64	bufferReadI64_LSB_MSB(const u8 *buffer)		                { return static_cast<i64>(bufferReadU64_LSB_MSB(buffer)); }

        inline u32	bufferReadU32(const u8 *buffer)				                { return (((u32)buffer[0]) << 24) | (((u32)buffer[1]) << 16) | (((u32)buffer[2]) << 8) | ((u32)buffer[3]); }
        inline u32	bufferReadU32_LSB_MSB(const u8 *buffer)                     { return (((u32)buffer[3]) << 24) | (((u32)buffer[2]) << 16) | (((u32)buffer[1]) << 8) | ((u32)buffer[0]); }
        inline i32	bufferReadI32(const u8 *buffer)				                { return static_cast<i32>(bufferReadU32(buffer)); }
		inline i32	bufferReadI32_LSB_MSB(const u8 *buffer)		                { return static_cast<i32>(bufferReadU32_LSB_MSB(buffer)); }
        
        inline u32	bufferReadU24(const u8 *buffer)				                { return (((u32)buffer[0]) << 16) | (((u32)buffer[1]) << 8) | ((u32)buffer[2]); }
        inline u32	bufferReadU24_LSB_MSB(const u8 *buffer)                     { return (((u32)buffer[2]) << 16) | (((u32)buffer[1]) << 8) | ((u32)buffer[0]); }

        inline u16	bufferReadU16(const u8 *buffer)				                { return (((u16)buffer[0]) << 8) | ((u16)buffer[1]); }
		inline u16	bufferReadU16_LSB_MSB(const u8 *buffer)		                { return (((u16)buffer[1]) << 8) | ((u16)buffer[0]); }
        inline i16	bufferReadI16(const u8 *buffer)				                { return static_cast<i16>(bufferReadU16(buffer)); }
		inline i16	bufferReadI16_LSB_MSB(const u8 *buffer)		                { return static_cast<i16>(bufferReadU16_LSB_MSB(buffer)); }

        f32	        bufferReadF32 (const u8 *buffer);        
    }

} //namespace gos

#endif //_gosUtils_h_