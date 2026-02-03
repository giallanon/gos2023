#ifndef _gosEngineRes2_h_
#define _gosEngineRes2_h_
#include "gosEngineResMan.h"
#include "../gosEngineEnumAndDefine.h"


#define GOS_DECL_RES_HANDLE(HANDLE_TYPE)\
struct HANDLE_TYPE\
{\
	gos::res::Handle res_handle;\
};\

namespace gos
{
	GOS_DECL_RES_HANDLE(ENGPippo);

	namespace res
	{
		//************************************
		struct Pippo
		{
			res::Descr			_descr;
			GPUVtxBufferHandle	vbHandle;
		};


	} //namespace res
} //namespace gos

#endif //_gosEngineRes2_h_