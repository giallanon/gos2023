#include "PIPE.include.shader"


//material parameteres
layout(std140, set = 2, binding = 0) readonly buffer SSBO_2_0
{
    vec4    color;
    uint    textureIndex;
} material;


struct sEsempio1
{
    float   fl;
    vec3    v3;
};

//obj data
layout(push_constant) uniform PushConstantData
{
    sEsempio1     es1;
    sEsempio1     arrayDiStruct[4];
    mat4    matW;
    vec2    unVecDi2Elem;
    float    width;
    int     unArrayDiIntDi8Elem[8];
} objInstance;
