#ifndef _gosResShaderBuilder_h_
#define _gosResShaderBuilder_h_
#include "gosResBuilder.h"

namespace gos
{
    namespace res
    {
        /**
         * @brief VtxhaderBuilder
         *
         */
        class ShaderBuilder : public IResBuilder
        {
        public:
                    ShaderBuilder (eResType resType) : IResBuilder (resType)            { }
                    ~ShaderBuilder()                                                    { }

            bool    build (sBuilderSession &session, const IniFileSection *sec, u64 lastTimeIniSectionWasUpdate);

            bool    calc_resUID (const IniFileSection *sec, u32 *out_resUID) const;

        private:
            struct Params
            {
                char src[128];
                char define[1024];
            };

            struct sData
            {
                char    runtimeName[128];
                Params  params;
            };

        private:
            bool    priv_parseSection (const IniFileSection *sec, sData *out) const;
            u32     priv_calc_resUID (const Params &params) const;
            bool    priv_do_build (sBuilderSession &session, const IniFileSection *sec, u64 lastTimeIniSectionWasUpdate);

            
        }; //class VtxShaderBuilder



        /**
         * @brief VtxhaderBuilder
         *
         */
        class VtxShaderBuilder : public ShaderBuilder
        {
        public:
                    VtxShaderBuilder () : ShaderBuilder (eResType::vtx_shader)      { }
                    ~VtxShaderBuilder()                                             { }
        }; //class VtxShaderBuilder


        /**
         * @brief PxlShaderBuilder
         *
         */
        class PxlShaderBuilder : public ShaderBuilder
        {
        public:
                    PxlShaderBuilder () : ShaderBuilder (eResType::pxl_shader)      { }
                    ~PxlShaderBuilder()                                             { }
        }; //class PxlShaderBuilder

    } //namespace res
} //namespace gos


#endif //_gosResShaderBuilder_h_