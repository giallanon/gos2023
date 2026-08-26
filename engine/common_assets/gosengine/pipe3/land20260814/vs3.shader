#version 450
#include "../PIPE3__descriptor_0.shader"
#include "../PIPE3__descriptor_1.shader"
#include "descriptor_2.shader"


//Input
layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_texCoord;

//output
layout(location = 0) out vec3 out_normal;
layout(location = 1) flat out vec3 out_diffuse_col;
layout(location = 2) out vec2 out_texCoord;



void main() 
{
 	const uint chunk_offset = instance_data.data[gl_InstanceIndex].chunk_data_offset;
 	const uint height_and_pad = chunk_data.data[chunk_offset + gl_VertexIndex].height_and_pad;
 	const float height = (height_and_pad & 0x0000FFFF) * 0.1f;

	const vec2 chunk_origin = in_position + instance_data.data[gl_InstanceIndex].chunk_origin;
	const vec3 world_origin = vec3(chunk_origin.x, height, chunk_origin.y);

	//gl_Position = (vec4(in_position, 1.0) * matW) * scene.camVP;
	gl_Position = ( vec4(world_origin, 1.0) ) * scene.camVP;

    out_normal = vec3(0,1,0);
	out_diffuse_col = vec3(1,1,1);
	out_texCoord = in_texCoord;

	switch (instance_data.data[gl_InstanceIndex].pad0)
	{
	default:  	out_diffuse_col = vec3(0,0,0); break;
	case 0: 	out_diffuse_col = vec3(1,1,1); break;
	case 1: 	out_diffuse_col = vec3(1,1,1); break;
	case 2: 	out_diffuse_col = vec3(1,0,0); break;
	case 3: 	out_diffuse_col = vec3(0,1,0); break;
	case 4: 	out_diffuse_col = vec3(0,0,1); break;
	}

/*
	out_texCoord.x = in_texCoord.x * 0.25;
	out_texCoord.y = in_texCoord.y * 0.5;

	const uint lod = instance_data.data[gl_InstanceIndex].lod;
	if (lod < 4)
	{
		out_texCoord.x += lod * 0.25f;
	}
	else
	{
		out_texCoord.x += (lod-4) * 0.25f;
		out_texCoord.y += 0.5;
	}
*/
}

