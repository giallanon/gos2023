#include "gosShapeImport.h"
#include "gosShapeImport_glTF.h"

using namespace gos;
using namespace gos::shape;


//*************************************************************
bool shape::importFrom_glTF (const char *filename, const VtxLayout &desiredLayout, gos::Allocator *shapeAllocator, gos::ShapeList &out_shapeList)
{
	glTFImporter importer;
	return importer.importFromFile (filename, desiredLayout, shapeAllocator, out_shapeList);
}