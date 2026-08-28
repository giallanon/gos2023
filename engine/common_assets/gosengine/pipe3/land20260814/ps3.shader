#version 450
#include "../PIPE3__descriptor_0.shader"
#include "../PIPE3__descriptor_1.shader"
#include "descriptor_2.shader"

//input
layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec3 in_diffuse_col;
layout(location = 2) in vec2 in_texCoord;
layout(location = 3) in float in_AO;

//output
layout(location = 0) out vec4 out_colorRGBA;



void main() 
{
// 	const uint texture_index = 16;
// 	const vec3 texCol = PIPE3_sample2D_bilinear (texture_index, in_texCoord).rgb;

    vec3 normal = normalize(in_normal);


	//const float sunLight = PIPE3_calcLight_01 (scene.lightDir_and_ambient, normal);
	//out_colorRGBA = vec4((in_diffuse_col) * sunLight, 1);


    //sun light
    float sunLight = max(-dot(scene.lightDir_and_ambient.xyz, normal), 0);
	vec3 final_col = in_diffuse_col * sunLight;

	//ambient color
	const vec3 sky_color = color_sRGB_to_linear(0x6e, 0xc8, 0xd4);
	const vec3 ground_color = color_sRGB_to_linear(0x8d, 0x92, 0x84);
	final_col += scene.lightDir_and_ambient.w * mix(ground_color, sky_color, normal.y);

	//final_col *= in_AO;
	final_col = vec3(in_AO, in_AO, in_AO);
	//final_col = normal;
	out_colorRGBA = vec4(final_col, 1);
	

}
