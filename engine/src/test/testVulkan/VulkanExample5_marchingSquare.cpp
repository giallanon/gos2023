#include "VulkanExample5.h"
#include "../gos/gos.h"

using namespace gos;


//*******************************************************
VulkanExample5::MarchingSquare::VtxHelper2::VtxHelper2 (gos::Allocator *allocatorIN, const World &world)
{
    localAllocator = allocatorIN;
    worldDimX = world.getDimX();
    worldDimY = world.getDimY();

    vtxList.setup (localAllocator, worldDimX * worldDimY);
    edgeList.setup (localAllocator, worldDimX * worldDimY);

    //ci sono (dimx*2+1) posizioni X e (dimy*2+1) posizioni Z
    //calcolo le coordinate x,z
    coordX = GOSALLOCT(f32*, localAllocator, sizeof(f32) * (worldDimX*2+1));
    coordZ = GOSALLOCT(f32*, localAllocator, sizeof(f32) * (worldDimY*2+1));
    {
        const f32 HALF_SPACE = World::SPACE * 0.5f;
        f32 c = -((worldDimX/2) * World::SPACE);
        for (u32 i=0; i<=worldDimX*2; i++)
        {
            coordX[i] = c;
            c += HALF_SPACE;
        }

        c = (worldDimY/2) * World::SPACE;
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
VulkanExample5::MarchingSquare::VtxHelper2::~VtxHelper2 ()
{
    vtxList.unsetup();
    edgeList.unsetup();
    GOSFREE(localAllocator, coordX);
    GOSFREE(localAllocator, coordZ);
    GOSFREE(localAllocator, existingVtx);
}

//*******************************************************
u32 VulkanExample5::MarchingSquare::VtxHelper2::priv_calc (u16 wx, u16 wy, ePos pos, gos:: vec3f *out_v) const
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
u16 VulkanExample5::MarchingSquare::VtxHelper2::priv_vtxAddIfNeeded (u16 wx, u16 wy, MarchingSquare::ePos pos)
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
VulkanExample5::MarchingSquare::VtxHelper2::sEdge VulkanExample5::MarchingSquare::VtxHelper2::addEdge (u16 wx, u16 wy, ePos pos1, ePos pos2)
{
    sEdge edge;
    edge.i0 = priv_vtxAddIfNeeded (wx, wy, pos1);
    edge.i1 = priv_vtxAddIfNeeded (wx, wy, pos2);
    edgeList.append(edge);
    return edge;
}



//*******************************************************
u8 VulkanExample5::MarchingSquare::computeSquareMask (const World &world, u32 x, u32 y)
{
    u8 mask = 0;
    if (world.isON(x,y))
        mask |= 0x08;
    if (world.isON(x+1,y))
        mask |= 0x04;
    if (world.isON(x+1,y+1))
        mask |= 0x02;
    if (world.isON(x,y+1))
        mask |= 0x01;

    return mask;
}

/*******************************************************
 * Versione super semplice
 * Disegna solo il perimetro, non shara i vertici, non fa niente di speciale
 */
void VulkanExample5::MarchingSquare::algo1 (const World &world, Line &line)
{
    //marching square
    const f32 HALF_SPACE = World::SPACE * 0.5f;
    const u32 dimx = world.getDimX();
    const u32 dimy = world.getDimY();
    const f32 startX = -((dimx/2) * World::SPACE) + HALF_SPACE;
    f32 zz = (dimy/2) * World::SPACE - HALF_SPACE;


    for (u32 y=0; y<dimy-1; y++)
    {
        f32 xx = startX;
        for (u32 x=0; x<dimx-1; x++)
        {
            /*
                       a
                0x08---------0x04
                   |         |
                 d |         | b
                   |         |
                0x01---------0x02
                        c
            */
            const u8 mask = computeSquareMask (world, x, y);

            const vec3f a(xx, 0, zz + HALF_SPACE); 
            const vec3f b(xx + HALF_SPACE, 0, zz);
            const vec3f c(xx, 0, zz - HALF_SPACE);
            const vec3f d(xx - HALF_SPACE, 0, zz);

            switch (mask)
            {
            case 0:     break;
            case 1:     line.addLine (c, d); break;
            case 2:     line.addLine (b, c); break;
            case 3:     line.addLine (b, d); break;
            case 4:     line.addLine (a, b); break;
            case 5:     line.addLine (a, d); line.addLine (b, c); break;
            case 6:     line.addLine (a, c); break;
            case 7:     line.addLine (a, d); break;
            case 8:     line.addLine (a, d); break;
            case 9:     line.addLine (a, c); break;
            case 10:    line.addLine (a, b); line.addLine (c, d); break;
            case 11:    line.addLine (a, b); break;
            case 12:    line.addLine (b, d); break;
            case 13:    line.addLine (b, c); break;
            case 14:    line.addLine (c, d); break;
            case 15:    break;

            default:
                //errore, qui non ci dobbiamo mai arrivare
                DBGBREAK;
                break;
            }

            xx += World::SPACE;
        }
        zz -= World::SPACE;
    }

}


/*******************************************************
 * Disegna solo il perimetro, share dei vtx
 */
void VulkanExample5::MarchingSquare::algo2 (const World &world, Line &line)
{
    VtxHelper2 helper (gos::getScrapAllocator(), world);

    //marching square
#define ADD(vtxPlacement1, vtxPlacement2)\
                edge = helper.addEdge (x,y, vtxPlacement1, vtxPlacement2);\


    VtxHelper2::sEdge edge;
    for (u32 y=0; y<world.getDimY()-1; y++)
    {
        for (u32 x=0; x<world.getDimX()-1; x++)
        {
            const u8 mask = computeSquareMask (world, x, y);
            switch (mask)
            {
            case 0:     
                break;

            case 1: //c,d
                ADD(ePos::c, ePos::d);
                break;

            case 2:
                //line.addLine (b, c); break;
                ADD(ePos::b, ePos::c);
                break;

            case 3:     
                //line.addLine (b, d); break;
                ADD(ePos::b, ePos::d);
                break;

            case 4:     
                //line.addLine (a, b); break;
                ADD(ePos::a, ePos::b);
                break;

            case 5:     
                //line.addLine (a, d);
                //line.addLine (b, c); break;
                //ADD(ePos::a, ePos::d);
                //ADD(ePos::b, ePos::c);
                ADD(ePos::a, ePos::b);
                ADD(ePos::c, ePos::d);
                break;

            case 6:     
                //line.addLine (a, c); break;
                ADD(ePos::a, ePos::c);
                break;

            case 7:     
                //line.addLine (a, d); break;
                ADD(ePos::a, ePos::d);
                break;

            case 8:
                //line.addLine (a, d); break;
                ADD(ePos::a, ePos::d);
                break;

            case 9:
                //line.addLine (a, c); break;
                ADD(ePos::a, ePos::c);
                break;

            case 10:    
                //line.addLine (a, b); 
                //line.addLine (c, d); break;
                //ADD(ePos::a, ePos::b);
                //ADD(ePos::c, ePos::d);
                ADD(ePos::b, ePos::c);
                ADD(ePos::d, ePos::a);

                break;

            case 11:    
                //line.addLine (a, b); break;
                ADD(ePos::a, ePos::b);
                break;

            case 12:    
                //line.addLine (b, d); break;
                ADD(ePos::b, ePos::d);
                break;

            case 13:    
                //line.addLine (b, c); break;
                ADD(ePos::b, ePos::c);
                break;

            case 14:    
                //line.addLine (c, d); break;
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

    //aggiungo un po' di noise random ai vertici
    for (u32 i=0; i<helper.vtxList.getNElem(); i++)
    {
        f32 x = 0;//(gos::random01() - 0.5f) * 0.1f;
        f32 z = 0;//(gos::random01() - 0.5f) * 0.1f;
        helper.vtxList[i].x += x;
        helper.vtxList[i].z += z;

        line.addVtx (helper.vtxList(i));
    }

    // aggiungo le linee
    for (u32 i=0; i<helper.edgeList.getNElem(); i++)
        line.line (helper.edgeList(i).i0, helper.edgeList(i).i1);

}


/*******************************************************
 * Disegna solo il perimetro, NO share dei vtx.
 * Il "peso" dei singoli dot viene preso in considerazione nel calcolo dei vtx
 */
void VulkanExample5::MarchingSquare::algo3 (const World &world, Line &line)
{
#define CALC(pa,pb,t)     pa + (pb-pa) * t


    for (u32 y=0; y<world.getDimY()-1; y++)
    {
        for (u32 x=0; x<world.getDimX()-1; x++)
        {
            gos::vec3f a,b,c,d;
            const gos::vec3f p1 = world.getPos3D(x,y);
            const f32 p1x = world.getScaleX(x,y);
            const f32 p1z = world.getScaleZ(x,y);

            const gos::vec3f p2 = world.getPos3D(x+1,y);
            const f32 p2x = world.getScaleX(x+1,y);
            const f32 p2z = world.getScaleZ(x+1,y);

            const gos::vec3f p3 = world.getPos3D(x+1,y+1);
            const f32 p3x = world.getScaleX(x+1,y+1);
            const f32 p3z = world.getScaleZ(x+1,y+1);

            const gos::vec3f p4 = world.getPos3D(x,y+1);
            const f32 p4x = world.getScaleX(x,y+1);
            const f32 p4z = world.getScaleZ(x,y+1);

            const u8 mask = computeSquareMask (world, x, y);
            switch (mask)
            {
            case 0:     break;
            case 15:    break;
            case 1:
                c = CALC(p4, p3, p4x);
                d = CALC(p4, p1, p4z);
                line.addLine (c, d);
                break;

            case 2:     
                b = CALC(p3, p2, p3z);
                c = CALC(p3, p4, p3x);
                line.addLine (b, c); 
                break;

            case 3:     
                b = CALC(p3, p2, p3z);
                d = CALC(p4, p1, p4z);
                line.addLine (b, d);
                break;

            case 4:     
                a = CALC(p2, p1, p2x);
                b = CALC(p2, p3, p2z);
                line.addLine (a, b); 
                break;

            case 5:     
                a = CALC(p2, p1, p2x);
                d = CALC(p4, p1, p4z);
                line.addLine (a, d); 

                b = CALC(p2, p3, p2z);
                c = CALC(p4, p3, p4x);
                line.addLine (b, c); 
                break;
            
            case 6:     
                a = CALC(p2, p1, p2x);
                c = CALC(p3, p4, p3x);
                line.addLine (a, c); 
                break;

            case 7:     
                a = CALC(p2, p1, p2x);
                d = CALC(p4, p1, p4z);
                line.addLine (a, d); 
                break;
            
            case 8:     
                a = CALC(p1, p2, p1x);
                d = CALC(p1, p4, p1z);
                line.addLine (a, d); 
                break;

            case 9:     
                a = CALC(p1, p2, p1x);
                c = CALC(p4, p3, p4x);
                line.addLine (a, c); 
                break;

            case 10:    
                a = CALC(p1, p2, p1x);
                b = CALC(p3, p2, p3z);
                line.addLine (a, b); 

                c = CALC(p3, p4, p3x);
                d = CALC(p1, p4, p1z);
                line.addLine (c, d); 
                break;

            case 11:    
                a = CALC(p1, p2, p1x);
                b = CALC(p3, p2, p3z);
                line.addLine (a, b); 
                break;

            case 12:    
                b = CALC(p1, p4, p1z);
                d = CALC(p2, p3, p2z);
                line.addLine (b, d); 
                break;

            case 13:    
                b = CALC(p2, p3, p2z);
                c = CALC(p4, p3, p4x);
                line.addLine (b, c); 
                break;

            case 14:    
                c = CALC(p1, p4, p1z);
                d = CALC(p3, p4, p3x);
                line.addLine (c, d); 
                break;
            }
        }
    }

}

