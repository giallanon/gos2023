#include "gosModel.h"

using namespace gos;
using namespace gos::model;

//************************** 
Model::Model()
{
    allocator = gos::getSysHeapAllocator();
    skeleton = NULL;
    shapeList.setup (allocator, 8);
    shapeAndBoneLinkList.setup (allocator, 8);
}

//************************** 
void Model::priv_free()
{
    if (NULL == allocator)
        return;

    shapeList.unsetup();
    shapeAndBoneLinkList.unsetup ();
    allocator = NULL;
}

//************************** 
void Model::addShape (gos::ENGShape handle)
{
    shapeList.append (handle);
}

//************************** 
void Model::linkShapeToBone (gos::ENGShape shape, const char *boneName)
{
    const u32 boneIndex = skeleton->getBoneIndexByName(boneName);
    assert (boneIndex != u32MAX);

    const u32 shapeIndex = shapeList.simpleSearch (shape);
    assert (shapeIndex != u32MAX);

    shapeAndBoneLinkList.append ({ 
        .shapeIndex = static_cast<u16>(shapeIndex),
        .boneIndex = static_cast<u16>(boneIndex)
    });
}


/**********************************************************************
 * 
 * ModelInstance
 * 
 ***********************************************************************/
ModelInstance::ModelInstance (const Model *modelIN)
{
    model = modelIN; 
    sk = model->skeleton->newInstance();
}

void ModelInstance::priv_free()
{
    SkeletonInstance::free (sk);
}