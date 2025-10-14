#ifndef _gosEntityList_h_
#define _gosEntityList_h_
#include "gosEntityEnumAndDefine.h"

namespace gos
{
	namespace ent
	{
		/********************************
		* @brief	List
		*			Mantiene una lista univoca di Entity
		* 
		*/
		class List
		{
		public:
			//Uso 1 bit per sapere se <ent> e' già in lista
			//Una pagina grossa 0xFFFF byte mi tiene traccia di 65536 * 8 = 524.288 entities.
			//Ipoteticamente posso avere fino a 0xFFFFFFFF ent, il che vuol dire che mi servono 8192 pagine.
			//Ogni pagina e' un pt, quindi 8192 * sizeof(pt) = 65K
			static constexpr u32 PAGE_SIZE = 64 * 1024;
			static constexpr u32 NUM_PAGES = 8192;

		public:
					List();
					~List()																		{ priv_free(); }

			void	setup (gos::Allocator *allocator);

			void	reset();
			bool	addIfNotExists (const Entity ent);

			u32							getCount() const										{ return entList.getNElem(); }
			const FastArray<Entity>*	getList() const											{ return &entList; }

		private:
			struct sPage
			{
				u32		numElement;
				u8		*bitmask;
				u64		lastTimeUsed;

				void	reset()						{ assert(NULL != bitmask); memset (bitmask, 0, List::PAGE_SIZE); numElement = 0;  }
			};

		private:
			void	priv_free();
			void	priv_calcAddress (const Entity ent, u32 *out_pageIndex, u32 *out_byte, u32 *out_bitmask) const;
			void	priv_allocPage (u32 pageIndex);
			void	priv_freePage (u32 pageIndex);
			bool	priv_pageExists (u32 pageIndex) const;

		private:			
			gos::Allocator		*allocator;
			sPage				*pageList;
			FastArray<Entity>	entList;
			
			FastArray<u16>		allocatedPage_indexList;		//elenco degli indici delle pagine allocate
			u8					allocatedPage_bitmask[1024];	//mi dice se una page e' allocata oppure no  (1024 == NUM_PAGES / 8 )

		};
	} //namespace ent
} //namespace gos


#endif //_gosEntityList_h_


