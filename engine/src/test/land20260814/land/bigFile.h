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

		bool			open (gos::Allocator *allocator, const char *filename, u32 num_max_cached_chunk);
		void 			close();

		const void*		get_chunk (u32 chunk_num);
		void			memcpy_chunk (u32 chunk_num, void *dest, u32 sizeof_dest);

		void			write_chunk (u32 chunk_num, const void *src, u32 sizeof_chunk);

		u32				get_sizeof_cache() const 											{ return chunk_size * num_max_cached_chunk; }

	private:
		static constexpr u32 VERSION = gos::magic::_makeID (0x01A770, 0x01);
		static constexpr u32 SIZE_OF_HEADER = 64;

	private:
		struct CachedChunk
		{
			u32		lru;
			u32		chunk_num;
			void	*p;			
		};

	private:
		void 			priv__free();
		void 			priv__load_chunk (u32 chunk_num, u32 cache_index, u32 timenow_msec);
		u32 			priv__is_already_cached (u32 chunk_num) const;
		void 			priv__save_chunk (u32 chunk_num, const void *src, u32 sizeof_chunk);

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

