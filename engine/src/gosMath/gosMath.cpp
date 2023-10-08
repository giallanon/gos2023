#include "gosMath.h"

using namespace gos;

//*******************************************************************
Quat math::slerp (const Quat &q1, const Quat &q2, f32 k, bool shortestPath)
{
    f32 fCos = math::dot(q1, q2);
    Quat rkT;

    // Do we need to invert rotation?
    if (fCos < 0.0f && shortestPath)
    {
        fCos = -fCos;
        rkT = -q2;
    }
    else
    {
        rkT = q2;
    }

	const float ms_fEpsilon = 1e-03f;
    if (fabsf(fCos) < 1.0 - ms_fEpsilon)
    {
        // Standard case (slerp)
        f32 fSin = sqrtf(1.0f - fCos*fCos);
        f32 fAngle = atan2f(fSin, fCos);
        f32 fInvSin = 1.0f / fSin;
        f32 fCoeff0 = sinf((1.0f - k) * fAngle) * fInvSin;
        f32 fCoeff1 = sinf(k * fAngle) * fInvSin;
        return fCoeff0 * q1 + fCoeff1 * rkT;
    }
    else
    {
        // There are two situations:
        // 1. "q1" and "q2" are very close (fCos ~= +1), so we can do a linear
        //    interpolation safely.
        // 2. "q1" and "q2" are almost inverse of each other (fCos ~= -1), there
        //    are an infinite number of possibilities interpolation. but we haven't
        //    have method to fix this case, so just use linear interpolation here.
        Quat t = (1.0f - k) * q1 + k * rkT;
        // taking the complement requires renormalisation
        t.normalize();
        return t;
    }
}




//*******************************************************************
void math::lerp (const mat4x4f &a, const mat4x4f &b, f32 t01, mat4x4f *out)
{
	const f32 *pa = a._getValuesPtConst();
	const f32 *pb = b._getValuesPtConst();
	f32 *p = out->_getValuesPt();
	for (u8 i=0; i<16; i++)
		p[i] = pa[i] + (pb[i] - pa[i]) * t01;
}

//*******************************************************************
void math::lerpArray (const mat4x4f *a, const mat4x4f *b, mat4x4f *out, f32 t01, u32 nMatrix)
{
	for (u32 n = 0; n < nMatrix; n++)
		math::lerp (a[n], b[n], t01, &out[n]);
}

//*******************************************************************
void math::lerp (const mat3x4f &a, const mat3x4f &b, f32 t01, mat3x4f *out)
{
	const f32 *pa = a._getValuesPtConst();
	const f32 *pb = b._getValuesPtConst();
	f32 *p = out->_getValuesPt();
	for (u8 i=0; i<12; i++)
		p[i] = pa[i] + (pb[i] - pa[i]) * t01;
}

//*******************************************************************
void math::lerpArray (const mat3x4f *a, const mat3x4f *b, mat3x4f *out, f32 t01, u32 nMatrix)
{
	for (u32 n=0; n<nMatrix; n++)
	    math::lerp (a[n], b[n], t01, &out[n]);
}

//*******************************************************************
vec3f math::vecTransform (const Quat &q, const vec3f &vIN)
{
	Quat v (vIN.x, vIN.y, vIN.z, 0);
	Quat qInv (-q.x, -q.y, -q.z, q.w);
	Quat ret = ((qInv * v) * q);

	return vec3f (ret.x, ret.y, ret.z);	
}

