#version 450
#include "PIPE.include.shader"

//per instance data
struct sPerInstanceData
{
    vec4    worldPos;
    float   altro;
};

layout(std140, set = 2, binding = 0) readonly buffer SSBO_2_0
{
    sPerInstanceData data[];
} perInstanceData;

/*layout(std140, set = 2, binding = 0) readonly buffer SSBO_2_0
{
    vec2 pippo[3];
    sPerInstanceData data[2];
} perInstanceData;
*/

//obj data
layout(push_constant) uniform PushConstantData
{
    layout(offset = 0) float    width;
} lineInfo;


//Input
layout(location = 0) in vec3 in_position;

//output
//layout(location = 0) out vec3 out_normal;
//layout(location = 1) out vec2 out_texCoord;


void main() 
{
    //worldA e B sono in world space
    const vec3 worldA = perInstanceData.data[gl_InstanceIndex].worldPos.xyz;
    const vec3 worldB = perInstanceData.data[gl_InstanceIndex+1].worldPos.xyz;
	
	//li trasformo in clip-space
    const vec4 clipA = vec4(worldA,1.0f) * scene.camVP;
    const vec4 clipB = vec4(worldB,1.0f) * scene.camVP;
	
    //in screen space
    const vec2 screenA = scene.screenWH * (0.5 * clipA.xy/clipA.w + 0.5);
    const vec2 screenB = scene.screenWH * (0.5 * clipB.xy/clipB.w + 0.5);

    const vec2 xBasis = normalize(screenB - screenA);
    const vec2 yBasis = vec2(-xBasis.y, xBasis.x);
  
	const vec2 pt0 = screenA + lineInfo.width * (in_position.y * yBasis);
	const vec2 pt1 = screenB + lineInfo.width * (in_position.y * yBasis);
	const vec2 pt = mix(pt0, pt1, in_position.x);	
    const vec4 clip = mix(clipA, clipB, in_position.x);
    
	gl_Position = vec4(clip.w * ((2.0 * pt) / scene.screenWH - 1.0), clip.z, clip.w);
	
}
