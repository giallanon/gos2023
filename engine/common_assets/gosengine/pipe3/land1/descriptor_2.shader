//////////////////// descr-set 2
//elenco dei vertici degli exa da renderizzare
layout(set = 2, binding = 0) readonly buffer SBO_2_0
{
    vec4	v[];
} exaVtxList;

struct sPackedInstanceData
{
	uint   quad_indices_0_1;	//2 indici da 16 bit
	uint   quad_indices_2_3;	//altri 2 indici da 16 bit
	uint   material_index;
	float  height;
};

layout(set = 2, binding = 1) readonly buffer SBO_2_1
{
    sPackedInstanceData	data[];
} instanceData;


//////////////////// push constant
layout(push_constant) uniform PushConstantData
{
    uint    is_basetta;
} pc;
