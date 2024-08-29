#include "gosShape.h"
#include "gosShapeVtxLayout.h"
#include "../gos/gos.h"
#include "../gos/gosUtils.h"

using namespace gos;


//*************************************************************
const char*	shape::enumToString (eVtxLayoutSemantic e)
{
	switch (e)
	{
	default:	return "??unkown??"; 
	case eVtxLayoutSemantic::position:	return "position";
	case eVtxLayoutSemantic::normal:	return "normal";
	case eVtxLayoutSemantic::texCoord:	return "texCoord";
	case eVtxLayoutSemantic::color:		return "color";
	case eVtxLayoutSemantic::tangent:	return "tangent";
	case eVtxLayoutSemantic::bitangent:	return "bitangent";
	case eVtxLayoutSemantic::blendIndices:	return "blendIndices";
	case eVtxLayoutSemantic::blendWeight:	return "blendWeight";
	case eVtxLayoutSemantic::custom:	return "custom";
	}
}

//*************************************************************
const char*	shape::enumToString (eVtxLayoutFormat e)
{
	switch (e)
	{
	default:	return "??unkown??"; 
	case eVtxLayoutFormat::_1f32: return "1f32";
	case eVtxLayoutFormat::_2f32: return "2f32";
	case eVtxLayoutFormat::_3f32: return "3f32";
	case eVtxLayoutFormat::_4f32: return "4f32";
	case eVtxLayoutFormat::_1i32: return "1i32";
	case eVtxLayoutFormat::_2i32: return "2i32";
	case eVtxLayoutFormat::_3i32: return "3i32";
	case eVtxLayoutFormat::_4i32: return "4i32";
	case eVtxLayoutFormat::_1u32: return "1u32";
	case eVtxLayoutFormat::_2u32: return "2u32";
	case eVtxLayoutFormat::_3u32: return "3u32";
	case eVtxLayoutFormat::_4u32: return "4u32";
	}
}

//*************************************************************
u32 shape::getSizeInByte (eVtxLayoutFormat fmt)
{
	switch (fmt)
	{
	default:
		DBGBREAK;
		return 0;
	case eVtxLayoutFormat::_1f32: return 4;
	case eVtxLayoutFormat::_2f32: return 8;
	case eVtxLayoutFormat::_3f32: return 12;
	case eVtxLayoutFormat::_4f32: return 16;

	case eVtxLayoutFormat::_1i32: return 4;
	case eVtxLayoutFormat::_2i32: return 8;
	case eVtxLayoutFormat::_3i32: return 12;
	case eVtxLayoutFormat::_4i32: return 16;

	case eVtxLayoutFormat::_1u32: return 4;
	case eVtxLayoutFormat::_2u32: return 8;
	case eVtxLayoutFormat::_3u32: return 12;
	case eVtxLayoutFormat::_4u32: return 16;
	}
}

//*************************************************************
bool shape::areEqual (const shape::VtxLayout &a, const shape::VtxLayout &b)
{
	if (a.numElem != b.numElem) return false;
	if (a.numElem == 0) return true;
	return ( memcmp(a.elemList, b.elemList, sizeof(u32) * a.numElem) == 0 );
}		

//*************************************************************
u32 shape::calcSizeOfAVertex (const VtxLayout &a)
{
	u32 ret = 0;
	for (u32 i=0; i<a.numElem; i++)
		ret += shape::getSizeInByte (VtxElem::getFormat (a.elemList[i]));
	return ret;
}

//*************************************************************
u32 shape::serialize (const VtxLayout &a, u8 *buffer, u32 sizeof_buffer)
{
	const u32 byteNeeded = sizeof(u32) * (1 + a.numElem);
	if (NULL == buffer)
		return byteNeeded;

	if (sizeof_buffer < byteNeeded)
		return 0;

	u32 ct = 0;
	ct += gos::utils::bufferWriteU32 (&buffer[ct], a.numElem);
	for (u32 i=0; i<a.numElem; i++)
		ct += gos::utils::bufferWriteU32 (&buffer[ct], a.elemList[i]);
	
	assert (ct == byteNeeded);
	return byteNeeded;
}


//*************************************************************
u32 shape::deserialize (VtxLayout *out, const u8 *buffer, u32 sizeof_buffer)
{
	assert (NULL != out);
	if (sizeof_buffer < sizeof(u32))
		return 0;

	u32 ct = 0;
	out->numElem = gos::utils::bufferReadU32 (&buffer[ct]);
	ct += 4;

	const u32 byteNeeded = sizeof(u32) * (1 + out->numElem);
	if (sizeof_buffer < byteNeeded || out->numElem >= VtxLayout::NUM_MAX_ELEM)
	{
		out->numElem = 0;
		return 0;
	}

	for (u32 i=0; i<out->numElem; i++)
	{
		out->elemList[i] = gos::utils::bufferReadU32 (&buffer[ct]);
		ct += 4;
	}

	return byteNeeded;
	
}


//********************************************************
bool shape::shapeAlloc (gos::Allocator *allocator, u32 numVtx, u32 numIdx, Shape *shapeIN)
{
	assert (NULL != allocator);
	assert (NULL != shapeIN);

	if (shapeIN->vtxLayout.numElem == 0)
	{
		logger::err ("shape::shapeAlloc() => shape->vtxLayout has 0 elements!\n");
		return false;
	}

	if (shapeIN->magic != Shape::MAGIC ||
		NULL != shapeIN->idxBuffer ||
		NULL != shapeIN->vtxBuffer||
		0 != shapeIN->numVtx ||
		0 != shapeIN->numIdx)
	{
		logger::err ("shape::shapeAlloc() => shape must be 'resetted'\n");
		return false;
	}

	shapeIN->numVtx = numVtx;
	shapeIN->vtxBuffer = GOSALLOCT(u8*, allocator, shape::calcSizeOfAVertex(shapeIN->vtxLayout) * numVtx);

	shapeIN->numIdx = numIdx;
	shapeIN->idxBuffer = GOSALLOCT(u16*, allocator, sizeof(u16) * numIdx);

	return true;

}

//********************************************************
void shape::shapeFree (gos::Allocator *allocator, Shape *shapeIN)
{
	assert (NULL != allocator);
	assert (NULL != shapeIN);

	if (shapeIN->magic != Shape::MAGIC)
	{
		logger::err ("shape::shapeFree() => invalid shape struct\n");
		return;
	}

	if (NULL != shapeIN->vtxBuffer)
		GOSFREE(allocator, shapeIN->vtxBuffer);

	if (NULL != shapeIN->idxBuffer)
		GOSFREE(allocator, shapeIN->idxBuffer);

	shapeIN->reset();
}

//********************************************************
bool shape::shapeLoad (const char *filename, gos::Allocator *allocator, Shape *out)
{
	u32 fsize;
	u8 *buffer = gos::fs::fileLoadInMemory (gos::getScrapAllocator(), filename, &fsize);
	if (NULL == buffer)
	{
		gos::logger::err ("shape::shapeLoad(%s) => file not found\n", filename);
		return false;
	}

	const bool ret = shape::shapeLoadFromMemory (buffer, fsize, allocator, out);
	GOSFREE(gos::getScrapAllocator(), buffer);
	return ret;
}

//********************************************************
u32 shape::shapeLoadFromMemory (const u8 *mem, u32 sizeof_mem, gos::Allocator *allocator, Shape *out)
{
	if (sizeof_mem < 12)
	{
		gos::logger::err ("shape::shapeLoadFromMemory() => not enough memory\n");
		return 0;
	}

	u32 ct = 0;
	out->reset();

	out->magic = gos::utils::bufferReadU32 (&mem[ct]);		ct+=4;
	if (out->magic != Shape::MAGIC)
	{
		gos::logger::err ("shape::shapeLoadFromMemory() => inavlid MAGIC\n");
		return 0;
	}

	const u32 numVtx = gos::utils::bufferReadU32 (&mem[ct]);			ct+=4;
	const u32 numIdx = gos::utils::bufferReadU32 (&mem[ct]);			ct+=4;
	
	const u32 n = shape::deserialize (&out->vtxLayout, &mem[ct], sizeof_mem - ct);
	if (0 == n)
	{
		gos::logger::err ("shape::shapeLoadFromMemory() => failed to deserialize VtxLayout\n");
		return 0;
	}
	ct += n;


	const u32 vtxBufferSize = shape::calcSizeOfAVertex(out->vtxLayout) * numVtx;
	const u32 idxBufferSize = sizeof(u16) * numIdx;
	if (sizeof_mem < ct +vtxBufferSize +idxBufferSize)
	{
		gos::logger::err ("shape::shapeLoadFromMemory() => not enough memory (2)\n");
		return 0;
	}	

	if (!shape::shapeAlloc (allocator, numVtx, numIdx, out))
	{
		gos::logger::err ("shape::shapeLoadFromMemory() => failed to alloc shape (numVtx=%d, numIdx=%d)\n", numVtx, numIdx);
		return 0;
	}

	//vtxBuffer
	if (vtxBufferSize)
	{
		memcpy (out->vtxBuffer, &mem[ct], vtxBufferSize);
		ct += vtxBufferSize;
	}

	if (idxBufferSize)
	{
		memcpy (out->idxBuffer, &mem[ct], idxBufferSize);
		ct += idxBufferSize;
	}

	return ct;
}

//********************************************************
bool shape::shapeSave (const char *filename, const Shape *shapeIN)
{
	gos::File hFile;
	if (!gos::fs::fileOpenForW (&hFile, filename))
	{
		gos::logger::err ("shape::shapeSave(%s) => unable to create file\n", filename);
		return false;
	}

	const bool ret = shape::shapeSave (hFile, shapeIN);
	gos::fs::fileClose (hFile);
	return ret;
}

//********************************************************
bool shape::shapeSave (gos::File &hFile, const Shape *shapeIN)
{
	assert (NULL != shapeIN);

	u8 mem[256];
	u32 ct = 0;
	ct += gos::utils::bufferWriteU32 (&mem[ct], shapeIN->magic);
	ct += gos::utils::bufferWriteU32 (&mem[ct], shapeIN->numVtx);
	ct += gos::utils::bufferWriteU32 (&mem[ct], shapeIN->numIdx);

	const u32 n = shape::serialize (shapeIN->vtxLayout, &mem[ct], sizeof(mem) - ct);
	if (0 == n)
	{
		gos::logger::err ("shape::shapeSave() => failed to serialize VtxLayout\n");
		return 0;
	}
	ct += n;

	fs::fileWrite (hFile, mem, ct);

	if (0 != shapeIN->numVtx)
	{
		const u32 size = shape::calcSizeOfAVertex(shapeIN->vtxLayout) * shapeIN->numVtx;
		fs::fileWrite (hFile, shapeIN->vtxBuffer, size);
	}

	if (0 != shapeIN->numIdx)
	{
		const u32 size = sizeof(u16) * shapeIN->numIdx;
		fs::fileWrite (hFile, shapeIN->idxBuffer, size);
	}

	return true;

}

//********************************************************
void shape::debug_shapePrint (const Shape *s)
{
	shape::VtxLayoutReader vtxR(&s->vtxLayout);
	const u32 sizeOfAVtx = shape::calcSizeOfAVertex(s->vtxLayout);

	gos::logger::log ("\nSHAPE PRINT\n");
	gos::logger::incIndent();

	gos::logger::log ("num vtx=%d, numIdx=%d\n", s->numVtx, s->numIdx);
	gos::logger::log ("VtxLayout:\n");
	{
		gos::logger::incIndent();
		for (u32 i=0; i<vtxR.getNumElem(); i++)
		{
			gos::logger::log ("OFFSET=%04d  FORMAT=%s  SEMANTIC=%s INDEX=%d\n",
				vtxR.getOffset(i),
				shape::enumToString (vtxR.getFormat(i)),
				shape::enumToString (vtxR.getSemantic(i)),
				vtxR.getIndex(i));
		}
		gos::logger::decIndent();
	}
	
	gos::logger::log ("Vtx list (%d):\n", s->numVtx);
	{
		gos::logger::incIndent();

		u32 ct = 0;
		for (u32 i=0; i<s->numVtx; i++)
		{
			gos::logger::log ("%05d  ", i);
			for (u32 i2=0; i2<vtxR.getNumElem(); i2++)
			{
				const u32 offset = vtxR.getOffset(i2);
				switch (vtxR.getFormat(i2))
				{
				default:
					gos::logger::log ("( ??? )  ");
					break;

				case eVtxLayoutFormat::_2f32:
					{
						const gos::vec2f *data = reinterpret_cast<const gos::vec2f*>(&s->vtxBuffer[ct+offset]);
						gos::logger::log ("(%.3f, %.3f)  ", data->x, data->y);
					}
					break;

				case eVtxLayoutFormat::_3f32:
					{
						const gos::vec3f *data = reinterpret_cast<const gos::vec3f*>(&s->vtxBuffer[ct+offset]);
						gos::logger::log ("(%.3f, %.3f, %.3f)  ", data->x, data->y, data->z);
					}
					break;

				case eVtxLayoutFormat::_4f32:
					{
						const gos::vec4f *data = reinterpret_cast<const gos::vec4f*>(&s->vtxBuffer[ct+offset]);
						gos::logger::log ("(%.3f, %.3f, %.3f)  ", data->x, data->y, data->z, data->w);
					}
					break;
				}
			}
			gos::logger::log ("\n");

			ct += sizeOfAVtx;
		}
		
		gos::logger::decIndent();
	}

	gos::logger::log ("Idx list (%d):\n", s->numIdx);
	{
		gos::logger::incIndent();
		for (u32 i=0; i<s->numIdx; i+=3)
		{
			gos::logger::log ("(%d,%d,%d)  ", s->idxBuffer[i], s->idxBuffer[i+1], s->idxBuffer[i+2]);
		}
		gos::logger::log ("\n");
		gos::logger::decIndent();
	}

	gos::logger::decIndent();
}