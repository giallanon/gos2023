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
#include "res2/res2.h"


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
\
	int		compare (const HANDLE_TYPE b) const		{ if (index == b.index) return 0; if (index > b.index) return 1; return -1; }\
};\

namespace gos
{
	struct ProxyRes
	{
		gos::res::Handle 	res_handle;	//questo "punta" alla risorsa SignatureUID associata a questo assetUID
		gos::asset2::UID	assetUID;
	};

	//GOS_DECL_PROXYRES_HANDLE(ENGVtxBuffer2);
	//GOS_DECL_PROXYRES_HANDLE(ENGIdxBuffer2);
	GOS_DECL_PROXYRES_HANDLE(ENGVtxShader2);
	GOS_DECL_PROXYRES_HANDLE(ENGPxlShader2);
	GOS_DECL_PROXYRES_HANDLE(ENGPipeline);
	//GOS_DECL_PROXYRES_HANDLE(ENGTexture2);
	GOS_DECL_PROXYRES_HANDLE(ENGShape);
	//GOS_DECL_PROXYRES_HANDLE(ENGGPUShape);
	GOS_DECL_PROXYRES_HANDLE(ENGSkeleton);
	GOS_DECL_PROXYRES_HANDLE(ENGModel3d);
	//GOS_DECL_PROXYRES_HANDLE(ENGModel3dInst);
	GOS_DECL_PROXYRES_HANDLE(ENGMaterialPBR);	

	//GOS_DECL_RES_HANDLE(ENGVtxBuffer2);
	//GOS_DECL_RES_HANDLE(ENGIdxBuffer2);
	//GOS_DECL_RES_HANDLE(ENGVtxShader2);
	//GOS_DECL_RES_HANDLE(ENGPxlShader2);
	//GOS_DECL_RES_HANDLE(ENGPipeline);
	//GOS_DECL_RES_HANDLE(ENGTexture);
	//GOS_DECL_RES_HANDLE(ENGShape);
	GOS_DECL_RES_HANDLE(ENGGPUShape);
	//GOS_DECL_RES_HANDLE(ENGSkeleton);
	//GOS_DECL_RES_HANDLE(ENGModel3d);
	GOS_DECL_RES_HANDLE(ENGModel3dInst);
	//GOS_DECL_RES_HANDLE(ENGMaterialPBR);
} //namespace gos



#define GOS_DECL_RES2_HANDLE(HANDLE_TYPE)\
struct HANDLE_TYPE: public res2::ResHandle\
{\
	bool        operator== (const HANDLE_TYPE &b) const                 { return viewAsU32() == b.viewAsU32(); }	\
};\


namespace gos
{
	enum class eRes2LoadResult : u8
	{
		success = 0,
		fail = 1,
		pending = 2
	};

	GOS_DECL_RES2_HANDLE(VtxBufferH);
	typedef res2::VtxBuffer::Data		VtxBufferData;

	GOS_DECL_RES2_HANDLE(IdxBufferH);
	typedef res2::IdxBuffer::Data		IdxBufferData;

	GOS_DECL_RES2_HANDLE(TextureH);
	typedef res2::Texture2d::Data		TextureData;

	GOS_DECL_RES2_HANDLE(MaterialPBRH);
	typedef res2::MaterialPBR::Data			MaterialPBRData;
	typedef res2::MaterialPBR::Updatable	MaterialPBRDataUpd;

	GOS_DECL_RES2_HANDLE(VtxShaderH);
	typedef res2::VtxShader::Data		VtxShaderData;

	GOS_DECL_RES2_HANDLE(PxlShaderH);
	typedef res2::PxlShader::Data		PxlShaderData;

	GOS_DECL_RES2_HANDLE(PipelineH);
	typedef res2::Pipeline::Data		PipelineData;
	
	GOS_DECL_RES2_HANDLE(GPUShapeH);
	typedef res2::GPUShape::Data		GPUShapeData;	

	// GOS_DECL_RES2_HANDLE(ShapeH);
	// typedef res2::Shape::Data			ShapeData;	
}


#endif //_gosEngineEnumAndDefine_h_

