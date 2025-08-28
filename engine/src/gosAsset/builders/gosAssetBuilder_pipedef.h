#ifndef _gosAssetBuilder_pipedef_h_
#define _gosAssetBuilder_pipedef_h_
#include "../gosAssetBuilderInterface.h"

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
                char param1[64];
                char param2[64];
            };

        public:
            static bool extractParams (const IniFileSection *sec, Params *out_params);

        public:
                    Builder_pipeDef () : BuilderInterface (eAssetType::pipeline_def)                { }
                    ~Builder_pipeDef()                                                              { }

            bool    build (Context &ctx, u64 buildTimeUTC, const IniFileSection *sec, sBuildResult *out);
            
        }; //class Builder_pipeDef

    } //namespace asset
} //namespace gos

#endif //_gosAssetBuilder_pipedef_h_

