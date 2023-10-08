#ifndef _gosVect_h_
#define _gosVect_h_
#include "gosMathEnumAndDefine.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
namespace gos
{
	namespace math
	{
		/*=========================================================================
		 *  Vec2
		 */
		template <class T>
		class Vec2
		{
		public:
			union
			{
				struct
				{
					T	x,y;
				};
				T	values[2];
			};

			typedef Vec2<T>		my_type;

		public:
									Vec2 ()										{ }
									Vec2 (const T &xIN, const T &yIN)			{ set(xIN, yIN); }

			T						operator [] (const i32 i)					{assert( i>=0 && i<2 );	return values[i];}
			const T&				operator [] (const i32 i) const				{assert( i>=0 && i<2 ); return values[i];}

			void					set (const T &xIN, const T &yIN)			{x=xIN; y=yIN;}
			my_type&				operator= (const my_type &b)				{ set(b.x, b.y); return *this; }
			
			bool					operator== (const my_type &b) const			{ return ( memcmp(values, b.values, sizeof(values)) == 0); }
			bool					operator!= (const my_type &b) const			{ return ( memcmp(values, b.values, sizeof(values)) != 0); }

			T						length ()	const							{return sqrtf(x*x +y*y);}
			T						length2 ()	const							{return x*x +y*y;}
			void					normalize()									{normalizeWithLen(length());}
			void					normalizeWithLen (const T &len)				{ assert(len != 0); x /= len; y /= len; }
			
			void					operator+= (const my_type &b)						{ x+= b.x; y+=b.y; }
			void					operator-= (const my_type &b)						{ x-= b.x; y-=b.y; }
			void					operator*= (const my_type &b)						{ x*=b.x; y*=b.y; }
			void					operator*= (const T &b)								{ x*=b; y*=b; }
			void					operator/= (const T &b)								{ assert(b!=0); x/=b; y/=b; }

			T						distance (const my_type &a, const my_type &b)		{ return (b-a).length(); }
			T						distance2 (const my_type &a, const my_type &b)		{ return (b-a).length2(); }
		};

		/*=========================================================================
		 * Vec3
		 */
		template <class T>
		class Vec3
		{
		public:
			union
			{
				struct
				{
					T	x,y,z;
				};
				T	values[3];
			};

			typedef Vec3<T>	my_type;

		public:
									Vec3 ()																{ }
									Vec3 (const T &xIN, const T &yIN, const T &zIN)						{ set(xIN, yIN, zIN); }

			T						operator [] (const i32 i)											{ assert( i>=0 && i<3 ); return values[i]; }
			const T&				operator [] (const i32 i) const										{ assert( i>=0 && i<3 ); return values[i]; }
			bool					operator== (const my_type &b) const									{ return ( memcmp(values, b.values, sizeof(values)) == 0); }
			bool					operator!= (const my_type &b) const									{ return ( memcmp(values, b.values, sizeof(values)) != 0); }

			void					set (const T &xIN, const T &yIN, const T &zIN)						{ x = xIN; y = yIN; z = zIN; }
			void					setAndNormalize(const T &xIN, const T &yIN, const T &zIN)			{set (xIN, yIN, zIN); normalize(); }
			my_type&				operator= (const my_type &b)										{ set(b.x, b.y, b.z); return *this; }

			T						length ()	const													{ return sqrtf(x*x +y*y + z*z); }
			T						length2 ()	const													{ return x*x +y*y + z*z; }
			void					normalize()															{ normalizeWithLen(length()); }
			void					normalizeWithLen (const T &len)										{ assert(len != 0); x /= len; y /= len; z /= len; }
			
			void					operator+= (const my_type &b)							{ x+= b.x; y+=b.y; z+=b.z; }
			void					operator-= (const my_type &b)							{ x-= b.x; y-=b.y; z-=b.z; }
			void					operator*= (const my_type &b)							{ x*=b.x; y*=b.y; z*=b.z; }
			void					operator*= (const T &b)									{ x*=b; y*=b; z*=b; }
			void					operator/= (const T &b)									{ assert(b != 0); x /= b; y /= b; z /= b; }
			T						distance (const my_type &a, const my_type &b)			{ return (b-a).length(); }
			T						distance2 (const my_type &a, const my_type &b)			{ return (b-a).length2(); }
		};

		/*=========================================================================
		 * Vec4
		 */
		template <class T>
		class Vec4
		{
		public:
			union
			{
				struct
				{
					T	x,y,z,w;
				};
				T	values[4];
			};

			typedef Vec4<T>	my_type;

		public:
									Vec4 ()																	{ }
									Vec4 (const T &xIN, const T &yIN, const T &zIN, const T &wIN)			{ set(xIN, yIN, zIN, wIN); }

			T						operator [] (const i32 i)												{ assert( i>=0 && i<4 ); return values[i]; }
			const T&				operator [] (const i32 i) const											{ assert( i>=0 && i<4 ); return values[i]; }
			bool					operator== (const my_type &b) const										{ return ( memcmp(values, b.values, sizeof(values)) == 0); }
			bool					operator!= (const my_type &b) const										{ return ( memcmp(values, b.values, sizeof(values)) != 0); }

			void					set (const T &xIN, const T &yIN, const T &zIN, const T &wIN)			{ x = xIN; y = yIN; z = zIN; w = wIN; }
			void					set (const math::Vec3<T> &v3, const T &p4)								{ x = v3.x; y = v3.y; z = v3.z; w = p4; }
			my_type&				operator= (const my_type &b)											{ set(b.x, b.y, b.z, b.w); return *this; }

			T						length ()	const														{ return sqrtf(x*x +y*y + z*z +w*w); }
			T						length2 ()	const														{ return x*x +y*y + z*z +w*w; }
			void					normalize()																{ normalizeWithLen(length()); }
			void					normalizeWithLen (const T &len)											{ assert(len != 0);  x /= len; y /= len; z /= len; w /= len; }
			
			void					operator+= (const my_type &b)								{ x += b.x; y += b.y; z += b.z; w += b.w; }
			void					operator-= (const my_type &b)								{ x-= b.x; y-=b.y; z-=b.z; w-=b.w;}
			void					operator*= (const my_type &b)								{ x*=b.x; y*=b.y; z*=b.z; w*=b.w; }
			void					operator*= (const T &b)										{ x*=b; y*=b; z*=b; w*=b; }
			void					operator/= (const T &b)										{ assert(b != 0); x /= b; y /= b; z /= b; w /= b;}

			T						distance (const my_type &a, const my_type &b)				{ return (b-a).length(); }
			T						distance2 (const my_type &a, const my_type &b)				{ return (b-a).length2(); }
		};

	} //namespace math
	
	typedef math::Vec2<f32>	vec2f;
	typedef math::Vec3<f32>	vec3f;
	typedef math::Vec4<f32>	vec4f;

	typedef math::Vec2<i32>	vec2i;
	typedef math::Vec3<i32>	vec3i;
	typedef math::Vec4<i32>	vec4i;

	typedef	math::Vec2<u16>	vec2u16;
	typedef	math::Vec2<i16>	vec2i16;
} //namespace gos
#pragma GCC diagnostic pop

#endif //_gosVect_h_