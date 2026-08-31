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

	public:
		static bool		create (const char *save_path, const CreateData &create);

	public:
						Map();
						~Map();

		bool			open (const char *folder_path);

		/**
		 * @brief	get_map_data
		 * 			Dato un punto <px,py> sulla mappa di risoluzione <resolution>, filla <out> con tutti i PointData rilevanti.
		 * 			<out> deve essere un array di PointData grosso almeno <num_point_per_lato> * <num_point_per_lato> * sizeof(PointData)
		 */
		bool 			map__get_data (u32 px, u32 py, land::Resol resolution, u32 num_point_per_lato, PointData *out, u32 sizeof_out);
		bool 			map__get_data (const QTreeCoord cc, PointData *out, u32 sizeof_out);

		u32 			map__get_num_points_per_lato() const					{ return mapInfo[0].num_point_per_lato; }
		u32 			map__get_num_lod() const 								{ return num_mapInfo; }
		f32				map__get_border_size__m() const 						{ return map_border_size__m; }
		land::Resol		map__get_best_resolution() const						{ return mapInfo[num_mapInfo-1].resolution; }
		land::Resol		map__get_worst_resolution() const						{ return mapInfo[0].resolution; }
		gos::vec2f		map__get_topLeft_WC() const								{ return map_topLeft_WC; }


		u32 			qtree__calc_visibility (gos::geom::Camera3 *cam, QTreeCoordList *out)			{ return qtree.calc_visibility (cam, out); }
		void 			qtree__aabb_from_coord (const QTreeCoord cc, gos::geom::AABB3 *out) const		{ return qtree.aabb_from_coord (cc, out); }
		u32 			qtree__get_num_vtx_per_chunk_side () const 										{ return QTREE__NUM_VTX_PER_CHUNK_SIDE; }


	private:
		/****************************************
		 * @brief	MapQTree
		 */
		class MapQTree
		{
		public:
						MapQTree();
						~MapQTree()																	{ unsetup(); }

			void 		setup (gos::Allocator *allocator, const Map *map, u32 num_vtx_per_chunk_side);
			void 		unsetup();

			u32 		calc_visibility (gos::geom::Camera3 *cam, QTreeCoordList *out);

			void 		aabb_from_coord (const QTreeCoord cc, gos::geom::AABB3 *out) const;

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
			gos::Allocator		*localAllocator;
			LODInfo				lodInfo[NUM_MAX_LOD];
			u32 				num_lod;
			u32 				num_vtx_per_chunk_side;
			gos::geom::AABB3	map_aabb;
			QTreeCoordList		tmp_ccList1, tmp_ccList2;
		};	




	private:
		static constexpr u32 VERSION = gos::magic::_makeID (0x01A782, 0x01);
		static constexpr u32 QTREE__NUM_VTX_PER_CHUNK_SIDE = 65;

	private:
		struct MapInfo
		{
			u32		num_point_per_lato;		//totale dei punti della mappa
			u32		num_chunk_per_lato;		//internamente la mappa e' in <num_chunk_per_lato> x <num_chunk_per_lato> chunk
			f32		border_size__m;
			Resol 	resolution;
			BigFile	*chunkData;
		};


	private:
		void 				priv__free();
		bool 				priv__map_get_data (u32 px, u32 py, MapInfo *mi, u32 num_point_per_latoIN, PointData *out, u32 sizeof_out);

	private:
		gos::Allocator	*localAllocator;
		u32				num_mapInfo;
		MapInfo 		*mapInfo;
		f32				map_border_size__m;
		gos::vec2f		map_topLeft_WC;			//coordinate dell'angolo in alto a sx della mappa (world coodinate)
		MapQTree		qtree;
	};



} //namespace land


#endif //_land_map_h_

