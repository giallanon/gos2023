#include "gosSkeleton.h"

using namespace gos;

/**********************************************************************
 * 
 * BONE
 * 
 **********************************************************************/
Bone* Bone::addChild (gos::Bone *b)
{
    assert (NULL != b);
    assert (NULL == b->sibling);
    if (NULL == firstChild)
    {
        firstChild = b;
    }
    else
    {
        b->sibling = firstChild;
        firstChild = b;
    }
    return b;
}

Bone* Bone::addSibling (gos::Bone *b)
{
    assert (NULL != b);
    assert (NULL == b->sibling);
    if (NULL == sibling)
    {
        sibling = b;
    }
    else
    {
        b->sibling = sibling;
        sibling = b;
    }
    return b;
}

/**********************************************************************
 * 
 * SKELETON
 * 
 **********************************************************************/
void Skeleton::free()
{
    if (NULL != root)
    {
        priv_free_ric (root);
        root = NULL;
    }
}

void Skeleton::priv_free_ric (gos::Bone *b)
{
    if (NULL != b->firstChild)
        priv_free_ric (b->firstChild);

    if (NULL != b->sibling)
        priv_free_ric (b->sibling);

    GOSFREE(allocator, b);
}

Bone* Skeleton::createRoot (gos::Allocator *allocatorIN)
{
    allocator = allocatorIN;
    root = newBone();
    return root;
}

Bone* Skeleton::newBone()
{
    Bone *bone = GOSALLOCT(Bone*, allocator, sizeof(Bone));
    bone->matrix.identity();
    bone->firstChild = bone->sibling = NULL;
    return bone;
}
