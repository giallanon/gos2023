#ifndef _gosEntityRegistry_h_
#define _gosEntityRegistry_h_
#include "gosEntityComponentList.h"

namespace gos
{
	namespace ent
	{
		/*******************************************
		 * @brief	Registry
		 * 
		 * 
		 */
		class Registry
		{
        public:
                    Registry()                                          { allocator = NULL; }
                    ~Registry()                                         { priv_free(); }

            void    setup();
            void    unsetup()                                           { priv_free(); }

                            template<class TCOMPONENT>
            bool            addComponentHandler()
            {
                constexpr u32 index = TCOMPONENT::getTypeIndex();
                static_assert (index < NUM_MAX_COMPONENT_PER_ENTITY);

                if (NULL != sparseSetList[index])
                {
                    //sto registrando 2 volte lo stesso componente, oppure il componente
                    //ha un <index> uguale ad un altro componente
                    DBGBREAK;
                    return false;
                }

                using CompSparseSet = ComponentList<TCOMPONENT>;
                CompSparseSet *list = GOSNEW(allocator, CompSparseSet)();
                list->setup(allocator);
                sparseSetList[index] = list;
                return true;
            }

            Entity          newEntity();

                            template<class TCOMPONENT>
            TCOMPONENT*     addComponent(Entity ent)
            {
                constexpr u32 index = TCOMPONENT::getTypeIndex();
                static_assert (index < NUM_MAX_COMPONENT_PER_ENTITY);
                assert (NULL != sparseSetList[index]);

                using CompSparseSet = ComponentList<TCOMPONENT>;
                CompSparseSet *list = (CompSparseSet*)sparseSetList[index];
                return list->addIfNotExists(ent);                
            }

                            template<class TCOMPONENT>
            void            removeComponent(Entity ent)
            {
                constexpr u32 index = TCOMPONENT::getTypeIndex();
                static_assert (index < NUM_MAX_COMPONENT_PER_ENTITY);
                assert (NULL != sparseSetList[index]);

                using CompSparseSet = ComponentList<TCOMPONENT>;
                CompSparseSet *list = (CompSparseSet*)sparseSetList[index];
                list->remove(ent);                
            }

                            template<class TCOMPONENT>
            TCOMPONENT*    get(Entity ent) const
            {
                constexpr u32 index = TCOMPONENT::getTypeIndex();
                static_assert (index < NUM_MAX_COMPONENT_PER_ENTITY);
                assert (NULL != sparseSetList[index]);

                using CompSparseSet = ComponentList<TCOMPONENT>;
                CompSparseSet *list = (CompSparseSet*)sparseSetList[index];
                return list->get(ent);                
            }


        private:
            static constexpr u32 NUM_MAX_COMPONENT_PER_ENTITY = 64;

        private:
            void     priv_free();

        private:
            gos::Allocator  *allocator;
            void            *sparseSetList[NUM_MAX_COMPONENT_PER_ENTITY];
            u32             nextEntID;

        };
    } //namespace ent
} //namespace gos

#endif //_gosEntityRegistry_h_