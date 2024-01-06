#version 450

//input
layout(location = 0) in vec3 in_colorRGB;

//output
layout(location = 0) out vec4 out_colorRGBA;


void main() 
{
    out_colorRGBA = vec4(in_colorRGB, 1.0);
}