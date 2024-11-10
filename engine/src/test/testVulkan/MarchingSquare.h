#ifndef _marchingSquare_h_
#define _marchingSquare_h_
#include "../gos/gos.h"
#include "../gos/gosFastArray.h"
#include "../gosMath/gosVect.h"
#include "SimpleLineRenderer.h"
#include "VkEx5MarchingSquare.h"

/**
 * @brief Marching Square
 * Data una <map> di MX x MY punti, allora:
 *  - esistono (MX-1) * (MY-1) quad
 *  - ogni quad ha associato un <tileType> che dipende dalla risoluzione del marchingSquare
 *  - ogni quad è indicizzato da una coppia di coordinate <x,y> con x>=0 && x<MX
 *  - ogni quad ha 4 vertici (a,b,c,d) che son utili al marching square
 *      
 *     |----a----|    posso riferirmi ad un vertice con la tupla <x,y,a|b|c|d>
 *     |         |    Attenzione che <0,0,b> è lo stesso di <1,0,d>
 *     d         b
 *     |         |
 *     |----c----|
 */
class MarchingSquare
{
public:
    struct sVertex2
    {
        gos::vec2f pos;
        gos::vec2f norm;
    };

    struct sVertex3
    {
        gos::vec3f pos;
        gos::vec3f norm;
    };

    struct sInfo
    {
        u32     numQuad;
        u32     numVtx;
        u8      *quadTypeList;
        u16     *xPos;
        u16     *yPos;
        sVertex2 *vtxList;
    };

    typedef gos::FastArray<sVertex3> VertexList3;

public:
                    MarchingSquare()                                                            { numInfo = 0; numQuadPieni=0; localAllocator=NULL; }
                    ~MarchingSquare()                                                           { priv_free(); }

    void            run (gos::Allocator *allocator, const VkEx5MarchingSquare::Map &map);

    /**
     * @brief posto che run() sia stata eseguita, questa fn ritorna in <out_vtxList> e <out_idxList> una mesh che renderizza
     * tutti i perimetri.
     * I primi 4 vtx sono deidicati alla mesh del "quad pieno"
     */
    void            buildMesh (VertexList3 &out_vtxList, gos::FastArray<u16> &out_idxList) const;

    u32             getNumQuadPieni() const                                                     { return numQuadPieni; }
    gos::vec2u16    getPosQuadPieno(u32 i) const                                                { assert(i<numQuadPieni); return posQuadPieni[i]; }


    u32             getNumPerimetri() const                                                     { return numInfo; }
    const sInfo*    getPerimetroByIndex (u32 i) const                                           { assert(i < getNumPerimetri()); return &infoList[i]; }


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

    struct sQuad
    {
        u8      quadType;           //da 0 a 15, rappresenta 1 dei 16 casi possibili dell'output del Marching cube
        u8      alreadyUsed;
        u16     x;
        u16     y;
    };

    struct sPerimetro
    {
        u32     numQuad;
        bool    bSiChiudeSuSeStesso;
        u32     *quadIndexList;
        u8      *quadTypeList;
    };

    struct sLinea
    {
        u32 quadIndex;
        u16 x;
        u16 y;
        u8  quadType;
    };

    typedef gos::FastArray<sLinea> LineaList;

    template<class T>
    class Matrix
    {
    public:
        Matrix()                                    { buffer = NULL; }
        ~Matrix()                                   { unsetup(); }

        void setup (u16 dimxIN, u16 dimyIN)
        {
            dimx = dimxIN;
            dimy = dimyIN;
            maxCT = (u32)dimx * (u32)dimy;
            buffer = GOSALLOCT(T*, gos::getScrapAllocator(), sizeof(T) * maxCT);
        }

        void unsetup()
        {
            if (NULL != buffer)
            {
                GOSFREE(gos::getScrapAllocator(), buffer);
                buffer = NULL;
            }
        }

        const u16 getDimX() const                   { return dimx; }
        const u16 getDimY() const                   { return dimy; }
        const u32 getMaxCT() const                  { return maxCT; }
        T*  get (u16 x, u16 y)
        {
            if (x >= dimx) return NULL;
            if (y >= dimy) return NULL;
            return &buffer[x +y*dimx];
        }

    public:
        T       *buffer;

    private:
        u16     dimx;
        u16     dimy;
        u32     maxCT;
    };

private:
    void        priv_free();
    u8          priv_computeSquareMask (const VkEx5MarchingSquare::Map &map, u32 x, u32 y) const;

    void        priv_perimetro_begin (LineaList &linea, u32 ct, gos::FastArray<sPerimetro> &listaPerimetri);
    bool        priv_perimetro_createTemp (LineaList &linea, u32 ct, const sQuad *parent);
    void        priv_perimetro_store (gos::FastArray<sPerimetro> &listaPerimetri, const LineaList &linea, bool bSiChiudeSuSeStesso) const;
    void        priv_perimetro_makeVtxList (const sPerimetro *p, gos::FastArray<gos::vec2f> &tempVtxList) const;
    void        priv_perimetro_smooth (const gos::FastArray<gos::vec2f> &srcVtxList, gos::FastArray<sVertex2> &dstVtxList, bool bLineaChiusa, gos::FastArray<gos::vec2f> &tmpSegmentNormList) const;
    void        priv_calcVtxCoord (const sQuad *quad, ePos pos, gos::vec2f *out) const;
    gos::vec2f  priv_perimetro_getVtx (const gos::FastArray<gos::vec2f> &vtxList, bool bLineaChiusa, i32 index) const;
    bool        priv_isValidStart (const sQuad *quad) const;

    void        priv_mesh_buildSingleQuad (const sInfo *info, const sQuad &quad, u32 firstVtxSRC, VertexList3 &out_vtxList, gos::FastArray<u16> &out_idxList) const;
    void        priv_mesh_calcNumVtxIdx (u32 iPerimetro, u32 *out_nVtx, u32 *out_nIdx) const;
    void        priv_mesh_addTris (gos::FastArray<u16> &out_idxList, u32 firstVtx, u32 idx0, u32 idx1, u32 idx2, const VertexList3 &out_vtxList) const;
    void        priv_mesh_addSpessore (u32 firstVtx, VertexList3 &out_vtxList, gos::FastArray<u16> &out_idxList) const;

private:
    gos::Allocator  *localAllocator;
    Matrix<sQuad>   quadList;
    u8              numInfo;
    sInfo           *infoList;
    
    u32             numQuadPieni;
    gos::vec2u16    *posQuadPieni;
};

#endif //_marchingSquare_h_