#version 450
#include "../PIPE3__descriptor_0.shader"
#include "../PIPE3__descriptor_1.shader"
#include "../PIPE3__octahedra_compression.shader"
#include "descriptor_2.shader"


//Input
layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_texCoord;

//output
layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec3 out_diffuse_col;
layout(location = 2) out vec2 out_texCoord;
layout(location = 3) out float out_AO;



void main() 
{
 	const uint chunk_offset = instance_data.data[gl_InstanceIndex].chunk_data_offset;
 	const uint height_and_pad = chunk_data.data[chunk_offset + gl_VertexIndex].height_and_pad;
 	const float height_m = (height_and_pad & 0x0000FFFF) * 0.1f;
	const float AO = ((height_and_pad & 0xFF000000) >> 24) / 255.0f;
	const uint materialID = (height_and_pad & 0x00FF0000) >> 16;

	const vec2 chunk_origin = in_position + instance_data.data[gl_InstanceIndex].chunk_originXZ;
	const vec3 world_origin = vec3(chunk_origin.x, height_m, chunk_origin.y);

	//gl_Position = (vec4(in_position, 1.0) * matW) * scene.camVP;
	gl_Position = ( vec4(world_origin, 1.0) ) * scene.camVP;

    //out_normal = vec3(0,1,0);
	out_normal = octahedral_decode (chunk_data.data[chunk_offset + gl_VertexIndex].encoded_norm, 16);

	out_texCoord = in_texCoord;


	out_diffuse_col.rgb = material_list.data[materialID].color.rgb;

	out_AO = (1.0f - AO);
	//out_diffuse_col *= (1.0f - AO);

}

