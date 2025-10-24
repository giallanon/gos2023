#extension GL_EXT_nonuniform_qualifier : require

//scene
layout(set = 0, binding = 0) uniform LAYOUT_SCENE_DATA
{
    mat4 camVP;
    vec4 lightDir;
} scene;

//lista di matrici degli oggetti
layout(set = 1, binding = 0) readonly buffer SBO_1_0
{
    mat4	matW[];
} matrixList;


//push constant
layout(push_constant) uniform PushConstantData
{
	uint matrixIndex;
} objInstance;

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
