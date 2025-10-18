#include "gosEntityRegistry.h"
#include "../gos/memory/gosAllocatorHeap.h"

using namespace gos;
using namespace gos::ent;


typedef gos::AllocatorHeap<gos::AllocPolicy_Track_simple, gos::AllocPolicy_Thread_Safe>		Ent_Registry_AllocatorTS;

//**********************************
void Registry::setup()
{
    Ent_Registry_AllocatorTS *aa = GOSNEW(gos::getSysHeapAllocator(), Ent_Registry_AllocatorTS)("Entity");;
    aa->setup (1024 * 1024 * 32);
    this->allocator = aa;

    nextEntID = 0;
    memset (sparseSetList, 0, sizeof(sparseSetList));
}

//**********************************
void Registry::priv_free()
{
    if (NULL == allocator)
        return;

    for (u32 i=0; i<NUM_MAX_COMPONENT_PER_ENTITY; i++)
    {
        if (NULL != sparseSetList[i])
        {
            IComponentList *list = reinterpret_cast<IComponentList*>(sparseSetList[i]);
            GOSDELETE(allocator, list);
        }
    }

    GOSDELETE(gos::getSysHeapAllocator(), allocator);
    allocator = NULL;
}

//**********************************
Entity Registry::newEntity()
{
    Entity e;
    e.id = nextEntID++;
    return e;
}