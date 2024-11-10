#include "VkEx5MarchingSquare.h"


using namespace gos;


//*******************************************************
VkEx5MarchingSquare::VtxHelper2::VtxHelper2 (gos::Allocator *allocatorIN, const Map &map)
{
    localAllocator = allocatorIN;
    worldDimX = map.getDimX();
    worldDimY = map.getDimY();

    vtxList.setup (localAllocator, worldDimX * worldDimY);
    edgeList.setup (localAllocator, worldDimX * worldDimY);

    //ci sono (dimx*2+1) posizioni X e (dimy*2+1) posizioni Z
    //calcolo le coordinate x,z
    coordX = GOSALLOCT(f32*, localAllocator, sizeof(f32) * (worldDimX*2+1));
    coordZ = GOSALLOCT(f32*, localAllocator, sizeof(f32) * (worldDimY*2+1));
    {
        static constexpr f32 MAP_SIZE_OF_A_QUAD = 1.0f;

        const f32 HALF_SPACE = MAP_SIZE_OF_A_QUAD * 0.5f;
        f32 c = -((worldDimX/2) * MAP_SIZE_OF_A_QUAD);
        for (u32 i=0; i<=worldDimX*2; i++)
        {
            coordX[i] = c;
            c += HALF_SPACE;
        }

        c = (worldDimY/2) * MAP_SIZE_OF_A_QUAD;
        for (u32 i=0; i<=worldDimY*2; i++)
        {
            coordZ[i] = c;
            c -= HALF_SPACE;
        }        
    }

    //mi serve una mappa per tenere traccia dei vtx gia' creati
    //Data un punto (x,y) in [map], voglio potermi riferire a (x), (x-1, y) e (x+1,y) per i vtx a sx e dx del punto
    //e, allo stesso modo, voglio usare (y), (y-1) e (y+1)
    existingVtx = GOSALLOCT(u16*, gos::getScrapAllocator(), (worldDimX*2+1) * (worldDimY*2+1) * sizeof(u16));
    memset (existingVtx, 0xff, (worldDimX*2+1) * (worldDimY*2+1) * sizeof(u16));
}

//*******************************************************
VkEx5MarchingSquare::VtxHelper2::~VtxHelper2 ()
{
    vtxList.unsetup();
    edgeList.unsetup();
    GOSFREE(localAllocator, coordX);
    GOSFREE(localAllocator, coordZ);
    GOSFREE(localAllocator, existingVtx);
}

//*******************************************************
u32 VkEx5MarchingSquare::VtxHelper2::priv_calc (u16 wx, u16 wy, ePos pos, gos:: vec3f *out_v) const
{
    assert (wx < worldDimX);
    assert (wy < worldDimY);

    u32 ctx;
    u32 cty;
    switch (pos)
    {
    default:
        DBGBREAK;
        break;

    case ePos::a:
        ctx = (wx*2) + 1;
        cty = (wy*2);
        break;

    case ePos::b:
        ctx = (wx*2) + 2;
        cty = (wy*2) + 1;
        break;

    case ePos::c:
        ctx = (wx*2) + 1;
        cty = (wy*2) + 2;
        break;

    case ePos::d:
        ctx = (wx*2);
        cty = (wy*2) + 1;
        break;
    }

    assert (ctx < (u32)worldDimX*2 +1);
    assert (cty < (u32)worldDimY*2 +1);
    out_v->set (coordX[ctx], 0, coordZ[cty]);
    return ((worldDimX*2+1)*cty + ctx);
}

//*******************************************************
u16 VkEx5MarchingSquare::VtxHelper2::priv_vtxAddIfNeeded (u16 wx, u16 wy, VkEx5MarchingSquare::ePos pos)
{
    gos::vec3f v;
    const u32 offset = priv_calc (wx, wy, pos, &v);
   
    u16 ret = existingVtx[offset];
    if (0xFFFF == ret)
    {
        ret = vtxList.getNElem();
        vtxList.append (v);
        existingVtx[offset] = ret;
    }
    return ret;
}

//*******************************************************
VkEx5MarchingSquare::sEdge VkEx5MarchingSquare::VtxHelper2::addEdge (u16 wx, u16 wy, ePos pos1, ePos pos2)
{
    sEdge edge;
    edge.i0 = priv_vtxAddIfNeeded (wx, wy, pos1);
    edge.i1 = priv_vtxAddIfNeeded (wx, wy, pos2);
    edgeList.append(edge);
    return edge;
}





//*******************************************************
u8 VkEx5MarchingSquare::priv_computeSquareMask (const Map &map, u32 x, u32 y) const
{
    u8 mask = 0;
    if (map.isON(x,y))
        mask |= 0x08;
    if (map.isON(x+1,y))
        mask |= 0x04;
    if (map.isON(x+1,y+1))
        mask |= 0x02;
    if (map.isON(x,y+1))
        mask |= 0x01;

    return mask;
}

//*******************************************************
void VkEx5MarchingSquare::priv_moveEdge (gos::FastArray<sEdge> *from, gos::FastArray<sEdge> *to, u32 i) const
{
    assert (i <from->getNElem());
    to->append (from->queryElem(i));
    from->removeAndSwapWithLast(i);
}

//*******************************************************
u32 VkEx5MarchingSquare::priv_findEdgeWithVtx (const gos::FastArray<sEdge> *list, const sEdge *edge) const
{
    for (u32 i=0; i<list->getNElem(); i++)
    {
        if (list->queryElem(i).i0 == edge->i1)
            return i;
    }

    return u32MAX;
}


/*******************************************************
 * Disegna solo il perimetro, share dei vtx
 */
void VkEx5MarchingSquare::algo2 (const Map &map, SimpleLineRenderer &line)
{
    VtxHelper2 helper (gos::getScrapAllocator(), map);

    //marching square
#define ADD(vtxPlacement1, vtxPlacement2)\
                edge = helper.addEdge (x,y, vtxPlacement1, vtxPlacement2);\


    sEdge edge;
    for (u32 y=0; y<map.getDimY()-1; y++)
    {
        for (u32 x=0; x<map.getDimX()-1; x++)
        {
            const u8 mask = priv_computeSquareMask (map, x, y);
            switch (mask)
            {
            case 0:     
                break;

            case 1: //c,d
                ADD(ePos::d, ePos::c);
                break;

            case 2:
                ADD(ePos::c, ePos::b);
                break;

            case 3:     
                ADD(ePos::d, ePos::b);
                break;

            case 4:     
                ADD(ePos::b, ePos::a);
                break;

            case 5:     
                //ADD(ePos::d, ePos::a);
                //ADD(ePos::b, ePos::c);
                ADD(ePos::b, ePos::a);
                ADD(ePos::d, ePos::c);
                break;

            case 6:     
                ADD(ePos::c, ePos::a);
                break;

            case 7:     
                ADD(ePos::d, ePos::a);
                break;

            case 8:
                ADD(ePos::a, ePos::d);
                break;

            case 9:
                ADD(ePos::a, ePos::c);
                break;

            case 10:    
                //ADD(ePos::a, ePos::b);
                //ADD(ePos::c, ePos::d);
                ADD(ePos::a, ePos::d);
                ADD(ePos::c, ePos::b);
                break;

            case 11:    
                ADD(ePos::a, ePos::b);
                break;

            case 12:    
                ADD(ePos::b, ePos::d);
                break;

            case 13:    
                ADD(ePos::b, ePos::c);
                break;

            case 14:    
                ADD(ePos::c, ePos::d);
                break;

            case 15:    break;

            default:
                //errore, qui non ci dobbiamo mai arrivare
                DBGBREAK;
                break;
            }
        }
    }

    //cerco di separare le linee
    FastArray<sEdge> aLine (gos::getSysHeapAllocator(), helper.edgeList.getNElem());

    FastArray<sEdge> *listSRC = &helper.edgeList;
    FastArray<sEdge> *listDST = &aLine;
    while (listSRC->getNElem())
    {
        //prendo la prima edge di listSRC e la sposto in listDST
        listDST->reset();
        priv_moveEdge (listSRC, listDST, 0);

        while (1)
        {
            //cerco un edge che inizi dove finisce la precedente edge che ho aggiunto a listDST
            const sEdge *edge = &listDST->queryElem( listDST->getNElem() - 1);
        
            u32 nextEdge = priv_findEdgeWithVtx (listSRC, edge);
            if (u32MAX == nextEdge)
                break;

            //l'ho trovato, la tolgo da listSRC e la metto in listDST e proseguo
            priv_moveEdge (listSRC, listDST, nextEdge);
        }

        //disegno lineDST
        priv_smoothLine (line, helper, *listDST);
    }
}

//*******************************************************
void VkEx5MarchingSquare::priv_renderLine (SimpleLineRenderer &line, gos::FastArray<sEdge> &edgeList) const
{
    for (u32 i=0; i<edgeList.getNElem(); i++)
        line.line (edgeList(i).i0, edgeList(i).i1);
}

//*******************************************************
void VkEx5MarchingSquare::priv_coloredLine (SimpleLineRenderer &line, const VtxHelper2 &helper, gos::FastArray<sEdge> &edgeList) const
{
    u32 v0 = edgeList(0).i0;
    for (u32 i=0; i<edgeList.getNElem(); i++)
    {
        const u32 v1 = edgeList(i).i1;        
        line.addLine (helper.vtxList(v0), helper.vtxList(v1));
        v0 = v1;
    }
}


vec3f VulkanExample5_MarchingSquare_getVtx (i32 index, const vec3f *list, u32 nElemInList)
{
    while (index < 0)
        index += nElemInList;
    while (index >= (i32)nElemInList)
        index -= nElemInList;

    assert (index >= 0 && index < (i32)nElemInList);
    return list[index];
}

//*******************************************************
void VkEx5MarchingSquare::priv_smoothLine (SimpleLineRenderer &line, const VtxHelper2 &helper, gos::FastArray<sEdge> &edgeList) const
{
    if (edgeList.getNElem() < 4)
    {
        priv_coloredLine (line, helper, edgeList);
        return;
    }


   static constexpr u8 NCOLOR = 2;
    vec3f colors[NCOLOR] = {
        vec3f(1,1,1),
        vec3f(1,0,0)
    };
    u8 curColor = 0;



    //lista dei vtx della linea
    const u32 N_MAX_POINTS = edgeList.getNElem()+4;
    vec3f *pointIN = GOSALLOCT(vec3f*, gos::getScrapAllocator(), sizeof(vec3f) * N_MAX_POINTS);
    u32 nPoint = 0;
    for (u32 i=0; i<edgeList.getNElem(); i++)
        pointIN[nPoint++] = helper.vtxList( edgeList(i).i0 );
//    pointIN[nPoint++] = helper.vtxList( edgeList(0).i0 );



/*
    //disegno la linea iniziale
    for (u32 i=0; i<nPoint-1; i++) 
    {
        line.setColor (colors[curColor++]);     if (curColor >= NCOLOR) curColor = 0;
        const u16 i0 = line.addVtx (pointIN[i]);

        line.setColor (colors[curColor++]);     if (curColor >= NCOLOR) curColor = 0;
        const u16 i1 = line.addVtx (pointIN[i+1]);
        line.line (i0, i1);
    }
*/

        
    //https://www.codeproject.com/Articles/1093960/2D-Polyline-Vertex-Smoothing
    vec3f *pointOUT = GOSALLOCT(vec3f*, gos::getScrapAllocator(), sizeof(vec3f) * (nPoint * (SMOOTH_LEVEL+1)));

    u32 nPointOUT = 0;
    for (u32 i=0; i<nPoint; i++)
    {
        const vec3f p0 = VulkanExample5_MarchingSquare_getVtx(i-1, pointIN, nPoint);
        const vec3f p1 = VulkanExample5_MarchingSquare_getVtx(i,   pointIN, nPoint);
        const vec3f p2 = VulkanExample5_MarchingSquare_getVtx(i+1, pointIN, nPoint);
        const vec3f p3 = VulkanExample5_MarchingSquare_getVtx(i+2, pointIN, nPoint);

        for (u32 i2=0; i2<SMOOTH_LEVEL; i2++)
        {
            const f32 t = ((f32)i2 / (f32)SMOOTH_LEVEL);
            const f32 t2 = t*t;
            const f32 t3 = t2*t;
                
            vec3f p = 0.5f * (
                                2.0f * p1 
                                + (-1.0f * p0 + p2) * t
                                + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 
                                + (-1.0f * p0 + 3.0f * p1 -3.0f * p2 + p3) * t3
                            );


                
            pointOUT[nPointOUT++] = p;
        }
    }

    printf ("NUM POINT in:%d, out: %d, NUM edge: %d\n\n", nPoint, nPointOUT, edgeList.getNElem());


    //disegno
    pointOUT[nPointOUT++] = pointOUT[0];
    for (u32 i=0; i<nPointOUT-1; i++) 
    {
        line.setColor (colors[curColor++]);     if (curColor >= NCOLOR) curColor = 0;
        const u16 i0 = line.addVtx (pointOUT[i]);

        line.setColor (colors[curColor++]);     if (curColor >= NCOLOR) curColor = 0;
        const u16 i1 = line.addVtx (pointOUT[i+1]);
        line.line (i0, i1);


        //normale del segmento
        //if we define dx=x2-x1 and dy=y2-y1, then the normals are (-dy, dx) and (dy, -dx).
        const vec3f p2 = pointOUT[i+1];
        const vec3f p1 = pointOUT[i];
        const f32 dx = p2.x - p1.x;
        const f32 dz = p2.z - p1.z;
        vec3f norm = vec3f (-dz, 0, dx);
        norm.normalize();
        
        const vec3f mid = p1 + (p2 - p1) * 0.5f;
        line.setColor (vec3f(0,1,0));
        line.addLine (mid, mid + 0.2f*norm);
    }

    
    
    GOSFREE(gos::getScrapAllocator(), pointIN);
    GOSFREE(gos::getScrapAllocator(), pointOUT);
}

