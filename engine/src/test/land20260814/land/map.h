#ifndef _land_map_h_
#define _land_map_h_
#include "enumAndDefine.h"
#include "bigFile.h"
#include "gosMagicUID.h"
#include "gosUniqueSortedList.h"


namespace land
{
	/****************************************
	 * @brief	Map
	 * 			Idealmente dovrebbe fornire in tempo "reale" info su un qualunque punto della mappa
	 *			ad una qualunque risoluzione.
	 *			Internamente cacha la mappa in chunk e LOD ma questo non dovrebbe essere importante per
	 *			l'utilizzatore il quale sostanzialmente dovrebbe solo usare get_point (x,y)

		8192 x 8192 = 64M   x 8 byte a punto = 512M
		4096 x 4096 = 16M   x 8 byte a punto = 128M
		2048 x 2048 =  4M   x 8 byte a punto =  32M
		1024 x 1024 =  1M   x 8 byte a punto =   8M


	 */
	class Map
	{
	public:
		//**************************************
		struct CreateData
		{
			u32		default_map__border_size__point;		//potenza del 2
			f32		default_height__m;
			Resol	default_map__resolution;
			Resol	resolution_min;

			CreateData()
			{
				default_map__border_size__point = 4096;
				default_map__resolution = Resol::_4m;

				default_height__m = 10.0f;
				resolution_min = Resol::_05m;
			}
		};

		//**************************************
		struct PointData	//ogni punto della mappa contiene le seguenti info
		{
		public:
			CompressedNorm	norm;
			CompressedH		height;
			u8				ao;
			u8				materialID;
		};


	public:
		static bool		create (const char *save_path, const CreateData &create);

	public:
						Map();
						~Map();

		bool			open (const char *folder_path);


		u32 			map__get_num_points_per_lato() const					{ return mapInfo[0].num_point_per_lato; }
		u32 			map__get_num_lod() const 								{ return num_mapInfo; }
		f32				map__get_border_size__m() const 						{ return map_border_size__m; }
		land::Resol		map__get_best_resolution() const						{ return mapInfo[num_mapInfo-1].resolution; }
		land::Resol		map__get_worst_resolution() const						{ return mapInfo[0].resolution; }
		gos::vec2f		map__get_topLeft_WC() const								{ return map_topLeft_WC; }

	private:
		static constexpr u32 VERSION = gos::magic::_makeID (0x01A782, 0x01);

	private:
		struct MapInfo
		{
			u32		num_point_per_lato;
			u32		num_chunk_per_lato;
			f32		border_size__m;
			Resol 	resolution;
			BigFile	*chunkData;
		};

		struct ChunkInfo
		{
			f32	min_height__m;
			f32	max_height__m;
		};


	private:
		void 				priv__free();
		const ChunkInfo*	priv__get_chunkInfo (u32 lod, u32 cx, u32 cy);

	private:
		gos::Allocator	*localAllocator;
		u32				num_mapInfo;
		MapInfo 		*mapInfo;
		f32				map_border_size__m;
		gos::vec2f		map_topLeft_WC;			//coordinate dell'angolo in alto a sx della mappa (world coodinate)
	};


	/****************************************
	 * @brief	MapQTree
	 */
	class MapQTree
	{
	public:
				MapQTree();
				~MapQTree();
		void 	setup (const Map *map);

		u32 	calc_visibility (gos::geom::Camera3 *cam, ChunkCoordList *out);

		void 	aabb_from_chunkCoord (const ChunkCoord cc, gos::geom::AABB3 *out) const;
		u32 	get_num_vtx_per_chunk_side () const 										{ return num_vtx_per_chunk_side; }
		land::Resol get_resolution_from_chunkCoord (const ChunkCoord cc) const;
		
	private:
		static constexpr u8 NUM_MAX_LOD = 32;

	private:
		struct LODInfo
		{
			u32	num_chunk_per_lato;
			f32 chunk_border_size__m;
			f32 min_visible_dist_squared__m;
			land::Resol resol;
		};

	private:
		

	private:
		LODInfo				lodInfo[NUM_MAX_LOD];
		u32 				num_lod;
		u32 				num_vtx_per_chunk_side;
		gos::geom::AABB3	map_aabb;
		ChunkCoordList		tmp_ccList1, tmp_ccList2;
	};	
} //namespace land


#endif //_land_map_h_

