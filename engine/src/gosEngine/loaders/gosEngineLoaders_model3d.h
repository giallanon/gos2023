#ifndef _gosEngineLoaders_model3d_h_
#define _gosEngineLoaders_model3d_h_
#include "gosEngineLoaders_shader.h"
#include "../gosEngine.h"

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
                bool    load (LoaderInfo &loaderInfo, asset2::UID uid, void *res_dataIN)
                {
                    ResModel3d *res_data = static_cast<ResModel3d*>(res_dataIN);
                    //gos::GPU *gpu = loaderInfo.gpu;
					Engine *eng = loaderInfo.engine;

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
                        //const u32 start_of_list_of_material_uid = reader.tell();
                        reader.advanceCursor (sizeof(u64) * num_materials);


                        const u32 num_meshes = reader.readU32();
						const u32 start_of_list_of_mesh_uid = reader.tell();

						if (!model::alloc (loaderInfo.engine_allocator, num_shapes, num_materials, num_meshes, &res_data->data.model))
							break;

						//skeleton
						ENGSkeleton handle_skeleton;
						ResSkeleton	*res_skeleton;
						if (!eng->internal__from_asset_to_raw_data (uid_of_skeleton, &handle_skeleton, &res_skeleton))
						{
							logger::log (eTextColor::red, "asset::  Loader_model3d::load() => unable to match skeleton_shader %016" PRIX64 " with raw data\n", uid_of_skeleton._uid);
							break;
						}
						model::set_skeleton (res_data->data.model, handle_skeleton);
						
						//shapes
						ret = true;
						reader.moveCursorTo(start_of_list_of_shape_uid);
						for (u32 i=0; i<num_shapes; i++)
						{
							asset2::UID uid_shape;
							uid_shape._uid = reader.readU64();

							ENGShape handle_shape;
							ResShape *res_shape;
							if (!eng->internal__from_asset_to_raw_data (uid_shape, &handle_shape, &res_shape))
							{
								logger::log (eTextColor::red, "asset::  Loader_model3d::load() => unable to match shape %016" PRIX64 " with raw data\n", uid_shape._uid);
								ret = false;
								break;
							}

							// vec3f bbmin, bbmax;
							// shape::shapeCalcAABB (&res_shape->data.shape, &bbmin, &bbmax);
							
							//creo la GPUshape (se non esise gia')
							ENGGPUShape handle_gpuShape;
							if (!eng->GPUShape_create (handle_shape, loaderInfo.stageHelper, &handle_gpuShape))
							{
								logger::log (eTextColor::red, "asset::  Loader_model3d::load() => error creating GPUShape from shape %016" PRIX64 "\n", uid_shape._uid);
								ret = false;
								break;
							}
							model::set_gpushape (res_data->data.model, i, handle_gpuShape);
						}
						if (!ret)
							break;
						ret = false;

						//meshes
						reader.moveCursorTo(start_of_list_of_mesh_uid);
						for (u32 i=0; i<num_meshes; i++)
						{
							const u32 index_of_concrete_shape = reader.readU32();
							const u32 bone_index = reader.readU32();
							const u32 my_material_index = reader.readU32();

							//model::set_mesh (res_data->data.model, i, (u16)index_of_concrete_shape, (u16)bone_index, (u16)my_material_index);
							model::set_mesh (res_data->data.model, i, (u16)index_of_concrete_shape, (u16)bone_index, 0);
						}


						//fine di while(1)
						ret = true;
						break;
                    }

                    return ret;
                }
            };


        } //namespace loaders
    } //namespace engine
} //namespace gos


#endif //_gosEngineLoaders_model3d_h_
