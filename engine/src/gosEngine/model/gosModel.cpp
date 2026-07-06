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

	u32 sizeof_blob =
		sizeof(u32)		//magic
		+sizeof(u32)	//total_size_of_blob
		
		+sizeof(u16)	//num_shape
		+sizeof(u16)	//num_material
		+sizeof(u16)	//num_meshes
		+sizeof(u16)	//abs-offset-to MESH 1
		+sizeof(u16)	//abs-offset-to MATERIAL 1
		+sizeof(u16)	//abs-offset-to NAME-TABLE
		+sizeof(u32)	//ENGSkeleton as u32
		;

	//ENGGPUShape 1 to n
	assert (0 == sizeof_blob % 8);
	assert (Reader::ENGSHAPE == sizeof_blob);
	sizeof_blob += +sizeof(u32) * num_shape; 
	sizeof_blob = utils::calcNextMultipleOf8(sizeof_blob);

	//ENGMaterialPBR 1 to n
	assert (0 == sizeof_blob % 8);
	const u16 START_of_MATERIAL = sizeof_blob;
	sizeof_blob += sizeof(u32) * num_material; 
	sizeof_blob = utils::calcNextMultipleOf8(sizeof_blob);

	//meshes
	const u16 START_of_MESH = sizeof_blob;
	assert (0 == sizeof_blob % 8);
	sizeof_blob += sizeof(Model::Mesh) * num_meshes;		
	sizeof_blob = utils::calcNextMultipleOf8(sizeof_blob);

	//tabella con i nomi
	const u16 START_of_NAME_TABLE = sizeof_blob;
	sizeof_blob += Reader::SIZE_OF_A_NAME * (num_shape + num_material + num_meshes);
	



	out->allocator = allocator;
	out->blob = GOSALLOCT(u8*, allocator, sizeof_blob);
	memset (out->blob, 0, sizeof_blob);

	u32 ct = 0;
	ct += utils::bufferWriteU32 (&out->blob[ct], GOS_MAGIC__ENGINE_MODEL);
	ct += utils::bufferWriteU32 (&out->blob[ct], sizeof_blob);
	
	assert (ct == Reader::NUM_SHAPES);
	ct += utils::bufferWriteU16 (&out->blob[ct], num_shape);

	assert (ct == Reader::NUM_MATERIAL);
	ct += utils::bufferWriteU16 (&out->blob[ct], num_material);

	assert (ct == Reader::NUM_MESHES);
	ct += utils::bufferWriteU16 (&out->blob[ct], num_meshes);

	assert (ct == Reader::OFFSET_TO_START_OF_MESHES);
	ct += utils::bufferWriteU16 (&out->blob[ct], START_of_MESH);

	assert (ct == Reader::OFFSET_TO_START_OF_MATERIALS);
	ct += utils::bufferWriteU16 (&out->blob[ct], START_of_MATERIAL);

	assert (ct == Reader::OFFSET_TO_START_OF_NAME_TABLE);
	ct += utils::bufferWriteU16 (&out->blob[ct], START_of_NAME_TABLE);


	assert (ct == Reader::SKELETON);
	ENGSkeleton handle_sk;
	handle_sk.setInvalid();
	ct += utils::bufferWriteU32 (&out->blob[ct], handle_sk.viewAsU32());

	//gpu shapes
	ct = Reader::ENGSHAPE;
	for (u32 i=0; i<num_shape; i++)
	{
		ENGGPUShape handle;
		handle.setInvalid();
		ct += utils::bufferWriteU32 (&out->blob[ct], handle.viewAsU32());
	}

	//materials
	ct = START_of_MATERIAL;
	for (u32 i=0; i<num_material; i++)
	{
		ENGMaterialPBR handle;
		handle.setInvalid();
		ct += utils::bufferWriteU32 (&out->blob[ct], handle.viewAsU32());
	}
	
	
	//meshes
	ct = START_of_MESH;
	assert (ct + sizeof(Model::Mesh) * num_meshes <= sizeof_blob);

	//name table
	ct = START_of_NAME_TABLE;
	assert (ct + Reader::SIZE_OF_A_NAME * (num_shape + num_material + num_meshes)  <= sizeof_blob);

	return true;
}

//************************** 
static void model__do_set_name (Model &m, u16 index, const char *name__can_be_NULL)
{
	const u32 START_of_NAMETABLE = utils::bufferReadU16 (&m.blob[model::Reader::OFFSET_TO_START_OF_NAME_TABLE]);
	const u32 i = START_of_NAMETABLE + index * model::Reader::SIZE_OF_A_NAME;
	
	memset (&m.blob[i], 0, model::Reader::SIZE_OF_A_NAME);
	if (NULL == name__can_be_NULL)
		return;
	if (0x00 == name__can_be_NULL[0])
		return;

	u32 len = (u32) strlen(name__can_be_NULL);
	if (len >= model::Reader::SIZE_OF_A_NAME)
		len = model::Reader::SIZE_OF_A_NAME - 1;
	memcpy (&m.blob[i], name__can_be_NULL, len);
}

//************************** 
bool model::set_skeleton (Model &m, ENGSkeleton handle)
{
	assert (model::isValid(m));
	utils::bufferWriteU32 (&m.blob[Reader::SKELETON], handle.viewAsU32());
	return true;
}

//************************** 
bool model::set_gpushape (Model &m, u32 shape_num, ENGGPUShape handle, const char *name__can_be_NULL)
{
	assert (model::isValid(m));

	const u32 num_shape = utils::bufferReadU16 (&m.blob[Reader::NUM_SHAPES]);
	assert (shape_num < num_shape);
	if (shape_num < num_shape)
	{
		const u32 u = handle.viewAsU32();
		memcpy (&m.blob[Reader::ENGSHAPE + sizeof(u32)*shape_num], &u, sizeof(u32) );
		model__do_set_name (m, shape_num, name__can_be_NULL);
		return true;
	}

	DBGBREAK;
	return false;
}

//************************** 
bool model::set_material (Model &m, u32 material_num, ENGMaterialPBR handle, const char *name__can_be_NULL)
{
	assert (model::isValid(m));

	const u32 num_materials = utils::bufferReadU16 (&m.blob[Reader::NUM_MATERIAL]);
	assert (material_num < num_materials);
	if (material_num >= num_materials)
	{
		DBGBREAK;
		return false;
	}

	const u32 START_of_MATERIALS = utils::bufferReadU16 (&m.blob[Reader::OFFSET_TO_START_OF_MATERIALS]);
	ENGMaterialPBR *list = reinterpret_cast<ENGMaterialPBR*>(&m.blob[START_of_MATERIALS]);
	list[material_num] = handle;

	model__do_set_name (m, utils::bufferReadU16 (&m.blob[Reader::NUM_SHAPES]) + material_num, name__can_be_NULL);
	return true;
}

//************************** 
bool model::set_mesh  (Model &m, u32 mesh_num, u16 shape_indexIN, u16 bone_indexIN, u16 material_indexIN, const char *name__can_be_NULL)
{
	assert (model::isValid(m));

	const u32 num_meshes = utils::bufferReadU16 (&m.blob[Reader::NUM_MESHES]);
	assert (mesh_num < num_meshes);
	if (mesh_num >= num_meshes)
	{
		DBGBREAK;
		return false;
	}

	Model::Mesh mesh {
        .shape_index = shape_indexIN,
        .bone_index = bone_indexIN,
        .material_index = material_indexIN,
        .pad = 0
	};

	const u32 START_of_MESH = utils::bufferReadU16 (&m.blob[Reader::OFFSET_TO_START_OF_MESHES]);
	Model::Mesh *list = reinterpret_cast<Model::Mesh*>(&m.blob[START_of_MESH]);
	list[mesh_num] = mesh;

	const u16 num_shapes = utils::bufferReadU16 (&m.blob[Reader::NUM_SHAPES]);
	const u16 num_material = utils::bufferReadU16 (&m.blob[Reader::NUM_MATERIAL]);
	model__do_set_name (m, num_shapes + num_material + mesh_num, name__can_be_NULL);
	return true;
}


