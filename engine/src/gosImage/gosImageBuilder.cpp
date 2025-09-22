#include "gosImageBuilder.h"
#include "loader/stb_image.h"
#include "../gos/gos.h"
#include "../gos/dataTypes/gosColorHDR.h"


using namespace gos;
using namespace gos::image;

//*****************************************************
Builder::Builder()
{ 
	out_img = NULL; 
	textureData = NULL;

	texList.setup (gos::getScrapAllocator(), 16);
}

//*****************************************************
Builder::~Builder()
{
	priv_free();
	texList.unsetup();
}

//*****************************************************
void Builder::priv_free()
{
	if (NULL != textureData)
	{
		GOSFREE(gos::getScrapAllocator(), textureData);
		textureData = NULL;
	}

	for (u32 i=0; i<texList.getNElem(); i++)
	{
		GOSFREE(gos::getScrapAllocator(), texList(i).textureData);
	}
	texList.reset();
}


//*****************************************************
Builder& Builder::begin (gos::Allocator *allocatorIN, Image *out_imgIN)
{
	priv_free();
	
	error = 0;
	out_img = out_imgIN;
	finalImgAllocator = allocatorIN;

	return *this;
}

//************************************************************
Builder& Builder::beginTexture2D (eImageFormat format, u16 width, u16 height, u8 nMipMap)
{
	if (anyError())
		return *this;
	
	if (nMipMap < 1 || width < 1 || height < 1 || NULL != textureData)
	{
		gos::logger::err ("Builder::beginTexture2D => invalid parameter\n");
		error = 1;
		return *this;
	}

	memset (&texHeader, 0, sizeof(image::sTextureHeader));
	texHeader.fmt = format;
	texHeader.width = width;
	texHeader.height = height;
	texHeader.numMipMap = nMipMap;

	//calcolo la dimensione della texture, comprensiva delle sue mip map
	while (nMipMap--)
	{
		texHeader.sizeInByteOfTextureData += image::calcSurfaceSize (width, height, format, 0);
		width>>=1;
		height>>=1;
	}

	textureData = GOSALLOCT(u8*, gos::getScrapAllocator(), texHeader.sizeInByteOfTextureData);
	return *this;
}

//************************************************************
Builder& Builder::setMipMapDataMemory (u8 mipMapNum_0toN, const void *imgData, u32 sizeOfImgData, eFilter filter)
{
	if (anyError())
		return *this;

	if (mipMapNum_0toN >= texHeader.numMipMap)
	{
		gos::logger::err ("Builder::setMipMapDataMemory => mipMapNum_0toN >= texHeader.numMipMap\n");
		error = 2;
		return *this;
	}

	const u32 expectedDataSize = image::calcSurfaceSize (texHeader.width, texHeader.height, texHeader.fmt, mipMapNum_0toN);
	if (expectedDataSize != sizeOfImgData)
	{
		gos::logger::err ("Builder::setMipMapDataMemory => expectedDataSize != sizeOfImgData\n");
		error = 3;
		return *this;
	}

	u32 w = texHeader.width;
	u32 h = texHeader.height;
	u32 offset = 0;
	while (mipMapNum_0toN--)
	{
		offset += image::calcSurfaceSize (w, h, texHeader.fmt, 0);
		w>>=1;
		h>>=1;
	}

	memcpy (&textureData[offset], imgData, sizeOfImgData);


	//filtri
	if (eFilter::sRGB_to_RGB == filter)
	{
		u32 ct = offset;
		for (u32 x = 0; x < texHeader.width; x++)
		{
			for (u32 y = 0; y < texHeader.height; y++)
			{
				u32 startOffset = offset;
				u8 r = textureData[offset++];
				u8 g = textureData[offset++];
				u8 b = textureData[offset++];
				u8 a = textureData[offset++];

				gos::ColorHDR col;
				col.setU8_argb(a, r, g, b);
				col.sRGBToLinear();

				const u32 argb = col.toU32ARGB();

				textureData[startOffset++] = (u8)((argb & 0x00FF0000) >> 16);
				textureData[startOffset++] = (u8)((argb & 0x0000FF00) >> 8);
				textureData[startOffset++] = (u8)((argb & 0x000000FF));
				textureData[startOffset++] = (u8)((argb & 0xFF000000) >> 24);
			}
		}
	}



	return *this;
}

//************************************************************
Builder& Builder::setMipMapDataFromFile (u8 mipMapNum_0toN, const char *filename, eFilter filter)
{
	if (anyError())
		return *this;

	if (mipMapNum_0toN >= texHeader.numMipMap)
	{
		gos::logger::err ("Builder::setMipMapDataFromFile => mipMapNum_0toN >= texHeader.numMipMap\n");
		error = 4;
		return *this;
	}

	u32 fsize;
	u8 *buffer = fs::fileLoadInMemory (gos::getScrapAllocator(), filename, &fsize);
	if (NULL == buffer)
	{
		gos::logger::err ("Builder::setMipMapDataFromFile => file '%s' not found\n", filename);
		error = 5;
		return *this;
	}

	//vediamo se abbiamo un formato TGA, BMP, JPG, PNG
	{
		int width, height, comp;
		u8 *rgba = stbi_load_from_memory (buffer, fsize, &width, &height, &comp, 4);
		if (NULL == rgba)
		{
			gos::logger::err ("Builder::setMipMapDataFromFile => invalid file format '%s'\n");
			error = 6;
		}	
		else
		{
			setMipMapDataMemory (mipMapNum_0toN, rgba, width * height * 4, filter);
			stbi_image_free (rgba);
		}
	}	

	GOSFREE(gos::getScrapAllocator(), buffer);
	return *this;
}

//************************************************************
Builder& Builder::endTexture2D()
{
	if (!anyError())
	{
		const u32 n = texList.getNElem();
		texList[n].texHeader = texHeader;
		texList[n].textureData = textureData;
		textureData = NULL;
	}
	else
	{
		if (NULL != textureData)
		{
			GOSFREE(gos::getScrapAllocator(), textureData);
			textureData = NULL;
		}
	}

	return *this;	
}

//************************************************************
Builder& Builder::buildTexture2DFromFile (eImageFormat format, const char *filename, eFilter filter)
{
	if (anyError())
		return *this;

	u32 fsize;
	u8 *buffer = fs::fileLoadInMemory (gos::getScrapAllocator(), filename, &fsize);
	if (NULL == buffer)
	{
		gos::logger::err ("Builder::buildTexture2DFromFile => file '%s' not found\n", filename);
		error = 5;
		return *this;
	}

	//vediamo se abbiamo un formato TGA, BMP, JPG, PNG
	{
		int width, height, comp;
		u8 *rgba = stbi_load_from_memory (buffer, fsize, &width, &height, &comp, 4);
		if (NULL == rgba)
		{
			gos::logger::err ("Builder::buildTexture2DFromFile => invalid file format '%s'\n");
			error = 6;
		}	
		else
		{
			beginTexture2D (format, width, height, 1);
			setMipMapDataMemory (0, rgba, width * height * 4, filter);
			stbi_image_free (rgba);
		}
	}	

	GOSFREE(gos::getScrapAllocator(), buffer);
	return *this;
}

//************************************************************
bool Builder::end()
{
	if (anyError())
	{
		priv_free();
		return false;
	}

	u32 sizeToAlloc = sizeof(image::sImageHeader);
	for (u32 i=0; i<texList.getNElem(); i++)
	{
		sizeToAlloc += sizeof(image::sTextureHeader);
		sizeToAlloc += texList(i).texHeader.sizeInByteOfTextureData;

		while (sizeToAlloc % 4 != 0)
			sizeToAlloc++;

		texList[i].texHeader.absAddrOfNextTexture = sizeToAlloc;
	}

	//alloco tutta la image
	u8 *p = GOSALLOCT(u8*, finalImgAllocator, sizeToAlloc);
	out_img->p = p;

	//header
	image::sImageHeader *header = reinterpret_cast<image::sImageHeader*>(p);
	memset (header, 0, sizeof(image::sImageHeader));
	header->signature = GOSIMAGE__IMAGE_SIGNATURE;
	header->numTexture = static_cast<u8>(texList.getNElem());
	header->totalSizeOfImage = sizeToAlloc;


	//textures
	u32 offset = sizeof(image::sImageHeader);
	for (u32 i=0; i<texList.getNElem(); i++)
	{
		memcpy (&p[offset], &texList(i).texHeader, sizeof(image::sTextureHeader));
		offset += sizeof(image::sTextureHeader);
		memcpy (&p[offset], texList(i).textureData, texList(i).texHeader.sizeInByteOfTextureData);

		offset = texList(i).texHeader.absAddrOfNextTexture;
	}

	assert (offset == sizeToAlloc);
	priv_free();

	return true;
}