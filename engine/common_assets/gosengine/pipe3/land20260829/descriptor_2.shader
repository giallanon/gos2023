//////////////////// descr-set 2
struct sInstanceData
{
    vec2	chunk_originXZ;		// (x,z) in world coordinate dell'angolo in alto a sx
	vec2	tutv_offset;
    float 	scale_XZ;
	uint 	chunk_data_offset;
};

layout(std430, set = 2, binding = 0) readonly buffer SBO_2_0
{
    sInstanceData   data[];
} instance_data;



//per ogni chunk, qui ci sono le info su altezza e normali
struct sChunkData
{
	uint  encoded_norm;
    uint  height_and_stuff; //8bit ao, 8bit material, 16bitLSB per height
};

layout(std430, set = 2, binding = 1) readonly buffer SBO_2_1
{
    sChunkData   data[];
} chunk_data;
