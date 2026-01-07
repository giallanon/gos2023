#ifndef _gosAsset2Builder_shader_h_
#define _gosAsset2Builder_shader_h_
#include "gosAsset2BuilderInterface.h"

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
    namespace asset2
    {
        /*********************************************
         * @brief Builder_shader
         *
         */
        class Builder_shader : public BuilderInterface
        {
        public:
                        Builder_shader (Builder *theBuilderIN, eAssetType assTypeIN) : BuilderInterface (theBuilderIN, assTypeIN)            { }
                        ~Builder_shader()                                                               { }

            bool        build (DBContext &ctx, u64 buildTime_UTC, const char *absFilename, UID uid_of_iniFile, const gos::IniFileSection *sec, bool doCreateAnAssetFile, sBuildResult *out_result);
            

        private:
            struct Params
            {
                char    src[1024];
                char    def[1024];
                UID     uid__resource_shader_txt;
            };

        private:
            bool priv_extractParams (const char *absFilename, const IniFileSection *sec, Params *out_params);


        }; //class Builder_shader



        /*********************************************
         * @brief Builder_vtxShader
         *
         */
        class Builder_vtxShader : public Builder_shader
        {
        public:
                        Builder_vtxShader (Builder *theBuilderIN) : Builder_shader (theBuilderIN, eAssetType::vtx_shader)  { }
                        ~Builder_vtxShader()                                            { }
        }; //class Builder_vtxShader


        /*********************************************
         * @brief Builder_pxlShader
         *
         */
        class Builder_pxlShader : public Builder_shader
        {
        public:
                    Builder_pxlShader (Builder *theBuilderIN) : Builder_shader (theBuilderIN, eAssetType::pxl_shader)      { }
                    ~Builder_pxlShader()                                                { }
        }; //class Builder_pxlShader

    } //namespace res
} //namespace gos


#endif //_gosAsset2Builder_shader_h_