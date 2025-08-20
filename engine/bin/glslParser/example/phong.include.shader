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
    float    width;
    sEsempio1     es1;
    sEsempio1     arrayDiStruct[5];
    mat4    matW;
    vec2    unVec2;
    int     unArray3x5diInt[3][5];
    vec2    unArrayDiVec2Di6Elem[6];
    mat4    unArrayDiMat42Di5Elem[5];
    float   height;
} objInstance;
