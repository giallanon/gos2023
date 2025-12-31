#include "gosModel.h"

using namespace gos;
using namespace gos::model;


//*******************************************
Builder::Builder (u32 preallocNumMesh)
{
    assert (preallocNumMesh > 0);
    gos::Allocator *allocator = gos::getScrapAllocator();
    meshList.setup (allocator, preallocNumMesh);
}

//*******************************************
Builder::~Builder()
{
    meshList.unsetup();
}

//*******************************************
void Builder::begin (Skeleton *skeletonIN)
{
    assert (NULL != skeletonIN);
    assert (skeletonIN->bone_getNum() > 0);
    
    skeleton = skeletonIN;
    meshList.reset();
}

//*******************************************
void Builder::addMeshToBone (gos::ENGGPUShape shape, u32 material_indexIN, const char *boneName)
{
    const u32 boneIndex = skeleton->bone_getIndexByName(boneName);
    assert (boneIndex != u32MAX);

    meshList.append ({ 
        .shape_handle = shape,
        .bone_index = static_cast<u16>(boneIndex),
        .material_index = static_cast<u16>(material_indexIN)
    });    
}

//*******************************************
Model* Builder::end (gos::Allocator *allocator)
{
    Model *ret = GOSNEW(allocator, Model)(allocator, skeleton, meshList._queryTypedPointer(), meshList.getNElem());
    return ret;
}