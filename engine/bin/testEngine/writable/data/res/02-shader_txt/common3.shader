#extension GL_EXT_nonuniform_qualifier : require

//////////////////// descr-set 0
#define PIPE_SAMPLER2D_BILINEAR  0
#define PIPE_SAMPLER2D_POINT     1
layout(set = 0, binding = 0) uniform sampler samplerList[2];
layout(set = 0, binding = 1) uniform texture2D textureList[];


//////////////////// descr-set 1
layout(set = 1, binding = 0) uniform UBO_1_0
{
    mat4 camVP;
    vec4 lightDir;
} scene;


//////////////////// descr-set 2
struct sMaterial
{
    vec3    diffuse_col;
    uint    texture_index;
};

layout(set = 2, binding = 0) readonly buffer SBO_2_0
{
    mat4	matW[];
} matrixList;

layout(set = 2, binding = 1) readonly buffer SBO_2_1
{
    sMaterial	material[];
} materialList;

struct sInstanceData
{
    uint shape_uid; //unused
    uint matrix_index;
    uint material_index;
};
layout(set = 2, binding = 2) readonly buffer SBO_2_2
{
    sInstanceData	data[];
} instanceData;





//**** sample di una texture 2D con bilinear filtering
vec4 PIPE_sample2D_bilinear (uint textureIndex, vec2 texCoord)
{
    return texture (sampler2D(textureList[textureIndex], samplerList[PIPE_SAMPLER2D_BILINEAR]), texCoord);
}

//**** sample di una texture 2D con point filtering
vec4 PIPE_sample2D_point (uint textureIndex, vec2 texCoord)
{
    return texture (sampler2D(textureList[textureIndex], samplerList[PIPE_SAMPLER2D_POINT]), texCoord);
}

//**** semplice calcolo luce
float PIPE_calcLight_01 (vec3 norm)
{
    //sun light
    float c = max(-dot(scene.lightDir.xyz, norm), 0);

    //ambient light
    c += scene.lightDir.w;

    //clamp
    return min(max(c, 0), 1);
}
