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

			bool	importFromFile (const char *filename, gos::Allocator *allocator, Shape *out);
			bool	importFromMemory (const u8 *buffer, u32 sizeof_buffer, gos::Allocator *allocator, Shape *out);

		private:
			struct sFaceInfo
			{
				u8 	numIdxPerTupla;
				u8	offset_pos;
				u8	offset_norm;
				u8	offset_tutv0;

				void reset()	{ numIdxPerTupla=offset_pos=offset_norm=offset_tutv0= 0; }
			};

		private:
			void 	priv_free();
			u32 	priv_extractFloatArray3 (const tinyxml2::XMLElement *floatArrayElem, gos::FastArray<vec3f> *dst);
			u32 	priv_extractFloatArray2 (const tinyxml2::XMLElement *floatArrayElem, gos::FastArray<vec2f> *dst);
			
			void	priv_parseInputSemantic (tinyxml2::XMLElement *inputElem, sFaceInfo *out_info) const;
			u32 	priv_extractFaceInfo (tinyxml2::XMLElement *trianglesElem, sFaceInfo *out_info, gos::FastArray<u16> *dst);
			u32 	priv_extractFromPolylist (tinyxml2::XMLElement *polylistElem, sFaceInfo *out_info, gos::FastArray<u16> *dst);

		private:
			gos::Allocator 			*localAllocator;
			gos::FastArray<vec3f>	sourcePos;
			gos::FastArray<vec3f>	sourceNorm;
			gos::FastArray<vec2f>	sourceTexCoord0;
			gos::FastArray<u16>		faceList;
		};
		
	} //namespace shape
 } //namespace gos

#endif //_gosShapeImport_Collada_h_