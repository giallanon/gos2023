#ifndef _gosImageUtils_h_
#define _gosImageUtils_h_
#include "gosImage.h"


namespace gos
{ 
    namespace image
    {
        namespace utils
        {
            bool    saveBufferRGBAasTGA (const void *bufferRGBA, u32 dimx, u32 dimy, const char *filename);
        } //namespace utils
    } //namespace image
} //namespace gos

#endif //_gosImageUtils_h_
