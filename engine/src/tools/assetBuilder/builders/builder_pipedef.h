#ifndef _gosAssetBuilder_pipedef_h_
#define _gosAssetBuilder_pipedef_h_
#include "gosAssetBuilderInterface.h"

namespace gos
{
    namespace asset
    {
        /**
         * @brief Builder_pipeDef
         *
         */
        class Builder_pipeDef : public BuilderInterface
        {
        public:
            struct Params
            {
                char        param1[64];
                char        param2[64];
                asset::UID  uid_vtxshader;
                asset::UID  uid_pxlshader;
            };

        public:
            static bool extractParams (const IniFileSection *sec, Params *out_params);

            /**
             * @brief   calc_dept e' mandatorio, va implementato in tutti i Builder.
                        Il <dept> indica quando profonda e' la descrizione testuale di questo asset.
                        Se l'asset non dipende da nessun altro asset, la sua dept e' 0  (vedi vtx_shader per esempio).
                        Se l'asset dipende da almeno un'altro asset, allora la sua dept e' 1 piu' la dept
                        piu' alta tra tutti gli asset da cui dipende*/
            static u32  calc_depth();

        public:
                    Builder_pipeDef () : BuilderInterface (eAssetType::pipeline_def)                { }
                    ~Builder_pipeDef()                                                              { }

            bool    build (Context &ctx, u64 buildTimeUTC, const char *sourceFileInfo, const asset::UID &uid_of_iniFile, const IniFileSection *sec, bool doCreateAnAssetFile, sBuildResult *out);


        private:
            bool    priv_do_create_assetFile (Context &ctx, const Params &params) const;
            
        }; //class Builder_pipeDef

    } //namespace asset
} //namespace gos

#endif //_gosAssetBuilder_pipedef_h_

