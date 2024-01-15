#version 450

//uniform
layout(binding = 0) uniform UniformBufferObject 
{
    mat4 world;
    mat4 view;
    mat4 proj;
} ubo;

//Input
layout(location = 0) in vec2 in_position;
layout(location = 1) in vec3 in_colorRGB;

//output
layout(location = 0) out vec3 out_colorRGB;


void main() 
{
    //[gl_Position] esiste automaticamente nei VertexShader e rappresenta le coordinate in clip space da passare al FragmentShader
	gl_Position = ubo.proj * ubo.view * ubo.world * vec4(in_position, 0.0, 1.0);;
    out_colorRGB = in_colorRGB;
}
