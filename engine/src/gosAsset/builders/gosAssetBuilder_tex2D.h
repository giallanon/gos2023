#ifndef _gosAssetBuilder_tex2D_h_
#define _gosAssetBuilder_tex2D_h_
#include "gosAssetBuilderInterface.h"
#include "../gosGPU/gosGPUEnumAndDefine.h"
#include "../gosImage/gosImageBuilder.h"

/* Sintassi:

@tex2D: runtimeName                                => il runtimeName e' opzionale come sempre
{
	(mandatory) src: godus_02.png
    (optional)  srcColorSpace: RGB|sRGB             => default "sRGB" => indica il color space dell'immagine sorgente

	(optional)  dstNumMipMap: max|<number >= 1>     => default = "max" => indica il num di mipmap totali (compreso il livello 0)
	(mandatory) dstFmt:U8_RGBA|U8_RGB|U8_R
}
*/

namespace gos
{
    namespace asset
    {
        /**
         * @brief Builder_tex2D
         *
         */
        class Builder_tex2D : public BuilderInterface
        {
        public:
            /**
             * @brief   calc_depth e' mandatorio, va implementato in tutti i Builder.
                        Per una descrizione piu' accurata del significato, vedi gosAssetBuilder_pipedef
            */
            static u32  calc_depth()                                                            { return 1; }

        public:
                    Builder_tex2D ();
                    ~Builder_tex2D()                                                            { }

            void    initOnce (gos::GPU *gpuIN);
            void    deinitOnce();
            bool    build (Context &ctx, u64 buildTimeUTC, const char *sourceFileInfo, const asset::UID &uid_of_iniFile, const IniFileSection *sec, bool doCreateAnAssetFile, sBuildResult *out);


        private:
            struct Params
            {
                char            src[128];
                u16             srcIs_sRGB;
                u16             dstNumMipMap;
                eImageFormat    dstFmt;
                asset::UID      uid__resource_image;
            };

        private:
            bool    priv_extractParams (const IniFileSection *sec, Params *out_params);
            bool    priv_do_create_assetFile (Context &ctx, const Params &params, const char *filenameDST);
            bool    priv_create_GPUResourceOnce();
            bool    priv_save (const gpu::sMappedImage &src, gos::image::Builder &builder, eImageFormat dstFmt, u32 srcW, u32 srcH, u32 mipMapNum_0toN, u32 numPallini);

        private:
            gos::GPU            *gpu;
            GPUSamplerHandle    samplerHandle;
            GPUPipelineHandle   pipeHandle;
            GPUDescrPoolHandle  descrPoolHandle;
            GPUDescrSetInstanceHandle descrSetInstanceHandle;

            
        }; //class Builder_tex2D

    } //namespace asset
} //namespace gos

#endif //_gosAssetBuilder_tex2D_h_

