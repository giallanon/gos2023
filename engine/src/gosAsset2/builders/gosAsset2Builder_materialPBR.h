#ifndef _gosAsset2Builder_materialPBR_h_
#define _gosAsset2Builder_materialPBR_h_
#include "gosAsset2BuilderInterface.h"
#include "../assetFile/gosAssetFile_materialPBR.h"

/* Sintassi:

@materialPBR: runtimeName							=> il runtimeName e' opzionale come sempre
{
    (optional)  diffuse_col_RGBA_HDR: r,g,b,a
	(optional)  metallic_factor: value_0_1
}
*/

namespace gos
{
    namespace asset2
    {
        /**
         * @brief Builder_materialPBR
         *
         */
        class Builder_materialPBR: public BuilderInterface
        {
        public:
                    Builder_materialPBR ();
                    ~Builder_materialPBR()                                            { }

			bool 	build_begin (DBContext &ctx, const UniqueUIDList &listof_UID_of_known_ini_file, const char *absFilename, UID uid_of_iniFile, const gos::IniFileSection *sec);
			bool 	build_exe (DBContext &ctx, bool doCreateAnAssetFile, bool *out_bCallMeAgain, sBuildResult *out_result);
			void 	build_end()		{ }


        private:
            struct Params
            {
                AssetFile_materialPBR 	mat;
            };

        private:
            bool    priv_extractParams (DBContext &ctx, const UniqueUIDList &listof_UID_of_known_ini_file, const char *absFilename);
			
        private:
			Params 				params;
			UID 				uid_of_iniFile;
			const gos::IniFileSection *sec;

            
        }; //class Builder_materialPBR

    } //namespace asset2
} //namespace gos

#endif //_gosAsset2Builder_materialPBR_h_

