#ifndef _gosMatrix_h_
#define _gosMatrix_h_
#include "gosMathEnumAndDefine.h"
#include "gosVect.h"
#include "gosMathMatrix3x3Helper.h"

namespace gos
{
	namespace math
	{
		//constexpr  i32 colMajorADDR(i32 row, i32 col, i32 NUM_ROWS) { return (col*NUM_ROWS + row); }
		//#define matrix_rowMajorAddr(row,col,NUM_COLS)	values[(row)*NUM_COLS + (col)]

		template<int ROWS, int COLS, bool isColMajor>
		static constexpr  i32 MATRIX_ADDR(i32 row, i32 col) 
		{ 
			if constexpr (isColMajor == true)
				return (col*ROWS + row); 
			else
				return (row*COLS + col); 
		}

		/*=========================================================================
		 * Il template che rappresenta una generica matrice<T,r,c>
		 *
		 *	Può memorizzare i dati in formato col_major oppure row_major
		 *	L'unica differenza tra i 2 è l'ordine con il quale vengono memorizzati i singoli elementi
		 *	della matrice
		 */
		template <class T, bool isColMajor, int ROWS, int COLS>
		class Matrix
		{
		private:
#define ADDR(row, col) MATRIX_ADDR<ROWS,COLS,isColMajor>(row,col)

		public:
			typedef Matrix<T,isColMajor,ROWS,COLS>		my_type;

		public:
							Matrix()									{ };
			T&				operator() (i32 r, i32 c)					{ assert(r>=0 && r<ROWS && c>=0 && c<COLS); return values[ADDR(r,c)]; }
			const T&		operator() (i32 r, i32 c) const				{ assert(r>=0 && r<ROWS && c>=0 && c<COLS); return values[ADDR(r,c)]; }
			my_type&		operator= (const my_type &b)				{ memcpy(values,b.values,sizeof(values)); return *this; }

			void			identity()
							{
								memset (values,0,sizeof(values));
								
								if (ROWS <= COLS)
								{
									for (u8 i = 0; i < ROWS; i++)
										values[ADDR(i,i)] = 1;
								}
								else
								{
									for (u8 i = 0; i < COLS; i++)
										values[ADDR(i,i)] = 1;
								}
							}

			const T*		_getValuesPtConst() const					{ return values; }
			T*				_getValuesPt()								{ return values; }

		private:
			T	values[ROWS * COLS];
#undef ADDR
		};


		/*=========================================================================
		 * Specializzazione per N=2,2
		 */
		template <class T, bool isColMajor>
		class Matrix<T, isColMajor, 2, 2>
		{
		private:
#define ADDR(row, col) MATRIX_ADDR<2,2,isColMajor>(row,col)

		public:
			typedef Matrix<T,isColMajor,2,2>		my_type;

		public:
							Matrix()								{ };

			T&				operator() (i32 r, i32 c)				{ assert(r>=0 && r<2 && c>=0 && c<2); return values[ADDR(r,c)]; }
			const T&		operator() (i32 r, i32 c) const			{ assert(r>=0 && r<2 && c>=0 && c<2); return values[ADDR(r,c)]; }
			my_type&		operator= (const my_type &b)			{ memcpy(values,b.values,sizeof(values)); return *this; }

			void			identity()								
							{ 
								memset (values,0,sizeof(values));
								values[ADDR(0, 0)] = 
								values[ADDR(1, 1)] = 1;
							}

			void			transpose()								
							{ 
								T temp = values[ADDR(0, 1)]; 
								values[ADDR(0, 1)] = values[ADDR(1, 0)]; 
								values[ADDR(1, 0)] = temp; 
							}

			void			buildRotationMatrix (f32 rad)
							{
								//cosa	-sina
								//sina	cosa
								const f32 cosa = cosf(rad);
								const f32 sina = sinf(rad);
								values[ADDR(0, 0)] = cosa;		values[ADDR(0, 1)] = -sina;
								values[ADDR(1, 0)] = sina;		values[ADDR(1, 1)] = cosa;
							}

		private:
			T	values[4];
#undef ADDR
		};



		/*=========================================================================
		 * Specializzazione per N=3,3
		 */
		template <class T, bool isColMajor>
		class Matrix<T, isColMajor,3,3>
		{
		private:
#define ADDR(row, col) MATRIX_ADDR<3,3,isColMajor>(row,col)

		public:
			typedef Matrix<T,isColMajor,3,3>		my_type;

		public:
							Matrix()																		{ };

			T&				operator() (i32 r, i32 c)														{ assert(r>=0 && r<3 && c>=0 && c<3); return values[ADDR(r,c)]; }
			const T&		operator() (i32 r, i32 c) const													{ assert(r>=0 && r<3 && c>=0 && c<3); return values[ADDR(r,c)]; }
			my_type&		operator= (const my_type &b)													{ memcpy(values,b.values,sizeof(values)); return *this; }

			void			identity()																		
							{ 
								memset (values,0,sizeof(values));
								values[ADDR(0, 0)] = 
								values[ADDR(1, 1)] = 
								values[ADDR(2, 2)] = 1;
							}

			void			transpose()								
							{ 
								T temp;
								#define SWAP(r1,c1,r2,c2)	temp = values[ADDR(r1, c1)];\
															values[ADDR(r1, c1)] = values[ADDR(r2, c2)];\
															values[ADDR(r2, c2)] = temp;\

								SWAP(0,1,  1,0);
								SWAP(0,2,  2,0); 
								SWAP(1,2,  2,1); 
								#undef SWAP


								
							}

			void			buildFromXYZAxes (const Vec3<T> &x, const Vec3<T> &y, const Vec3<T> &z)			{ helper_Matrix3x3_buildFromXYZAxes (x, y, z); }
			void			buildFromAxeAndAngle (const Vec3<T> &ax, f32 rad)								{ helper_Matrix3x3_buildFromAxeAndAngle (ax, rad); }
			void			buildFromEulerAngles_YXZ (f32 rad_y, f32 rad_x, f32 rad_z)						{ helper_Matrix3x3_buildFromEulerAngles_YXZ(rad_y, rad_x, rad_z); }

			void			buildLookAt (const Vec3<T> &eye, const Vec3<T> &at, const Vec3<T> &up)			{ helper_Matrix3x3_buildLookAt (eye, at, up); }

			void			buildRotationAboutX (f32 rad)													{ helper_Matrix3x3_buildRotationAboutX (rad); }
		 	void			buildRotationAboutY (T rad)														{ helper_Matrix3x3_buildRotationAboutY (rad); }
			void			buildRotationAboutZ (T rad)														{ helper_Matrix3x3_buildRotationAboutZ (rad); }
			void			buildScale (const vec3f &s)														{ helper_Matrix3x3_buildScale (s); }

			T				determinant() const																
							{ 
								return	  values[ADDR(0,0)] * ( values[ADDR(1,1)]* values[ADDR(2,2)]- values[ADDR(2,1)]* values[ADDR(1,2)])
										- values[ADDR(0,1)] * ( values[ADDR(1,0)]* values[ADDR(2,2)]- values[ADDR(1,2)]* values[ADDR(2,0)])
										+ values[ADDR(0,2)] * ( values[ADDR(1,0)]* values[ADDR(2,1)]- values[ADDR(1,1)]* values[ADDR(2,0)]); 
							}

			void			calcInverse (my_type *out) const
							{
								T det = determinant();
								if (0 == det)
									return;
								(*out)(0,0) =  ( values[ADDR(1,1)] * values[ADDR(2,2)] - values[ADDR(2,1)] * values[ADDR(1,2)])/det;
								(*out)(0,1) = -( values[ADDR(1,0)] * values[ADDR(2,2)] - values[ADDR(1,2)] * values[ADDR(2,0)])/det;
								(*out)(0,2) =  ( values[ADDR(1,0)] * values[ADDR(2,1)] - values[ADDR(2,0)] * values[ADDR(1,1)])/det;
								(*out)(1,0) = -( values[ADDR(0,1)] * values[ADDR(2,2)] - values[ADDR(0,2)] * values[ADDR(2,1)])/det;
								(*out)(1,1) =  ( values[ADDR(0,0)] * values[ADDR(2,2)] - values[ADDR(0,2)] * values[ADDR(2,0)])/det;
								(*out)(1,2) = -( values[ADDR(0,0)] * values[ADDR(2,1)] - values[ADDR(2,0)] * values[ADDR(0,1)])/det;
								(*out)(2,0) =  ( values[ADDR(0,1)] * values[ADDR(1,2)] - values[ADDR(0,2)] * values[ADDR(1,1)])/det;
								(*out)(2,1) = -( values[ADDR(0,0)] * values[ADDR(1,2)] - values[ADDR(1,0)] * values[ADDR(0,2)])/det;
								(*out)(2,2) =  ( values[ADDR(0,0)] * values[ADDR(1,1)] - values[ADDR(1,0)] * values[ADDR(0,1)])/det;
							}

		private:
			T	values[9];
#undef ADDR
		};

			   		 
		/*=========================================================================
		 * Specializzazione per N=4,4
		 *========================================================================*/
		template <class T, bool isColMajor>
		class Matrix<T, isColMajor,4,4>
		{
		private:
#define ADDR(row, col) MATRIX_ADDR<4,4,isColMajor>(row,col)

		public:
			typedef Matrix<T,isColMajor,4,4>		my_type;

		public:
							Matrix()													{ };

			T&				operator() (i32 r, i32 c)									{ assert(r>=0 && r<4 && c>=0 && c<4); return values[ADDR(r,c)]; }
			const T&		operator() (i32 r, i32 c) const								{ assert(r>=0 && r<4 && c>=0 && c<4); return values[ADDR(r,c)]; }
			my_type&		operator= (const my_type &b)								{ memcpy(values,b.values,sizeof(values)); return *this; }

			void			identity()
							{ 
								memset (values,0,sizeof(values));
								values[ADDR(0, 0)] = 
								values[ADDR(1, 1)] = 
								values[ADDR(2, 2)] = 
								values[ADDR(3, 3)] = 1;
							}

			void			transpose()								
							{ 
								T temp;
								#define SWAP(r1,c1,r2,c2)	temp = values[ADDR(r1, c1)];\
															values[ADDR(r1, c1)] = values[ADDR(r2, c2)];\
															values[ADDR(r2, c2)] = temp;\

								SWAP(0,1,	1,0);
								SWAP(0,2,	2,0);
								SWAP(0,3,	3,0);
								SWAP(1,2,	2,1);
								SWAP(1,3,	3,1);
								SWAP(2,3,	3,2);
								#undef SWAP
							}

			void			extractTranslation (Vec3<T> *out) const											
							{ 
								out->x = values[ADDR(0, 3)];
								out->y = values[ADDR(1, 3)];
								out->z = values[ADDR(2, 3)];
							}

			void			extractRotationMatrix (Matrix<T,isColMajor,3,3> *out) const
							{
								(*out)(0,0) = values[ADDR(0,0)];	(*out)(0,1) = values[ADDR(0,1)];	(*out)(0,2) = values[ADDR(0,2)];
								(*out)(1,0) = values[ADDR(1,0)];	(*out)(1,1) = values[ADDR(1,1)];	(*out)(1,2) = values[ADDR(1,2)];
								(*out)(2,0) = values[ADDR(2,0)];	(*out)(2,1) = values[ADDR(2,1)];	(*out)(2,2) = values[ADDR(2,2)];
							}

			void			setTranslation (const Vec3<T> &v)
							{
								values[ADDR(0, 3)] = v.x;
								values[ADDR(1, 3)] = v.y;
								values[ADDR(2, 3)] = v.z;
							}

			void			buildTranslation (const Vec3<T> &v)							{ buildTranslation(v.x, v.y, v.z); }

			void			buildTranslation (T x, T y, T z)
							{
								values[ADDR(0,0)] = 1;	values[ADDR(0,1)] = 0;  values[ADDR(0,2)] = 0;	values[ADDR(0,3)] = x;
								values[ADDR(1,0)] = 0;  values[ADDR(1,1)] = 1;	values[ADDR(1,2)] = 0;	values[ADDR(1,3)] = y;
								values[ADDR(2,0)] = 0;  values[ADDR(2,1)] = 0;  values[ADDR(2,2)] = 1;	values[ADDR(2,3)] = z;
							    values[ADDR(3,0)] = 0;  values[ADDR(3,1)] = 0;	values[ADDR(3,2)] = 0;	values[ADDR(3,3)] = 1;
							}

			void			buildScale (const vec3f &s)						{ buildScale (s.x, s.y, s.z); }
			
			void			buildScale (const T sx, T sy, T sz)
							{
								values[ADDR(0,0)] = sx;		values[ADDR(0,1)] = 0;  values[ADDR(0,2)] = 0;  values[ADDR(0,3)]=0;
								values[ADDR(1,0)] = 0;		values[ADDR(1,1)] = sy;	values[ADDR(1,2)] = 0;  values[ADDR(1,3)]=0;
								values[ADDR(2,0)] = 0;		values[ADDR(2,1)] = 0;  values[ADDR(2,2)] = sz;	values[ADDR(2,3)]=0;
							    values[ADDR(3,0)] = 0;		values[ADDR(3,1)] = 0;	values[ADDR(3,2)] = 0;	values[ADDR(3,3)]=1;
							}

			void			buildFromXYZAxes (const Vec3<T> &ax, const Vec3<T> &ay, const Vec3<T> &az)
							{
								helper_Matrix3x3_buildFromXYZAxes(ax, ay, az);
								values[ADDR(0,3)] =
								values[ADDR(1,3)] =
								values[ADDR(2,3)] =
								values[ADDR(3,0)] =
								values[ADDR(3,1)] =
								values[ADDR(3,2)] = 0;
								values[ADDR(3,3)] = 1;
							}

			void			buildFromAxeAndAngle (const Vec3<T> &axle, f32 rad)
							{
								helper_Matrix3x3_buildFromAxeAndAngle(axle, rad);
								values[ADDR(0,3)] = values[ADDR(1,3)] = values[ADDR(2,3)] = values[ADDR(3,0)] = values[ADDR(3,1)] = values[ADDR(3,2)] = 0;
								values[ADDR(3,3)]=1;
							}

			void			buildRotationAboutX (f32 rad)
							{
								helper_Matrix3x3_buildRotationAboutX(rad);
								values[ADDR(0,3)] = values[ADDR(1,3)] = values[ADDR(2,3)] = values[ADDR(3,0)] = values[ADDR(3,1)] = values[ADDR(3,2)] = 0;
								values[ADDR(3,3)]=1;
							}

		 	void			buildRotationAboutY (T rad)
							{
								helper_Matrix3x3_buildRotationAboutY(rad);
								values[ADDR(0,3)] = values[ADDR(1,3)] = values[ADDR(2,3)] = values[ADDR(3,0)] = values[ADDR(3,1)] = values[ADDR(3,2)] = 0;
								values[ADDR(3,3)]=1;
							}
		
			void			buildRotationAboutZ (T rad)
							{
								helper_Matrix3x3_buildRotationAboutZ(rad);
								values[ADDR(0,3)] = values[ADDR(1,3)] = values[ADDR(2,3)] = values[ADDR(3,0)] = values[ADDR(3,1)] = values[ADDR(3,2)] = 0;
								values[ADDR(3,3)]=1;
							}

			void			buildLookAt (const Vec3<T> &eye, const Vec3<T> &at, const Vec3<T> &up)
							{
								helper_Matrix3x3_buildLookAt(eye,at,up);
								values[ADDR(0,3)] = values[ADDR(1,3)] = values[ADDR(2,3)] = values[ADDR(3,0)] = values[ADDR(3,1)] = values[ADDR(3,2)] = 0;
								values[ADDR(3,3)]=1;
							}
			
			void			calcInverse (my_type *out) const
							{
								T m00 = values[ADDR(0,0)], m01 = values[ADDR(0,1)], m02 = values[ADDR(0,2)], m03 = values[ADDR(0,3)];
								T m10 = values[ADDR(1,0)], m11 = values[ADDR(1,1)], m12 = values[ADDR(1,2)], m13 = values[ADDR(1,3)];
								T m20 = values[ADDR(2,0)], m21 = values[ADDR(2,1)], m22 = values[ADDR(2,2)], m23 = values[ADDR(2,3)];
								T m30 = values[ADDR(3,0)], m31 = values[ADDR(3,1)], m32 = values[ADDR(3,2)], m33 = values[ADDR(3,3)];

								T v0 = m20 * m31 - m21 * m30;
								T v1 = m20 * m32 - m22 * m30;
								T v2 = m20 * m33 - m23 * m30;
								T v3 = m21 * m32 - m22 * m31;
								T v4 = m21 * m33 - m23 * m31;
								T v5 = m22 * m33 - m23 * m32;

								const T t00 = + (v5 * m11 - v4 * m12 + v3 * m13);
								const T t10 = - (v5 * m10 - v2 * m12 + v1 * m13);
								const T t20 = + (v4 * m10 - v2 * m11 + v0 * m13);
								const T t30 = - (v3 * m10 - v1 * m11 + v0 * m12);

								const T invDet = 1 / (t00 * m00 + t10 * m01 + t20 * m02 + t30 * m03);

								const T d00 = t00 * invDet;
								const T d10 = t10 * invDet;
								const T d20 = t20 * invDet;
								const T d30 = t30 * invDet;

								const T d01 = - (v5 * m01 - v4 * m02 + v3 * m03) * invDet;
								const T d11 = + (v5 * m00 - v2 * m02 + v1 * m03) * invDet;
								const T d21 = - (v4 * m00 - v2 * m01 + v0 * m03) * invDet;
								const T d31 = + (v3 * m00 - v1 * m01 + v0 * m02) * invDet;

								v0 = m10 * m31 - m11 * m30;
								v1 = m10 * m32 - m12 * m30;
								v2 = m10 * m33 - m13 * m30;
								v3 = m11 * m32 - m12 * m31;
								v4 = m11 * m33 - m13 * m31;
								v5 = m12 * m33 - m13 * m32;

								const T d02 = + (v5 * m01 - v4 * m02 + v3 * m03) * invDet;
								const T d12 = - (v5 * m00 - v2 * m02 + v1 * m03) * invDet;
								const T d22 = + (v4 * m00 - v2 * m01 + v0 * m03) * invDet;
								const T d32 = - (v3 * m00 - v1 * m01 + v0 * m02) * invDet;

								v0 = m21 * m10 - m20 * m11;
								v1 = m22 * m10 - m20 * m12;
								v2 = m23 * m10 - m20 * m13;
								v3 = m22 * m11 - m21 * m12;
								v4 = m23 * m11 - m21 * m13;
								v5 = m23 * m12 - m22 * m13;

								const T d03 = - (v5 * m01 - v4 * m02 + v3 * m03) * invDet;
								const T d13 = + (v5 * m00 - v2 * m02 + v1 * m03) * invDet;
								const T d23 = - (v4 * m00 - v2 * m01 + v0 * m03) * invDet;
								const T d33 = + (v3 * m00 - v1 * m01 + v0 * m02) * invDet;

								(*out)(0,0) = d00;	(*out)(0,1) = d01; (*out)(0,2) = d02; (*out)(0,3) = d03;
								(*out)(1,0) = d10;	(*out)(1,1) = d11; (*out)(1,2) = d12; (*out)(1,3) = d13;
								(*out)(2,0) = d20;	(*out)(2,1) = d21; (*out)(2,2) = d22; (*out)(2,3) = d23;
								(*out)(3,0) = d30;	(*out)(3,1) = d31; (*out)(3,2) = d32; (*out)(3,3) = d33;
							}

			bool			isAffine() const
							{
								return (values[ADDR(0,3)] == 0 && values[ADDR(1,3)] == 0 && values[ADDR(2,3)] == 0 && values[ADDR(3,3)] == 1);
							}

			void			calcInverseAffine (my_type *out) const
							{
								assert(isAffine());
								const T m00 = values[ADDR(0,0)];
								const T m01 = values[ADDR(0,1)];
								const T m02 = values[ADDR(0,2)];
								const T m10 = values[ADDR(1,0)];
								const T m11 = values[ADDR(1,1)];
								const T m12 = values[ADDR(1,2)];
								const T m20 = values[ADDR(2,0)];
								const T m21 = values[ADDR(2,1)];
								const T m22 = values[ADDR(2,2)];
								const T m30 = values[ADDR(3,0)];
								const T m31 = values[ADDR(3,1)];
								const T m32 = values[ADDR(3,2)];

								T v0 = m20 * m31 - m21 * m30;
								T v1 = m20 * m32 - m22 * m30;
								T v3 = m21 * m32 - m22 * m31;

								const T t00 = m22 * m11 - m21 * m12;
								const T t10 = m20 * m12 - m22 * m10;
								const T t20 = m21 * m10 - m20 * m11;
								const T t30 = - (v3 * m10 - v1 * m11 + v0 * m12);

								const T invDet = 1 / (t00 * m00 + t10 * m01 + t20 * m02);

								const T d00 = t00 * invDet;
								const T d10 = t10 * invDet;
								const T d20 = t20 * invDet;
								const T d30 = t30 * invDet;

								const T d01 = - (m22 * m01 - m21 * m02) * invDet;
								const T d11 = + (m22 * m00 - m20 * m02) * invDet;
								const T d21 = - (m21 * m00 - m20 * m01) * invDet;
								const T d31 = + (v3 * m00 - v1 * m01 + v0 * m02) * invDet;

								v0 = m10 * m31 - m11 * m30;
								v1 = m10 * m32 - m12 * m30;
								v3 = m11 * m32 - m12 * m31;

								const T d02 = + (m12 * m01 - m11 * m02) * invDet;
								const T d12 = - (m12 * m00 - m10 * m02) * invDet;
								const T d22 = + (m11 * m00 - m10 * m01) * invDet;
								const T d32 = - (v3 * m00 - v1 * m01 + v0 * m02) * invDet;


								(*out)(0,0) = d00;	(*out)(0,1) = d01; (*out)(0,2) = d02; (*out)(0,3) = 0;
								(*out)(1,0) = d10;	(*out)(1,1) = d11; (*out)(1,2) = d12; (*out)(1,3) = 0;
								(*out)(2,0) = d20;	(*out)(2,1) = d21; (*out)(2,2) = d22; (*out)(2,3) = 0;
								(*out)(3,0) = d30;	(*out)(3,1) = d31; (*out)(3,2) = d32; (*out)(3,3) = 1;
							}


			const T*		_getValuesPtConst() const	{ return values; }
			T*				_getValuesPt()				{ return values; }

		private:
			T	values[16];
#undef ADDR
		};

		/*=========================================================================
		 * Specializzazione per N=3,4
		 */
		template <class T, bool isColMajor>
		class Matrix<T, isColMajor,3,4>
		{
		private:
#define ADDR(row, col) MATRIX_ADDR<3,4,isColMajor>(row,col)

		public:
			typedef Matrix<T,isColMajor,3,4>		my_type;

		public:
							Matrix()																		{ };

			T&				operator() (i32 r, i32 c)														{ assert(r>=0 && r<3 && c>=0 && c<4); return values[ADDR(r,c)]; }
			const T&		operator() (i32 r, i32 c) const													{ assert(r>=0 && r<3 && c>=0 && c<4); return values[ADDR(r,c)]; }
			my_type&		operator= (const my_type &b)													{ memcpy(values,b.values,sizeof(values)); return *this; }

			void			identity()
							{ 
								memset (values,0,sizeof(values));
								values[ADDR(0, 0)] =
								values[ADDR(1, 1)] =
								values[ADDR(2, 2)] = 1;
							}			

			void			setTranslation (const Vec3<T> &v)
							{
								values[ADDR(0, 3)] = v.x;
								values[ADDR(1, 3)] = v.y;
								values[ADDR(2, 3)] = v.z;
							}

			void			extractTranslation (Vec3<T> *out) const											
							{ 
								out->x = values[ADDR(0, 3)];
								out->y = values[ADDR(1, 3)];
								out->z = values[ADDR(2, 3)];
							}

			void			transpose (Matrix<T, isColMajor, 4, 3> *out) const
							{
								(*out)(0, 0) = values[ADDR(0, 0)];	(*out)(0, 1) = values[ADDR(1, 0)];	(*out)(0, 2) = values[ADDR(2, 0)];
								(*out)(1, 0) = values[ADDR(0, 1)];	(*out)(1, 1) = values[ADDR(1, 1)];	(*out)(1, 2) = values[ADDR(2, 1)];
								(*out)(2, 0) = values[ADDR(0, 2)];	(*out)(2, 1) = values[ADDR(1, 2)];	(*out)(2, 2) = values[ADDR(2, 2)];
								(*out)(3, 0) = values[ADDR(0, 3)];	(*out)(3, 1) = values[ADDR(1, 3)];	(*out)(3, 2) = values[ADDR(2, 3)];
							}

			void			extractRotationMatrix (my_type *out) const										{ memcpy (out->values, values, sizeof(T)*9); }
								 
			void			buildTranslation (const Vec3<T> &v)												{ buildTranslation(v.x, v.y, v.z); }
			
			void			buildTranslation (T x, T y, T z)
							{
								values[ADDR(0,0)] = 1;	values[ADDR(0,1)] = 0;  values[ADDR(0,2)] = 0;	values[ADDR(0,3)] = x;
								values[ADDR(1,0)] = 0;  values[ADDR(1,1)] = 1;	values[ADDR(1,2)] = 0;	values[ADDR(1,3)] = y;
								values[ADDR(2,0)] = 0;  values[ADDR(2,1)] = 0;  values[ADDR(2,2)] = 1;	values[ADDR(2,3)] = z;
							}

			void			buildFromAxeAndAngle (const Vec3<T> &ax, f32 rad)								{ helper_Matrix3x3_buildFromAxeAndAngle (ax, rad); values[ADDR(0,3)] = values[ADDR(1,3)] = values[ADDR(2,3)] = 0; }
			void			buildFromXYZAxes (const Vec3<T> &x, const Vec3<T> &y, const Vec3<T> &z)			{ helper_Matrix3x3_buildFromXYZAxes (x, y, z); values[ADDR(0,3)] = values[ADDR(1,3)] = values[ADDR(2,3)] = 0; }
			void			buildScale (const vec3f &s)														{ helper_Matrix3x3_buildScale (s); values[ADDR(0,3)] = values[ADDR(1,3)] = values[ADDR(2,3)] = 0; }
			void			buildRotationAboutX (f32 rad)													{ helper_Matrix3x3_buildRotationAboutX (rad); values[ADDR(0,3)] = values[ADDR(1,3)] = values[ADDR(2,3)] = 0; }
		 	void			buildRotationAboutY (T rad)														{ helper_Matrix3x3_buildRotationAboutY (rad); values[ADDR(0,3)] = values[ADDR(1,3)] = values[ADDR(2,3)] = 0; }
			void			buildRotationAboutZ (T rad)														{ helper_Matrix3x3_buildRotationAboutZ (rad); values[ADDR(0,3)] = values[ADDR(1,3)] = values[ADDR(2,3)] = 0; }
			void			buildLookAt (const Vec3<T> &eye, const Vec3<T> &at, const Vec3<T> &up)			{ helper_Matrix3x3_buildLookAt (eye, at, up); values[ADDR(0,3)] = values[ADDR(1,3)] = values[ADDR(2,3)] = 0; }
			void			buildFromEulerAngles_YXZ (f32 rad_y, f32 rad_x, f32 rad_z)						{ helper_Matrix3x3_buildFromEulerAngles_YXZ(rad_y, rad_x, rad_z); }
			
			const T*		_getValuesPtConst() const	{ return values; }
			T*				_getValuesPt()				{ return values; }

		private:
			T	values[12];
#undef ADDR
		};
	}// namespace math


	//versione row major 
	typedef math::Matrix<f32, false, 2, 2>	mat2x2f;
	typedef math::Matrix<f32, false, 3, 3>	mat3x3f;
	typedef math::Matrix<f32, false, 4, 4>	mat4x4f;
	typedef math::Matrix<f32, false, 3, 4>	mat3x4f;

	//versione row major
	typedef math::Matrix<f32, true, 3, 4>	matCM3x4f;
	
}// namespace gos

#endif //_gosMatrix_h_