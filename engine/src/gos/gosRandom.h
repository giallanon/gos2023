#ifndef _gosRandom_h_
#define _gosRandom_h_
#include <math.h>
#include "helpers/mtrand.h"
#include "gosEnumAndDefine.h"


namespace gos
{
	/*===================================================
	 *
	 *==================================================*/
	class Random
	{
	public:
						Random() : mt((u32)0)				{}
						Random	(u32 aSeed) : mt(aSeed)		{}

		void			seed	(u32 aSeed)					{ mt.seed(aSeed); }
		f32				get01()								{ return (f32)mt(); }
							//ritorna un num compreso tra 0 e 1 inclusi
		u32				getU32 (u32 iMax)					{ return (u32)floorf(0.5f + (f32)mt() * iMax); }
							//ritorna un u32 compreso tra 0 e iMax incluso

	private:
		MTRand_closed	mt;
	};
} //namespace gos

#endif //_gosRandom_h_
