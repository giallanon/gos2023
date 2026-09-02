#ifndef _land_bigFile_h_
#define _land_bigFile_h_
#include "enumAndDefine.h"
#include "gosMagicUID.h"
#include "gosUniqueSortedList.h"
#include "../gosGameUtils/gosGameUtils.h"

namespace land
{
	/****************************************
	 * @brief	BigFile
	 * 
	 */
	class BigFile
	{
	public:
		static bool		create (const char *filename, u32 chunk_size, u32 num_chunk);

	public:
						BigFile();

		bool			open_1 (gos::Allocator *allocator, const char *filename, u32 num_max_cached_chunk);
		bool			open_2 (gos::Allocator *allocator, const char *filename, u32 max_memory_for_cache);
		void 			close();

		u32				get_sizeof_cache() const 											{ return chunk_size * num_max_cached_chunk; }

		/***
		 * @brief		get_chunk
		 *				Se il chunk NON e' in cache, lo carica.
		 *				Ritorna il pt al chunk cachato */		 
		const void*		get_chunk (u32 chunk_num);

		/***
		 * @brief		update_whole_chunk
		 *				Se il chunk e' gia' in cache, aggiorna la cache e poi salva il chunk su disco.
		 *				Se il chunk NON e' in cache, si limita ad aggiornarlo su disco */
		void			update_whole_chunk (u32 chunk_num, const void *src, u32 sizeof_chunk);

		/***
		 * @brief		get_chunk_for_update
		 *				Se il chunk NON e' gia' in cache, lo carica.
		 *				Marca il chunk cachato come "to be saved" il che vuol dire che se questo chunk viene rimosso dalla cache, prima viene salvato su disco e poi rimosso.
		 *				Alternativamente, un comando di "save all updated chunk" scatena il salvataggio di tutti i chunk marcati come "to be saved" */
		void*			get_chunk_for_update (u32 chunk_num);

		void			save_all_updated_chunk();

	private:
		static constexpr u32 VERSION = gos::magic::_makeID (0x01A770, 0x01);
		static constexpr u32 SIZE_OF_HEADER = 64;

	private:
		struct CachedChunk
		{
			static constexpr u8	FLAG__TO_BE_SAVED = 0;
			u32			lru;
			u32			chunk_num;
			void		*p;
			gos::Flag8	flag;
		};

	private:
		bool			priv__open (gos::Allocator *allocator, const char *filename);
		void			priv__alloc_cache (u32 num_max_cached_chunk);
		void 			priv__free();
		void 			priv__load_chunk_into_cache (u32 chunk_num, u32 cache_index, u32 timenow_msec);
		u32 			priv__is_already_cached (u32 chunk_num) const;
		void 			priv__save_chunk (u32 chunk_num, const void *src, u32 sizeof_chunk);
		u32 			priv__get_chunk_or_load (u32 chunk_num);

	private:
		gos::Allocator	*localAllocator;
		char 			debug_only_fname[128];
		u32 			chunk_size;
		u32 			num_total_chunk_in_file;
		u32 			start_of_chunk_data;

		gos::File		hFile;
		CachedChunk		*cached_chunk_list;
		u8				*cached_chunk_pt;
		u32 			num_cached_chunk;
		u32 			num_max_cached_chunk;
	};

} //namespace land


#endif //_land_bigFile_h_

