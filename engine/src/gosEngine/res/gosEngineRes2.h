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
	GOS_DECL_RES_HANDLE(ENGVtxBuffer);
	GOS_DECL_RES_HANDLE(ENGIdxBuffer);
	GOS_DECL_RES_HANDLE(ENGVtxShader);
	GOS_DECL_RES_HANDLE(ENGPxlShader);

	namespace res
	{
		//************************************
		struct VtxBuffer
		{
			res::Descr			_descr;
			GPUVtxBufferHandle	vbHandle;
		};

		//************************************
		struct IdxBuffer
		{
			res::Descr			_descr;
			GPUIdxBufferHandle	ibHandle;
		};

		struct Shader
		{
			res::Descr			_descr;
			GPUShaderHandle		shaderHandle;
		};

	} //namespace res
} //namespace gos

#endif //_gosEngineRes2_h_