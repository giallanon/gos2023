#version 450

//scene
layout(set = 0, binding = 0) uniform UBO_0_0
{
    mat4 camVP;
    vec4 lightDir;
} scene;

//layout(set = 1, binding = 0) uniform sampler2D sampler_diffuse[];

/*layout(set = 2, binding = 0) uniform UBO_2_0
{
    vec3    color;
    uint    textureIndex;
} material;
*/


layout(push_constant) uniform PushConstantData
{
    layout(offset = 0) mat4    matW;
} objInstance;

//Input
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texCoord;

//output
layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec2 out_texCoord;


void main() 
{
    //[gl_Position] esiste automaticamente nei VertexShader e rappresenta le coordinate in clip space da passare al FragmentShader
	gl_Position = vec4(in_position, 1.0) * objInstance.matW * scene.camVP;

    //normale in world coordinate
    out_normal = in_normal * mat3(objInstance.matW);
    
    out_texCoord = in_texCoord;
}
