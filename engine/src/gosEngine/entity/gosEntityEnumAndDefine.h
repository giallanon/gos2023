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

		int 	compare (const Entity &b) const 					{ if(id==b.id) return 0; if (id>b.id) return 1; return -1; }

	public:
		u32	id;
	};


	namespace ent
	{
		class Registry; //Fwd decl

		struct internal__component_unique_index final
		{
			[[nodiscard]] static u32 next() noexcept {
				static u32 value{};
				return value++;
			}
		};

		template<typename COMP>
		struct Component final
		{
			[[nodiscard]] static u32 getTypeIndex() noexcept
			{
				static const u32 value = internal__component_unique_index::next();
				return value;
			}
		};

	
	} //namespace ent


} //namespace gos

#endif //_gosEntityEnumAndDefine_h_

