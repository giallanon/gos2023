#include "gosSkeleton.h"
#include "../gos/gosMagicUID.h"

using namespace gos;


//***********************************************************************
Skeleton* Skeleton::createFromMemory (gos::Allocator *allocatorIN, const u8 *buffer, u32 sizeof_buffer, u32 *out__numByteUsed)
{
    assert (NULL != out__numByteUsed);

    if (sizeof_buffer < 4)
    {
        logger::err ("Skeleton::deserialize => input buffer is too small (%d, expected at least 4 byte)\n", sizeof_buffer);
        return 0;
    }

    u32 ct = 0;

    //header    
    const u32 magic = utils::bufferReadU32 (&buffer[ct]);
    ct += 4;
    if (!magic::signatureMatch (magic, GOS_MAGIC__ENGINE_SKELETON))     { logger::err ("Skeleton::deserialize => invalid signature\n"); return 0; }
    if (!gos::magic::versionMatch (magic, GOS_MAGIC__DATA_BLOB_DEF))    { logger::err ("Skeleton::deserialize => invalid file version\n"); return 0; }

    const u32 sizeof_memoryBlock = utils::bufferReadU32 (&buffer[ct]);
    ct += 4;

    //memory block
    const u32 total_size_needed = 4 + sizeof_memoryBlock;
    if (sizeof_buffer < total_size_needed)
    {
        logger::err ("Skeleton::deserialize => input buffer is too small (%d, expected %d)\n", sizeof_buffer, total_size_needed);
        return 0;
    }

    const u32 numBones = utils::bufferReadU32 (&buffer[ct]);
    ct += 4;

    //Creo lo skeleton
    Skeleton *sk = GOSNEW(allocatorIN, Skeleton)(allocatorIN, numBones);
    memcpy (sk->boneList, &buffer[ct], sizeof(Bone) * numBones);
    ct += sizeof(Bone) * numBones;

    u32 n = sk->nameList.deserialize_fromMemory (allocatorIN, &buffer[ct], sizeof_buffer - ct);
    if (0 == n)
    {
        logger::err ("Skeleton::deserialize => error deserializing nameList\n");
        return 0;
    }
    ct += n;
    


    assert (ct == total_size_needed);
    assert (ct <= sizeof_buffer);
    *out__numByteUsed = total_size_needed;
    return sk;
}

//***********************************************************************
Skeleton::Skeleton (gos::Allocator *allocatorIN, u32 numBonesIN)
{
    allocator = allocatorIN;
    numBones = numBonesIN;
    boneList = GOSALLOCT(Bone*, allocator, sizeof(Bone) * numBones);
}

//***********************************************************************
void Skeleton::priv_free()
{
    if (NULL == allocator)
        return;
    GOSFREE(allocator, boneList);
    nameList.unsetup();
    allocator = NULL;
}

//***********************************************************************
u32 Skeleton::bone_getIndexByName (const char *name) const
{
    for (u32 i=0; i<numBones; i++)
    {
        if (strcmp (nameList.getStringAtOffset(boneList[i].nameIndex), name) == 0)
            return i;
    }
    return u32MAX;
}

//***********************************************************************
Bone* Skeleton::bone_getByName (const char *name) const
{
    const u32 index = bone_getIndexByName(name);
    if (u32MAX != index)
        return &boneList[index];
    return NULL;
}

//***********************************************************************
SkeletonInstance* Skeleton::newInstance() const
{
    return GOSNEW(allocator, SkeletonInstance)(this);
}

//***********************************************************************
u32 Skeleton::serialize_toMemory (u8 *out_buffer, u32 sizeof_buffer) const
{
    const u32 sizeof_header =
          sizeof(u32)   //magic
        + sizeof(u32);  //sizeof_memoryBlock

    const u32 sizeof_memoryBlock = 
          sizeof(u32)                   //numBones
        + sizeof(Bone) * numBones       //bone info
        + nameList.serialize_calcSizeNeeded();  //nomi

    
    const u32 total_size_needed = sizeof_header + sizeof_memoryBlock;
    if (NULL == out_buffer)
        return total_size_needed;
    if (sizeof_buffer < total_size_needed)
    {
        logger::err ("Skeleton::serialize => sizeof_buffer is to small (%d, expexted %d)\n", sizeof_buffer, total_size_needed);
        return 0;
    }

    u32 ct = 0;

    //header
    ct += utils::bufferWriteU32 (&out_buffer[ct], GOS_MAGIC__ENGINE_SKELETON);
    ct += utils::bufferWriteU32 (&out_buffer[ct], sizeof_memoryBlock);

    //memory block
    ct += utils::bufferWriteU32 (&out_buffer[ct], numBones);
    memcpy (&out_buffer[ct], boneList, sizeof(Bone) * numBones);
    ct += sizeof(Bone) * numBones;

    ct += nameList.serialize_toMemory (&out_buffer[ct], sizeof_buffer - ct);
    
    assert (ct == total_size_needed);
    assert (ct <= sizeof_buffer);
    return total_size_needed;
}

//***********************************************************************
void Skeleton::debug__print (gos::Logger *logger) const
{
    logger->log ("Num bones: %d\n", numBones);
    debug__print_rec (logger, &boneList[0]);
}
void Skeleton::debug__print_rec (gos::Logger *logger, const Bone *bone) const
{
    logger->log ("name: %s\n", nameList.getStringAtOffset(bone->nameIndex));
    logger->incIndent();

    u8 index = bone->firstChildIndex;
    while (0xFF != index)
    {
        bone = &boneList[index];
        debug__print_rec (logger, bone);
        index = bone->sigblinIndex;
    }
    logger->decIndent();
}



