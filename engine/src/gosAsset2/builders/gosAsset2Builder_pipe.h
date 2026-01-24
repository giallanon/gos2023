#ifndef _gosAssetBuilder_pipe_h_
#define _gosAssetBuilder_pipe_h_
#include "gosAsset2BuilderInterface.h"
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
				
    (optional)	zb_allow_depthTestEnablingDisabling: 1          => serve per abilitare la possibilita' di settare al volo il depthTest
    (optional)	zb_allow_depthWriteEnablingDisabling: 1         => serve per abilitare la possibilita' di settare al volo il depthWrite


				
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
    namespace asset2
    {
        /********************************
         * @brief Builder_pipe
         *
         */
        class Builder_pipe : public BuilderInterface
        {
        public:
                    Builder_pipe () : BuilderInterface (eAssetType::pipe)		{ }
                    ~Builder_pipe()												{ }

			bool 	build_begin (DBContext &ctx, const UniqueUIDList &listof_UID_of_known_ini_file, const char *absFilename, UID uid_of_iniFile, const gos::IniFileSection *sec);
			bool 	build_exe (DBContext &ctx, bool doCreateAnAssetFile, bool *out_bCallMeAgain, sBuildResult *out_result);
			void 	build_end()	{ }


        private:
            struct Params
            {
                u32             magic;    //uso interno
                eCullMode       cullMode;
                eDrawPrimitive  drawPrimitive;
                u8              bWireframe;

                Flag8           zbuffer_flag;
                eImageFormat    zbuffer_format;
                eZFunc          zbuffer_cmpFn;

                u32             numRT;
                eImageFormat    renderTargetFormat[GOSGPU__NUM_MAX_ATTACHMENT];

                UID             uid__virtual_vtxshader;
                UID             uid__virtual_pxlshader;
            };

        private:
            bool    priv_extractParams ();
            bool    priv_do_create_assetFile (DBContext &ctx, UID uid_concrete_asset, const Params &params, const char *filenameDST) const;
            u32     priv_writePushConstant_rec (gos::BufferW_linear &buffer, gos::datablob::DefElem &elem) const;
            
		private:
			Params 	params;
			UID 	uid_of_iniFile;
			const gos::IniFileSection *sec;

        }; //class Builder_pipe

    } //namespace asset2
} //namespace gos

#endif //_gosAssetBuilder_pipe_h_

