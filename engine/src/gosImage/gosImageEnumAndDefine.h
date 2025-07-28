#ifndef _gosImageEnumAndDefine_h_
#define _gosImageEnumAndDefine_h_
#include "../gos/gosEnumAndDefine.h"


#define GOSIMAGE__IMAGE_SIGNATURE	0xc012


namespace gos
{
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