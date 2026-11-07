#ifndef _gosAsset2Builder_tex2D_h_
#define _gosAsset2Builder_tex2D_h_
#include "gosAsset2BuilderInterface.h"
#include "../gosGPU/gosGPUEnumAndDefine.h"
#include "../gosImage/gosImageBuilder.h"
#include "../gosImage/gosImageBufferRGBA.h"

/* Sintassi:

@tex2D: runtimeName                                => il runtimeName e' opzionale come sempre
{
	(mandatory) src: godus_02.png					=> il file "godus_02.png"
					 (oppure)
					 filename:offset:size			=> l'immagine src e' contenuta nel file <filename> a partire dal <offset> ed e' grossa <size>

    (optional)  srcColorSpace: RGB|sRGB             => default "sRGB" => indica il color space dell'immagine sorgente

	(optional)  dstNumMipMap: max|<number >= 1>     => default = "max" => indica il num di mipmap totali (compreso il livello 0)
	
	(mandatory) dstFmt:U8_RGBA|U8_RGB|U8_R

	(optional) post_op:  <operation>,<param1>, ... , <paramN> ;		=> un elenco di <operation> da eseguire sulla img src prima di produrre l'asset
						 ....										=> le <operation> sono separate da ;  i parametri di una <operation>
						 <operation>,<param1>, ... , <paramN>		=> sono separti da ,

		Le <operation> supportate sono:
			normalizeRGB			    => calcola il min e il max colore per ogni canale e lo rimappa in 0-255
            normalizeRGBA			    => calcola il min e il max colore per ogni canale e lo rimappa in 0-255

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
                    Builder_tex2D ();
                    ~Builder_tex2D()                                                            { }

            void    initOnce (gos::GPU *gpuIN, BuilderList *builderList);
            void    deinitOnce();

			bool 	build_begin (DBContext &ctx, const UniqueUIDList *listof_UID_of_known_ini_file, const char *absFilename, UID uid_of_iniFile, const gos::IniFileSection *sec);
			bool 	build_exe (DBContext &ctx, bool doCreateAnAssetFile, bool *out_bCallMeAgain, sBuildResult *out_result);
			void 	build_end();


        private:
            struct Params
            {
                u32				calculated_image_crc32;
                u16             srcIs_sRGB;
                u16             dstNumMipMap;
                eImageFormat    dstFmt;
            };

        private:
            bool    priv_extractParams (DBContext &ctx, const UniqueUIDList *listof_UID_of_known_ini_file, const char *absFilename);
            bool    priv_do_create_assetFile (DBContext &ctx, UID signatureUID, const Params &params, const char *filenameDST);
            bool    priv_create_GPUResourceOnce();
            bool    priv_save (const gpu::sMappedImage &src, gos::image::Builder &builder, eImageFormat dstFmt, u32 srcW, u32 srcH, u32 mipMapNum_0toN, u32 numPallini);
            bool    priv_apply_post_op (image::BufferRGBA *img);
			bool    priv_load_and_process_src_image();

        private:
			Params 				params;
			UID 				uid_of_iniFile;
			char 				imagesrc_filename[512];
			u32					imagesrc_offset_in_file;
			u32					imagesrc_size_in_file;

			ResourceDep			resource_image;
			image::BufferRGBA 	srcImage;
			const gos::IniFileSection *sec;
            gos::GPU            *gpu;
			gos::gpu::StageHelper	stageHelper;
            GPUSamplerHandle    samplerHandle;
            GPUPipelineHandle   pipeHandle;
            GPUDescrPoolHandle  descrPoolHandle;
            GPUDescrSetInstanceHandle descrSetInstanceHandle;

            
        }; //class Builder_tex2D

    } //namespace asset2
} //namespace gos

#endif //_gosAsset2Builder_tex2D_h_

