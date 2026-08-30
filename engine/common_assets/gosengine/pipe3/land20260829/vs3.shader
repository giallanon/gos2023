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
	const vec2 scale_XZ = instance_data.data[gl_InstanceIndex].scale_XZ;
	const vec2 tutv_offset = instance_data.data[gl_InstanceIndex].tutv_offset;
	const vec2 tutv_scale = instance_data.data[gl_InstanceIndex].tutv_scale;
	const vec2 chunk_origin = instance_data.data[gl_InstanceIndex].chunk_originXZ;

	const vec2 world_origin = chunk_origin + in_position * scale_XZ;
	gl_Position = ( vec4(world_origin.x, 0, world_origin.y, 1.0) ) * scene.camVP;

	out_normal = vec3(0,1,0);

	out_texCoord = tutv_offset + in_texCoord * tutv_scale;

	out_diffuse_col.rgb = vec3(1,1,1);
}

