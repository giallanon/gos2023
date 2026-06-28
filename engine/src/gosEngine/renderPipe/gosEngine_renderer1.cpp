#include "gosEngine_renderer1.h"
#include "../gosEngine.h"
#include <algorithm>

using namespace gos;
using namespace gos::engine;



//**********************************
Renderer1::Renderer1()
{
    engine = NULL;
    gpu = NULL;
    localAllocator = NULL;
    material_buffer = NULL;
    pRenderableList = NULL;
    nRenderable = 0;
}

//**********************************
void Renderer1::priv_unsetup()
{
	if (NULL == engine)
	{
		return;
	}

	engine->release (handle_pipeline);

    gpu->buffer_unmap (matrix_buffer);
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

//***************************************
void Renderer1::on__detach (const RPIPE::Context &ctx)
{
	priv_unsetup();
}

//**********************************
bool Renderer1::on__attach (const RPIPE::Context &ctx)
{
	//load pipe
	if (!ctx.engine->pipeline_createFromAsset ("gosengine_renderer1", &handle_pipeline, res::eLoadMode::asap))
	{
		DBGBREAK;
        return false; 
	}	

	localAllocator = ctx.allocator;
	engine = ctx.engine;
	gpu = engine->gpu;


    //SBO matrici
    matrix_sizeof_buffer = NUM_MAX_MATRIX * sizeof(mat4x4f);
    matrix_default.identity();
    gpu->storageBuffer_create (matrix_sizeof_buffer, eMemAccessMode::shared_cpuW_manualSync, &handle_sbo_matrixList);
    gpu->map (handle_sbo_matrixList, 0, u32MAX, &matrix_buffer);

    //SBO materialList
    material_sizeof_buffer = NUM_MAX_MATERIAL * sizeof(Material);
    material_buffer = (Material*) GOSALIGNEDALLOC(localAllocator, material_sizeof_buffer, gpu->limits_get_minStorageBufferOffsetAlignment());
    material_bitmask.setup (localAllocator, NUM_MAX_MATERIAL);
    material_bitmask.zero();
    material_wasUpdated = 1;
    material_default.texture_index = 0;
    material_default.diffuse_col.set (1.0f, 1.0f, 1.0f);
    gpu->storageBuffer_create (material_sizeof_buffer, eMemAccessMode::shared_cpuW_autoSync, &handle_sbo_materiaList);

    ///SBO instance data
    //instance_sizeof_buffer = NUM_MAX_MATRIX * sizeof(InstanceData);
    instance_sizeof_buffer = NUM_MAX_MATRIX * sizeof(u64);
    gpu->storageBuffer_create (instance_sizeof_buffer, eMemAccessMode::shared_cpuW_manualSync, &handle_sbo_instanceData);
    gpu->map (handle_sbo_instanceData, 0, u32MAX, &instance_buffer);
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
		gos::logger::err ("Renderer1::setup() => can't create an instance of descriptorSet_2\n");
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


    return true;
}

//**********************************
u32 Renderer1::material_create (u32 texture_index, const vec3f diffuse_col)
{
    u32 material_index;
    if (!material_bitmask.findAndSetFirstFreeBit(&material_index))
    {
        DBGBREAK;
        return u32MAX;
    }
    
    //creo il nuovo materiale
    material_buffer[material_index].texture_index = texture_index;
    material_buffer[material_index].diffuse_col = diffuse_col;
    
    //mi segno che l'array dei materiali e' da aggiornare su GPU
    material_wasUpdated = 1;
    
    return material_index;
}

//**********************************
void Renderer1::material_delete (u32 material_index)
{
    material_bitmask.clear (material_index);
}

//**********************************
Renderer1::Material* Renderer1::material_getForUpdate (u32 material_index)
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
const Renderer1::Material* Renderer1::material_query (u32 material_index) const
{
    if (material_bitmask.isBitSet(material_index))
        return &material_buffer[material_index];

    DBGBREAK;
    return &material_default;
}

//**********************************
u64 Renderer1::priv_pack_renderable (ENGGPUShape shape, u32 material_index, u32 matrix_index) const
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
void Renderer1::priv_unpack_renderable (u64 packed, ENGGPUShape *out_shape, u32 *out_material_index, u32 *out_matrix_index) const
{
    out_shape->setFromU32 ( (u32)(packed >> 32) );
    *out_material_index = (u32) ((packed >> 18) & 0x3FFF);
    *out_matrix_index = (u32)(packed & 0x3FFFF);
}

//**********************************
void Renderer1::begin ()
{
    matrix_nextIndex = 0;
    nRenderable = 0;
}

//**********************************
void Renderer1::add (const ENGGPUShape shape, const mat4x4f &m, u32 material_index)
{
    if (matrix_nextIndex >= NUM_MAX_MATRIX)
        return;
    const u32 matrix_index = matrix_nextIndex++;

    u8 *p = reinterpret_cast<u8*>(matrix_buffer.host_pt);
    memcpy (&p[sizeof(mat4x4f) * matrix_index], m._getValuesPtConst(), sizeof(mat4x4f));

    //u64 *pRenderableList = static_cast<u64*>(instance_buffer.host_pt);
    pRenderableList[nRenderable++] = priv_pack_renderable (shape, material_index, matrix_index);
}

//**********************************
void Renderer1::add (const ent::CompModelInstance *comp_mi)
{
	add (comp_mi->handle_mi);
}

//**********************************
void Renderer1::add (gos::ENGModel3dInst handle)
{
	const res::Model3dInst *res_mi;
	if (!engine->get (handle, &res_mi))
		return;

	const gos::ModelInstance *mi = &res_mi->minst;
	for (u32 i=0; i<mi->num_meshes; i++)
	{
		const Model::Mesh *mesh = &mi->listof_meshes[i];
		
		add(	mi->listof_gpushapes[mesh->shape_index],
				mi->listof_bones[mesh->bone_index].matrix,
				mesh->material_index);
	}
}

//**********************************
void Renderer1::end ()
{
}

//**********************************
void Renderer1::priv_do_render (const RPIPE::Context &ctx, gpu::RenderCtx &rctx)
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

    //devo aggiornare SBO dei materiali?
    if (material_wasUpdated)
    {
        material_wasUpdated = 0;

        //TODO: non c'e' bisogno di uppare l'intero material_buffer tutte le volte, idealmente basta uppare solo
        //gli elementi che sono stati modificati
        gpu->writeAndSync (handle_sbo_materiaList, 0, material_buffer, material_sizeof_buffer);
    }

    //devo aggiornare SBO delle matrici
    if (matrix_nextIndex)
    {
        gpu->buffer_manualSync_cpuWrite (matrix_buffer, 0, sizeof(mat4x4f) * matrix_nextIndex);
    }    

    

    //sort
    std::sort (pRenderableList, &pRenderableList[nRenderable]);

    //memcpio nel SSBO
    memcpy (instance_buffer.host_pt, pRenderableList, sizeof(u64) *nRenderable);
    gpu->buffer_manualSync_cpuWrite (instance_buffer, 0, u32MAX);




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

//**********************************
void Renderer1::on__render (const RPIPE::Context &ctx, gos::gpu::RenderCtx &rctx)
{
	priv_do_render (ctx, rctx);
}
