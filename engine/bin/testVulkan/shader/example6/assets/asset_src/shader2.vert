#version 450

//uniform
layout(set = 0, binding = 0) uniform UBO_0_0 
{
    mat4 camView;
    mat4 camProj;
    vec4 lightDir;
    mat4 objWorld;
} ubo;

//Input
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_texCoord;
layout(location = 2) in vec3 in_normal;

//output
layout(location = 0) out vec3 out_colorRGB;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_lightDir;


void main() 
{
    //[gl_Position] esiste automaticamente nei VertexShader e rappresenta le coordinate in clip space da passare al FragmentShader
	gl_Position = vec4(in_position, 1.0) * ubo.objWorld * ubo.camView * ubo.camProj;

    out_colorRGB = vec3(1,1,1);
    out_normal = in_normal;
    out_lightDir.xyz = ubo.lightDir.xyz;
}
