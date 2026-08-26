#ifndef _land_map_h_
#define _land_map_h_
#include "enumAndDefine.h"
#include "gosMagicUID.h"

namespace land
{
	/****************************************
	 * @brief	Map
	 * 
	 */
	class Map
	{
	private:
		static constexpr u32 VERSION = gos::magic::_makeID (0x01A78f, 0x01);

	public:
		struct ChunkData
		{
			u16	height;	//espressa in step da 0.1m
		};

	public:
		static bool			create (const char *fullpath_to_png, u32 chunk__num_vtx_per_lato, f32 scala_xz__m, f32 scala_h__m);

	public:
							Map();
							~Map();

		bool				load (const char *path_to_folder);

		u32 				calc_visible_chunk (gos::geom::Camera3 *cam, gos::FastArray<ChunkCoord> *out) const;

		f32					get_height (f32 wx, f32 wz) const;

		u32					chunk__get_num_vtx_per_lato() const			{ return header.chunk__num_vtx_per_lato; }
		f32					chunk__get_scala_xz__m() const				{ return header.scala_xz__m; }
		f32					chunk__get_border_length__m() const			{ return header.chunk__border_len__m; }
		const ChunkData*	chunk__get (u32 cx, u32 cy) const;


	private:
		struct Header
		{
			u32	version;
			u32	chunk__num_vtx_per_lato;
			u32	chunk__num_x;				//numero di chunk X
			u32	chunk__num_y;				//numero di chunk Y
			f32 scala_xz__m;				//distanza tra un vtx e l'altro all'iterno di ogni singolo chunk
			f32 chunk__border_len__m;
			u32 chunk__size_in_byte;		//size in byte di un chunk su disco
		};
		
	private:
		void		priv__free();
		u32 		priv__chunk_calc_offset  (u32 cx, u32 cy) const;
		ChunkData*	priv__get_pointer_to_chunk (u32 cx, u32 cy);

	private:
		gos::Allocator	*localAllocator;
		char			path_to_folder[1024];
		Header			header;
		u8				*chunk_data;
	};

} //namespace land


#endif //_land_map_h_

