#include "gosSkeleton.h"

using namespace gos;

//*******************************************
SkeletonBuilder::SkeletonBuilder()
{
    gos::Allocator *allocator = gos::getScrapAllocator();

    nameList.setup (allocator, 1024);
    boneList.setup (allocator, 64);
}

//*******************************************
SkeletonBuilder::~SkeletonBuilder()
{
    nameList.unsetup();
    boneList.unsetup();
}

//*******************************************
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

//*******************************************
u32 SkeletonBuilder::begin (const char *rootName, Bone **out_canBeNULL)
{
    numBones = 0;
    nameList.reset();
    boneList.reset();
    const u32 newBoneIndex = priv_newBone (rootName);

    if (NULL != out_canBeNULL)
        *out_canBeNULL = &boneList[newBoneIndex];

    return newBoneIndex;
}

//*******************************************
u32 SkeletonBuilder::addChildTo (u32 srcBoneIndex, const char *dstBoneName, Bone **out_canBeNULL)
{
    assert (srcBoneIndex < numBones);
    Bone *srcBone = &boneList[srcBoneIndex];

    if (0xFF == srcBone->firstChildIndex)
    {
        const u32 newBoneIndex = priv_newBone (dstBoneName);
        Bone *newBone = &boneList[newBoneIndex];
        
        srcBone->firstChildIndex = newBoneIndex;
        if (NULL != out_canBeNULL)
            *out_canBeNULL = newBone;
        return newBoneIndex;
    }
    else
    {
        return addSiblingTo (srcBone->firstChildIndex, dstBoneName, out_canBeNULL);
    }
}

//*******************************************
u32 SkeletonBuilder::addSiblingTo (u32 srcBoneIndex, const char *dstBoneName, Bone **out_canBeNULL)
{
    assert (srcBoneIndex < numBones);

    const u32 newBoneIndex = priv_newBone (dstBoneName);
    Bone *srcBone = &boneList[srcBoneIndex];
    Bone *newBone = &boneList[newBoneIndex];
    
    while (0xFF != srcBone->sigblinIndex)
    {
        srcBone = &boneList[srcBone->sigblinIndex];
    }
    srcBone->sigblinIndex = newBoneIndex;

    if (NULL != out_canBeNULL)
        *out_canBeNULL = newBone;
    return newBoneIndex;    
}

//*******************************************
Skeleton* SkeletonBuilder::end (gos::Allocator *allocatorIN)
{
    Skeleton *sk = GOSNEW(allocatorIN, Skeleton)(allocatorIN, numBones);
    memcpy (sk->boneList, boneList._queryPointer(), sizeof(Bone) * numBones);
    sk->nameList.clone_from (allocatorIN, nameList);
    return sk;
}