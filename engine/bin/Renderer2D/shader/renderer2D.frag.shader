#version 450
#include "renderer2D.include.shader"


//input
layout(location = 0) in vec4 in_color;
layout(location = 1) in vec2 in_texCoord;

//output
layout(location = 0) out vec4 out_colorRGBA;

void main() 
{
    out_colorRGBA = in_color;
}
