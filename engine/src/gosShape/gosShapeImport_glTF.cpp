#include "gosShapeImport_glTF.h"
#include "gosShapeImport_arrays.h"
#include "../gos/gos.h"
#include "../gos/gosUtils.h"

#define GOS__glTF_VERBOSE


using namespace gos;
using namespace gos::shape;


/************************************************************************************************************************************
 * 
 * AvailVtxChannel
 * 
 * 
 *************************************************************************************************************************************/
void glTFImporter::AvailVtxChannel::addAccessorIndex (u32 accessorIndex, eVtxLayoutSemantic semantic, u32 index, eVtxLayoutFormat fmt)
{
	if (numElem >= MAX_NUM_ELEM)
		return;
	elem[numElem++] = VtxElem::define (accessorIndex, semantic, index, fmt);
}

bool glTFImporter::AvailVtxChannel::getAccessorIndex (eVtxLayoutSemantic semantic, u32 index, eVtxLayoutFormat fmt, u32 *out_accessorIndex) const
{
	const u32 key = VtxElem::buildSearchKey (semantic, index, fmt);
	for (u32 i=0; i<numElem; i++)
	{
		if (VtxElem::doesKeyMatch (elem[i], key, out_accessorIndex))
			return true;
	}
	return false;
}



/************************************************************************************************************************************
 * 
 * sAccessors
 * 
 * 
 *************************************************************************************************************************************/
glTFImporter::sAccessors::eFmt glTFImporter::sAccessors::parseComponentType (u32 num)
{
	switch (num)
	{
	default:
		DBGBREAK;
		return eFmt::unknown;
	case 5120: return eFmt::_i8;
	case 5121: return eFmt::_u8;
	case 5122: return eFmt::_i16;
	case 5123: return eFmt::_u16;
	case 5124: return eFmt::_i32;
	case 5125: return eFmt::_u32;
	case 5126: return eFmt::_f32;
	}
}

glTFImporter::sAccessors::eType glTFImporter::sAccessors::parseType (const char *name)
{
	if (strcmp (name, "SCALAR") == 0)	return eType::scalar;
	if (strcmp (name, "VEC2") == 0)	return eType::vec2;
	if (strcmp (name, "VEC3") == 0)	return eType::vec3;
	if (strcmp (name, "VEC4") == 0)	return eType::vec4;
	if (strcmp (name, "MAT2") == 0)	return eType::matrix2;
	if (strcmp (name, "MAT3") == 0)	return eType::matrix3;
	if (strcmp (name, "MAT4") == 0)	return eType::matrix4;
	DBGBREAK;
	return eType::unknown;
	
}

bool glTFImporter::sAccessors::isCompatibile (eVtxLayoutFormat fmtIN) const
{
	switch (fmtIN)
	{
	default:
		DBGBREAK;
		return false;

	case eVtxLayoutFormat::_1f32:	return (eFmt::_f32 == fmt && eType::scalar == type);
	case eVtxLayoutFormat::_2f32:	return (eFmt::_f32 == fmt && eType::vec2 == type);
	case eVtxLayoutFormat::_3f32:	return (eFmt::_f32 == fmt && eType::vec3 == type);
	case eVtxLayoutFormat::_4f32:	return (eFmt::_f32 == fmt && eType::vec4 == type);		

	case eVtxLayoutFormat::_1i32:	return (eFmt::_i32 == fmt && eType::scalar == type);
	case eVtxLayoutFormat::_2i32:	return (eFmt::_i32 == fmt && eType::vec2 == type);
	case eVtxLayoutFormat::_3i32:	return (eFmt::_i32 == fmt && eType::vec3 == type);
	case eVtxLayoutFormat::_4i32:	return (eFmt::_i32 == fmt && eType::vec4 == type);		

	case eVtxLayoutFormat::_1u32:	return (eFmt::_u32 == fmt && eType::scalar == type);
	case eVtxLayoutFormat::_2u32:	return (eFmt::_u32 == fmt && eType::vec2 == type);
	case eVtxLayoutFormat::_3u32:	return (eFmt::_u32 == fmt && eType::vec3 == type);
	case eVtxLayoutFormat::_4u32:	return (eFmt::_u32 == fmt && eType::vec4 == type);		
	}

}

eVtxLayoutFormat glTFImporter::sAccessors::toVtxLayoutFmt() const
{
	switch (fmt)
	{
	default:
		break;

	case eFmt::_i8:
		switch (type)
		{
			default: break;
			case eType::scalar:	return eVtxLayoutFormat::_1i8;
			case eType::vec2:	return eVtxLayoutFormat::_2i8;
			case eType::vec3:	return eVtxLayoutFormat::_3i8;
			case eType::vec4:	return eVtxLayoutFormat::_4i8;
		}
		break;

	case eFmt::_u8:
		switch (type)
		{
			default: break;
			case eType::scalar:	return eVtxLayoutFormat::_1u8;
			case eType::vec2:	return eVtxLayoutFormat::_2u8;
			case eType::vec3:	return eVtxLayoutFormat::_3u8;
			case eType::vec4:	return eVtxLayoutFormat::_4u8;
		}
		break;

	case eFmt::_i16:
		switch (type)
		{
			default: break;
			case eType::scalar:	return eVtxLayoutFormat::_1i16;
			case eType::vec2:	return eVtxLayoutFormat::_2i16;
			case eType::vec3:	return eVtxLayoutFormat::_3i16;
			case eType::vec4:	return eVtxLayoutFormat::_4i16;
		}
		break;

	case eFmt::_u16:
		switch (type)
		{
			default: break;
			case eType::scalar:	return eVtxLayoutFormat::_1u16;
			case eType::vec2:	return eVtxLayoutFormat::_2u16;
			case eType::vec3:	return eVtxLayoutFormat::_3u16;
			case eType::vec4:	return eVtxLayoutFormat::_4u16;
		}
		break;

	case eFmt::_i32:
		switch (type)
		{
			default: break;
			case eType::scalar:	return eVtxLayoutFormat::_1i32;
			case eType::vec2:	return eVtxLayoutFormat::_2i32;
			case eType::vec3:	return eVtxLayoutFormat::_3i32;
			case eType::vec4:	return eVtxLayoutFormat::_4i32;
		}
		break;

	case eFmt::_u32:
		switch (type)
		{
			default: break;
			case eType::scalar:	return eVtxLayoutFormat::_1u32;
			case eType::vec2:	return eVtxLayoutFormat::_2u32;
			case eType::vec3:	return eVtxLayoutFormat::_3u32;
			case eType::vec4:	return eVtxLayoutFormat::_4u32;
		}
		break;

	case eFmt::_f32:
		switch (type)
		{
			default: break;
			case eType::scalar:	return eVtxLayoutFormat::_1f32;
			case eType::vec2:	return eVtxLayoutFormat::_2f32;
			case eType::vec3:	return eVtxLayoutFormat::_3f32;
			case eType::vec4:	return eVtxLayoutFormat::_4f32;
		}
		break;		
	}

	DBGBREAK;
	return eVtxLayoutFormat::unknown;
}

/************************************************************************************************************************************
 * 
 * glTFImporter
 * 
 * 
 *************************************************************************************************************************************/
glTFImporter::glTFImporter()
{
	localAllocator = gos::getScrapAllocator();
	json = NULL;
	bin = NULL;
	rootBone.reset();

	bufferViewList.setup (localAllocator, 128);
	accessorsList.setup (localAllocator, 128);
	nodesList.setup (localAllocator, 128);
	shapesInMesh.setup (localAllocator, 1024);
	meshesList.setup (localAllocator, 1024);
}

//********************************************
glTFImporter::~glTFImporter()
{
	bufferViewList.unsetup ();
	accessorsList.unsetup ();
	nodesList.unsetup ();
	shapesInMesh.unsetup ();
	meshesList.unsetup ();
	priv_free();
}

//********************************************
void glTFImporter::priv_free()
{
	json = NULL;
	bin = NULL;
	bufferViewList.reset();
	accessorsList.reset();
	nodesList.reset();
	shapesInMesh.reset ();
	meshesList.reset ();
	
	rootBone.deleteAllChildren (localAllocator);
	rootBone.reset();
}

//********************************************
bool glTFImporter::priv_parseBufferView (const gos::IniFileSection *sec)
{
	const u32 byteLength = sec->getOrDefaultAsU32("byteLength", u32MAX);
	const u32 byteOffset = sec->getOrDefaultAsU32("byteOffset", u32MAX);

	if (u32MAX == byteOffset || u32MAX == byteLength)
		return false;

	const u32 n = bufferViewList.getNElem();
	bufferViewList[n].p = &bin[byteOffset];
	bufferViewList[n].len = byteLength;
	return true;
}

//********************************************
bool glTFImporter::priv_parseAccessor (const gos::IniFileSection *sec)
{
	char s[64];
	const u32 bufferView = sec->getOrDefaultAsU32("bufferView", u32MAX);
	const u32 byteOffset = sec->getOrDefaultAsU32("byteOffset", 0);
	const u32 componentType = sec->getOrDefaultAsU32("componentType", u32MAX);
	const u32 count = sec->getOrDefaultAsU32("count", u32MAX);
	sec->getOrDefault ("type", "??", s, sizeof(s));

	if (u32MAX == bufferView || u32MAX == count)
		return false;
	if (bufferView >= bufferViewList.getNElem())
		return false;

	const u32 n = accessorsList.getNElem();
	accessorsList[n].pData = &bufferViewList(bufferView).p[byteOffset];
	accessorsList[n].fmt = sAccessors::parseComponentType(componentType);
	accessorsList[n].count = count;
	accessorsList[n].type = sAccessors::parseType(s);

	if (sAccessors::eFmt::unknown == accessorsList[n].fmt)
		return false;

	if (sAccessors::eType::unknown == accessorsList[n].type)
		return false;

	return true;
}

//********************************************
void glTFImporter::priv_parseMeshAttributes (const gos::IniFileSection *sec, AvailVtxChannel *out) const
{
	u32 accessorIndex;
	out->reset();

	accessorIndex = sec->getOrDefaultAsU32("POSITION", u32MAX);
	if (u32MAX != accessorIndex)
	{
		const eVtxLayoutFormat accessorFmt =accessorsList(accessorIndex).toVtxLayoutFmt();
		out->addAccessorIndex (accessorIndex, eVtxLayoutSemantic::position, 0, accessorFmt);
	}

	accessorIndex = sec->getOrDefaultAsU32("NORMAL", u32MAX);
	if (u32MAX != accessorIndex)
	{
		const eVtxLayoutFormat accessorFmt =accessorsList(accessorIndex).toVtxLayoutFmt();
		out->addAccessorIndex (accessorIndex, eVtxLayoutSemantic::normal, 0, accessorFmt);
	}

	accessorIndex = sec->getOrDefaultAsU32("TANGENT", u32MAX);
	if (u32MAX != accessorIndex)
	{
		const eVtxLayoutFormat accessorFmt =accessorsList(accessorIndex).toVtxLayoutFmt();
		out->addAccessorIndex (accessorIndex, eVtxLayoutSemantic::tangent, 0, accessorFmt);
	}

	accessorIndex = sec->getOrDefaultAsU32("TEXCOORD_0", u32MAX);
	if (u32MAX != accessorIndex)
	{
		const eVtxLayoutFormat accessorFmt =accessorsList(accessorIndex).toVtxLayoutFmt();
		out->addAccessorIndex (accessorIndex, eVtxLayoutSemantic::texCoord, 0, accessorFmt);
	}

	accessorIndex = sec->getOrDefaultAsU32("TEXCOORD_1", u32MAX);
	if (u32MAX != accessorIndex)
	{
		const eVtxLayoutFormat accessorFmt =accessorsList(accessorIndex).toVtxLayoutFmt();
		out->addAccessorIndex (accessorIndex, eVtxLayoutSemantic::texCoord, 1, accessorFmt);
	}

	accessorIndex = sec->getOrDefaultAsU32("TEXCOORD_2", u32MAX);
	if (u32MAX != accessorIndex)
	{
		const eVtxLayoutFormat accessorFmt =accessorsList(accessorIndex).toVtxLayoutFmt();
		out->addAccessorIndex (accessorIndex, eVtxLayoutSemantic::texCoord, 2, accessorFmt);
	}
}

//********************************************
bool glTFImporter::priv_parseMesh (const gos::IniFileSection *sec)
{
	const u32 meshNum = meshesList.getNElem();
	meshesList[meshNum].begin (shapesInMesh);


	char s[128];
	u32 index = 0;
	while (1)
	{
		sprintf_s (s, sizeof(s), "primitives[%d]", index);
		const gos::IniFileSection *prim = sec->getSubsection(s);
		if (NULL == prim)
			break;

		//informazioni su cosa c'e' di disponibile per ogni vertice (pos, norm, tan....)
		AvailVtxChannel availVtxChannel;
		{
			const gos::IniFileSection *attr = prim->getSubsection("attributes");
			if (NULL == attr)
			{
				gos::logger::err ("glTFImporter::priv_parseMesh(%d) => missing 'attributes' section\n", index);
				return false;
			}
			priv_parseMeshAttributes (attr, &availVtxChannel);
		}

		//recupero index buffer
		const u16 *idxBuffer = NULL;
		u32 numIdx = 0;
		{
			const u32 indices_accessorIndex = prim->getOrDefaultAsU32 ("indices", u32MAX);
			if (u32MAX == indices_accessorIndex)
			{
				gos::logger::err ("glTFImporter::priv_parseMesh(%d) => non 'indexed' mesh are not supported\n", index);
				return false;
			}
			if (accessorsList(indices_accessorIndex).fmt != sAccessors::eFmt::_u16 ||
				accessorsList(indices_accessorIndex).type != sAccessors::eType::scalar)
			{
				gos::logger::err ("glTFImporter::priv_parseMesh(%d) => index buffer is not in u16 format\n", index);
				return false;
			}

			idxBuffer = reinterpret_cast<const u16*> (accessorsList(indices_accessorIndex).pData);
			numIdx = accessorsList(indices_accessorIndex).count;
		}


		//tipo di primitiva (supporto solo triangle)
		const u32 primitiveType = prim->getOrDefaultAsU32 ("mode", 4);
		if (4 != primitiveType)
		{
			gos::logger::err ("glTFImporter::priv_parseMesh(%d) => non 'primitive type' other than 'tris' are not supported\n", index);
			return false;
		}		

		//preparo l'importer e creo la shape
		shape::VtxLayoutReader vtxLayoutR (&shapeOut.vtxLayot);

#define IMPORTA_USANDO_FACELIST

#ifdef IMPORTA_USANDO_IDXBUFFER
		shape::ArraysImporter imp;
		imp.beginUsingRealIdxBuffer (shapeOut.vtxLayot, idxBuffer, numIdx);
#endif


#ifdef IMPORTA_USANDO_FACELIST
		//preparo l'array "faceInfo"
			const u32 numElemPerTupla = vtxLayoutR.getNumElem();
			FastArray<u16> faceList;
			{
				faceList.setup (gos::getScrapAllocator(), numIdx * numElemPerTupla);
				for (u32 i=0; i<numIdx; i++)
				{
					const u16 idx = idxBuffer[i];
					for (u32 t=0; t<numElemPerTupla; t++)
						faceList.append(idx);
				}
			}

			//preparo l'importer
			shape::ArraysImporter imp;
			imp.beginUsingFaceList (shapeOut.vtxLayot, faceList._queryTypedPointer(), faceList.getNElem(), numElemPerTupla);
#endif




		for (u32 i=0; i<vtxLayoutR.getNumElem(); i++)
		{
			u32 ii;
			if (!availVtxChannel.getAccessorIndex (vtxLayoutR.getSemantic(i), vtxLayoutR.getIndex(i), vtxLayoutR.getFormat(i), &ii))
			{
				gos::logger::verbose ("glTFImporter::priv_parseMesh(%d) => file does not contains info for <%s, %d, %s>\n", index, shape::enumToString(vtxLayoutR.getSemantic(i)), vtxLayoutR.getIndex(i), shape::enumToString(vtxLayoutR.getFormat(i)));
			}
			else
			{
				switch (vtxLayoutR.getFormat(i))
				{
				default:
					DBGBREAK;
					break;

				case eVtxLayoutFormat::_2f32:
					imp.addImportArray<vec2f> (accessorsList(ii).pData, i, vtxLayoutR.getSemantic(i), vtxLayoutR.getIndex(i), vtxLayoutR.getFormat(i));
					break;

				case eVtxLayoutFormat::_3f32:
					imp.addImportArray<vec3f> (accessorsList(ii).pData, i, vtxLayoutR.getSemantic(i), vtxLayoutR.getIndex(i), vtxLayoutR.getFormat(i));
					break;
				}
			}
		}
		
		Shape myShape;
		if (!imp.end (shapeOut.shapeAllocator, &myShape))
		{
			gos::logger::err ("glTFImporter::priv_parseMesh(%d) => error creating shape\n", index);
			return false;
		}
		shapeOut.shapeList->append(myShape);

		//shape::shapeRightHandedToLeftHanded (&myShape);



		//registro la primitiva nella mesh
		meshesList[meshNum].addShapeIndex (shapeOut.shapeList->getNElem() -1);

		//prossimo nodo 'primitives'
		index++;
	}

	return true;
}

//********************************************
bool glTFImporter::priv_parseNodes (const gos::IniFileSection *sec)
{
	const u32 n = nodesList.getNElem();
	nodesList[n].reset();
	nodesList[n].meshIndex = sec->getOrDefaultAsU32 ("mesh", u32MAX);


	if (sec->exists ("matrix[0]"))
	{
		nodesList[n].localTRS(0,0) = sec->getOrDefaultAsF32 ("matrix[0]", 0);
		nodesList[n].localTRS(0,1) = sec->getOrDefaultAsF32 ("matrix[1]", 0);
		nodesList[n].localTRS(0,2) = sec->getOrDefaultAsF32 ("matrix[2]", 0);
		nodesList[n].localTRS(0,3) = sec->getOrDefaultAsF32 ("matrix[3]", 0);

		nodesList[n].localTRS(1,0) = sec->getOrDefaultAsF32 ("matrix[4]", 0);
		nodesList[n].localTRS(1,1) = sec->getOrDefaultAsF32 ("matrix[5]", 0);
		nodesList[n].localTRS(1,2) = sec->getOrDefaultAsF32 ("matrix[6]", 0);
		nodesList[n].localTRS(1,3) = sec->getOrDefaultAsF32 ("matrix[7]", 0);

		nodesList[n].localTRS(2,0) = sec->getOrDefaultAsF32 ("matrix[8]", 0);
		nodesList[n].localTRS(2,1) = sec->getOrDefaultAsF32 ("matrix[9]", 0);
		nodesList[n].localTRS(2,2) = sec->getOrDefaultAsF32 ("matrix[10]", 0);
		nodesList[n].localTRS(2,3) = sec->getOrDefaultAsF32 ("matrix[11]", 0);

		nodesList[n].localTRS(3,0) = sec->getOrDefaultAsF32 ("matrix[12]", 0);
		nodesList[n].localTRS(3,1) = sec->getOrDefaultAsF32 ("matrix[13]", 0);
		nodesList[n].localTRS(3,2) = sec->getOrDefaultAsF32 ("matrix[14]", 0);
		nodesList[n].localTRS(3,3) = sec->getOrDefaultAsF32 ("matrix[15]", 0);

		mat3x3f matRot;
		nodesList[n].localTRS.extractRotationMatrix(&matRot);
		nodesList[n].localRot.buildFromMatrix3x3(matRot);

/*		nodesList[n].localTRS(0,0) = sec->getOrDefaultAsF32 ("matrix[0]", 0);
		nodesList[n].localTRS(1,0) = sec->getOrDefaultAsF32 ("matrix[1]", 0);
		nodesList[n].localTRS(2,0) = sec->getOrDefaultAsF32 ("matrix[2]", 0);
		nodesList[n].localTRS(3,0) = sec->getOrDefaultAsF32 ("matrix[3]", 0);

		nodesList[n].localTRS(0,1) = sec->getOrDefaultAsF32 ("matrix[4]", 0);
		nodesList[n].localTRS(1,1) = sec->getOrDefaultAsF32 ("matrix[5]", 0);
		nodesList[n].localTRS(2,1) = sec->getOrDefaultAsF32 ("matrix[6]", 0);
		nodesList[n].localTRS(3,1) = sec->getOrDefaultAsF32 ("matrix[7]", 0);

		nodesList[n].localTRS(0,2) = sec->getOrDefaultAsF32 ("matrix[8]", 0);
		nodesList[n].localTRS(1,2) = sec->getOrDefaultAsF32 ("matrix[9]", 0);
		nodesList[n].localTRS(2,2) = sec->getOrDefaultAsF32 ("matrix[10]", 0);
		nodesList[n].localTRS(3,2) = sec->getOrDefaultAsF32 ("matrix[11]", 0);

		nodesList[n].localTRS(0,3) = sec->getOrDefaultAsF32 ("matrix[12]", 0);
		nodesList[n].localTRS(1,3) = sec->getOrDefaultAsF32 ("matrix[13]", 0);
		nodesList[n].localTRS(2,3) = sec->getOrDefaultAsF32 ("matrix[14]", 0);
		nodesList[n].localTRS(3,3) = sec->getOrDefaultAsF32 ("matrix[15]", 0);
*/		
	}
	else
	{
		vec3f v;
		Quat q;

		mat4x4f matT;
		v.set (0,0,0);
		if (sec->exists ("translation[0]"))
		{
			v.x = sec->getOrDefaultAsF32 ("translation[0]", 0);
			v.y = sec->getOrDefaultAsF32 ("translation[1]", 0);
			v.z = sec->getOrDefaultAsF32 ("translation[2]", 0);
		}
		matT.buildTranslation (v);

		mat4x4f matS;
		v.set (1,1,1);
		if (sec->exists ("scale[0]"))
		{
			v.x = sec->getOrDefaultAsF32 ("scale[0]", 1);
			v.y = sec->getOrDefaultAsF32 ("scale[1]", 1);
			v.z = sec->getOrDefaultAsF32 ("scale[2]", 1);
		}
		matS.buildScale (v);

		
		mat4x4f matR;
		if (sec->exists ("rotation[0]"))
		{
			nodesList[n].localRot.x = sec->getOrDefaultAsF32 ("rotation[0]", 0);
			nodesList[n].localRot.y = sec->getOrDefaultAsF32 ("rotation[1]", 0);
			nodesList[n].localRot.z = sec->getOrDefaultAsF32 ("rotation[2]", 0);
			nodesList[n].localRot.w = sec->getOrDefaultAsF32 ("rotation[3]", 1);
		}
		nodesList[n].localRot.toMatrix4x4 (&matR);

		nodesList[n].localTRS = matT * (matR * matS);
	}


	//vediamo se ha figli
	u32 index = 0;
	while (1)
	{
		char s[32];
		sprintf_s (s, sizeof(s), "children[%d]", index++);
		const u32 childIndex = sec->getOrDefaultAsU32 (s, u32MAX);
		if (u32MAX == childIndex)
			break;			
		nodesList[n].addChild (childIndex);
	}

	return true;
}

//********************************************
bool glTFImporter::priv_parseScene (const gos::IniFileSection *sec, Bone *me)
{
	me->reset();

	u32 index = 0;
	while (1)
	{
		char s[32];
		sprintf_s (s, sizeof(s), "nodes[%d]", index++);
		u32 nodeIndex = sec->getOrDefaultAsU32 (s, u32MAX);
		if (u32MAX == nodeIndex)
			break;

		//bo trovato un figlio
		Bone *bone = GOSNEW(localAllocator, Bone)();
		bone->reset();
		bone->nodeIndex = nodeIndex;

		me->addAsChild(bone);
	}

	//creo lo scheletro
	Bone *bone = me->firstChild;
	while (bone)
	{
		priv_resolveNodesHierarcy(bone);
		bone = bone->nextSibling;
	}

	return true;
}

//********************************************
void glTFImporter::priv_resolveNodesHierarcy (Bone *me)
{
	assert (u32MAX != me->nodeIndex);

	const u32 ii = me->nodeIndex;
	for (u32 i=0; i<nodesList(ii).numChildren; i++)
	{
		Bone *bone = GOSNEW(localAllocator, Bone)();
		bone->reset();
		bone->nodeIndex = nodesList(ii).childrenList[i];
		me->addAsChild(bone);
	}

	Bone *bone = me->firstChild;
	while (bone)
	{
		priv_resolveNodesHierarcy(bone);
		bone = bone->nextSibling;
	}
}

//********************************************
void glTFImporter::priv_resolveSkeleton (Bone *rootBone)
{
	assert (NULL == rootBone->nextSibling);
	assert (u32MAX == rootBone->nodeIndex);

	Bone *b = rootBone->firstChild;
	while (b)
	{
		priv_resolveSkeletonChildren (b, rootBone);
		b = b->nextSibling;
	}
}

//********************************************
void glTFImporter::priv_resolveSkeletonChildren (Bone *me, const Bone *father)
{
	if (u32MAX != me->nodeIndex)
	{
		me->globalTRS = nodesList(me->nodeIndex).localTRS;
		me->globalRot = nodesList(me->nodeIndex).localRot;;
	}
	
	me->globalTRS = father->globalTRS * me->globalTRS;
	me->globalRot = father->globalRot * me->globalRot;

	Bone *b = me->firstChild;
	while (b)
	{
		priv_resolveSkeletonChildren (b, me);
		b = b->nextSibling;
	}
}

//********************************************
void glTFImporter::priv_applySkeleton (Bone *me)
{
	if (u32MAX != me->nodeIndex)
	{
		const u32 ii = nodesList(me->nodeIndex).meshIndex;
		if (u32MAX != ii)
		{
			//il nodo di questa scena si applica alla mesh ii
			//La mesh ii a sua volta e' composta da n shapes
			for (u32 i=0; i<meshesList(ii).getNumShapes(); i++)
			{
				const u16 shapeIdx = meshesList(ii).getShapeIndex(i);
				shape::shapeTransformPos (&shapeOut.shapeList->getElem(shapeIdx), me->globalTRS);
				shape::shapeRotateNormals (&shapeOut.shapeList->getElem(shapeIdx), me->globalRot);
			}
		}
	}

	Bone *b = me->firstChild;
	while (b)
	{
		priv_applySkeleton (b);
		b = b->nextSibling;
	}

}

//********************************************
bool glTFImporter::importFromFile (const char *filename, const VtxLayout &desiredLayout, gos::Allocator *shapeAllocator, FastArray<Shape> &out_shapeList)
{
	u32 fsize;
	u8 *buffer = fs::fileLoadInMemory (localAllocator, filename, &fsize);
	if (NULL == buffer)
	{
		logger::verbose ("glTFImporter::importFromFile(%s) => file not found\n", filename);
		return false;
	}

	const bool ret = importFromMemory (buffer, fsize, desiredLayout, shapeAllocator, out_shapeList);
	GOSFREE(localAllocator, buffer);
	return ret;
}

//********************************************
bool glTFImporter::importFromMemory (const u8 *buffer, u32 sizeof_buffer, const VtxLayout &desiredLayout, gos::Allocator *shapeAllocator, FastArray<Shape> &out_shapeList)
{
	priv_free();
	timeStarted_msec = gos::getTimeSinceStart_msec();


	shapeOut.shapeAllocator = shapeAllocator;
	shapeOut.vtxLayot = desiredLayout;
	shapeOut.shapeList = &out_shapeList;

	if (sizeof_buffer < 20)
	{
		gos::logger::err ("glTFImporter::importFromMemory() => file size is too small\n");
		return false;
	}

	u32 ct = 0;

	sHeader header;
	header.magic = utils::bufferReadU32_LSB_MSB (&buffer[ct]); ct+=4;
	header.version = utils::bufferReadU32_LSB_MSB (&buffer[ct]); ct+=4;
	header.length = utils::bufferReadU32_LSB_MSB (&buffer[ct]); ct+=4;

	if (0x46546C67 != header.magic)
	{
		gos::logger::err ("glTFImporter::importFromMemory() => invalid file signature\n");
		return false;
	}

	if (0x02 != header.version)
	{
		gos::logger::err ("glTFImporter::importFromMemory() => unsupported .glb file version. Version is %d, supported version is 2\n", header.version);
		return false;
	}


	//il primo chunk e' sempre il JSON
	const u32 jsonChunkLen = utils::bufferReadU32_LSB_MSB (&buffer[ct]);
	{
		ct+=4;
		
		const u32 chunkType = utils::bufferReadU32_LSB_MSB (&buffer[ct]);
		ct+=4;
		if (0x4E4F534A != chunkType)
		{
			gos::logger::err ("glTFImporter::importFromMemory() => invalid chunk 0. It should be a JSON chunk\n");
			return false;
		}

		json = reinterpret_cast<const char*>(&buffer[ct]);
		ct+=jsonChunkLen;		
	}

	//il secondo blocco e' sempre un blocco bin
	const u32 binChunkLen = utils::bufferReadU32_LSB_MSB (&buffer[ct]);
	{
		ct+=4;
		
		const u32 chunkType = utils::bufferReadU32_LSB_MSB (&buffer[ct]);
		ct+=4;
		if (0x004E4942 != chunkType)
		{
			gos::logger::err ("glTFImporter::importFromMemory() => invalid chunk 1. It should be a BIN chunk\n");
			return false;
		}

		bin = &buffer[ct];
		ct+=binChunkLen;		
	}

#ifdef _DEBUG
	{
		gos::File hFile;
		fs::fileOpenForW (&hFile, "@w/debug_imported_glTF.json");
		fs::fileWrite (hFile, json, jsonChunkLen);
		fs::fileClose (hFile);
	}
#endif


	gos::IniFile ini;
	if (!ini.fromJSon (json, jsonChunkLen))
	{
		gos::logger::err ("glTFImporter::importFromMemory() => unable to parse json block\n");
		return false;
	}
#ifdef _DEBUG
	ini.saveAs ("@w/debug_imported_glTF.ini");
#endif


	//verifichiamo la versione
	if (!ini.checkString ("asset.version", "2.0"))
	{
		gos::logger::err ("glTFImporter::importFromMemory() => unsupported version in asset.version\n");
		return false;
	}

	//recupero tutti i buffer view
	char s[512];
	u32 index = 0;
	{
		while (1)
		{
			sprintf_s (s, sizeof(s), "bufferViews[%d]", index);
			const gos::IniFileSection *sec = ini.getSubsection (s);
			if (NULL == sec)
				break;
			
			if (!priv_parseBufferView (sec))
			{
				gos::logger::err ("glTFImporter::importFromMemory() => error parsing 'bufferView[%d]\n", index);
				return false;
			}		
			++index;
		}
	}

	//recupero tutti gli accessor
	index = 0;
	{
		while (1)
		{
			sprintf_s (s, sizeof(s), "accessors[%d]", index);
			const gos::IniFileSection *sec = ini.getSubsection (s);
			if (NULL == sec)
				break;
			
			if (!priv_parseAccessor (sec))
			{
				gos::logger::err ("glTFImporter::importFromMemory() => error parsing 'accessors[%d]\n", index);
				return false;
			}		
			++index;
		}
	}



	//recupero le mesh
	index = 0;
	{
		while (1)
		{
			sprintf_s (s, sizeof(s), "meshes[%d]", index);
			const gos::IniFileSection *sec = ini.getSubsection (s);
			if (NULL == sec)
				break;
			
			if (!priv_parseMesh (sec))
			{
				gos::logger::err ("glTFImporter::importFromMemory() => error parsing 'mesh[%d]\n", index);
				return false;
			}		
			++index;
		}

		//glTF e' right handed, devo convertire tutto
		for (u32 i=0; i<out_shapeList.getNElem(); i++)
		{
			shape::Shape *myShape = &out_shapeList[i];
			shape::shapeRightHandedToLeftHanded (myShape);
		}

	}

	//parsing dei nodes
	index = 0;
	{
		while (1)
		{
			sprintf_s (s, sizeof(s), "nodes[%d]", index);
			const gos::IniFileSection *sec = ini.getSubsection (s);
			if (NULL == sec)
				break;
			
			if (!priv_parseNodes (sec))
			{
				gos::logger::err ("glTFImporter::importFromMemory() => error parsing 'nodes[%d]\n", index);
				return false;
			}		
			++index;
		}
	}	

	//analizzo la scena 0
	{
		const gos::IniFileSection *sec = ini.getSubsection ("scenes[0]");
		if (NULL == sec)
		{
			gos::logger::err ("glTFImporter::importFromMemory() => can't find scene[0]\n");
			return false;
		}

		if (!priv_parseScene (sec, &rootBone))
		{
			gos::logger::err ("glTFImporter::importFromMemory() => error parsing 'scene[0]\n");
			return false;
		}

		priv_resolveSkeleton (&rootBone);
		priv_applySkeleton (&rootBone);
	}


	/*glTF e' right handed, devo convertire tutto
	for (u32 i=0; i<out_shapeList.getNElem(); i++)
	{
		shape::Shape *myShape = &out_shapeList[i];
		shape::shapeRightHandedToLeftHanded (myShape);
	}
	*/



#ifdef GOS__glTF_VERBOSE
	const f32 timeElapsed_msec = static_cast<f32>(gos::getTimeSinceStart_msec() - timeStarted_msec);
	gos::logger::verbose ("shape::glTFImporter() => time elapsed %.1f s\n\n", timeElapsed_msec / 1000.0f);

	priv_printStatistics();
#endif


	return true;
}


//********************************************
void glTFImporter::priv_printStatistics() const
{
	logger::log (eTextColor::green, "===================================================\n");
	logger::log (eTextColor::green, "glTFImporter::printStatistics()\n");
	logger::incIndent();

	const u32 numShapes = shapeOut.shapeList->getNElem();
	logger::log ("num shapes: %d\n", numShapes);

	u32 totNumVtx = 0;
	u32 totNumIdx = 0;
	for (u32 i=0; i<numShapes; i++)
	{
		const u32 numVtx = shapeOut.shapeList->queryElem(i).numVtx;
		const u32 numIdx = shapeOut.shapeList->queryElem(i).numIdx;
		logger::log ("shape #%d,  vtx=%d, idx=%d\n", i, numVtx, numIdx);

		totNumVtx += numVtx;
		totNumIdx += numIdx;
	}

	logger::log ("\n");
	logger::log ("totalNumVtx=%d, totalNumIdx=%d\n", totNumVtx, totNumIdx);
	logger::decIndent();
}