#ifndef _gosShape_h_
#define _gosShape_h_
#include "gosShapeVtxLayout.h"
#include "../gos/gos.h"


namespace gos
{ 
	namespace shape
	{
		const char*		enumToString (eVtxLayoutSemantic e);
		const char*		enumToString (eVtxLayoutFormat e);

		//======================================= VtxLayoutFormat
		u32 	getSizeInByte (eVtxLayoutFormat fmt);


		//======================================= VtxLayout
		bool 	areEqual (const VtxLayout &a, const VtxLayout &b);
		u32 	calcSizeOfAVertex (const VtxLayout &a);
		
				//serialize
				//se [buffer] == NULL; ritorna il num di byte necessari alla serializzazione
		u32 	serialize (const VtxLayout &a, u8 *buffer, u32 sizeof_buffer);

		u32 	deserialize (VtxLayout *out, const u8 *buffer, u32 sizeof_buffer);	

		//======================================= Shape

				//ShapeAlloc()
				//[in_out] deve avere gia' un valido VtxFormat.
				//Questa fn alloca il numero di vtx e idx indicati rispettando il VtxFormat di [in_out]
		bool	shapeAlloc (gos::Allocator *allocator, u32 numVtx, u32 numIdx, Shape *in_out);
		void	shapeFree (gos::Allocator *allocator, Shape *shape);

		bool	shapeLoad (const char *filename, gos::Allocator *allocator, Shape *out);
		u32		shapeLoadFromMemory (const u8 *mem, u32 sizeof_mem, gos::Allocator *allocator, Shape *out);

		bool 	shapeSave (const char *filename, const Shape *shape);
		bool 	shapeSave (gos::File &hFile, const Shape *shape);

		void	debug_shapePrint (const Shape *shape);


	} //namespace shape
 } //namespace gos

#endif //_gosShape_h_