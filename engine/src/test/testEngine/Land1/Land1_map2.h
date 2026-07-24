#ifndef _Land1_map2_h_
#define _Land1_map2_h_
#include "Land1_enumAndDefine.h"
#include "Land1_exaGenerator.h"
#include "../gosGameUtils/examap/gosExamap.h"
#include "gosHashMap.h"
#include "../gosGeom/gosGeomAABB3.h"

namespace Land1
{
	/*******************************
	* Map2
	*
	*/
	class Map2
	{
	public:
		static constexpr f32 EXA_HEIGHT_MUL = 0.01f;

		struct Node
		{
			gos::vec2f	pos;
			u8 	material_index;
			u8	num_adj_vtx;
			u16	height;
			GVC	gvc;
			GVC	connected_node[6];
			GVC	other_node[6];			//ogni quad e' composto da <coord>, connected_node[i], connected_node[i+1], other_node[i]
										//<other_node> non e' direttamente linkato a questo node
			gos::vec2f	quad_center[6];
			eMeshType	mesh_type[6];
		};

		struct Vtx
		{
			gos::vec3f	pos;
			u8 	material_index;
			u8	num_adj_vtx;
			u16	height;
			u16	adj_vtx_list[6];
			GVC	gvc;
		};

	public:
				Map2();
				~Map2()																{ unsetup(); }

		void	setup (gos::Allocator *allocator);
		void	unsetup();

		//======================= map creation
		void	map_create (f32 exa_radius_world, u32 random_seed);

				//crea un nuovo exa e lo adda alla mappa in posizione <coord>
		void	exa__add (const gos::examap::Coord coord);
		void	exa__add_with_radius (const gos::examap::Coord center_coord, u32 radius);

		//======================= utils
				//filla <outList> con tutti i vtx necessari a renderizzare l'exa.
				//Ritorna il numero di "vertici originali" dell'exa. Ogni vtx e' collegato ad altri vertici ma non tutti i vtx
				//fanno parte di <exa_coord>. Il num di vtx ritornato e' il num di vtx di <outList> che fanno effettivamente
				//parte dell'exa
		u32		get_exa_vtxList (const gos::examap::Coord &exa_coord, gos::FastArray<Vtx> &outList, bool bClear_outList=true) const;
		
		ExaR* 	calc_exaR (gos::Allocator *allocator, const gos::examap::Coord &exa_coord) const;

		void 	set_node_material_index (const GVC gvc, u8 material_index);
		void 	set_node_height (const GVC gvc, u16 height);
		void 	inc_node_height (const GVC gvc, u16 h);
		void 	dec_node_height (const GVC gvc, u16 h);

		//======================= query
		gos::vec3f 			exa_coord_to_world (const gos::examap::Coord &exa_coord) const			{ return exacc.exa_coord_to_world(exa_coord); }
		gos::examap::Coord	world_coord_to_exa (const gos::vec3f &world_coord) const				{ return exacc.world_coord_to_exa (world_coord); }
		gos::examap::Coord	world_coord_to_exa (f32 x, f32 z) const									{ return exacc.world_coord_to_exa (x,z); }
		f32					get_exa_world_radius() const											{ return exacc.get_exa_world_radius(); }
		gos::vec3f			get_map_world_center() const											{ return exacc.get_map_world_center(); }
		bool				world_coord_to_GVC  (const gos::vec3f &world_coord, GVC *out) const;
		bool				GVC_to_world_coord  (const GVC gvc, gos::vec3f *out_world_coord) const;
		bool				GVC_to_node (const GVC gvc, Node *out) const;

		bool				world_ray_to_GVC  (const gos::vec3f &world_o, const gos::vec3f &world_dir, GVC *out) const;
		bool				does_world_ray_intersect_GVC  (const gos::vec3f &world_o, const gos::vec3f &world_dir, const GVC &gvc) const;

		bool				exaInfo__get_last_time_updated(const gos::examap::Coord &exa_coord, u16 *out__last_time_updated) const;
		bool				exaInfo__get_AABB (const gos::examap::Coord &exa_coord, gos::geom::AABB3 *out__aabb) const;

	private:
		struct ExaInfo
		{
		public:
			void	reset()					{ num_node = 0; node_list = NULL; last_time_updated = 0; }

		public:
			Node				*node_list;
			gos::geom::AABB3	aabb;
			gos::examap::Coord	coord;
			u16					num_node;
			u16					last_time_updated;
			
			
		};



	private:
		typedef gos::FastHashMap<GVC, Node>			Nodemap;
		typedef gos::FastHashMap<gos::examap::Coord, ExaInfo>		Examap;


	private:
		void 		priv_destroy_map();
		bool		priv_examap__update_node (const GVC gvc, const Node &nodeIN);
		bool 		priv_examap__get_node (const GVC gvc, Node *out) const;
		Node*		priv_examap__get_nodePointer (const GVC gvc);
		const Node*	priv_examap__get_nodePointer (const GVC gvc) const;
		void		priv_examap__merge_node_adj (Node *nodeIN, const Node *other_node);
		void		priv_examap__recalc_AABB (ExaInfo *exa);

		void		priv_node_to_vtx (const Node *node, Vtx *out) const;
		void		priv_node__update_quad_center (Node *node);
		void 		priv_calc_mesh_type (const GVC gvc);
		void 		priv_do_set_node_height (const GVC gvc, Node *node, u16 height);
		bool		priv_world_ray_intersect_quad (const ExaInfo *exa, const gos::vec3f &world_o, const gos::vec3f &world_dir, f32 rayLen, u16 *out__node_idx) const;
		bool		priv_world_ray_intersect_quad (const Node *node, const gos::vec3f &world_o, const gos::vec3f &world_dir, f32 rayLen) const;


	private:
		gos::Allocator				*localAllocator;
		gos::Random					rnd;
		gos::examap::CoordConverter	exacc;
		Examap						examap;
		Nodemap						temp_node_list;

	};
} //namespace Land1

#endif //_Land1_map2_h_

