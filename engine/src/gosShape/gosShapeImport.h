#ifndef _ggosShapeImport_h_
#define _ggosShapeImport_h_
#include "gosShapeEnumAndDefine.h"
#include "../gos/gos.h"


namespace gos
{ 
	namespace shape
	{
		bool 	importFrom_dae (const char *filename, const VtxLayout &desiredLayout, gos::Allocator *shapeAllocator, gos::ShapeList &out_shapeList);
		
		bool 	importFrom_glTF (const char *filename, const VtxLayout &desiredLayout, gos::Allocator *shapeAllocator, gos::ShapeList &out_shapeList);

	} //namespace shape
 } //namespace gos

#endif //_ggosShapeImport_h_