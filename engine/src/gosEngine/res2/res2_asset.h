#ifndef _gosEngineRes2_asset_h_
#define _gosEngineRes2_asset_h_
#include "../../gosGPU/gosGPU.h"
#include "../gosShape/gosShape.h"
#include "../gosShape/skeleton/gosSkeleton.h"

namespace gos
{
	namespace res2
	{
		//************************************
		struct AssetVtxShader
		{
			static constexpr eAssetType type = eAssetType::vtx_shader;

			AssetDescr			base;
			ResVtxShader::Data	data;
		};

		//************************************
		struct AssetPxlShader
		{
			static constexpr eAssetType type = eAssetType::pxl_shader;

			AssetDescr			base;
			ResPxlShader::Data	data;
		};		

		//************************************
		struct AssetPipeline
		{
			static constexpr eAssetType type = eAssetType::pipeline;

			AssetDescr			base;
			ResPipeline::Data	data;
		};

		//************************************
		struct AssetTexture2d
		{
			static constexpr eAssetType type = eAssetType::texture_2d;

			AssetDescr			base;
			ResTexture2d::Data	data;
		};

		//************************************
		struct AssetShape
		{
			static constexpr eAssetType type = eAssetType::shape;

			AssetDescr		base;
			ResShape::Data	data;
		};

		//************************************
		struct AssetSkeleton
		{
			static constexpr eAssetType type = eAssetType::skeleton;

			AssetDescr			base;
			ResSkeleton::Data	skeleton;
		};		

		//************************************
		struct AssetMaterialPBR
		{
			static constexpr eAssetType type = eAssetType::materialPBR;

			struct Data
			{
				gos::vec4f			diffuse_col_HDR_RGBA;
				ResTexture2d::Data	diffuse_tex;
			};

			AssetDescr	base;
			Data		data;
		};		

		/************************************
		struct AssetModel3d
		{
			struct Data
			{
				gos::Model		model;
			};

			AssetDescr	base;
			Data		data;
		};		
		*/

		/************************************
		struct AssetModel3dInst
		{
			struct Data
			{
				gos::ModelInstance	minst;
				mat4x4f 			matW;
			};

			AssetDescr	base;
			Data		data;
		};
		*/

	} //namespace res2
} //namespace gos

#endif //_gosEngineRes2_asset_h_