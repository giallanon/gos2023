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
                eResult load (LoaderInfo &loaderInfo, void *resIN, CallbackData *in_out__callback_data)
                {
                    res::Model3d *res_model = static_cast<res::Model3d*>(resIN);
                    gos::Allocator *thread_allocator = loaderInfo.thread_allocator;
					Engine *eng = loaderInfo.engine;

                    char s[1024];
                    asset2::asset_manufacture_fullFilename (*loaderInfo.ctx, res_model->_descr.uid, s, sizeof(s));

                    u32 fsize;
                    u8 *buffer = fs::fileLoadInMemory (thread_allocator, s, &fsize);
                    if (NULL == buffer)
                    {
                        logger::err ("Loader_model3d::load() => file not found %s\n", s);
                        return eResult::failed;
                    }

                    gos::BufferR reader;
                    reader.setup (buffer, fsize);
                    
                    eResult ret = eResult::failed;
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
						const u32 start_of_list_of_mesh_uid = reader.tell();

						if (!model::alloc (loaderInfo.engine_allocator, num_shapes, num_materials, num_meshes, &res_model->model))
							break;

						//skeleton
						ENGSkeleton handle_skeleton;
						res::Skeleton *res_skeleton;
						if (!eng->internal__getResFromUID (uid_of_skeleton, &res_skeleton, &handle_skeleton))
						{
							logger::log (eTextColor::red, "resMT::  Loader_model3d::load() => unable to match skeleton %016" PRIX64 " with raw data\n", uid_of_skeleton._uid);
							break;
						}
						model::set_skeleton (res_model->model, handle_skeleton);
						

						//shapes
						ret = eResult::success;
						reader.moveCursorTo(start_of_list_of_shape_uid);
						for (u32 i=0; i<num_shapes; i++)
						{
							asset2::UID uid_shape;
							uid_shape._uid = reader.readU64();

							ENGShape handle_shape;
							res::Shape *res_shape;
							if (!eng->internal__getResFromUID(uid_shape, &res_shape, &handle_shape))
							{
								logger::log (eTextColor::red, "resMT::  Loader_model3d::load() => unable to match shape %016" PRIX64 " with raw data\n", uid_shape._uid);
								ret = eResult::failed;
								break;
							}

							// vec3f bbmin, bbmax;
							// shape::shapeCalcAABB (&res_shape->data.shape, &bbmin, &bbmax);
						}
						if (eResult::failed == ret)
							break;


						//materials
						reader.moveCursorTo(start_of_list_of_material_uid);
						for (u32 i=0; i<num_materials; i++)
						{
							asset2::UID uid_materialPBR;
							uid_materialPBR._uid = reader.readU64();

							ENGMaterialPBR handle_materialPBR;
							res::MaterialPBR *res_materialPBR;
							if (!eng->internal__getResFromUID(uid_materialPBR, &res_materialPBR, &handle_materialPBR))
							{
								logger::log (eTextColor::red, "resMT::  Loader_model3d::load() => unable to match material %016" PRIX64 " with raw data\n", uid_materialPBR._uid);
								ret = eResult::failed;
								break;
							}

							model::set_material (res_model->model, i, handle_materialPBR);
						}
						if (eResult::failed == ret)
							break;




						//meshes
						reader.moveCursorTo(start_of_list_of_mesh_uid);
						for (u32 i=0; i<num_meshes; i++)
						{
							const u32 index_of_concrete_shape = reader.readU32();
							const u32 bone_index = reader.readU32();
							const u32 local_material_index = reader.readU32();

							model::set_mesh (res_model->model, i, (u16)index_of_concrete_shape, (u16)bone_index, local_material_index);
						}



						//fine di while(1)
						break;
                    }


					if (eResult::success == ret)
					{
						//chiamo la callback perche' il modello deve aggiungere le risorse di tipo GPUShape
						//e questa cosa va fatta nel main thread
						in_out__callback_data->user_data_pt = buffer;
						in_out__callback_data->user_data_1 = fsize;
						return eResult::callback;
					}


					GOSFREE(thread_allocator, buffer);
                    return ret;
                }
            
			
				//*********************************************************
				bool	load_continued (LoaderInfo &loaderInfo, bool anyError, CallbackData *callback_data)
				{
                    gos::Allocator *thread_allocator = loaderInfo.thread_allocator;
					u8 *buffer = (u8*)callback_data->user_data_pt;
					GOSFREE(thread_allocator, buffer);
                    return !anyError;
				}
			};


        } //namespace loaders
    } //namespace engine
} //namespace gos


#endif //_gosEngineLoaders_model3d_h_
