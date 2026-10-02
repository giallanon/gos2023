#ifndef _gosEngineRes2_asset_h_
#define _gosEngineRes2_asset_h_
#include "res2.h"


namespace gos
{
	namespace res2
	{
		//************************************
		struct AssetShader
		{
			AssetDescr		base;
			ResShader::Data	data;
		};

		//************************************
		struct AssetPipeline
		{
		public:
			AssetDescr			base;
			ResPipeline::Data	data;
		};

		//************************************
		struct AssetTexture2d
		{
		public:
			AssetDescr			base;
			ResTexture2d::Data	data;
		};

		//************************************
		struct AssetShape
		{
			AssetDescr		base;
			ResShape::Data	data;
		};

		//************************************
		struct AssetSkeleton
		{
		public:
			AssetDescr			base;
			ResSkeleton::Data	skeleton;
		};		

		//************************************
		struct AssetMaterialPBR
		{
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