#include "gosModel.h"
#include "../gos/gosMagicUID.h"

using namespace gos;
using namespace gos::model;

//************************** 
Model::Model (gos::Allocator *allocatorIN, Skeleton *skeletonIN, const Mesh *meshListIN, u32 numMeshesIN)
{
    allocator = allocatorIN;
    skeleton = skeletonIN;
    numMeshes = numMeshesIN;
    meshList = GOSALLOCT(Mesh*, allocator, numMeshes * sizeof(Mesh));
    memcpy (meshList, meshListIN, numMeshes * sizeof(Mesh));
}

//************************** 
Model::~Model()
{
    if (NULL == allocator)
        return;
    GOSFREE(allocator, meshList);
    meshList = NULL;
    allocator = NULL;
}

