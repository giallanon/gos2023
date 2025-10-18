#include "gosEngine_fixedSizeBufferTracker.h"


using namespace gos;
using namespace gos::engine;


//*******************************
FixedSizeBufferTracker::FixedSizeBufferTracker()
{
    allocator = NULL;
    numObject = numMaxObject = 0;
}

//*******************************
void FixedSizeBufferTracker::setup (gos::Allocator *allocatorIN, u32 numMaxObjectIN)
{
    assert (numMaxObject <= 0xFFFF);

    allocator = allocatorIN;
    numMaxObject = numMaxObjectIN;
    numObject = 0;
    
    valid_if = GOSALLOCT(u16*, allocator, sizeof(u16) * numMaxObject);
    memset (valid_if, 0, sizeof(u16) * numMaxObject);

    is_busy.setup (allocator, numMaxObject);
    is_busy.zero();
}

//*******************************
void FixedSizeBufferTracker::unsetup ()
{
    if (NULL == allocator)
        return;

    GOSFREE(allocator, valid_if);
    is_busy.unsetup(allocator);
    allocator = NULL;
}

//*******************************
bool FixedSizeBufferTracker::bind (ResHandle *out)
{
    if (numObject >= numMaxObject)
        return false;

    u32 index;
    if (is_busy.findAndSetFirstFreeBit(&index))
    {
        numObject++;
        out->index = static_cast<u16>(index);
        out->valid_if = valid_if[index];
        return true;
    }

    return false;
}

//*******************************
void FixedSizeBufferTracker::unbind (ResHandle handle)
{
    if (is_busy.isBitSet (handle.index))
    {
        if (valid_if[handle.index] == handle.valid_if)
        {
            valid_if[handle.index]++;
            is_busy.clear (handle.index);

            assert (numObject > 0);
            numObject--;
        }
    }
}

//*******************************
bool FixedSizeBufferTracker::isBound (ResHandle handle) const
{
    if (is_busy.isBitSet (handle.index))
    {
        if (valid_if[handle.index] == handle.valid_if)
            return true;
    }
    return false;
}