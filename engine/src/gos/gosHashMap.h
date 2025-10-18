#ifndef _gosHashMap_h_
#define _gosHashMap_h_
#include "gosFastArray.h"

namespace gos
{
    template<class TKEY>
    inline int HashMap_compareFn (const TKEY &t1, const TKEY &t2)   { return t1.compare(t2); }

    template<>
    inline int HashMap_compareFn (const u64 &t1, const u64 &t2)    { if (t1==t2) return 0; if (t1>t2) return 1; return -1; }

    template<>
    inline int HashMap_compareFn (const i32 &t1, const i32 &t2)    { if (t1==t2) return 0; if (t1>t2) return 1; return -1; }

    template<>
    inline int HashMap_compareFn (const u32 &t1, const u32 &t2)    { if (t1==t2) return 0; if (t1>t2) return 1; return -1; }

    /**
     * @brief HashMap
     * E' una hash map dove TKEY e' l'hash e TVALUE e' il value
     * 
     * TKEY deve essere una classe con un metodo "int compare (const TKEY &b) const" che ritorna 0 se a==b, 1 se a>b, -1 se a<<b
     * Per i tipi piu' comuni (come u64 e u32), ho definito una specializzazione del template HashMap_compareFn<>
     * in modo da non dover implementare una classe "u32" con dentro un meotodo compare
    */
    template<class TKEY, class TVALUE>
    class HashMap
    {
    public:
        struct Position
        {
        private:
            TKEY    _key;
            u32     _index;

        friend HashMap<TKEY, TVALUE>;
        };

        struct sElem
        {
            TKEY    key;
            TVALUE  value;
        };

    public:
                HashMap ()                                                              { }
                HashMap (Allocator *backingallocator, u32 preallocNumElem=0)            { setup (backingallocator, preallocNumElem); }
                ~HashMap ()                                                             { list.unsetup (); }

                //======================================= memory
        void	setup (Allocator *backingallocator, u32 preallocNumElem=0)              { list.setup (backingallocator, preallocNumElem); }
        void	unsetup ()																{ list.unsetup (); }
        void	prealloc (u32 n)														{ list.prealloc (n); }

        void    reset()                                                                 { list.reset(); }
        void    copyFrom (const HashMap<TKEY, TVALUE> &src)                             { list.copyFrom (src.list); }

        /**
         * @brief   inserisce la coppia (key, value) solo se (key) non e' gia' presente
         * 
         * @return  true se ha inserito, false se key esisteva gia' e quindi non ha inserito
         */
        bool    insertIfNotExists (const TKEY &key, const TVALUE &value)
                {
                    if (list.getNElem() == 0)
                    {
                        list[0].key = key;
                        list[0].value = value;
                        return true;
                    }

                    sSearchRange s;
                    s.start = 0;
                    s.end_incluso = list.getNElem() - 1;

                    u32 index;
                    if (priv_binarySearch (s, key, &index))
                        return false;

                    if (index < list.getNElem())
                        list.shiftaAvanti (index, 1);
                    
                    list[index].key = key;
                    list[index].value = value;
                    return true;
                }    

        /**
         * @brief   inserisce (value) nella giusta posizione mantenendo l'array ordinato. [pos] e' ottenibile chiamando findWithPos()
         */
        void    insertInPosition (const Position &pos, const TVALUE &value)
                {
                    assert (pos._index != u32MAX);
                    if (pos._index < list.getNElem())
                        list.shiftaAvanti (pos._index , 1);
                    
                    list[pos._index ].key = pos._key;
                    list[pos._index ].value = value;            
                }

        /**
         * @brief   cerca key e la rimuove
         * 
         * @return  true se (key) esiste ed e' stata rimossa, false altrimenti
         */
        bool    remove (const TKEY &key)
                {
                    if (list.getNElem() == 0)
                        return false;

                    sSearchRange s;
                    s.start = 0;
                    s.end_incluso = list.getNElem() - 1;

                    u32 index;
                    if (!priv_binarySearch (s, key, &index))
                        return false;

                    list.remove (index);
                    return true;
                }


        /**
         * @brief   cerca key
         * 
         * @return  true se (key) esiste, false altrimenti.
         */
        bool    exists (const TKEY &key) const
                {
                    if (list.getNElem() == 0)
                        return false;

                    sSearchRange s;
                    s.start = 0;
                    s.end_incluso = list.getNElem() - 1;

                    u32 index;
                    return priv_binarySearch (s, key, &index);
                }

        /**
         * @brief   cerca key e, se la trova, valorizza out_value
         * 
         * @return  true se (key) esiste, false altrimenti.
         *          Se (key) esiste, ritorna in *out_value il valore associato a key
         */
        bool    find (const TKEY &key, TVALUE *out_value) const
                {
                    if (list.getNElem() == 0)
                        return false;

                    sSearchRange s;
                    s.start = 0;
                    s.end_incluso = list.getNElem() - 1;

                    u32 index;
                    if (!priv_binarySearch (s, key, &index))
                        return false;

                    *out_value = list(index).value;
                    return true;
                }


        /**
         * @brief   cerca key e valorizza <out_position>
         *          Se ritorna true, allora <out_position> punta all'elemento nell'array lineare.
         *          Se ritorna false, allora e' possibile usare <out_position> come parametro per insertInPosition()
         *          garantendo che la posizione di inserimento sia coerente
         * 
         * @return  true se (key) esiste, false altrimenti.
         */
        bool    findPosition (const TKEY &key, Position *out_position) const
                {
                    if (list.getNElem() == 0)
                    {
                        out_position->_key = key;
                        out_position->_index = 0;
                        return false;
                    }

                    sSearchRange s;
                    s.start = 0;
                    s.end_incluso = list.getNElem() - 1;

                    if (priv_binarySearch (s, key, &out_position->_index))
                        return true;

                    out_position->_key = key;
                    return false;
                }

        /**
         * @brief   cerca key e, se la trova, valorizza out_value
         *          Se ritorna false, allora e' possibile usare out_position come parametro per insertInPosition()
         *          garantendo che la posizione di inserimento sia coerente
         * 
         * @return  true se (key) esiste, false altrimenti.
         *          Se (key) esiste, ritorna in *out_value il valore associato a key
         */
        bool    findWithPos (const TKEY &key, TVALUE *out_value, Position *out_position) const
                {
                    if (findPosition(key, out_position))
                    {
                        *out_value = list(out_position->_index).value;
                        return true;
                    }
                    return false;
                }




        TVALUE* getValueAtPos (const Position &pos) const                               { return &list.getElem(pos._index).value; }


        /**
         * @brief   ritorna l'array lineare nel quale sono memorizzati i dati
         *
         */
        const gos::FastArray<sElem>*    _queryList() const      { return &list; }

        u32 getNElem() const                                    { return list.getNElem(); }

        /**
         * @brief   scansiona l'array lineare nel quale sono memorizzati i dati
         *          e chiama la LAMBDA su ogni elemento. Se la LAMBDA ritorna false, la
         *          scansione termina
         *
         */        
                template<typename LAMBDA>                        
        void    forEach (LAMBDA&& evalParamFn) const
                {
                    const u32 n = list.getNElem();
                    for (u32 index=0; index<n; index++)
                    {
                        if (false == evalParamFn(list(index).key, list(index).value) )
                            break;
                    }
                }           

    private:
        struct sSearchRange
        {
            u32 start;
            u32 end_incluso;
        };

    private:
        bool    priv_binarySearch (sSearchRange &search, const TKEY &key, u32 *out_index) const
                {
                    //se trova KEY, ritorna true e mette in [out_index] l'indice all'interno di [list] dove ha trovato key
                    //se non trova KEY, ritorna false e mette in [out_index] l'indice da utilizzarsi se si volesse inserire KEY all'interno di list
                    while (1)
                    {
                        const u32 numElem = 1 + (search.end_incluso - search.start);

                        if (numElem <= 4)
                        {
                            *out_index = search.start;
                            for (u32 i=search.start; i<(search.start+numElem); i++)
                            {
                                //switch (key.compare(list(i).key))
                                switch (HashMap_compareFn<TKEY>(key, list(i).key))
                                {
                                default:
                                    break;

                                case 0: //sono uguali
                                    *out_index = i;
                                    return true;
                                
                                case 1: //elem e' maggiore di list(i)
                                    *out_index = i+1;
                                    break;
                                }
                            }

                            return false;
                        }


                        const u32 middle = search.start + numElem / 2;
                        //switch (key.compare(list(middle).key))
                        switch (HashMap_compareFn<TKEY>(key, list(middle).key))
                        {
                        case 0: //sono uguali
                            *out_index = middle;
                            return true;

                        case 1: //elem e' maggiore di list(i)
                            search.start = middle+1;
                            break;

                        case -1: //elem e' minore di list(i)
                            search.end_incluso = middle-1;
                            break;
                        }
                    }   
                }    

    private:
        gos::FastArray<sElem>     list;
    };
} //namespace gos

#endif // _gosHashMap_h_