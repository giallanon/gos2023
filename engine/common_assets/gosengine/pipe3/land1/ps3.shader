#version 450
#include "descriptor.shader"

//input
layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec2 in_texCoord;
layout(location = 2) flat in uint in_material_index;

//output
layout(location = 0) out vec4 out_colorRGBA;
//layout(location = 1) out vec4 out_color2RGBA;


vec3 color_sRGB_to_linear (float r, float g, float b)
{
    vec3 ret;
    ret.x = pow (r/255.0f, 2.2f);
    ret.y = pow (g/255.0f, 2.2f);
    ret.z = pow (b/255.0f, 2.2f);

    return ret;
}

void main() 
{
    vec3 normal = normalize(in_normal);
    const float sunLight = PIPE3_calcLight_01 (scene.lightDir_and_ambient, normal);

	const uint texture_index = 0;
    
	//const vec3 texCol = PIPE3_sample2D_bilinear (texture_index, in_texCoord).rgb;

	vec3 diffuse_col;
    switch (in_material_index)
    {
        case 0: diffuse_col = color_sRGB_to_linear(66,136,71); break;
        case 1: diffuse_col = color_sRGB_to_linear(89,170,92); break;
        case 2: diffuse_col = color_sRGB_to_linear(116,95,55); break;
        //basetta
        case 0xFF:
            //diffuse_col = color_sRGB_to_linear(144,120,48); break;
            diffuse_col = color_sRGB_to_linear(200,200,200); break;
    }

    out_colorRGBA = vec4(diffuse_col * sunLight, 1);
}
