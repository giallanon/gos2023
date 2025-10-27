#ifndef _gosSortedList_h_
#define _gosSortedList_h_
#include "gosFastArray.h"

namespace gos
{
	template<class T>
	class SortedList
	{
	public:
                SortedList ()                                                           { }
                SortedList (Allocator *backingallocator, u32 preallocNumElem=0)         { setup (backingallocator, preallocNumElem); }
                ~SortedList ()                                                          { list.unsetup (); }

                //======================================= memory
        void	setup (Allocator *backingallocator, u32 preallocNumElem=0)              { list.setup (backingallocator, preallocNumElem); }
        void	unsetup ()																{ list.unsetup (); }
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
                                if (data == list(i))
                                {
                                    *out_index = i;
                                    return true;
                                }
                                else if (data > list(i))
                                {
                                    //data e' maggiore di list(i)
                                    *out_index = i+1;
                                }
                            }

                            return false;
                        }


                        const u32 middle = search.start + numElem / 2;
                        if (data == list(middle))
                        {
                            *out_index = middle;
                            return true;
                        }
                        if (data > list(middle))
                        {
                            //elem e' maggiore di list(i)
                            search.start = middle + 1;
                        }
                        else
                        {
                            //elem e' minore di list(i)
                            search.end_incluso = middle-1;
                        }
                    }   
                }    

	private:
		FastArray<T>	list;
	};
    
} //namespace gos

#endif //_gosSortedList_h_

