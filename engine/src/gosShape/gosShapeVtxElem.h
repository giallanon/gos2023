#ifndef _gosShapeVtxElem_h_
#define _gosShapeVtxElem_h_
#include "gosShapeEnumAndDefine.h"

namespace gos
{ 
	namespace shape
	{
		namespace VtxElem
		{
			u32 					define (u32 offset, eVtxLayoutSemantic semantic, u8 index, eVtxLayoutFormat fmt);
			u32 					getOffset (u32 elem);
			eVtxLayoutSemantic		getSemantic(u32 elem);
			u8						getIndex(u32 elem);
			eVtxLayoutFormat		getFormat (u32 elem);
			
			u32 					buildSearchKey (eVtxLayoutSemantic semantic, u8 index, eVtxLayoutFormat fmt);
			bool 					doesKeyMatch (u32 elem, u32 key, u32 *out_offset);
		} //namespace VtxElem

	} //namespace shape
 } //namespace gos

#endif //_gosShapeVtxElem_h_