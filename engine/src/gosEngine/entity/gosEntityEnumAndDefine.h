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

		/*************************
		 * @brief	CompPos
		 * 
		 */
		struct CompPos
		{
			gos::vec3f		pos;
			gos::vec3f		rot_grad;
			gos::vec3f		scale;
			gos::mat4x4f	_matrix;

			void 	identity()								{ pos.set(0,0,0); rot_grad.set(0,0,0); scale.set(1.0f, 1.0f, 1.0f); }
			void 	updateMatrix();
		};

		/*************************
		 * @brief	CompScriptable
		 * 
		 */		
		typedef void (*entity_script_function) (Entity ent, Registry *registry);
		struct CompScriptable
		{
			entity_script_function	callback;
		};	

		/*************************
		 * @brief	CompModelInstance
		 * 
		 */		
		struct CompModelInstance
		{
			gos::model::ModelInstance	model_instance;
		};

	
	
	} //namespace ent


} //namespace gos

#endif //_gosEntityEnumAndDefine_h_

