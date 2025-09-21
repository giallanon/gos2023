#include "gosGPUPipe2_pipeline_def.h"
#include "../gosGPU.h"


using namespace gos;
using namespace gos::gpu;
using namespace gos::gpu::pipe2;

//******************************************
void Pipeline::deleteResources (GPU *gpu)
{
    for (u32 i=0; i<descrset_num; i++)
    {
        gpu->deleteResource (descrset_handle_defList[i]);
    }
    gpu->deleteResource (pipeline_handle);
    reset();
}