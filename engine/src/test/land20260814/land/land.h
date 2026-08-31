#ifndef _land_h_
#define _land_h_
#include "materialList.h"
#include "map.h"
#include "renderer.h"


/*
ID	MaterialHeight 		RangeHex 		ColorVisual 	PurposeDeep 
===================================================================================================
0	Water				.00 – 0.10		#0d233a		Deep ocean floor anchoring the map.
1	Shallow Water		0.10 – 0.15		#285973		Coastal shelf / riverbeds.
2	Sand / Shore		0.15 – 0.18		#d2b48c		Beaches and low-lying sediment transition.
3	Lush Grass			0.18 – 0.40		#4a703b		Main lowlands and valleys.
4	Forest / Moss		0.40 – 0.55		#2d4a22		Darker greens for higher alpine plateaus.
5	Dirt / Scree		0.55 – 0.68		#5a4d41		Tree-line transition where soil thins out.
6	Mountain Rock		0.68 – 0.85		#7a7a7a		Exposed steep cliff faces and crags.
7	Snow Cap			0.85 – 1.00		#f0f4f7		High altitude glacial peaks.
*/	
static constexpr u8	MATERIAL_ID__DEEP_WATER = 0;
static constexpr u8	MATERIAL_ID__SHALLOW_WATER = 1;
static constexpr u8	MATERIAL_ID__SAND = 2;
static constexpr u8	MATERIAL_ID__LUSH_GRASS = 3;
static constexpr u8	MATERIAL_ID__FOREST = 4;
static constexpr u8	MATERIAL_ID__DIRT = 5;
static constexpr u8	MATERIAL_ID__ROCK = 6;
static constexpr u8	MATERIAL_ID__SNOW = 7;

namespace land
{
	inline	Resol	resolution_next (Resol res)		{ u8 n=(u8)res; if (n<11) n++; return static_cast<Resol>(n); }
	inline	Resol	resolution_prev (Resol res)		{ u8 n=(u8)res; if (n>0)  n--; return static_cast<Resol>(n); }
	inline	u8		resolution_to_u8(Resol res)		{ return static_cast<u8>(res); }
	f32 	resolution_to_m (Resol res);
} //namespace land



#endif //_land_h_
