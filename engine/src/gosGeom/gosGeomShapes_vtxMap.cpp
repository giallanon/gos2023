#include "gosGeomShapes.h"
#include "../gos/gos.h"

using namespace gos;
using namespace gos::shape;


//*************************************************************
VtxMap& VtxMap::begin ()
{
	n = 0;
	return *this;
}

//*************************************************************
VtxMap&	VtxMap::add (u32 offset, eVtxMapSemantic semantic, u8 index, eVtxMapType type, u8 numElem)
{
	if (n >= NUM_MAX_ELEM)
	{
		n = 0xff;
		return *this;
	}
	assert (offset < 0xffff);
	assert (static_cast<int>(semantic) < 16);
	assert (index < 16);
	assert (static_cast<int>(type) < 16);
	assert (numElem < 16);

	elemList[n] = offset << 16;

	elemList[n] |= ((static_cast<u32>(semantic) & 0x0f) << 12);
	elemList[n] |= ((static_cast<u32>(index) & 0x0f) << 8);
	elemList[n] |= ((static_cast<u32>(type) & 0x0f) << 4);
	elemList[n] |= (static_cast<u32>(numElem) & 0x0f);
	n++;
	return *this;
}

//*************************************************************
bool VtxMap::end()
{
	if (0xff == n)
	{
		logger::err ("VtxMap::end() => too many elements");
		return false;
	}
	return true;
}

//*************************************************************
bool VtxMap::find (eVtxMapSemantic semantic, u8 index, eVtxMapType type, u8 numElem, u32 *out_offset) const
{
	assert (NULL != out_offset);
	u32 key = ((static_cast<u32>(semantic) & 0x0f) << 12);
	key |= ((static_cast<u32>(index) & 0x0f) << 8);
	key |= ((static_cast<u32>(type) & 0x0f) << 4);
	key |= (static_cast<u32>(numElem) & 0x0f);

	for (u32 i=0; i<numElem; i++)
	{
		if ((elemList[i] & 0x0000FFFF) == key)
		{
			*out_offset = (elemList[i] >> 16);
			return true;
		}
	}
	return false;
}