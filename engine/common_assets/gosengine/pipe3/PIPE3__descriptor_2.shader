//////////////////// descr-set 2
struct sMaterial
{
    vec3    diffuse_col;
    uint    texture_index;
};

layout(set = 2, binding = 0) readonly buffer SBO_2_0
{
    mat4	matW[];
} matrixList;

layout(set = 2, binding = 1) readonly buffer SBO_2_1
{
    sMaterial	material[];
} materialList;

struct sPackedInstanceData
{
    //nel renderer le info sono packet in un u64 con shape_uid (32bit) | material_index (14bit) | matrix_index (18bit)
    //Qui nello shader pero', l'u64 lo uso come 2 u32 ma gli MSB dell'u64 finiscono nel u32 basso.. si vede che la GPU e' little endian
    uint packed_material_and_matrix_index;  //LSB
    uint shape_uid;                         //MSB (al momento unused)
};

layout(set = 2, binding = 2) readonly buffer SBO_2_2
{
    sPackedInstanceData	data[];
} instanceData;



