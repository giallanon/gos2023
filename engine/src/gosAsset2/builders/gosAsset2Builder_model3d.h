#ifndef _gosAssetBuilder_model3d_h_
#define _gosAssetBuilder_model3d_h_
#include "gosAsset2BuilderInterface.h"
#include "../gosGPU/gosGPUEnumAndDefine.h"
#include "../../gos/string/gosUTF8String.h"
#include "../../gosShape/skeleton/gosSkeleton.h"
#include "gosAsset2Builder_glb_importer.h"

/* Sintassi:


######## sintassi 1 ##########
@model3d: <rtname>    => il runtimeName e' opzionale come sempre
{
	import: ...xxx.glb                         => il modello 3d da importare

	(optional) post_op:  <operation>,<param1>, ... , <paramN> ;		=> un elenco di <operation> da eseguire post importazione
						 ....										=> le <operation> sono separate da ;  i parametri di una <operation>
						 <operation>,<param1>, ... , <paramN>		=> sono separti da ,

		Le <operation> supportate sono:
			skeleton-resolve			=> risolve lo skeleton riposizionando tutte le shape. Lo skeleton viene eliminato (consiste della solla root bone)

			uniform-resize-x, <val>		=> scala uniformemente in modo che AABB.dimx == <val>
			uniform-resize-y, <val>		=> scala uniformemente in modo che AABB.dimy == <val>
			uniform-resize-z, <val>		=> scala uniformemente in modo che AABB.dimz == <val>
	
			center-at, <x>, <y>, <z>					=> trasla affinche' il centro dell'AABB sia in <x,y,z>
			top-center-at, <x>, <y>, <z>
			bottom-center-at, <x>, <y>, <z>
			top-bottom-left-corner-at, <x>, <y>, <z>	=> il vtx in basso a sx della faccia top viene posizionato in <x,y,z>
}

Questo genera:
	1 asset di tipo model3d che rispecchia esattamente il modello importato e:
	- N asset di tipo shape, 
	- J asset di tipo Skeleton
	- TODO: M asset di tipo Material (che a sua volta possono riferire ad asset di tipo Texture)



######## sintassi 2 ##########
@model3d: <rtname>    => il runtimeName e' opzionale come sempre
{
	[shape]:  <rtname-of-existing-model3d>.<name> as <my-shape-name>    =>	definisce una shape di nome <my-shape-name> presa dalla shape di nome <name> di un determinato <model3d> importato tramiote la sintasi #1
	...																		<my-shape-name> e' opzionale e, se manca, la shape assume il nome <name>
	[shape]: ...



	[material]: <rtname-of-existing-model3d>.<material-name> as <my-material-name>
	...
	[material]: ...



	skeleton: none                          => vuol dire uno skeleton di default consistente del solo nodo root automaticamente generato
			  <rtname-of-existing-model3d>	=> lo sk di un determinato <model3d> importato tramite la sintasi #1



	[mesh]: <my-shape-name>;<my-material-name>;<bone-name>;<local-transform-matrix3x3>
	...
	[mesh]: ...

}

*/

namespace gos
{
	namespace asset2
	{
		/********************************
		 * @brief Builder_model3d
		 *
		 */
		class Builder_model3d : public BuilderInterface
		{
		public:
					Builder_model3d ();
					~Builder_model3d();

			bool 	build_begin (DBContext &ctx, const UniqueUIDList &listof_UID_of_known_ini_file, const char *absFilename, UID uid_of_iniFile, const gos::IniFileSection *sec);
			bool 	build_exe (DBContext &ctx, bool doCreateAnAssetFile, bool *out_bCallMeAgain, sBuildResult *out_result);
			void 	build_end();

		private:
			/**********************************************
			* @brief	Syntax1
			*			Classe di comodo per la gestione della "sintassi 1"
			* 
			*/
			class Syntax1 : public BuilderInterface
			{
			public:
						Syntax1 ();
						~Syntax1();

				bool 	build_begin (DBContext &ctx, const UniqueUIDList &listof_UID_of_known_ini_file, const char *absFilename, UID uid_of_iniFile, const gos::IniFileSection *sec);
				bool 	build_exe (DBContext &ctx, bool doCreateAnAssetFile, bool *out_bCallMeAgain, sBuildResult *out_result);
				void 	build_end();

			private:
				enum class eWhatToBuild : u8
				{
					shapes = 0,
					skeleton = 1,
					materials = 2,
					end = 0xff
				};

				struct Params
				{
					char            import_name[512];
					UID             uid__resource_file_glb;
					eAssetType		subresource_type;
					u32 			subresource_index;
				};

				struct sBuildCtx
				{
					bool 			bAModelWasImported;
					gos::VtxLayout 	vtxLayout;
					Importer_glb::Result   imported;
					eWhatToBuild	whatToBuild;
					u32				iToBuild;
				};

			private:
				bool	priv_extractParams (DBContext &ctx, const UniqueUIDList &listof_UID_of_known_ini_file, const char *absFilename);
				bool 	priv_build_shape (DBContext &ctx, bool doCreateAnAssetFile, sBuildResult *out_result);
				bool 	priv_build_skeleton (DBContext &ctx, bool doCreateAnAssetFile, sBuildResult *out_result);
				bool 	priv_build_material (DBContext &ctx, bool doCreateAnAssetFile, sBuildResult *out_result);
				void 	priv_print_report(const char *filenameDST) const;
				bool 	priv_apply_post_op();

			private:
				gos::Allocator				*localAllocator;
				Params 						params;
				UID 						uid_of_iniFile;
				const gos::IniFileSection 	*sec;
				char 						glb_rtname[128];
				sBuildCtx					buildCtx;

				UID 						uid_of_concrete_model3d;
				UID 						uid_of_virtual_model3d;
				UID							uid_of_concrete_skeleton;
				FastArray<UID>				listof_uid_of_concreste_shape;
				FastArray<UID>				listof_uid_of_concrete_material;

			}; //class Sintax1


			/**********************************************
			* @brief	Syntax2
			*			Classe di comodo per la gestione della "sintassi 2"
			* 
			*/
			class Syntax2 : public BuilderInterface
			{
			public:
						Syntax2 ();
						~Syntax2();

				bool 	build_begin (DBContext &ctx, const UniqueUIDList &listof_UID_of_known_ini_file, const char *absFilename, UID uid_of_iniFile, const gos::IniFileSection *sec);
				bool 	build_exe (DBContext &ctx, bool doCreateAnAssetFile, bool *out_bCallMeAgain, sBuildResult *out_result);
				void 	build_end()		{ }

			private:
				struct sShapeInfo
				{
							sShapeInfo()		{ my_shape_name = src_shape_name = NULL; }
					char	*my_shape_name;
					char 	*src_shape_name;
					UID		uid_of_concrete_shape_asset;
				};

				struct sMeshInfo
				{
							sMeshInfo()			{ my_shape_index = my_material_index = u32MAX; bone_name = NULL; }
					u32 	my_shape_index;
					u32 	my_material_index;
					char 	*bone_name;
				};			

				struct ParsedParams
				{
					FastArray<sShapeInfo>	listof_shapeInfo;
					FastArray<sMeshInfo>	listof_meshes;
					char					*skeleton_name;
				};


				struct sFinalMeshInfo
				{
							sFinalMeshInfo()			{ my_shape_index = my_material_index = bone_index = u32MAX;}
					u32 	my_shape_index;
					u32 	my_material_index;
					u32 	bone_index;
				};

			private:
				void 	priv_reset_parsed_params();
				bool    priv_extractParams (DBContext &ctx, const UniqueUIDList &listof_UID_of_known_ini_file, const char *absFilename);
				bool 	priv_do_create_assetFile (DBContext &ctx, UID uid_concrete_asset, const char *filenameDST, const FastArray<sFinalMeshInfo> &listof_final_meshes) const;

			private:
				gos::Allocator				*localAllocator;
				ParsedParams				parsed_params;
				UID 						uid_of_iniFile;
				const gos::IniFileSection 	*sec;

				UniqueUIDList				listof_UID_of_virtual_shape_that_I_need;
				FastArray<asset2::UID>		listof_UID_of_concrete_shape_that_I_need;
				UID							uid_of_virtual_skeleton;
				UID							uid_of_concrete_skeleton;

			}; //class Sintax2

		private:
			Syntax1		builder1;
			Syntax2		builder2;
			u8			which_one;
		};
	} //namespace asset2
} //namespace gos

#endif //_gosAssetBuilder_model3d_h_

