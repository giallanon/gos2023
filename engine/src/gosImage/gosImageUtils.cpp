#include "gosImageUtils.h"


using namespace gos;
using namespace gos::image;

//********************************
bool image::utils::saveBufferRGBAasTGA (const void *bufferRGBA, u32 dimx, u32 dimy, const char *filename)
{
    gos::File f;
    if (!fs::fileOpenForW (&f, filename))
		return false;

	//preparo il tga
	const u32 imgSizeInByte = dimx * dimy * 4;

	u8 *tga = GOSALLOC_SCRAPT(u8*, 18 + imgSizeInByte);
	
	//header
	memset (tga, 0, 18);
	tga[2]	= 2;	/* image type = uncompressed RGB */
	tga[12] = (char) (dimx & 0x00FF);
	tga[13] = (char) ((dimx & 0xFF00) >> 8);
	tga[14] = (char) (dimy & 0x00FF);
	tga[15] = (char) ((dimy & 0xFF00) >> 8);
	tga[16] = 32;
	tga[17] = 0x20;	/* Top-down, non-interlaced */

    const u8 *_bufferRGBA = reinterpret_cast<const u8*>(bufferRGBA);

	u32 ctSRC = 0;
	u32 ctDST = 18;
	while (ctSRC < imgSizeInByte)
	{
		u8  r = _bufferRGBA[ctSRC++];
		u8  g = _bufferRGBA[ctSRC++];
		u8  b = _bufferRGBA[ctSRC++];
		u8  a = _bufferRGBA[ctSRC++];

		tga[ctDST++] = b;
		tga[ctDST++] = g;
		tga[ctDST++] = r;
		tga[ctDST++] = a;
	}
	
	fs::fileWrite (f, tga, 18 + imgSizeInByte);
	fs::fileClose(f);

	GOSFREE_SCRAP(tga);
	return true;
}