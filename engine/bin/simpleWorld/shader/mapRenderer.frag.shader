#version 450
#include "mapRenderer.include.shader"


//input
layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec4 in_color;
layout(location = 2) in vec2 in_texCoord;

//output
layout(location = 0) out vec4 out_colorRGBA;


//**** semplice calcolo luce
float calcLight (vec3 norm, float AO)
{
    //sun light
    float c = AO * max(dot(-scene.lightDir.xyz, norm), 0);
    
    //ambient light
    c += scene.lightDir.w;

    //clamp
    return min(max(c, 0), 1);    
}


void main() 
{
    const float AO = in_color.a;
    const float c = calcLight (in_normal, AO);

    out_colorRGBA = vec4(in_color.rgb * c, 1);
}
