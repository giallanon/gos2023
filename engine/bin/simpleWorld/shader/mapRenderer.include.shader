#include "baseLayout.include.shader"


//per instance data
struct sPerInstanceData
{
    vec4    worldPosAndScale;
    vec4    colorAndAO;
};

layout(std140, set = 2, binding = 0) readonly buffer SSBO_2_0
{
    sPerInstanceData data[];
} perInstanceData;

