#include "gosModel.h"

using namespace gos;
using namespace gos::model;

//************************** 
Model::Model()
{
    allocator = gos::getSysHeapAllocator();
    skeleton = NULL;
    meshList.setup (allocator, 8);
}

//************************** 
void Model::priv_free()
{
    if (NULL == allocator)
        return;

    meshList.unsetup ();
    allocator = NULL;
}


//************************** 
void Model::addMesh (gos::ENGShape shape, u32 material_indexIN, const char *boneName)
{
    const u32 boneIndex = skeleton->getBoneIndexByName(boneName);
    assert (boneIndex != u32MAX);

    meshList.append ({ 
        .shape_handle = shape,
        .bone_index = static_cast<u16>(boneIndex),
        .material_index = static_cast<u16>(material_indexIN)
    });
}
