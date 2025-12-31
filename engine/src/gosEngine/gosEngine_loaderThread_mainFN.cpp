#include "gosEngine.h"
#include "../gosAsset2/gosAsset2.h"
#include "../gos/gosBufferReader.h"
#include "../gos/gosDataBlob.h"

using namespace gos;
using namespace gos::engine;

typedef gos::AllocatorHeap<gos::AllocPolicy_Track_simple, gos::AllocPolicy_Thread_Unsafe>		GOSENGINELoaderMemAllocatorTS;

/*******************************************************
 * @brief   KnownAssets
 *          Mantiene una lista di <uid, pt_to_data> di tutti gli asset che sono stati caricati con successo,
 *          dove <pt_to_data> e' il membro 'Data' di una risorsa dell'engine (ad es: ResTexture::Data).
 *          E' un doppione perche' anche Engine tiene una lista degli asset, ma serve per caricare gli asset
 *          con dipendenza.
 *          Ad esempio, una pipe viene schedulata caricando vtx_shader, poi pxl_shader e poi la pipe stessa-
 *          Quando arriviamo a dove creare la pipe, mi servono vtx e pxl shader e non li posso chiedere all'engine
 *          per problemi di concorrenza ma anche per il fatto che l'engine potrebbe non essere ancora consapevole che
 *          vtx/pxl sono stati caricati e sono pronti (perche' non ha ancora processato il msg che LoaderThread gli ha
 *          mandato dicendo che gli shader sono ready).
 *          A questo punto mi serve poter accedere agli shader e lo faccio utilizzando KnownAssets * 
 */
class KnownAssets
{
public:
            KnownAssets()                               { allocator = NULL; }
            ~KnownAssets()                              { unsetup(); }

    void    setup (gos::Allocator *allocatorIN)         { allocator = allocatorIN; list.setup (allocator, 8192); }
    void    unsetup()
    {
        if (NULL == allocator)
            return;
        list.forEach ( [allocator=this->allocator](asset2::UID uid, Info value) {
            GOSFREE(allocator, value.pt_to_data);
            return true;
        });

        list.unsetup();
        allocator = NULL;
    }
    
    void    add_or_replace (asset2::UID uid, const void *data, u32 sizeof_data)
    {
        HashedUIDList::Position pos;
        if (list.findPosition (uid, &pos))
        {
            Info *cur_value = list.getValueAtPos(pos);
            assert (cur_value->allocated_size == sizeof_data);
            memcpy (cur_value->pt_to_data, data, sizeof_data);
        }
        else
        {
            Info value;
            value.allocated_size = sizeof_data;
            value.pt_to_data = GOSALLOC(allocator, sizeof_data);
            memcpy (value.pt_to_data, data, sizeof_data);
            list.insertInPosition (pos, value);
        }
    }

    void*   find_data_by_uid (asset2::UID uid) const
    {
        Info info;
        if (list.find (uid, &info))
            return info.pt_to_data;
        return NULL;
    }

private:
    struct Info
    {
        void    *pt_to_data;
        u32     allocated_size;
    };

private:
    gos::Allocator *allocator;
    typedef FastHashMap<asset2::UID, Info> HashedUIDList;

private:
    HashedUIDList list;
};


//********************************************************
struct LoaderInfo
{
    gos::Allocator      *allocator;
    gos::Logger			*logger;
    gos::GPU            *gpu;
    asset2::DBContext   *ctx;
    KnownAssets         *listof_knownAssets;
};


//********************************************************
class BaseLoader
{
public:
                    BaseLoader()        { } 
    virtual         ~BaseLoader()       { }
    virtual bool    load (LoaderInfo &loaderInfo, asset2::UID uid, void *out_data) = 0;
};


//********************************************************
class Loader_tex2D : public BaseLoader
{
public:
    bool    load (LoaderInfo &loaderInfo, asset2::UID uid, void *out_dataIN)
    {
        ResTexture::DataForLoaderThread *out_data = static_cast<ResTexture::DataForLoaderThread*>(out_dataIN);
        gos::Allocator *allocator = loaderInfo.allocator;
        gos::GPU *gpu = loaderInfo.gpu;

        char s[1024];
        asset2::asset_manufacture_fullFilename (*loaderInfo.ctx, uid, s, sizeof(s));

        gos::Image image;
        if (!image::load (allocator, s, &image))
        {
            logger::err ("Loader_tex2D::load() => file not found %s\n", s);
            return false;
        }
        
        const bool ret = gpu->texture_create2D (&image, 0, eMemAccessMode::onGPU, &out_data->data.texHandle);
        image::free (allocator, image);

        //mi aggiungo alla lista degli asset noti
        if (ret)
            loaderInfo.listof_knownAssets->add_or_replace (uid, &out_data->data, sizeof(out_data->data));
        return ret;           
    }
};


//********************************************************
class Loader_vtxShader : public BaseLoader
{
public:
    bool    load (LoaderInfo &loaderInfo, asset2::UID uid, void *out_dataIN)
    {
        ResShader::DataForLoaderThread *out_data = static_cast<ResShader::DataForLoaderThread*>(out_dataIN);
        gos::GPU *gpu = loaderInfo.gpu;

        char s[1024];
        asset2::asset_manufacture_fullFilename (*loaderInfo.ctx, uid, s, sizeof(s));
        if (!gpu->vtxshader_createFromFile (s, "main", &out_data->data.shaderHandle))
            return false;

        //mi aggiungo alla lista degli asset noti
        loaderInfo.listof_knownAssets->add_or_replace (uid, &out_data->data, sizeof(out_data->data));
        return true;
    }
};


//********************************************************
class Loader_pxlShader : public BaseLoader
{
public:
    bool    load (LoaderInfo &loaderInfo, asset2::UID uid, void *out_dataIN)
    {
        ResShader::DataForLoaderThread *out_data = static_cast<ResShader::DataForLoaderThread*>(out_dataIN);
        gos::GPU *gpu = loaderInfo.gpu;

        char s[1024];
        asset2::asset_manufacture_fullFilename (*loaderInfo.ctx, uid, s, sizeof(s));
        if (!gpu->pxlshader_createFromFile (s, "main", &out_data->data.shaderHandle))
            return false;

        //mi aggiungo alla lista degli asset noti
        loaderInfo.listof_knownAssets->add_or_replace (uid, &out_data->data, sizeof(out_data->data));
        return true;

    }
};


//********************************************************
class Loader_pipeline : public BaseLoader
{
public:
    bool    load (LoaderInfo &loaderInfo, asset2::UID uid, void *out_dataIN)
    {
        ResPipeline::DataForLoaderThread *out_data = static_cast<ResPipeline::DataForLoaderThread*>(out_dataIN);
        gos::GPU *gpu = loaderInfo.gpu;

        char s[1024];
        asset2::asset_manufacture_fullFilename (*loaderInfo.ctx, uid, s, sizeof(s));

        u32 fsize;
        u8 *buffer = fs::fileLoadInMemory (gos::getScrapAllocator(), s, &fsize);
        if (NULL == buffer)
        {
            logger::err ("Loader_pipeline::load() => file not found %s\n", s);
            return false;
        }

        gos::BufferR reader;
        reader.setup (buffer, fsize);
        
        bool ret = false;
        while (1)
        {
            const u32 magic = reader.readU32();
            if (!magic::signatureMatch(magic, GOS_MAGIC__ASSET_PIPELINE_DEF) || !magic::versionMatch(magic, GOS_MAGIC__ASSET_PIPELINE_DEF))
            {
                logger::err ("Loader_pipeline::load() => invalid magic for file %s\n", s);
                break;
            }

            gpu::pipe2::Pipeline_def def;
            def.reset();


            //uid vtx shader
            //L'asset dovrebbe gia' essere stato caricato perche' engine ha schedulato i vari load in maniera intelligente.
            //Se cosi' e', allora lo trovo in <listof_knownAssets>
            asset2::UID uid;
            uid._uid = reader.readU64 ();
            {
                const void *pt_to_data = loaderInfo.listof_knownAssets->find_data_by_uid (uid);
                if (NULL == pt_to_data)
                {
                    logger::log (eTextColor::red, "asset::  Loader_pipeline::load() => vtx_shader %016" PRIX64 " not available\n");
                    break;
                }

                const engine::ResShader::Data *data = static_cast<const engine::ResShader::Data*>(pt_to_data);
                def.shader_add (data->shaderHandle);
            }
            

            //uid pxl shader
            uid._uid = reader.readU64 ();
            {
                const void *pt_to_data = loaderInfo.listof_knownAssets->find_data_by_uid (uid);
                if (NULL == pt_to_data)
                {
                    logger::log (eTextColor::red, "asset::  Loader_pipeline::load() => pxl_shader %016" PRIX64 " not available\n");
                    break;
                }

                const engine::ResShader::Data *data = static_cast<const engine::ResShader::Data*>(pt_to_data);
                def.shader_add (data->shaderHandle);
            }

            //cull/draw
            def.set_cullMode (static_cast<eCullMode>(reader.readU8()));
            def.set_drawPrimitive (static_cast<eDrawPrimitive>(reader.readU8()));
            if (0 != reader.readU8())
                def.enable_wireframe();

            //zbuffer
            {
                const bool zEnabled = reader.readBool();
                const eImageFormat fmt = static_cast<eImageFormat>(reader.readU8());
                const bool zwrite = reader.readBool();
                const eZFunc zfunc = static_cast<eZFunc>(reader.readU8());

                if (zEnabled)
                    def.set_zbuffer (fmt, zwrite, zfunc);
            }


            //render target
            u32 n = reader.readU32 ();
            for (u32 i=0; i<n; i++)
            {
                const eImageFormat fmt = static_cast<eImageFormat>(reader.readU8());
                def.add_rt (fmt);
            }

            //vtx declaration
            n = reader.readU32 ();
            if (n)
            {
                auto &builder = def.vtxStream_add(eVtxStreamInputRate::perVertex);
                for (u32 i = 0; i < n; i++)
                {
                    const u8 binding = reader.readU8 ();
                    const u32 offset = reader.readU32 ();
                    const eDataFormat fmt = static_cast<eDataFormat> (reader.readU8 ());
                    builder.add (binding, offset, fmt);
                }        
            }

            //push constant
            n = reader.readU32 ();
            while (n--)
            {
                const u32 offset = reader.readU32();
                const u32 paddedSize = reader.readU32();
                const eShaderType shaderType = static_cast<eShaderType> (reader.readU32());
                def.pushConst_add (offset, paddedSize, shaderType);
            }


            //descriptor set
            const u32 numSet = reader.readU32 ();
            for (u32 i = 0; i < numSet; i++)
            {
                auto &builder = def.descriptorset_add();

                eGPUDescriptrorSetOptionBitmask options;
                options.setFromU32 (reader.readU32 ());


                const u32 numElem = reader.readU32 ();
                for (u32 i2 = 0; i2 < numElem; i2++)
                {
                    const u8 binding = reader.readU8();
                    const eGPUDescriptrorType type = static_cast<eGPUDescriptrorType>(reader.readU8());
                    u32 count = reader.readU32();
                    eGPUDescriptrorUsageBitmask usage;
                    usage.bitmask = reader.readU32();

                    //TODO
                    //u32MAX == count => il buffer e' di tipo bindless... in attesa di capirci megli qualcosa
                    //                      semplicemente lo alloco "grosso"
                    if (u32MAX == count)
                    {
                        count = 1;
                        builder.addCreationFlag (VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT);
                    }
                    if (options.isset(eGPUDescriptrorSetOption::bindless))
                        builder.addCreationFlag (VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT);

                    builder.add (binding, type, count, usage);
                }
            }        

            //creo la pipe
            if (!gpu->pipeline_createNew (def, &out_data->data.pipeHandle))
                break;



            //finito
            ret = true;
            break;
        }
        GOSFREE_SCRAP(buffer);

        //mi aggiungo alla lista degli asset noti
        if (ret)
            loaderInfo.listof_knownAssets->add_or_replace (uid, &out_data->data, sizeof(out_data->data));

        return ret;        
    }
};





//***************************************
i16	Engine::LoaderThread_mainFN (void *paramsIN)
{
    GOSENGINELoaderMemAllocatorTS *localAllocator = GOSNEW(gos::getSysHeapAllocator(), GOSENGINELoaderMemAllocatorTS)("ENGLoader");
    localAllocator->setup (1024 * 1024 * 128); //128MB
    
    HThreadMsgR         msgqR;
    HThreadMsgW         msgqW;
    LoaderInfo          loaderInfo;

    //copia dei params
    {
        const sLoaderThreadInitParams *params = reinterpret_cast<const sLoaderThreadInitParams*>(paramsIN);
        msgqR = params->msgqR;
        msgqW = params->msgqW;
        loaderInfo.allocator = localAllocator;
        loaderInfo.logger = params->logger;
        loaderInfo.gpu = params->gpu;
        loaderInfo.ctx = params->ctx;

        //segnalo che sono partito
        thread::eventFire (params->hEvent_started);
    }

    //spawn dei loader
    static constexpr u32 NUM_MAX_LOADER = (u32)eAssetType::__NUM;
    BaseLoader  *loaderList[NUM_MAX_LOADER];
    {
        memset (loaderList, 0, sizeof(loaderList));
        loaderList[(u32)eAssetType::vtx_shader] = GOSNEW(localAllocator, Loader_vtxShader)();
        loaderList[(u32)eAssetType::pxl_shader] = GOSNEW(localAllocator, Loader_pxlShader)();
        loaderList[(u32)eAssetType::tex2D] = GOSNEW(localAllocator, Loader_tex2D)();
        loaderList[(u32)eAssetType::pipe] = GOSNEW(localAllocator, Loader_pipeline)();
    }

    //lista degli asset noti
    KnownAssets listof_knownAssets;
    listof_knownAssets.setup (localAllocator);
    loaderInfo.listof_knownAssets = &listof_knownAssets;


    //loop
    static constexpr u8 NUM_MAX_MESSAGES = 64;
    thread::sMsg msgList[NUM_MAX_MESSAGES];
    bool bQuit = false;
    while (bQuit == false)
    {
        if (!thread::waitForAnEvent (msgqR, u32MAX))
            break;

        u32 nMsg;
        while (0 != (nMsg = thread::popMultipleMsg(msgqR, msgList, NUM_MAX_MESSAGES)))
        {
            for (u32 i=0; i<nMsg; i++)
            {
                switch (msgList[i].what)
                {
                default:
                    DBGBREAK;
                    break;

                case MSG_FOR_LOADER_THREAD__DIE:
                    bQuit = true;
                    break;

                case MSG_FOR_LOADER_THREAD__LOAD:
                    {
                        asset2::UID uid;
                        uid._uid = msgList[i].paramU64;

                        loaderInfo.logger->log (eTextColor::darkGreen, "asset::MT  [%s] %016" PRIX64 " do load\n", asset2::enumToString(uid.getAssetType()), uid._uid);

                        BaseLoader *loader = loaderList[(u32)uid.getAssetType()];
                        assert (NULL != loader);
                        if (loader->load (loaderInfo, uid, msgList[i].buffer))
                            thread::pushMsg (msgqW, MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_OK, uid._uid, msgList[i].buffer, msgList[i].bufferSize);
                        else
                            thread::pushMsg (msgqW, MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_KO, uid._uid, msgList[i].buffer, msgList[i].bufferSize);
                    }
                    break;
                }

                thread::deleteMsg (msgList[i]);
            }

            if (bQuit)
                break;
        }
    }

    //TODO:: eliminare tutti i messaggi pendendi nel caso in cui ci siano ancora
    //risorse da unloadare


    //free dei loader
    for (u32 i=0; i<NUM_MAX_LOADER; i++)
    {
        if (NULL != loaderList[i])
        {
            GOSDELETE(localAllocator, loaderList[i]);
            loaderList[i] = NULL;
        }
    }

    listof_knownAssets.unsetup();

    //free dell'allocator
    GOSDELETE(gos::getSysHeapAllocator(), localAllocator);
    return 0;
}

