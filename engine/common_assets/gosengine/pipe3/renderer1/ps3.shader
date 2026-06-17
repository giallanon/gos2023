#version 450
#include "descriptor.shader"


//input
layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec2 in_texCoord;
layout(location = 2) in flat uint in_material_index;

//output
layout(location = 0) out vec4 out_colorRGBA;
//layout(location = 1) out vec4 out_color2RGBA;

void main() 
{
    vec3 normal = normalize(in_normal);
    const float sunLight = PIPE_calcLight_01 (normal);
    const sMaterial material = materialList.material[in_material_index];

    const vec3 texCol = PIPE_sample2D_bilinear (material.texture_index, in_texCoord).rgb;
    out_colorRGBA = vec4(material.diffuse_col * texCol * sunLight, 1);
}
