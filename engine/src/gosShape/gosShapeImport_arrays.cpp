#include "gosShapeImport_arrays.h"
#include "../gos/gos.h"

using namespace gos;
using namespace gos::shape;

//********************************************
ArraysImporter::ArraysImporter()
{
	finalVtxList.setup (gos::getScrapAllocator(), 1024);
	finalIdxList.setup (gos::getScrapAllocator(), 1024);
}

//********************************************
ArraysImporter::~ArraysImporter()
{
	finalVtxList.unsetup();
	finalIdxList.unsetup();
}

//********************************************
bool ArraysImporter::priv_extractTupla (const FastArray<u16> *trisList, u32 iStart, u32 numIdxPerTupla, sTupla *out) const
{
	if (iStart + numIdxPerTupla > trisList->getNElem())
		return false;

	for (u32 i=0;i<numIdxPerTupla; i++)
		out->idx[i] = trisList->queryElem(iStart++);
	return true;
}

//********************************************
void ArraysImporter::priv_extractVertex (const sTupla &tupla, const sImportData &importData, sVertex *out) const
{
	out->numIndices = 0;
	if (importData.position.isValid())		{ out->numIndices++; out->idx[importData.position.offsetInVtx] = tupla.idx[importData.position.offsetInTupla]; }
	if (importData.norm.isValid())			{ out->numIndices++; out->idx[importData.norm.offsetInVtx] = tupla.idx[importData.norm.offsetInTupla]; }
	if (importData.tutv0.isValid())			{ out->numIndices++; out->idx[importData.tutv0.offsetInVtx] = tupla.idx[importData.tutv0.offsetInTupla]; }
}

//********************************************
u32 ArraysImporter::priv_findOrCreateVtx (const sVertex &vIN, const sImportData &importData)
{
	const u32 n = finalVtxList.getNElem();
	for (u32 i=0; i<n; i++)
	{
		if (vIN == finalVtxList(i))
			return i;
	}

	//devo creare un nuovo vtx
	finalVtxList.append (vIN);
	return n;
}





//********************************************
bool ArraysImporter::create (const VtxLayout &desiredLayout, gos::Allocator *allocator, Shape *out,
							const FastArray<u16> *trisList, u8 numIdxPerOgniVtxDiTrisList,
							const FastArray<vec3f> *posListIN, u8 indexOffsetForPosition,
							const FastArray<vec3f> *normListIN, u8 indexOffsetForNormal,
							const FastArray<vec2f> *tutv0ListIN, u8 indexOffsetForTutv0)
{
	assert (NULL != allocator);
	assert (NULL != out);
	assert (NULL != trisList);

	finalVtxList.reset();
	finalIdxList.reset();

	//[trisList] e' un elenco di tuple, ciascuna composta da [numIdxPerOgniVtxDiTrisList] indici.
	//3 tuple consecutive sono un triangolo
	//Ogni tupla contiene informazioni su posizione, normale, texCoeord e via dicendo
	//Gli indici [indexOffsetForPosition], [indexOffsetForNormal].. se diversi da 0xFF, indicano la posizione all'interno della tupla
	if (numIdxPerOgniVtxDiTrisList > NUM_MAX_INDEX_PER_TUPLA)
	{
		gos::logger::verbose ("shape::ArraysImporter::create() => too many indices per tupla. Max supported is %d\n", NUM_MAX_INDEX_PER_TUPLA);
		return false;
	}	
	if (0 == numIdxPerOgniVtxDiTrisList)
	{
		gos::logger::verbose ("shape::ArraysImporter::create() => too few indices per tupla (%d)\n", numIdxPerOgniVtxDiTrisList);
		return false;
	}	

	//verifichiamo che ci sia compatibilta' tra il VtxLayout e gli array in input
	sImportData importData;
	{
		VtxLayoutReader vxtLayoutR(&desiredLayout);
		u8 nextValidOffsetInVtx = 0;

		if (NULL != posListIN)
		{
			if (!vxtLayoutR.exists (eVtxLayoutSemantic::position, 0, eVtxLayoutFormat::_3f32))
			{
				gos::logger::verbose ("shape::ArraysImporter::create() => VtxLayout does not contains 'position'\n");
				return false;
			}
			if (indexOffsetForPosition >= numIdxPerOgniVtxDiTrisList)
			{
				gos::logger::verbose ("shape::ArraysImporter::create() => index for 'position' is out of range\n");
				return false;
			}

			importData.position.setup (posListIN, indexOffsetForPosition, nextValidOffsetInVtx++);
		}

		if (NULL != normListIN)
		{
			if (!vxtLayoutR.exists (eVtxLayoutSemantic::normal, 0, eVtxLayoutFormat::_3f32))
			{
				gos::logger::verbose ("shape::ArraysImporter::create() => VtxLayout does not contains 'normal'\n");
				return false;
			}
			if (indexOffsetForNormal >= numIdxPerOgniVtxDiTrisList)
			{
				gos::logger::verbose ("shape::ArraysImporter::create() => index for 'normal' is out of range\n");
				return false;
			}

			importData.norm.setup (normListIN, indexOffsetForNormal, nextValidOffsetInVtx++);
		}

		if (NULL != tutv0ListIN)
		{
			if (!vxtLayoutR.exists (eVtxLayoutSemantic::texCoord, 0, eVtxLayoutFormat::_2f32))
			{
				gos::logger::verbose ("shape::ArraysImporter::create() => VtxLayout does not contains 'texCoord0'\n");
				return false;
			}
			if (indexOffsetForTutv0 >= numIdxPerOgniVtxDiTrisList)
			{
				gos::logger::verbose ("shape::ArraysImporter::create() => index for 'texCoord0' is out of range\n");
				return false;
			}

			importData.tutv0.setup (tutv0ListIN, indexOffsetForTutv0, nextValidOffsetInVtx++);
		}
	}

	//devo combinare pos/norm/texcoord in vertici univoci
	const u32 numIndices = trisList->getNElem();
	for (u32 i=0; i<numIndices;)
	{
		sVertex v[3];
		for (u32 i2=0; i2<3; i2++)
		{
			sTupla tupla;
			if (!priv_extractTupla (trisList, i, numIdxPerOgniVtxDiTrisList, &tupla))
			{
				gos::logger::verbose ("shape::ArraysImporter::create() => error extracting tupla num %d (not enough indices)\n", i);
				return false;
			}
		
			priv_extractVertex (tupla, importData, &v[i2]);
			i += numIdxPerOgniVtxDiTrisList;
		}

		//ora ho i 3 vertici del tris.
		//Devo verificare se esistono di gia' nel mio elenco globale di vtx che sto creando
		const u32 vtx1 = priv_findOrCreateVtx (v[0], importData);
		const u32 vtx2 = priv_findOrCreateVtx (v[1], importData);
		const u32 vtx3 = priv_findOrCreateVtx (v[2], importData);

		finalIdxList.append (vtx1);
		finalIdxList.append (vtx2);
		finalIdxList.append (vtx3);
	}


	//creo la shape
	out->reset();
	out->vtxLayout = desiredLayout;
	if (!shape::shapeAlloc (allocator, finalVtxList.getNElem(), finalIdxList.getNElem(), out))
	{
		gos::logger::err ("shape::ArraysImporter::create() => error allocating shape with %d vtx and %d indices\n", finalVtxList.getNElem(), finalIdxList.getNElem());
		return false;
	}

	//copio idx buffer
	memcpy (out->idxBuffer, finalIdxList._queryPointer(), sizeof(u16) * out->numIdx);

	//creo il vtxBuffer
	VtxArrayWriter writer;
	writer.setup (out);
	priv_finalizeShapeVtxBuffer (writer, importData, importData.position, eVtxLayoutSemantic::position, 0, eVtxLayoutFormat::_3f32);
	priv_finalizeShapeVtxBuffer (writer, importData, importData.norm, eVtxLayoutSemantic::normal, 0, eVtxLayoutFormat::_3f32);
	priv_finalizeShapeVtxBuffer (writer, importData, importData.tutv0, eVtxLayoutSemantic::texCoord, 0, eVtxLayoutFormat::_2f32);



	return true;

}

