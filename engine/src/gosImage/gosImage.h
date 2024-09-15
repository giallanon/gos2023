#ifndef _gosImage_h_
#define _gosImage_h_
#include "gosImageEnumAndDefine.h"
#include "../gos/gos.h"


namespace gos
{ 
	namespace image
	{
		u16 	getFormatSize (const eImageFormat fmt);
		u32		calcSurfaceSize (u16 width, u16 height, eImageFormat fmt, u8 mipMapNum_0toN=0);


		/*=======================================================================================
		*	Image e' un contenitore di texture, ne puo' contenere N e ognuna di queste pu� essere di tipo diverso
		*	Ogni texture ha un eImageFormat, un numero di mipMap (da 1 a n) e una width/height
		*	
		* Image � un generico buffer in memoria che deve essere formattato come segue:
		*		start	size	value
		*		0		2		signature	(aka GOSIMAGE_IMAGE_SIGNATURE)
		*		2		1		numTexture		//numero di texture contenute in questa image. Ogni texture poi a sua volta pu� essere composta da n MipMap
		*		3		1		pad1
				
				4		4		totalSizeOfImage	//numero tot di bytes allocati da questa image e tutte le sue texture
				
				8		4		pad3 per usi futuri
				
				12		4		pad4 per usi futuri

		------------------ inizio header di una generica texture --------------------------
				0		1		eImageFormat
				1		1		nMipMap			//minimo 1. Indica quante "surface" sono contenute da questa immagine in particolare
				2		2		pad

				4		2		width
				6		2		height
				
				8		4		sizeInByteOfTextureData			//dimensione totale dei texture data, comprensivi di tutte le mip map
				12		4		addr of next texture header		//se "image" e' composta da N texture, qui c'e' l'indirizzo assoluto dell' header della texture seguente
				16		...		texture data
						...		eventuale pad per fare in modo che il prossimo "texture header" inizi su un indirizzo multiplo di 4

			------------------ inizio header di una generica texture --------------------------
			..
			..

		*/

								/**
								 * @brief questa load() accetta solo il formato .gosimage
								 * Per caricare formati diversi, vedi Builder::addFromFile
								 */
		bool					load (gos::Allocator *allocator, const char *filePathAndName, Image *out);
		bool					save (const Image &img, const char* filePathAndName);
		void					free (gos::Allocator *allocator, Image &img);

		const sImageHeader*		getInfo (const Image &img);
		const sTextureHeader*	getTextureInfo (const Image &img, u8 textureNum_0toN); 
		bool					getTextureData (const Image &img, u8 textureNum_0toN, u8 mipMapNum_0toN, sTextureData *out);
		
	} //namespace image
 } //namespace gos

#endif //_gosImage_h_