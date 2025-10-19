#version 450
#include "phong.include.shader"


//input
layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec2 in_texCoord;

//output
layout(location = 0) out vec4 out_colorRGBA;
//layout(location = 1) out vec4 out_color2RGBA;

void main() 
{
    const float sunLight = PIPE_calcLight_01 (in_normal);

    //const uint ii = nonuniformEXT(material.textureIndex);
    const vec3 texCol = PIPE_sample2D_bilinear (material.textureIndex, in_texCoord).rgb;
    out_colorRGBA = vec4(material.color.rgb * texCol * sunLight, 1);
}
