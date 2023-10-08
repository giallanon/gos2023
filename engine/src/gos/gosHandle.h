#ifndef _gosHandle_h_
#define _gosHandle_h_
#include "memory/gosMemory.h"

namespace gos
{
	/*******************************************************************
	 * HandleT
	 *
	 *	E' un 32 bit diviso in 4 sezioni (detti canali), "chunk", "user", "index", "counter".
	 *	Ogni sezione e' composta da n bit, A per "chunk", B per "user", C per "index", D per "counter".
	 *
	 *	Incrementando il singolo canale, eventualmente il valore ripartira' da 0
	 *	C deve essere <= 16
	 *	D deve essere <= 16
	 */
	template<int A,int B,int C, int D>
	struct HandleT
	{
	private:
		static const constexpr u32	MASKSHIFT_0 = D + C + B;
		static const constexpr u32	MASKSHIFT_1 = D + C;
		static const constexpr u32	MASKSHIFT_2 = D;
		static const constexpr u32	MASKSHIFT_3 = 0;
		
		static const constexpr u32	MASK_0 = static_cast<u32> (((u64)((0x0000000000000001 << A) - 1) << (u64)MASKSHIFT_0) & 0x00000000FFFFFFFF);
		static const constexpr u32	MASK_1 = static_cast<u32> (((u64)((0x0000000000000001 << B) - 1) << (u64)MASKSHIFT_1) & 0x00000000FFFFFFFF);
		static const constexpr u32	MASK_2 = static_cast<u32> (((u64)((0x0000000000000001 << C) - 1) << (u64)MASKSHIFT_2) & 0x00000000FFFFFFFF);
		static const constexpr u32	MASK_3 = static_cast<u32> (((u64)((0x0000000000000001 << D) - 1) << (u64)MASKSHIFT_3) & 0x00000000FFFFFFFF);

	public:
		typedef HandleT<A, B, C, D> ThisHandle;

	public:
		static ThisHandle			INVALID()								{ ThisHandle h; h.setInvalid(); return h; }
		static constexpr u8			getNumBitsChunk()						{ return A; }
		static constexpr u8			getNumBitsUser()						{ return B; }
		static constexpr u8			getNumBitsIndex()						{ return C; }
		static constexpr u8			getNumBitsCounter()						{ return D; }
		
		static constexpr u32		getNumMaxChunk()						{ return (u32)(0x0001 << A); }
		static constexpr u32		getNumMaxHandleAA()						{ return (u32)(0x0001 << C); }
		static constexpr u32		getNumMaxCounter()						{ return (u32)(0x0001 << D); }
		static constexpr u32		getNumMaxHandlePerChunk()				{ return (getNumMaxHandleAA() >> A); }

	public:
					HandleT()								{ assert (A + B + C + D == 32); assert(MASK_0 && MASK_2 && MASK_3); setInvalid(); }

		bool		operator== (const ThisHandle &b) const  { return (id == b.id); }
		bool		operator!= (const ThisHandle &b) const  { return (id != b.id); }
					
		void		setInvalid()							{ id = u32MAX; }
		bool		isInvalid() const						{ return (id == u32MAX); }
		bool		isValid() const							{ return (id != u32MAX); }

		u32			getChunkValue() const					{ return ((id & MASK_0) >> MASKSHIFT_0); }
		u32			getUserValue() const					{ return ((id & MASK_1) >> MASKSHIFT_1); }
		u32			getIndexValue() const					{ return ((id & MASK_2) >> MASKSHIFT_2); }
		u32			getCounterValue() const					{ return ((id & MASK_3) >> MASKSHIFT_3); }

		void		setChunkValue(u32 value)				{ id &= ~(MASK_0);  id |= ((value << MASKSHIFT_0) & MASK_0); }
		void		setUserValue(u32 value)					{ assert(MASK_1); id &= ~(MASK_1);  id |= ((value << MASKSHIFT_1) & MASK_1); }
		void		setIndexValue(u32 value)				{ id &= ~(MASK_2);  id |= ((value << MASKSHIFT_2) & MASK_2); }
		void		setCounterValue(u32 value)				{ id &= ~(MASK_3);  id |= ((value << MASKSHIFT_3) & MASK_3); }

		u32			incChunkValue()							{ u32 v = getChunkValue();  v++; setChunkValue(v); return getChunkValue(); }
		u32			incUserValue()							{ assert(MASK_1); u32 v = getUserValue();  v++; setUserValue(v); return getUserValue(); }
		u32			incIndexValue()							{ u32 v = getIndexValue();  v++; setIndexValue(v); return getIndexValue(); }
		u32			incCounterValue()						{ u32 v = getCounterValue();  v++; setCounterValue(v); return getCounterValue(); }

		void		setFromU32 (u32 u)						{ id = u; }
		u32			viewAsU32() const						{ return id; }

		u32			debug_getValueByIndex(u8 which) const 
		{ 
			switch (which)
			{
			case 0: return getChunkValue();
			case 1: return getUserValue();
			case 2: return getIndexValue();
			case 3: return getCounterValue();
			default: DBGBREAK; return 0;
			}
		}
		u32			debug_incValueByIndex(u8 which)
		{
			switch (which)
			{
			case 0: return incChunkValue();
			case 1: return incUserValue();
			case 2: return incIndexValue();
			case 3: return incCounterValue();
			default: DBGBREAK; return 0;
			}
		}
		void		debug_setValueByIndex(u8 which, u32 value)
		{
			switch (which)
			{
			case 0: setChunkValue(value); return;
			case 1: setUserValue(value); return;
			case 2: setIndexValue(value); return;
			case 3: setCounterValue(value); return;
			default: DBGBREAK;
			}
		}

	private:
		u32			id;
	};




	/*******************************************************************
	 * HandleList
	 *
	 *	Mantiene un array di DATASTRUCT, ciascuno associato ad un HANDLE.
	 *	Dato una istanza di HANDLE, e' possibile risalire al DATASTRUCT ad esso associato tramite la fromHandleToPointer()
	 *
	 *	La composizione dell'HANDLE e' importante:
	 *		C	determina il numero massimo di HANDLE riservabili (2^C)
	 *		A	determina il numero di "chunk" usati per memorizzare tutti e C i DATASTRUCT (2^A).
				Se C=8 e A=3, allora abbiamo un massimo di 256 handle allocabili, da divire in 8 chunk
				Essendo 256 gli handle allocabili, e' necessario riservare spazio per 256 strutture di tipo DATASTRUCT.
				Invece di allocarle tutte e 256 in un colpo solo, ne alloco 256/(2^A), quindi 256/8 quindi 32.
				Quando i primi 32 sono tutti occupati, creo un secondo chunk da 32 e via dicendo fino ad un totale di 8 chunk da 32
	 */
	template <typename HANDLE, typename DATASTRUCT>
	class HandleList
	{
	public:
					HandleList()
					{
						allocator = NULL;
						chunkList = NULL;
					}
		
		bool		isAlreadySetup() const																		{ return (allocator != NULL);  }

		void		setup (Allocator *allocatorIN)
					{
						assert (HANDLE::getNumBitsIndex() <= 16);
						assert (HANDLE::getNumBitsCounter() <= 16);
			
						allocator = allocatorIN;
			
						const u32 nChunk = HANDLE::getNumMaxChunk();
						chunkList = (sRecord**)GOSALIGNEDALLOC(allocator, nChunk * sizeof(sRecord*), alignof(sRecord*));
						firstFree = (u32*)GOSALLOC (allocator,sizeof(u32) * nChunk);
						for (u32 i = 0; i < nChunk; i++)
						{
							chunkList[i] = NULL;
							firstFree[i] = 0;
						}

						priv_allocChunk(0);
					}

		void		unsetup()
		{
			if (NULL == chunkList)
				return;
			const u32 nChunk = HANDLE::getNumMaxChunk();

#ifdef _DEBUG
			//verifico che tutti gli handle siano stati rilasciati
			for (u32 i = 0; i < nChunk; i++)
			{
				if (NULL == chunkList[i])
					continue;
				u32 ct = 0;
				u32 free = firstFree[i];
				while (free != u16MAX)
				{
					free = chunkList[i][free].nextFree;
					ct++;
				}
				assert (ct == HANDLE::getNumMaxHandlePerChunk());
			}
#endif

			for (u32 i = 0; i < nChunk; i++)
				GOSFREE(allocator, chunkList[i]);
			GOSFREE(allocator, chunkList);
			GOSFREE(allocator, firstFree);
			chunkList = NULL;
		}

		DATASTRUCT*	reserve (HANDLE *out_handle)
		{
			const u32 nChunk = HANDLE::getNumMaxChunk();
			for (u32 i = 0; i < nChunk; i++)
			{
				if (firstFree[i] != 0xFFFF)
				{
					if (NULL == chunkList[i])
						priv_allocChunk((u8)i);

					const u32 indexReturned = firstFree[i];
					sRecord *ret = &chunkList[i][indexReturned];
					firstFree[i] = ret->nextFree;

					out_handle->setChunkValue(i);
					out_handle->setUserValue(0);
					out_handle->setIndexValue(indexReturned);
					out_handle->setCounterValue(ret->curCounter);
					return &ret->userData;
				}
			}

			out_handle->setInvalid();
			return NULL;
		}

		void		release (HANDLE &handle)
		{
			sRecord *r = priv_getRecordFromHandle(handle);
			if (NULL == r)
			{
				DBGBREAK;
				handle.setInvalid();
				return;
			}

			const u32 chunk = handle.getChunkValue();
			const u32 index = handle.getIndexValue();
			r->curCounter = (u16)handle.incCounterValue();
			r->nextFree = (u16)firstFree[chunk];
			handle.setInvalid();

			firstFree[chunk] = index;
		}

		bool		fromHandleToPointer (const HANDLE &h, DATASTRUCT* *out) const
					{
						sRecord *ret = priv_getRecordFromHandle(h);
						if (ret)
						{
							*out = &ret->userData;
							return true;
						}

						*out = NULL;
						return false;
					}


	private:
		struct sRecord
		{
			u16			curCounter;
			u16			nextFree;
			DATASTRUCT	userData;
		};

	private:
		void		priv_allocChunk(u8 which)
					{
						#ifdef _DEBUG
							const u32 nChunk = HANDLE::getNumMaxChunk();
							assert(which < nChunk);
						#endif
						const u32 nMaxHandlePerChunk = HANDLE::getNumMaxHandlePerChunk();
						assert(NULL==chunkList[which]);

						firstFree[which] = 0;
						chunkList[which] = (sRecord*)GOSALIGNEDALLOC(allocator, nMaxHandlePerChunk * sizeof(sRecord), alignof(sRecord));
						for (u32 i = 0; i < nMaxHandlePerChunk; i++)
						{
							chunkList[which][i].curCounter = 1;
							chunkList[which][i].nextFree = (u16)(i+1);
						}
						chunkList[which][nMaxHandlePerChunk-1].nextFree = 0xFFFF;
						
					}

		sRecord*	priv_getRecordFromHandle (const HANDLE &h) const
					{
						if (h.isInvalid())
							return NULL;
						const u32	chunk = h.getChunkValue();
						const u32	index = h.getIndexValue();
						const u32	counter = h.getCounterValue();

			#ifdef _DEBUG
						assert(chunk < HANDLE::getNumMaxChunk());
						assert(index < HANDLE::getNumMaxHandlePerChunk());
						assert(NULL != chunkList[chunk]);
			#endif
						if (counter != chunkList[chunk][index].curCounter)
							return NULL;
						return &chunkList[chunk][index];
					}

	private:
		Allocator	*allocator;
		sRecord		**chunkList;
		u32			*firstFree;
	};
} // namespace gos
#endif //_gosHandle_h_