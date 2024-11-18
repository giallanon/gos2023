#version 450
#include "mapRenderer.include.shader"

//Input
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texCoord;

//output
layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec4 out_color;
layout(location = 2) out vec2 out_texCoord;


void main() 
{
    const float scale = perInstanceData.data[gl_InstanceIndex].worldPosAndScale.w;
    const vec3 worldPos = (in_position*scale) + perInstanceData.data[gl_InstanceIndex].worldPosAndScale.xyz;
    gl_Position = vec4(worldPos,1.0f) * scene.camVP;

    //normale in world coordinate
    out_normal = in_normal;
    
    //color
    out_color = perInstanceData.data[gl_InstanceIndex].colorAndAO;

    //tex coord
    out_texCoord = in_texCoord;
}
