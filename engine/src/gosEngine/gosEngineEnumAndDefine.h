#ifndef _gosEngineEnumAndDefine_h_
#define _gosEngineEnumAndDefine_h_
#include "../gos/gosHandle.h"
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


#define GOS_DECL_PROXYRES_HANDLE(HANDLE_TYPE)\
struct HANDLE_TYPE\
{\
	u32	index;\
\
	void	setInvalid()							{ index = u32MAX; }\
	bool	isInvalid() const						{ return index == u32MAX; }\
	bool	isValid() const							{ return index != u32MAX; }\
};\


namespace gos
{
	struct ProxyRes
	{
		gos::res::Handle 	res_handle;	//questo "punta" alla risorsa SignatureUID associata a questo assetUID
		gos::asset2::UID	assetUID;
	};

	GOS_DECL_PROXYRES_HANDLE(ENGVtxBuffer2);
	GOS_DECL_PROXYRES_HANDLE(ENGIdxBuffer2);
	GOS_DECL_PROXYRES_HANDLE(ENGVtxShader2);
	GOS_DECL_PROXYRES_HANDLE(ENGPxlShader2);
	GOS_DECL_PROXYRES_HANDLE(ENGPipeline);
	GOS_DECL_PROXYRES_HANDLE(ENGTexture2);
	

	//GOS_DECL_RES_HANDLE(ENGVtxBuffer2);
	//GOS_DECL_RES_HANDLE(ENGIdxBuffer2);
	//GOS_DECL_RES_HANDLE(ENGVtxShader2);
	//GOS_DECL_RES_HANDLE(ENGPxlShader2);
	//GOS_DECL_RES_HANDLE(ENGPipeline);
	//GOS_DECL_RES_HANDLE(ENGTexture);
	GOS_DECL_RES_HANDLE(ENGShape);
	GOS_DECL_RES_HANDLE(ENGGPUShape);
	GOS_DECL_RES_HANDLE(ENGSkeleton);
	GOS_DECL_RES_HANDLE(ENGModel3d);
	GOS_DECL_RES_HANDLE(ENGModel3dInst);
	GOS_DECL_RES_HANDLE(ENGMaterialPBR);
	


} //namespace gos


#endif //_gosEngineEnumAndDefine_h_

