#ifndef _gosPoint2_h_
#define _gosPoint2_h_
#include "../gosEnumAndDefine.h"
#include "../gosFastArray.h"

namespace gos
{
	/*=====================================
	 * Point2
	 */
	struct Point2
	{
	public:
		i16	x;
		i16 y;

	public:
						Point2()											{ }
						Point2 (i16 xIN, i16 yIN)							{ x = xIN; y = yIN; }
						Point2 (const Point2 &b)							{ x = b.x; y = b.y; }

		void			set(i16 xIN, i16 yIN)								{ x = xIN; y = yIN; }
		Point2&			operator=	(const Point2 &b)						{ x = b.x; y = b.y; return *this; }
		
		Point2&			operator+= (const Point2& b)						{ x += b.x; y += b.y; return *this; }
		friend  Point2	operator+ (const Point2& a, const Point2& b)		{ return Point2(a.x+b.x, a.y+b.y); }
		
		Point2&			operator-=	(const Point2& b)						{ x -= b.x; y -= b.y; return *this; }
		friend  Point2	operator- (const Point2& a, const Point2& b)		{ return Point2(a.x-b.x, a.y-b.y); }

		Point2&			operator*=	(i16 m)									{ x *= m; y *= m; return *this; }
		friend  Point2	operator* (const Point2& a, i16 m)					{ return Point2(a.x*m, a.y*m); }

		Point2&			operator/=	(i16 m)									{ x /= m; y /= m; return *this; }
		friend  Point2	operator/ (const Point2& a, i16 m)					{ return Point2(a.x/m, a.y/m); }

		bool			operator== (const Point2 &b) const					{ return (x==b.x && y==b.y); }
		bool			operator!= (const Point2 &b) const					{ return (x!=b.x || y!=b.y); }
	};

	/*=====================================
	 * Rect2
	 */
	struct Rect2
	{
	public:
		i16	x;
		i16 y;
		i16	dimx;
		i16 dimy;

	public:
						Rect2()												{ }
						Rect2 (i16 xIN, i16 yIN, i16 dimxIN, i16 dimyIN)	{ x = xIN; y = yIN; dimx = dimxIN; dimy = dimyIN; }
						Rect2 (const Rect2 &b)								{ x = b.x; y = b.y; dimx = b.dimx; dimy = b.dimy; }

		void			set (i16 xIN, i16 yIN, i16 dimxIN, i16 dimyIN)		{ x = xIN; y = yIN; dimx = dimxIN; dimy = dimyIN; }
		bool			isPointInside (const Point2 &p) const				{ return isPointInside(p.x, p.y); }
		bool			isPointInside (i16 px, i16 py)	const				{ return (px>=x && py>=y && px<(x+dimx) && py<(y+dimy)); }
		i16				calcX2() const										{ return x + dimx -1; }
		i16				calcY2() const										{ return y + dimy -1; }
		i16				calcCX() const										{ return x + (dimx -1) / 2; }
		i16				calcCY() const										{ return y + (dimy -1) / 2; }

		Rect2&			operator=	(const Rect2 &b)						{ x = b.x; y = b.y; dimx = b.dimx; dimy = b.dimy; return *this; }
		bool			operator== (const Rect2 &b) const					{ return (x==b.x && y==b.y && dimx==b.dimx && dimy==b.dimy); }
		bool			operator!= (const Rect2 &b) const					{ return (x!=b.x || y!=b.y || dimx!=b.dimx || dimy!=b.dimy); }
		Rect2&			operator+=	(const Rect2& b)						
						{
							const i16	x2 = x + dimx-1;
							const i16	y2 = y + dimy-1;
								
							if (b.x < x)	x = b.x;
							if (b.y < y)	y = b.y;

							const i16	bx2 = b.x + b.dimx-1;
							if (bx2 > x2)	
								dimx = bx2 - x + 1; 
							else 
								dimx = x2 - x + 1;

							const i16	by2 = b.y + b.dimy-1;
							if (by2 > y2)	
								dimy = by2 - y + 1; 
							else 
								dimy = y2 - y + 1;

							return *this;
						}
		friend  Rect2	operator+ (const Rect2& a, const Rect2& b)			{ Rect2 r(a); r += b; return r; }

		eClipResult			clipMeAgainstThis(const Rect2 &b)
						{
							const i16 mex2 = calcX2();
							const i16 bx2 = b.calcX2();
							if (mex2 <= b.x)
								return eClipResult::outside;
							if (x >= bx2)
								return eClipResult::outside;

							//giunti qui, mex2 sicuramente > b.x
							eClipResult ret = eClipResult::inside;
							if (x < b.x)
							{
								ret = eClipResult::intersect;
								x = b.x;
							}
							if (mex2 > bx2)
							{
								ret = eClipResult::intersect;
								dimx = bx2 - x + 1;
							}
									
							const i16 mey2 = calcY2();
							const i16 by2 = b.calcY2();
							if (mey2 <= b.y)
								return eClipResult::outside;
							if (y >= by2)
								return eClipResult::outside;

							//giunti qui, mey2 sicuramente > b.y
							if (y < b.y)
							{
								ret = eClipResult::intersect;
								y = b.y;
							}
							if (mey2 > by2)
							{
								ret = eClipResult::intersect;
								dimy = by2 - y + 1;
							}

							assert (dimx > 0);
							assert (dimy > 0);
							return ret;	


							
						}

	};

	/*=====================================
	 * BBox2
	 */
	struct BBox2
	{
	public:
		i16	x1;
		i16 y1;
		i16	x2;
		i16 y2;

	public:
						BBox2()												{ }
						BBox2 (i16 x1IN, i16 y1IN, i16 x2IN, i16 y2IN)		{ set(x1IN, y1IN, x2IN, y2IN); }
						BBox2 (const Point2 &p1, const Point2 &p2)			{ set(p1, p2); }
						BBox2 (const BBox2 &b)								{ set(b); }

		void			set (const Point2 &p1, const Point2 &p2)			{ x1 = p1.x; y1 = p1.y; x2 = p2.x; y2 = p2.y; }
		void			set (i16 x1IN, i16 y1IN, i16 x2IN, i16 y2IN)		{ x1 = x1IN; y1 = y1IN; x2 = x2IN; y2 = y2IN; }
		void			set (const BBox2 &b)								{ x1 = b.x1; y1 = b.y1; x2 = b.x2; y2 = b.y2; }
		
		bool			isPointInside (const Point2 &p) const				{ return isPointInside(p.x, p.y); }
		bool			isPointInside (i16 px, i16 py)	const				{ return (px>=x1 && py>=y1 && px<=x2 && py<=y2); }
		i16				calcDimX() const									{ return x2 - x1 + 1; }
		i16				calcDimY() const									{ return y2 - y1 + 1; }

		eClipResult			doesOverlap (const BBox2 &b) const
						{
							if (x2 < b.x1)	return eClipResult::outside;
							if (x1 > b.x2)	return eClipResult::outside;
							if (y2 < b.y1)	return eClipResult::outside;
							if (y1 > b.y2)	return eClipResult::outside;

							if (!b.isPointInside(x1, y1))
								return eClipResult::intersect;;
							if (!b.isPointInside(x1, y2))
								return eClipResult::intersect;;
							if (!b.isPointInside(x2, y1))
								return eClipResult::intersect;;
							if (!b.isPointInside(x2, y2))
								return eClipResult::intersect;;

							return eClipResult::inside;
						}

		BBox2&			operator=  (const BBox2 &b)							{ set(b); return *this; }
		bool			operator== (const BBox2 &b) const					{ return (x1==b.x1 && y1==b.y1 && x2==b.x2 && y2==b.y2); }
		bool			operator!= (const BBox2 &b) const					{ return (x1!=b.x1 || y1!=b.y1 || x2!=b.x2 || y2!=b.y2); }

		BBox2&			operator+=	(const BBox2& b)						
						{
							if (b.x1 < x1)	x1 = b.x1;
							if (b.y1 < y1)	y1 = b.y1;
							if (b.x2 > x2)	x2 = b.x2;
							if (b.y2 > y2)	y2 = b.y2;
							return *this;
						}
		friend  BBox2	operator+ (const BBox2& a, const BBox2& b)			{ BBox2 r(a); r += b; return r; }

	};

	
	typedef FastArray<Point2> Point2List;

} // namespace gos
#endif //_gosPoint2_h_
