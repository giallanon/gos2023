#ifndef _gosAsset2Builder_tex2D_h_
#define _gosAsset2Builder_tex2D_h_
#include "gosAsset2BuilderInterface.h"
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
    namespace asset2
    {
        /**
         * @brief Builder_tex2D
         *
         */
        class Builder_tex2D : public BuilderInterface
        {
        public:
                    Builder_tex2D (Builder *theBuilderIN);
                    ~Builder_tex2D()                                                            { }

            void    initOnce (gos::GPU *gpuIN);
            void    deinitOnce();
            bool    build (DBContext &ctx, u64 buildTime_UTC, const char *absFilename, UID uid_of_iniFile, const gos::IniFileSection *sec, bool doCreateAnAssetFile, sBuildResult *out_result);


        private:
            struct Params
            {
                char            src[512];
                u16             srcIs_sRGB;
                u16             dstNumMipMap;
                eImageFormat    dstFmt;
                UID             uid__resource_image;
                UID             uid__concrete_asset;
            };

        private:
            bool    priv_extractParams (const char *absFilename, const IniFileSection *sec, Params *out_params);
            bool    priv_do_create_assetFile (DBContext &ctx, UID uid_concrete_asset, const Params &params, const char *filenameDST);
            bool    priv_create_GPUResourceOnce();
            bool    priv_save (const gpu::sMappedImage &src, gos::image::Builder &builder, eImageFormat dstFmt, u32 srcW, u32 srcH, u32 mipMapNum_0toN, u32 numPallini);

        private:
            gos::GPU            *gpu;
            GPUSamplerHandle    samplerHandle;
            GPUPipelineHandle   pipeHandle;
            GPUDescrPoolHandle  descrPoolHandle;
            GPUDescrSetInstanceHandle descrSetInstanceHandle;

            
        }; //class Builder_tex2D

    } //namespace asset2
} //namespace gos

#endif //_gosAsset2Builder_tex2D_h_

