#include "gosShapeColladaImporter.h"
#include "../gos/gos.h"
#include "../external/tinyxml/tinyxml2.h"

using namespace gos;
using namespace gos::shape;
using namespace tinyxml2;

//********************************************
ColladaImporter::ColladaImporter()
{
	localAllocator = gos::getScrapAllocator();
	sourceVtx.setup (localAllocator, 1024);
	sourceNorm.setup (localAllocator, 1024);
	sourceTexCoord0.setup (localAllocator, 1024);
	idxBuffer.setup (localAllocator, 1024);
}

//********************************************
ColladaImporter::~ColladaImporter()
{
	sourceVtx.unsetup ();
	sourceNorm.unsetup ();
	sourceTexCoord0.unsetup ();
	idxBuffer.unsetup ();
}

//********************************************
void ColladaImporter::priv_free()
{
}

//********************************************
bool ColladaImporter::importFromFile (const char *filename)
{
	priv_free();

	u32 fsize;
	u8 *buffer = fs::fileLoadInMemory (localAllocator, filename, &fsize);
	if (NULL == buffer)
	{
		logger::verbose ("ColladaImporter::importFromFile(%s) => file not found\n", filename);
		return false;
	}

	const bool ret = importFromMemory (buffer, fsize);
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
u32  ColladaImporter::priv_extractTriangles (tinyxml2::XMLElement *trianglesElem, gos::FastArray<u16> *dst)
{
	const u32 count = gos::string::ansi::toU32 (trianglesElem->Attribute("count", NULL));
	if (0 == count)
		return 0;

	u32 numElemPerTris = 0;
	u32 offsetVtx = 0;
	u32 offsetNorm = 0;
	u32 offsetTexCoord = 0;
	XMLElement *inputElem = trianglesElem->FirstChildElement("input");
	while (inputElem)
	{
		const char *semantic =inputElem->Attribute("semantic", NULL);
		const u32 offset = gos::string::ansi::toU32 (inputElem->Attribute("offset", NULL));

		if (offset > numElemPerTris)
			numElemPerTris = offset;

		if (strcmp (semantic, "VERTEX") == 0)			offsetVtx = offset;
		else if (strcmp (semantic, "NORMAL") == 0)		offsetNorm = offset;
		else if (strcmp (semantic, "TEXCOORD") == 0)	offsetTexCoord = offset;


		inputElem = inputElem->NextSiblingElement("input");
	}
	numElemPerTris++;


	XMLElement *pElem = trianglesElem->FirstChildElement("p");
	if (NULL == pElem)
		return 0;

	gos::string::utf8::Iter iter;
	iter.setup (pElem->GetText());
	for (u32 i=0; i<count; i++)
	{
		u32 v[16];
		u32 n = numElemPerTris;
		if (gos::string::utf8::extractU32Array (iter, v, &n, " "))
			dst->append (v[offsetVtx]);
		if (gos::string::utf8::extractU32Array (iter, v, &n, " "))
			dst->append (v[offsetVtx]);
		if (gos::string::utf8::extractU32Array (iter, v, &n, " "))
			dst->append (v[offsetVtx]);
	}
	

	return count;
}

//********************************************
u32  ColladaImporter::priv_extractPolylist (tinyxml2::XMLElement *polylistElem, gos::FastArray<u16> *dst)
{
	const u32 count = gos::string::ansi::toU32 (polylistElem->Attribute("count", NULL));
	if (0 == count)
		return 0;

	u32 numElemPerTris = 0;
	u32 offsetVtx = 0;
	u32 offsetNorm = 0;
	u32 offsetTexCoord = 0;
	XMLElement *inputElem = polylistElem->FirstChildElement("input");
	while (inputElem)
	{
		const char *semantic =inputElem->Attribute("semantic", NULL);
		const u32 offset = gos::string::ansi::toU32 (inputElem->Attribute("offset", NULL));

		if (offset > numElemPerTris)
			numElemPerTris = offset;

		if (strcmp (semantic, "VERTEX") == 0)			offsetVtx = offset;
		else if (strcmp (semantic, "NORMAL") == 0)		offsetNorm = offset;
		else if (strcmp (semantic, "TEXCOORD") == 0)	offsetTexCoord = offset;


		inputElem = inputElem->NextSiblingElement("input");
	}
	numElemPerTris++;


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

	for (u32 i=0; i<count; i++)
	{
		u32 numVtx;
		if (gos::string::utf8::extractU32 (iterV, &numVtx))
		{
			u32 v[16];
			u32 n = numElemPerTris;
			switch (numVtx)
			{
			default:
				logger::err ("ColladaImporter::priv_extractPolylist() => poly with %d vertex are not supported\n", numVtx);
				break;

			case 3:
				if (gos::string::utf8::extractU32Array (iterP, v, &n, " "))
					dst->append (v[offsetVtx]);
				if (gos::string::utf8::extractU32Array (iterP, v, &n, " "))
					dst->append (v[offsetVtx]);
				if (gos::string::utf8::extractU32Array (iterP, v, &n, " "))
					dst->append (v[offsetVtx]);
				break;
			}
		}
	}
	

	return count;
}

//********************************************
bool ColladaImporter::importFromMemory (const u8 *buffer, u32 sizeof_buffer)
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
			sourceVtx.reset ();
			sourceNorm.reset ();
			sourceTexCoord0.reset ();
			idxBuffer.reset ();


			//cerco i <source> per recuperare vtx, norm e tutv
			XMLElement *sourceElem = meshElem->FirstChildElement ("source");
			while (sourceElem)
			{
				char s[512];

				const char *sourceID = sourceElem->Attribute("id", NULL);

				sprintf_s (s, sizeof(s), "%s-positions", geomElemID);
				if (strcmp (sourceID, s) == 0)
					priv_extractFloatArray3 (sourceElem->FirstChildElement ("float_array"), &sourceVtx);

				sprintf_s (s, sizeof(s), "%s-normals", geomElemID);
				if (strcmp (sourceID, s) == 0)
					priv_extractFloatArray3 (sourceElem->FirstChildElement ("float_array"), &sourceNorm);


				sprintf_s (s, sizeof(s), "%s-map-0", geomElemID);
				if (strcmp (sourceID, s) == 0)
					priv_extractFloatArray2 (sourceElem->FirstChildElement ("float_array"), &sourceTexCoord0);

				sourceElem = sourceElem->NextSiblingElement("source");
			}

			//cerco <triangles>
			XMLElement *elem = meshElem->FirstChildElement ("triangles");
			if (NULL != elem)
				priv_extractTriangles (elem, &idxBuffer);
			else 
			{
				elem = meshElem->FirstChildElement ("polylist");
				if (NULL != elem)
					priv_extractPolylist (elem, &idxBuffer);
			}


			//report
			printf ("VERTEX\n");
			for (u32 i=0; i<sourceVtx.getNElem(); i++)
				printf ("  %03d %.3f %.3f %.3f\n", i, sourceVtx(i).x, sourceVtx(i).y, sourceVtx(i).z);

			printf ("NORMALS\n");
			for (u32 i=0; i<sourceNorm.getNElem(); i++)
				printf ("  %03d %.3f %.3f %.3f\n", i, sourceNorm(i).x, sourceNorm(i).y, sourceNorm(i).z);

			printf ("TEXMAP0\n");
			for (u32 i=0; i<sourceTexCoord0.getNElem(); i++)
				printf ("  %03d %.3f %.3f\n", i, sourceTexCoord0(i).x, sourceTexCoord0(i).y);

			printf ("TRIS\n");
			for (u32 i=0; i<idxBuffer.getNElem(); i+=3)
				printf ("  %03d %d %d %d\n", i/3, idxBuffer(i), idxBuffer(i+1), idxBuffer(i+2));

		} // mesh Elem

		geomElem = geomElem->NextSiblingElement("geometry");
	} //geom elem

	return true;	
}


