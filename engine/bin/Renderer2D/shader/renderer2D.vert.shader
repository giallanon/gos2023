#version 450
#include "renderer2D.include.shader"

//Input
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec4 in_color;
layout(location = 2) in vec2 in_texCoord;

//output
layout(location = 0) out vec4 out_color;
layout(location = 1) out vec2 out_texCoord;


void main() 
{
    //[gl_Position] esiste automaticamente nei VertexShader e rappresenta le coordinate in clip space da passare al FragmentShader
	gl_Position = vec4(in_position, 1.0) * scene.camVP;

    out_color = in_color;
    out_texCoord = in_texCoord;
}
