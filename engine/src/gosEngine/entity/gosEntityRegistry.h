#ifndef _gosEntityRegistry_h_
#define _gosEntityRegistry_h_
#include "gosEntityComponentList.h"
#include "gosEntityUniqueList.h"

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
                            Registry();
                            ~Registry()                                         { priv_free(); }

            void            setup();
            void            unsetup()                                           { priv_free(); }

            /****
             * @brief   addComponentHandler
             *          Informa il registry che deve iniziare a gestire componenti di tipo <TCOMPONENT>
             *          Internamente crea uno SparseSet<TCOMPONENT> dove memorizzare i componenti.
             * 
             *          Se <bTrackUpdate> == true, allora crea anche una entity::UniqueList dedicata a questo componente.
             *          La lista in questione tiene traccia di tutte le entita' che hanno get<> o add<> il componente di quel tipo.
             *          La lista delle entita' che hanno modificato il componente e' accessibile tramite getUpdatedEntityList()
             * 
             */
                            template<class TCOMPONENT>
            bool            addComponentHandler(bool bTrackUpdate = false)
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

                if (true == bTrackUpdate)
                {
                    updatedListArray[index] = GOSNEW(allocator, UniqueList)();
                    updatedListArray[index]->setup (allocator, 1024);
                }
                return true;
            }

            Entity          newEntity();

                            template<class TCOMPONENT>
            TCOMPONENT*     addComponent(Entity ent)
            {
                constexpr u32 index = TCOMPONENT::getTypeIndex();
                static_assert (index < NUM_MAX_COMPONENT_PER_ENTITY);
                assert (NULL != sparseSetList[index]);

                //se il componente e' associato ad una lista che ne traccia l'update...
                if (NULL != updatedListArray[index])
                    updatedListArray[index]->insertIfNotExists(ent);

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

            /****
             * @brief   get
             *          ritorna il pt al componente <TCOMPONENT> per l'entita <ent>.
             *          Se il componenete e' stato creato con un Handler che ne traccia gli update, allora
             *          chiamando get<>, questa <ent> viene addata alla rispettiva UpdatedEntityList che tiene traccia
             *          di tutte le entita' che hanno modificato il componenete <TCOMPONENT>
             */            
                            template<class TCOMPONENT>
            TCOMPONENT*     get(Entity ent, bool triggerUpdatedEntityList = true)
            {
                constexpr u32 index = TCOMPONENT::getTypeIndex();
                static_assert (index < NUM_MAX_COMPONENT_PER_ENTITY);
                assert (NULL != sparseSetList[index]);

                //se il componente e' associato ad una lista che ne traccia l'update...
                if (triggerUpdatedEntityList && NULL != updatedListArray[index])
                    updatedListArray[index]->insertIfNotExists(ent);


                using CompSparseSet = ComponentList<TCOMPONENT>;
                CompSparseSet *list = (CompSparseSet*)sparseSetList[index];
                return list->get(ent);                
            }
                                
            /****
             * @brief   query
             *          funziona come get<> solo che non aggiunte <ent> alla lista delle entita' il cui componente <TCOMPONENET>
             *          e' stato modificato
             */            
                              template<class TCOMPONENT>
            const TCOMPONENT* query(Entity ent) const
            {
                constexpr u32 index = TCOMPONENT::getTypeIndex();
                static_assert (index < NUM_MAX_COMPONENT_PER_ENTITY);
                assert (NULL != sparseSetList[index]);

                using CompSparseSet = ComponentList<TCOMPONENT>;
                CompSparseSet *list = (CompSparseSet*)sparseSetList[index];
                return list->get(ent);                
            }            


            /****
             * @brief   getUpdatedEntityList
             *          Posto che <TCOMPONENT> sia associato ad un handler con <bTrackUpdate == true>, allora ritorna
             *          una lista di entity il cui componente <TCOMPONENT> e' stato modificato
             */              
                            template<class TCOMPONENT>
            UniqueList*     getUpdatedEntityList()
            {
                constexpr u32 index = TCOMPONENT::getTypeIndex();
                static_assert (index < NUM_MAX_COMPONENT_PER_ENTITY);
                assert (NULL != updatedListArray[index]);
                return updatedListArray[index];
            }


                            template<class TCOMPONENT>
            ComponentList<TCOMPONENT>*  getAllEntitiesWith()
            {
                constexpr u32 index = TCOMPONENT::getTypeIndex();
                static_assert (index < NUM_MAX_COMPONENT_PER_ENTITY);
                assert (NULL != sparseSetList[index]);

                using CompSparseSet = ComponentList<TCOMPONENT>;
                CompSparseSet *list = (CompSparseSet*)sparseSetList[index];
                return list;
            }   

        private:
            static constexpr u32 NUM_MAX_COMPONENT_PER_ENTITY = 64;

        private:
            void            priv_free();

        private:
            gos::Allocator  *allocator;
            void            *sparseSetList[NUM_MAX_COMPONENT_PER_ENTITY];
            UniqueList      *updatedListArray[NUM_MAX_COMPONENT_PER_ENTITY];
            u32             nextEntID;

        };
    } //namespace ent
} //namespace gos

#endif //_gosEntityRegistry_h_