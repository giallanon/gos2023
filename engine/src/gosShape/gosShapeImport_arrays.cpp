#include "gosShapeImport_arrays.h"
#include "../gos/gos.h"

using namespace gos;
using namespace gos::shape;

//********************************************
ArraysImporter::ArraysImporter()
{
	finalVtxList.setup (gos::getScrapAllocator(), 1024);
	sortedFinalVtxList.setup (gos::getScrapAllocator(), 1024);
	finalIdxList.setup (gos::getScrapAllocator(), 1024);
}

//********************************************
ArraysImporter::~ArraysImporter()
{
	sortedFinalVtxList.unsetup();
	finalVtxList.unsetup();
	finalIdxList.unsetup();
}

//********************************************
bool ArraysImporter::priv_extractTupla (u32 iStart, u32 numIdxPerTupla, sTupla *out) const
{
	if (iStart + numIdxPerTupla > totNumOfIdxInTupleList)
		return false;

	for (u32 i=0;i<numIdxPerTupla; i++)
		out->idx[i] = tupleList[iStart++];
	return true;
}

//********************************************
void ArraysImporter::priv_extractVertex (const sTupla &tupla, sVertex *out) const
{
	out->reset();

	const u32 n = importData.channels.getNElem();
	for (u32 i=0; i<n; i++)
	{
		if (importData.channels(i)->isValid())
		{
			//out->numIndices++;
			//out->idx[importData.channels(i)->getOffsetInVtx()] = tupla.idx[importData.channels(i)->getOffsetInTupla()];
			out->add (importData.channels(i)->getOffsetInVtx(), tupla.idx[importData.channels(i)->getOffsetInTupla()]);
		}
	}
}

//********************************************
u32 ArraysImporter::priv_findOrCreateVtx (const sVertex &vIN)
{
	/*const u32 n = finalVtxList.getNElem();
	for (u32 i=0; i<n; i++)
	{
		if (vIN == finalVtxList(i))
			return i;	}

	//devo creare un nuovo vtx
	finalVtxList.append (vIN);
	return n;
*/

	SortedFastArray<sVertex, u32>::Position pos;
	u32 vtxIndex;
	if (sortedFinalVtxList.find(vIN, &vtxIndex, &pos))
		return vtxIndex;

	const u32 n = finalVtxList.getNElem();
	finalVtxList.append (vIN);
	//sortedFinalVtxList.insertIfNotExists (vIN, n);
	sortedFinalVtxList.insertInPosition (pos, n);
	return n;
}

//********************************************
void ArraysImporter::priv_reset (eMode modeIN, const VtxLayout &desiredLayout)
{
	importData.reset();
	shapeVtxLayout = desiredLayout;
	finalVtxList.reset();
	finalIdxList.reset();
	sortedFinalVtxList.reset();
	nextValidOffsetInVtx = 0;
	tupleList = NULL;
	numIdxPerOgniTupla = 0;

	errorCode = 0;
	mode = modeIN;
}

//************************************************************
ArraysImporter&	ArraysImporter::beginUsingFaceList (const VtxLayout &desiredLayout, const u16 *tupleListIN, u32 totNumOfIdxInTupleListIN, u8 numIdxPerOgniTuplaIN)
{
	priv_reset (eMode::importFromFaceList, desiredLayout);
	tupleList = tupleListIN;
	numIdxPerOgniTupla = numIdxPerOgniTuplaIN;
	totNumOfIdxInTupleList = totNumOfIdxInTupleListIN;

	if (numIdxPerOgniTupla > NUM_MAX_INDEX_PER_TUPLA)
	{
		gos::logger::verbose ("shape::ArraysImporter::begin() => too many indices per tupla. Max supported is %d\n", NUM_MAX_INDEX_PER_TUPLA);
		errorCode = 2;
		return *this;
	}	
	if (0 == numIdxPerOgniTupla)
	{
		gos::logger::verbose ("shape::ArraysImporter::begin() => too few indices per tupla (%d)\n", numIdxPerOgniTupla);
		errorCode = 3;
		return *this;
	}

	if ((totNumOfIdxInTupleList % numIdxPerOgniTupla) != 0)
	{
		gos::logger::verbose ("shape::ArraysImporter::begin() => tupleList has %d elements. A single tuple is %d elements. %s is not perfectly divisible by %d\n", totNumOfIdxInTupleList, numIdxPerOgniTupla, numIdxPerOgniTupla);
		errorCode = 4;
		return *this;
	}

	return *this;
}

//************************************************************
ArraysImporter&	ArraysImporter::beginUsingRealIdxBuffer (const VtxLayout &desiredLayout, const u16 *idxBuffer, u32 totNumOfIdxInIdxBuffer)
{
	priv_reset (eMode::importFromRealIdxBuffer, desiredLayout);
	tupleList = idxBuffer;
	numIdxPerOgniTupla = 1;
	totNumOfIdxInTupleList = totNumOfIdxInIdxBuffer;
	return *this;
}

//************************************************************
bool ArraysImporter::end (gos::Allocator *shapeAllocator, Shape *out_shape)
{
	assert (NULL != shapeAllocator);
	assert (NULL != out_shape);

	if (errorCode)
	{
		gos::logger::err ("shape::ArraysImporter::end() => there was an errore before callind end(). ErrorCode = %d\n", errorCode);
		return false;
	}

	switch (mode)
	{
	default:
		DBGBREAK;
		return false;

	case eMode::importFromFaceList:
		return priv_endWithTuplaList (shapeAllocator, out_shape);

	case eMode::importFromRealIdxBuffer:
		return priv_endWithRealIdxBuffer (shapeAllocator, out_shape);
	}		
}

//************************************************************
bool ArraysImporter::priv_endWithTuplaList (gos::Allocator *shapeAllocator, Shape *out_shape)
{
	//devo combinare pos/norm/texcoord in vertici univoci
	for (u32 i=0; i<totNumOfIdxInTupleList;)
	{
		//estraggo 3 tuple e le trasformo in 3 vertici univoci
		for (u32 i2=0; i2<3; i2++)
		{
			sTupla tupla;
			if (!priv_extractTupla (i, numIdxPerOgniTupla, &tupla))
			{
				errorCode = 100;
				gos::logger::verbose ("shape::ArraysImporter::end() => error extracting tupla num %d (not enough indices)\n", i);
				return false;
			}
			i += numIdxPerOgniTupla;


			sVertex v;
			priv_extractVertex (tupla, &v);
			const u32 vtxIndex = priv_findOrCreateVtx (v);
			finalIdxList.append (vtxIndex);
		}
	}

	return priv_buildFinalShape (shapeAllocator, out_shape);
}

//************************************************************
bool ArraysImporter::priv_endWithRealIdxBuffer (gos::Allocator *shapeAllocator, Shape *out_shape)
{
	const u8 numElemInVtx = static_cast<u8>(importData.channels.getNElem());

	for (u32 i=0; i<totNumOfIdxInTupleList; i++)
	{
		const u16 idx = tupleList[i];

		sVertex v;
		v.reset();

		for (u8 i2=0; i2<numElemInVtx; i2++)
			v.add(i2, idx);

		finalVtxList[idx] = v;
		finalIdxList.append (idx);		
	}		

	return priv_buildFinalShape (shapeAllocator, out_shape);
}

/************************************************************
 * si basa su [finalVtxList] e [finalIdxList] e crea la shape
 */
bool ArraysImporter::priv_buildFinalShape (gos::Allocator *shapeAllocator, Shape *out_shape)
{
	out_shape->reset();
	if (!shape::shapeAlloc (shapeAllocator, shapeVtxLayout, finalVtxList.getNElem(), finalIdxList.getNElem(), out_shape))
	{
		errorCode = 101;
		gos::logger::err ("shape::ArraysImporter::priv_buildFinalShape() => error allocating shape with %d vtx and %d indices\n", finalVtxList.getNElem(), finalIdxList.getNElem());
		return false;
	}

	//copio idx buffer
	memcpy (out_shape->idxBuffer, finalIdxList._queryPointer(), sizeof(u16) * out_shape->numIdx);

	//creo il vtxBuffer
	VtxArrayWriter writer;
	writer.setup (out_shape);

	for (u32 i=0; i<importData.channels.getNElem(); i++)
	{
		const DataArrayInterface *da = importData.channels(i);
		switch (da->getFormat())
		{
		default:
			errorCode = 102;
			gos::logger::err ("shape::ArraysImporter::priv_buildFinalShape() => unsupported vtxFormat (%s)\n", shape::enumToString(da->getFormat()));
			break;

		case eVtxLayoutFormat::_2f32:
			priv_finalizeShapeVtxBuffer<vec2f> (writer, da);
			break;

		case eVtxLayoutFormat::_3f32:
			priv_finalizeShapeVtxBuffer<vec3f> (writer, da);
			break;
		}
	}


	return true;

}