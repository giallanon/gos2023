#ifndef _gosGeomKDop_h_
#define _gosGeomKDop_h_
#include "gosGeomPlane3.h"

namespace gos
{
	namespace geom
	{
		/***********************************************
		 * @brief	KDop
		 * 			un k-DOP con un numero (massimo) di Plane3 limitato in modo
		 * 			da non dover allocare memoria
		 **/
		template <unsigned nMaxPlanes>
		class KDop
		{
		public:
							KDop()								{ reset(); }
				
							//================= operators
			Plane3&			operator[] (u32 i)					{ assert(i<nPlanes); return planes[i]; }
			const Plane3&	operator() (u32 i) const			{ assert(i<nPlanes); return planes[i]; }

							//================= fn
			void			reset()								{ nPlanes=0; }

			bool			add_plane (const Plane3 &planeIN) 
							{
								if (nPlanes<nMaxPlanes)
								{
									planes[nPlanes++] = planeIN;
									return true;
								}
								return false;
							}

							//================= query 
			u32				get_num_planes () const				{ return nPlanes; }

		protected:
			Plane3			planes[nMaxPlanes];
			u32				nPlanes;
		};
	} //namespace geom
} //namespace gos
#endif //_gosGeomKDop_h_