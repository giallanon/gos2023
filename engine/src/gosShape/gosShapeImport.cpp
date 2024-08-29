#include "gosShapeImport.h"
#include "gosShapeImport_Collada.h"

using namespace gos;
using namespace gos::shape;


//*************************************************************
bool shape::importFromCollada (const char *filename, gos::Allocator *allocator, Shape *out)
{
	ColladaImporter importer;
	return importer.importFromFile (filename, allocator, out);
}

