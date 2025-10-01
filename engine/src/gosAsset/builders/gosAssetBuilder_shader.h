#ifndef _gosAssetBuilder_shader_h_
#define _gosAssetBuilder_shader_h_
#include "gosAssetBuilderInterface.h"

/* Sintassi:

@vtx_shader: phong.vert.spv                 => il runtimeName e' opzionale come sempre
{
  src: phong/phong.vert.shader              => mandatorio
  def: parola1     parola2 parola3          => opzionale
}

Parametro <src>:    indica il nome del file di testo da utilizzare per compilare lo shader.
                    Le risorse shader sono nella cartella res/02-shader_txt
                    Sono validi anche nomi che includo path relativi a sottodir di res/02-shader_txt

Parametro <def>:    OPZIONALE. Se esiste, e' una lista di "define" che vengono passate al compilatore durante il build dello shader.
                    La lista e' un elenco di parola separate da spazio.
                    L'ordine con cui appaiono le parole non e' importante ("a" "b" "c"  vien considerato alla stessa stregua di "b" "a" "c")
*/

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
            /**
             * @brief   calc_depth e' mandatorio, va implementato in tutti i Builder.
                        Per una descrizione piu' accurata del significato, vedi gosAssetBuilder_pipedef
            */
            static u32  calc_depth()                                                                     { return 1; }

        public:
                        Builder_shader (eAssetType assTypeIN) : BuilderInterface (assTypeIN)            { }
                        ~Builder_shader()                                                               { }

            bool        build (Context &ctx, u64 buildTimeUTC, const char *sourceFileInfo, const asset::UID &uid_of_iniFile, const IniFileSection *sec, bool doCreateAnAssetFile, sBuildResult *out);
            

        private:
            struct Params
            {
                char        src[128];
                char        def[1024];
                asset::UID  uid__resource_shader_txt;
            };

        private:
            bool priv_extractParams (const IniFileSection *sec, Params *out_params);


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