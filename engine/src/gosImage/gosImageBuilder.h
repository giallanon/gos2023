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
		 * classe di comodo da utilizzarsi per la creazione di gos::Image
		 */
		class Builder
		{
		public:
			enum class eFilter : u8
			{
				none = 0,
				sRGB_to_RGB
			};

		public:
						Builder();
						~Builder();

			Builder&	begin (gos::Allocator *allocator, Image *out_img);
			
			Builder&	beginTexture2D (eImageFormat format, u16 width, u16 height, u8 nMipMap);
			Builder&		setMipMapDataMemory (u8 mipMapNum_0toN, const void *imgData, u32 sizeOfImgData, eFilter filter);
			Builder&		setMipMapDataFromFile (u8 mipMapNum_0toN, const char *filename, eFilter filter);
			Builder&	endTexture2D();

			/**
			 * @brief accetta file di tipo TGA, BMP, JPG, PNG
			 * Al momento NON crea nessuna mipmap
			 */
			Builder&	buildTexture2DFromFile (eImageFormat format, const char *fileName, eFilter filter);


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