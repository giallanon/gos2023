#ifndef _land_enumAndDefine_h_
#define _land_enumAndDefine_h_
#include "../gosGameUtils/gosGameUtils.h"
#include "gosEngine.h"
#include "gosMath.h"

namespace land
{
	static constexpr f32 LAND__VIEW_DISTANCE_m = 1000.0f;

	//map point resolution
	enum class Resol : u8
	{
		_0125m	= 0,		//0.125m
		_025m	= 1,		//0.25m
		_05m	= 2,		//0.5m
		_1m		= 3,		//1m
		_2m		= 4,		//2m
		_4m		= 5,
		_8m		= 6,
		_16m	= 7,
		_32m	= 8,
		_64m	= 9,
		_128m	= 10,
		_256m	= 11		//256m
	};

	/***********************************
	 * @brief	ChunkCoord
	 * 
	 */
	class ChunkCoord
	{
	public:
		void	set (u32 lod, u32 cx, u32 cy)
		{
			//20 bit cx, 20 bit cy, 8 bit lod e ne avanza ancora un po'
			_encoded = cx & 0x000FFFFF;
			_encoded |= (u64)(cy & 0x000FFFFF) << 20;
			_encoded |= (u64)(lod & 0x0000000F) << 40;
		}
		
		u32 	get_cx() const								{ return (u32) _encoded & 0x000FFFFF; }
		u32 	get_cy() const								{ return (u32) (_encoded >> 20) & 0x000FFFFF; }
		u32 	get_lod() const								{ return (u32) (_encoded >> 40) & 0x0000000F; }

	public:
		u64		_encoded;
	};

	typedef gos::FastArray<ChunkCoord>	ChunkCoordList;


	/***********************************
	 * @brief	CompressedH
	 * 			Altezza compresa tra 0.0 e 6553.5 m  (in step da 0.1m)
	 */
	class CompressedH
	{
	public:
		void	set (f32 metri)			{ _encoded = (u16)gos::math::round(metri * 10.0f); }
		f32 	decode() const			{ return (f32)_encoded * 0.1f; }

	public:
		u16		_encoded;
	};


	/***********************************
	 * @brief	CompressedNorm
	 * 			Normale compressa in 32bit
	 */
	class CompressedNorm
	{
	public:
		void		set (const gos::vec3f n)			{ _encoded = gos::utils::normal_encode_octahedral (n); }
		gos::vec3f 	decode() const						{ return gos::utils::normal_decode_octahedral(_encoded, false); }

	public:
		u32			_encoded;
	};

} //namespace land




#endif //_land_enumAndDefine_h_
