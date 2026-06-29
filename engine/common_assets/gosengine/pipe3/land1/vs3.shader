#version 450
#include "../PIPE3__descriptor_0.shader"
#include "../PIPE3__descriptor_1.shader"
#include "descriptor_2.shader"


//Input
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texCoord;

//output
layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec2 out_texCoord;
layout(location = 2) flat out uint out_material_index;

void main() 
{
	const uint quad_index_0 = (instanceData.data[gl_InstanceIndex].quad_indices_0_1 & 0xFFFF0000) >> 16;
	const uint quad_index_1 = (instanceData.data[gl_InstanceIndex].quad_indices_0_1 & 0x0000FFFF);
	const uint quad_index_2 = (instanceData.data[gl_InstanceIndex].quad_indices_2_3 & 0xFFFF0000) >> 16;
	const uint quad_index_3 = (instanceData.data[gl_InstanceIndex].quad_indices_2_3 & 0x0000FFFF);
	const float height = instanceData.data[gl_InstanceIndex].height;

	const vec4 v0 = exaVtxList.v[quad_index_0];
	const vec4 v1 = exaVtxList.v[quad_index_1];
	const vec4 v2 = exaVtxList.v[quad_index_2];
	const vec4 v3 = exaVtxList.v[quad_index_3];


	const float tx_0_1 = in_position.x;
	const float tz_0_1 = in_position.z;
	
	const vec4 v_up   = mix (v0, v1, tx_0_1);
	const vec4 v_down = mix (v3, v2, tx_0_1);
	const vec4 v = mix (v_down, v_up, tz_0_1);

	const vec4 world = vec4 (v.x, height + in_position.y, v.z, v.w);
	gl_Position = world * scene.camVP;


	const vec3 ax = (v2.xyz - v3.xyz);
	const vec3 az = (v0.xyz - v3.xyz);
	//const vec3 ax = vec3(az.z, 0, -az.x);

	mat3 matW;
// 	matW[0][0] = ax.x;	matW[0][1] = ax.y;	matW[0][2] = ax.z;
// 	matW[1][0] = 0;		matW[1][1] = 1;		matW[1][2] = 0;
// 	matW[2][0] = az.x;	matW[2][1] = az.y;	matW[2][2] = az.z;

	matW[0][0] = ax.x;	matW[0][1] = 0;		matW[0][2] = az.x;
	matW[1][0] = ax.y;	matW[1][1] = 1;		matW[1][2] = az.y;
	matW[2][0] = ax.z;	matW[2][1] = 0;		matW[2][2] = az.z;

	//normale
    //out_normal = in_normal;
    out_normal = in_normal * matW;

    out_texCoord = in_texCoord;


	//material
//	out_material_index = instanceData.data[gl_InstanceIndex].material_index;
	if (0 == pc.is_basetta)
		out_material_index = instanceData.data[gl_InstanceIndex].material_index;
 	else
 		out_material_index = 0xFF;

}

