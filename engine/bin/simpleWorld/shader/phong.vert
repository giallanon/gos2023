#version 450

//uniform
layout(set = 0, binding = 0) uniform UBO_0_0
{
    mat4 camVP;
    vec4 lightDir;
} ubo;

layout(set = 1, binding = 0) uniform UBO_1_0
{
    vec3   color;
} material;

layout(push_constant) uniform PushConstantData
{
    mat4    matW;
} objInstance;

//Input
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texCoord;

//output
layout(location = 0) out vec3 out_colorRGB;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec4 out_lightDir;


void main() 
{
    //[gl_Position] esiste automaticamente nei VertexShader e rappresenta le coordinate in clip space da passare al FragmentShader
	gl_Position = vec4(in_position, 1.0) * objInstance.matW * ubo.camVP;

    //normale in world coordinate
    out_normal = in_normal * mat3(objInstance.matW);
    
    out_colorRGB = material.color;

    out_lightDir = ubo.lightDir;
}
