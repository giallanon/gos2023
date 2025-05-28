#ifndef _SPVReflectEnumAndDefine_h_
#define _SPVReflectEnumAndDefine_h_
#include "gos.h"
#include "spirv_reflect.h"


enum class eDescriptrorType : u8
{
    //sono pari pari ai corrispondenti enum di vulkan
    SAMPLER = 0,                //aka VK_DESCRIPTOR_TYPE_SAMPLER
    COMBINED_IMAGE_SAMPLER = 1, //aka VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
    TEXTURE2D = 2,
    STORAGE_IMAGE = 3,
    UNIFORM_TEXEL_BUFFER = 4,
    STORAGE_TEXEL_BUFFER = 5,
    UNIFORM_BUFFER = 6,
    STORAGE_BUFFER = 7,
    DYNAMIC_UNIFORM_BUFFER = 8,
    DYNAMIC_STORAGE_BUFFER = 9,
    INPUT_ATTACHMENT = 10,

    UNKNOWN = 0xff
};


enum eResourceType
{
    _struct = 0,
    _array = 1,
    _dynamicArray = 2,
};

struct sResAsStruct
{
    u8 numElem;
    char name[64][32];
    eDataFormat fmt[32];
};

struct sResAsArray
{
    u8  ordine;     //1=pippo[], 2=pippo[][], 3=pippo[][][]
    u8  numElem[4];    //per ogni "ordine", c'e' il num di elementi
};

struct sResAsDynamicArray
{
    u8  ordine;     //1=pippo[], 2=pippo[][], 3=pippo[][][]
};

union uResTypeDetails
{
    sResAsStruct        asStruct;
    sResAsArray         asArray;
    sResAsDynamicArray  asDynArray;
};

struct sResInfo
{
    eResourceType       type;
    uResTypeDetails    info;
};


#endif //_SPVReflectEnumAndDefine_h_