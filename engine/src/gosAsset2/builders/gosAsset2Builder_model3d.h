#ifndef _gosAssetBuilder_model3d_h_
#define _gosAssetBuilder_model3d_h_
#include "gosAsset2BuilderInterface.h"
#include "../gosGPU/gosGPUEnumAndDefine.h"
#include "../../gos/string/gosUTF8String.h"
#include "../../gosShape/skeleton/gosSkeleton.h"

/* Sintassi:

@model3d: <rtname>    => il runtimeName e' opzionale come sempre
{
	[shape]: <rtname-of-imported-3dmodel>.*		=> importa tutte le shape del modello src

		OPPURE

    [shape]:  <rtname-of-imported-3dmodel>.<name> as <my-shape-name>    => definisce una shape di nome <my-shape-name> presa dalla shape di nome <name> di un determinato <imported-3dmodel>
	...																   <my-shape-name> e' opzionale e, se manca, la shape assume il nome <name>
    [shape]: ...



    [material]: <rtname-of-imported-3dmodel>.<material-name> as <my-material-name>
    ...
    [material]: ...



    skeleton: none                          => vuol dire uno skeleton di default consistente del solo nodo root automaticamente generato
              <rtname-of-imported-3dmodel>	=> lo sk di un determinato <imported-3dmodel>



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

        }; //class Builder_model3d

    } //namespace asset2
} //namespace gos

#endif //_gosAssetBuilder_model3d_h_

