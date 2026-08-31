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



void main() 
{
	const vec2 chunk_origin = instance_data.data[gl_InstanceIndex].chunk_originXZ;
	const vec2 tutv_offset = instance_data.data[gl_InstanceIndex].tutv_offset;
	const float scale_XZ = instance_data.data[gl_InstanceIndex].scale_XZ;
	const uint chunk_offset = instance_data.data[gl_InstanceIndex].chunk_data_offset;

	const uint height_and_stuff = chunk_data.data[chunk_offset + gl_VertexIndex].height_and_stuff;
		const float height_m = (height_and_stuff & 0x0000FFFF) * 0.1f;
		//const float AO = ((height_and_stuff & 0xFF000000) >> 24) / 255.0f;
		//const uint materialID = (height_	and_stuff & 0x00FF0000) >> 16;


	const vec2 world_origin = chunk_origin + in_position * scale_XZ;
	gl_Position = ( vec4(world_origin.x, height_m, world_origin.y, 1.0) ) * scene.camVP;

	//out_normal = vec3(0,1,0);
	out_normal = octahedral_decode (chunk_data.data[chunk_offset + gl_VertexIndex].encoded_norm, 16);

	out_texCoord = tutv_offset + in_texCoord * 0.25f;

	out_diffuse_col.rgb = vec3(1,1,1);
}

