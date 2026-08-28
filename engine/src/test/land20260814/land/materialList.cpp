#include "materialList.h"
#include "gosImageBufferRGBA.h"
#include "gosGeomIntersect3D.h"

using namespace gos;
using namespace land;


//********************************
MaterialList::MaterialList()
{
	num_material = 0;
	memset (list_of_materialID, 0xff, sizeof(list_of_materialID));
}

//********************************
void MaterialList::add (u8 materialID, u32 rgb)
{
	if (num_material >= MATERIAL__NUM_MAX)
	{
		DBGBREAK;
		return;
	}

	for (u32 i=0; i<num_material; i++)
	{
		if (list_of_materialID[i] == materialID)
		{
			DBGBREAK;
			return;
		}
	}

	list_of_materialID[num_material] = materialID;

	
	gos::ColorHDR hdr(rgb);
	hdr.sRGBToLinear();
	list_of_material[num_material].diffuse_r = hdr.col.r;
	list_of_material[num_material].diffuse_g = hdr.col.g;
	list_of_material[num_material].diffuse_b = hdr.col.b;

	num_material++;
}

//********************************
const Material*	MaterialList::get_by_id (u8 materialID) const
{
	for (u32 i=0; i<num_material; i++)
	{
		if (list_of_materialID[i] == materialID)
		{
			return &list_of_material[i];
		}
	}

	DBGBREAK;
	return NULL;
}
