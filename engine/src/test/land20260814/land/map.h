#ifndef _land_map2_h_
#define _land_map2_h_
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
	public:
		struct CreateData
		{
			f32 map__border_size__m;	//una mappa quadrata di lato == map__border_size__m
			f32	map__default_height__m;	//altezza di default della mappa
			f32	lod0__scala_xz__m;		//distanza tra un vtx e l'altro in metri nel LOD 0
			u32	lod0__num_vtx_lato;		//deve essere == a 1 + una potenza del 2 (es: 129)

			CreateData()
			{
				map__border_size__m = 1000.0f;
				map__default_height__m = 10.0f;
				lod0__scala_xz__m = 0.5f;
				lod0__num_vtx_lato = 129;
			}
		};

	public:
		static bool			create (const char *save_path, const CreateData &create);



	public:
		struct ChunkData
		{
			u16	height;	//espressa in step da 0.1m
		};


	public:
						Map();
						~Map();

		bool			load (const char *path_to_folder);
		u32 			calc_visible_chunk (gos::geom::Camera3 *cam, gos::FastArray<ChunkCoord> *out) const;


		u32					chunk__get_num_vtx_per_lato() const			{ return chunk__num_vtx_lato; }
		f32					chunk__get_border_length__m() const			{ return chunk__border_size__m; }
		f32					chunk__get_lod0_scala_xz__m() const			{ return lod0__scala_xz__m; }
		const ChunkData*	chunk__get (u32 cx, u32 cy) const;


	private:
		static constexpr u32 VERSION = gos::magic::_makeID (0x01A781, 0x01);

	private:
		struct ChunkInfo
		{
			f32	min_height__m;
			f32	max_height__m;
		};

	private:
		void			priv__free();
		bool			priv__world_to_chunk (f32 wx, f32 wz, u32 *out__cx, u32 *out__cy) const;
		bool			priv__chunk_to_world (u32 cx, u32 cy, f32 *out__wx, f32 *out__wz) const;
		const ChunkInfo* priv__chunk_get_info (u32 cx, u32 cy) const;

	private:
		gos::Allocator	*localAllocator;
		u8				*chunk_data;
		ChunkInfo		*chunk_info;
		char			path_to_folder[1024];

		f32 map__border_size__m;	//l'intera mappa e' un quadrato di di bordo <map__border_size__m>
		f32 map__min;				//l'angolo in basso a sx della mappa ha coordinate world (map__min, map__min)
		f32 map__max;				//l'angolo in alto a dx della mappa ha coordinate world (map__max, map__max)

		u32 chunk__num;				//l'intera mappa e' un quadrato di <chunk__num> x <chunk__num> chunk
		f32 chunk__border_size__m;	//ogni chunk e' un quadrato di lato <chunk__border_size__m>
		u32 chunk__num_vtx_lato;	//ogni chunk e' un quadrato di lato <chunk__num_vtx_lato> x <chunk__num_vtx_lato> vertici
		u32 mid_chunk;				//il chunk (mid_chunk, mid_chunk) e' quello con origine in woorld coord (0,0)

		u32 lod0__sizeof;			//dimensioni in byte di un chunk a lod 0 su disco
		f32 lod0__scala_xz__m;		//distanza tra un vtx e l'altro in un chunk a lod 0
		


			
	};

} //namespace land


#endif //_land_map_h_

