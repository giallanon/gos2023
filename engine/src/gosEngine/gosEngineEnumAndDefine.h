#ifndef _gosEngineEnumAndDefine_h_
#define _gosEngineEnumAndDefine_h_
#include "../gos/gosHandle.h"
#include "../gos/gosFastArray.h"
#include "../gos/gosEZProfile.h"
#include "../gosGPU/gosGPU.h"
#include "../gosInput/gosInput.h"
#include "../gosAsset2/gosAsset2EnumAndDefine.h"
#include "../gosGeom/gosGeomCamera3.h"
#include "../gosShape/gosShape.h"
#include "../gosShape/skeleton/gosSkeleton.h"
#include "res2/res2.h"



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

	GOS_DECL_RES2_HANDLE(SkeletonH);
	typedef res2::Skeleton::Data		SkeletonData;
	
	GOS_DECL_RES2_HANDLE(ModelH);
	typedef res2::Model3d::Data			ModelData;
	typedef res2::Model3d::Updatable	ModelDataUpd;

	GOS_DECL_RES2_HANDLE(ModelInstH);
	typedef res2::ModelInstance::Data	ModelInstData;

}


#endif //_gosEngineEnumAndDefine_h_

