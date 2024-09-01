#ifndef _ggosShapeImport_h_
#define _ggosShapeImport_h_
#include "gosShapeEnumAndDefine.h"
#include "../gos/gos.h"
#include "../gos/gosFastArray.h"

namespace gos
{ 
	namespace shape
	{
		bool 	importFrom_dae (const char *filename, const VtxLayout &desiredLayout, gos::Allocator *shapeAllocator, FastArray<Shape> &out_shapeList);
		
		bool 	importFrom_glTF (const char *filename, const VtxLayout &desiredLayout, gos::Allocator *shapeAllocator, FastArray<Shape> &out_shapeList);

	} //namespace shape
 } //namespace gos

#endif //_ggosShapeImport_h_