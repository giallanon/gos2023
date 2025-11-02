#ifndef _gosEntityUniqueList_h_
#define _gosEntityUniqueList_h_
#include "gosEntityEnumAndDefine.h"
#include "../../gos/gosUniqueSortedList.h"


namespace gos
{
	namespace ent
	{
        /*************************************
         * @brief   UniqueList
         *          Mantiene una lista univoca di Entity, ordinate
         * 
         */
        class UniqueList : public UniqueSortedList<gos::Entity>
        {
        public:
                    UniqueList() : UniqueSortedList<gos::Entity>()  { }


            Entity  get (u32 index) const                   { return _queryList()->queryElem(index); }
        };

    } //namespace ent
} //namespace gos        


#endif //_gosEntityUniqueList_h_

