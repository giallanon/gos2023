#version 450
#include "common3.shader"

//Input
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texCoord;

//output
layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec2 out_texCoord;
layout(location = 2) out flat uint out_material_index;

void main() 
{
    const uint packed_material_and_matrix_index = instanceData.data[gl_InstanceIndex].packed_material_and_matrix_index;
    //const uint matrix_index = instanceData.data[gl_InstanceIndex].matrix_index;
    //out_material_index = instanceData.data[gl_InstanceIndex].material_index;
    const uint matrix_index = (packed_material_and_matrix_index & 0x3FFFF);
    out_material_index = (packed_material_and_matrix_index  >> 18) & 0x3FFF;

	const mat4 matW = matrixList.matW[matrix_index];
	gl_Position = (vec4(in_position, 1.0) * matW) * scene.camVP;

    //normale in world coordinate
    out_normal = in_normal * mat3(matW);
    
    out_texCoord = in_texCoord;

}

