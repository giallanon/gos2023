/* adapted from glib */
#include <stdlib.h>
#include <string.h>
#include "../gosEnumAndDefine.h"
#include "../gosUtils.h"

static const char base64_alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const unsigned char mime_base64_rank[256] = {
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255, 62,255,255,255, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61,255,255,255,  0,255,255,
    255,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,255,255,255,255,255,
    255, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
};


//*******************************************************************
size_t gos::utils::base64_howManyBytesNeededForEncoding (size_t sizeInBytesOfBufferToEncode)
{
    /* ogni byte viene usato per rappresentare 6bit
     * Il risultato finale è da arrotondare al multiplo di 4 più grande (se non è già da se un multiplo di 4)
     * Infine, si aggiunge 1 per lo 0x00 che viene appeso a fine string
     */

    size_t nBytes = (sizeInBytesOfBufferToEncode * 8) / 6;
    size_t m = (nBytes % 4);
    if (m > 0)
        nBytes += (4-m);
    return nBytes;
}

//*******************************************************************
u32 base64_encode_step (const unsigned char *in, size_t length, int break_lines, char *out, int *state, int *save)
{
    if(length <= 0)
        return 0;


    char *outptr = out;
    const unsigned char *inptr = in;

    if(length + ((char *)save)[0] > 2)
    {
        const unsigned char *inend = in + length - 2;
        int c1, c2, c3;
        int already;

        already = *state;

        switch (((char *) save) [0])
        {
            case 1:
                c1 = ((unsigned char *) save) [1];
                goto skip1;
            case 2:
                c1 = ((unsigned char *) save) [1];
                c2 = ((unsigned char *) save) [2];
                goto skip2;
        }

        /*
        * yes, we jump into the loop, no i'm not going to change it,
        * it's beautiful!
        */
        while(inptr < inend)
        {
            c1 = *inptr++;
        skip1:
            c2 = *inptr++;
        skip2:
            c3 = *inptr++;
            *outptr++ = base64_alphabet [ c1 >> 2 ];
            *outptr++ = base64_alphabet [ c2 >> 4 |
                                        ((c1&0x3) << 4) ];
            *outptr++ = base64_alphabet [ ((c2 &0x0f) << 2) |
                                        (c3 >> 6) ];
            *outptr++ = base64_alphabet [ c3 & 0x3f ];
            /* this is a bit ugly ... */
            if (break_lines && (++already) >= 19)
            {
                *outptr++ = '\n';
                already = 0;
            }
        }

        ((char *)save)[0] = 0;
        length = 2 - (inptr - inend);
        *state = already;
    }

    if(length > 0)
    {
        char *saveout;

        /* points to the slot for the next char to save */
        saveout = & (((char *)save)[1]) + ((char *)save)[0];

        /* len can only be 0 1 or 2 */
        switch(length)
        {
            case 2: *saveout++ = *inptr++;
            case 1: *saveout++ = *inptr++;
        }
            ((char *)save)[0] += (char)length;
    }

    return (u32)(outptr - out);
}

/**
 * g_base64_encode_close:
 * @break_lines: whether to break long lines
 * @out: (out) (array) (element-type guint8): pointer to destination buffer
 * @state: (inout): Saved state from g_base64_encode_step()
 * @save: (inout): Saved state from g_base64_encode_step()
 *
 * Flush the status from a sequence of calls to g_base64_encode_step().
 *
 * The output buffer must be large enough to fit all the data that will
 * be written to it. It will need up to 4 bytes, or up to 5 bytes if
 * line-breaking is enabled.
 *
 * Return value: The number of bytes of output that was written
 *
 * Since: 2.12
 */
u32 base64_encode_close (int break_lines, char *out, int *state, int *save)
{
    int c1, c2;
    char *outptr = out;

    c1 = ((unsigned char *) save) [1];
    c2 = ((unsigned char *) save) [2];

    switch (((char *) save) [0])
    {
    case 2:
      outptr [2] = base64_alphabet[ ( (c2 &0x0f) << 2 ) ];
      /* g_assert (outptr [2] != 0); */
      goto skip;
    case 1:
      outptr[2] = '=';
    skip:
      outptr [0] = base64_alphabet [ c1 >> 2 ];
      outptr [1] = base64_alphabet [ c2 >> 4 | ( (c1&0x3) << 4 )];
      outptr [3] = '=';
      outptr += 4;
      break;
    }
    if (break_lines)
    *outptr++ = '\n';

    *save = 0;
    *state = 0;

	return (u32)(outptr - out);
}


//*****************************************************************************************
bool gos::utils::base64_encode (void *outIN, u32 sizeOfOutInBytes, const void *in, u32 sizeInBytesOfIn)
{
    char *out = (char*)outIN;
    if (sizeOfOutInBytes < base64_howManyBytesNeededForEncoding(sizeInBytesOfIn))
        return false;

    const u32 MAX_SIZE_IN = 512;
    int state = 0, outlen;
    int save = 0;

    /* We can use a smaller limit here, since we know the saved state is 0,
       +1 is needed for trailing \0, also check for unlikely integer overflow */
    if (sizeInBytesOfIn >= ((MAX_SIZE_IN - 1) / 4 - 1) * 3)
        return false;
    outlen = base64_encode_step ((const unsigned char*)in, sizeInBytesOfIn, 0, (char*)out, &state, &save);
    outlen += base64_encode_close (0, &out[outlen], &state, &save);
    out[outlen] = '\0';
    return true;
}



//*****************************************************************************************
u32 base64_decode_step(const char *base64, u32 length, unsigned char *binary, int *state, unsigned int *save)
{
    const unsigned char *inptr;
    unsigned char *outptr;
    unsigned char *inend;
    unsigned char c, rank;
    unsigned char last[2];
    unsigned int v;
    int i;

    if(length <= 0)
    {
        return 0;
    }

    inend = (unsigned char *)base64 + length;
    outptr = binary;

    /* convert 4 base64 bytes to 3 normal bytes */
    v=*save;
    i=*state;
    inptr = (unsigned char *)base64;
    last[0] = last[1] = 0;
    while(inptr < inend)
    {
        c = *inptr++;
        rank = mime_base64_rank[c];
        if(rank != 0xff)
        {
            last[1] = last[0];
            last[0] = c;
            v = (v<<6) | rank;
            i++;
            if(i==4)
            {
                *outptr++ = (unsigned char)(v >> 16);
                if (last[1] != '=')
                    *outptr++ = (unsigned char)(v >> 8);
                if (last[0] != '=')
                    *outptr++ = (unsigned char)v;
                i=0;
            }
        }
    }

    *save = v;
    *state = i;

	return (u32)(outptr - binary);
}

//*****************************************************************************************
int gos::utils::base64_decode(unsigned char *binary, u32 *binary_length, const char *base64, u32 base64_length)
{
    int state = 0;
    unsigned int save = 0;

    *binary_length = base64_decode_step(base64, base64_length, binary, &state, &save);
    return 0;
}
