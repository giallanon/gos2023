#version 450
#include "PIPE.include.shader"


//input
//layout(location = 0) in vec3 in_normal;
//layout(location = 1) in vec2 in_texCoord;

//output
layout(location = 0) out vec4 out_colorRGBA;


//obj data
layout(push_constant) uniform PushConstantData
{
    layout(offset = 4) float    col;
} altroinPS;


void main() 
{
    out_colorRGBA = vec4(1,0,0,altroinPS.col);
}
