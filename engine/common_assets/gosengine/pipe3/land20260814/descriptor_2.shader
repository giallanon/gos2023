//////////////////// descr-set 2
struct sInstanceData
{
    vec2	chunk_originXZ;		// (x,z) in world coordinate
    uint    chunk_data_offset;	//indica l'offset all'interno di chunk_data[]
    uint 	pad0;
};

layout(set = 2, binding = 0) readonly buffer SBO_2_0
{
    sInstanceData   data[];
} instance_data;


//per ogni chunk, qui ci sono le info su altezza e normali
struct sChunkData
{
    uint  height_and_pad; //16bit MSB unused, 16bitLSB per height
};

layout(std430, set = 2, binding = 1) readonly buffer SBO_2_1
{
    sChunkData   data[];
} chunk_data;
