#version 450
#include "common2.shader"


//input
layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec2 in_texCoord;

//output
layout(location = 0) out vec4 out_colorRGBA;
//layout(location = 1) out vec4 out_color2RGBA;

void main() 
{
    const float sunLight = PIPE_calcLight_01 (in_normal);
    const sMaterial material = materialList.material[objInstance.materialIndex];

    const vec3 texCol = PIPE_sample2D_bilinear (material.texture_index, in_texCoord).rgb;
    out_colorRGBA = vec4(material.diffuse_col * texCol * sunLight, 1);
}
