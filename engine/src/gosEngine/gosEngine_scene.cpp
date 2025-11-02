#include "gosEngine_scene.h"

using namespace gos;
using namespace gos::engine;

//*************************************
void Scene::setup (gos::Allocator *allocatorIN)
{
    allocator = allocatorIN;
    entityList.setup (allocator, 1024);
}

//*************************************
void Scene::unsetup()
{
    if (NULL == allocator)
        return;

    entityList.unsetup();
    allocator = NULL;
}

//*************************************
void Scene::begin()
{
    entityList.reset();
}

//*************************************
void Scene::add (Entity ent)
{
    entityList.append (ent);
}

//*************************************
void Scene::end()
{
}

//*************************************
void Scene::query (geom::Camera3 &cam, ent::UniqueList *out_list, bool bClearList) const
{
    if (bClearList)
        out_list->reset();

    for (u32 i=0; i<entityList.getNElem(); i++)
    {
        out_list->insertIfNotExists(entityList(i));
    }
}


