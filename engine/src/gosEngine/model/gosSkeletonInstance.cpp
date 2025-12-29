#include "gosSkeleton.h"

using namespace gos;


//**********************************************************************
SkeletonInstance::SkeletonInstance (const Skeleton *modelIN)
{
    model = modelIN;
    numBones = model->bone_getNum();
    boneList = GOSALLOCT(Bone*, model->getAllocator(), sizeof(Bone) * numBones);
    memcpy (boneList, model->bone_getList(), sizeof(Bone) * numBones);
}

//**********************************************************************
void SkeletonInstance::priv_free()
{
    GOSFREE(model->getAllocator(), boneList);
    boneList = NULL;
}

//**********************************************************************
Bone* SkeletonInstance::bone_getByName (const char *name) const
{
    const u32 index = model->bone_getIndexByName(name);
    if (u32MAX != index)
        return &boneList[index];
    return NULL;    
}

//**********************************************************************
void SkeletonInstance::applyTransform (const mat4x4f &matW)
{
    priv_applyTransform_ric (0, matW);
}

//**********************************************************************
void SkeletonInstance::priv_applyTransform_ric (u32 boneIndex, const mat4x4f &parent_matW)
{
    Bone *bone = &boneList[boneIndex];
    bone->matrix = parent_matW * model->boneList[boneIndex].matrix;
    
    u32 childrenIndex = bone->firstChildIndex;
    while (0xFF != childrenIndex)
    {
        priv_applyTransform_ric (childrenIndex, bone->matrix);
        childrenIndex = boneList[childrenIndex].sigblinIndex;
    }
}