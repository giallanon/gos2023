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
ENGSkeleton Reader::skeleton_get_handle() const
{
	assert (model::isValid(*m));
	
	ENGSkeleton ret;
	ret.setFromU32 (utils::bufferReadU32 (&m->blob[SKELETON]));
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
u32 Reader::gpushape_get_num() const
{
	assert (model::isValid(*m));
	return utils::bufferReadU16 (&m->blob[NUM_SHAPES]);
}

//*******************************************
ENGGPUShape Reader::gpushape_get_by_index (u32 index) const
{
	assert (index < gpushape_get_num());
	const ENGGPUShape *list = gpushape_get_pt_to_list();
	return list[index];

}

//*******************************************
const ENGGPUShape* Reader::gpushape_get_pt_to_list () const
{
	const u32 ct = ENGSHAPE;
	assert (0 == ct % 8);
	return reinterpret_cast<const ENGGPUShape*>(&m->blob[ct]);
}


//*******************************************
u32 Reader::material_get_num() const
{
	assert (model::isValid(*m));
	return utils::bufferReadU16 (&m->blob[NUM_MATERIAL]);
}

//*******************************************
ENGMaterialPBR Reader::material_get_by_index (u32 index) const
{
	assert (index < material_get_num());
	const ENGMaterialPBR *list = material_get_pt_to_list();
	return list[index];
}

//*******************************************
const ENGMaterialPBR* Reader::material_get_pt_to_list () const
{
	const u32 ct = utils::bufferReadU16 (&m->blob[OFFSET_TO_START_OF_MATERIALS]);
	assert (0 == ct % 8);
	return reinterpret_cast<const ENGMaterialPBR*>(&m->blob[ct]);
}




