#ifndef _Land1_enumAndDefine_h_
#define _Land1_enumAndDefine_h_
#include "../gosGameUtils/examap/gosExamap.h"
#include "../gos/gosBit.h"
#include "../gos/gosHashMap.h"

namespace Land1
{
	enum class eMeshType : u8
	{
		full = 0,
		angolo = 1,
		angolo_interno = 2,
		bordo_singolo_su = 3,
		bordo_singolo_dx = 4,
		bordo_strano = 5,

		_COUNT = 6	//deve sempre valere il num totale di opzioni disponibili (escluso COUNT))
	};


	/******************************************
	* @brief	GVC
	*			Global Vertex Coordinate
	*/
#define DEBUG__GVC_HELPER
	struct GVC
	{			GVC()																		{ }
				GVC(i16 hex_x, i16 hex_z, u16 vertex_idx)									{ set(hex_x, hex_z, vertex_idx); }
				
		void	set_as_u32 (u32 c)															{ bitmask = c; }
		void	set (const gos::examap::Coord &hex_coord, u16 vertex_idx)					{ set(hex_coord.x, hex_coord.z, vertex_idx); }
		void	set (i16 hex_x, i16 hex_z, u16 vertex_idx)
		{
			//10 bit per coordinata hex_x
			//10 bit per coordinata hex_z
			//12 bit per coordinata vertex_idx
			assert (hex_x >= -512 && hex_x < 512);
			assert (hex_z >= -512 && hex_z < 512);
			assert (vertex_idx < 4096);
			bitmask  = (hex_x + 512) << 22;
			bitmask |= (hex_z + 512) << 12;
			bitmask |= vertex_idx;

			#ifdef DEBUG__GVC_HELPER
			xx = hex_x; zz = hex_z; vidx=vertex_idx;
			#endif
		}

		void	set_invalid()																{ bitmask = u32MAX; }
		bool	is_valid() const															{ return bitmask != u32MAX; }

		gos::examap::Coord get_exa_coord() const											{ 
			gos::examap::Coord ret; 
			ret.x = ((i16)(bitmask >> 22)) - 512; 
			ret.z = ((i16)((bitmask >> 12) & 0x03FF)) - 512; 
			return ret; }
		u16		get_vertex_idx() const														{ return (u16)(bitmask & 0x00000FFF); }

		u32		get_as_u32() const															{ return bitmask; }

		int		compare (const GVC &b) const												{ if (bitmask==b.bitmask) return 0; if (bitmask>b.bitmask) return 1; return -1; }
		bool	operator== (const GVC &b) const												{ return ( bitmask == b.bitmask ); }
		bool	operator!= (const GVC &b) const												{ return ( bitmask != b.bitmask ); }

	private:
		u32		bitmask;

		#ifdef DEBUG__GVC_HELPER
		i16 xx, zz;
		u16 vidx;
		#endif
	};


	/******************************************
	* @brief	ExaR
	*			Tutto quello che serve per renderizzare
	*			un exa
	*/
	class ExaR
	{
	public:
		static constexpr u8 NUM_MAX_QUADS = 6;

	public:
		struct VtxInfo
		{
			u8	num_quad;				//num quad centrati sul vtx i-esimo
			u8 	material_index;
			u16	height;
			eMeshType	mesh_type[NUM_MAX_QUADS];	//uno per ogni quad
			u16	idx_list[1 + NUM_MAX_QUADS*2];		//idx0 = centro, gli altri 2*num_quad in senso orario
		};

	public:
		static ExaR*	alloc (gos::Allocator *allocatorIN, u32 num_vtxInfoIN, u32 num_vtx_totIN)
		{
			assert (num_vtxInfoIN < u16MAX);
			assert (num_vtx_totIN < u16MAX);

			ExaR *ret = GOSALLOCT(ExaR*, allocatorIN, sizeof(ExaR));
			ret->num_vtxInfo = num_vtxInfoIN;
			ret->num_vtx_tot = num_vtx_totIN;
			ret->vtxInfoList = GOSALLOCT(VtxInfo*, allocatorIN, sizeof(VtxInfo) * num_vtxInfoIN);
			ret->vtxList = GOSALLOCT(gos::vec2f*, allocatorIN, sizeof(gos::vec2f) * num_vtx_totIN);
			return ret;
		}

		static void 	free (gos::Allocator *allocatorIN, ExaR *exar)
		{
			GOSFREE (allocatorIN, exar->vtxInfoList);
			GOSFREE (allocatorIN, exar->vtxList);
			GOSFREE (allocatorIN, exar);
		}

	public:
		u16			num_vtxInfo;		//quelli che compongono l'exa originale
		u16			num_vtx_tot;		//tutti quelli che sono in vtxList
		VtxInfo		*vtxInfoList;		//una VtxInfo per ogni vtx-originale
		gos::vec2f	*vtxList;			//tutti i vtx utili al rendering
	};


}// namespace Land1

#endif //_Land1_enumAndDefine_h_


