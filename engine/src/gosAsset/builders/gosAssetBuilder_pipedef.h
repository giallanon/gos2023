#ifndef _gosAssetBuilder_pipedef_h_
#define _gosAssetBuilder_pipedef_h_
#include "../gosAssetBuilderInterface.h"
#include "../gosGPU/gosGPUEnumAndDefine.h"

/* Sintassi:

@pipeline_def: pipeline1                            => il runtimeName e' opzionale come sempre
{
	//max 16 render target (GOSGPU__NUM_MAX_ATTACHMENT=16)
	(optional)	rt0: <ImgFormat = U8_RGBA | ... >
	...
	(optional)	rt15: <ImgFormat = U8_RGBA | ... >


	//optional,	DEFAULT => zb: _DEPTH_BEST, 1, LESS
	(optional)	zb: <imgFormat = BEST | ...>, <zwrite = 0|1>, <zcmpFn = LESS|...>
					oppure
				zb: none
				
				
	(optional)	cullMode: CCW					=> <mode>							=> default: eCullMode::CCW
	(optional)	drawPrimitive: trisList			=> <primitive>						=> default: trisList

	(optional)	@vtx_shader
	(optional)	@pxl_shader
	
	//se esiste <vtx_shader>, da questo si ricava il vtxFormat
	//da <vtx_shader> e <pxl_shader> si ricavano i descrittori e le push constant
}

*/

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
                u32             magic;    //uso interno
                eCullMode       cullMode;
                eDrawPrimitive  drawPrimitive;

                u8              zbuffer_enabled;
                eImageFormat    zbuffer_format;
                bool            zbuffer_write;
                eZFunc          zbuffer_cmpFn;

                u32             numRT;
                eImageFormat    renderTargetFormat[GOSGPU__NUM_MAX_ATTACHMENT];

                asset::UID      uid_vtxshader;
                asset::UID      uid_pxlshader;
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
            bool    priv_do_create_assetFile (Context &ctx, const Params &params, const char *filenameDST) const;
            
        }; //class Builder_pipeDef

    } //namespace asset
} //namespace gos

#endif //_gosAssetBuilder_pipedef_h_

