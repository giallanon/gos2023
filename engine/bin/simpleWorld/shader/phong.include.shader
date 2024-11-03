#extension GL_EXT_nonuniform_qualifier : require

//scene
layout(set = 0, binding = 0) uniform UBO_0_0
{
    mat4 camVP;
    vec4 lightDir;
} scene;

//sampler & texture array
layout(set = 1, binding = 0) uniform sampler samplerList[];
layout(set = 1, binding = 1) uniform texture2D textureList[];

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

