#extension GL_EXT_nonuniform_qualifier : require

//////////////////// descr-set 0
layout(set = 0, binding = 0) uniform UBO_0_0
{
    mat4    camVP;
    vec2    screen_wh;

} scene;


//////////////////// descr-set 1
layout(set = 1, binding = 0) readonly buffer SBO_1_0
{
    float	pos[];    //NB: sarebbe bello avere dei vec3 qui ma Vulkan fa casino con gli allineamenti!
} vtxList;


layout(set = 1, binding = 1) readonly buffer SBO_1_1
{
    uint	idx[];     //ogni uint rappresenta l'indice del vtx-1 (16bit MSB) | vtx-2 (16 bit LSB)
} indexList;


//////////////////// psu constant
layout(push_constant) uniform PushConstantData
{
    vec4    color_RGBA;
    uint    line_width;
} pc;
