#ifndef _VkEx5MarchingSquare_h_
#define _VkEx5MarchingSquare_h_
#include "../gos/gos.h"
#include "../gos/gosFastArray.h"
#include "../gosMath/gosVect.h"
#include "SimpleLineRenderer.h"

/**
 * @brief VkEx5MarchingSquare
 * usato in Vulkan Example5
 */
class VkEx5MarchingSquare
{
public:
    class Map
    {
    public:
                        Map()                       { }
        virtual         ~Map()                      { }
        virtual u32     getDimX() const = 0;
        virtual u32     getDimY() const = 0;
        virtual bool    isON (u32 x, u32 y) const = 0;
    };


public:
    void    algo2 (const Map &map, SimpleLineRenderer &line);


private:
    //L'algo nella sua forma basilare ritorna una serie di segmenti che formano delle linee a delimitare i perimetri della mappa in input.
    //In un secondo passo, queste linee vengono arrotondate e, per ogni segmento originale, vengono creati SMOOTH_LEVEL segmenti
    static constexpr u8 SMOOTH_LEVEL = 3;

    enum class ePos : u8
    {
        a = 0,
        b = 1,
        c = 2,
        d = 3
    };
   
private:
    struct sEdge
    {
        u16 i0;
        u16 i1;
    };
    
    class VtxHelper2
    {

    public:
                VtxHelper2 (gos::Allocator *allocator, const Map &map);
                ~VtxHelper2 ();

        sEdge   addEdge (u16 worldX, u16 worldY, ePos pos1, ePos pos2);
    
    public:
        gos::FastArray<gos::vec3f>  vtxList;
        gos::FastArray<sEdge>       edgeList;

    private:
        u16     priv_vtxAddIfNeeded (u16 worldX, u16 worldY, ePos pos);
        u32     priv_calc (u16 worldX, u16 worldY, ePos pos, gos:: vec3f *out_v) const;
        
    private:
        gos::Allocator *localAllocator;
        u32     worldDimX;
        u32     worldDimY;
        f32     *coordX;
        f32     *coordZ;
        u16     *existingVtx;
        

    };


private:
    u8      priv_computeSquareMask (const Map &map, u32 x, u32 y) const;
    void    priv_renderLine (SimpleLineRenderer &line, gos::FastArray<sEdge> &edgeList) const;
    void    priv_moveEdge (gos::FastArray<sEdge> *from, gos::FastArray<sEdge> *to, u32 i) const;
    u32     priv_findEdgeWithVtx (const gos::FastArray<sEdge> *list, const sEdge *edge) const;
    void    priv_coloredLine (SimpleLineRenderer &line, const VtxHelper2 &helper, gos::FastArray<sEdge> &edgeList) const;
    void    priv_smoothLine (SimpleLineRenderer &line, const VtxHelper2 &helper, gos::FastArray<sEdge> &edgeList) const;
};

#endif //_marchingSquare_h_