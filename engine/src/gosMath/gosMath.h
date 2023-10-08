#ifndef _gosMath_h_
#define _gosMath_h_
#include "gosMathEnumAndDefine.h"
#include "gosVect.h"
#include "gosMatrix.h"
#include "gosQuat.h"

namespace gos
{
	 /*=========================================================
	 * 
	 * quaternion math
	 *
	 *=========================================================*/
	inline Quat			operator+ (const Quat &a, const Quat &b)					{return Quat(a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w);}
	inline Quat			operator- (const Quat &a, const Quat &b)					{return Quat(a.x-b.x, a.y-b.y, a.z-b.z, a.w-b.w);}
	inline Quat			operator* (const Quat &a, const Quat &b)					{return Quat ( a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y, a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x, a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w, a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z);}
	inline Quat			operator* (const Quat &a, f32 k)							{return Quat(a.x*k, a.y*k, a.z*k, a.w*k);}
	inline Quat			operator* (f32 k, const Quat &a)							{return Quat(a.x*k, a.y*k, a.z*k, a.w*k);}

	namespace math
	{
		/*=========================================================
		 * 
		 * generic
		 *
		 *=========================================================*/
		inline f32			floor(f32 a)												{ return floorf(a);  }
		inline f32			round(f32 a)												{ return roundf(a);  }
		inline f32			trunc(f32 a)												{ return truncf(a); }

	
		inline f32			dot (const Quat &q1, const Quat &q2)					{return (q1.x*q2.x + q1.y*q2.y + q1.z*q2.z + q1.w*q2.w);}
		Quat				slerp (const Quat &q1, const Quat &q2, f32 k, bool shortestPath=true);

		/*=========================================================
		 * 
		 * vector math
		 *
		 *=========================================================*/
								template <class T>
		Vec2<T>					operator- (const Vec2<T> &a)							{return Vec2<T> (-a.x, -a.y);}
								template <class T>
		Vec3<T>					operator- (const Vec3<T> &a)							{return Vec3<T> (-a.x, -a.y, -a.z);}
								template <class T>
		Vec4<T>					operator- (const Vec4<T> &a)							{return Vec4<T> (-a.x, -a.y, -a.z, -a.w);}

								template <class T>
		Vec2<T>					operator+ (const Vec2<T> &a, const Vec2<T> &b)		{return Vec2<T> (a.x+b.x, a.y+b.y);}
								template <class T>
		Vec3<T>					operator+ (const Vec3<T> &a, const Vec3<T> &b)		{return Vec3<T> (a.x+b.x, a.y+b.y, a.z+b.z);}
								template <class T>
		Vec4<T>					operator+ (const Vec4<T> &a, const Vec4<T> &b)		{return Vec4<T> (a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w);}

								template <class T>
		Vec2<T>					operator- (const Vec2<T> &a, const Vec2<T> &b)		{return Vec2<T> (a.x-b.x, a.y-b.y);}
								template <class T>
		Vec3<T>					operator- (const Vec3<T> &a, const Vec3<T> &b)		{return Vec3<T> (a.x-b.x, a.y-b.y, a.z-b.z);}
								template <class T>
		Vec4<T>					operator- (const Vec4<T> &a, const Vec4<T> &b)		{return Vec4<T> (a.x-b.x, a.y-b.y, a.z-b.z, a.w-b.w);}

								template <class T>
		Vec2<T>					operator* (const Vec2<T> &a, const T &b)				{return Vec2<T> (a.x*b, a.y*b);}
								template <class T>
		Vec2<T>					operator* (const T &b, const Vec2<T> &a)				{return Vec2<T> (a.x*b, a.y*b);}
								template <class T>
		Vec2<T>					operator* (const Vec2<T> &a, const Vec2<T> &b)		{return Vec2<T> (a.x*b.x, a.y*b.y);}

								template <class T>
		Vec3<T>					operator* (const Vec3<T> &a, const T &b)				{return Vec3<T> (a.x*b, a.y*b, a.z*b);}
								template <class T>
		Vec3<T>					operator* (const T &b, const Vec3<T> &a)				{return Vec3<T> (a.x*b, a.y*b, a.z*b);}
								template <class T>
		Vec3<T>					operator* (const Vec3<T> &a, const Vec3<T> &b)		{return Vec3<T> (a.x*b.x, a.y*b.y, a.z*b.z);}

								template <class T>
		Vec4<T>					operator* (const Vec4<T> &a, const T &b)				{return Vec4<T> (a.x*b, a.y*b, a.z*b, a.w*b);}
								template <class T>
		Vec4<T>					operator* ( const T &b, const Vec4<T> &a)				{return Vec4<T> (a.x*b, a.y*b, a.z*b, a.w*b);}
								template <class T>
		Vec4<T>					operator* (const Vec4<T> &a, const Vec4<T> &b)		{return Vec4<T> (a.x*b.x, a.y*b.y, a.z*b.z, a.w*b.w);}

								template <class T>
		Vec2<T>					operator/ (const Vec2<T> &a, const T &b)				{assert(b!=0);	return Vec2<T> (a.x/b, a.y/b);}
								template <class T>
		Vec3<T>					operator/ (const Vec3<T> &a, const T &b)				{assert(b!=0);	return Vec3<T> (a.x/b, a.y/b, a.z/b);}
								template <class T>
		Vec4<T>					operator/ (const Vec4<T> &a, const T &b)				{assert(b!=0);	return Vec4<T> (a.x/b, a.y/b, a.z/b, a.w/b);}

								template<class T>
		T						dot (const Vec2<T> &a, const Vec2<T> &b)			{return (a.x*b.x + a.y*b.y);}
								template<class T>
		T						dot (const Vec3<T> &a, const Vec3<T> &b)			{return (a.x*b.x + a.y*b.y + a.z*b.z);}
								template<class T>
		T						dot (const Vec4<T> &a, const Vec4<T> &b)			{return (a.x*b.x + a.y*b.y + a.z*b.z +a.w*b.w);}

								template<class T>
		Vec3<T>					cross (const Vec3<T> &a, const Vec3<T> &b)			{return Vec3<T> (a.y*b.z - a.z*b.y, -a.x*b.z + a.z*b.x, a.x*b.y - a.y*b.x);	}

								template<class T>
		Vec3<T>					normalize (const Vec3<T> &a)							{T len = a.length(); return Vec3<T>(a.x/len, a.y/len, a.z/len);}

								template<class T>
		void					normalizeArray (Vec3<T> *a, u32 n)				
								{
									for (u32 i=0; i<n; i++)
									{
										T len = a[i].length(); 
										a[i] /= len;
									}
								}

								template<class T>
		Vec3<T>					lerp (const Vec3<T> &a, const Vec3<T> &b, f32 t01)
								{
									return Vec3<T> (a.x + (b.x - a.x) * t01, a.y + (b.y - a.y) * t01, a.z + (b.z - a.z) * t01);
								}

		/*=========================================================
		 * 
		 * matrix math
		 *
		 *=========================================================*/
									template<class T,bool isColMajor>
		Matrix<T,isColMajor,2,2>	operator* (const Matrix<T,isColMajor,2,2> &a, const Matrix<T,isColMajor,2,2> &b)
									{
										Matrix<T,isColMajor,2,2>	ret;
										ret(0,0) = a(0,0)*b(0,0) + a(0,1)*b(1,0);
										ret(0,1) = a(0,0)*b(0,1) + a(0,1)*b(1,1);
										ret(1,0) = a(1,0)*b(0,0) + a(1,1)*b(1,0);
										ret(1,1) = a(1,0)*b(0,1) + a(1,1)*b(1,1);
										return ret;
									}

									template<class T,bool isColMajor>
		Matrix<T,isColMajor,3,3>	operator* (const Matrix<T,isColMajor,3,3> &a, const Matrix<T,isColMajor,3,3> &b)
									{
										Matrix<T,isColMajor,3,3>	ret;
										ret(0,0) = a(0,0)*b(0,0) + a(0,1)*b(1,0) + a(0,2)*b(2,0);
										ret(0,1) = a(0,0)*b(0,1) + a(0,1)*b(1,1) + a(0,2)*b(2,1);
										ret(0,2) = a(0,0)*b(0,2) + a(0,1)*b(1,2) + a(0,2)*b(2,2);

										ret(1,0) = a(1,0)*b(0,0) + a(1,1)*b(1,0) + a(1,2)*b(2,0);
										ret(1,1) = a(1,0)*b(0,1) + a(1,1)*b(1,1) + a(1,2)*b(2,1);
										ret(1,2) = a(1,0)*b(0,2) + a(1,1)*b(1,2) + a(1,2)*b(2,2);

										ret(2,0) = a(2,0)*b(0,0) + a(2,1)*b(1,0) + a(2,2)*b(2,0);
										ret(2,1) = a(2,0)*b(0,1) + a(2,1)*b(1,1) + a(2,2)*b(2,1);
										ret(2,2) = a(2,0)*b(0,2) + a(2,1)*b(1,2) + a(2,2)*b(2,2);

										return ret;
									}

									template<class T,bool isColMajor>
		Matrix<T,isColMajor,4,4>	operator* (const Matrix<T,isColMajor,4,4> &a, const Matrix<T,isColMajor,4,4> &b)
									{
										Matrix<T,isColMajor,4,4>	ret;
										ret(0,0) = a(0,0)*b(0,0) + a(0,1)*b(1,0) + a(0,2)*b(2,0) +a(0,3)*b(3,0);
										ret(0,1) = a(0,0)*b(0,1) + a(0,1)*b(1,1) + a(0,2)*b(2,1) +a(0,3)*b(3,1);
										ret(0,2) = a(0,0)*b(0,2) + a(0,1)*b(1,2) + a(0,2)*b(2,2) +a(0,3)*b(3,2);
										ret(0,3) = a(0,0)*b(0,3) + a(0,1)*b(1,3) + a(0,2)*b(2,3) +a(0,3)*b(3,3);

										ret(1,0) = a(1,0)*b(0,0) + a(1,1)*b(1,0) + a(1,2)*b(2,0) +a(1,3)*b(3,0);
										ret(1,1) = a(1,0)*b(0,1) + a(1,1)*b(1,1) + a(1,2)*b(2,1) +a(1,3)*b(3,1);
										ret(1,2) = a(1,0)*b(0,2) + a(1,1)*b(1,2) + a(1,2)*b(2,2) +a(1,3)*b(3,2);
										ret(1,3) = a(1,0)*b(0,3) + a(1,1)*b(1,3) + a(1,2)*b(2,3) +a(1,3)*b(3,3);

										ret(2,0) = a(2,0)*b(0,0) + a(2,1)*b(1,0) + a(2,2)*b(2,0) +a(2,3)*b(3,0);
										ret(2,1) = a(2,0)*b(0,1) + a(2,1)*b(1,1) + a(2,2)*b(2,1) +a(2,3)*b(3,1);
										ret(2,2) = a(2,0)*b(0,2) + a(2,1)*b(1,2) + a(2,2)*b(2,2) +a(2,3)*b(3,2);
										ret(2,3) = a(2,0)*b(0,3) + a(2,1)*b(1,3) + a(2,2)*b(2,3) +a(2,3)*b(3,3);

										ret(3,0) = a(3,0)*b(0,0) + a(3,1)*b(1,0) + a(3,2)*b(2,0) +a(3,3)*b(3,0);
										ret(3,1) = a(3,0)*b(0,1) + a(3,1)*b(1,1) + a(3,2)*b(2,1) +a(3,3)*b(3,1);
										ret(3,2) = a(3,0)*b(0,2) + a(3,1)*b(1,2) + a(3,2)*b(2,2) +a(3,3)*b(3,2);
										ret(3,3) = a(3,0)*b(0,3) + a(3,1)*b(1,3) + a(3,2)*b(2,3) +a(3,3)*b(3,3);

										return ret;
									}

									template<class T,bool isColMajor>
		Matrix<T,isColMajor,4,3>	operator* (const Matrix<T,isColMajor,4,3> &a, const Matrix<T,isColMajor,4,3> &b)
									{
										Matrix<T,isColMajor,4,3>	ret;
										ret(0,0) = a(0,0)*b(0,0) + a(0,1)*b(1,0) + a(0,2)*b(2,0);
										ret(0,1) = a(0,0)*b(0,1) + a(0,1)*b(1,1) + a(0,2)*b(2,1);
										ret(0,2) = a(0,0)*b(0,2) + a(0,1)*b(1,2) + a(0,2)*b(2,2);

										ret(1,0) = a(1,0)*b(0,0) + a(1,1)*b(1,0) + a(1,2)*b(2,0);
										ret(1,1) = a(1,0)*b(0,1) + a(1,1)*b(1,1) + a(1,2)*b(2,1);
										ret(1,2) = a(1,0)*b(0,2) + a(1,1)*b(1,2) + a(1,2)*b(2,2);

										ret(2,0) = a(2,0)*b(0,0) + a(2,1)*b(1,0) + a(2,2)*b(2,0);
										ret(2,1) = a(2,0)*b(0,1) + a(2,1)*b(1,1) + a(2,2)*b(2,1);
										ret(2,2) = a(2,0)*b(0,2) + a(2,1)*b(1,2) + a(2,2)*b(2,2);

										ret(3,0) = a(3,0)*b(0,0) + a(3,1)*b(1,0) + a(3,2)*b(2,0) +b(3,0);
										ret(3,1) = a(3,0)*b(0,1) + a(3,1)*b(1,1) + a(3,2)*b(2,1) +b(3,1);
										ret(3,2) = a(3,0)*b(0,2) + a(3,1)*b(1,2) + a(3,2)*b(2,2) +b(3,2);

										return ret;
									}

									template<class T,bool isColMajor, int R, int C>
		Matrix<T,isColMajor,R,C>	operator+ (const Matrix<T,isColMajor,R,C> &a, const Matrix<T,isColMajor,R,C> &b)
									{
										Matrix<T,isColMajor,R,C>	ret;
										u32 n = R*C;
										for (u32 i=0; i<n; i++)
											ret._getValuesPt()[i] = a._getValuesPtConst[i] + b._getValuesPtConst[i];
										return ret;
									}

									template<class T,bool isColMajor, int R, int C>
		Matrix<T,isColMajor,R,C>	operator- (const Matrix<T,isColMajor,R,C> &a, const Matrix<T,isColMajor,R,C> &b)
									{
										Matrix<T,isColMajor,R,C>	ret;
										u32 n = R*C;
										for (u32 i=0; i<n; i++)
											ret._getValuesPt[i] = a._getValuesPtConst[i] - b._getValuesPtConst[i];
										return ret;
									}

		inline f32				lerp (f32 a, f32 b, f32 t)				{ return a + (b-a)*t; }
		void					lerp (const mat4x4f &a, const mat4x4f &b, f32 t01, mat4x4f *out);
		void					lerpArray (const mat4x4f *a, const mat4x4f *b, mat4x4f *out, f32 t01, u32 nMatrix);
		
		void					lerp (const mat3x4f &a, const mat3x4f &b, f32 t01, mat3x4f *out);
		void					lerpArray (const mat3x4f *a, const mat3x4f *b, mat3x4f *out, f32 t01, u32 nMatrix);


		/*=========================================================
		 * 
		 * transformation
		 *
		 *=========================================================*/
							template<class T, bool isColMajor>
		Vec4<T>				vecTransform (const Matrix<T,isColMajor,4,4> &m, const Vec4<T> &v)
							{
								return Vec4<T> (	m(0,0)*v.x +m(0,1)*v.y +m(0,2)*v.z +m(0,3)*v.w,
													m(1,0)*v.x +m(1,1)*v.y +m(1,2)*v.z +m(1,3)*v.w,
													m(2,0)*v.x +m(2,1)*v.y +m(2,2)*v.z +m(2,3)*v.w,
													m(3,0)*v.x +m(3,1)*v.y +m(3,2)*v.z +m(3,3)*v.w
												);
							}

							template<class T, bool isColMajor>
		Vec3<T>				vecTransform (const Matrix<T,isColMajor,4,4> &m, const Vec3<T> &v)
							{
								Vec4<T> v4 = math::vecTransform (m, Vec4<T>(v.x, v.y, v.z, 1));
								return Vec3<T> (v4.x / v4.w, v4.y / v4.w, v4.z / v4.w);
							}

							template<class T, bool isColMajor>
		Vec3<T>				vecTransform (const Matrix<T,isColMajor,3,3> &m, const Vec3<T> &v)
							{
								return Vec3<T> (	m(0,0)*v.x +m(0,1)*v.y +m(0,2)*v.z,
													m(1,0)*v.x +m(1,1)*v.y +m(1,2)*v.z,
													m(2,0)*v.x +m(2,1)*v.y +m(2,2)*v.z
												);
							}

							template<class T, bool isColMajor>
		Vec3<T>				vecTransform (const Matrix<T,isColMajor,3,4> &m, const Vec4<T> &v)
							{
								return Vec3<T>(		m(0,0)*v.x +m(0,1)*v.y +m(0,2)*v.z +m(0,3)*v.w,
													m(1,0)*v.x +m(1,1)*v.y +m(1,2)*v.z +m(1,3)*v.w,
													m(2,0)*v.x +m(2,1)*v.y +m(2,2)*v.z +m(2,3)*v.w
												);
							}

							template<class T, bool isColMajor>
		Vec3<T>				vecTransform (const Matrix<T,isColMajor,3,4> &m, const Vec3<T> &v)
							{
								return Vec3<T>(		m(0,0)*v.x +m(0,1)*v.y +m(0,2)*v.z +m(0,3),
													m(1,0)*v.x +m(1,1)*v.y +m(1,2)*v.z +m(1,3),
													m(2,0)*v.x +m(2,1)*v.y +m(2,2)*v.z +m(2,3)
												);
							}

							template<class T, bool isColMajor>
		Vec2<T>				vecTransform (const Matrix<T,isColMajor,2,2> &m, const Vec2<T> &v)
							{
								return Vec2<T> (	m(0,0)*v.x +m(0,1)*v.y,
													m(1,0)*v.x +m(1,1)*v.y
												);
							}

		vec3f				vecTransform (const Quat &q, const vec3f &v);

	}//namespace math
} //namespace gos
#endif //_gosMath_h_