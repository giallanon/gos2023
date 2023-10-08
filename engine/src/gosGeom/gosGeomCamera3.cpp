#include "gosGeomCamera3.h"

using namespace gos;
using namespace gos::geom;


//*************************************************************
void Camera3::priv_copyFrom (const Camera3 &b)
{
	pos= b.pos;
	frustumWC=b.frustumWC; 
	ortoWidth = b.ortoWidth;
	ortoHeight = b.ortoHeight;
	aspectRatio = b.aspectRatio;
	matP = b.matP;
	nearDistance = b.nearDistance;
	farDistance = b.farDistance;
	fovy_rad = b.fovy_rad;
	bIsPerspective = b.bIsPerspective;

	lastTimeUpdated=0;
	lastTimeFrustumUpdated = lastTimeMatVPUpdated = lastTimeMatVUpdated = u32MAX;
}



//*************************************************************
void Camera3::setPerspectiveFovLH (f32 aspectIN, f32 fovY_radIN, f32 nearplane, f32 farplane)
{
	bIsPerspective = true;
	lastTimeUpdated=0;
	lastTimeFrustumUpdated = lastTimeMatVPUpdated = lastTimeMatVUpdated = u32MAX;

	//width = (f32)widthIN;
	//height = (f32)heightIN;
	aspectRatio = aspectIN;
	nearDistance = nearplane;
	farDistance = farplane;
	fovy_rad = fovY_radIN;


	const f32 tanHalfFov = tanf (fovy_rad * 0.5f);
	const f32 m00 = 1.0f / (aspectRatio * tanHalfFov);
	const f32 m11 = 1.0f / tanHalfFov;
	const f32 m22 = (-nearplane - farplane) / (nearplane - farplane);
	const f32 m23 = (2.0f*farplane*nearplane) / (nearplane - farplane);

	matP(0, 0) = m00;	matP(0, 1) = 0;		matP(0, 2) = 0;		matP(0, 3) = 0;
	matP(1, 0) = 0;		matP(1, 1) = m11;	matP(1, 2) = 0;		matP(1, 3) = 0;
	matP(2, 0) = 0;		matP(2, 1) = 0;		matP(2, 2) = m22;	matP(2, 3) = m23;
	matP(3, 0) = 0;		matP(3, 1) = 0;		matP(3, 2) = 1;		matP(3, 3) = 0;
}

//*************************************************************
void Camera3::setOrthoLH (f32 widthIN, f32 heightIN, f32 nearplane, f32 farplane, f32 zoom)
{
	DBGBREAK; // da rivedere dopo che ho cambiadato da row major a col major
	assert (zoom > 0);
	bIsPerspective = false;
	lastTimeUpdated=0;
	lastTimeFrustumUpdated = lastTimeMatVPUpdated = lastTimeMatVUpdated = u32MAX;

	fovy_rad = zoom;
	ortoWidth = widthIN;
	ortoHeight = heightIN;
	nearDistance = nearplane;
	farDistance = farplane;
	aspectRatio = ortoWidth / ortoHeight;


	/*This function uses the following formula to compute the returned matrix. 
		2/w  0    0           0
		0    2/h  0           0
		0    0    1/(zf-zn)   0
		0    0    zn/(zn-zf)  1
	*/
	const f32 zn = nearplane;
	const f32 zf = farplane;
	matP(0,0) = (zoom*2.0f) / ortoWidth;	matP(0,1) = 0;							matP(0,2) = 0;					matP(0,3) = 0;
	matP(1,0) = 0;							matP(1,1) = (zoom*2.0f) / ortoHeight;	matP(1,2) = 0;					matP(1,3) = 0;
	matP(2,0) = 0;							matP(2,1) = 0;							matP(2,2) = 1.0f / (zf-zn);		matP(2,3) = 0;
	matP(3,0) = 0;							matP(3,1) = 0;							matP(3,2) = zn / (zn-zf);		matP(3,3) = 1;
}

//*************************************************************
void Camera3::priv_calcFrustum()
{
	lastTimeFrustumUpdated = lastTimeUpdated;
	
	if (bIsPerspective)
		frustumWC.buildPerspective (pos.o, pos.getAsseZ(), pos.getAsseY(), pos.getAsseX(), nearDistance, farDistance, fovy_rad, aspectRatio);
	else
		frustumWC.buildOrtho (pos.o, pos.getAsseZ(), pos.getAsseY(), pos.getAsseX(), ortoWidth, ortoHeight, nearDistance, farDistance, getOrhtoZoom());
}

//*************************************************************
const mat4x4f& Camera3::getMatV ()
{
	if (lastTimeUpdated != lastTimeMatVUpdated)
	{
		lastTimeMatVUpdated = lastTimeUpdated;
		
		const vec3f ax = pos.getAsseX();
		const vec3f ay = pos.getAsseY();
		const vec3f az = pos.getAsseZ();

		matV.identity();
		matV(0, 0) = ax.x;		matV(0, 1) = ax.y;		matV(0, 2) = ax.z;
		matV(1, 0) = ay.x;		matV(1, 1) = ay.y;		matV(1, 2) = ay.z;
		matV(2, 0) = az.x;		matV(2, 1) = az.y;		matV(2, 2) = az.z;

		matV(0, 3) = -math::dot(pos.o, ax);
		matV(1, 3) = -math::dot(pos.o, ay);
		matV(2, 3) = -math::dot(pos.o, az);
	}
	return matV;
}

//*************************************************************
const mat4x4f& Camera3::getMatVP ()
{
	if (lastTimeUpdated != lastTimeMatVPUpdated)
	{
		lastTimeMatVPUpdated = lastTimeUpdated;
		//se devo aggiornare anche la matV
		if (lastTimeUpdated != lastTimeMatVUpdated)
			getMatV();
		
		matVP = matP * matV;
	}

	return matVP;
}

//*************************************************************
void Camera3::projectI (f32 viewportDimx, f32 viewportDimy, const vec3f *points3D, vec2i *point2D, u32 nPoints)
{
	getMatVP ();

	if (isPerspective())
	{
		const f32 w = viewportDimx;
		const f32 h = viewportDimy;
		for (u32 i=0; i<nPoints;i++)
		{
			vec3f v = math::vecTransform (matVP, points3D[i]);

			point2D[i].x = (i32)(((v.x +1) / 2)* w);
			point2D[i].y = (i32)((1.0f - ((v.y +1) / 2))* h);
		}
	}
	else
	{
		const f32 w2 = viewportDimx / 2;
		const i32 h = (i32)viewportDimy;
		const f32 h2 = (f32)viewportDimy / 2;

		for (u32 i=0; i<nPoints;i++)
		{
			vec3f v = math::vecTransform (matVP, points3D[i]);

			point2D[i].x =     (i32)(w2 + v.x * w2);
			point2D[i].y = h - (i32)(h2 + v.y * h2);
		}
	}
}

//*************************************************************
void Camera3::projectF (f32 viewportDimx, f32 viewportDimy, const vec3f *points3D, vec2f *point2D, u32 nPoints)
{
	getMatVP ();

	if (isPerspective())
	{
		const f32 w = viewportDimx;
		const f32 h = viewportDimy;
		for (u32 i=0; i<nPoints;i++)
		{
			vec3f v = math::vecTransform (matVP, points3D[i]);

			point2D[i].x = (((v.x +1) / 2)* w);
			point2D[i].y = ((1.0f - ((v.y +1) / 2))* h);
		}
	}
	else
	{
		const f32 w2 = viewportDimx / 2;
		const i32 h = (i32)viewportDimy;
		const f32 h2 = viewportDimy / 2;

		for (u32 i=0; i<nPoints;i++)
		{
			vec3f v = math::vecTransform (matVP, points3D[i]);

			point2D[i].x =     (w2 + v.x * w2);
			point2D[i].y = h - (h2 + v.y * h2);
		}
	}
}

//*************************************************************
void Camera3::unproject (f32 viewportDimx, f32 viewportDimy, const vec2f *points2D, vec3f *points3D, u32 nPoints)
{
	getFrustumWC();
	const vec3f	nearCenter = frustumWC.getNearCenter();
	const vec3f	nearHalfAsseX = frustumWC.getNearPlaneHalfAsseX();
	const vec3f	nearHalfAsseY = frustumWC.getNearPlaneHalfAsseY();
	const vec3f	camO = pos.o;

	f32 vpHalfDimx = viewportDimx * 0.5f;
	f32 vpHalfDimy = viewportDimy * 0.5f;
	for (u32 i=0; i<nPoints; i++)
	{
		/* screen to device coord
		 * in device coordinate,  l'angolo in alto a sx è definito come -1,1 mentre l'angolo in basso a dx è definito come 1,-1
		 */
		vec2f pointDevCoord (points2D[i].x / vpHalfDimx -1.0f, 1.0f - points2D[i].y / vpHalfDimy);
		
		if (!bIsPerspective)
			pointDevCoord *= 0.005f; //boh, numero magico venuto fuori da vari debuggamenti
 
		/* il punto in 3D giace sul near plane */
		vec3f p3D = nearCenter + nearHalfAsseX*pointDevCoord.x +nearHalfAsseY*pointDevCoord.y;

		/* quello che ritorno, è una direzione di un ipotetico ray che parte dall'origine della cam e interseca il punto 2D sul near plane */
		points3D[i] = p3D - camO;
		points3D[i].normalize();
	}
}