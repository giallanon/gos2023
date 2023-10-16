#ifndef _gosFastArray_h_
#define _gosFastArray_h_
#include "gosBufferLinear.h"


namespace gos
{
    /*===============================================================================
     * Funziona come gos::Array solo che qui non vengono chiamati i costruttori
     * e i distruttori delle classi. In pratica è da utilizzare quando si vuole un array
     * di tipi base (int, float..) o puntatori o cmq strutture POD che non necessitano
     * di costruttore/distruttore e che possono essere "spostate" con la semplice memcpy
     *==============================================================================*/
    template <typename T>
    class FastArray
    {
    public:
        typedef bool (*BubblesortFn) (const T &a, const T &b);
                    //ritorna true se devo swappare a con b

    public:
                        FastArray  ()																					{ nElem = nallocati = 0; }

                        //======================================= memory
        void			setupWithBase (void *startingBlock, u32 sizeOfStartingBlock, Allocator *backingallocator)       { memBlock.setupWithBase (startingBlock, sizeOfStartingBlock, backingallocator); nallocati=sizeOfStartingBlock/sizeof(T); }
        void			setup (Allocator *backingallocator, u32 preallocNumElem=0)                                      { memBlock.setup (backingallocator, preallocNumElem*sizeof(T)); nallocati=preallocNumElem; }
        void			unsetup ()																						{ memBlock.unsetup(); nElem = nallocati = 0; }
        void			prealloc (u32 n)																				{ _grow (n); }


                        //======================================= get / set
        T&				append (const T& t)																				{ T *ret = _insert(nElem);  (*ret)=t; return *ret; }
        T&				operator[] (u32 i)																				{ return *_insert(i); }
        T&				getElem (u32 i) const																			{ assert(i<nElem);	return * reinterpret_cast<T*>(memBlock._getPointer(i *sizeof(T))); }
        const T&		queryElem(u32 i) const																			{ assert(i < nElem);	return * reinterpret_cast<T*>(memBlock._getPointer(i * sizeof(T))); }
		const T&		operator() (u32 i)	const																		{ return queryElem(i); }
        T&				insertAt (u32 i, const T& t)
                        {
                            if (nElem == 0)
                            {
                                assert (i == 0);
                                return append(t);
                            }
                            assert (i < nElem);
                            shiftaAvanti (i, 1);
                            T *ret = _insert(i);
                            (*ret)=t; return *ret;
                        }

                        //======================================= copy
        void			copyFrom (const FastArray<T> &source)
                        {
                            if (source.nElem)
                            {
#ifdef _DEBUG
                                assert( memBlock.copyFrom (source.memBlock, 0, source.nElem * sizeof(T), 0, true) );
#else
                                memBlock.copyFrom (source.memBlock, 0, source.nElem * sizeof(T), 0, true);
#endif
                            }
                            nElem = source.nElem;
                        }


                        //======================================= fn
        void			reset()																							{ nElem=0; }
        void			trunc (u32 quantiElementiLascio)																{ assert (quantiElementiLascio<=nElem); nElem = quantiElementiLascio; }
        void			move (u32 iFrom, u32 iTo)
                        {
                            //prende [iFrom] e lo mette in posizione [iTo] shiftando avanti tutto quello che c'è da iForm in poi
                            assert (iFrom < nElem && iTo < nElem);
                            if (iFrom == iTo)
                                return;

                            u8 *p = memBlock._getPointer(0);
                            iFrom *= sizeof(T);
                            iTo *= sizeof(T);
                            u8	swap[sizeof(T)];
                            memcpy (&swap, &p[iFrom], sizeof(T));

                            if (iFrom < iTo)
                            {
                                const u32 howMuch = iTo - iFrom;
                                memcpy (&p[iFrom], &p[iFrom + sizeof(T)], howMuch );
                            }
                            else
                            {
                                const u32 howMuch = iFrom - iTo;
                                memmove (&p[iTo + sizeof(T)], &p[iTo], howMuch );
                            }
                            memcpy (&p[iTo], &swap, sizeof(T));
                        }

        void			remove (u32 i)																					{ removeFromTo(i,1); }
        void			removeFromTo (u32 iFrom, u32 iHowMany)
                        {
                            assert (nElem>0 && iHowMany>0 && iFrom+iHowMany<=nElem);
                            nElem -= iHowMany;
                            u32 nElementiDaMuovere = nElem-iFrom;
                            if (nElementiDaMuovere>0)
                            {
                                u8 *pt = memBlock._getPointer(0);
                                memcpy (&pt[iFrom * sizeof(T)], &pt[(iFrom+iHowMany) * sizeof(T)], nElementiDaMuovere * sizeof(T));
                            }
                        }

        void			swap (u32 i1, u32 i2)
                        {
                            assert (i1<nElem && i2<nElem);
                            if (i1==i2)
                                return;
                            u8 *p = memBlock._getPointer(0);
                            i1 *= sizeof(T);
                            i2 *= sizeof(T);
                            u8	swap[sizeof(T)];
                            memcpy (&swap, &p[i1], sizeof(T));
                            memcpy (&p[i1], &p[i2], sizeof(T));
                            memcpy (&p[i2], &swap, sizeof(T));
                        }

                        //prende l'elemento i-esimo e lo "rimuove" semplicemente copiandoci
                        //al suo posto l'ultimo elemento
        void			removeAndSwapWithLast (u32 i)
                        {
                            assert (nElem>0 && i<nElem);
                            --nElem;
                            if (i < nElem)
                            {
                                u8 *p = memBlock._getPointer(0);
                                memcpy (&p[i * sizeof(T)], &p[nElem * sizeof(T)], sizeof(T));
                            }
                        }

        u32				simpleSearch (const T &tofind) const
                        {
                            for (u32 i=0; i<nElem; i++)
                                if (queryElem(i) == tofind)
                                    return i;
                            return u32MAX;
                        }

        bool			findAndRemove (const T &toFind)
                        {
                            u32 index = simpleSearch(toFind);
                            if (index == u32MAX)
                                return false;
                            remove (index);
                            return true;
                        }

                        // muove tutti gli elementi dell'array a partire a "aPartireDa" (compreso) di "diQuanto" posizioni in avanti
        void			shiftaAvanti (u32 aPartireDa, u32 diQuanto)
                        {
                            assert (aPartireDa<nElem && diQuanto>0);
                            _grow (nElem+diQuanto);
                            u8 *pt = memBlock._getPointer(0);
                            u32 nElementiDaMuovere = nElem-aPartireDa;
                            memmove (&(pt[(aPartireDa+diQuanto) * sizeof(T)]), &(pt[aPartireDa * sizeof(T)]), sizeof(T)*nElementiDaMuovere);
                            nElem += diQuanto;
                        }

                        // muove tutti gli elementi dell'array a partire a "aPartireDa" (compreso) di "diQuanto" posizioni indietro
        void			shiftaIndietro (u32 aPartireDa, u32 diQuanto)
                        {
                            assert (aPartireDa<nElem && aPartireDa>0 && diQuanto>0 && diQuanto<=aPartireDa);
                            u8 *p = memBlock._getPointer(0);
                            u32 nElementiDaMuovere = nElem-aPartireDa;
                            memcpy (&(p[(aPartireDa-diQuanto) * sizeof(T)]), &(p[aPartireDa * sizeof(T)]), sizeof(T)*nElementiDaMuovere);
                            nElem -= diQuanto;
                        }

        void			bubbleSort (BubblesortFn cmpFn)
                        {
                            u32 n = getNElem();
                            if (n < 2)
                                return;

                            bool bEsci = false;
                            while (bEsci == false)
                            {
                                --n;
                                bEsci = true;
                                u32 ct = 0;
                                T *t1 = reinterpret_cast<T*>(memBlock._getPointer(ct));
                                for (u32 i=0; i<n; i++)
                                {
                                    ct+=sizeof(T);
                                    T *t2 = reinterpret_cast<T*>(memBlock._getPointer(ct));
                                    if (cmpFn(*t1, *t2))
                                    {
                                        bEsci = false;
                                        swap (i, i+1);
                                    }
                                    t1 = t2;
                                }
                            }
                        }

                        //===================== query
        u32				getNElem()	const										{ return nElem; }
        u32				getNAllocatedElem() const								{ assert (nallocati*sizeof(T) <= memBlock.getTotalSizeAllocated()); return nallocati; }
        Allocator*      getAllocator() const									{ return memBlock.getAllocator(); }
        const void*		_queryPointer () const									{ return memBlock._getPointer(0); }

    private:
        void			_grow (u32 nuovoNumMaxDiElementi)
                        {
                            if (nuovoNumMaxDiElementi <= nallocati)
                                return;
                            nallocati = nuovoNumMaxDiElementi;
#ifdef _DEBUG
                            assert(memBlock.growUpTo (nuovoNumMaxDiElementi * sizeof(T)));
#else
                            memBlock.growUpTo (nuovoNumMaxDiElementi * sizeof(T));
#endif
                        }

        T*				_insert (u32 atPos)
                        {
                            if (nallocati < 2)
                                _grow(32);
                            while (atPos >= nallocati)
                                _grow(nallocati + nallocati/2);
                            if (atPos >= nElem)
                                nElem = atPos+1;
#ifdef _DEBUG
                            debugArray = (T*)memBlock._getPointer(0);
#endif
                            return reinterpret_cast<T*>(memBlock._getPointer((atPos) * sizeof(T)));
                        }

    private:
        BufferLinear		memBlock;
        u32					nElem;
        u32					nallocati;

#ifdef _DEBUG
        const T*			debugArray;
#endif
    };
} //namespace gos

#endif //_gosFastArray_h_
