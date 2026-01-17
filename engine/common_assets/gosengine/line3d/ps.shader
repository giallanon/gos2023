#version 450
#include "common.shader"


//input
layout(location = 0) in vec4 in_colorRGBA;

//output
layout(location = 0) out vec4 out_colorRGBA;

void main() 
{
    out_colorRGBA = in_colorRGBA;
}
