#include "gosShape.h"
#include "../gos/gos.h"
#include "../gos/gosUtils.h"

using namespace gos;
using namespace gos::shape;


//*************************************************************
u32 VtxElem::define (u32 offset, eVtxLayoutSemantic semantic, u8 index, eVtxLayoutFormat fmt)
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
u32 VtxElem::getOffset (u32 elem)								{ return (elem >> 16); }
eVtxLayoutSemantic VtxElem::getSemantic (u32 elem)				{ return static_cast<eVtxLayoutSemantic>((elem >> 12) & 0x0f); }
u8 VtxElem::getIndex(u32 elem)									{ return static_cast<u8>((elem >> 8) & 0x0f); }
eVtxLayoutFormat VtxElem::getFormat(u32 elem)					{ return static_cast<eVtxLayoutFormat>((elem >> 4) & 0x0f); }

//*************************************************************
u32 VtxElem::buildSearchKey (eVtxLayoutSemantic semantic, u8 index, eVtxLayoutFormat fmt)
{
	u32 key = ((static_cast<u32>(semantic) & 0x0f) << 12);
	key |= ((static_cast<u32>(index) & 0x0f) << 8);
	key |= ((static_cast<u32>(fmt) & 0x0f) << 4);
	return key;
}

//*************************************************************
bool VtxElem::doesKeyMatch (u32 elem, u32 key, u32 *out_offset)
{
	if ((elem & 0x0000FFFF) == key)
	{
		*out_offset = VtxElem::getOffset(elem);
		return true;
	}
	return false;
}




//*************************************************************
bool VtxLayout::end()
{
	if (0xff == numElem)
	{
		logger::err ("VtxLayout::end() => too many elements");
		return false;
	}
	return true;
}

//*************************************************************
bool VtxLayout::find (eVtxLayoutSemantic semantic, u8 index, eVtxLayoutFormat fmt, u32 *out_offset) const
{
	assert (NULL != out_offset);
	const u32 key = VtxElem::buildSearchKey (semantic, index, fmt);

	for (u32 i=0; i<getNumElem(); i++)
	{
		if (VtxElem::doesKeyMatch (elemList[i], key, out_offset))
			return true;
	}
	return false;
}

//*************************************************************
u32 VtxLayout::serialize (u8 *buffer, u32 sizeof_buffer) const
{
	const u32 byteNeeded = sizeof(u32) * (1 + getNumElem());
	if (sizeof_buffer < byteNeeded)
		return 0;

	u32 ct = 0;
	ct += gos::utils::bufferWriteU32 (&buffer[ct], getNumElem());
	for (u32 i=0; i<getNumElem(); i++)
		ct += gos::utils::bufferWriteU32 (&buffer[ct], elemList[i]);
	
	assert (ct == byteNeeded);
	return byteNeeded;
}

//*************************************************************
u32 VtxLayout::deserialize (const u8 *buffer, u32 sizeof_buffer)
{
	if (sizeof_buffer < sizeof(u32))
		return 0;

	u32 ct = 0;
	numElem = gos::utils::bufferReadU32 (&buffer[ct]);
	ct += 4;

	const u32 byteNeeded = sizeof(u32) * (1 + numElem);
	if (sizeof_buffer < byteNeeded || numElem >= NUM_MAX_ELEM)
	{
		numElem = 0;
		return 0;
	}

	for (u32 i=0; i<numElem; i++)
	{
		elemList[i] = gos::utils::bufferReadU32 (&buffer[ct]);
		ct += 4;
	}

	return byteNeeded;
	
}
