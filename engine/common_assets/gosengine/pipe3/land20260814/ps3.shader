#version 450
#include "../PIPE3__descriptor_0.shader"
#include "../PIPE3__descriptor_1.shader"
#include "descriptor_2.shader"

//input
layout(location = 0) in vec3 in_normal;
layout(location = 1) flat in vec3 in_diffuse_col;
layout(location = 2) in vec2 in_texCoord;

//output
layout(location = 0) out vec4 out_colorRGBA;



void main() 
{
// 	const uint texture_index = 16;
// 	const vec3 texCol = PIPE3_sample2D_bilinear (texture_index, in_texCoord).rgb;


    vec3 normal = normalize(in_normal);
    const float sunLight = PIPE3_calcLight_01 (scene.lightDir_and_ambient, normal);

	out_colorRGBA = vec4((in_diffuse_col) * sunLight, 1);
	

}
