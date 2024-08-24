#version 450

//uniform
layout(set = 0, binding = 0) uniform UniformBufferObject 
{
    mat4 view;
    mat4 proj;
    vec4 lightDir;

} ubo;

//Input
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_worldPos;
layout(location = 3) in vec3 in_color;

//output
layout(location = 0) out vec3 out_colorRGB;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_lightDir;


void main() 
{
    //[gl_Position] esiste automaticamente nei VertexShader e rappresenta le coordinate in clip space da passare al FragmentShader
	gl_Position = vec4(in_position + in_worldPos, 1.0) * ubo.view * ubo.proj;
    out_normal = in_normal;
    out_colorRGB = in_color;
    out_lightDir.xyz = ubo.lightDir.xyz;

    //float c = max(-dot(ubo.lightDir.xyz, in_normal), 0);
    //c += 0.1f;
    //out_colorRGB = in_color * c;
}
