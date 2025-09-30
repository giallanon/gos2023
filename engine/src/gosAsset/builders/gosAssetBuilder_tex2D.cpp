#include "gos.h"
#include "../gos/gosString.h"
#include "../gosAssetBuilder.h"
#include "../gosImage/gosImageBufferRGBA.h"
#include "gosAssetBuilder_tex2D.h"


using namespace gos;
using namespace gos::asset;



//************************************
bool Builder_tex2D::priv_extractParams (const IniFileSection *sec, Params *out_params)
{
    assert (NULL != sec);
    assert (NULL != out_params);
    
    //setto i default
    memset (out_params, 0, sizeof(Params));
    out_params->srcIs_sRGB = 1;
    out_params->dstNumMipMap = u16MAX;

    //parse della section
    char s[1024];

    //param:src         e' mandatorio ed indica il nome della risorsa immagine di partenza
    //                  L'immagine in questione deve esistere in res/03-image
    if (!sec->get("src", out_params->src, sizeof(out_params->src)))
    {
        gos::logger::err ("asset::Builder_tex2D::extractParams => can't find param <src>\n");
        return false;
    }

    //param:srcColorSpace      e' opzionale
    if (sec->get("srcColorSpace", s, sizeof(s)))
    {
        if (strcmp(s, "sRGB") == 0)         out_params->srcIs_sRGB = 1;
        else if (strcmp(s, "RGB") == 0)     out_params->srcIs_sRGB = 0;
        else
        {
            logger::err ("asset::Builder_tex2D::extractParams => invalid option '%s' for <srcColorSpace>\n", s);
            return false;
        }
    }

    //param:dstNumMipMap      e' opzionale
    if (sec->get("dstNumMipMap", s, sizeof(s)))
    {
        if (strcmp(s, "max") == 0)
            out_params->dstNumMipMap = u16MAX;
        else
        {
            i32 n = gos::string::ansi::toI32(s);
            if (n < 1)
            {
                logger::err ("asset::Builder_tex2D::extractParams => invalid option '%s' for <dstNumMipMap>. The value cannot be less than 1\n", s);
                return false;
            }
            out_params->dstNumMipMap = static_cast<u16>(n);
        }
    }

    //param:dstFmt      e' mandatorio ed indica il formato della texture da generare
    if (!sec->get("dstFmt", s, sizeof(s)))
    {
        gos::logger::err ("asset::Builder_tex2D::extractParams => can't find param <dstFmt>\n");
        return false;
    }    
    if (!gos::utils::stringToEnum (s, &out_params->dstFmt))
    {
        logger::err ("asset::Builder_pipe::extractParams => invalid option '%s' for <dstFmt> (invalid format)\n", s);
        return false;
    }
    switch (out_params->dstFmt)
    {
    case eImageFormat::U8_R:
    case eImageFormat::U8_RGB:
    case eImageFormat::U8_RGBA:
        break;

    default:
        logger::err ("asset::Builder_pipe::extractParams => invalid option '%s' for <dstFmt> (format not supported)\n", s);
        return false;
    }


    return true;
}

//************************************
bool Builder_tex2D::build (Context &ctx, u64 buildTimeUTC, const char *sourceFileInfo, const asset::UID &uid_of_iniFile, const IniFileSection *sec, bool doCreateAnAssetFile, gos::GPU *gpu, sBuildResult *out)
{
    assert (ctx.isValid());
    assert (NULL != sec);
    assert (NULL != out);

    if (NULL == gpu)
    {
        gos::logger::err ("error, a valid instance of GPU is needed\n");
        return false;
    }


    out->reset();

    //parse della sezione
    Params params;
    if (!priv_extractParams(sec, &params))
    {
        gos::logger::err ("error parsing IniFileSection\n");
        return false;
    }

    //il parametro src indica una risorsa eResType::image da cui io dipendo
    //La risorsa deve esistere nel DB
    if (!prot_needResource (ctx, eResType::image, params.src, &params.uid__resource_image))
    {
        gos::logger::err ("resource [%s] '%s' not found in DB\n", asset::enumToString(eResType::image), params.src);
        return false;
    }     

    //calcolo assetUID
    if (!asset::asset_createUID (getAssType(), calc_depth(), &params, sizeof(Params), &out->uid))
    {
        gos::logger::err ("error generating assetUID\n");
        return false;
    }


    /*  Idealmente asset::UID non dovrebbe esistere in tabella visto che lo sto buildando.
        Potenzialmente pero', lo stesso UID puo' essere generato da diversi asset perche' lo specificano
        inline o perche' vi fanno riferimento direttamente usando un runtimeName.
        In linea di massimo quindi, se l'asset esiste gia', non sto a ricompilarlo dato che il 
        risultato sarebbe il medesimo
    */
    const u64 lastTimeBuilt = asset::asset_query_lastTimeBuilt (ctx, out->uid);
    if (0 != lastTimeBuilt)
    {
        //asset::UID esiste gia' nel DB ma e' stato buildato a questo giro di build, quindi va bene,
        //semplicemente non sto a buildarlo una seconda volta
        out->result = eBuildResult::was_already_built;
        return true;

    }

    //asset::UID non esisteva nel DB, ottimo, lo aggiungo e termino con successo
    if (!asset::asset_insert (ctx, out->uid, getAssType(), buildTimeUTC, sourceFileInfo))
    {
        gos::logger::err ("error inserting asset\n");
        return false;
    }

    //aggiungo le sue dipendenze
    if (!asset::depend_add (ctx, out->uid, uid_of_iniFile)) return false;
    if (!asset::depend_add (ctx, out->uid, params.uid__resource_image)) return false;

    //segno che e' stato buildato di fresco
    out->result = eBuildResult::just_built;

    
    //a questo punto devo davvero creare il file dell'asset
    if (doCreateAnAssetFile)
    {
        char filenameDST[1024];
        asset::asset_manufacture_fullFilename (ctx, out->uid, filenameDST, sizeof(filenameDST));
        return priv_do_create_assetFile (ctx, params, gpu, filenameDST);
    }

    return true;
}


//************************************
bool Builder_tex2D::priv_do_create_assetFile (Context &ctx, const Params &params, gos::GPU *gpu, const char *filenameDST) const
{
    bool result = false;
    char s[1024];

    //carico l'immagine e creo la texture in GPU
    GPUTextureHandle texHandle;
    u16 srcImg_dimx = 0;
    u16 srcImg_dimy = 0;
    {
        asset::res_get_folder_nameByType (ctx, eResType::image, s, sizeof(s));
        strcat_s (s, sizeof(s), "/");
        strcat_s (s, sizeof(s), params.src);

        image::BufferRGBA srcImage;
        if (!srcImage.loadFromFile (gos::getSysHeapAllocator(), s))
        {
            gos::logger::err ("image type not supported: '%s'\n");
            return false;
        }
        srcImg_dimx = srcImage.getW();
        srcImg_dimy = srcImage.getH();

        //se necessario converto sRGB to RGB
        if (params.srcIs_sRGB)
            srcImage.convert_sRGB_to_RGB();

        result = gpu->texture_create2D (srcImage.getW(), srcImage.getH(), 1, eImageFormat::U8_RGBA, eMemAccessMode::onGPU, srcImage._bufferRGBA, &texHandle);
        srcImage.free (gos::getSysHeapAllocator());

        if (!result)
        {
            gos::logger::err ("gpu->texture_create2D() => failed\n");
            return false;
        }
    }

    //creo i render target
    const u32 rt_width = gos::utils::calcClosestPowerOf2(srcImg_dimx);
    const u32 rt_height = gos::utils::calcClosestPowerOf2(srcImg_dimy);
    GPUViewportHandle viewportHandle;
    GPURenderTargetHandle rt1;
    GPURenderTargetHandle rtReadback;
    GPUSamplerHandle samplerHandle;

    while (1)
    {
        gpu->viewport_create (0, 0, rt_width, rt_height, &viewportHandle);

        if (!gpu->renderTarget_create (rt_width, rt_height, eImageFormat::U8_RGBA, &rt1))
        {
            gos::logger::err ("gpu->renderTarget_create(rt1) => failed\n");
            result = false;
            break;
        }

        if (!gpu->renderTarget_create (rt_width, rt_height, eImageFormat::U8_RGBA, eMemAccessMode::readback, &rtReadback))
        {
            gos::logger::err ("gpu->renderTarget_create(rtReadback) => failed\n");
            result = false; 
            break;
        }

        if (!gpu->sampler_create (gpu::SamplerDesc(), &samplerHandle))
        {
            gos::logger::err ("gpu->renderTarget_create(rtReadback) => failed\n");
            result = false; 
            break;
        }

        break;
    }

    if (result)
    {
    }


    //free gpu resource
    gpu->deleteResource (texHandle);
    gpu->deleteResource (viewportHandle);
    gpu->deleteResource (rt1);
    gpu->deleteResource (rtReadback);
    
    return false;
}