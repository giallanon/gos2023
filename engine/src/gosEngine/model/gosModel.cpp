#include "gosModel.h"
#include "../gos/gosMagicUID.h"
#include "../gosEngine.h"

using namespace gos;

//************************** 
bool model::isValid (const Model &m)
{
	if (NULL == m.allocator)
		return false;
	const u32 magic = utils::bufferReadU32 (m.blob, 0);
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
     	
		+sizeof(u32)	//ENGSkeleton as u32
     	+sizeof(u32) * num_shape //ENGGPUShape 1 to n
		+sizeof(u16) * 4 * num_meshes;

	out->allocator = allocator;
	out->blob = GOSALLOCT(u8*, allocator, sizeof_blob);
	memset (out->blob, 0, sizeof_blob);

	u32 ct = 0;
	ct += utils::bufferWriteU32 (&out->blob[ct], GOS_MAGIC__ENGINE_MODEL);
	ct += utils::bufferWriteU32 (&out->blob[ct], sizeof_blob);
	
	ct += utils::bufferWriteU16 (&out->blob[ct], num_shape);
	ct += utils::bufferWriteU16 (&out->blob[ct], num_material);
	ct += utils::bufferWriteU16 (&out->blob[ct], num_meshes);
	ct += utils::bufferWriteU16 (&out->blob[ct], 0);

	ENGSkeleton handle_sk;
	handle_sk.setInvalid();
	ct += utils::bufferWriteU32 (&out->blob[ct], handle_sk.viewAsU32());

	for (u32 i=0; i<num_shape; i++)
	{
		ENGGPUShape handle;
		handle.setInvalid();
		ct += utils::bufferWriteU32 (&out->blob[ct], handle.viewAsU32());
	}
	
	const u16 START_of_MESH = ct;
	ct += utils::bufferWriteU16 (&out->blob[14], START_of_MESH);

	assert (ct +sizeof(u16) * 4 * num_meshes == sizeof_blob);
	return true;
}


//************************** 
bool model::set_skeleton (Model &m, ENGSkeleton handle)
{
	assert (model::isValid(m));
	utils::bufferWriteU32 (&m.blob[16], handle.viewAsU32());
	return true;
}

//************************** 
bool model::set_shape (Model &m, u32 shape_num, ENGGPUShape handle)
{
	assert (model::isValid(m));

	const u32 num_shape = utils::bufferReadU16 (&m.blob[8]);
	if (shape_num < num_shape)
	{
		utils::bufferWriteU32 (&m.blob[20 + sizeof(u32)*shape_num], handle.viewAsU32());
	}

	DBGBREAK;
	return false;
}

//************************** 
bool model::set_mesh  (Model &m, u32 mesh_num, u16 shape_index, u16 bone_index, u16 material_index)
{
	assert (model::isValid(m));

	const u32 num_meshes = utils::bufferReadU16 (&m.blob[12]);
	if (mesh_num < num_meshes)
	{
		const u32 START_of_MESH = utils::bufferReadU16 (&m.blob[14]);
		u32 ct = START_of_MESH + sizeof(u16) * 4 * mesh_num;
		
		ct += utils::bufferWriteU16 (&m.blob[ct], shape_index);
		ct += utils::bufferWriteU16 (&m.blob[ct], bone_index);
		ct += utils::bufferWriteU16 (&m.blob[ct], material_index);
		return true;
	}

	DBGBREAK;
	return false;
}
