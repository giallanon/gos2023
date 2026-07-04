//////////////////// descr-set 2
//elenco dei vertici degli exa da renderizzare
layout(set = 2, binding = 0) readonly buffer SBO_2_0
{
    vec2	v[];
} exaVtxList;


struct VtxInfo
{
	uint height;
	uint material_index;
};

layout(set = 2, binding = 1) readonly buffer SBO_2_1
{
	VtxInfo	data[];
} exaVtxInfo;



struct sPackedInstanceData
{
	uint   	quad_indices_0_1;	//2 indici da 16 bit
	uint   	quad_indices_2_3;	//altri 2 indici da 16 bit
	uint	reference_vtx_idx;
};

layout(set = 2, binding = 2) readonly buffer SBO_2_2
{
    sPackedInstanceData	data[];
} instanceData;


struct sSBO2_3
{
	uint   	packed_index;
};

layout(set = 2, binding = 3) readonly buffer SBO_2_3
{
    sSBO2_3	data[];
} meshInstanceData;

