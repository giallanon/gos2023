#include "PIPE.include.shader"


//material parameteres
layout(std140, set = 2, binding = 0) readonly buffer SSBO_2_0
{
    vec4    color;
    uint    textureIndex;
} material;

//obj data
layout(push_constant) uniform PushConstantData
{
    layout(offset = 0) mat4    matW;
} objInstance;