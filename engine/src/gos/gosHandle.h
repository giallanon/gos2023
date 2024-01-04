#ifndef _gosHandle_h_
#define _gosHandle_h_
#include "memory/gosMemory.h"

namespace gos
{
	/*******************************************************************
	 * 	HandleT
	 * 
	 *	E' un 32 bit diviso in 5 sezioni (detti canali):
	 *		A index 					=> identifica il numero massimo di handle allicabili
	 *		B chunk						=> vedi HandleList
	 *		C counter					=> quando un handle viene riciclato, il suo counter viene incrementato di 1 per generare un handle differente dal precedente
	 *		D signature1 + signature2 	=> servono da PAD per raggiungere un totale di 32bit
	 *										L'utilita' di avere SIG_1 e SIG_2 in luogo di un solo valore D e' quella di poter dichiarare handle con lo stesso valore
	 *										di A,B e C ma, giocando con SIG_1 e SIG_2, riuscire a considerarli come tipi differenti.
	 *										Per esempio:
	 *											typedef HandleT<16,6,8, 0,2>  GOSHandle1;
	 *											typedef HandleT<16,6,8, 1,1>  GOSHandle2;
	 *										sono effettivamente 2 tipi diversi. In passato, in luogo di SIG_1 e 2 avevo D ma capitava di voler dichiarare 2 handle con
	 *										gli stessi A,B,C e che quindi non potevo distinguere (le fn accettavano sia Handle1 che Handle2 senza prob).
	 *										Per esempio:
	 *											typedef HandleT<16,6,8, 2>  GOSHandleViewport;
	 *											typedef HandleT<16,6,8, 2>  GOSHandleTexture;
	 *										Pur esendo chiamati diversamente, ai fini pratici sono lo stesso identico tipo e quindi una fn che accetta GOSHandleViewport
	 *										accetta anche GOSHandleTexture senza warning o errore.
	 *										Giocando con SIG_1 e SIG_2 invece:
	 *											typedef HandleT<16,6,8, 0,2>  GOSHandleViewport;
	 *											typedef HandleT<16,6,8, 1,1>  GOSHandleTexture;
	 *										diventano davvero 2 tipi diversi
	 *
	 *
	 *	Ogni sezione e' composta da n bit, A per "index", B per "chunk", C per "counter", D per "signature1 + signature2".
	 *
	 *	Incrementando il singolo canale, eventualmente il valore ripartira' da 0
	 *
	 *	A => con (A>0 && A <= 16) 	=> 2^A e' il num max di handle allocabili 
	 *	B => con (B>0 && B <= 16)	=> 2^B e' li num di chunck, ovvero quandi handle vengono allocati per ogni chunk (vedi HandleList<>)
	 *	C => con (C>0 && C <= 16)	=> 2^C e' la dimensione del [counter]. 
	 *	D => con (D>=0)				=> 2^D per pad fino a raggiungere A+B+C+D = 32 
	 *
	 *  A + B + C + D == 32
	*/
	template<int A, int B, int C, int SIGNATURE_1, int SIGNATURE_2>
	struct HandleT
	{
	private:
		static const constexpr u32	D = SIGNATURE_1 + SIGNATURE_2;
		static const constexpr u32	MASKSHIFT_0 = D + C + B;
		static const constexpr u32	MASKSHIFT_1 = D + C;
		static const constexpr u32	MASKSHIFT_2 = D;
		static const constexpr u32	MASKSHIFT_3 = 0;
		
		static const constexpr u32	MASK_0 = static_cast<u32> (((u64)((0x0000000000000001 << A) - 1) << (u64)MASKSHIFT_0) & 0x00000000FFFFFFFF);
		static const constexpr u32	MASK_1 = static_cast<u32> (((u64)((0x0000000000000001 << B) - 1) << (u64)MASKSHIFT_1) & 0x00000000FFFFFFFF);
		static const constexpr u32	MASK_2 = static_cast<u32> (((u64)((0x0000000000000001 << C) - 1) << (u64)MASKSHIFT_2) & 0x00000000FFFFFFFF);
		static const constexpr u32	MASK_3 = static_cast<u32> (((u64)((0x0000000000000001 << D) - 1) << (u64)MASKSHIFT_3) & 0x00000000FFFFFFFF);

	public:
		typedef HandleT<A, B, C, SIGNATURE_1, SIGNATURE_2> ThisHandle;

	public:
		static ThisHandle			INVALID()								{ static ThisHandle hINVALID; hINVALID.setInvalid(); return hINVALID; }
		
		static constexpr u8			getNumBitsIndex()						{ return A; }
		static constexpr u8			getNumBitsChunk()						{ return B; }
		static constexpr u8			getNumBitsCounter()						{ return C; }
		static constexpr u8			getNumBitsUser()						{ return D; }
		
		static constexpr u32		getNumMaxHandle()						{ return (u32)(0x0001 << A); }
		static constexpr u32		getNumMaxChunk()						{ return (u32)(0x0001 << B); }
		static constexpr u32		getNumMaxCounter()						{ return (u32)(0x0001 << C); }
		static constexpr u32		getNumMaxHandlePerChunk()				{ return (getNumMaxHandle() >> B); }

	public:
					HandleT()								{ assert (A + B + C + D == 32); assert(A>0 && A<=16); assert(B>0 && B<=16); assert(C>0 && C<=16); setInvalid(); }

		bool		operator== (const ThisHandle &b) const  { return (id == b.id); }
		bool		operator!= (const ThisHandle &b) const  { return (id != b.id); }
					
		void		setInvalid()							{ id = u32MAX; }
		bool		isInvalid() const						{ return (id == u32MAX); }
		bool		isValid() const							{ return (id != u32MAX); }

		u32			getIndexValue() const					{ return ((id & MASK_0) >> MASKSHIFT_0); }
		u32			getChunkValue() const					{ return ((id & MASK_1) >> MASKSHIFT_1); }
		u32			getCounterValue() const					{ return ((id & MASK_2) >> MASKSHIFT_2); }
		u32			getPADValue() const						
					{ 
						if constexpr (D==0)
							return 0; 
						else 
							return ((id & MASK_3) >> MASKSHIFT_3); 
					}

		void		setIndexValue(u32 value)				{ id &= ~(MASK_0);  id |= ((value << MASKSHIFT_0) & MASK_0); }
		void		setChunkValue(u32 value)				{ id &= ~(MASK_1);  id |= ((value << MASKSHIFT_1) & MASK_1); }
		void		setCounterValue(u32 value)				{ id &= ~(MASK_2);  id |= ((value << MASKSHIFT_2) & MASK_2); }
		void		setPADValue(u32 value)					
					{ 
						if constexpr (D==0)
							return; 
						else 
						{
							id &= ~(MASK_3);
							id |= ((value << MASKSHIFT_3) & MASK_3); 
						}
					}

		u32			incIndexValue()							{ u32 v = getIndexValue();  v++; setIndexValue(v); return getIndexValue(); }
		u32			incChunkValue()							{ u32 v = getChunkValue();  v++; setChunkValue(v); return getChunkValue(); }
		u32			incCounterValue()						{ u32 v = getCounterValue();  v++; setCounterValue(v); return getCounterValue(); }
		u32			incPADValue()
					{ 
						if constexpr (D==0)
							return 0; 
						else 
						{
							u32 v = getPADValue();  
							v++; 
							setPADValue(v); 
							return getPADValue(); 
						}
					}

		void		setFromU32 (u32 u)						{ id = u; }
		u32			viewAsU32() const						{ return id; }

		u32			debug_getValueByIndex(u8 which) const 
		{ 
			switch (which)
			{
			case 0: return getIndexValue();
			case 1: return getChunkValue();
			case 2: return getCounterValue();
			case 3: return getPADValue();
			default: DBGBREAK; return 0;
			}
		}
		u32			debug_incValueByIndex(u8 which)
		{
			switch (which)
			{
			case 0: return incIndexValue();
			case 1: return incChunkValue();
			case 2: return incCounterValue();
			case 3: return incPADValue();
			default: DBGBREAK; return 0;
			}
		}
		void		debug_setValueByIndex(u8 which, u32 value)
		{
			switch (which)
			{
			case 0: setIndexValue(value); return;
			case 1: setChunkValue(value); return;
			case 2: setCounterValue(value); return;
			case 3: setPADValue(value); return;
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
	 *		A	determina il numero massimo di HANDLE riservabili (2^A)
	 *		B	determina il numero di "chunk" usati per memorizzare tutti e (2^A) i DATASTRUCT.
				Se A=8 e B=3, allora abbiamo un massimo di 256 handle allocabili, da divire in 8 chunk.
				Essendo 256 gli handle allocabili, e' necessario riservare spazio per 256 strutture di tipo DATASTRUCT ma,
				invece di allocarle tutte e 256 in un colpo solo, ne alloco 256/(2^B), quindi 256/8 quindi 32.
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

					out_handle->setFromU32(0);
					out_handle->setChunkValue(i);
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