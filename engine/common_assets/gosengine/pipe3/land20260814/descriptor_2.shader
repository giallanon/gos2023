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
	uint  encoded_norm;
    uint  height_and_pad; //8bit ao, 8bit material, 16bitLSB per height
};

layout(std430, set = 2, binding = 1) readonly buffer SBO_2_1
{
    sChunkData   data[];
} chunk_data;


//elenco dei possibili materiali
struct sMaterial
{
	vec4	color;
};

layout(std430, set = 2, binding = 2) readonly buffer SBO_2_2
{
    sMaterial   data[];
} material_list;
