#include "Renderer2D.h"

using namespace gos;


//********************************
Renderer2D::Renderer2D()
{
    gpu = NULL;
    localAllocator = NULL;
}

//********************************
Renderer2D::~Renderer2D()
{
    unsetup();
}

//********************************
void Renderer2D::unsetup()
{
    if (NULL != gpu)
    {
        gpu = NULL;
    }

    if (NULL != localAllocator)
    {
        vtxList.unsetup();
        idxList.unsetup();
        localAllocator = NULL;
    }
}

//********************************
bool Renderer2D::setup (gos::GPU *gpuIN, gos::Allocator *allocatorIN)
{
    gpu = gpuIN;
    localAllocator = allocatorIN;

    flags.zero();
    vtxList.setup (localAllocator, 1024*1024);
    idxList.setup (localAllocator, 1024*1024*3);
 
    return true;
}

//********************************
void Renderer2D::begin()
{
    assert (!flags.isBitSet(FLAG__IS_BEGIN));
    flags.set (FLAG__IS_BEGIN);

    fifoLineSize.reset();
    fifoLineSize.push(1);

    fifoColor.reset();
    fifoColor.push(ColorHDR(255, 255, 255, 255));
}

//********************************
void Renderer2D::end()
{
    if (!flags.isBitSet(FLAG__IS_BEGIN))
    {
        DBGBREAK;
        return;
    }

    flags.clear (FLAG__IS_BEGIN);
}

//********************************
gos::ColorHDR Renderer2D::getColor() const
{
    ColorHDR color;
    fifoColor.top (color);
    return color;
}

//********************************
u8 Renderer2D::getLineSize() const
{
    u8 s;
    fifoLineSize.top (s);
    return s;
}

//********************************
void Renderer2D::beginPrimitive (ePrimitive p)
{
    assert (flags.isBitSet(FLAG__IS_BEGIN));
    assert (!flags.isBitSet(FLAG__IS_BEGIN_PRIMITIVE));
}

//********************************
void Renderer2D::endPrimitive()
{
    if (!flags.isBitSet(FLAG__IS_BEGIN_PRIMITIVE))
    {
        DBGBREAK;
        return;
    }

    flags.clear (FLAG__IS_BEGIN_PRIMITIVE);
}

//********************************
u32 Renderer2D::addVtx (const gos::vec3f &pos, const gos::ColorHDR &color, const gos::vec2f &tutv)
{
    assert (flags.isBitSet(FLAG__IS_BEGIN_PRIMITIVE));
    
    const u32 ret = vtxList.getNElem();

    vtxList[ret].pos = pos;
    vtxList[ret].tutv = tutv;
    vtxList[ret].color.set (color.col.r, color.col.g, color.col.b, color.col.a);
        
    return  ret;
}

//********************************
void Renderer2D::addIdx (u32 i0)
{
    assert (flags.isBitSet(FLAG__IS_BEGIN_PRIMITIVE));
    assert (i0 < vtxList.getNElem());
    idxList.append(i0);
}

