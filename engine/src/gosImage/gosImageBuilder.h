#ifndef _gosImageBuilder_h_
#define _gosImageBuilder_h_
#include "gosImage.h"
#include "../gos/gosFastArray.h"

namespace gos
{ 
	namespace image
	{
		/**
		 * @brief Builder
		 * classe di comodo da utilizzarsi per la creazione di image::Image
		 */
		class Builder
		{
		public:
						Builder();
						~Builder();

			Builder&	begin (gos::Allocator *allocator, Image *out_img);
			
			Builder&	beginTexture (eImageFormat format, u16 width, u16 height, u8 nMipMap);
			Builder&		setMipMapDataMemory (u8 mipMapNum_0toN, const void *imgData, u32 sizeOfImgData);
			Builder&		setMipMapDataFromFile (u8 mipMapNum_0toN, const char *filename);
			Builder&	endTexture();

			bool		end();

			bool 		anyError() const 															{ return (error!=0); }

		private:
			struct sElem
			{
				image::sTextureHeader	texHeader;
				u8 						*textureData;
			};

		private:
			void 		priv_free();

		private:
			gos::Allocator			*finalImgAllocator;
			Image					*out_img;

			FastArray<sElem>		texList;
			image::sTextureHeader	texHeader;
			u8 						*textureData;
			u8 						error;
		};

	} //namespace image
 } //namespace gos

#endif //_gosImageBuilder_h_