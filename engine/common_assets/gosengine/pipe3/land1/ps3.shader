#version 450
#include "../PIPE3__descriptor_0.shader"
#include "../PIPE3__descriptor_1.shader"
#include "descriptor_2.shader"

//input
layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec2 in_texCoord;
layout(location = 2) flat in vec3 in_diffuse_col;

//output
layout(location = 0) out vec4 out_colorRGBA;
//layout(location = 1) out vec4 out_color2RGBA;


void main() 
{
    vec3 normal = normalize(in_normal);
    const float sunLight = PIPE3_calcLight_01 (scene.lightDir_and_ambient, normal);

	const uint texture_index = 0;
    
	//const vec3 texCol = PIPE3_sample2D_bilinear (texture_index, in_texCoord).rgb;
    out_colorRGBA = vec4(in_diffuse_col * sunLight, 1);
}
