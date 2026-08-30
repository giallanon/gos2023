//////////////////// descr-set 2
struct sInstanceData
{
    vec2	chunk_originXZ;		// (x,z) in world coordinate dell'angolo in alto a sx
    vec2 	scale_XZ;
	vec2	tutv_offset;
	vec2	tutv_scale;
};

layout(set = 2, binding = 0) readonly buffer SBO_2_0
{
    sInstanceData   data[];
} instance_data;


