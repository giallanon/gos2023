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
    //out_colorRGBA = vec4(in_diffuse_col * sunLight, 1);
	
	
	
	float f01 = PIPE3_sample2D_bilinear_REPEAT (16, in_texCoord).r;
	if (f01 < 0.2)
		f01 = 0.7;
	else if (f01 < 0.4)
		f01 = 0.8;
	else if (f01 < 0.6)
		f01 = 0.9;
	
	
	/*const vec3 rock_color = color_sRGB_to_linear(200,200,200);
	const vec3 material_color_final = mix (color_sRGB_to_linear(66,136,71), color_sRGB_to_linear(89,170,92), f01);
	const vec3 diffuse_col = mix (rock_color, material_color_final, in_normal.y);	
	out_colorRGBA = vec4(diffuse_col * sunLight, 1);
	*/
	
	
	out_colorRGBA = vec4(in_diffuse_col * sunLight * f01, 1);
	

}
