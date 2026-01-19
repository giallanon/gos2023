#ifndef _gosAssetBuilder_glb_h_
#define _gosAssetBuilder_glb_h_
#include "gosAsset2BuilderInterface.h"
#include "../gosGPU/gosGPUEnumAndDefine.h"

/* Sintassi:

@import-glb: <rtname>    => il runtimeName e' mandatorio
{
    (mandatory) src: ...xxx.glb                         => il modello 3d da importare
    (optional)	scale: [varie opzioni]
                        uniform-resize-y; <number>      => dato AABB del modello, riscala il modello in maniera uniforme affinchè la dimy di AABB sia esattamente uguale a <number>
                        uniform-resize-x; <number>
                        uniform-resize-z; <number>
                        
    (optional)	translate: [varie opzioni]
                            center-at; <x>; <y>; <z>            => dato AABB del modello, muove il centro dell'AABB alle coordinate x,y,z
                            bottom-center-at; <x>; <y>; <z>     => dato AABB del modello, muove il centro della faccia bottom dell'AABB alle coordinate x,y,z
}
Questo genera:
    N asset di tipo shape, 
    M asset di tipo Material (che a sua volta possono riferire ad asset di tipo Texture)
    J asset di tipo Skeleton


*/

namespace gos
{
    namespace asset2
    {
        /********************************
         * @brief Builder_glb
         *
         */
        class Builder_glb : public BuilderInterface
        {
        public:
                    Builder_glb () : BuilderInterface (eAssetType::imported_glb)        { }
                    ~Builder_glb()                                                      { }

            bool    build (DBContext &ctx, u64 buildTime_UTC, const UniqueUIDList &listof_UID_of_known_ini_file, const char *absFilename, UID uid_of_iniFile, const gos::IniFileSection *sec, bool doCreateAnAssetFile, sBuildResult *out_result);


        private:
            struct Params
            {
                char            src[512];
                UID             uid__resource_file_glb;
            };

        private:
            bool    priv_extractParams (DBContext &ctx, const UniqueUIDList &listof_UID_of_known_ini_file, const char *absFilename, const IniFileSection *sec, Params *out_params);
            bool    priv_do_create_assetFile (DBContext &ctx, UID uid_concrete_asset, const Params &params, const char *filenameDST);
            
        }; //class Builder_glb

    } //namespace asset2
} //namespace gos

#endif //_gosAssetBuilder_glb_h_

