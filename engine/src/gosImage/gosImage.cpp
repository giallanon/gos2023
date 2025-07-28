#include "gosImage.h"
#include "loader/stb_image.h"
#include "../gos/gos.h"

using namespace gos;

//********************************************************** 
bool image::isFormatWithDepth (const eImageFormat fmt)
{
	return (static_cast<u8>(fmt) >= 0xE0 && static_cast<u8>(fmt)<=0xEF);
}

//********************************************************** 
bool image::isFormatWithStencil (const eImageFormat fmt)
{
	return (static_cast<u8>(fmt) >= 0xEA && static_cast<u8>(fmt)<=0xEF);
}

//***********************************************
u32 image::calcSurfaceSize (u16 width, u16 height, eImageFormat fmt, u8 mipMapNum_0toN)
{
	while (mipMapNum_0toN--)
	{
		width>>=1;
		height>>=1;
	}
	return width * height * image::getFormatSize(fmt);
}

//********************************************************** 
u16 image::getFormatSize (const eImageFormat fmt)
{
    switch (fmt)
    {
    default:
        DBGBREAK;
        return 0;

    case eImageFormat::U8_RGBA_sRGB: return 4;
    case eImageFormat::U8_RGBA: return 3;
    case eImageFormat::U8_RGB: return 3;
    case eImageFormat::U8_R: return 1;

    case eImageFormat::U16_RGBA: return sizeof(u16)*4;
    case eImageFormat::U16_RGB: return sizeof(u16)*3;
    case eImageFormat::U16_R: return sizeof(u16);

    case eImageFormat::U32_RGBA: return sizeof(u32)*4;
    case eImageFormat::U32_RGB: return sizeof(u32)*3;
    case eImageFormat::U32_R: return sizeof(u32);

    case eImageFormat::F32_RGBA: return sizeof(f32)*4;
    case eImageFormat::F32_RGB: return sizeof(f32)*3;
    case eImageFormat::F32_R: return sizeof(f32);
    }
}

//***********************************************
bool image::load (gos::Allocator *allocator, const char *filePathAndName, Image *out)
{
	u32 size = 0;
	u8 *buffer = fs::fileLoadInMemory (allocator, filePathAndName, &size);
	if (NULL == buffer)
	{
		gos::logger::err ("image::load() => file not found [%s]\n", filePathAndName);
		return false;
	}

	out->p = buffer;
	const image::sImageHeader *header = image::getInfo(*out);
	if (header->signature == GOSIMAGE__IMAGE_SIGNATURE)
		return true;

	gos::logger::err ("image::load() => invalid signature for file [%s]\n", filePathAndName);
	out->p = NULL;
	GOSFREE(allocator, buffer);
	return false;
}

//***********************************************
bool image::save (const Image &img, const char* filePathAndName)
{
	gos::File f;
	if (!fs::fileOpenForW (&f, filePathAndName))
	{
		gos::logger::err ("image::save() => unable to open for write [%s]\n", filePathAndName);
		return false;
	}

	const image::sImageHeader *header = image::getInfo(img);
	fs::fileWrite (f, img.p, header->totalSizeOfImage);
	fs::fileClose(f);
	return true;
}

//***********************************************
void image::free (gos::Allocator *allocator, Image &img)
{
	GOSFREE(allocator, img.p);
	img.p = NULL;
}


//***********************************************
const image::sImageHeader* image::getInfo (const Image &img)
{
	const image::sImageHeader *header = static_cast<const image::sImageHeader*>(img.p);
	return header;
}


//***********************************************
static u32 image_getAddrOfTextureHeader (const Image &img, u8 textureNum_0toN)
{
	const image::sImageHeader *header = image::getInfo(img);
	if (textureNum_0toN >= header->numTexture)
		return u32MAX;


	const u8 *p = static_cast<const u8*>(img.p);
	u32 addr = 16;
	const image::sTextureHeader *tex = reinterpret_cast<const image::sTextureHeader*> (&p[addr]);
	
	while (textureNum_0toN--)
	{
		addr = tex->absAddrOfNextTexture;
		tex = reinterpret_cast<const image::sTextureHeader*>(&p[addr]);
	}

	return addr;
}

//***********************************************
const image::sTextureHeader* image::getTextureInfo (const Image &img, u8 textureNum_0toN)
{
	const u32 addr = image_getAddrOfTextureHeader(img, textureNum_0toN);
	if (u32MAX == addr)
	{
		DBGBREAK;
		return NULL;
	}

	const u8 *p = (const u8*)img.p;
	return reinterpret_cast<const image::sTextureHeader*> (&p[addr]);
}

//***********************************************
bool image::getTextureData (const Image &img, u8 textureNum_0toN, u8 mipMapNum_0toN, image::sTextureData *out)
{
	u32 addr = image_getAddrOfTextureHeader (img, textureNum_0toN);
	if (u32MAX == addr)
	{
		DBGBREAK;
		return false;
	}

	u8 *p = reinterpret_cast<u8*>(img.p);
	const image::sTextureHeader *header = reinterpret_cast<const image::sTextureHeader*> (&p[addr]);
	addr += sizeof(image::sTextureHeader);

	out->uncompressed_width = header->width;
	out->uncompressed_height = header->height;

	while (mipMapNum_0toN--)
	{
		addr += image::calcSurfaceSize (out->uncompressed_width, out->uncompressed_height, header->fmt);
		out->uncompressed_width >>= 1;
		out->uncompressed_height >>= 1;
	}
	
	out->textureData = &p[addr];
	out->compressed_sizeInByteOfTextureData = image::calcSurfaceSize (out->uncompressed_width, out->uncompressed_height, header->fmt);


	out->compressed_height = out->uncompressed_height;
	out->compressed_sizeOfARowInBytes = out->uncompressed_width * image::getFormatSize(header->fmt);
	
	return true;
}
