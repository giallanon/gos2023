#include "gosGeomPos3.h""
#include "../gosMath/gosMath.h"

using namespace gos;
using namespace gos::geom;


//***************************************************
void Pos3::setFromMatrix4x4 (const mat4x4f &m)
{
	rot(0, 0) = m(0, 0);	rot(0, 1) = m(0, 1);	rot(0, 2) = m(0, 2);
	rot(1, 0) = m(1, 0);	rot(1, 1) = m(1, 1);	rot(1, 2) = m(1, 2);
	rot(2, 0) = m(2, 0);	rot(2, 1) = m(2, 1);	rot(2, 2) = m(2, 2);

	o.set (m(0, 3), m(1, 3), m(2, 3));
}

//***************************************************
void Pos3::setFromMatrix3x4 (const mat3x4f &m)
{
	rot(0, 0) = m(0, 0);	rot(0, 1) = m(0, 1);	rot(0, 2) = m(0, 2);
	rot(1, 0) = m(1, 0);	rot(1, 1) = m(1, 1);	rot(1, 2) = m(1, 2);
	rot(2, 0) = m(2, 0);	rot(2, 1) = m(2, 1);	rot(2, 2) = m(2, 2);

	o.set (m(0, 3), m(1, 3), m(2, 3));
}


/***************************************************
 * aa  = gos::math::math::cross (ay, az);//ax
 * aa  = gos::math::math::cross (az, ax);//ay
 * aa  = gos::math::math::cross (ax, ay);//az*/
void Pos3::alignAsseX(const vec3f &align)
{
	vec3f	xaxis = align;
	xaxis.normalize();

	const vec3f az = getAsseZ();

	vec3f	zaxis, yaxis;
	if (fabsf(math::dot (xaxis, az)) > 0.8f)
	{
		const vec3f ay = getAsseY();
		zaxis = math::normalize (math::cross (xaxis, ay));
		yaxis = math::normalize (math::cross (zaxis, xaxis));
	}
	else
	{
		yaxis = math::normalize (math::cross (az, xaxis));
		zaxis = math::normalize (math::cross (xaxis, yaxis));
	}

	rot.buildFromXYZAxes (xaxis, yaxis, zaxis);
}

/***************************************************
 * aa  = gos::math::math::cross (ay, az);//ax
 * aa  = gos::math::math::cross (az, ax);//ay
 * aa  = gos::math::math::cross (ax, ay);//az*/
void Pos3::alignAsseY (const vec3f &align)
{
	vec3f	yaxis = align;
	yaxis.normalize();

	const vec3f az = getAsseZ();

	vec3f	xaxis, zaxis;
	if (fabsf(math::dot (yaxis, az)) > 0.8f)
	{
		const vec3f ax = getAsseX();
		zaxis = math::normalize (math::cross (ax, yaxis));
		xaxis = math::normalize (math::cross (yaxis, zaxis));
	}
	else
	{
		xaxis = math::normalize (math::cross (yaxis, az));
		zaxis = math::normalize (math::cross (xaxis, yaxis));
	}

	rot.buildFromXYZAxes (xaxis, yaxis, zaxis);
}

//***************************************************
void Pos3::alignAsseZ(const vec3f &align)
{
	vec3f	zaxis = align;
	zaxis.normalize();

	vec3f ay (0, 1, 0);
	//getAsseY(&ay);

	vec3f	xaxis, yaxis;
	const f32 d = math::dot (zaxis, ay);
	if (fabsf(d) > 0.8f)
	{
		vec3f ax (1, 0, 0);
		//getAsseX(&ax);
		yaxis = math::normalize (math::cross (zaxis, ax));
		xaxis = math::normalize (math::cross (yaxis, zaxis));
	}
	else
	{
		xaxis = math::normalize (math::cross (ay, zaxis));
		yaxis = math::normalize (math::cross (zaxis, xaxis));
	}

	rot.buildFromXYZAxes (xaxis, yaxis, zaxis);
}

//***************************************************
void Pos3::vect_ToLocal (const vec3f *vIn, vec3f *vOut, u32 n) const
{
	for (u32 i=0; i<n; i++)
	{
		vOut[i].x = vIn[i].x * rot(0,0) + vIn[i].y * rot(1,0) + vIn[i].z * rot(2,0);
		vOut[i].y = vIn[i].x * rot(0,1) + vIn[i].y * rot(1,1) + vIn[i].z * rot(2,1);
		vOut[i].z = vIn[i].x * rot(0,2) + vIn[i].y * rot(1,2) + vIn[i].z * rot(2,2);
	}
}

//***************************************************
void Pos3::vect_ToWorld (const vec3f *vIn, vec3f *vOut, u32 n) const
{
	for (u32 i = 0; i < n; i++)
		vOut[i] = math::vecTransform (this->rot, vIn[i]);
}

//***************************************************
void Pos3::point_ToLocal (const vec3f *vIn, vec3f *vOut, u32 n) const
{
	for (u32 i=0; i<n; i++)
	{
		vec3f	v = vIn[i] - o;
		vect_ToLocal (&v, vOut, 1);
	}
}

//***************************************************
void Pos3::point_ToWorld (const vec3f *vIn, vec3f *vOut, u32 n) const
{
	vect_ToWorld (vIn, vOut, n);
	for (u32 i=0; i<n; i++)
		vOut[i] += o;
}

//***************************************************
void Pos3::getMatrix3x4 (mat3x4f *out) const
{
	(*out)(0, 0) = rot(0, 0);	(*out)(0, 1) = rot(0, 1);	(*out)(0, 2) = rot(0, 2);	(*out)(0, 3) = o.x;
	(*out)(1, 0) = rot(1, 0);	(*out)(1, 1) = rot(1, 1);	(*out)(1, 2) = rot(1, 2);	(*out)(1, 3) = o.y;
	(*out)(2, 0) = rot(2, 0);	(*out)(2, 1) = rot(2, 1);	(*out)(2, 2) = rot(2, 2);	(*out)(2, 3) = o.z;
}

//***************************************************
void Pos3::getMatrix4x4 (mat4x4f *out) const
{
	(*out)(0, 0) = rot(0, 0);	(*out)(0, 1) = rot(0, 1);	(*out)(0, 2) = rot(0, 2);	(*out)(0, 3) = o.x;
	(*out)(1, 0) = rot(1, 0);	(*out)(1, 1) = rot(1, 1);	(*out)(1, 2) = rot(1, 2);	(*out)(1, 3) = o.y;
	(*out)(2, 0) = rot(2, 0);	(*out)(2, 1) = rot(2, 1);	(*out)(2, 2) = rot(2, 2);	(*out)(2, 3) = o.z;
	(*out)(3, 0) = 0;			(*out)(3, 1) = 0;			(*out)(3, 2) = 0;			(*out)(3, 3) = 1;
}

//***************************************************
void Pos3::normalizeRotMatrix()
{
	vec3f ax = getAsseX();
	ax.normalize();

	vec3f ay = getAsseY();
	ay.normalize();

	vec3f az = math::normalize (math::cross (ax, ay));

	rot.buildFromXYZAxes (ax, ay, az);
}	