#version 450
#include "descriptor.shader"


//input
layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec2 in_texCoord;

//output
layout(location = 0) out vec4 out_colorRGBA;
//layout(location = 1) out vec4 out_color2RGBA;


void main() 
{
    vec3 normal = normalize(in_normal);
    const float sunLight = PIPE3_calcLight_01 (scene.lightDir_and_ambient, normal);

	const uint texture_index = 0;
	const vec3 diffuse_col = vec3(1,1,1);
    
	//const vec3 texCol = PIPE3_sample2D_bilinear (texture_index, in_texCoord).rgb;
    const vec3 texCol = vec3(75.0f/255.0f, 190.0f/255.0f, 40.0f/255.0f);
    out_colorRGBA = vec4(diffuse_col * texCol * sunLight, 1);
}
