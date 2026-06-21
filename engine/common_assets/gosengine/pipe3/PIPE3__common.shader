#extension GL_EXT_nonuniform_qualifier : require


//////////////////// descr-set 0
#define PIPE3_SAMPLER2D_BILINEAR  0
#define PIPE3_SAMPLER2D_POINT     1
layout(set = 0, binding = 0) uniform sampler   PIPE3_samplerList[2];
layout(set = 0, binding = 1) uniform texture2D PIPE3_textureList[];


//////////////////// descr-set 1
layout(set = 1, binding = 0) uniform UBO_1_0
{
    mat4 camVP;
    vec4 lightDir_and_ambient;
} scene;





//**** sample di una texture 2D con bilinear filtering
vec4 PIPE3_sample2D_bilinear (uint textureIndex, vec2 texCoord)
{
    return texture (sampler2D(PIPE3_textureList[textureIndex], PIPE3_samplerList[PIPE3_SAMPLER2D_BILINEAR]), texCoord);
}

//**** sample di una texture 2D con point filtering
vec4 PIPE3_sample2D_point (uint textureIndex, vec2 texCoord)
{
    return texture (sampler2D(PIPE3_textureList[textureIndex], PIPE3_samplerList[PIPE3_SAMPLER2D_POINT]), texCoord);
}

//**** semplice calcolo luce
float PIPE3_calcLight_01 (vec4 lightDir_and_ambient, vec3 norm)
{
    //sun light
    float c = max(-dot(lightDir_and_ambient.xyz, norm), 0);

    //ambient light
    c += lightDir_and_ambient.w;

    //clamp
    return min(max(c, 0), 1);
}
