#include "gosShapeVtxLayout.h"
#include "../gos/gos.h"

using namespace gos;
using namespace gos::shape;




//*************************************************************
bool VtxLayoutWriter::end()
{
	if (0xff == vl->numElem)
	{
		logger::err ("VtxLayout::end() => too many elements");
		return false;
	}
	return true;
}
