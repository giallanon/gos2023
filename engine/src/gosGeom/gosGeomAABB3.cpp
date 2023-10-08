/*****************************************************************************
*
*              p[7]                p[6]
*              @-------------------@
*             /|                  /|                 
*            /                   / |                
*           /  |           p[5] /  |               
*       p[4]@------------------@   |
*           |                  |   |
*           |  |               |   |
*           |  @ - - - - - - - | - @ p[2]
*           | / p[3]           |  /
*           |                  | /
*           @------------------@/
*          min                p[1]
*         (p[0])
*
*
*****************************************************************************/
#include "gosGeomAABB3.h"
#include "../gosMath/gosMath.h"

using namespace gos;
using namespace gos::geom;


//*************************** 
AABB3& AABB3::operator+=(const AABB3 &b)
{
	if (b.vmin.x < vmin.x)	vmin.x=b.vmin.x;
	if (b.vmin.y < vmin.y)	vmin.y=b.vmin.y;
	if (b.vmin.z < vmin.z)	vmin.z=b.vmin.z;

	if (b.vmax.x > vmax.x)	vmax.x=b.vmax.x;
	if (b.vmax.y > vmax.y)	vmax.y=b.vmax.y;
	if (b.vmax.z > vmax.z)	vmax.z=b.vmax.z;
	return *this;
}

//*************************** 
void AABB3::get8Point (vec3f *out) const
{
	out[0].set (vmin.x, vmin.y, vmin.z);
	out[1].set (vmax.x, vmin.y, vmin.z);
	out[2].set (vmax.x, vmin.y, vmax.z);
	out[3].set (vmin.x, vmin.y, vmax.z);

	out[4].set (vmin.x, vmax.y, vmin.z);
	out[5].set (vmax.x, vmax.y, vmin.z);
	out[6].set (vmax.x, vmax.y, vmax.z);
	out[7].set (vmin.x, vmax.y, vmax.z);
}

//*******************************************************************
void AABB3::quatTransform (const AABB3 &aabb, const Quat &b, AABB3 *out)
{
	vec3f	p8[8];
	aabb.get8Point (p8);
	for (u8 i=0; i<8; i++)
		p8[i] = math::vecTransform (b, p8[i]);

	AABB3 ret (p8[0], p8[0]);
	for (u8 i=1; i<8; i++)
	{
		if (p8[i].x < ret.vmin.x)	ret.vmin.x = p8[i].x;
		if (p8[i].x > ret.vmax.x)	ret.vmax.x = p8[i].x;
		if (p8[i].y < ret.vmin.y)	ret.vmin.y = p8[i].y;
		if (p8[i].y > ret.vmax.y)	ret.vmax.y = p8[i].y;
		if (p8[i].z < ret.vmin.z)	ret.vmin.z = p8[i].z;
		if (p8[i].z > ret.vmax.z)	ret.vmax.z = p8[i].z;
	}

	*out = ret;
}

//*******************************************************************
void AABB3::matrixTransform (const AABB3 &aabb, const mat4x4f &b, AABB3 *out)
{
	vec3f	p8[8];
	aabb.get8Point (p8);
	for (u8 i=0; i<8; i++)
		p8[i] = math::vecTransform (b, p8[i]);

	AABB3 ret (p8[0], p8[0]);
	for (u8 i=1; i<8; i++)
	{
		if (p8[i].x < ret.vmin.x)	ret.vmin.x = p8[i].x;
		if (p8[i].x > ret.vmax.x)	ret.vmax.x = p8[i].x;
		if (p8[i].y < ret.vmin.y)	ret.vmin.y = p8[i].y;
		if (p8[i].y > ret.vmax.y)	ret.vmax.y = p8[i].y;
		if (p8[i].z < ret.vmin.z)	ret.vmin.z = p8[i].z;
		if (p8[i].z > ret.vmax.z)	ret.vmax.z = p8[i].z;
	}

	*out = ret;
}

//*******************************************************************
void AABB3::matrixTransform (const AABB3 &aabb, const mat3x4f &b, AABB3 *out)
{
	vec3f	p8[8];
	aabb.get8Point (p8);
	for (u8 i=0; i<8; i++)
		p8[i] = math::vecTransform (b, p8[i]);

	AABB3 ret (p8[0], p8[0]);
	for (u8 i=1; i<8; i++)
	{
		if (p8[i].x < ret.vmin.x)	ret.vmin.x = p8[i].x;
		if (p8[i].x > ret.vmax.x)	ret.vmax.x = p8[i].x;
		if (p8[i].y < ret.vmin.y)	ret.vmin.y = p8[i].y;
		if (p8[i].y > ret.vmax.y)	ret.vmax.y = p8[i].y;
		if (p8[i].z < ret.vmin.z)	ret.vmin.z = p8[i].z;
		if (p8[i].z > ret.vmax.z)	ret.vmax.z = p8[i].z;
	}

	*out = ret;
}