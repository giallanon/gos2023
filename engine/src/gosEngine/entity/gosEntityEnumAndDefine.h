#ifndef _gosEntityEnumAndDefine_h_
#define _gosEntityEnumAndDefine_h_
#include "../../gos/gos.h"

namespace gos
{
	struct Entity
	{
		u32	id;

        bool    operator== (const Entity &b) const                  { return id == b.id; }
        bool    operator!= (const Entity &b) const                  { return id != b.id; }

	};




} //namespace gos

#endif //_gosEntityEnumAndDefine_h_

