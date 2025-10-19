#version 450
#extension GL_EXT_nonuniform_qualifier : require


#define PIPE_SAMPLER2D_BILINEAR  0
#define PIPE_SAMPLER2D_POINT     1

// layout(set = 0, binding = 0) uniform sampler sampler1;
// layout(set = 0, binding = 1) uniform sampler samplerList[];
// layout(set = 0, binding = 2) uniform sampler samplerList2[4];
//
// layout(set = 0, binding = 3) uniform texture2D texture1;
// layout(set = 0, binding = 4) uniform texture2D textureList[];
// layout(set = 0, binding = 5) uniform texture2D textureList2[8];
//
//
// layout(set = 1, binding = 0) uniform UBO_0
// {
//     uint   pippo;
// } ubo_struct;
//
// layout(set = 1, binding = 1) uniform UBO_1
// {
//     uint   pippo;
// } dyn_ubo_struct;

layout(set = 1, binding = 2) uniform UBO_2
{
    uint    pippo;
    vec2    screenWH;
} bindless_ubo_array;



// layout(set = 2, binding = 0) readonly buffer SBO_0
// {
//     uint   pippo;
// } sbo_struct;
//
//
// layout(set = 2, binding = 1) readonly buffer SBO_1
// {
//     uint   pippo;
// } dyn_sbo_struct;

// layout(set = 2, binding = 2) uniform SBO_2
// {
//     uint   pippo;
// } bindless_sbo_array[];


//layout(set = 0, binding = 1) uniform texture2D textureList[];


//Input
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texCoord;

//output
layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec2 out_texCoord;


// //**** sample di una texture 2D con bilinear filtering
// vec4 PIPE_sample2D_bilinear (uint textureIndex, vec2 texCoord)
// {
//     return texture (sampler2D(textureList[textureIndex], samplerList[PIPE_SAMPLER2D_BILINEAR]), texCoord);
// }
//
// //**** sample di una texture 2D con point filtering
// vec4 PIPE_sample2D_point (uint textureIndex, vec2 texCoord)
// {
//     return texture (sampler2D(textureList[textureIndex], samplerList[PIPE_SAMPLER2D_POINT]), texCoord);
// }
//

void main()
{
    //[gl_Position] esiste automaticamente nei VertexShader e rappresenta le coordinate in clip space da passare al FragmentShader
	gl_Position = vec4(in_position, 1.0);

    //normale in world coordinate
    out_normal = in_normal;

    out_texCoord = in_texCoord;
}
