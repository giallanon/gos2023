#ifndef _gosAssetBuilder_DEBUG_ASSET_h_
#define _gosAssetBuilder_DEBUG_ASSET_h_
#include "gosAssetBuilderInterface.h"


/* Sintassi:

@DEBUG_ASSET: <runtimeName>                            => il runtimeName e' opzionale come sempre
{
    @vtx_shader:
    @pipeline_def:
}

*/

namespace gos
{
    namespace asset
    {
        /**
         * @brief Builder_DEBUG_ASSET
         *
         */
        class Builder_DEBUG_ASSET : public BuilderInterface
        {
        public:
            /**
             * @brief   calc_dept e' mandatorio, va implementato in tutti i Builder.
                        Il <dept> indica quando profonda e' la descrizione testuale di questo asset.
                        Se l'asset non dipende da nessun altro asset, la sua dept e' 0  (vedi vtx_shader per esempio).
                        Se l'asset dipende da almeno un'altro asset, allora la sua dept e' 1 piu' la dept
                        piu' alta tra tutti gli asset da cui dipende*/
            static u32  calc_depth();

        public:
                    Builder_DEBUG_ASSET () : BuilderInterface (eAssetType::DEBUG_ASSET)                { }
                    ~Builder_DEBUG_ASSET()                                                              { }

            bool    build (Context &ctx, u64 buildTimeUTC, const char *sourceFileInfo, const asset::UID &uid_of_iniFile, const IniFileSection *sec, bool doCreateAnAssetFile, sBuildResult *out);


        private:
            struct Params
            {
                asset::UID      uid_vtxshader;
                asset::UID      uid_pipedef;
            };

        private:
            bool    priv_extractParams (const IniFileSection *sec, Params *out_params);
            bool    priv_do_create_assetFile (Context &ctx, const Params &params, const char *filenameDST) const;
            
        }; //class Builder_pipe

    } //namespace asset
} //namespace gos
#endif //_gosAssetBuilder_DEBUG_ASSET_h_