#extension GL_EXT_nonuniform_qualifier : require

//sampler & texture array
//Un array di tutti i possibili sampler e un array
//di tutte le texture
//Questo layout (set 0) e' condiviso da tutti i renderer
#define PIPE_SAMPLER2D_BILINEAR  0
#define PIPE_SAMPLER2D_POINT     1
layout(set = 0, binding = 0) uniform sampler samplerList[];
layout(set = 0, binding = 1) uniform texture2D textureList[];
layout(set = 0, binding = 2) readonly buffer SSBO_0_2
{
	mat4	matW;
} matrixList[];

//scene
layout(set = 1, binding = 0) uniform LAYOUT_SCENE_DATA
{
    mat4 camVP;
    vec4 lightDir;
    vec2 screenWH;
} scene;


//psuh constant
layout(push_constant) uniform PushConstantData
{
	uint materialIndex;
    uint matrixIndex;
} objInstance;



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

