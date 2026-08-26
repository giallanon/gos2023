#include "map.h"
#include "gosImageBufferRGBA.h"
#include "gosGeomIntersect3D.h"

using namespace gos;
using namespace land;

typedef gos::AllocatorHeap<gos::AllocPolicy_Track_simple, gos::AllocPolicy_Thread_Unsafe>	LandMapMemAllocator;

//********************************
bool Map::create (const char *fullpath_to_png, u32 chunk__num_vtx_per_lato, f32 scala_xz__m, f32 scala_h__m)
{
	gos::err::clear();


	if (chunk__num_vtx_per_lato < 5)
	{
		logger::err ("chunk__num_vtx_per_lato must be at least 5\n");
		return false;
	}
	if (!GOS_IS_POWER_OF_TWO(chunk__num_vtx_per_lato-1))
	{
		logger::err ("chunk__num_vtx_per_lato must be equal to '1 + a power of 2 '\n");
		return false;
	}


	u16 *height = NULL;
	u32 height_dimx = 0;
	u32 height_dimy = 0;

	//carico l'immagine e genero height[]
	{
		image::BufferRGBA src_image;
		if (!src_image.loadFromFile (gos::getScrapAllocator(), fullpath_to_png))
			return false;
	
		//converto l'altezza
		//La risoluzione e di 0.1m
		height_dimx = src_image.getW();
		height_dimy = src_image.getH();
		height = GOSALLOCT(u16*, gos::getScrapAllocator(), height_dimx * height_dimy * sizeof(u16) );
		{
			const u8 *buffer = src_image.getBuffer();
			
			u32 ct1 = 0;
			u32 ct2 = 0;
			while (ct1 < height_dimx * height_dimy)
			{
				const u8 red = buffer[ct2];
				ct2+=4;

				f32 h = (f32)red * scala_h__m;
				h = math::floor (h*10.0);

				u32 h2 = (u32)h;
				if (h2 > 0xFFFF)
				{
					DBGBREAK;
					h2 = 0xFFFF;
				}

				height[ct1++] = (u16)h2;
			}
		}
		src_image.free (gos::getScrapAllocator());
	}


	while (1)
	{
		char s[1024];

		//salvo tutte le info in una directory che ha come nome lo stesso nome dell'immagine di input
		char out_folder[1024];
		fs::resolvePath (fullpath_to_png, out_folder, sizeof(out_folder));
		fs::remove_ext_in_place (out_folder);
		fs::folderDeleteAllFileRecursively (out_folder, eFolderDeleteMode::deleteAlsoTheSubfolderAndTheMainFolder);
		if (!fs::folderCreate (out_folder))
		{
			logger::err ("Unable to create folder %s\n", out_folder);
			break;
		}

		Map::Header header;
		header.version = Map::VERSION;
		header.chunk__num_vtx_per_lato = chunk__num_vtx_per_lato;
		header.chunk__num_x = (height_dimx-1) / (chunk__num_vtx_per_lato-1);
		header.chunk__num_y = (height_dimy-1) / (chunk__num_vtx_per_lato-1);
		header.scala_xz__m = scala_xz__m;
		header.chunk__border_len__m = (f32)(chunk__num_vtx_per_lato-1) * scala_xz__m;
		header.chunk__size_in_byte = sizeof(ChunkData) * chunk__num_vtx_per_lato * chunk__num_vtx_per_lato;

		//voglio che <header.chunk__size_in_byte> sia un multiplo di 64 per questioni di allocazione memoria
		header.chunk__size_in_byte = GOS_ALIGN_NUMBER_TO_POWER_OF_TWO(header.chunk__size_in_byte, 64);

		//inizio a creare i chunk
		if (header.chunk__num_x < 1 || header.chunk__num_y < 1)
		{
			logger::err ("heightmap is too small for a chunk size of %d", chunk__num_vtx_per_lato);
			break;
		}

		ChunkData *data = GOSALLOCT(ChunkData*, gos::getScrapAllocator(), header.chunk__size_in_byte);
		for (u32 cy=0; cy<header.chunk__num_y; cy++)
		{
			for (u32 cx=0; cx<header.chunk__num_x; cx++)
			{
				const u32 ct_hmap = cy * (height_dimx * (chunk__num_vtx_per_lato-1)) +
									cx * (chunk__num_vtx_per_lato-1);

				u32 ct_data = 0;
				for (u32 y=0; y<chunk__num_vtx_per_lato; y++)
				{
					u32 ct = ct_hmap + y*height_dimx;
					for (u32 x=0; x<chunk__num_vtx_per_lato; x++)
					{
						assert (ct < height_dimx * height_dimy);
						data[ct_data].height = height[ct++];
						ct_data++;
					}
				}

				//salvo il chunk
				const u32 out_cx = cx;
				const u32 out_cy = header.chunk__num_y - cy -1;
				sprintf_s (s, sizeof(s), "%s/chunk_%05d_%05d", out_folder, out_cx, out_cy);
				fs::fileSaveBuffer (s, data, header.chunk__size_in_byte);

				// //debug
				// if (out_cx == 0 && out_cy == 0)
				// {
				// 	sprintf_s (s, sizeof(s), "%s/chunk_%05d_%05d.txt", out_folder, out_cx, out_cy);
				// 	gos::File f;
				// 	fs::fileOpenForW (&f, s);
				// 	ct_data = 0;
				// 	for (u32 y=0; y<chunk__num_vtx_per_lato; y++)
				// 	{
				// 		for (u32 x=0; x<chunk__num_vtx_per_lato; x++)
				// 		{
				// 			sprintf_s (s, sizeof(s), "%05d ", data[ct_data++].height);
				// 			fs::fileWrite (f, s, 6);
				// 		}

				// 		s[0] = '\n';
				// 		fs::fileWrite (f, s, 1);
				// 	}
				// 	fs::fileClose(f);
				// }
			}
		}
		GOSFREE(gos::getScrapAllocator(), data);


		//salvo l'header
		sprintf_s (s, sizeof(s), "%s/map", out_folder);
		fs::fileSaveBuffer (s, &header, sizeof(Map::Header));




		//fine
		break;
	} //while(1);
	
	GOSFREE(gos::getScrapAllocator(), height);
	return !err::anyError();
}


//********************************
Map::Map()
{
	LandMapMemAllocator *myAllocator = GOSNEW(gos::getSysHeapAllocator(), LandMapMemAllocator)("LandMap");
	myAllocator->setup (1024 * 1024 * 128); //128MB
	this->localAllocator = myAllocator;

	chunk_data = NULL;
}

//********************************
Map::~Map()
{ 
	priv__free(); 
	GOSDELETE(gos::getSysHeapAllocator(), localAllocator);
	localAllocator = NULL;
}

//********************************
void Map::priv__free()
{
	if (NULL != chunk_data)			GOSFREE_AND_NULL (localAllocator, chunk_data);
}

//********************************
u32 Map::priv__chunk_calc_offset  (u32 cx, u32 cy) const
{
	return cx * header.chunk__size_in_byte  +  cy * header.chunk__size_in_byte * header.chunk__num_x;
}

//********************************
Map::ChunkData*	Map::priv__get_pointer_to_chunk (u32 cx, u32 cy)
{
	return reinterpret_cast<Map::ChunkData*>( &chunk_data[priv__chunk_calc_offset(cx, cy)] );
}

//********************************
const Map::ChunkData* Map::chunk__get (u32 cx, u32 cy) const
{
	if (cx >= header.chunk__num_x || cy >= header.chunk__num_y)
		return NULL;
	return reinterpret_cast<const Map::ChunkData*>( &chunk_data[priv__chunk_calc_offset(cx, cy)] );
}

//********************************
bool Map::load (const char *path_to_folderIN)
{
	err::clear();
	fs::resolvePath (path_to_folderIN, path_to_folder, sizeof(path_to_folder));
	
	char s[1024];
	{
		sprintf_s (s, sizeof(s), "%s/map", path_to_folder);
		u32 fsize;
		u8 * buffer = fs::fileLoadInMemory (gos::getScrapAllocator(), s, &fsize);
		if (NULL == buffer)
		{
			logger::err ("unable to load %s/map\n", path_to_folder);
			return false;
		}
		memcpy (&header, buffer, sizeof(header));
		GOSFREE(gos::getScrapAllocator(), buffer);
	}

	//verifica versione
	if (!magic::signatureMatch (header.version, Map::VERSION) || !magic::versionMatch (header.version, Map::VERSION))
	{
		logger::err ("invalid signature or version\n");
		return false;
	}

	//alloco i chucnk e li carico tutti (per ora)
	chunk_data = GOSALLOCT(u8*, localAllocator, header.chunk__size_in_byte * header.chunk__num_x * header.chunk__num_y);
	for (u32 cy=0; cy<header.chunk__num_y; cy++)
	{
		for (u32 cx=0; cx<header.chunk__num_x; cx++)
		{
			sprintf_s (s, sizeof(s), "%s/chunk_%05d_%05d", path_to_folder, cx, cy);
			
			gos::File f;
			if (!fs::fileOpenForR(&f, s))
			{
				logger::err ("can't find %s", s);
			}
			else
			{
				fs::fileRead (f, priv__get_pointer_to_chunk(cx, cy), header.chunk__size_in_byte);
				fs::fileClose(f);
			}
		}
	}

	return !err::anyError();
}

//********************************
f32 Map::get_height (f32 wx, f32 wz) const
{
	const i32 cx = math::floor( wx / header.chunk__border_len__m);
	const i32 cy = math::floor( wz / header.chunk__border_len__m);
	const ChunkData *chunk = chunk__get (cx, cy);
	if (NULL == chunk)
		return 0;

	wx -= cx * header.chunk__border_len__m;
	wz -= cy * header.chunk__border_len__m;

	assert (wx >= 0);
	assert (wz >= 0);

	// wx,wz e' nel quad qx,qz del chunk dove il quad 0,0 e' quello in alto a sx
	const u32 qx = (u32)math::floor( wx / header.scala_xz__m);
	const u32 qz = (header.chunk__num_vtx_per_lato - 1) - (u32)math::floor( wz / header.scala_xz__m);
	assert (qx < header.chunk__num_vtx_per_lato);
	assert (qz < header.chunk__num_vtx_per_lato);


	const f32 x0 = header.scala_xz__m * qx;
	const f32 x1 = x0 + header.scala_xz__m;
	const f32 z0 = header.scala_xz__m * qz;
	const f32 z1 = z0 - header.scala_xz__m;
	assert (wx >= x0 && wx <= x1);
	assert (wz >= z0 && wz <= z1);

	const vec3f v0 (x0, z0, chunk[qz     * header.chunk__num_vtx_per_lato + qx].height);
	const vec3f v1 (x1, z0, chunk[qz     * header.chunk__num_vtx_per_lato + (qx + 1)].height);
	const vec3f v2 (x1, z1, chunk[(qz+1) * header.chunk__num_vtx_per_lato + (qx + 1)].height);
	const vec3f v3 (x0, z1, chunk[(qz+1) * header.chunk__num_vtx_per_lato + qx].height);

	return ( (v0.y + v1.y + v2.y + v3.y) * 0.25f) * 0.1f;
}

//***********************************
u32 Map::calc_visible_chunk (gos::geom::Camera3 *cam, gos::FastArray<ChunkCoord> *out) const
{
	assert (NULL != out);
	out->reset();

	const geom::Frustum3 fr = cam->get_frustumWC();
	const f32 VIEW_DISTANCE_SQUARED = fr.get_far_distance() * fr.get_far_distance();

	geom::AABB3 aabb;
	fr.calc_AABB (&aabb);

	const i32 xmin = 0; //math::floor( aabb.vmin.x / header.chunk__border_len__m);
	const i32 xmax = 7; //math::floor( aabb.vmax.x / header.chunk__border_len__m);
	const i32 zmin = 0; //math::floor( aabb.vmin.z / header.chunk__border_len__m);
	const i32 zmax = 7; //math::floor( aabb.vmax.z / header.chunk__border_len__m);

	for (i32 z=zmin; z<=zmax; z++)
	{
		for (i32 x=xmin; x<=xmax; x++)
		{
			aabb.vmin.set (x*header.chunk__border_len__m, -1e36f, z*header.chunk__border_len__m);
			aabb.vmax.x = aabb.vmin.x + header.chunk__border_len__m;
			aabb.vmax.y = 1e36f;
			aabb.vmax.z = aabb.vmin.z + header.chunk__border_len__m;

			if (eClipResult::outside != geom::AABB3__intersect_frustum3 (aabb, fr) )
			{
				ChunkCoord cc;
				cc.origin.set (aabb.vmin.x, aabb.vmin.z);
				cc.center.set (cc.origin.x + header.chunk__border_len__m*0.5f, cc.origin.y + header.chunk__border_len__m*0.5f);
				
				cc.distance2_from_pov = math::distance2 (cam->pos.o, vec3f(cc.center.x, 0, cc.center.y));
				//if (cc.distance2_from_pov < VIEW_DISTANCE_SQUARED)
				{
					cc.x = (i16)x;
					cc.z = (i16)z;
					out->append (cc);
				}
			}

		}
	}

	return out->getNElem();
}