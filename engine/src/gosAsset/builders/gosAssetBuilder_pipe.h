#ifndef _gosAssetBuilder_pipe_h_
#define _gosAssetBuilder_pipe_h_
#include "gosAssetBuilderInterface.h"
#include "../gosGPU/gosGPUEnumAndDefine.h"
#include "../gos/gosBufferWriter.h"
#include "../gos/gosDataBlob.h"

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
    (optional)	wireframe: 0|1                                                      => default: 0

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
         * @brief Builder_pipe
         *
         */
        class Builder_pipe : public BuilderInterface
        {
        public:
            /**
             * @brief   calc_dept e' mandatorio, va implementato in tutti i Builder.
                        Il <dept> indica quando profonda e' la descrizione testuale di questo asset.
                        Se l'asset non dipende da nessun altro asset, la sua dept e' 1  (vedi vtx_shader per esempio).
                        Se l'asset dipende da almeno un'altro asset, allora la sua dept e' 1 piu' la dept
                        piu' alta tra tutti gli asset da cui dipende*/
            static u32  calc_depth();

        public:
                    Builder_pipe () : BuilderInterface (eAssetType::pipe)                       { }
                    ~Builder_pipe()                                                             { }

            bool    build (Context &ctx, u64 buildTimeUTC, const char *sourceFileInfo, const asset::UID &uid_of_iniFile, const IniFileSection *sec, bool doCreateAnAssetFile, gos::GPU *gpu, sBuildResult *out);


        private:
            struct Params
            {
                u32             magic;    //uso interno
                eCullMode       cullMode;
                eDrawPrimitive  drawPrimitive;
                u8              bWireframe;

                u8              zbuffer_enabled;
                eImageFormat    zbuffer_format;
                bool            zbuffer_write;
                eZFunc          zbuffer_cmpFn;

                u32             numRT;
                eImageFormat    renderTargetFormat[GOSGPU__NUM_MAX_ATTACHMENT];

                asset::UID      uid_vtxshader;
                asset::UID      uid_pxlshader;
            };

        private:
            bool    priv_extractParams (const IniFileSection *sec, Params *out_params);
            bool    priv_do_create_assetFile (Context &ctx, const Params &params, const char *filenameDST) const;
            u32     priv_writePushConstant_rec (gos::BufferW_linear &buffer, gos::datablob::DefElem &elem) const;
            
        }; //class Builder_pipe

    } //namespace asset
} //namespace gos

#endif //_gosAssetBuilder_pipe_h_

