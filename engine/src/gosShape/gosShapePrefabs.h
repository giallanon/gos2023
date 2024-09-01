#ifndef _gosShapePrefabs_h_
#define _gosShapePrefabs_h_
#include "gosShapeVtxArrayWriter.h"
#include "../gos/gos.h"

namespace gos
{ 
	namespace shape
	{
		//buildCube24()
		// Crea una cubo con 4 vtx separati per ogni faccia (24 vtx in totale) e 6 idx per faccia (36idx in totale)
		// In base a come e' stato definito [shape->vtxLayout], filla "pos" e/o "norm" e/o "texCoord"
		bool 	buildCube24 (const vec3f &center, const vec3f &size, const VtxLayout &vtxLayout, gos::Allocator *shapeAllocator, Shape *out_shape);

		//buildSphere()
		//Crea la circonferenza di base attorno a [center] e poi [numHalfStack] circonferenze verso l'altro e altrettante verso il basso
		// In base a come e' stato definito [shape->vtxLayout], filla "pos" e/o "norm"
		bool 	buildSphere (const vec3f &center, const vec3f &radius, u32 numPointPerCirconferenza, u32 numHalfStack, const VtxLayout &vtxLayout, gos::Allocator *shapeAllocator, Shape *out_shape);
		
		//buildCylinder
		//La base e' centrata in [center] e sale verso l'alto. Crea [numStack] circonferenze (compresa quella di base)
		// In base a come e' stato definito [shape->vtxLayout], filla "pos" e/o "norm"
		bool 	buildCylinder (const vec3f &center, f32 radius, f32 height, u32 numPointPerCirconferenza, u32 numStack, bool bCloseTop, bool bCloseBottom, const VtxLayout &vtxLayout, gos::Allocator *shapeAllocator, Shape *out_shape);

	} //namespace shape
 } //namespace gos

#endif //_gosShapePrefabs_h_