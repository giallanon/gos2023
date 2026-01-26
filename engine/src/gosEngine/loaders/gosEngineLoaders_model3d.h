#ifndef _gosEngineLoaders_model3d_h_
#define _gosEngineLoaders_model3d_h_
#include "gosEngineLoaders_shader.h"

namespace gos
{
    namespace engine
    {
        namespace loaders
        {
            //********************************************************
            class Loader_model3d : public loaders::BaseLoader
            {
            public:
                bool    load (LoaderInfo &loaderInfo, asset2::UID uid, void *out_dataIN)
                {
                    ResModel3d::DataForLoaderThread *out_data = static_cast<ResModel3d::DataForLoaderThread*>(out_dataIN);
                    gos::GPU *gpu = loaderInfo.gpu;

                    char s[1024];
                    asset2::asset_manufacture_fullFilename (*loaderInfo.ctx, uid, s, sizeof(s));

                    u32 fsize;
                    u8 *buffer = fs::fileLoadInMemory (gos::getScrapAllocator(), s, &fsize);
                    if (NULL == buffer)
                    {
                        logger::err ("Loader_model3d::load() => file not found %s\n", s);
                        return false;
                    }

                    gos::BufferR reader;
                    reader.setup (buffer, fsize);
                    
                    bool ret = false;
                    while (1)
                    {
                        const u32 magic = reader.readU32();
                        if (!magic::signatureMatch(magic, GOS_MAGIC__ASSET_MODEL3D) || !magic::versionMatch(magic, GOS_MAGIC__ASSET_MODEL3D))
                        {
                            logger::err ("Loader_model3d::load() => invalid magic for file %s\n", s);
                            break;
                        }


                        asset2::UID uid_of_skeleton;
                        uid_of_skeleton._uid = reader.readU64();

                        const u32 num_shapes = reader.readU32();
                        const u32 start_of_list_of_shape_uid = reader.tell();
                        reader.advanceCursor (sizeof(u64) * num_shapes);

                        const u32 num_materials = reader.readU32();
                        const u32 start_of_list_of_material_uid = reader.tell();
                        reader.advanceCursor (sizeof(u64) * num_materials);


                        const u32 num_meshes = reader.readU32();


                    }

                    //mi aggiungo alla lista degli asset noti
                    loaderInfo.listof_knownAssets->add_or_replace (uid, &out_data->data, sizeof(out_data->data));
                    return true;
                }
            };


        } //namespace loaders
    } //namespace engine
} //namespace gos


#endif //_gosEngineLoaders_model3d_h_
