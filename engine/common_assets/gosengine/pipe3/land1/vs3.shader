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
layout(location = 2) flat out vec3 out_diffuse_col;

vec3 color_sRGB_to_linear (float r, float g, float b)
{
    vec3 ret;
    ret.x = pow (r/255.0f, 2.2f);
    ret.y = pow (g/255.0f, 2.2f);
    ret.z = pow (b/255.0f, 2.2f);

    return ret;
}

void main() 
{
	const uint packed_index = meshInstanceData.data[gl_InstanceIndex].packed_index;

	const uint quad_index_0 = (instanceData.data[packed_index].quad_indices_0_1 & 0xFFFF0000) >> 16;
	const uint quad_index_1 = (instanceData.data[packed_index].quad_indices_0_1 & 0x0000FFFF);
	const uint quad_index_2 = (instanceData.data[packed_index].quad_indices_2_3 & 0xFFFF0000) >> 16;
	const uint quad_index_3 = (instanceData.data[packed_index].quad_indices_2_3 & 0x0000FFFF);
	const uint ref_vtx_idx = instanceData.data[packed_index].reference_vtx_idx;

	const float height = 0.1 * exaVtxInfo.data[ref_vtx_idx].height;
	const uint material_index = exaVtxInfo.data[ref_vtx_idx].material_index;


	const vec2 v3 = exaVtxList.v[quad_index_0];
	const vec2 v0 = exaVtxList.v[quad_index_1];
	const vec2 v1 = exaVtxList.v[quad_index_2];
	const vec2 v2 = exaVtxList.v[quad_index_3];

	const float tx_0_1 = in_position.x;
	const float tz_0_1 = in_position.z;
	
	const vec2 v_up   = mix (v0, v1, tx_0_1);
	const vec2 v_down = mix (v3, v2, tx_0_1);
	const vec2 v = mix (v_down, v_up, tz_0_1);

	const vec4 world = vec4 (v.x, height + in_position.y, v.y, 1);
	gl_Position = world * scene.camVP;


// 	const vec3 ax = (v2.xyz - v3.xyz);
// 	const vec3 az = (v0.xyz - v3.xyz);
//
// 	mat3 matW;
// 	matW[0][0] = ax.x;	matW[0][1] = 0;		matW[0][2] = az.x;
// 	matW[1][0] = ax.y;	matW[1][1] = 1;		matW[1][2] = az.y;
// 	matW[2][0] = ax.z;	matW[2][1] = 0;		matW[2][2] = az.z;

	const vec2 ax = (v2 - v3);
	const vec2 az = (v0 - v3);

	mat3 matW;
	matW[0][0] = ax.x;	matW[0][1] = 0;		matW[0][2] = az.x;
	matW[1][0] = 0;		matW[1][1] = 1;		matW[1][2] = 0;
	matW[2][0] = ax.y;	matW[2][1] = 0;		matW[2][2] = az.y;

	//normale
    out_normal = in_normal * matW;

    out_texCoord = in_texCoord;


	//material
    switch (material_index)
    {
		default: out_diffuse_col = color_sRGB_to_linear(255,0,255); break;
        case 1: out_diffuse_col = color_sRGB_to_linear(66,136,71); break;
        case 2: out_diffuse_col = color_sRGB_to_linear(89,170,92); break;
        case 3: out_diffuse_col = color_sRGB_to_linear(116,95,55); break;
		case 0xff: out_diffuse_col = color_sRGB_to_linear(76,58,20); break;
    }

}

