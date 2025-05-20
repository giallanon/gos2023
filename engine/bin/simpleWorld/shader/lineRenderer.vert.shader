#version 450
#include "PIPE.include.shader"

//per instance data
struct sPerInstanceData
{
    vec4    worldPos;
};

layout(std140, set = 2, binding = 0) readonly buffer SSBO_2_0
{
    sPerInstanceData data[];
} perInstanceData;


//Input
layout(location = 0) in vec3 in_position;

//output
//layout(location = 0) out vec3 out_normal;
//layout(location = 1) out vec2 out_texCoord;


#define LINE_WIDTH 3
void main() 
{
    //gl_Position = vec4(in_position, 1.0) * scene.camVP;
    
    /*const vec3 pointA = perInstanceData.data[gl_InstanceIndex].worldPos.xyz;
    const vec3 pointB = perInstanceData.data[gl_InstanceIndex+1].worldPos.xyz;
    gl_Position = vec4(in_position + pointB, 1.0) * scene.camVP;
    */
    
    //poinaA e B n clip space (0,0  -  1,1)
    const vec3 pointA = perInstanceData.data[gl_InstanceIndex].worldPos.xyz;
    const vec3 pointB = perInstanceData.data[gl_InstanceIndex+1].worldPos.xyz;
    const vec4 clip0 = vec4(pointA,1.0f) * scene.camVP;
    const vec4 clip1 = vec4(pointB,1.0f) * scene.camVP;

    //in screen space
    const vec2 screen0 = scene.screenWH * (0.5 * clip0.xy/clip0.w + 0.5);
    const vec2 screen1 = scene.screenWH * (0.5 * clip1.xy/clip1.w + 0.5);

    const vec2 xBasis = normalize(screen1 - screen0);
    const vec2 yBasis = vec2(-xBasis.y, xBasis.x);
    const vec2 pt0 = screen0 + LINE_WIDTH * (in_position.x * xBasis + in_position.y * yBasis);
    const vec2 pt1 = screen1 + LINE_WIDTH * (in_position.x * xBasis + in_position.y * yBasis);
    const vec2 pt = mix(pt0, pt1, in_position.z);

    const vec4 clip = mix(clip0, clip1, in_position.z);
    gl_Position = vec4(clip.w * ((2.0 * pt) / scene.screenWH - 1.0), clip.z, clip.w);
    
}
