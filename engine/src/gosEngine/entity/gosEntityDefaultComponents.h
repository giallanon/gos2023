#ifndef _gosEntityDefaultComponents_h_
#define _gosEntityDefaultComponents_h_
#include "gosEntityEnumAndDefine.h"
#include "../model/gosModelInstance.h"

namespace gos
{
	namespace ent
	{
		/*************************
		 * @brief	CompTransform3
		 *
		 */
		struct CompTransform3
		{
			gos::mat4x4f matrix;
		};

		/*************************
		 * @brief	CompPos
		 *
		 */
		struct CompPos
		{
			gos::vec3f		pos;
			gos::vec3f		scale;
			gos::Quat		quat;

			void 	reset()								{ pos.set(0, 0, 0); quat.identity(); scale.set(1.0f, 1.0f, 1.0f); }
			void 	buildMatrix (gos::mat4x4f *out);
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
			gos::ModelInstance	model_instance;
		};


	} //namespace ent
} //namespace gos

#endif //_gosEntityDefaultComponents_h_
