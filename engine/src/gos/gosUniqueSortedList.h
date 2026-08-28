#ifndef _gosUniqueSortedList_h_
#define _gosUniqueSortedList_h_
#include "gosFastArray.h"

namespace gos
{
    template<class TKEY>
    inline int UniqueSortedList_compareFn (const TKEY &t1, const TKEY &t2)   { return t1.compare(t2); }

    template<>  inline int UniqueSortedList_compareFn (const u64 &t1, const u64 &t2)    { if (t1==t2) return 0; if (t1>t2) return 1; return -1; }
    template<>  inline int UniqueSortedList_compareFn (const i64 &t1, const i64 &t2)    { if (t1==t2) return 0; if (t1>t2) return 1; return -1; }

    template<>  inline int UniqueSortedList_compareFn (const u32 &t1, const u32 &t2)    { if (t1==t2) return 0; if (t1>t2) return 1; return -1; }
    template<>  inline int UniqueSortedList_compareFn (const i32 &t1, const i32 &t2)    { if (t1==t2) return 0; if (t1>t2) return 1; return -1; }
    
    template<>  inline int UniqueSortedList_compareFn (const u16 &t1, const u16 &t2)    { if (t1==t2) return 0; if (t1>t2) return 1; return -1; }
    template<>  inline int UniqueSortedList_compareFn (const i16 &t1, const i16 &t2)    { if (t1==t2) return 0; if (t1>t2) return 1; return -1; }

    template<>  inline int UniqueSortedList_compareFn (const u8 &t1, const u8 &t2)    { if (t1==t2) return 0; if (t1>t2) return 1; return -1; }
    template<>  inline int UniqueSortedList_compareFn (const i8 &t1, const i8 &t2)    { if (t1==t2) return 0; if (t1>t2) return 1; return -1; }
    
    /****************************************************
     * @brief   UniqueSortedList
     *          Mantiene una lista di oggetti di tipo T univoci e ordinati
     *
     *          T deve essere una classe con un metodo "int compare (const T &b) const" che ritorna 0 se a==b, 1 se a>b, -1 se a<<b
     *          Per i tipi piu' comuni (come u64 e u32), ho definito una specializzazione del template HashMap_compareFn<>
     *          in modo da non dover implementare una classe "u32" con dentro un meotodo compare
     */
	template<class T>
	class UniqueSortedList
	{
	public:
                UniqueSortedList ()                                                     { }
                UniqueSortedList (Allocator *backingallocator, u32 preallocNumElem=0)   { setup (backingallocator, preallocNumElem); }
                ~UniqueSortedList ()                                                    { list.unsetup (); }

                //======================================= memory
        void	setup (Allocator *backingallocator, u32 preallocNumElem=0)              { list.setup (backingallocator, preallocNumElem); }
        void	unsetup ()																{ list.unsetup (); }
		bool	is_setup() const 														{ return list.getAllocator() != NULL; }
        void	prealloc (u32 n)														{ list.prealloc (n); }

        void    reset()                                                                 { list.reset(); }

        bool    insertIfNotExists (const T &data)
                {
                    if (list.getNElem() == 0)
                    {
                        list[0] = data;
                        return true;
                    }

                    sSearchRange s;
                    s.start = 0;
                    s.end_incluso = list.getNElem() - 1;

                    u32 index;
                    if (priv_binarySearch (s, data, &index))
                        return false;

                    if (index < list.getNElem())
                        list.shiftaAvanti (index, 1);
                    
                    list[index] = data;
                    return true;
                }   

        bool    remove (const T &data)
                {
                    if (list.getNElem() == 0)
                        return false;

                    sSearchRange s;
                    s.start = 0;
                    s.end_incluso = list.getNElem() - 1;

                    u32 index;
                    if (!priv_binarySearch (s, data, &index))
                        return false;

                    list.remove (index);
                    return true;
                }
        
        bool    exists (const T &data) const
                {
                    if (list.getNElem() == 0)
                        return false;

                    sSearchRange s;
                    s.start = 0;
                    s.end_incluso = list.getNElem() - 1;

                    u32 index;
                    return priv_binarySearch (s, data, &index);
                }

        /**
         * @brief   ritorna l'array lineare nel quale sono memorizzati i dati
         *
         */
        const gos::FastArray<T>*    _queryList() const                                  { return &list; }
        u32                         getNElem() const                                    { return list.getNElem(); }

                        template<typename LAMBDA>
        void            forEach (LAMBDA&& evalParamFn)
                        {
                            for (u32 index=0; index<list.getNElem(); index++)
                            {
                                if (false == evalParamFn (index, list.getElem(index)))
                                    break;
                            }
                        }

                        template<typename LAMBDA>                        
        void            forEach (LAMBDA&& evalParamFn) const
                        {
                            for (u32 index=0; index<list.getNElem(); index++)
                            {
                                if (false == evalParamFn(index, list.queryElem(index) ) )
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
        bool    priv_binarySearch (sSearchRange &search, const T &data, u32 *out_index) const
                {
                    //se trova <data>, ritorna true e mette in [out_index] l'indice all'interno di [list] dove ha trovato <data>.
                    //se non trova <data>, ritorna false e mette in [out_index] l'indice da utilizzarsi se si volesse inserire <data> all'interno di list
                    while (1)
                    {
                        const u32 numElem = 1 + (search.end_incluso - search.start);

                        if (numElem <= 8)
                        {
                            *out_index = search.start;
                            for (u32 i=search.start; i<(search.start+numElem); i++)
                            {
                                switch (UniqueSortedList_compareFn<T>(data, list(i)))
                                {
                                default:
                                    DBGBREAK;
                                    return false;                                

                                case  0: //sono uguali
                                    *out_index = i;
                                    return true;

                                case 1: //data e' maggiore di list(i)
                                    *out_index = i+1;
                                    break;

                                case -1: //data e' minore di list(i)
                                    *out_index = i;
                                    return false;
                                }
                            }

                            return false;
                        }


                        const u32 middle = search.start + numElem / 2;
                        switch (UniqueSortedList_compareFn<T>(data, list(middle)))
                        {
                        default:
                            DBGBREAK;
                            return false;

                        case  0: //sono uguali
                            *out_index = middle;
                            return true;

                        case 1: //data e' maggiore di list(i)
                            search.start = middle+1;
                            break;

                        case -1: //data e' minore di list(i)
                            search.end_incluso = middle-1;
                            break;
                        }                        
                    }   
                }    

	private:
		FastArray<T>	list;
	};
    
} //namespace gos

#endif //_gosUniqueSortedList_h_

