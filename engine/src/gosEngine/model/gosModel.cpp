#include "gosModel.h"
#include "../gos/gosMagicUID.h"
#include "../gosEngine.h"

using namespace gos;


//************************** 
bool model::isValid (const Model &m)
{
	if (NULL == m.allocator)
		return false;
	const u32 magic = utils::bufferReadU32 (m.blob);
	return (GOS_MAGIC__ENGINE_MODEL == magic);	
}

//************************** 
void model::free (Model &m)
{
	if (NULL == m.allocator)
		return;
	GOSFREE(m.allocator, m.blob);
	m.reset();
}

//************************** 
bool model::alloc (gos::Allocator *allocator, u16 num_shape, u16 num_material, u16 num_meshes, Model *out)
{
	assert (NULL != out);

	const u32 sizeof_blob =
		sizeof(u32)		//magic
		+sizeof(u32)	//total_size_of_blob
		
		+sizeof(u16)	//num_shape
		+sizeof(u16)	//num_material
		+sizeof(u16)	//num_meshes
		+sizeof(u16)	//abs-offset-to MESH 1
		+sizeof(u16)	//abs-offset-to MATERIAL 1
     	
		+sizeof(u32)	//ENGSkeleton as u32
     	+sizeof(u32) * num_shape //ENGGPUShape 1 to n
		+sizeof(u32) * num_material //ENGMaterialPBR 1 to n
		+sizeof(u16) * 4 * num_meshes;

	out->allocator = allocator;
	out->blob = GOSALLOCT(u8*, allocator, sizeof_blob);
	memset (out->blob, 0, sizeof_blob);

	u32 ct = 0;
	ct += utils::bufferWriteU32 (&out->blob[ct], GOS_MAGIC__ENGINE_MODEL);
	ct += utils::bufferWriteU32 (&out->blob[ct], sizeof_blob);
	
	assert (ct == Reader::OFFSET_TO_NUM_SHAPES);
	ct += utils::bufferWriteU16 (&out->blob[ct], num_shape);

	assert (ct == Reader::OFFSET_TO_NUM_MATERIAL);
	ct += utils::bufferWriteU16 (&out->blob[ct], num_material);

	assert (ct == Reader::OFFSET_TO_NUM_MESHES);
	ct += utils::bufferWriteU16 (&out->blob[ct], num_meshes);

	assert (ct == Reader::OFFSET_TO_START_OF_MESHES);
	ct += utils::bufferWriteU16 (&out->blob[ct], 0);

	assert (ct == Reader::OFFSET_TO_START_OF_MATERIALS);
	ct += utils::bufferWriteU16 (&out->blob[ct], 0);

	assert (ct == Reader::OFFSET_TO_SKELETON);
	ENGSkeleton handle_sk;
	handle_sk.setInvalid();
	ct += utils::bufferWriteU32 (&out->blob[ct], handle_sk.viewAsU32());


	assert (ct == Reader::OFFSET_TO_ENGSHAPE);
	for (u32 i=0; i<num_shape; i++)
	{
		ENGGPUShape handle;
		handle.setInvalid();
		ct += utils::bufferWriteU32 (&out->blob[ct], handle.viewAsU32());
	}


	const u16 START_of_MATERIAL = ct;
	for (u32 i=0; i<num_material; i++)
	{
		ENGMaterialPBR handle;
		handle.setInvalid();
		ct += utils::bufferWriteU32 (&out->blob[ct], handle.viewAsU32());
	}
	utils::bufferWriteU16 (&out->blob[16], START_of_MATERIAL);
	
	const u16 START_of_MESH = ct;
	utils::bufferWriteU16 (&out->blob[14], START_of_MESH);

	assert (ct +sizeof(u16) * 4 * num_meshes == sizeof_blob);
	return true;
}


//************************** 
bool model::set_skeleton (Model &m, ENGSkeleton handle)
{
	assert (model::isValid(m));
	utils::bufferWriteU32 (&m.blob[Reader::OFFSET_TO_SKELETON], handle.viewAsU32());
	return true;
}

//************************** 
bool model::set_gpushape (Model &m, u32 shape_num, ENGGPUShape handle)
{
	assert (model::isValid(m));

	const u32 num_shape = utils::bufferReadU16 (&m.blob[Reader::OFFSET_TO_NUM_SHAPES]);
	assert (shape_num < num_shape);
	if (shape_num < num_shape)
	{
		
		//utils::bufferWriteU32 (&m.blob[Reader::OFFSET_TO_ENGSHAPE + sizeof(u32)*shape_num], handle.viewAsU32());
		const u32 u = handle.viewAsU32();
		memcpy (&m.blob[Reader::OFFSET_TO_ENGSHAPE + sizeof(u32)*shape_num], &u, sizeof(u32) );
		return true;
	}

	DBGBREAK;
	return false;
}

//************************** 
bool model::set_mesh  (Model &m, u32 mesh_num, u16 shape_indexIN, u16 bone_indexIN, u16 material_indexIN)
{
	assert (model::isValid(m));

	const u32 num_meshes = utils::bufferReadU16 (&m.blob[Reader::OFFSET_TO_NUM_MESHES]);
	assert (mesh_num < num_meshes);
	if (mesh_num < num_meshes)
	{
		const u32 START_of_MESH = utils::bufferReadU16 (&m.blob[Reader::OFFSET_TO_START_OF_MESHES]);
		u32 ct = START_of_MESH + sizeof(u16) * 4 * mesh_num;
	
		Model::Mesh mesh {
            .shape_index = shape_indexIN,
            .bone_index = bone_indexIN,
            .material_index = material_indexIN,
            .pad = 0
		};
		memcpy (&m.blob[ct], &mesh, sizeof(Model::Mesh));
		return true;
	}

	DBGBREAK;
	return false;
}

//************************** 
bool model::set_material (Model &m, u32 material_num, ENGMaterialPBR handle)
{
	assert (model::isValid(m));

	const u32 num_materials = utils::bufferReadU16 (&m.blob[Reader::OFFSET_TO_NUM_MATERIAL]);
	assert (material_num < num_materials);
	if (material_num < num_materials)
	{
		const u32 START_of_MATERIALS = utils::bufferReadU16 (&m.blob[Reader::OFFSET_TO_START_OF_MATERIALS]);
		const u32 ct = START_of_MATERIALS + sizeof(u16) * 4 * material_num;
	
		utils::bufferWriteU32 (&m.blob[ct], handle.viewAsU32());
		return true;
	}

	DBGBREAK;
	return false;
}

