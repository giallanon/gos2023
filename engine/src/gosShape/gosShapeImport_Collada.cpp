#include "gosShapeImport_Collada.h"
#include "gosShapeImport_arrays.h"
#include "../gos/gos.h"
#include "../external/tinyxml/tinyxml2.h"

using namespace gos;
using namespace gos::shape;
using namespace tinyxml2;

//********************************************
ColladaImporter::ColladaImporter()
{
	localAllocator = gos::getScrapAllocator();
	sourcePos.setup (localAllocator, 1024);
	sourceNorm.setup (localAllocator, 1024);
	sourceTexCoord0.setup (localAllocator, 1024);
	faceList.setup (localAllocator, 1024);
}

//********************************************
ColladaImporter::~ColladaImporter()
{
	sourcePos.unsetup ();
	sourceNorm.unsetup ();
	sourceTexCoord0.unsetup ();
	faceList.unsetup ();
}

//********************************************
void ColladaImporter::priv_free()
{
}

//********************************************
bool ColladaImporter::importFromFile (const char *filename, const VtxLayout &desiredLayout, gos::Allocator *shapeAllocator, FastArray<Shape> &out_shapeList)
{
	priv_free();

	u32 fsize;
	u8 *buffer = fs::fileLoadInMemory (localAllocator, filename, &fsize);
	if (NULL == buffer)
	{
		logger::verbose ("ColladaImporter::importFromFile(%s) => file not found\n", filename);
		return false;
	}

	const bool ret = importFromMemory (buffer, fsize, desiredLayout, shapeAllocator, out_shapeList);
	GOSFREE(localAllocator, buffer);
	return ret;
}

//********************************************
bool ColladaImporter::priv_parse_technique_common (const XMLElement *technique_commonElem, sTechiqueIndices *out) const
{
	assert (NULL != technique_commonElem);
	assert (NULL != out);
	out->reset();

	const XMLElement *accessorElem = technique_commonElem->FirstChildElement ("accessor");
	if (NULL == accessorElem)
	{
		logger::warn ("ColladaImporter::priv_parse_technique_common() => <accessorElem> is NULL\n");
		return false;
	}

	const u32 stride = gos::string::ansi::toU32 (accessorElem->Attribute("stride", NULL));
	if (0 == stride)
	{
		logger::warn ("ColladaImporter::priv_parse_technique_common() => <accessorElem stride> is 0\n");
		return false;
	}

	const XMLElement *paramElem = accessorElem->FirstChildElement ("param");	
	for (u32 i=0; i<stride; i++)
	{
		if (NULL == paramElem)
		{
			logger::warn ("ColladaImporter::priv_parse_technique_common() => can't find <param> %d-esimo\n", i);
			return false;
		}

		const char *paramName = paramElem->Attribute("name", NULL);
		switch (paramName[0])
		{
		case 'X':
		case 'R':
		case 'S':
			out->indexX = out->numIndices++;
			break;

		case 'Y':
		case 'G':
		case 'T':
			out->indexY = out->numIndices++;
			break;

		case 'Z':
		case 'B':
			out->indexZ = out->numIndices++;
			break;

		case 'W':
		case 'A':
			out->indexW = out->numIndices++;
			break;
		}

		paramElem = paramElem->NextSiblingElement("param");
	}

	return true;
}

//********************************************
bool ColladaImporter::priv_checkSourceElem (const XMLElement *sourceElem, u8 NUM_ELEM_PER_ENTRY, sTechiqueIndices *out, const XMLElement **out_floatArrayElem, u32 *out_count) const
{
	assert (NULL != out);
	out->reset();
	*out_count = 0;

	//<source id="Cube_001-mesh-normals">
	//	<float_array id="Cube_001-mesh-normals-array" count="4236">...</float_array>
	//	<technique_common>
	//		<accessor source="#Cube_001-mesh-normals-array" count="1412" stride="3">
	//		<param name="X" type="float"/>
	//		<param name="Y" type="float"/>
	//		<param name="Z" type="float"/>
	//		</accessor>
	//	</technique_common>
	//</source>

	if (NULL == sourceElem)
	{
		logger::warn ("ColladaImporter::priv_checkSourceElem() => <sourceElem> is NULL\n");
		return false;
	}

	const XMLElement *floatArrayElem = sourceElem->FirstChildElement ("float_array");
	if (NULL == floatArrayElem)
	{
		logger::warn ("ColladaImporter::priv_checkSourceElem() => <floatArrayElem> is NULL\n");
		return false;
	}
	*out_floatArrayElem = floatArrayElem;

	const u32 count = gos::string::ansi::toU32 (floatArrayElem->Attribute("count", NULL));
	if (0 == count)
	{
		logger::warn ("ColladaImporter::priv_checkSourceElem() => <floatArrayElem count> is 0\n");
		return false;
	}
	*out_count = count;

	const XMLElement *technique_commonElem = sourceElem->FirstChildElement ("technique_common");
	if (NULL == technique_commonElem)
	{
		logger::warn ("ColladaImporter::priv_checkSourceElem() => <technique_common> is NULL\n");
		return false;
	}

	//parso il tag <technique_common>
	if (!priv_parse_technique_common (technique_commonElem, out))
	{
		logger::err ("ColladaImporter::priv_checkSourceElem() => error parsing <technique_common> element\n");
		return false;
	}

	if (out->numIndices != NUM_ELEM_PER_ENTRY)
	{
		logger::warn ("ColladaImporter::priv_checkSourceElem() => <technique_common> contains %d <param> tag but I was expecint %d\n", out->numIndices, NUM_ELEM_PER_ENTRY);
		return false;
	}

	return true;
}

//********************************************
bool ColladaImporter::priv_extractSourceElem2 (const XMLElement *sourceElem, gos::FastArray<vec2f> *dst)
{
	//<source id="Cube_001-mesh-normals">
	//	<float_array id="Cube_001-mesh-normals-array" count="4236">...</float_array>
	//	<technique_common>
	//		<accessor source="#Cube_001-mesh-normals-array" count="1412" stride="3">
	//		<param name="X" type="float"/>
	//		<param name="Y" type="float"/>
	//		<param name="Z" type="float"/>
	//		</accessor>
	//	</technique_common>
	//</source>

	const u32 NUM_ELEM_PER_ENTRY = 2;
	sTechiqueIndices techiqueIndices;
	const XMLElement *floatArrayElem;
	u32 count;
	if (!priv_checkSourceElem (sourceElem, NUM_ELEM_PER_ENTRY, &techiqueIndices, &floatArrayElem, &count))
	{
		logger::err ("ColladaImporter::priv_extractSourceElem2() => error parsing <technique_common>\n");
		return false;
	}

	gos::string::utf8::Iter iter;
	iter.setup (floatArrayElem->GetText());
	for (u32 i=0; i<count; i+=NUM_ELEM_PER_ENTRY)
	{
		f32 coord[16];
		u32 n = NUM_ELEM_PER_ENTRY;
		if (gos::string::utf8::extractFloatArray (iter, coord, &n, ".", " "))
			dst->append (vec2f(coord[techiqueIndices.indexX], coord[techiqueIndices.indexY]));
		else
		{
			logger::err ("ColladaImporter::priv_extractSourceElem2() => error parsing <float_array> data\n");
			return false;
		}
	}

	return true;
}

//********************************************
bool ColladaImporter::priv_extractSourceElem3 (const XMLElement *sourceElem, gos::FastArray<vec3f> *dst)
{
	//<source id="Cube_001-mesh-normals">
	//	<float_array id="Cube_001-mesh-normals-array" count="4236">...</float_array>
	//	<technique_common>
	//		<accessor source="#Cube_001-mesh-normals-array" count="1412" stride="3">
	//		<param name="X" type="float"/>
	//		<param name="Y" type="float"/>
	//		<param name="Z" type="float"/>
	//		</accessor>
	//	</technique_common>
	//</source>

	const u32 NUM_ELEM_PER_ENTRY = 3;
	sTechiqueIndices techiqueIndices;
	const XMLElement *floatArrayElem;
	u32 count;
	if (!priv_checkSourceElem (sourceElem, NUM_ELEM_PER_ENTRY, &techiqueIndices, &floatArrayElem, &count))
	{
		logger::err ("ColladaImporter::priv_extractSourceElem3() => error parsing <technique_common>\n");
		return false;
	}

	gos::string::utf8::Iter iter;
	iter.setup (floatArrayElem->GetText());
	for (u32 i=0; i<count; i+=NUM_ELEM_PER_ENTRY)
	{
		f32 coord[16];
		u32 n = NUM_ELEM_PER_ENTRY;
		if (gos::string::utf8::extractFloatArray (iter, coord, &n, ".", " "))
			dst->append (vec3f(coord[techiqueIndices.indexX], coord[techiqueIndices.indexY], coord[techiqueIndices.indexZ]));
		else
		{
			logger::err ("ColladaImporter::priv_extractSourceElem3() => error parsing <float_array> data\n");
			return false;
		}
	}

	return true;
}

//********************************************
bool ColladaImporter::priv_extractSourceElem4 (const XMLElement *sourceElem, gos::FastArray<vec4f> *dst)
{
	const u32 NUM_ELEM_PER_ENTRY = 4;
	sTechiqueIndices techiqueIndices;
	const XMLElement *floatArrayElem;
	u32 count;
	if (!priv_checkSourceElem (sourceElem, NUM_ELEM_PER_ENTRY, &techiqueIndices, &floatArrayElem, &count))
	{
		logger::err ("ColladaImporter::priv_extractSourceElem4() => error parsing <technique_common>\n");
		return false;
	}

	gos::string::utf8::Iter iter;
	iter.setup (floatArrayElem->GetText());
	for (u32 i=0; i<count; i+=NUM_ELEM_PER_ENTRY)
	{
		f32 coord[16];
		u32 n = NUM_ELEM_PER_ENTRY;
		if (gos::string::utf8::extractFloatArray (iter, coord, &n, ".", " "))
			dst->append (vec4f(coord[techiqueIndices.indexX], coord[techiqueIndices.indexY], coord[techiqueIndices.indexZ], coord[techiqueIndices.indexW]));
		else
		{
			logger::err ("ColladaImporter::priv_extractSourceElem4() => error parsing <float_array> data\n");
			return false;
		}
	}

	return true;
}

//********************************************
void  ColladaImporter::priv_parseInputSemantic (const tinyxml2::XMLElement *inputElem, sFaceInfo *out_info) const
{
	out_info->numIdxPerTupla = 0;
	while (inputElem)
	{
		const char *semantic =inputElem->Attribute("semantic", NULL);
		const u8 offset = static_cast<u8> (gos::string::ansi::toU32 (inputElem->Attribute("offset", NULL)));

		if (offset > out_info->numIdxPerTupla)
			out_info->numIdxPerTupla = offset;

		if (strcmp (semantic, "VERTEX") == 0)			out_info->offset_pos = offset;
		else if (strcmp (semantic, "NORMAL") == 0)		out_info->offset_norm = offset;
		else if (strcmp (semantic, "TEXCOORD") == 0)	out_info->offset_tutv0 = offset;


		inputElem = inputElem->NextSiblingElement("input");
	}
	out_info->numIdxPerTupla++;	
}

//********************************************
u32  ColladaImporter::priv_extractFaceInfo (const tinyxml2::XMLElement *trianglesElem, sFaceInfo *out_info, gos::FastArray<u16> *dst)
{
	out_info->reset();
	const u32 numTris = gos::string::ansi::toU32 (trianglesElem->Attribute("count", NULL));
	if (0 == numTris)
		return 0;

	//decodifica il significato dell'elenco di tuple
	priv_parseInputSemantic (trianglesElem->FirstChildElement("input"), out_info);


	const XMLElement *pElem = trianglesElem->FirstChildElement("p");
	if (NULL == pElem)
		return 0;

	gos::string::utf8::Iter iter;
	iter.setup (pElem->GetText());
	for (u32 i=0; i<numTris; i++)
	{
		u32 v[16];

		u32 numV = out_info->numIdxPerTupla;
		gos::string::utf8::extractU32Array (iter, v, &numV, " ");
		for (u32 i2=0; i2<out_info->numIdxPerTupla; i2++)
			dst->append (v[i2]);

		numV = out_info->numIdxPerTupla;
		gos::string::utf8::extractU32Array (iter, v, &numV, " ");
		for (u32 i2=0; i2<out_info->numIdxPerTupla; i2++)
			dst->append (v[i2]);

		numV = out_info->numIdxPerTupla;
		gos::string::utf8::extractU32Array (iter, v, &numV, " ");
		for (u32 i2=0; i2<out_info->numIdxPerTupla; i2++)
			dst->append (v[i2]);
	}
	

	return numTris;
}

//********************************************
u32  ColladaImporter::priv_extractFromPolylist (const tinyxml2::XMLElement *polylistElem, sFaceInfo *out_info, gos::FastArray<u16> *dst)
{
	out_info->reset();
	const u32 numPoly = gos::string::ansi::toU32 (polylistElem->Attribute("count", NULL));
	if (0 == numPoly)
		return 0;

	//decodifica il significato dell'elenco di tuple
	priv_parseInputSemantic (polylistElem->FirstChildElement("input"), out_info);


	const XMLElement *vcountElem = polylistElem->FirstChildElement("vcount");
	if (NULL == vcountElem)
		return 0;
	
	const XMLElement *pElem = polylistElem->FirstChildElement("p");
	if (NULL == pElem)
		return 0;

	gos::string::utf8::Iter iterV;
	iterV.setup (vcountElem->GetText());

	gos::string::utf8::Iter iterP;
	iterP.setup (pElem->GetText());

	for (u32 i=0; i<numPoly; i++)
	{
		u32 numVtx;
		if (gos::string::utf8::extractU32 (iterV, &numVtx))
		{
			u32 v[16];
			u32 numV;
			switch (numVtx)
			{
			default:
				logger::err ("ColladaImporter::priv_extractPolylist() => poly with %d vertex are not supported\n", numVtx);
				break;

			case 3:
				numV = out_info->numIdxPerTupla;
				gos::string::utf8::extractU32Array (iterP, v, &numV, " ");
				for (u32 i2=0; i2<out_info->numIdxPerTupla; i2++)
					dst->append (v[i2]);

				numV = out_info->numIdxPerTupla;
				gos::string::utf8::extractU32Array (iterP, v, &numV, " ");
				for (u32 i2=0; i2<out_info->numIdxPerTupla; i2++)
					dst->append (v[i2]);

				numV = out_info->numIdxPerTupla;
				gos::string::utf8::extractU32Array (iterP, v, &numV, " ");
				for (u32 i2=0; i2<out_info->numIdxPerTupla; i2++)
					dst->append (v[i2]);
				break;
			}
		}
	}
	

	return numPoly;
}

//********************************************
bool ColladaImporter::priv_parse_geometry (const tinyxml2::XMLElement *geomElem, gos::Allocator *shapeAllocator, Shape *out_shape)
{
	const char *geomElemID = geomElem->Attribute("id", NULL);
	const u32 lenOf_geomElemID = (u32)strlen(geomElemID);
	const char *geomName = geomElem->Attribute("name", NULL);
//printf ("geomElem: id=%s, name=%s\n", geomElemID, geomName);

	//tutte le mesh
	const XMLElement *meshElem = geomElem->FirstChildElement ("mesh");
	if (NULL == meshElem)
	{
		gos::logger::verbose ("ColladaImporter::priv_parse_geometry => <geometry name='%s'> does not contains any <mesh> node\n", geomName);
		return false;
	}

	sourcePos.reset ();
	sourceNorm.reset ();
	sourceTexCoord0.reset ();
	faceList.reset ();

	shape::VtxLayoutReader vtxLayoutR(&shapeVtxLayout);

	const bool bShapeWants_pos = vtxLayoutR.exists (eVtxLayoutSemantic::position, 0, eVtxLayoutFormat::_3f32);
	const bool bShapeWants_norm = vtxLayoutR.exists (eVtxLayoutSemantic::normal, 0, eVtxLayoutFormat::_3f32);
	const bool bShapeWants_tutv0 = vtxLayoutR.exists (eVtxLayoutSemantic::texCoord, 0, eVtxLayoutFormat::_2f32);

	//cerco i <source> per recuperare vtx, norm e tutv
	const XMLElement *sourceElem = meshElem->FirstChildElement ("source");
	while (sourceElem)
	{
		const char *sourceID = sourceElem->Attribute("id", NULL);

		if (bShapeWants_pos && strcmp (&sourceID[lenOf_geomElemID], "-positions") == 0)
			priv_extractSourceElem3 (sourceElem, &sourcePos);

		if (bShapeWants_norm && strcmp (&sourceID[lenOf_geomElemID], "-normals") == 0)
			priv_extractSourceElem3 (sourceElem, &sourceNorm);

		if (bShapeWants_tutv0 && strcmp (&sourceID[lenOf_geomElemID], "-map-0") == 0)
			priv_extractSourceElem2 (sourceElem, &sourceTexCoord0);

		sourceElem = sourceElem->NextSiblingElement("source");
	}

	//cerco <triangles> o <polylist>
	sFaceInfo faceInfo;
	faceInfo.reset();

	const XMLElement *elem = meshElem->FirstChildElement ("triangles");
	if (NULL != elem)
		priv_extractFaceInfo (elem, &faceInfo, &faceList);
	else 
	{
		elem = meshElem->FirstChildElement ("polylist");
		if (NULL != elem)
			priv_extractFromPolylist (elem, &faceInfo, &faceList);
	}

	if (0 == faceInfo.numIdxPerTupla)
	{
		gos::logger::verbose ("ColladaImporter::priv_parse_geometry => <geometry name='%s'> does not contains any <triangles> or <polylist> node\n", geomName);
		return false;
	}



	/*report
	{
		printf ("VERTEX\n");
		for (u32 i=0; i<sourcePos.getNElem(); i++)
			printf ("  %03d %.3f %.3f %.3f\n", i, sourcePos(i).x, sourcePos(i).y, sourcePos(i).z);

		printf ("NORMALS\n");
		for (u32 i=0; i<sourceNorm.getNElem(); i++)
			printf ("  %03d %.3f %.3f %.3f\n", i, sourceNorm(i).x, sourceNorm(i).y, sourceNorm(i).z);

		printf ("TEXMAP0\n");
		for (u32 i=0; i<sourceTexCoord0.getNElem(); i++)
			printf ("  %03d %.3f %.3f\n", i, sourceTexCoord0(i).x, sourceTexCoord0(i).y);

		printf ("FACE\n");
		{
			u32 ct = 0;
			for (u32 i=0; i<faceList.getNElem(); )
			{
				printf ("  %03d   ", ct++);
				for (u32 i2=0; i2<3; i2++)
				{
					printf ("(%0d", faceList(i++));
					for (u32 i3=1; i3<faceInfo.numIdxPerTupla; i3++)
					{
						printf (", %0d", faceList(i++));
					}
					printf (") ");
				}
				printf ("\n");
			}
		}
	}
	*/

	//creo la shape
	shape::ArraysImporter imp;
	imp.beginUsingFaceList (shapeVtxLayout, faceList._queryTypedPointer(), faceList.getNElem(), faceInfo.numIdxPerTupla);

	for (u32 i=0; i<vtxLayoutR.getNumElem(); i++)
	{
		switch (vtxLayoutR.getSemantic(i))
		{
		default:
			break;

		case eVtxLayoutSemantic::position:
			if (vtxLayoutR.getIndex(i) == 0)
			{
				if (bShapeWants_pos)
					imp.addImportArray<vec3f> (sourcePos._queryPointer(), faceInfo.offset_pos, eVtxLayoutSemantic::position, 0, eVtxLayoutFormat::_3f32);
			}
			break;

		case eVtxLayoutSemantic::normal:
			if (vtxLayoutR.getIndex(i) == 0)
			{
				if (bShapeWants_norm)
					imp.addImportArray<vec3f> (sourceNorm._queryPointer(), faceInfo.offset_norm, eVtxLayoutSemantic::normal, 0, eVtxLayoutFormat::_3f32);
			}
			break;

		case eVtxLayoutSemantic::texCoord:
			if (vtxLayoutR.getIndex(i) == 0)
			{
				if (bShapeWants_tutv0)
					imp.addImportArray<vec2f> (sourceTexCoord0._queryPointer(), faceInfo.offset_tutv0, eVtxLayoutSemantic::texCoord, 0, eVtxLayoutFormat::_2f32);
			}
			break;
		}
	}

	if (!imp.end (shapeAllocator, out_shape))
	{
		gos::logger::err ("ColladaImporter::priv_parse_geometry => <geometry name='%s'>, error creating shape\n", geomName);
		return false;
	}

//shape::debug_shapePrint (out_shape);

	return true;
}

//********************************************
bool ColladaImporter::importFromMemory (const u8 *buffer, u32 sizeof_buffer, const VtxLayout &desiredLayout, gos::Allocator *shapeAllocator, FastArray<Shape> &out_shapeList)
{
	shapeVtxLayout = desiredLayout;

	tinyxml2::XMLDocument doc;
	if (tinyxml2::XMLError::XML_SUCCESS != doc.Parse (reinterpret_cast<const char*>(buffer), sizeof_buffer))
	{
		logger::err ("ColladaImporter::importFromMemory() => invalid xml file\n");
		return false;
	}

	const XMLElement *colladaElem = doc.FirstChildElement ("COLLADA");
	if (NULL == colladaElem)
	{
		logger::err ("ColladaImporter::importFromMemory() => can't find <COLLADA>\n");
		return false;
	}
 
 	const XMLElement *geomLibElem = colladaElem->FirstChildElement ("library_geometries");
	if (NULL == geomLibElem)
	{
		logger::err ("ColladaImporter::importFromMemory() => can't find <library_geometries>\n");
		return false;
	}

	//scan di tutti i nodi <geometry>
	u32 nShapes = 0;
	const XMLElement *geomElem = geomLibElem->FirstChildElement ("geometry");
	while (geomElem)
	{
		priv_parse_geometry (geomElem, shapeAllocator, &out_shapeList[nShapes]);
		geomElem = geomElem->NextSiblingElement("geometry");
	}

	return true;	
}


