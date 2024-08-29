#ifndef _ggosShapeImport_h_
#define _ggosShapeImport_h_
#include "gosShapeEnumAndDefine.h"
#include "../gos/gos.h"

namespace gos
{ 
	namespace shape
	{
		bool 	importFromCollada (const char *filename, gos::Allocator *allocator, Shape *out);


	} //namespace shape
 } //namespace gos

#endif //_ggosShapeImport_h_