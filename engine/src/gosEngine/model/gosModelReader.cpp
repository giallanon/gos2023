#include "gosModel.h"
#include "../gosEngine.h"

using namespace gos;
using namespace gos::model;


//*******************************************
void Reader::setup (const Model *mIN)
{
	m = mIN;
	assert (model::isValid(*m));
}

//*******************************************
SkeletonH Reader::skeleton_get_handle() const
{
	assert (model::isValid(*m));
	
	SkeletonH ret;
	ret.setFromU32 ( utils::bufferReadU32 (&m->blob[SKELETON]) );
	return ret;
}

//*******************************************
u32 Reader::mesh_get_num() const
{
	assert (model::isValid(*m));
	return utils::bufferReadU16 (&m->blob[NUM_MESHES]);
}

//*******************************************
const Model::Mesh* Reader::mesh_get_by_index (u32 index) const
{
	assert (index < mesh_get_num());
	const u32 ABS_OFFSET_of_MESH1 = utils::bufferReadU16 (&m->blob[OFFSET_TO_START_OF_MESHES]);

	return reinterpret_cast<const Model::Mesh*> (&m->blob[ABS_OFFSET_of_MESH1 + sizeof(Model::Mesh) * index]);
}

//*******************************************
const char* Reader::mesh_get_name_by_index (u32 index) const
{
	assert (index < mesh_get_num());
	const u32 ABS_OFFSET_NAME_TABLE = utils::bufferReadU16 (&m->blob[OFFSET_TO_START_OF_NAME_TABLE]);
	const u32 POS = ABS_OFFSET_NAME_TABLE + model::Reader::SIZE_OF_A_NAME * (gpushape_get_num() + material_get_num() + index);
	return reinterpret_cast<const char*> (&m->blob[POS]);
}

//*******************************************
u32 Reader::gpushape_get_num() const
{
	assert (model::isValid(*m));
	return utils::bufferReadU16 (&m->blob[NUM_SHAPES]);
}

//*******************************************
GPUShapeH Reader::gpushape_get_by_index (u32 index) const
{
	assert (index < gpushape_get_num());
	const GPUShapeH *list = gpushape_get_pt_to_list();
	return list[index];

}

//*******************************************
const GPUShapeH* Reader::gpushape_get_pt_to_list () const
{
	const u32 ct = ENGSHAPE;
	assert (0 == ct % 8);
	return reinterpret_cast<const GPUShapeH*>(&m->blob[ct]);
}

//*******************************************
const char* Reader::gpushape_get_name_by_index (u32 index) const
{
	assert (index < gpushape_get_num());
	const u32 ABS_OFFSET_NAME_TABLE = utils::bufferReadU16 (&m->blob[OFFSET_TO_START_OF_NAME_TABLE]);
	const u32 POS = ABS_OFFSET_NAME_TABLE + model::Reader::SIZE_OF_A_NAME * index;
	return reinterpret_cast<const char*> (&m->blob[POS]);
}




//*******************************************
u32 Reader::material_get_num() const
{
	assert (model::isValid(*m));
	return utils::bufferReadU16 (&m->blob[NUM_MATERIAL]);
}

//*******************************************
MaterialPBRH Reader::material_get_by_index (u32 index) const
{
	assert (index < material_get_num());
	const MaterialPBRH *list = material_get_pt_to_list();
	return list[index];
}

//*******************************************
const MaterialPBRH* Reader::material_get_pt_to_list () const
{
	const u32 ct = utils::bufferReadU16 (&m->blob[OFFSET_TO_START_OF_MATERIALS]);
	assert (0 == ct % 8);
	return reinterpret_cast<const MaterialPBRH*>(&m->blob[ct]);
}

//*******************************************
const char* Reader::material_get_name_by_index (u32 index) const
{
	assert (index < material_get_num());
	const u32 ABS_OFFSET_NAME_TABLE = utils::bufferReadU16 (&m->blob[OFFSET_TO_START_OF_NAME_TABLE]);
	const u32 POS = ABS_OFFSET_NAME_TABLE + model::Reader::SIZE_OF_A_NAME * (gpushape_get_num() + index);
	return reinterpret_cast<const char*> (&m->blob[POS]);
}


