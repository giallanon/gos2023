#ifndef _Renderer2D_h_
#define _Renderer2D_h_
#include "../gosGPU/gosGPU.h"
#include "../gos/gosBit.h"
#include "../gos/gosFIFOFixedSize.h"


/**
 * @brief Renderer2D
 *  
 */
class Renderer2D
{
public:
    enum class ePrimitive : u8
    {
        lineList = 0,
        trisList = 1
    };

public:
            Renderer2D();
            ~Renderer2D();

    bool    setup (gos::GPU *gpu, gos::Allocator *allocator);
    void    unsetup();

    void    begin();
    void            pushColor (const gos::ColorHDR &color)                                  { fifoColor.push(color); }
    void            popColor ()                                                             { gos::ColorHDR color; fifoColor.pop (color); }
    gos::ColorHDR   getColor() const;
    void            pushLineSize (u8 size)                                                  { fifoLineSize.push(size); }
    void            popLineSize ()                                                          { u8 s; fifoLineSize.pop (s); }
    u8              getLineSize() const;
    void    end();

    void    beginPrimitive (ePrimitive p);
    u32         addVtx (const gos::vec3f &pos)                                              { return addVtx (pos, getColor(), gos::vec2f(0,0)); }
    u32         addVtx (const gos::vec3f &pos, const gos::ColorHDR &color)                  { return addVtx (pos, color, gos::vec2f(0,0)); }
    u32         addVtx (const gos::vec3f &pos, const gos::vec2f &tutv)                      { return addVtx (pos, getColor(), tutv); }
    u32         addVtx (const gos::vec3f &pos, const gos::ColorHDR &color, const gos::vec2f &tutv);
    void        addIdx (u32 i0);
    void        addIdx (u32 i0, u32 i1)                                                     { addIdx(i0); addIdx(i1); }
    void        addIdx (u32 i0, u32 i1, u32 i2)                                             { addIdx(i0); addIdx(i1); addIdx(i2); }
    void    endPrimitive();


private:
    static constexpr u8     FLAG__IS_BEGIN = 0;
    static constexpr u8     FLAG__IS_BEGIN_PRIMITIVE = 1;

private:
    struct Vertex
    {
        gos::vec3f  pos;
        gos::vec4f  color;  //TODO: cambiare in u 32
        gos::vec2f  tutv;
    };

private:
    gos::GPU                    *gpu;
    gos::Allocator              *localAllocator;
    gos::FastArray<Vertex>      vtxList;
    gos::FastArray<u16>         idxList;
    gos::Flag16                 flags;
    gos::FIFOFixedSize<u8, 128>             fifoLineSize;
    gos::FIFOFixedSize<gos::ColorHDR, 128>  fifoColor;
};



#endif //_Renderer2D_h_
