#ifndef _gosShapeImport_Collada_h_
#define _gosShapeImport_Collada_h_
#include "gosShape.h"
#include "../gos/gosFastArray.h"

namespace tinyxml2
{
	class XMLElement;
}

namespace gos
{ 
	namespace shape
	{
		class ColladaImporter
		{
		public:
					ColladaImporter();
					~ColladaImporter();

			bool	importFromFile (const char *filename, const VtxLayout &desiredLayout, gos::Allocator *shapeAllocator, gos::ShapeList &out_shapeList);
			bool	importFromMemory (const u8 *buffer, u32 sizeof_buffer, const VtxLayout &desiredLayout, gos::Allocator *shapeAllocator, gos::ShapeList &out_shapeList);

		private:
			struct sFaceInfo
			{
				u8 	numIdxPerTupla;
				u8	offset_pos;
				u8	offset_norm;
				u8	offset_tutv0;

				void reset()	{ numIdxPerTupla=offset_pos=offset_norm=offset_tutv0= 0; }
			};

			struct sTechiqueIndices
			{
			public:
				void 	reset()	{ numIndices = 0; indexX=indexY=indexZ=indexW=0xff; }

			public:
				u8 	numIndices;
				u8	indexX;
				u8	indexY;
				u8	indexZ;
				u8	indexW;
			};

		private:
			void 	priv_free();
			bool 	priv_checkSourceElem (const tinyxml2::XMLElement *sourceElem, u8 NUM_ELEM_PER_ENTRY, sTechiqueIndices *out, const tinyxml2::XMLElement **out_floatArrayElem, u32 *out_count) const;
			bool 	priv_parse_technique_common (const tinyxml2::XMLElement *technique_commonElem, sTechiqueIndices *out) const;
			bool 	priv_extractSourceElem2 (const tinyxml2::XMLElement *sourceElem, gos::FastArray<vec2f> *dst);
			bool 	priv_extractSourceElem3 (const tinyxml2::XMLElement *sourceElem, gos::FastArray<vec3f> *dst);
			bool 	priv_extractSourceElem4 (const tinyxml2::XMLElement *sourceElem, gos::FastArray<vec4f> *dst);
			
			bool 	priv_parse_geometry (const tinyxml2::XMLElement *geometryElem, gos::Allocator *shapeAllocator, Shape *out_shape);

			void	priv_parseInputSemantic (const tinyxml2::XMLElement *inputElem, sFaceInfo *out_info) const;
			u32 	priv_extractFaceInfo (const tinyxml2::XMLElement *trianglesElem, sFaceInfo *out_info, gos::FastArray<u16> *dst);
			u32 	priv_extractFromPolylist (const tinyxml2::XMLElement *polylistElem, sFaceInfo *out_info, gos::FastArray<u16> *dst);

		private:
			gos::Allocator 			*localAllocator;
			VtxLayout				shapeVtxLayout;
			gos::FastArray<vec3f>	sourcePos;
			gos::FastArray<vec3f>	sourceNorm;
			gos::FastArray<vec2f>	sourceTexCoord0;
			gos::FastArray<u16>		faceList;
		};
		
	} //namespace shape
 } //namespace gos

#endif //_gosShapeImport_Collada_h_