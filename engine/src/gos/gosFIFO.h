#ifndef _gosFIFO_h_
#define _gosFIFO_h_
#include "memory/gosMemory.h"
#include "memory/gosAllocatorPolicy_Thread.h"


namespace gos
{
    /***********************************************************************
     * template di un FIFO generica, con opzioni per la politica di thread safe
     * Il tipo T deve essere o un tipo semplice (int, float..) oppure una POS, ovvero deve essere possibile memcpiarlo.
     *
     * Vedi le classi FIFO e FIFOts a fine file per la versione non threadsafe e per la versione threadsafe
     * di questa classe
     */
    template<typename T, typename TThreadSafePolicy>
    class TemplateFIFO
    {
    public:
                    TemplateFIFO ()														{ allocator = NULL; first = last = NULL; }

        virtual     ~TemplateFIFO()                                                     { unsetup(); }

		void		setup (Allocator *allocatorIN)
					{
						//se la policy è di thread-safe, è necessario che anche l'allocatore lo sia
						assert((tsPolicy.isThreadSafe() && allocatorIN->isThreadSafe()) || !tsPolicy.isThreadSafe());
						allocator = allocatorIN;
						first = last = NULL;
					}
		
		void		unsetup()															{ empty(); allocator = NULL; }

        void        empty()
                    {
                        tsPolicy.lock();
                            while (first)
                            {
                                sRecord *p = first;
                                first = first->next;
                                GOSFREE(allocator, p);
                            }
                            last = NULL;
                        tsPolicy.unlock();
                    }

        void        push (const T &data)
                    {
                        sRecord *p = GOSALLOCSTRUCT(allocator, sRecord);
                        memcpy (&p->data, &data, sizeof(T));
                        p->next = NULL;

                        tsPolicy.lock();
                            if (last)
                            {
                                last->next = p;
                                last = p;
                            }
                            else
                            {
                                first = last = p;
                            }
                        tsPolicy.unlock();
                    }

        bool        pop (T *out_data)
                    {
                        tsPolicy.lock();
                            if (NULL == first)
                            {
                                tsPolicy.unlock();
                                return false;
                            }

                            memcpy (out_data, &first->data, sizeof(T));
                            sRecord *p = first;
                            first = first->next;
                            GOSFREE(allocator, p);

                            if (NULL == first)
                                last = NULL;
                        tsPolicy.unlock();
                        return true;
                    }

		bool		isEmpty() const
					{
						tsPolicy.lock();
						bool ret = (first == NULL);
						tsPolicy.unlock();
						return ret;
					}

    private:
        struct sRecord
        {
            sRecord *next;
            T       data;
        };

    private:
        Allocator      *allocator;
        sRecord             *first, *last;
        TThreadSafePolicy   tsPolicy;
    };




    /***********************************************************************
     * FIFO non thread safe
     *
     */
    template<typename T>
    class FIFO : public TemplateFIFO<T,AllocPolicy_Thread_Unsafe>
    {
    public:
                    FIFO () : TemplateFIFO<T,AllocPolicy_Thread_Unsafe>()		{ }
        virtual     ~FIFO ()                                                    { }
    };




    /***********************************************************************
     * FIFO thread safe
     *
     */
    template<typename T>
    class FIFOts : public TemplateFIFO<T, AllocPolicy_Thread_Safe>
    {
    public:
                    FIFOts () : TemplateFIFO<T,AllocPolicy_Thread_Safe>()		{ }
        virtual     ~FIFOts ()                                                  { }
    };

} //namespace rhea
#endif // _rheaFIFO_h_
