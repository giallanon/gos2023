#ifndef _gosEntityComponentList_h_
#define _gosEntityComponentList_h_
#include "gosEntityEnumAndDefine.h"
#include "../../gos/gosSparseSet.h"

namespace gos
{
	namespace ent
	{
		/*******************************************
		 * @brief	ComponentList
		 * 
		 * 
		 */
		class IComponentList
		{
		public:
			virtual 		~IComponentList()	{ }
			virtual void 	unsetup() = 0;
		};


		template<class DATA>
		class ComponentList : public IComponentList, public gos::SparseSet<DATA, 2048, 1024>
		{
			using TSparseSet = gos::SparseSet<DATA, 2048, 1024>;

		public:
					ComponentList()																{ }

			void 	unsetup()																	{ TSparseSet::unsetup(); }

			DATA*	addIfNotExists (gos::Entity ent)											{ return TSparseSet::addIfNotExists (ent.id); }
			void 	remove (gos::Entity ent)													{ TSparseSet::remove (ent.id); }
			DATA*	get (gos::Entity ent) const													{ return TSparseSet::get (ent.id); }


			bool	next (SparseSetIter &iter, DATA **out, gos::Entity *out_ent) const			{ return TSparseSet::next (iter, out, &out_ent->id); }
		};
		
		
	} //namespace ent
} //namespace gos


#endif //_gosEntityComponentList_h_


