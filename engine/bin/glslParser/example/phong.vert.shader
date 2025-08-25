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
    //[gl_Position] esiste automaticamente nei VertexShader e rappresenta le coordinate in clip space da passare al FragmentShader
	gl_Position = (vec4(in_position, 1.0) * objInstance.matW) * scene.camVP;

    //normale in world coordinate
    out_normal = in_normal * mat3(objInstance.matW);

    out_normal.x += objInstance.arrayDiStruct[1].fl + objInstance.es1.fl;
    
    out_texCoord = in_texCoord + objInstance.unVec2;


#ifdef  SBBO2_1
    out_texCoord.x += sbbo2_1.m1.x;
    out_texCoord.y += sbbo2_1.m1.y;
#endif

    out_texCoord.y += sbbo2_2.m3[1].z;
}
