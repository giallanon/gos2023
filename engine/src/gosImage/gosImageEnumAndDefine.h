#ifndef _gosImageEnumAndDefine_h_
#define _gosImageEnumAndDefine_h_
#include "../gos/gosEnumAndDefine.h"


#define GOSIMAGE__IMAGE_SIGNATURE	0xc012


namespace gos
{
	enum class eImageFormat : u8
	{
		U8_RGBA_sRGB	= 0,
		U8_RGBA			= 1,
		U8_RGB			= 2,
		U8_R			= 3,
		U16_RGBA		= 4,
		U16_RGB			= 5,
		U16_R			= 6,

		U32_RGBA		= 7,
		U32_RGB			= 8,
		U32_R			= 9,

		F32_RGBA		= 10,
		F32_RGB			= 11,
		F32_R			= 12,
		
		DDS_BC3			= 0xf2,
		DDS_BC4			= 0xf3,
		DDS_BC5			= 0xf4
	};

	struct Image
	{
		void	*p;
	};	

	namespace image
	{ 
		struct sImageHeader
		{
			u16		signature;
			u8		numTexture;
			u8		pad1;
			u32		totalSizeOfImage;
			u32		pad3;
			u32		pad4;
		};

		struct sTextureHeader
		{
			eImageFormat	fmt;
			u8				numMipMap;
			u16				pad1;
			u16				width;
			u16				height;
			u32				sizeInByteOfTextureData;
			u32				absAddrOfNextTexture;
		};

		struct sTextureData
		{
			u8				*textureData;
			u16				uncompressed_width;						//dimensione originale della texture non compressa
			u16				uncompressed_height;				

			u32				compressed_sizeInByteOfTextureData;		//dimensione totale in memoria della texture in formato compresso
			u16				compressed_sizeOfARowInBytes;			//dimensione di una "riga" in formato compresso in memoria
			u16				compressed_height;						//numero di "righe" in formato compresso in memoria
			
		};

	} //namespace image
 } //namespace gos

#endif //_gosImageEnumAndDefine_h_