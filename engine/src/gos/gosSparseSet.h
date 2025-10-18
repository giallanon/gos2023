#ifndef _gosSparseSet_h_
#define _gosSparseSet_h_
#include "gosHashMap.h"


namespace gos
{
    /*******************************************
     * @brief	SparseSet
     *          Mantiene una lista di oggetti di tipo <DATA>, ciascuno associato ad un index di tipo u32.
     *          Il <denseList> e' un array che ha tutti i <DATA> uno dietro l'altro, senza buchi.
     *          Il <sparseSet> e' un lookup che associa ogni <INDEX> ad un preciso slot di <denseList>
     * 
     *          Ad esempio:
     *              add (1000);
     *              add (2000);
     *              add (3000);
     * 
     *              In <denseList> ci sono 3 elementi di tipo <DATA> in posizione 0,1,2.
     *              SparseSet invece e' grossa almeno 3000 elementi e:
     *                  sparseSet[1000] = 0;
     *                  sparseSet[2000] = 1;
     *                  sparseSet[3000] = 2;
     * 
     *          Per risparmiare memoria, <sparseSet> e' paginato in pagine di <NUM_DATA_PER_PAGE> elementi.
     *          Le pagine vengono create e rimosse automaticamente.
     * 
     *          add(), remove() e get() sono tutte O(1)
     * 
     *          ATTENZIONE: dato che <denseList> e' un gos::FastArray, operazioni di add() e remove() possono invalidare
     *                      eventuali puntatori a <DATA> ottenuti in precedenza (dato che il FastArray e' libero di espandesi e contrarsi
     *                      alla bisogna, riorganizzando eventualmente i dati al suo interno).
     *                      Una possibile soluzione potrebbe essere quella di paginare anche denseList e di non eliminare i <DATA> rimossi ma semplicemente
     *                      marcarli come non validi (in questo modo, tramite <SparseSetIter>, sarebbe sempre cmq possibile iterare tutti gli elementi
     *                      di <denseSet> saltando automaticamente gli elementi rimossi
     */
    struct SparseSetIter
    {
        u32 curIndex;
    };    

    template<class DATA, u32 NUM_DATA_PER_PAGE, u32 INITIAL_DENSELIST_NUM_ELEMENT>
    class SparseSet
    {
    private:
        static constexpr u32 PAGE_SIZE_IN_BYTE = sizeof(u32) * NUM_DATA_PER_PAGE;

    public:
                    SparseSet()									{ allocator = NULL; }
                    ~SparseSet()								{ unsetup(); }

        void		setup (gos::Allocator *allocatorIN)
        {
            allocator = allocatorIN;
            pageList.setup (allocator, 32);
            denseList.setup (allocator, INITIAL_DENSELIST_NUM_ELEMENT);
        }

        void 		unsetup()
        {
            if (NULL == allocator)
                return;
            reset();
            denseList.unsetup();
            pageList.unsetup();
            
            allocator = NULL;
        }

        void		reset()
        {
            denseList.reset();

            pageList.forEach( [lambdaAllocator = this->allocator] (u32 key, const sPage value) 
            {
                GOSFREE(lambdaAllocator, value.indicesList);
                return true;
            });
            pageList.reset();
        }

        DATA*		addIfNotExists (u32 dataIndexIN)
        {
            u32 pageIndex, offset;
            priv_calcPageIndexAndOffset (dataIndexIN, &pageIndex, &offset);

            //recupero (o creo) la pagina appropriata
            typename HashedPageList::Position pos; 
            if (!pageList.findPosition (pageIndex, &pos))
            {
                //creo nuova pagina
                sPage newPage;
                newPage.numElements = 0;
                newPage.indicesList = GOSALLOCT(u32*, allocator, PAGE_SIZE_IN_BYTE);
                memset (newPage.indicesList, 0xFF, PAGE_SIZE_IN_BYTE);
                pageList.insertInPosition (pos, newPage);
            }
            
            sPage *page = pageList.getValueAtPos (pos);
            const u32 dataIndexInDenseArray = page->indicesList[offset];
            if (u32MAX == dataIndexInDenseArray)
            {
                //lo slot e' disponibile
                assert (page->numElements < NUM_DATA_PER_PAGE);

                const u32 n = denseList.getNElem();
                page->numElements++;
                page->indicesList[offset] = n;
                
                denseList[n].index = dataIndexIN;
                return &denseList.getElem(n).data;
            }

            //lo slot era occupato
            return NULL;
        }

        void 		remove (u32 dataIndexIN)
        {
            u32 pageIndex, offset;
            sPage *page = priv_findPageAndOffset (dataIndexIN, &pageIndex, &offset);
            if (NULL != page)
            {
                const u32 dataIndexInDenseArray = page->indicesList[offset];
                if (u32MAX != dataIndexInDenseArray)
                {
                    //lo slot era effettivamente occupato
                    assert (page->numElements > 0);

                    //elimino DATA da denseList swappandolo con l'ultimo elemento
                    denseList.removeAndSwapWithLast(dataIndexInDenseArray);

                    //aggiorno page ed eventualmente la elimino
                    page->numElements--;
                    page->indicesList[offset] = u32MAX;
                    if (0 == page->numElements)
                    {
                        //elimino anche la pagina
                        GOSFREE(allocator, page->indicesList);
                        pageList.remove (pageIndex);
                    }

                    //dato che ho swappato l'elemento eliminato con l'ultimo elemento del denseArray,
                    //devo aggiornare lo sparseArray per farlo puntare all'elemento swappato
                    if (denseList.getNElem() > dataIndexInDenseArray)
                    {
                        //indice del nuovo DATA che ha preso il posto del vecchio
                        const u32 swappedDataIndex = denseList(dataIndexInDenseArray).index;
                        page = priv_findPageAndOffset (swappedDataIndex, &pageIndex, &offset);
                        assert (NULL != page);
                        page->indicesList[offset] = dataIndexInDenseArray;
                    }
                }
            }
        }

        DATA*		get (u32 dataIndexIN) const
        {
            u32 pageIndex, offset;
            sPage *page = priv_findPageAndOffset (dataIndexIN, &pageIndex, &offset);
            if (NULL != page)
            {
                const u32 dataIndexInDenseArray = page->indicesList[offset];
                if (u32MAX != dataIndexInDenseArray)
                    return &denseList.getElem(dataIndexInDenseArray).data;
            }
            return NULL;
        }


        void        toStart (SparseSetIter *out_iter) const              { out_iter->curIndex = 0; };
        bool        next (SparseSetIter &iter, DATA **out) const
        {
            if (iter.curIndex >= denseList.getNElem())
                return false;
            *out = &denseList.getElem(iter.curIndex++).data;
            return true;
        }
        bool        next (SparseSetIter &iter, DATA **out, u32 *out_index) const
        {
            if (iter.curIndex >= denseList.getNElem())
                return false;
            *out_index = denseList.getElem(iter.curIndex).index;
            *out = &denseList.getElem(iter.curIndex++).data;
            return true;
        }


        u32         debug_denseList_getNumElem() const          { return denseList.getNElem(); }
        u32         debug_pageList_getNumElem() const           { return pageList.getNElem(); }


    private:
        struct sElem
        {
            DATA	data;
            u32     index;
        };

        struct sPage
        {
            u32     *indicesList;
            u32     numElements;
        };

        typedef gos::HashMap<u32, sPage>	HashedPageList;		

     private:
        void	priv_calcPageIndexAndOffset (u32 index, u32 *out_pageIndex, u32 *out_offset) const              { (*out_pageIndex) = index / NUM_DATA_PER_PAGE; (*out_offset) = index % NUM_DATA_PER_PAGE; }
        sPage*  priv_findPageAndOffset (u32 dataIndexIN, u32 *out_pageIndex, u32 *out_offset) const
        {
            priv_calcPageIndexAndOffset (dataIndexIN, out_pageIndex, out_offset);

            //recupero la pagina appropriata
            typename HashedPageList::Position pos; 
            if (pageList.findPosition (*out_pageIndex, &pos))
                return pageList.getValueAtPos (pos);
            return NULL;                
        }
        
    private:			
        gos::Allocator			*allocator;
        HashedPageList			pageList;
        gos::FastArray<sElem>	denseList;
    };
    
} //namespace gos

#endif //_gosSparseSet_h_

