
//questo include idealmente dovrebbe essere 
//#include "<common_assets>/gosengine/pipe3/common3.shader"
#include "gosengine_pipe3_common3.shader"


//////////////////// descr-set 1
layout(set = 1, binding = 0) uniform UBO_1_0
{
    mat4 camVP;
    vec4 lightDir;
} scene;



//////////////////// descr-set 2
//elenco dei vertici degli exa da renderizzare
layout(set = 2, binding = 0) readonly buffer SBO_2_0
{
    vec4	v[];
} hexaVtxList;

struct sPackedInstanceData
{
	uint	quad_indices_0_1;	//2 indici da 16 bit
	uint	quad_indices_2_3;	//altri 2 indici da 16 bit
};
layout(set = 2, binding = 1) readonly buffer SBO_2_2
{
    sPackedInstanceData	data[];
} instanceData;


