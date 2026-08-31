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
	if (!ctx.engine->pipeline_createFromAsset ("gosengine_PIPE3", &handle_pipeline, res::eLoadMode::asap))
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
    material_bitmask.setup (localAllocator, NUM_MAX_MATERIAL);
    material_bitmask.zero();
    material_wasUpdated = 1;
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
    const res::Pipeline *res_pipeline;
    if (!engine->get (handle_pipeline, &res_pipeline, 5000))
	{
		DBGBREAK;
		return false;
	}

	//alloco una istanza dei descriptor-set
	gos::gpu::DescrSetInstanceWriter dsw;

	//descriptor set 2        
	if (!gpu->descrSetInstance_create (ctx.handle_descrPool, res_pipeline->pipeHandle, 2, &handle_descrSet2))
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
u32 Renderer_PIPE3::material_create (u32 texture_index, const vec3f diffuse_col_HDR_RGB)
{
    u32 material_index;
    if (!material_bitmask.findAndSetFirstFreeBit(&material_index))
    {
        DBGBREAK;
        return u32MAX;
    }
    
    //creo il nuovo materiale
    material_buffer[material_index].texture_index = texture_index;
    material_buffer[material_index].diffuse_col = diffuse_col_HDR_RGB;
    
    //mi segno che l'array dei materiali e' da aggiornare su GPU
    material_wasUpdated = 1;
    
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
        material_wasUpdated = 1;        
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
u64 Renderer_PIPE3::priv_pack_renderable (ENGGPUShape shape, u32 material_index, u32 matrix_index) const
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
void Renderer_PIPE3::priv_unpack_renderable (u64 packed, ENGGPUShape *out_shape, u32 *out_material_index, u32 *out_matrix_index) const
{
    out_shape->setFromU32 ( (u32)(packed >> 32) );
    *out_material_index = (u32) ((packed >> 18) & 0x3FFF);
    *out_matrix_index = (u32)(packed & 0x3FFFF);
}

//**********************************
u32 Renderer_PIPE3::priv_material_create_from_PBR (const res::MaterialPBR *material)
{
    const vec3f diffuse_col (material->diffuse_col_HDR_RGBA[0], material->diffuse_col_HDR_RGBA[1], material->diffuse_col_HDR_RGBA[2]);
    const u32 texture_index = 0;
    return material_create (texture_index, diffuse_col);
}

//**********************************
void Renderer_PIPE3::begin ()
{
    matrix_nextIndex = 0;
    nRenderable = 0;
	gpu->begin_write (handle_sbo_matrixList, &mapped_matrix_buffer);
}

//**********************************
void Renderer_PIPE3::add (const ENGGPUShape handle_shape, const mat4x4f &m, ENGMaterialPBR handle_material)
{
    const res::MaterialPBR *res_mat;
	if (!engine->get (handle_material, &res_mat))
		return;

    u32 material_index = res_mat->renderer_bindings[renderer_UID];
    if (u32MAX == material_index)
    {
        //questo materiale e' la prima volta che lo vedo
        material_index = priv_material_create_from_PBR(res_mat);
        if (!engine->internal__materialPBR_update_renderer_binding (handle_material, renderer_UID, material_index))
        {
            DBGBREAK;
            return;
        }
    }

    add (handle_shape, m, material_index);
}

//**********************************
void Renderer_PIPE3::add (const ENGGPUShape handle_shape, const mat4x4f &m, u32 material_index)
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
void Renderer_PIPE3::add (const ent::CompModelInstance *comp_mi)
{
	add (comp_mi->handle_mi);
}

//**********************************
void Renderer_PIPE3::add (gos::ENGModel3dInst handle_mi)
{
	const res::Model3dInst *res_mi;
	if (!engine->get (handle_mi, &res_mi))
		return;

    const gos::ModelInstance *mi = &res_mi->minst;

    //addo le shape al renderer
	for (u32 i=0; i<mi->num_meshes; i++)
	{
		const Model::Mesh *mesh = &mi->listof_meshes[i];

        if (u16MAX != mesh->material_index)
        {
            add(mi->listof_gpushapes[mesh->shape_index],
                mi->listof_bones[mesh->bone_index].matrix,
                mi->listof_materials[mesh->material_index]);
        }
        else
        {
            add(mi->listof_gpushapes[mesh->shape_index],
                mi->listof_bones[mesh->bone_index].matrix,
                material_default_index);
        }
	}
}

//**********************************
void Renderer_PIPE3::end ()
{
	mapped_matrix_buffer.end();

    //devo aggiornare SBO dei materiali?
    if (material_wasUpdated)
    {
        material_wasUpdated = 0;

        //TODO: non c'e' bisogno di uppare l'intero material_buffer tutte le volte, idealmente basta uppare solo
        //gli elementi che sono stati modificati
		//gpu->writeAndSync (handle_sbo_materiaList, 0, material_buffer, material_sizeof_buffer);
		gpu::MappedBufW mm;
		gpu->begin_write (handle_sbo_materiaList, &mm);
			mm.write (material_buffer, material_sizeof_buffer, 0);
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
	
    const res::Pipeline *res_pipeline;
    if (!engine->get (handle_pipeline, &res_pipeline))
    {
        return;
    }


    //command
    rctx.bindPipeline (res_pipeline->pipeHandle)
        .bindDescriptorSet (ctx.handle_descrSet0, 0)
        .bindDescriptorSet (ctx.handle_descrSet1, 1)
        .bindDescriptorSet (handle_descrSet2, 2);


    //render delle shape
    u32 cur_index = 0;
    u32 first_instance_index = 0;
    while (cur_index < nRenderable)
    {
        ENGGPUShape cur_shape;
        u32 material_index;
        u32 matrix_index;
        priv_unpack_renderable (pRenderableList[cur_index], &cur_shape, &material_index, &matrix_index);
        cur_index++;

        //conto quante shape identiche a cur_shape ci sono
        u32 numInstances = 1;
        while (cur_index < nRenderable)
        {
            ENGGPUShape shape;
            priv_unpack_renderable (pRenderableList[cur_index], &shape, &material_index, &matrix_index);
            if (cur_shape == shape)
            {
                cur_index++;
                numInstances++;
            }
            else
                break;
        }


        const res::GPUShape *cur_shape_info;
        if (engine->get (cur_shape, &cur_shape_info))
        {
            rctx.bindVtxIdxBuffer (cur_shape_info->vbHandle, 0, cur_shape_info->ibHandle, 0)
                .drawIndexed (cur_shape_info->numIndices, numInstances, cur_shape_info->indexStart, cur_shape_info->vtxStart, first_instance_index);

            first_instance_index += numInstances;
        }
    }
}


