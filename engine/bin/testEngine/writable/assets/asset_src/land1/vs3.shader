#version 450
#include "common3.shader"

//Input
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texCoord;

//output
layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec2 out_texCoord;

void main() 
{
	const uint quad_index_0 = (instanceData.data[gl_InstanceIndex].quad_indices_0_1 & 0xFFFF0000) >> 16;
	const uint quad_index_1 = (instanceData.data[gl_InstanceIndex].quad_indices_0_1 & 0x0000FFFF);
	const uint quad_index_2 = (instanceData.data[gl_InstanceIndex].quad_indices_2_3 & 0xFFFF0000) >> 16;
	const uint quad_index_3 = (instanceData.data[gl_InstanceIndex].quad_indices_2_3 & 0x0000FFFF);

	const vec4 v0 = hexaVtxList.v[quad_index_0];
	const vec4 v1 = hexaVtxList.v[quad_index_1];
	const vec4 v2 = hexaVtxList.v[quad_index_2];
	const vec4 v3 = hexaVtxList.v[quad_index_3];


	const float tx_0_1 = in_position.x;
	const float tz_0_1 = in_position.z;
	
	const vec4 v_up   = mix (v0, v1, tx_0_1);
	const vec4 v_down = mix (v3, v2, tx_0_1);
	const vec4 v = mix (v_down, v_up, tz_0_1);

	gl_Position = v * scene.camVP;

    //normale 
    out_normal = in_normal;
    
    out_texCoord = in_texCoord;

}

