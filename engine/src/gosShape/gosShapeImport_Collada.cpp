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
bool ColladaImporter::importFromFile (const char *filename, gos::Allocator *allocator, Shape *out)
{
	priv_free();

	u32 fsize;
	u8 *buffer = fs::fileLoadInMemory (localAllocator, filename, &fsize);
	if (NULL == buffer)
	{
		logger::verbose ("ColladaImporter::importFromFile(%s) => file not found\n", filename);
		return false;
	}

	const bool ret = importFromMemory (buffer, fsize, allocator,out);
	GOSFREE(localAllocator, buffer);
	return ret;
}

//********************************************
u32 ColladaImporter::priv_extractFloatArray3 (const XMLElement *floatArrayElem, gos::FastArray<vec3f> *dst)
{
	if (NULL == floatArrayElem)
		return 0;

	const u32 count = gos::string::ansi::toU32 (floatArrayElem->Attribute("count", NULL));

	gos::string::utf8::Iter iter;
	iter.setup (floatArrayElem->GetText());
	
	for (u32 i=0; i<count; i++)
	{
		f32 coord[3];
		u32 n = 3;
		if (gos::string::utf8::extractFloatArray (iter, coord, &n, ".", " "))
			dst->append (vec3f(coord[0], coord[1], coord[2]));
	}

	return count;
}

//********************************************
u32 ColladaImporter::priv_extractFloatArray2 (const XMLElement *floatArrayElem, gos::FastArray<vec2f> *dst)
{
	if (NULL == floatArrayElem)
		return 0;

	const u32 count = gos::string::ansi::toU32 (floatArrayElem->Attribute("count", NULL));

	gos::string::utf8::Iter iter;
	iter.setup (floatArrayElem->GetText());
	
	for (u32 i=0; i<count; i++)
	{
		f32 coord[2];
		u32 n = 2;
		if (gos::string::utf8::extractFloatArray (iter, coord, &n, ".", " "))
			dst->append (vec2f(coord[0], coord[1]));
	}

	return count;
}

//********************************************
void  ColladaImporter::priv_parseInputSemantic (tinyxml2::XMLElement *inputElem, sFaceInfo *out_info) const
{
	out_info->numIdxPerTupla = 0;
	while (inputElem)
	{
		const char *semantic =inputElem->Attribute("semantic", NULL);
		const u32 offset = gos::string::ansi::toU32 (inputElem->Attribute("offset", NULL));

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
u32  ColladaImporter::priv_extractFaceInfo (tinyxml2::XMLElement *trianglesElem, sFaceInfo *out_info, gos::FastArray<u16> *dst)
{
	out_info->reset();
	const u32 numTris = gos::string::ansi::toU32 (trianglesElem->Attribute("count", NULL));
	if (0 == numTris)
		return 0;

	//decodifica il significato dell'elenco di tuple
	priv_parseInputSemantic (trianglesElem->FirstChildElement("input"), out_info);


	XMLElement *pElem = trianglesElem->FirstChildElement("p");
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
u32  ColladaImporter::priv_extractFromPolylist (tinyxml2::XMLElement *polylistElem, sFaceInfo *out_info, gos::FastArray<u16> *dst)
{
	out_info->reset();
	const u32 numPoly = gos::string::ansi::toU32 (polylistElem->Attribute("count", NULL));
	if (0 == numPoly)
		return 0;

	//decodifica il significato dell'elenco di tuple
	priv_parseInputSemantic (polylistElem->FirstChildElement("input"), out_info);


	XMLElement *vcountElem = polylistElem->FirstChildElement("vcount");
	if (NULL == vcountElem)
		return 0;
	
	XMLElement *pElem = polylistElem->FirstChildElement("p");
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
bool ColladaImporter::importFromMemory (const u8 *buffer, u32 sizeof_buffer, gos::Allocator *shapeAllocator, Shape *out)
{
	tinyxml2::XMLDocument doc;
	if (tinyxml2::XMLError::XML_SUCCESS != doc.Parse (reinterpret_cast<const char*>(buffer), sizeof_buffer))
	{
		logger::verbose ("ColladaImporter::importFromMemory() => invalid xml file\n");
		return false;
	}

	XMLElement *colladaElem = doc.FirstChildElement ("COLLADA");
	XMLElement *geomLibElem = colladaElem->FirstChildElement ("library_geometries");
	if (NULL == geomLibElem)
	{
		logger::verbose ("ColladaImporter::importFromMemory() => can't find 'COLLADA' node\n");
		return false;
	}


	//scan di tutte le geometrie
	XMLElement *geomElem = geomLibElem->FirstChildElement ("geometry");
	while (geomElem)
	{
		const char *geomElemID = geomElem->Attribute("id", NULL);
		printf ("geomElem: id=%s, name=%s\n",geomElem->Attribute("id", NULL), geomElem->Attribute("name", NULL));

		//tutte le mesh
		XMLElement *meshElem = geomElem->FirstChildElement ("mesh");
		if (NULL != meshElem)
		{
			sourcePos.reset ();
			sourceNorm.reset ();
			sourceTexCoord0.reset ();
			faceList.reset ();


			//cerco i <source> per recuperare vtx, norm e tutv
			XMLElement *sourceElem = meshElem->FirstChildElement ("source");
			while (sourceElem)
			{
				char s[512];

				const char *sourceID = sourceElem->Attribute("id", NULL);

				sprintf_s (s, sizeof(s), "%s-positions", geomElemID);
				if (strcmp (sourceID, s) == 0)
					priv_extractFloatArray3 (sourceElem->FirstChildElement ("float_array"), &sourcePos);

				sprintf_s (s, sizeof(s), "%s-normals", geomElemID);
				if (strcmp (sourceID, s) == 0)
					priv_extractFloatArray3 (sourceElem->FirstChildElement ("float_array"), &sourceNorm);


				sprintf_s (s, sizeof(s), "%s-map-0", geomElemID);
				if (strcmp (sourceID, s) == 0)
					priv_extractFloatArray2 (sourceElem->FirstChildElement ("float_array"), &sourceTexCoord0);

				sourceElem = sourceElem->NextSiblingElement("source");
			}

			//cerco <triangles>
			sFaceInfo faceInfo;
			faceInfo.reset();

			XMLElement *elem = meshElem->FirstChildElement ("triangles");
			if (NULL != elem)
				priv_extractFaceInfo (elem, &faceInfo, &faceList);
			else 
			{
				elem = meshElem->FirstChildElement ("polylist");
				if (NULL != elem)
					priv_extractFromPolylist (elem, &faceInfo, &faceList);
			}


			//report
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

			//creo la shape
			{
				VtxLayout vtxLayout;
				VtxLayoutWriter writer(&vtxLayout);
				writer.begin()
					.addPos3(0)
					.addNorm3(12)
				.end();

				shape::ArraysImporter imp;
				imp.create (vtxLayout, shapeAllocator, out, 
							&faceList, faceInfo.numIdxPerTupla,
							&sourcePos, faceInfo.offset_pos,
							&sourceNorm, faceInfo.offset_norm,
							NULL, faceInfo.offset_tutv0);

				shape::debug_shapePrint (out);
			}

		} // mesh Elem

		geomElem = geomElem->NextSiblingElement("geometry");
	} //geom elem

	return true;	
}


