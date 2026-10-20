#include "gosEngineRenderPipe_PIPE3.h"
#include "../gosEngine.h"
#include <algorithm>

using namespace gos;
using namespace gos::engine;



//**********************************
Renderer_PIPE3::Renderer_PIPE3()
{
    engine = NULL;
    gpu = NULL;
    localAllocator = NULL;
    material_buffer = NULL;
	material_update_ct = NULL;
    pRenderableList = NULL;
    nRenderable = 0;
    renderer_UID = 0xFF;
}

//**********************************
void Renderer_PIPE3::priv_unsetup()
{
	if (NULL == engine)
	{
		return;
	}

	engine->release (handle_pipeline);

    GOSFREE(localAllocator, material_buffer);
	GOSFREE(localAllocator, material_update_ct);
    GOSFREE(localAllocator, pRenderableList);
    material_bitmask.unsetup (localAllocator);
    
    gpu->deleteResource(handle_sbo_matrixList);
    gpu->deleteResource(handle_sbo_materiaList);
    gpu->deleteResource(handle_sbo_instanceData);
    gpu->deleteResource(handle_descrSet2);

    engine = NULL;
    gpu = NULL;
}


//**********************************
bool Renderer_PIPE3::on__attach (const RPIPE::Context &ctx, u8 renderer_UID_IN)
{
	//load pipe
	if (!ctx.engine->pipeline_createFromAsset ("gosengine_PIPE3", &handle_pipeline, res2::eLoadMode::asap))
	{
		DBGBREAK;
        return false; 
	}	

	localAllocator = ctx.allocator;
	engine = ctx.engine;
	gpu = engine->gpu;
    renderer_UID = renderer_UID_IN;


    //SBO matrici
    matrix_sizeof_buffer = NUM_MAX_MATRIX * sizeof(mat4x4f);
    matrix_default.identity();
    gpu->storageBuffer_create (matrix_sizeof_buffer, eMemAccessMode::shared_cpuW, &handle_sbo_matrixList);
    //gpu->map (handle_sbo_matrixList, 0, u32MAX, &matrix_buffer);
	

    //SBO materialList
    material_sizeof_buffer = NUM_MAX_MATERIAL * sizeof(Material);
    material_buffer = (Material*) GOSALIGNEDALLOC(localAllocator, material_sizeof_buffer, gpu->limits_get_minStorageBufferOffsetAlignment());
	material_update_ct = GOSALLOCT(u32*, localAllocator, sizeof(u32)* NUM_MAX_MATERIAL);
	memset (material_update_ct, 0xFF, sizeof(u32)* NUM_MAX_MATERIAL);
    material_bitmask.setup (localAllocator, NUM_MAX_MATERIAL);
    material_bitmask.zero();
    material_upd_info.reset();
    material_default.texture_index = 0;
    material_default.diffuse_col.set (1.0f, 0.1f, 1.0f);
    gpu->storageBuffer_create (material_sizeof_buffer, eMemAccessMode::shared_cpuW, &handle_sbo_materiaList);

    ///SBO instance data
    //instance_sizeof_buffer = NUM_MAX_MATRIX * sizeof(InstanceData);
    instance_sizeof_buffer = NUM_MAX_MATRIX * sizeof(u64);
    gpu->storageBuffer_create (instance_sizeof_buffer, eMemAccessMode::shared_cpuW, &handle_sbo_instanceData);
    //gpu->map (handle_sbo_instanceData, 0, u32MAX, &instance_buffer);
    pRenderableList = GOSALLOCT(u64*, localAllocator, sizeof(u64) * NUM_MAX_MATRIX);

    //attendo che la pipe sia stata caricata perche' mi servono le definizioni dei descrittori
    const PipelineData *pipeline_data;
    if (!engine->get (handle_pipeline, &pipeline_data, 5000))
	{
		DBGBREAK;
		return false;
	}

	//alloco una istanza dei descriptor-set
	gos::gpu::DescrSetInstanceWriter dsw;

	//descriptor set 2        
	if (!gpu->descrSetInstance_create (ctx.handle_descrPool, pipeline_data->pipe_handle, 2, &handle_descrSet2))
	{
		gos::logger::err ("Renderer_PIPE3::setup() => can't create an instance of descriptorSet_2\n");
		return false;
	}
	else
	{
		dsw.begin (gpu, handle_descrSet2)
			.bindStorageBuffer (0, handle_sbo_matrixList, 0)
			.bindStorageBuffer (1, handle_sbo_materiaList, 0)
			.bindStorageBuffer (2, handle_sbo_instanceData)
			.end();
	}

    material_default_index = material_create (material_default.texture_index, material_default.diffuse_col);
    return true;
}

//**********************************
u32 Renderer_PIPE3::material_create_empty()
{
    u32 material_index;
    if (!material_bitmask.findAndSetFirstFreeBit(&material_index))
    {
        DBGBREAK;
        return u32MAX;
    }
    
    //creo il nuovo materiale
	material_update_ct[material_index] = u32MAX;
	material_buffer[material_index].diffuse_col.set(1,1,1);
	material_buffer[material_index].texture_index = 0;
    
    //mi segno che l'array dei materiali e' da aggiornare su GPU
    material_upd_info.mark_updated (material_index);
    return material_index;
}

//**********************************
u32 Renderer_PIPE3::material_create (u32 texture_index, const vec3f diffuse_col_HDR_RGB)
{
    const u32 material_index = material_create_empty();
    if (u32MAX == material_index)
        return u32MAX;
    
    material_buffer[material_index].texture_index = texture_index;
    material_buffer[material_index].diffuse_col = diffuse_col_HDR_RGB;
    
    return material_index;
}

//**********************************
void Renderer_PIPE3::material_delete (u32 material_index)
{
    material_bitmask.clear (material_index);
}

//**********************************
Renderer_PIPE3::Material* Renderer_PIPE3::material_getForUpdate (u32 material_index)
{
    if (material_bitmask.isBitSet(material_index))
    {
        //mi segno che l'array dei materiali e' da aggiornare su GPU
        material_upd_info.mark_updated (material_index);
        return &material_buffer[material_index];
    }
    DBGBREAK;
    return &material_default;
}

//**********************************
const Renderer_PIPE3::Material* Renderer_PIPE3::material_query (u32 material_index) const
{
    if (material_bitmask.isBitSet(material_index))
        return &material_buffer[material_index];

    DBGBREAK;
    return &material_default;
}

//**********************************
u64 Renderer_PIPE3::priv_pack_renderable (GPUShapeH shape, u32 material_index, u32 matrix_index) const
{
	u64 ret = shape.viewAsU32();
    ret <<= 32;

    //14bit per material_index (16.384)
    ret |= (u64)(material_index & 0x3FFF) << 18;

    //18bit per matrix_index (262.144)
    ret |= (u64)(matrix_index & 0x3FFFF);

    return ret;
}

//**********************************
void Renderer_PIPE3::priv_unpack_renderable (u64 packed, GPUShapeH *out_shape, u32 *out_material_index, u32 *out_matrix_index) const
{
    out_shape->setFromU32 ( (u32)(packed >> 32) );
    *out_material_index = (u32) ((packed >> 18) & 0x3FFF);
    *out_matrix_index = (u32)(packed & 0x3FFFF);
}

//**********************************
void Renderer_PIPE3::priv_material_update_from_PBR (u32 material_index, const MaterialPBRData *material)
{
	material_upd_info.mark_updated (material_index);

	material_update_ct[material_index] = material->update_ct;	
	material_buffer[material_index].diffuse_col.set( material->diffuse_col_HDR_RGBA[0], material->diffuse_col_HDR_RGBA[1], material->diffuse_col_HDR_RGBA[2] );
    material_buffer[material_index].texture_index = material->diffuse_texture->index;
}

//**********************************
void Renderer_PIPE3::begin ()
{
    matrix_nextIndex = 0;
    nRenderable = 0;
	gpu->begin_write (handle_sbo_matrixList, &mapped_matrix_buffer);
}

//**********************************
void Renderer_PIPE3::add (const GPUShapeH handle_shape, const mat4x4f &m, MaterialPBRH handle_material)
{
	const MaterialPBRData *res_mat;
	if (!engine->get (handle_material, &res_mat))
		return;

    u32 material_index = res_mat->renderer_bindings[renderer_UID];
    if (u32MAX == material_index)
    {
		MaterialPBRDataUpd upd;
		if (engine->get_for_update (handle_material, &upd))
		{
			//questo materiale e' la prima volta che lo vedo
			material_index = material_create_empty();
			engine->materialPBR_update_renderer_binding (&upd, renderer_UID, material_index);
            return;
        }
		else
		{
			DBGBREAK;
			return;
		}
    }

	if (res_mat->update_ct != material_update_ct[material_index])
		priv_material_update_from_PBR (material_index, res_mat);

	add (handle_shape, m, material_index);
}

//**********************************
void Renderer_PIPE3::add (const GPUShapeH handle_shape, const mat4x4f &m, u32 material_index)
{
    if (matrix_nextIndex >= NUM_MAX_MATRIX)
        return;
    const u32 matrix_index = matrix_nextIndex++;

    //u8 *p = reinterpret_cast<u8*>(matrix_buffer.host_pt);
    //memcpy (&p[sizeof(mat4x4f) * matrix_index], m._getValuesPtConst(), sizeof(mat4x4f));
	mapped_matrix_buffer.write (m._getValuesPtConst(), sizeof(mat4x4f), sizeof(mat4x4f) * matrix_index);

    pRenderableList[nRenderable++] = priv_pack_renderable (handle_shape, material_index, matrix_index);
}

//**********************************
void Renderer_PIPE3::add (const ModelH handle_model, const mat4x4f &m)
{
	const ModelData *model_data;
	if (!engine->get (handle_model, &model_data))
		return;

	model::Reader reader (&model_data->model);
	const u32 num_mesh = reader.mesh_get_num();

    //addo le shape al renderer
	for (u32 i=0; i<num_mesh; i++)
	{
		const Model::Mesh *mesh = reader.mesh_get_by_index(i);
		const u16 shape_index = mesh->shape_index;
        const u16 material_index = mesh->material_index;

		GPUShapeH shape_handle = reader.gpushape_get_by_index(shape_index);
		MaterialPBRH material_handle = reader.material_get_by_index (material_index);

		add (shape_handle, m, material_handle);
	}
}

//**********************************
void Renderer_PIPE3::end ()
{
	mapped_matrix_buffer.end();

    //devo aggiornare SBO dei materiali?
    if (material_upd_info.was_updated())
    {
        //Non c'e' bisogno di uppare l'intero material_buffer tutte le volte, idealmente basta uppare solo
        //gli elementi che sono stati modificati
		//A tale proposito, tengo traccia degli indici dei materiali aggiornati e uppo solo quelli
		//TODO: forse si puo' ottimizzare ulteriormante, ma non so se ne vale la pena

		const u32 first_updated_material = material_upd_info.min_index;
		const u32 size =  sizeof(Material) * (material_upd_info.max_index - material_upd_info.min_index +1);
        material_upd_info.reset();

		gpu::MappedBufW mm;
		gpu->begin_write (handle_sbo_materiaList, &mm);
			//mm.write (material_buffer, material_sizeof_buffer, 0);
			mm.write (&material_buffer[first_updated_material], size, sizeof(Material) * first_updated_material);
			mm.end();
        
    }


    //sort dei renderabili
	if (nRenderable)
	{
    	std::sort (pRenderableList, &pRenderableList[nRenderable]);

    	//memcpio nel SSBO
    	//memcpy (instance_buffer.host_pt, pRenderableList, sizeof(u64) *nRenderable);
    	//gpu->buffer_manualSync_cpuWrite (instance_buffer, 0, u32MAX);

		gpu::MappedBufW mm;
		gpu->begin_write (handle_sbo_instanceData, &mm);
			mm.write (pRenderableList, sizeof(u64) *nRenderable, 0);
			mm.end();
	}

	
}

//**********************************
void Renderer_PIPE3::priv_do_render (const RPIPE::Context &ctx, gpu::RenderCtx &rctx)
{
    if (0 == nRenderable)
	{
		return;
	}
	
    const PipelineData *pipeline_data;
    if (!engine->get (handle_pipeline, &pipeline_data))
    {
        return;
    }


    //command
    rctx.bindPipeline (pipeline_data->pipe_handle)
        .bindDescriptorSet (ctx.handle_descrSet0, 0)
        .bindDescriptorSet (ctx.handle_descrSet1, 1)
        .bindDescriptorSet (handle_descrSet2, 2);


    //render delle shape
    u32 cur_index = 0;
    u32 first_instance_index = 0;
    while (cur_index < nRenderable)
    {
        GPUShapeH cur_shape_handle;
        u32 material_index;
        u32 matrix_index;
        priv_unpack_renderable (pRenderableList[cur_index], &cur_shape_handle, &material_index, &matrix_index);
        cur_index++;

        //conto quante shape identiche a cur_shape_handle ci sono
        u32 numInstances = 1;
        while (cur_index < nRenderable)
        {
            GPUShapeH shape_handle;
            priv_unpack_renderable (pRenderableList[cur_index], &shape_handle, &material_index, &matrix_index);
            if (cur_shape_handle == shape_handle)
            {
                cur_index++;
                numInstances++;
            }
            else
                break;
        }


        const GPUShapeData *cur_shape_info;
        if (engine->get (cur_shape_handle, &cur_shape_info))
        {
            rctx.bindVtxIdxBuffer (cur_shape_info->vbHandle, 0, cur_shape_info->ibHandle, 0)
                .drawIndexed (cur_shape_info->numIndices, numInstances, cur_shape_info->indexStart, cur_shape_info->vtxStart, first_instance_index);

            first_instance_index += numInstances;
        }
    }
}


