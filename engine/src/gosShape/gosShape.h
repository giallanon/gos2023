#ifndef _gosShape_h_
#define _gosShape_h_
#include "gosShapeVtxLayout.h"
#include "../gos/gos.h"


namespace gos
{ 
	namespace shape
	{
		const char*		enumToString (eVtxLayoutSemantic e);

		
		/*============================================================================== 
		 *
		 *	VtxLayout
		 *
		 *==============================================================================*/
		bool 	areEqual (const VtxLayout &a, const VtxLayout &b);
		void	clone (const VtxLayout &src, VtxLayout *out_dst);
		u32 	calcSizeOfAVertex (const VtxLayout &a);
		
				//serialize
				//se [buffer] == NULL ritorna il num di byte necessari alla serializzazione
				//se [buffer] != NULL ritorna 0 in caso di errore oppure il num di byte memcpyati in [buffer]
		u32 	serialize (const VtxLayout &a, u8 *buffer, u32 sizeof_buffer);

				//ritorna 0 in caso di errore
				//altrimenti ritorna il num di byte consumati per la deserializzazione
		u32 	deserialize (const u8 *buffer, u32 sizeof_buffer, VtxLayout *out);	

		/*============================================================================== 
		 *
		 *	Shape
		 *
		 *==============================================================================*/
		
		/**
		 * @brief 	inizializza una shape 
		 * 			Alloca il numero di vtx e idx indicati rispettando @vtxLayout
		 */
		bool	shapeAlloc (gos::Allocator *allocator, const VtxLayout &vtxLayout, u32 numVtx, u32 numIdx, Shape *out_shape);
		void	shapeFree (gos::Allocator *allocator, Shape *shape);

				//se [buffer] == NULL ritorna il num di byte necessari alla serializzazione
				//se [buffer] != NULL ritorna 0 in caso di errore oppure il num di byte memcpyati in [buffer]
		u32 	serialize (const Shape *shape, u8 *buffer, u32 sizeof_buffer);

				//ritorna 0 in caso di errore
				//altrimenti ritorna il num di byte consumati per la deserializzazione.
				//[allocator] e' utilizzato per allocare vtx/idx buffer di [out]
		u32 	deserialize (const u8 *buffer, u32 sizeof_buffer, gos::Allocator *allocator, Shape *out);	


		bool	shapeLoad (const char *filename, gos::Allocator *allocator, Shape *out);
		bool 	shapeSave (const char *filename, const Shape *shape);
		bool 	shapeSave (gos::File &hFile, const Shape *shape);
		

		void	debug_shapePrint (const Shape *shape);

		void 	shapeCalcAABB (const Shape *shape, vec3f *out_min, vec3f *out_max);

		void 	shapeRightHandedToLeftHanded (Shape *shape);
		void 	shapeTranslate (Shape *shape, const vec3f &tr);
		void 	shapeTransformPos (Shape *shape, const mat4x4f &mat);
		void 	shapeTransformPos (Shape *shape, const mat3x3f &mat);
		
		void 	shapeRotateNormals (Shape *shape, const mat3x3f &mat);
		void 	shapeRotateNormals (Shape *shape, const Quat &quat);

	} //namespace shape
 } //namespace gos

#endif //_gosShape_h_