#ifndef _Land1_enumAndDefine_h_
#define _Land1_enumAndDefine_h_
#include "../gosGameUtils/examap/gosExamap.h"
#include "../gos/gosBit.h"
#include "../gos/gosHashMap.h"

namespace Land1
{
	enum class eMeshType : u8
	{
		boh = 0,
		angolo = 1,
		full = 2,
		bordo_doppio = 3,
		bordo_singolo_dx = 4,
		bordo_singolo_su = 5,

		_COUNT = 6	//deve sempre valere il num totale di opzioni disponibili (escluso COUNT))
	};


	//Global Vertex Coordinate
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
	};

}// namespace Land1

#endif //_Land1_enumAndDefine_h_


