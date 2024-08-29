#include "gosShapeVtxArrayWriter.h"
#include "gosShape.h"
#include "../gos/gos.h"


using namespace gos;
using namespace gos::shape;


//*********************************************
void VtxArrayWriter::setup (Shape *shapeIN)
{
	setup ( &shapeIN->vtxLayout, 
			shapeIN->vtxBuffer, 
			gos::shape::calcSizeOfAVertex(shapeIN->vtxLayout),
			shapeIN->numVtx, 
			shapeIN->idxBuffer,
			shapeIN->numIdx);
}


