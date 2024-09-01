#include "gosShapeVtxLayout.h"
#include "gosShape.h"
#include "../gos/gos.h"

using namespace gos;
using namespace gos::shape;




//*************************************************************
bool VtxLayoutWriter::end()
{
	if (0xff == vl->numElem)
	{
		logger::err ("VtxLayout::end() => too many elements");
		vl->numElem = 0;
		return false;
	}
	if (0 ==  vl->numElem)
	{
		logger::err ("VtxLayout::end() => need at least 1 element");
		return false;
	}

	//gli elementi devono essere in ordine di offset
	u32 ct = getOffset(0) + shape::getSizeInByte(getFormat(0));
	for (u32 i=1; i<getNumElem(); i++)
	{
		if (getOffset(i) < ct)
		{
			logger::err ("VtxLayout::end() => invalid offset for element %d\n");
			vl->numElem = 0;
			return false;
		}

		ct = getOffset(i) + shape::getSizeInByte(getFormat(i));
	}



	return true;
}
