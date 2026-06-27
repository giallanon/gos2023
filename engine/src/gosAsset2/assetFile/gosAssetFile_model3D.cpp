#include "../gos/gosBufferWriter.h"
#include "gosAssetFile_model3D.h"
#include "gos.h"


using namespace gos;
using namespace gos::asset2;

//************************************
AssetFile_model3D::AssetFile_model3D()
{
	localAllocator = NULL;
}

//************************************
void AssetFile_model3D::priv_free()
{
	if (NULL == localAllocator)
		return;
	localAllocator = NULL;
}

//************************************
void AssetFile_model3D::begin(gos::Allocator *localAllocatorIN)
{
	if (localAllocatorIN != localAllocator)
	{
		priv_free();
		localAllocator = localAllocatorIN;

		listof_shape.setup (localAllocator, 128);
		listof_mesh.setup (localAllocator, 128);
		listof_material.setup (localAllocator, 128);
	}

	listof_shape.reset();
	listof_mesh.reset();
	listof_material.reset();
	uid_of_concrete_skeleton.setInvalid();
}

//************************************
void AssetFile_model3D::skeleton_set (UID uid_of_concrete_skeleton__IN)
{
	uid_of_concrete_skeleton = uid_of_concrete_skeleton__IN;
}

//************************************
u32 AssetFile_model3D::shape_add (UID uid_of_concrete_shape)
{
	const u32 n = listof_shape.getNElem();
	for (u32 i = 0; i < n; i++)
	{
		if (listof_shape(i) == uid_of_concrete_shape)
			return i;
	}
	listof_shape.append (uid_of_concrete_shape);
	return n;
}

//************************************
u32 AssetFile_model3D::material_add (UID uid_of_concrete_material)
{
	const u32 n = listof_material.getNElem();
	for (u32 i = 0; i < n; i++)
	{
		if (listof_material(i) == uid_of_concrete_material)
			return i;
	}
	listof_material.append (uid_of_concrete_material);
	return n;
}

//************************************
bool AssetFile_model3D::mesh_add (u32 shape_index, u32 bone_index, u32 material_index)
{
	if (shape_index >= listof_shape.getNElem())
		return false;

	sMeshInfo m { 
		.shape_index = shape_index, 
		.bone_index = bone_index, 
		.material_index = material_index
	};

	listof_mesh.append (m);
	return true;
}

//************************************
void AssetFile_model3D::end()
{
}

//************************************
bool AssetFile_model3D::save (const char *filenameDST)
{
    u8 stackBuffer[2048];
	gos::BufferW_linear buffer;
	buffer.setupWithBase (stackBuffer, sizeof(stackBuffer), gos::getScrapAllocator(), eEndianess::big);

	//magic
	buffer.writeU32 (GOS_MAGIC__ASSET_MODEL3D);

	//skeleton (concrete asset UID)
	buffer.writeU64 (uid_of_concrete_skeleton._uid);


	//num shape e relativi UID concreti
	buffer.writeU32 (listof_shape.getNElem());
	for (u32 i=0; i<listof_shape.getNElem(); i++)
	{
		assert (listof_shape(i).isAnAssetOfType(eAssetType::shape));
		buffer.writeU64 (listof_shape(i)._uid);
	}

	//num materiali e relativi UID concreti
	buffer.writeU32 (listof_material.getNElem());
	for (u32 i=0; i<listof_material.getNElem(); i++)
	{
		assert (listof_material(i).isAnAssetOfType(eAssetType::materialPBR));
		buffer.writeU64 (listof_material(i)._uid);
	}

	//num meshes e relative info
	buffer.writeU32 (listof_mesh.getNElem());
	for (u32 i=0; i<listof_mesh.getNElem(); i++)
	{
		//shape index
		buffer.writeU32 (listof_mesh(i).shape_index);

		//bone index
		buffer.writeU32 (listof_mesh(i).bone_index);

		//material index
		buffer.writeU32 (listof_mesh(i).material_index);		
	}

	//salvo il file asset
	return fs::fileSaveBuffer (filenameDST, stackBuffer, buffer.tell());
}

