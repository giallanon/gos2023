#ifndef _gosEngineEnumAndDefine_h_
#define _gosEngineEnumAndDefine_h_
#include "../gos/gosFastArray.h"
#include "../gosGPU/gosGPU.h"
#include "../gosInput/gosInput.h"
#include "../gosAsset2/gosAsset2EnumAndDefine.h"
#include "../gosGeom/gosGeomCamera3.h"
#include "../gosShape/gosShape.h"
#include "../gosShape/skeleton/gosSkeleton.h"
#include "res/gosEngineResEnumAndDefine.h"


#define GOS_DECL_RES_HANDLE(HANDLE_TYPE)\
struct HANDLE_TYPE\
{\
	gos::res::Handle res_handle;\
\
	void	setInvalid()							{ res_handle.setInvalid(); }\
	bool	isInvalid() const						{ return res_handle.isInvalid(); }\
	bool	isValid() const							{ return res_handle.isValid(); }\
\
	int		compare (const HANDLE_TYPE b) const		{ return res_handle.compare(b.res_handle); }\
	bool	operator== (const HANDLE_TYPE b) const	{ return (res_handle == b.res_handle); }\
	bool	operator!= (const HANDLE_TYPE b) const	{ return (res_handle != b.res_handle); }\
\
	void	setFromU32 (u32 u)						{ res_handle.setFromU32(u); }\
	u32		viewAsU32() const						{ return res_handle.viewAsU32(); }\
};\


namespace gos
{
	GOS_DECL_RES_HANDLE(ENGVtxBuffer);
	GOS_DECL_RES_HANDLE(ENGIdxBuffer);
	GOS_DECL_RES_HANDLE(ENGVtxShader);
	GOS_DECL_RES_HANDLE(ENGPxlShader);
	GOS_DECL_RES_HANDLE(ENGPipeline);
	GOS_DECL_RES_HANDLE(ENGTexture);
	GOS_DECL_RES_HANDLE(ENGShape);
	GOS_DECL_RES_HANDLE(ENGGPUShape);
	GOS_DECL_RES_HANDLE(ENGSkeleton);
	GOS_DECL_RES_HANDLE(ENGModel3d);
	GOS_DECL_RES_HANDLE(ENGModel3dInst);
	
} //namespace gos


#endif //_gosEngineEnumAndDefine_h_

