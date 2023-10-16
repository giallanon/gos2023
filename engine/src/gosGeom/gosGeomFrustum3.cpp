#include "gosGeomFrustum3.h"

using namespace gos;
using namespace gos::geom;


/************************************************************************
 * I piani sono definiti in modo da avere le normali che puntano all'interno 
   del frustum*/
void Frustum3::buildPerspective (const vec3f &o, const vec3f &asseZ, const vec3f &asseY, const vec3f &asseX, f32 nearDistance, f32 farDistanceIN, f32 fovY_rad, f32 aspect)
{
	farDistance = farDistanceIN;
	const f32 tanFOV = (f32)tanf (fovY_rad*0.5f);
	f32		f;
	vec3f	halfDimX;
	vec3f	halfDimY;

	//calcolo dei punti del nearplane
	vec3f	nearCenter = o + nearDistance*asseZ;
	f = nearDistance * tanFOV;
	nearPlaneHalfAsseY = halfDimY = f*asseY;
	nearPlaneHalfAsseX = halfDimX = (f*aspect)* asseX;
	vec3f	p4near[4] = {	nearCenter +halfDimY -halfDimX,
							nearCenter +halfDimY +halfDimX,
							nearCenter -halfDimY +halfDimX,
							nearCenter -halfDimY -halfDimX};

	//calcolo dei punti del nearplane
	vec3f	farCenter = o + farDistance*asseZ;
	f = farDistance * tanFOV;
	farPlaneHalfAsseY = halfDimY = f*asseY;
	farPlaneHalfAsseX = halfDimX = (f*aspect)* asseX;
	vec3f	p4far[4]  = {	farCenter +halfDimY -halfDimX,
							farCenter +halfDimY +halfDimX,
							farCenter -halfDimY +halfDimX,
							farCenter -halfDimY -halfDimX};

	//calcolo i piani
	planes[(u8)ePlane::front].setFromPointAndNormal (nearCenter, asseZ);
	planes[(u8)ePlane::back].setFromPointAndNormal (farCenter, -asseZ);
	planes[(u8)ePlane::left].setFrom3Points (p4near[0], p4far[0], p4far[3]);
	planes[(u8)ePlane::right].setFrom3Points (p4far[2], p4far[1], p4near[1]);
	planes[(u8)ePlane::top].setFrom3Points (p4near[0], p4near[1], p4far[1]);
	planes[(u8)ePlane::bottom].setFrom3Points (p4near[2], p4near[3], p4far[3]);

	/*nearPlaneHalfAsseX = (p4near[1] - p4near[0]) *0.5f;
	nearPlaneHalfAsseY = (p4near[1] - p4near[2]) *0.5f;
	farPlaneHalfAsseX = (p4far[1] - p4far[0]) *0.5f;
	farPlaneHalfAsseY = (p4far[1] - p4far[2]) *0.5f;
	*/
}

/************************************************************************
 * I piani sono definiti in modo da avere le normali che puntano all'interno 
   del frustum*/
void Frustum3::buildOrtho (const vec3f &o, const vec3f &asseZ, const vec3f &asseY, const vec3f &asseX, f32 width, f32 height, f32 nearDistance, f32 farDistanceIN, f32 zoom)
{
	farDistance = farDistanceIN;
	assert (zoom > 0);
	width/=zoom;
	height/=zoom;

	vec3f	nearCenter = o + nearDistance*asseZ;
	planes[(u8)ePlane::front].setFromPointAndNormal (nearCenter, asseZ);
	
	vec3f	farCenter  = o + farDistance*asseZ;
	planes[(u8)ePlane::back].setFromPointAndNormal (farCenter, -asseZ);

	farPlaneHalfAsseX = nearPlaneHalfAsseX = (width * 0.5f) * asseX;
	farPlaneHalfAsseY = nearPlaneHalfAsseY = (height * 0.5f) * asseY;
	const vec3f  p = nearCenter + ((farDistance * 0.5f) * asseZ);

	planes[(u8)ePlane::left].setFromPointAndNormal		(p -farPlaneHalfAsseX,  asseX);
	planes[(u8)ePlane::right].setFromPointAndNormal	(p +farPlaneHalfAsseX, -asseX);
	planes[(u8)ePlane::top].setFromPointAndNormal		(p +farPlaneHalfAsseY, -asseY);
	planes[(u8)ePlane::bottom].setFromPointAndNormal	(p -farPlaneHalfAsseY,  asseY);

	
}

//************************************************************************
void Frustum3::calc8Points (vec3f *vtx) const
{
	vec3f	nearCenter = getNearCenter();
	vec3f	farCenter = getFarCenter();
	vec3f	farAx = getFarPlaneHalfAsseX ();
	vec3f	farAy = getFarPlaneHalfAsseY();
	vec3f	nearAx = getNearPlaneHalfAsseX();
	vec3f	nearAy = getNearPlaneHalfAsseY();

	vtx[0] = nearCenter - nearAx +nearAy;
	vtx[1] = nearCenter + nearAx +nearAy;
	vtx[2] = nearCenter + nearAx -nearAy;
	vtx[3] = nearCenter - nearAx -nearAy;
	vtx[4] = farCenter - farAx +farAy;
	vtx[5] = farCenter + farAx +farAy;
	vtx[6] = farCenter + farAx -farAy;
	vtx[7] = farCenter - farAx -farAy;
}

//************************************************************************
void Frustum3::calcAABB (AABB3 &out) const
{
	vec3f pt[8];
	calc8Points (pt);

	out.vmin = out.vmax = pt[0];
	for (u8 i=1; i<8; i++)
	{
		if (pt[i].x < out.vmin.x)	out.vmin.x = pt[i].x;
		if (pt[i].y < out.vmin.y)	out.vmin.y = pt[i].y;
		if (pt[i].z < out.vmin.z)	out.vmin.z = pt[i].z;
		if (pt[i].x > out.vmax.x)	out.vmax.x = pt[i].x;
		if (pt[i].y > out.vmax.y)	out.vmax.y = pt[i].y;
		if (pt[i].z > out.vmax.z)	out.vmax.z = pt[i].z;
	}
}