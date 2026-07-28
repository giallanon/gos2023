#extension GL_EXT_nonuniform_qualifier : require


//////////////////// descr-set 0
#define PIPE3_SAMPLER2D_BILINEAR  			0
#define PIPE3_SAMPLER2D_POINT     			1
#define PIPE3_SAMPLER2D_BILINEAR_REPEAT  	2
layout(set = 0, binding = 0) uniform sampler   PIPE3_samplerList[3];
layout(set = 0, binding = 1) uniform texture2D PIPE3_textureList[];

//**** utils
vec3 color_sRGB_to_linear (float r, float g, float b)
{
    vec3 ret;
    ret.x = pow (r/255.0f, 2.2f);
    ret.y = pow (g/255.0f, 2.2f);
    ret.z = pow (b/255.0f, 2.2f);

    return ret;
}

//**** sample di una texture 2D con bilinear filtering
vec4 PIPE3_sample2D_bilinear (uint textureIndex, vec2 texCoord)
{
    return texture (sampler2D(PIPE3_textureList[textureIndex], PIPE3_samplerList[PIPE3_SAMPLER2D_BILINEAR]), texCoord);
}

vec4 PIPE3_sample2D_bilinear_REPEAT (uint textureIndex, vec2 texCoord)
{
    return texture (sampler2D(PIPE3_textureList[textureIndex], PIPE3_samplerList[PIPE3_SAMPLER2D_BILINEAR_REPEAT]), texCoord);
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
