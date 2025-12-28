#version 450
#include "phong.include.shader"

//Input
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texCoord;

//output
layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec2 out_texCoord;


void main() 
{
	const uint ii = objInstance.matrixIndex;
    
	gl_Position = (vec4(in_position, 1.0) * matrixList[ii].matW) * scene.camVP;

    //normale in world coordinate
    out_normal = in_normal * mat3(matrixList[ii].matW);
    
    out_texCoord = in_texCoord;
}
