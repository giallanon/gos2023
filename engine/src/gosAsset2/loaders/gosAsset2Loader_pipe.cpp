#include "gosAsset2Loader_pipe.h"
#include "gosAsset2Loader_shader.h"
#include "../gosAsset2Loader.h"
#include "../gosAsset2Hub.h"
#include "../gosAsset2.h"
#include "../gos/gosBufferReader.h"
#include "../gos/gosDataBlob.h"


using namespace gos;
using namespace gos::asset2;

//******************************************************
void Loader_pipe::unload (Loader *assetLoader, const DBContext &ctx, const UID &uid, void *ptToAssetData)
{
	Asset_pipe *asset = static_cast <Asset_pipe*>(ptToAssetData);
	
    gos::GPU *gpu = assetLoader->getGPU();
    gpu->deleteResource (asset->handle_pipe);
}

//******************************************************
bool Loader_pipe::load (Loader *assetLoader, const DBContext &ctx, const UID &uid, void *in_out_asset)
{
	Asset_pipe *out = static_cast <Asset_pipe*>(in_out_asset);
	assert (uid.isValid());
	assert (uid.isAnAssetOfType(this->getAssetType()));


	char s[1024];
	asset_manufacture_fullFilename (ctx, uid, s, sizeof(s));
    u32 fsize;
    u8 *buffer = fs::fileLoadInMemory (gos::getScrapAllocator(), s, &fsize);
    if (NULL == buffer)
    {
        logger::err ("Loader_pipe::load() => file not found %s\n", s);
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
            logger::err ("Loader_pipe::load() => invalid magic for file %s\n", s);
            break;
        }

        gpu::pipe2::Pipeline_def def;
        def.reset();


        //uid vtx shader
        //L'assert e' garantito che sia gia' stato caricato, ci ha pensato TheHub
        UID uid;
        const Asset_shader *shader;
        uid._uid = reader.readU64 ();
        assetLoader->getTheHub()->internalUSE_getExistingAssetByUID(uid, &shader);
        //out->handle_vtxshader = shader->handle_shader;
        def.shader_add (shader->handle_shader);
        

        //uid pxl shader
        uid._uid = reader.readU64 ();
        assetLoader->getTheHub()->internalUSE_getExistingAssetByUID(uid, &shader);
        //out->handle_pxlshader = shader->handle_shader;
        def.shader_add (shader->handle_shader);

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
        if (!assetLoader->getGPU()->pipeline_createNew (def, &out->handle_pipe))
            return false;




        //finito
        ret = true;
        break;
    }
    GOSFREE_SCRAP(buffer);
    return ret;
}