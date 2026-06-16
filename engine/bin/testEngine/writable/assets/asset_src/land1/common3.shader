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


//elenco dei vertici degli exa da renderizzare
layout(set = 2, binding = 0) readonly buffer SBO_2_0
{
    vec4	v[];
} hexaVtxList;

struct sPackedInstanceData
{
	uint	quad_indices_0_1;	//2 indici da 16 bit
	uint	quad_indices_2_3;	//altri 2 indici da 16 bit
};
layout(set = 2, binding = 1) readonly buffer SBO_2_2
{
    sPackedInstanceData	data[];
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
