#include "gosModel.h"
#include "../gosEngine.h"

using namespace gos;
using namespace gos::model;


//*******************************************
void Reader::setup (const Model *mIN)
{
	m = mIN;
}

//*******************************************
ENGSkeleton Reader::skeleton_get_handle() const
{
	assert (model::isValid(*m));
	
	ENGSkeleton ret;
	ret.setFromU32 (utils::bufferReadU32 (&m->blob[16]));
	return ret;
}

//*******************************************
u32 Reader::mesh_get_num() const
{
	assert (model::isValid(*m));
	return utils::bufferReadU16 (&m->blob[12]);
}

//*******************************************
const Model::Mesh* Reader::mesh_get_by_index (u32 index) const
{
	assert (index < mesh_get_num());
	const u32 ABS_OFFSET_of_MESH1 = utils::bufferReadU16 (&m->blob[14]);

	return reinterpret_cast<const Model::Mesh*> (&m->blob[ABS_OFFSET_of_MESH1 + sizeof(Model::Mesh) * index]);
}

//*******************************************
u32 Reader::gpushape_get_num() const
{
	assert (model::isValid(*m));
	return utils::bufferReadU16 (&m->blob[8]);
}

//*******************************************
ENGGPUShape Reader::gpushape_get_by_index (u32 index) const
{
	assert (index < gpushape_get_num());

	ENGGPUShape ret;
	ret.setFromU32 (utils::bufferReadU32 (&m->blob[20 + sizeof(u32) * index]));
	return ret;
}

//*******************************************
const ENGGPUShape* Reader::gpushape_get_pt_to_list () const
{
	return reinterpret_cast<const ENGGPUShape*>(&m->blob[20]);
}


//*******************************************
u32 Reader::material_get_num() const
{
	assert (model::isValid(*m));
	return utils::bufferReadU16 (&m->blob[10]);
}

