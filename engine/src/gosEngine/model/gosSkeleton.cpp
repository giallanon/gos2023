#include "gosSkeleton.h"

using namespace gos;

/**********************************************************************
 * 
 * SkeletonBuilder
 * 
 ***********************************************************************/
SkeletonBuilder::SkeletonBuilder()
{
    gos::Allocator *allocator = gos::getScrapAllocator();

    nameList.setup (allocator, 1024);
    boneList.setup (allocator, 64);
}

SkeletonBuilder::~SkeletonBuilder()
{
    nameList.unsetup();
    boneList.unsetup();
}

u32 SkeletonBuilder::priv_newBone (const char *name)
{
    assert (numBones < 0xff);
    Bone bone;
    bone.matrix.identity();
    bone.firstChildIndex = bone.sigblinIndex = 0xFF;
    bone.nameIndex = static_cast<u16>( nameList.add(name));

    boneList.append(bone);
    return numBones++;
}

u32 SkeletonBuilder::begin (const char *rootName)
{
    numBones = 0;
    nameList.reset();
    boneList.reset();
    return priv_newBone (rootName);
}

u32 SkeletonBuilder::addChildTo (u32 srcBoneIndex, const char *dstBoneName, Bone **out_canBeNULL)
{
    assert (srcBoneIndex < numBones);

    const u32 newBoneIndex = priv_newBone (dstBoneName);
    Bone *srcBone = &boneList[srcBoneIndex];
    Bone *newBone = &boneList[newBoneIndex];

    if (0xFF != srcBone->firstChildIndex)
    {
        newBone->sigblinIndex = srcBone->firstChildIndex;
    }
    srcBone->firstChildIndex = newBoneIndex;

    if (NULL != out_canBeNULL)
        *out_canBeNULL = newBone;
    return newBoneIndex;
}

u32 SkeletonBuilder::addSiblingdTo (u32 srcBoneIndex, const char *dstBoneName, Bone **out_canBeNULL)
{
    assert (srcBoneIndex < numBones);

    const u32 newBoneIndex = priv_newBone (dstBoneName);
    Bone *srcBone = &boneList[srcBoneIndex];
    Bone *newBone = &boneList[newBoneIndex];
    
    if (0xFF != srcBone->sigblinIndex)
    {
        newBone->sigblinIndex = srcBone->sigblinIndex;
    }
    srcBone->sigblinIndex = newBoneIndex;

    if (NULL != out_canBeNULL)
        *out_canBeNULL = newBone;
    return newBoneIndex;    
}

void SkeletonBuilder::end (gos::Allocator *allocatorIN, Skeleton *out)
{
    out->priv_alloc (allocatorIN, numBones);
    memcpy (out->boneList, boneList._queryPointer(), sizeof(Bone) * numBones);
    out->nameList.clone_from (allocatorIN, nameList);
}




/**********************************************************************
 * 
 * Skeleton
 * 
 ***********************************************************************/
void Skeleton::priv_alloc (gos::Allocator *allocatorIN, u32 numBonesIN)
{
    priv_free();
    
    allocator = allocatorIN;
    numBones = numBonesIN;
    boneList = GOSALLOCT(Bone*, allocator, sizeof(Bone) * numBones);
}

void Skeleton::priv_free()
{
    if (NULL == allocator)
        return;
    GOSFREE(allocator, boneList);
    nameList.unsetup();
    allocator = NULL;
}

u32 Skeleton::getBoneIndexByName (const char *name) const
{
    for (u32 i=0; i<numBones; i++)
    {
        if (strcmp (nameList.getStringAtOffset(boneList[i].nameIndex), name) == 0)
            return i;
    }
    return u32MAX;
}

Bone* Skeleton::getBoneByName (const char *name) const
{
    const u32 index = getBoneIndexByName(name);
    if (u32MAX != index)
        return &boneList[index];
    return NULL;
}

SkeletonInstance* Skeleton::newInstance()
{
    return GOSNEW(allocator, SkeletonInstance)(this);
}


/**********************************************************************
 * 
 * SkeletonInstance
 * 
 ***********************************************************************/
SkeletonInstance::SkeletonInstance (const Skeleton *modelIN)
{
    model = modelIN;
    numBones = model->getNumBones();
    boneList = GOSALLOCT(Bone*, model->getAllocator(), sizeof(Bone) * numBones);
    memcpy (boneList, model->getBoneList(), sizeof(Bone) * numBones);
}

void SkeletonInstance::priv_free()
{
    GOSFREE(model->getAllocator(), boneList);
    boneList = NULL;
}

Bone* SkeletonInstance::getBoneByName (const char *name) const
{
    const u32 index = model->getBoneIndexByName(name);
    if (u32MAX != index)
        return &boneList[index];
    return NULL;    
}

void SkeletonInstance::applyTransform (const mat4x4f &matW)
{
    priv_applyTransform_ric (0, matW);
}

void SkeletonInstance::priv_applyTransform_ric (u32 boneIndex, const mat4x4f &parent_matW)
{
    Bone *bone = &boneList[boneIndex];
    bone->matrix = model->boneList[boneIndex].matrix * parent_matW;
    
    u32 childrenIndex = bone->firstChildIndex;
    while (0xFF != childrenIndex)
    {
        priv_applyTransform_ric (childrenIndex, bone->matrix);
        childrenIndex = boneList[childrenIndex].sigblinIndex;
    }
}