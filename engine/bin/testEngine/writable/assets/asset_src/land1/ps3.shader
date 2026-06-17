#version 450
#include "descriptor.shader"


//input
layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec2 in_texCoord;

//output
layout(location = 0) out vec4 out_colorRGBA;
//layout(location = 1) out vec4 out_color2RGBA;



float calcLight (vec3 norm)
{
    //sun light
    float c = max(-dot(scene.lightDir.xyz, norm), 0);

    //ambient light
    c += scene.lightDir.w;

    //clamp
    return min(max(c, 0), 1);
}



void main() 
{
    vec3 normal = normalize(in_normal);
    const float sunLight = calcLight (normal);

	const uint texture_index = 0;
	const vec3 diffuse_col = vec3(1,1,1);
    
	const vec3 texCol = PIPE_sample2D_bilinear (texture_index, in_texCoord).rgb;
    out_colorRGBA = vec4(diffuse_col * texCol * sunLight, 1);
}
