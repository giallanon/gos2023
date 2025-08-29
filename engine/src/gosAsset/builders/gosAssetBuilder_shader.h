#ifndef _gosAssetBuilder_shader_h_
#define _gosAssetBuilder_shader_h_
#include "../gosAssetBuilderInterface.h"

namespace gos
{
    namespace asset
    {
        /**
         * @brief Builder_shader
         *
         */
        class Builder_shader : public BuilderInterface
        {
        public:
            struct Params
            {
                char        src[128];
                char        define[1024];
                asset::UID  uid__resource_shader_txt;
            };

        public:
            static bool shader_compile (const char *shaderSRCFile, const char *shaderStage, const char *spaceSeparateDefineList, const char *shaderDSTFile, bool bIncludeDebugInfo);
            static bool extractParams (const IniFileSection *sec, Params *out_params);

        public:
            /**
             * @brief   calc_depth e' mandatorio, va implementato in tutti i Builder.
                        Per una descrizione piu' accurata del significato, vedi gosAssetBuilder_pipedef
            */
            static u32  calc_depth()                                                                     { return 0; }

        public:
                        Builder_shader (eAssetType assTypeIN) : BuilderInterface (assTypeIN)            { }
                        ~Builder_shader()                                                               { }

            bool        build (Context &ctx, u64 buildTimeUTC, const char *sourceFileInfo, const asset::UID &uid_of_iniFile, const IniFileSection *sec, sBuildResult *out);
            
        }; //class Builder_shader



        /**
         * @brief Builder_vtxShader
         *
         */
        class Builder_vtxShader : public Builder_shader
        {
        public:
            static u32  calc_depth()                                                    { return Builder_shader::calc_depth(); }

        public:
                        Builder_vtxShader () : Builder_shader (eAssetType::vtx_shader)  { }
                        ~Builder_vtxShader()                                            { }
        }; //class Builder_vtxShader


        /**
         * @brief Builder_pxlShader
         *
         */
        class Builder_pxlShader : public Builder_shader
        {
        public:
            static u32  calc_depth()                                                    { return Builder_shader::calc_depth(); }

        public:
                    Builder_pxlShader () : Builder_shader (eAssetType::pxl_shader)      { }
                    ~Builder_pxlShader()                                                { }
        }; //class Builder_pxlShader

    } //namespace res
} //namespace gos


#endif //_gosAssetBuilder_shader_h_