#ifndef _gosShapeColladaImporter_h_
#define _gosShapeColladaImporter_h_
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

			bool	importFromFile (const char *filename);
			bool	importFromMemory (const u8 *buffer, u32 sizeof_buffer);

		private:
			void 	priv_free();
			u32 	priv_extractFloatArray3 (const tinyxml2::XMLElement *floatArrayElem, gos::FastArray<vec3f> *dst);
			u32 	priv_extractFloatArray2 (const tinyxml2::XMLElement *floatArrayElem, gos::FastArray<vec2f> *dst);
			u32 	priv_extractTriangles (tinyxml2::XMLElement *trianglesElem, gos::FastArray<u16> *dst);
			u32 	priv_extractPolylist (tinyxml2::XMLElement *polylistElem, gos::FastArray<u16> *dst);

		private:
			gos::Allocator 			*localAllocator;
			gos::FastArray<vec3f>	sourceVtx;
			gos::FastArray<vec3f>	sourceNorm;
			gos::FastArray<vec2f>	sourceTexCoord0;
			gos::FastArray<u16>		idxBuffer;
		};
		
	} //namespace shape
 } //namespace gos

#endif //_gosShapeColladaImporter_h_