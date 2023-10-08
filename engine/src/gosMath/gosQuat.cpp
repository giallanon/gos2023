#include "gosQuat.h"
#include "gosMath.h"

using namespace gos;


//*******************************************************************
void Quat::buildFromEuler_YXZ (f32 rad_ay, f32 rad_ax, f32 rad_az)
{
	Quat qx, qy, qz;
	qx.buildRotationAboutAsseX(rad_ax);
	qy.buildRotationAboutAsseY(rad_ay);
	qz.buildRotationAboutAsseZ(rad_az);
	*this = qz*qx*qy;
	normalize();
}

//*******************************************************************
void Quat::toEuler (f32 *rad_ax, f32 *rad_ay, f32 *rad_az) const
{
	f32 test = x*y + z*w;
	if (test > 0.499f)
	{ 
		// singularity at north pole
		(*rad_ay) = 2.0f * atan2f(x,w);
		(*rad_az) = math::PIMEZZI;
		(*rad_ax) = 0;
		return;
	}
	if (test < -0.499f)
	{ 
		// singularity at south pole
		(*rad_ay) = -2.0f * atan2f(x,w);
		(*rad_az) = -math::PIMEZZI;
		(*rad_ax) = 0;
		return;
	}
    
	f32 sqx = 2.0f *x *x;
    f32 sqy = 2.0f *y *y;
    f32 sqz = 2.0f *z *z;
    (*rad_ay) = atan2f (2.0f *y *w - 2.0f *x *z , 1.0f - sqy - sqz);
	(*rad_az) = asinf (2.0f *test);
	(*rad_ax) = atan2f (2.0f *x *w - 2.0f *y *z , 1.0f - sqx - sqz);
}

//*******************************************************************
void Quat::buildFromMatrix3x3(const mat3x3f &m)
{
	f32 trace = m(0,0) + m(1,1) + m(2,2);
	if( trace > 0 )
	{
		f32 s = 0.5f / sqrtf(trace+ 1.0f);
		w = 0.25f / s;
		x = -( m(2,1) - m(1,2) ) * s;
		y = -( m(0,2) - m(2,0) ) * s;
		z = -( m(1,0) - m(0,1) ) * s;
	} 
	else 
	{
		if ( m(0,0) > m(1,1) && m(0,0) > m(2,2) ) 
		{
			f32 s = 2.0f * sqrtf( 1.0f + m(0,0) - m(1,1) - m(2,2));
			w = -(m(2,1) - m(1,2) ) / s;
			x = 0.25f * s;
			y = (m(0,1) + m(1,0) ) / s;
			z = (m(0,2) + m(2,0) ) / s;
		} 
		else if (m(1,1) > m(2,2)) 
		{
			f32 s = 2.0f * sqrtf( 1.0f + m(1,1) - m(0,0) - m(2,2));
			w = -(m(0,2) - m(2,0) ) / s;
			x =  (m(0,1) + m(1,0) ) / s;
			y = 0.25f * s;
			z =  (m(1,2) + m(2,1) ) / s;
		} 
		else 
		{
			f32 s = 2.0f * sqrtf( 1.0f + m(2,2) - m(0,0) - m(1,1) );
			w = -(m(1,0) - m(0,1) ) / s;
			x =  (m(0,2) + m(2,0) ) / s;
			y =  (m(1,2) + m(2,1) ) / s;
			z = 0.25f * s;
		}
	}

	normalize();
}




//*******************************************************************
void Quat::buildFromXYZAxes (const vec3f &xaxis, const vec3f &yaxis, const vec3f &zaxis)
{
    mat3x3f	kRot;
	kRot.buildFromXYZAxes (xaxis, yaxis, zaxis);
    buildFromMatrix3x3 (kRot);

}

//*******************************************************************
void Quat::toMatrix3x3 (mat3x3f *out_m) const
{
    const f32 fTx  = 2.0f*x;
    const f32 fTy  = 2.0f*y;
    const f32 fTz  = 2.0f*z;
    const f32 fTwx = fTx*w;
    const f32 fTwy = fTy*w;
    const f32 fTwz = fTz*w;
    const f32 fTxx = fTx*x;
    const f32 fTxy = fTy*x;
    const f32 fTxz = fTz*x;
    const f32 fTyy = fTy*y;
    const f32 fTyz = fTz*y;
    const f32 fTzz = fTz*z;

    (*out_m)(0,0) = 1.0f-(fTyy+fTzz);
    (*out_m)(1,0) = fTxy-fTwz;
    (*out_m)(2,0) = fTxz+fTwy;
    (*out_m)(0,1) = fTxy+fTwz;
    (*out_m)(1,1) = 1.0f-(fTxx+fTzz);
    (*out_m)(2,1) = fTyz-fTwx;
    (*out_m)(0,2) = fTxz-fTwy;
    (*out_m)(1,2) = fTyz+fTwx;
    (*out_m)(2,2) = 1.0f-(fTxx+fTyy);

}

//*******************************************************************
void Quat::toAxis (vec3f *out_ax, vec3f *out_ay, vec3f *out_az) const
{
	mat3x3f	m;
	toMatrix3x3 (&m);

	out_ax->x = m(0,0);		out_ay->x=m(0,1);		out_az->x=m(0,2);
	out_ax->y = m(1,0);		out_ay->y=m(1,1);		out_az->y=m(1,2);
	out_ax->z = m(2,0);		out_ay->z=m(2,1);		out_az->z=m(2,2);
}

//*******************************************************************
void Quat::inverse()
{
	x=-x;
	y=-y;
	z=-z;
}

//*******************************************************************
void Quat::normalize()
{
	f32	f = 1.0f / sqrtf(w*w+x*x+y*y+z*z);
	x*=f;
	y*=f;
	z*=f;
	w*=f;
}

//*******************************************************************
vec3f	Quat::vec3Transform (const vec3f &v) const
{
	// nVidia SDK implementation
	vec3f uv, uuv;
	vec3f qvec(x, y, z);
	uv = math::cross (qvec, v);
	uuv = math::cross (qvec, uv);
	uv *= (2.0f * w);
	uuv *= 2.0f;

	return v + uv + uuv;
}

//*******************************************************************
void Quat::buildRotationAboutAsseX (f32 angle_rad)
{
	angle_rad *= 0.5f;
	x = sinf(angle_rad);
	y = 0;
	z = 0;
	w = cosf (angle_rad);
}

//*******************************************************************
void Quat::buildRotationAboutAsseY (f32 angle_rad)
{
	angle_rad *= 0.5f;
	x = 0;
	y = sinf(angle_rad);
	z = 0;
	w = cosf (angle_rad);
}

//*******************************************************************
void Quat::buildRotationAboutAsseZ (f32 angle_rad)
{
	angle_rad *= 0.5f;
	x = 0;
	y = 0;
	z = sinf(angle_rad);
	w = cosf (angle_rad);
}

//*******************************************************************
void Quat::buildRotation (const vec3f &ax, f32 angle_rad)
{
	angle_rad *= 0.5f;
	const f32 sina = sinf(angle_rad);
	x = ax.x * sina;
	y = ax.y * sina;
	z = ax.z * sina;
	w = cosf (angle_rad);
}

//*******************************************************************
void Quat::rotateMeAbout (const vec3f &ax, f32 angle_rad)
{
	Quat	local;
	local.buildRotation (ax, angle_rad);

	(*this) = (*this) * local;
}
