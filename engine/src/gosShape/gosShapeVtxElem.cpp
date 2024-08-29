#include "gosShapeVtxElem.h"
#include "../gos/gos.h"

using namespace gos;


//*************************************************************
u32 shape::VtxElem::define (u32 offset, eVtxLayoutSemantic semantic, u8 index, eVtxLayoutFormat fmt)
{
	assert (offset < 0xffff);
	assert (static_cast<int>(semantic) < 16);
	assert (index < 16);
	assert (static_cast<int>(fmt) < 16);

	//offset	16bit
	//semantic	4bit
	//index		4bit
	//format	4bit
	//empty		4bit
	u32 ret = offset << 16;

	ret |= ((static_cast<u32>(semantic) & 0x0f) << 12);
	ret |= ((static_cast<u32>(index) & 0x0f) << 8);
	ret |= ((static_cast<u32>(fmt) & 0x0f) << 4);
	return ret;
}

//*************************************************************
u32 shape::VtxElem::getOffset (u32 elem)								{ return (elem >> 16); }
eVtxLayoutSemantic shape::VtxElem::getSemantic (u32 elem)				{ return static_cast<eVtxLayoutSemantic>((elem >> 12) & 0x0f); }
u8 shape::VtxElem::getIndex(u32 elem)									{ return static_cast<u8>((elem >> 8) & 0x0f); }
eVtxLayoutFormat shape::VtxElem::getFormat(u32 elem)					{ return static_cast<eVtxLayoutFormat>((elem >> 4) & 0x0f); }

//*************************************************************
u32 shape::VtxElem::buildSearchKey (eVtxLayoutSemantic semantic, u8 index, eVtxLayoutFormat fmt)
{
	u32 key = ((static_cast<u32>(semantic) & 0x0f) << 12);
	key |= ((static_cast<u32>(index) & 0x0f) << 8);
	key |= ((static_cast<u32>(fmt) & 0x0f) << 4);
	return key;
}

//*************************************************************
bool shape::VtxElem::doesKeyMatch (u32 elem, u32 key, u32 *out_offset)
{
	if ((elem & 0x0000FFFF) == key)
	{
		*out_offset = VtxElem::getOffset(elem);
		return true;
	}
	return false;
}
