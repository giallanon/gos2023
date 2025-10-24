#extension GL_EXT_nonuniform_qualifier : require

//scene
layout(set = 0, binding = 0) uniform LAYOUT_SCENE_DATA
{
    mat4 camVP;
    vec4 lightDir;
} scene;


//push constant
layout(push_constant) uniform PushConstantData
{
	mat4	matW;
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
