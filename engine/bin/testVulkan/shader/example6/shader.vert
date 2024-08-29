#version 450

//uniform
layout(set = 0, binding = 0) uniform UniformBufferObject 
{
    mat4 camView;
    mat4 camProj;
    vec4 lightDir;
    mat4 objWorld;
} ubo;

//Input
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;

//output
layout(location = 0) out vec3 out_colorRGB;


void main() 
{
    //[gl_Position] esiste automaticamente nei VertexShader e rappresenta le coordinate in clip space da passare al FragmentShader
	gl_Position = vec4(in_position, 1.0) * ubo.objWorld * ubo.camView * ubo.camProj;

    vec3 norm = in_normal * mat3(ubo.objWorld);
    //vec4 norm = vec4(in_normal, 1) * ubo.objWorld;
    float c = max(-dot(ubo.lightDir.xyz, norm.xyz), 0);
    c += 0.1f;


    out_colorRGB = vec3(c,c,c);
}
