#ifndef _gosEntityEnumAndDefine_h_
#define _gosEntityEnumAndDefine_h_
#include "../../gos/gos.h"
#include "../model/gosModel.h"

namespace gos
{
	struct Entity
	{
	public:
		void  	setInvalid()										{ id = u32MAX; }
		bool 	isValid() const 									{ return u32MAX != id; }
		bool 	isInvalid() const 									{ return u32MAX == id; }

//		operator u32() const 										{ return id; }
        bool    operator== (const Entity &b) const                  { return id == b.id; }
        bool    operator!= (const Entity &b) const                  { return id != b.id; }

	public:
		u32	id;
	};


	namespace ent
	{
		struct CompPos
		{
			static constexpr u32 getTypeIndex() 			{ return 0; }
			
			gos::mat4x4f	matrix;
		};

		struct CompModelInstance
		{
			static constexpr u32 getTypeIndex() 			{ return 1; }
			
			model::ModelInstance	*modelInstance;
		};
	
	
	
	} //namespace ent


} //namespace gos

#endif //_gosEntityEnumAndDefine_h_

