#include "gosSkeleton.h"

using namespace gos;
using namespace gos::skeleton;

//*******************************************
Builder::Builder()
{
    gos::Allocator *allocator = gos::getScrapAllocator();

    nameList.setup (allocator, 1024);
    boneList.setup (allocator, 64);
}

//*******************************************
Builder::~Builder()
{
    nameList.unsetup();
    boneList.unsetup();
}

//*******************************************
u32 Builder::priv_newBone (const char *name)
{
    u32 ret = boneList.getNElem();
    assert (ret < 0xff);

    Bone bone;
    bone.matrix.identity();
    bone.firstChildIndex = bone.sigblinIndex = 0xFF;
    bone.nameIndex = static_cast<u16>( nameList.add(name));

    boneList.append(bone);
    return ret;
}

//*******************************************
u32 Builder::begin (const char *rootName, Bone **out_canBeNULL)
{
    nameList.reset();
    boneList.reset();
    const u32 newBoneIndex = priv_newBone (rootName);

    if (NULL != out_canBeNULL)
        *out_canBeNULL = &boneList[newBoneIndex];

    return newBoneIndex;
}

//*******************************************
u32 Builder::addChildTo (u32 srcBoneIndex, const char *dstBoneName, Bone **out_canBeNULL)
{
    assert (srcBoneIndex < boneList.getNElem());
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
u32 Builder::addSiblingTo (u32 srcBoneIndex, const char *dstBoneName, Bone **out_canBeNULL)
{
    assert (srcBoneIndex < boneList.getNElem());

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
bool Builder::end (gos::Allocator *allocatorIN, Skeleton *out)
{
    assert (NULL == out->allocator);

    const u8 num_bones = (u8)boneList.getNElem();


    const u32 sizeof_header = sizeof(u32)     //magic
        + sizeof(u32)    //total_size_of_blob
        + sizeof(u8)     //num-bones
        + sizeof(u8)     //pad
        + sizeof(u16);   //abs-offset to START-OF-NAME-TABLE

    assert (sizeof(Bone) % 4 == 0);
    const u32 sizeof_boneList = GOS_ALIGN_NUMBER_TO_POWER_OF_TWO(sizeof(Bone) * num_bones, 4);

    const u32 sizeof_nameTOC = GOS_ALIGN_NUMBER_TO_POWER_OF_TWO(sizeof(u16) * num_bones, 4);

    u32 sizeof_allNames = 0;
    u32 iter;
    const char *s;
    nameList.toStart (&iter);
    while (NULL != (s = nameList.next (&iter)))
    {
        const u32 len = string::utf8::lengthInByte(s);
        sizeof_allNames += GOS_ALIGN_NUMBER_TO_POWER_OF_TWO(len + 1, 4);
    }


    const u32 blob_size = sizeof_header + sizeof_boneList + sizeof_nameTOC + sizeof_allNames;

    out->allocator = allocatorIN;
    out->blob = GOSALLOCT(u8*, out->allocator, blob_size);
    memset (out->blob, 0, blob_size);


    u32 ct = 0;
    ct += gos::utils::bufferWriteU32 (&out->blob[ct], GOS_MAGIC__SKELETON);
    ct += gos::utils::bufferWriteU32 (&out->blob[ct], blob_size);
    out->blob[ct++] = num_bones;
    out->blob[ct++] = 0x00;
    out->blob[ct++] = 0x00;
    out->blob[ct++] = 0x00;

    for (u32 i = 0; i < num_bones; i++)
    {
        memcpy (&out->blob[ct], &boneList(i), sizeof(Bone));
        ct += sizeof(Bone);
    }
    assert (ct == sizeof_header + sizeof_boneList);

    const u32 START_OF_NAME_TABLE = ct;
    gos::utils::bufferWriteU16 (&out->blob[10], START_OF_NAME_TABLE);

    u32 ct_name = START_OF_NAME_TABLE + sizeof_nameTOC;
    nameList.toStart (&iter);
    for (u32 i = 0; i < num_bones; i++)
    {
        s = nameList.next (&iter);
        assert (NULL != s);

        const u32 len = string::utf8::lengthInByte(s);
        memcpy (&out->blob[ct_name], s, len + 1);

        assert (ct_name < 0xFFFF);
        gos::utils::bufferWriteU16 (&out->blob[START_OF_NAME_TABLE + sizeof(u16) * i], (u16)ct_name);
        ct_name += GOS_ALIGN_NUMBER_TO_POWER_OF_TWO(len + 1, 4);
    }

    assert (ct_name == blob_size);
    return true;
}