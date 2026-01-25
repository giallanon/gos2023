#include "gos.h"
#include "../gos/gosString.h"
#include "../gosAsset2Builder.h"
#include "../gosImage/gosImageBufferRGBA.h"
#include "gosAsset2Builder_tex2D.h"
#include "gosAsset2Builder_tex2D_shaders.h"


using namespace gos;
using namespace gos::asset2;

//************************************
Builder_tex2D::Builder_tex2D () : BuilderInterface (eAssetType::tex2D)
{ 
    gpu = NULL;
    samplerHandle.setInvalid();
}

//************************************
void Builder_tex2D::initOnce (gos::GPU *gpuIN)
{ 
    gpu = gpuIN; 
	stageHelper.setup (gpu, 8192*8192);
}

//************************************
bool Builder_tex2D::priv_create_GPUResourceOnce()
{
    assert (NULL != gpu);

    if (samplerHandle.isValid())
        return true;

    if (!gpu->sampler_create (gpu::SamplerDesc(), &samplerHandle))
    {
        gos::logger::err ("gpu->sampler_create() => failed\n");
        return false;
    }

    if (pipeHandle.isInvalid())
    {
        bool ret = true;

        GPUShaderHandle vtxShaderHandle;
        {
            if (!gpu->vtxshader_createFromMemory (GOS_ASSET2__BUILDER_TEX2D_VTX_SHADER, sizeof(GOS_ASSET2__BUILDER_TEX2D_VTX_SHADER), "main", &vtxShaderHandle))
            {
                gos::logger::err ("gpu->vtxshader_createFromMemory() => failed\n");
                ret = false;
            }
        }

        GPUShaderHandle pxlShaderHandle;
        if (ret)
        {
            if (!gpu->pxlshader_createFromMemory (GOS_ASSET2__BUILDER_TEX2D_PXL_SHADER, sizeof(GOS_ASSET2__BUILDER_TEX2D_PXL_SHADER), "main", &pxlShaderHandle))
            {
                gos::logger::err ("gpu->pxlshader_createFromMemory() => failed\n");
                ret = false;
            }
        }

        if (ret)
        {
            gpu::Pipeline_def def;
            def
                .reset()
                .rt_add (eImageFormat::U8_RGBA)
                .shader_add (vtxShaderHandle)
                .shader_add (pxlShaderHandle)
                .pushConst_add (0, 8, eShaderType::vtxShader)   //screenWH
                .pushConst_add (8, 8, eShaderType::vtxShader)   //quadSize
                .descriptorset_add ()
                    .add(0, eGPUDescriptrorType::COMBINED_IMAGE_SAMPLER, 1, eGPUDescriptrorUsage::pxl_shader)
                    .endDescriptorSet()
                ;

            if (!gpu->pipeline_createNew (def, &pipeHandle))
            {
                gos::logger::err ("gpu->pipeline_createNew() => failed\n");
                ret = false;
            }
        }

        gpu->deleteResource(vtxShaderHandle);
        gpu->deleteResource(pxlShaderHandle);
        if (!ret)
            return false;
    }


    //creo un descriptor pool
    gpu->descrPool_createNew (&descrPoolHandle)
        .setMaxNumDescriptorSet(1)
        .addPool_combinedTextureAndSampler(1)
        .end();
    if (descrPoolHandle.isInvalid())
    {
        gos::logger::err ("gpu->descrPool_createNew() => failed\n");
        return false;
    }

    //alloco una istanza del descriptorSet
    if (!gpu->descrSetInstance_create (descrPoolHandle, pipeHandle, 0, &descrSetInstanceHandle))
    {
        gos::logger::err ("gpu->descrSetInstance_create() => failed\n");
        return false;
    }    

    return true;
}

//************************************
void Builder_tex2D::deinitOnce()
{
    //gpu->deleteResource (samplerHandle); non serve, ci pensa GPU da sola
	stageHelper.unsetup();
    gpu->deleteResource(pipeHandle);
    gpu->deleteResource(descrSetInstanceHandle);
    gpu->deleteResource(descrPoolHandle);
    gpu = NULL;

}

//************************************
bool Builder_tex2D::priv_extractParams (DBContext &ctx, const UniqueUIDList &listof_UID_of_known_ini_file, const char *absFilename)
{
    assert (NULL != sec);
    
    //setto i default
    memset (&params, 0, sizeof(Params));
    params.srcIs_sRGB = 1;
    params.dstNumMipMap = u16MAX;

    //parse della section
    char s[1024];

    //param:src         e' mandatorio ed indica il nome della risorsa immagine di partenza
    if (!sec->get("src", s, sizeof(s)))
    {
        logger->log(eTextColor::red, "line %d => can't find param <src>\n", sec->getLineStarted());
        return false;
    }
    if (!asset2::Builder::makeABSPathFromFilename (ctx, logger, listof_UID_of_known_ini_file, absFilename, s, params.src, sizeof(params.src)))
        return false;


    //param:srcColorSpace      e' opzionale
    if (sec->get("srcColorSpace", s, sizeof(s)))
    {
        if (strcmp(s, "sRGB") == 0)         params.srcIs_sRGB = 1;
        else if (strcmp(s, "RGB") == 0)     params.srcIs_sRGB = 0;
        else
        {
            logger->log(eTextColor::red, "line %d => invalid option '%s' for <srcColorSpace>\n", sec->getLineStarted(), s);
            return false;
        }
    }

    //param:dstNumMipMap      e' opzionale
    if (sec->get("dstNumMipMap", s, sizeof(s)))
    {
        if (strcmp(s, "max") == 0)
            params.dstNumMipMap = u16MAX;
        else
        {
            i32 n = gos::string::ansi::toI32(s);
            if (n < 1)
            {
                logger->log(eTextColor::red, "line %d => invalid option '%s' for <dstNumMipMap>. The value cannot be less than 1\n", sec->getLineStarted(), s);
                return false;
            }
            params.dstNumMipMap = static_cast<u16>(n);
        }
    }

    //param:dstFmt      e' mandatorio ed indica il formato della texture da generare
    if (!sec->get("dstFmt", s, sizeof(s)))
    {
        logger->log(eTextColor::red, "line %d => can't find param <dstFmt>\n", sec->getLineStarted());
        return false;
    }    
    if (!gos::utils::stringToEnum (s, &params.dstFmt))
    {
        logger->log(eTextColor::red, "line %d => invalid option '%s' for <dstFmt> (invalid format)\n", sec->getLineStarted(), s);
        return false;
    }
    switch (params.dstFmt)
    {
    case eImageFormat::U8_R:
    case eImageFormat::U8_RGB:
    case eImageFormat::U8_RGBA:
        break;

    default:
        logger->log(eTextColor::red, "line %d => invalid option '%s' for <dstFmt> (format not supported)\n", sec->getLineStarted(), s);
        return false;
    }


    return true;
}

//************************************
bool Builder_tex2D::build_begin (DBContext &ctx, const UniqueUIDList &listof_UID_of_known_ini_file, const char *absFilename, UID uid_of_iniFileIN, const gos::IniFileSection *secIN)
{
    assert (ctx.isValid());
    assert (NULL != secIN);
	uid_of_iniFile = uid_of_iniFileIN;
	sec = secIN;
    

    if (NULL == gpu)
    {
        gos::logger::err ("error, a valid instance of GPU is needed\n");
        return false;
    }


    //parse della sezione
    if (!priv_extractParams(ctx, listof_UID_of_known_ini_file, absFilename))
    {
        logger->log (eTextColor::red, "error parsing IniFileSection\n");
        return false;
    }

    //il parametro src indica una risorsa eResType::image da cui io dipendo
    //La risorsa deve esistere nel DB
    if (!prot_needResource (ctx, listof_UID_of_known_ini_file, eResType::image, params.src, &params.uid__resource_image))
    {
        logger->log (eTextColor::red, "resource [%s] '%s' not found in DB\n", asset2::enumToString(eResType::image), params.src);
        return false;
    }     

    return true;
}

//************************************
bool Builder_tex2D::build_exe (DBContext &ctx, bool doCreateAnAssetFile, bool *out_bCallMeAgain, sBuildResult *out_result)
{
	assert (NULL != out_result);
	assert (NULL != out_bCallMeAgain);
	*out_bCallMeAgain = false;
    out_result->reset();

	//questo file gosasset_d dipende dalla risorsa params.uid__resource_image)
    if (!asset2::dependency_exists(ctx, uid_of_iniFile, params.uid__resource_image))
    {
        if (!asset2::dependency_add (ctx, uid_of_iniFile, params.uid__resource_image)) 
            return false;  
    }
    
    

    //setup di virtual-asset
    //All'uscita da questa fn:
    //  out_result->uid_virtual_asset       contiene l'UID di questo virtual asset, gia' inserito nel DB
    //  out_result->uid_concrete_asset      contiene l'UID dell'asset concreto a cui questo virtual-asset punta
    //  out_result->result                  vale <eBuildResult::just_built> se e' necessario creare fisicamente il concrete-asset, altrimenti vale <eBuildResult::was_already_built>
    if (!prot_setupVirtualAsset (ctx, &params, sizeof(Params), uid_of_iniFile, sec, out_result))
        return false;


    //aggiungo le dipendenze di virtual-asset dalla risorsa IMMAGINE
    if (!dependency_add (ctx, out_result->uid_virtual_asset, params.uid__resource_image)) return false;


    
    //a questo punto devo davvero creare il file dell'asset
    if (doCreateAnAssetFile && eBuildResult::just_built == out_result->result)
    {
        char filenameDST[1024];
        asset_manufacture_fullFilename (ctx, out_result->uid_concrete_asset, filenameDST, sizeof(filenameDST));
        return priv_do_create_assetFile (ctx, out_result->uid_concrete_asset, params, filenameDST);
    }

	return true;
}


//************************************
bool Builder_tex2D::priv_do_create_assetFile (DBContext &ctx, UID uid_concrete_asset, const Params &params, const char *filenameDST)
{
    bool result = false;

    //se necessario creo le risorse di GPU comuni a tutte le volte che buildo questo tipo di asset
    if (!priv_create_GPUResourceOnce())
        return false;



    //carico l'immagine e creo la texture in GPU
    GPUTextureHandle texHandle;
    u16 srcImg_dimx = 0;
    u16 srcImg_dimy = 0;
    {
        image::BufferRGBA srcImage;
        if (!srcImage.loadFromFile (gos::getSysHeapAllocator(), params.src))
        {
            logger->log (eTextColor::red, "image type not supported: '%s'\n", params.src);
            return false;
        }
        srcImg_dimx = srcImage.getW();
        srcImg_dimy = srcImage.getH();

        //se necessario converto sRGB to RGB
        if (params.srcIs_sRGB)
            srcImage.convert_sRGB_to_RGB();

        result = gpu->texture_create2D (srcImage.getW(), srcImage.getH(), 1, eImageFormat::U8_RGBA, eMemAccessMode::onGPU, srcImage._bufferRGBA, &texHandle, stageHelper);
        srcImage.free (gos::getSysHeapAllocator());

        if (!result)
        {
            logger->log (eTextColor::red, "gpu->texture_create2D() => failed\n");
            return false;
        }
    }

    //creo i render target dimensionati in base alla texture caricata
    const u32 rt_width = gos::utils::calcClosestPowerOf2(srcImg_dimx);
    const u32 rt_height = gos::utils::calcClosestPowerOf2(srcImg_dimy);
    GPUViewportHandle   viewportHandle;
    GPURenderTargetHandle rt1;
    GPURenderTargetHandle rtReadback;

    while (1)
    {
        gpu->viewport_create (0, 0, rt_width, rt_height, &viewportHandle);

        if (!gpu->renderTarget_create (rt_width, rt_height, eImageFormat::U8_RGBA, &rt1))
        {
            logger->log (eTextColor::red, "gpu->renderTarget_create(rt1) => failed\n");
            result = false;
            break;
        }

        if (!gpu->renderTarget_create (rt_width, rt_height, eImageFormat::U8_RGBA, eMemAccessMode::readback, &rtReadback))
        {
            logger->log (eTextColor::red, "gpu->renderTarget_create(rtReadback) => failed\n");
            result = false; 
            break;
        }

        break;
    }

    if (result)
    {
        //calcola il num massimo di mip-map ottenibili
        u32 dim = srcImg_dimx;
        if (srcImg_dimy < dim)
            dim = srcImg_dimy;

        u32 numMipMap = 1;
        while (dim > 32)
        {
            dim/=2;
            numMipMap++;
        }

        if (u32MAX != params.dstNumMipMap)
        {
            if (params.dstNumMipMap < numMipMap)
                numMipMap = params.dstNumMipMap;
        }

        //preparo image::builder
        gos::Image  image;
        image::Builder builder;
        builder.begin (gos::getSysHeapAllocator(), &image);
        builder.beginTexture2D (params.dstFmt, srcImg_dimx, srcImg_dimy, numMipMap);

        //aggiorno il descriptor-set con la texture
        gos::gpu::DescrSetInstanceWriter descrWriter;
        descrWriter.begin (gpu, descrSetInstanceHandle)
            .bindCombinedTextureAndSampler (0, texHandle, samplerHandle)
            .end();        

        //crea le mipmap
        GPUCmdBufferHandle  cmdBufferHandle;
        gpu->cmdBuffer_create (eGPUQueueFamily::gfx, &cmdBufferHandle);

        gpu::GFXJob job;
        job.setup (gpu);

        for (u32 i=0; i<numMipMap; i++)
        {
            //job per la GPU
            {
                vec2f screenWH;
                screenWH.set ((f32)rt_width, (f32)rt_height);

                vec2f quadWH;
                quadWH.set ((f32)srcImg_dimx, (f32)srcImg_dimy);

                gos::gpu::CmdBufferWriter2 cw;
                cw  .begin (gpu, cmdBufferHandle)
                    .setViewport (viewportHandle)
                    .imageTransition (rt1, eImageLayout::undefined, eImageLayout::color_attachment_optimal)
                    .imageTransition (rtReadback, eImageLayout::undefined, eImageLayout::transfer_dst);

                gos::gpu::RenderCtx rctx;
                cw  .renderCtx_define_begin(&rctx)
                    .withRenderArea (rt1)
                    .withRT (rt1, eAttachmentLoadOp::clear, eAttachmentStoreOp::dont_care, gos::ColorHDR(0, 1.0f, 0))
                    .define_end();

                assert (!rctx.anyError());
                rctx.bindPipeline (pipeHandle)
                    .bindDescriptorSet(descrSetInstanceHandle, 0)
                    .pushConstant (0, &screenWH, sizeof(screenWH))
                    .pushConstant (1, &quadWH, sizeof(quadWH))
                    .draw (6, 1, 0, 0)
                    .end_render_ctx();

                cw  .imageTransition (rt1, eImageLayout::color_attachment_optimal, eImageLayout::transfer_src)
                    .copyImageToImage (rt1, rtReadback, { rt_width, rt_height}, { rt_width, rt_height} )
                    .imageTransition (rtReadback, eImageLayout::transfer_dst, eImageLayout::general)
                .end();            

                job.submit (cmdBufferHandle);
                while (!job.hasFinished())
                    gpu->waitIdle();
            }

            gpu::sMappedImage m;
            if (gpu->map (rtReadback, &m))
            {
                gpu->image_manualSync_cpuRead(&m ,1);
                if (!priv_save (m, builder, params.dstFmt, srcImg_dimx, srcImg_dimy, i, numMipMap-i))
                    result = false;
                gpu->image_unmap (m);
            }

            srcImg_dimx/=2;
            srcImg_dimy/=2;

            if (!result)
                break;
        }        
        job.unsetup();
        gpu->deleteResource (cmdBufferHandle);

        builder.endTexture2D();
        if (!builder.end())
        {
            gos::logger::err ("builder.end() => failed\n");
            result = false; 
        }

        //salvo l'asset
        if (result)
            gos::image::save (image, filenameDST);
        gos::image::free (gos::getSysHeapAllocator(), image);
    }


    //free gpu resource    
    gpu->deleteResource (viewportHandle);
    gpu->deleteResource (texHandle);
    gpu->deleteResource (rt1);
    gpu->deleteResource (rtReadback);
    
    return result;
}

//************************************
bool Builder_tex2D::priv_save (const gpu::sMappedImage &src, gos::image::Builder &imgBuilder, eImageFormat dstFmt, u32 srcW, u32 srcH, u32 mipMapNum_0toN, u32 numPallini)
{
    gos::Allocator *allocator = gos::getSysHeapAllocator();
    image::BufferRGBA rgba;

    rgba.alloc (allocator, (u16)srcW, (u16)srcH);
    rgba.clear (255,0,0,255);

    const u32 dst_row_stride = srcW * 4;
    const u8 *srcPT = reinterpret_cast<const u8*>(src.host_image_pt);

    u32 ctSRC = 0;
    u32 ctDST = 0;
    for (u32 y=0; y<srcH; y++)
    {
        //src e' in formato U8_RGBA
        memcpy (&rgba._bufferRGBA[ctDST], &srcPT[ctSRC], dst_row_stride);
        ctSRC += src.row_stride;
        ctDST += dst_row_stride;
    }

    /*per debug ci metto dei pallini sulle mip-map
    const u32 radius = 10;
    u32 x = radius+2;
    for (u32 i=0; i<numPallini; i++)
    {
        rgba.circle (x, radius+2, radius, 255, 255, 255);
        x += (radius+5);
    }

    //sempre per debug salvo l'output
    char s[1024];
    sprintf_s (s, sizeof(s), "out_%d_%d.tga", srcW, srcH);
    rgba.saveAsTGA (s);
    */

    //aggiorno il builder dell'image
    const u32 rgba_size = rgba.getSize();
    bool result = true;
    switch (dstFmt)
    {
    default:
        gos::logger::err ("invalid dst format\n");
        result = false;
        break;

    case eImageFormat::U8_RGBA:
        imgBuilder.setMipMapDataMemory (mipMapNum_0toN, rgba.getBuffer(), rgba_size, image::Builder::eFilter::none);
        break;

    case eImageFormat::U8_RGB:
        {
            u8 *buffer = GOSALLOC_SCRAPT(u8*, srcW*srcH*3);
            ctSRC = 0;
            ctDST = 0;
            while (ctSRC < rgba_size)
            {
                const u8 r = rgba._bufferRGBA[ctSRC++];
                const u8 g = rgba._bufferRGBA[ctSRC++];
                const u8 b = rgba._bufferRGBA[ctSRC++];
                ctSRC++;

                buffer[ctDST++] = r;
                buffer[ctDST++] = g;
                buffer[ctDST++] = b;
            }
            imgBuilder.setMipMapDataMemory (mipMapNum_0toN, buffer, ctDST, image::Builder::eFilter::none);
            GOSFREE_SCRAP(buffer);
        }
        break;

    case eImageFormat::U8_R:
        {
            u8 *buffer = GOSALLOC_SCRAPT(u8*, srcW*srcH);
            ctSRC = 0;
            ctDST = 0;
            while (ctSRC < rgba_size)
            {
                const u8 r = rgba._bufferRGBA[ctSRC++];
                ctSRC++;
                ctSRC++;
                ctSRC++;

                buffer[ctDST++] = r;
            }
            imgBuilder.setMipMapDataMemory (mipMapNum_0toN, buffer, ctDST, image::Builder::eFilter::none);
            GOSFREE_SCRAP(buffer);
        }
        break;        
    }


    rgba.free (allocator);
    return result;
}
