#ifndef _gosUtils_h_
#define _gosUtils_h_
#include "gosEnumAndDefine.h"

namespace gos
{
    namespace utils
    {
        u8	    getFormatSize (eDataFormat f);



        /* setta/resetta il bit in posizione [pos].
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


        /* setta/resetta il byte posizione [pos].
         * [pos] == 0  => LSB
        */        
        void    byteSET (u32 *dst, u8 value, u32 pos);
        u8      byteGET (const u32 *dst, u32 pos);        
    }

} //namespace gos

#endif //_gosUtils_h_