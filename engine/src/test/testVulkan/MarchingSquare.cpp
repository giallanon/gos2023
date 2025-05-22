#include "MarchingSquare.h"


using namespace gos;


//*******************************************************
void MarchingSquare::priv_free()
{
    if (0 != numInfo)
    {
        for (u32 i=0;i<numInfo; i++)
        {
            GOSFREE(localAllocator, infoList[i].quadTypeList);
            GOSFREE(localAllocator, infoList[i].vtxList);
            GOSFREE(localAllocator, infoList[i].xPos);
            GOSFREE(localAllocator, infoList[i].yPos);
        }
        numInfo = 0;
    }

    if (0 != numQuadPieni)
    {
        GOSFREE(localAllocator, posQuadPieni);
        numQuadPieni = 0;
    }

    if (NULL != localAllocator)
    {
        GOSFREE(localAllocator, infoList);
        localAllocator = NULL;
    }

}

//*******************************************************
u8 MarchingSquare::priv_computeSquareMask (const VkEx5MarchingSquare::Map &map, u32 x, u32 y) const
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
bool MarchingSquare::priv_isValidStart (const sQuad *quad) const
{
    //i quad vuoti non si connectano a nulla
    if (0 == quad->quadType)
        return false;

    //i quad pieni non si connectano a nulla nel senso che non formano mai un perimetro
    if (15 == quad->quadType)
        return false;

    //questi sono quad speciali con le doppie linee
    if (5 == quad->quadType || 10 == quad->quadType)
        return false;
    
    //il quad in questione è già stato connesso quindi lo scarto
    if (quad->alreadyUsed)
        return false;

    if (0 == quad->y)
    {
        if (4 == quad->quadType || 12 == quad->quadType || 13 == quad->quadType || 14 == quad->quadType)
            return false;
    }

    if (quadList.getDimX()-1 == quad->x)
    {
        if (2 == quad->quadType || 6 == quad->quadType)
            return false;
    }
    return true;
}

//*******************************************************
void MarchingSquare::run (gos::Allocator  *allocator, const VkEx5MarchingSquare::Map &map)
{
    priv_free();

    numQuadPieni = 0;
    u32 numQuadVuoti = 0;
    u32 numQuadDiConfine = 0;
    u32 stimaNumeroDiPerimetri = 4;
    u32 numMaxQuadPerPerimetro = 0;


    localAllocator = allocator;
    quadList.setup (static_cast<u16>(map.getDimX()-1), static_cast<u16>(map.getDimY()-1));

    //scanno tutti i quad e ne determino il tipo
    u32 ct = 0;
    for (u16 y=0; y<quadList.getDimY(); y++)
    {
        for (u16 x=0; x<quadList.getDimX(); x++)
        {
            quadList.buffer[ct].x = x;
            quadList.buffer[ct].y = y;
            quadList.buffer[ct].alreadyUsed = 0;
            quadList.buffer[ct].quadType = priv_computeSquareMask (map, x, y);
            switch (quadList.buffer[ct].quadType)
            {
            default:    numQuadDiConfine++; break;
            case 0:     numQuadVuoti++; break;
            case 5:     numQuadDiConfine+=2; stimaNumeroDiPerimetri++; break; //questo caso ha una doppia linea quindi comparira' 2 volte nei perimetri
            case 10:    numQuadDiConfine+=2; stimaNumeroDiPerimetri++; break; //questo caso ha una doppia linea quindi comparira' 2 volte nei perimetri
            case 15:    numQuadPieni++; break;
            }
            ct++;
        }
    }

    //preparo un array per le posizioni dei quad pieni
    if (numQuadPieni)
        posQuadPieni = GOSALLOCT(gos::vec2u16*, localAllocator, sizeof(gos::vec2u16) * numQuadPieni);

    //preparo un array che contenga tutti i perimetri che trovo
    FastArray<sPerimetro> listaPerimetri (gos::getScrapAllocator(), stimaNumeroDiPerimetri);


    //ricerca dei perimetri
    {
        //so che un perimetro non potra' mai essere piu' lungo di <numQuadDiConfine> quad
        LineaList  linea (gos::getScrapAllocator(), numQuadDiConfine);


        //cerco di connettere i quad, ovvero di determinare quali quad fanno parte di un certo perimetro
        //Qui cerco il primo quad non ancora connesso e parto da quello per tracciare l'intera linea del perimetro
        ct = 0;
        for (u16 y=0; y<quadList.getDimY(); y++)
        {
            for (u16 x=0; x<quadList.getDimX(); x++, ct++)
            {
                if (15 == quadList.buffer[ct].quadType)
                    posQuadPieni[numQuadPieni++].set (x, y);

                if (!priv_isValidStart(&quadList.buffer[ct]))
                    continue;

                priv_perimetro_begin (linea, ct, listaPerimetri);
                if (linea.getNElem() > numMaxQuadPerPerimetro)
                    numMaxQuadPerPerimetro = linea.getNElem();
            }
        }

        linea.unsetup();
    } //fine ricerca dei perimetri


    //A questo punto in <listaPerimetri> ho un elenco di perimetri, ciascuno formato da un elenco
    //di indici ad un quad di <quadList>
    //Per ogni perimetro, creo una lista di vertici a formare una linea
    {
        numInfo = listaPerimetri.getNElem();
        infoList = GOSALLOCT(sInfo*, localAllocator, sizeof(sInfo) * numInfo);

        FastArray<gos::vec2f>   tempVtxList (gos::getScrapAllocator(), numMaxQuadPerPerimetro);
        FastArray<gos::vec2f>   tempSegmentNormList (gos::getScrapAllocator(), numMaxQuadPerPerimetro * SMOOTH_LEVEL);
        FastArray<sVertex2>     tempVtxList2 (gos::getScrapAllocator(), numMaxQuadPerPerimetro * SMOOTH_LEVEL);
        for (u32 i=0; i<numInfo; i++)
        {
            infoList[i].numQuad = listaPerimetri(i).numQuad;
            infoList[i].quadTypeList = GOSALLOCT(u8*, localAllocator, sizeof(u8) * infoList[i].numQuad);
            infoList[i].xPos = GOSALLOCT(u16*, localAllocator, sizeof(u16) * infoList[i].numQuad);
            infoList[i].yPos = GOSALLOCT(u16*, localAllocator, sizeof(u16) * infoList[i].numQuad);
            for (u32 i2=0; i2<infoList[i].numQuad; i2++)
            {
                infoList[i].quadTypeList[i2] = listaPerimetri(i).quadTypeList[i2];
                infoList[i].xPos[i2] = quadList.buffer[listaPerimetri(i).quadIndexList[i2]].x;
                infoList[i].yPos[i2] = quadList.buffer[listaPerimetri(i).quadIndexList[i2]].y;
            }
            priv_perimetro_makeVtxList (&listaPerimetri(i), tempVtxList);
            priv_perimetro_smooth (tempVtxList, tempVtxList2, listaPerimetri(i).bSiChiudeSuSeStesso, tempSegmentNormList);

            const u32 nVtx = infoList[i].numVtx = tempVtxList2.getNElem();
            infoList[i].vtxList = GOSALLOCT(sVertex2*, localAllocator, sizeof(sVertex2) * nVtx);
            memcpy (infoList[i].vtxList, tempVtxList2._queryPointer(), sizeof(sVertex2) * nVtx);
        }
    }


    //report in console
    printf ("perimetri: %d\n", getNumPerimetri());
    for (u32 i=0; i<getNumPerimetri(); i++)
    {
        const sInfo *info = getPerimetroByIndex(i);
        printf ("  perimetro #%d, num quad: %d, linea-chiusa:%c, num-vtx:%d\n    ", i, info->numQuad, listaPerimetri(i).bSiChiudeSuSeStesso?'Y':'N', info->numVtx);

        for (u32 i2=0; i2<info->numQuad; i2++)
        {
            const sQuad *q = &quadList.buffer[listaPerimetri(i).quadIndexList[i2]];
            printf ("(%d,%d, t=%d) ", q->x, q->y, info->quadTypeList[i2]);
        }
        printf ("\n");
    }

    //free vari
    quadList.unsetup();
    
    //free dei perimetri
    for (u32 i=0; i<listaPerimetri.getNElem(); i++)
    {
        GOSFREE(gos::getScrapAllocator(), listaPerimetri[i].quadIndexList);
        GOSFREE(gos::getScrapAllocator(), listaPerimetri[i].quadTypeList);
    }
    listaPerimetri.unsetup();
}

//*******************************************************
void MarchingSquare::priv_perimetro_begin (LineaList &linea, u32 ct, FastArray<sPerimetro> &listaPerimetri)
{
    //ok, ho trovato un candidato per l'inizio di un perimetro
    //Uso <linea> come buffer temporaneo
    linea.reset();
    const bool bSiChiudeSuSeStesso = priv_perimetro_createTemp (linea, ct, NULL);

    //in <linea> c'e' un valido perimetro, lo memorizzo
    priv_perimetro_store (listaPerimetri, linea, bSiChiudeSuSeStesso);
}

/*******************************************************
 * ritorna true se il perimetro trovato si "chiude su se stesso"
 */
bool MarchingSquare::priv_perimetro_createTemp (LineaList &linea, u32 ct, const sQuad *parent)
{
    //in base al tipo di quad, determino quale puo' essere il prossimo quad nel perimetro
    sQuad *quad = &quadList.buffer[ct];
    quad->alreadyUsed = 1;
    assert (0 != quad->quadType);
    assert (15 != quad->quadType);

    u32 newX = 0;
    u32 newY = 0;
//#define CONNECT(dstX, dstY) ct = (quad->x+dstX) + (quad->y+dstY) * quadList.getDimX();
    #define CONNECT(dstX, dstY) {\
        newX = (quad->x + dstX);\
        newY = (quad->y + dstY);}\


    //inserisco il quad <ct> nell'elenco
    const u32 n = linea.getNElem();
    linea[n].quadIndex = ct;
    linea[n].quadType = quad->quadType;
    linea[n].x = quad->x;
    linea[n].y = quad->y;


    switch (quad->quadType)
    {
    case 1:     CONNECT(0  , 1);   break;
    case 2:     CONNECT(1  , 0);   break;
    case 3:     CONNECT(1  , 0);   break;
    case 4:     CONNECT(0  ,-1);   break;    
    
    case 5:
        //dipende da dove arriva il padre di questo nodo
        //if (parent->y == quad->y-1)         CONNECT( 1  , 0)
        //else if (parent->y == quad->y+1)    CONNECT(-1  , 0)
        quad->alreadyUsed = 0;
        if (NULL == parent)
        {
            CONNECT( 0  , 1)
            linea[n].quadType = 1;
            quad->quadType = 4;
        }
        else if (parent->x == quad->x+1)
        {
            CONNECT( 0  ,-1)
            linea[n].quadType = 4;
            quad->quadType = 1;
        }
        else if (parent->x == quad->x-1)
        {
            CONNECT( 0  , 1)
            linea[n].quadType = 1;
            quad->quadType = 4;
        }
#ifdef _DEBUG
        else
            DBGBREAK;
#endif
        break;    

    case 6:     CONNECT( 0 , -1);   break;
    case 7:     CONNECT( 0 , -1);   break;

    case 8:     CONNECT(-1 , 0);   break;
    case 9:     CONNECT( 0 , 1);   break;
    case 10:
        //dipende da dove arriva il padre di questo nodo
        quad->alreadyUsed = 0;
        if (NULL == parent)
        {
            CONNECT( 1  , 0)
            linea[n].quadType = 2;
            quad->quadType = 8;
        }
        else if (parent->y == quad->y-1)
        {
            CONNECT(-1  , 0)
            linea[n].quadType = 8;
            quad->quadType = 2;
        }
        else if (parent->y == quad->y+1)
        {
            CONNECT(1  , 0)
            linea[n].quadType = 2;
            quad->quadType = 8;
        }
#ifdef _DEBUG        
        else
            DBGBREAK;
#endif
        break;

    case 11:     CONNECT( 1 , 0);   break;
    case 12:     CONNECT(-1 , 0);   break;
    case 13:     CONNECT( 0 , 1);   break;
    case 14:     CONNECT(-1 , 0);   break;

    default:
        DBGBREAK;
    }        


    if (newX >= quadList.getDimX() || newY >= quadList.getDimY())
    {
        //sono uscito fuori dalla mappa, il perimetro finisce qui
        return false;
    }

    ct = newX + newY * quadList.getDimX();
    if (ct == linea(0).quadIndex)
    {
        //ho fatto il giro e sono tornato al quad di partenza, la linea finisce qui
        return true;
    }

    return priv_perimetro_createTemp (linea, ct, quad);
}

//*******************************************************
void MarchingSquare::priv_perimetro_store (gos::FastArray<sPerimetro> &listaPerimetri, const LineaList &linea, bool bSiChiudeSuSeStesso) const
{
    const u32 numQuad = linea.getNElem();
    assert (numQuad != 0);

    const u32 n = listaPerimetri.getNElem();
    listaPerimetri[n].numQuad = numQuad;
    listaPerimetri[n].bSiChiudeSuSeStesso = bSiChiudeSuSeStesso;
    listaPerimetri[n].quadIndexList = GOSALLOCT(u32*, gos::getScrapAllocator(), sizeof(u32) * numQuad);
    listaPerimetri[n].quadTypeList = GOSALLOCT(u8*, gos::getScrapAllocator(), sizeof(u8) * numQuad);
    for (u32 i=0; i<numQuad; i++)
    {
        listaPerimetri[n].quadIndexList[i] = linea(i).quadIndex;
        listaPerimetri[n].quadTypeList[i] = linea(i).quadType;
    }

    
}

//*******************************************************
void MarchingSquare::priv_calcVtxCoord (const sQuad *quad, ePos pos, gos::vec2f *out) const
{
    out->x = (f32)quad->x;
    out->y = -(f32)quad->y;
    switch (pos)
    {
    case ePos::a:
        out->x += 0.5f;
        break;

    case ePos::b:
        out->x += 1.0f;
        out->y -= 0.5f;
        break;

    case ePos::c:
        out->x += 0.5f;
        out->y -= 1.0f;
        break;

    case ePos::d:
        out->y -= 0.5f;
        break;
    }
}

//*******************************************************
void MarchingSquare::priv_perimetro_makeVtxList (const sPerimetro *p, gos::FastArray<gos::vec2f> &tempVtxList) const
{
#define ADD(v1,v2)  { priv_calcVtxCoord(quad, v1, &v); lastV2=v2; }


    tempVtxList.reset();

    u32 n = 0;
    gos::vec2f v;
    ePos lastV2;

    const u32 numQuad = p->numQuad;
    for (u32 i=0; i<numQuad; i++)
    {
        const sQuad *quad = &quadList.buffer[p->quadIndexList[i]];
        switch (p->quadTypeList[i])
        {
        default:
            //errore!
            DBGBREAK;

        case 1:     ADD(ePos::d, ePos::c); break;
        case 2:     ADD(ePos::c, ePos::b); break;
        case 3:     ADD(ePos::d, ePos::b); break;
        case 4:     ADD(ePos::b, ePos::a); break;

        case 5:     
            DBGBREAK;
            /* questo case non e' pie' valido perchè durante la priv_perimetro_createTemp() i type 5 e 10 li splitto
                in type 1, 2, 4, 8
            {
                if (0 == i)
                    ADD(ePos::d, ePos::c)
                else
                {
                    const sQuad *parent = &quadList.buffer[p->quadIndexList[i-1]];
                    if (parent->x == quad->x+1)
                        ADD(ePos::b, ePos::a)
                    else
                        ADD(ePos::d, ePos::c);
                }
            }
            */
            break;

        case 6:     ADD(ePos::c, ePos::a); break;
        case 7:     ADD(ePos::d, ePos::a); break;
        case 8:     ADD(ePos::a, ePos::d); break;
        case 9:     ADD(ePos::a, ePos::c); break;

        case 10:    
            DBGBREAK;
            /* questo case non e' pie' valido perchè durante la priv_perimetro_createTemp() i type 5 e 10 li splitto
                in type 1, 2, 4, 8

            {
                if (0 == i)
                    ADD(ePos::c, ePos::b)
                else
                {
                    const sQuad *parent = &quadList.buffer[p->quadIndexList[i-1]];
                    if (parent->y == quad->y-1)
                        ADD(ePos::a, ePos::d)
                    else
                        ADD(ePos::c, ePos::b);
                }
            }
            */
            break;

        case 11:    ADD(ePos::a, ePos::b); break;
        case 12:    ADD(ePos::b, ePos::d); break;
        case 13:    ADD(ePos::b, ePos::c); break;
        case 14:    ADD(ePos::c, ePos::d); break;
        }

        tempVtxList[n++] = v;
    }

    if (!p->bSiChiudeSuSeStesso)
    {
        const sQuad *quad = &quadList.buffer[p->quadIndexList[numQuad-1]];
        priv_calcVtxCoord(quad, lastV2, &v);
        tempVtxList[n++] = v;
    }
}

//*******************************************************
gos::vec2f MarchingSquare::priv_perimetro_getVtx (const gos::FastArray<gos::vec2f> &vtxList, bool bLineaChiusa, i32 index) const
{
    const i32 n = static_cast<i32>(vtxList.getNElem());
    if (bLineaChiusa)
    {
        if (index < 0)
            index = n + index;
        else if (index >= n)
            index -= n;
    }
    else
    {
        if (index < 0)
            index = 0;
        else if (index >= n)
            index = n-1;
    }

    return vtxList(index);
}

//*******************************************************
void MarchingSquare::priv_perimetro_smooth (const gos::FastArray<gos::vec2f> &src, gos::FastArray<sVertex2> &dst, bool bLineaChiusa, FastArray<gos::vec2f> &tmpSegmentNormList) const
{
    dst.reset();

    const i32 numSrcVtx = static_cast<i32>(src.getNElem());
    u32 nPointOUT = 0;
    if (numSrcVtx < 4)
    {
        for (i32 i=0; i<numSrcVtx; i++)
            dst[nPointOUT++].pos = src(i);
        return;
    }
    else
    {
        for (i32 i=0; i<numSrcVtx; i++)
        {
            const vec2f p0 = priv_perimetro_getVtx(src, bLineaChiusa, i-1);
            const vec2f p1 = priv_perimetro_getVtx(src, bLineaChiusa, i);
            const vec2f p2 = priv_perimetro_getVtx(src, bLineaChiusa, i+1);
            const vec2f p3 = priv_perimetro_getVtx(src, bLineaChiusa, i+2);

            for (u32 i2=0; i2<SMOOTH_LEVEL; i2++)
            {
                const f32 t = ((f32)i2 / (f32)SMOOTH_LEVEL);
                const f32 t2 = t*t;
                const f32 t3 = t2*t;
                    
                //catmull rom
                dst[nPointOUT++].pos  = 0.5f * (
                                                2.0f * p1 
                                                + (-1.0f * p0 + p2) * t
                                                + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 
                                                + (-1.0f * p0 + 3.0f * p1 -3.0f * p2 + p3) * t3
                                            );
                

                /*bezier
                dst[nPointOUT++].pos  = p0 * (-t3 +3*t2 -3*t +1) +
                                        p1 * (3*t3 -6*t2 +3*t) +
                                        p2 * (-3*t3 +3*t2) +
                                        p3 * (t3);
*/
            }
        }
    }

        
    //calcolo le normali dei segmenti
    tmpSegmentNormList.reset();
    for (u32 i=0; i<nPointOUT-1; i++)
    {
        //normale del segmento
        //if we define dx=x2-x1 and dy=y2-y1, then the normals are (-dy, dx) and (dy, -dx).
        const vec2f p = dst(i+1).pos - dst(i).pos;
        tmpSegmentNormList[i] = vec2f (-p.y, p.x);
        tmpSegmentNormList[i].normalize();        
    }
    
    //normale dell'ultimo segmento
    {
        const u32 i = nPointOUT-1;
        const vec2f p = dst(0).pos - dst(i).pos;
        tmpSegmentNormList[i] = vec2f (-p.y, p.x);
        tmpSegmentNormList[i].normalize();        
    }

    //calcolo la normale dei vtx
    for (u32 i=1; i<nPointOUT; i++)
    {
        dst[i].norm = (tmpSegmentNormList(i-1) + tmpSegmentNormList(i)) * 0.5f;
        //dst[i].norm.normalize();
    }
    dst[0].norm = (tmpSegmentNormList(nPointOUT-1) + tmpSegmentNormList(0)) * 0.5f;
    dst[0].norm.normalize();

    /*altero leggermente i vtx in base alla loro normale
    for (u32 i=1; i<nPointOUT-1; i++)
    {
        f32 q = 0.05f - gos::random01() * 0.1f;
        dst[i].pos += (dst[i].norm * q);

        q/=3.0f;
        dst[i-1].pos += (dst[i-1].norm * q);
        dst[i+1].pos += (dst[i+1].norm * q);
    }
    */
}

//*******************************************************
void MarchingSquare::buildMesh (f32 spessore, VertexList3 &out_vtxList, gos::FastArray<u16> &out_idxList) const
{
    out_vtxList.reset();
    out_idxList.reset();


    /*mesh del quad pieno
    out_vtxList.append (vec3f(0,0,0));
    out_vtxList.append (vec3f(1,0,0));
    out_vtxList.append (vec3f(1,0,-1));
    out_vtxList.append (vec3f(0,0,-1));
    out_idxList.append (0); out_idxList.append (1); out_idxList.append (2);
    out_idxList.append (2); out_idxList.append (3); out_idxList.append (0);
*/


    for (u32 i=0; i<getNumPerimetri(); i++)
    {
        u32 firstVtxSRC = 0;
        const sInfo *info = getPerimetroByIndex(i);
        for (u32 i2=0; i2<info->numQuad; i2++)
        {
            sQuad quad;
            quad.quadType = info->quadTypeList[i2];
            quad.x = info->xPos[i2];
            quad.y = info->yPos[i2];
            priv_mesh_buildSingleQuad (spessore, info, quad, firstVtxSRC, out_vtxList, out_idxList);
            firstVtxSRC += SMOOTH_LEVEL;
        }

        /*printf ("out_vtxList:n=%d, out_idxList:n=%d\n", out_vtxList.getNElem(), out_idxList.getNElem());
        for (u32 i2=0; i2<out_idxList.getNElem();)
        {
            const u16 idx1 = out_idxList(i2++);
            const u16 idx2 = out_idxList(i2++);
            const u16 idx3 = out_idxList(i2++);
            printf ("  tr (%d, %d, %d)    ", idx1, idx2, idx3);
            printf ("(%.1f, %.1f, %.1f) (%.1f, %.1f, %.1f) (%.1f, %.1f, %.1f)\n",
                out_vtxList(idx1).x, out_vtxList(idx1).y, out_vtxList(idx1).z,
                out_vtxList(idx2).x, out_vtxList(idx2).y, out_vtxList(idx2).z,
                out_vtxList(idx3).x, out_vtxList(idx3).y, out_vtxList(idx3).z);
            
        }*/        
    }
}

//*******************************************************
void MarchingSquare::priv_mesh_calcNumVtxIdx (u32 iPerimetro, u32 *out_nVtx, u32 *out_nIdx) const
{
    const sInfo *info = getPerimetroByIndex(iPerimetro);
    u32 nVtx = 0;
    u32 nTris = 0;

    for (u32 i=0; i<info->numQuad; i++)
    {
        switch (info->quadTypeList[i])
        {
        default:
            DBGBREAK;
            break;

        case 1:
        case 2:
        case 4:
        case 8:
            nVtx  += 3 + (SMOOTH_LEVEL-1);
            nTris += 1 + (SMOOTH_LEVEL-1);
            break;

        case 3:
        case 6:
        case 9:
        case 12:
            nVtx  += 4 + (SMOOTH_LEVEL-1)*2;
            nTris += 2 + (SMOOTH_LEVEL-1)*2;
            break;

        case 5:
        case 10:
            nVtx  += 2 * (3 + (SMOOTH_LEVEL-1));
            nTris += 2 * (1 + (SMOOTH_LEVEL-1));
            break;

        case 7:
        case 11:
        case 13:
        case 14:
            nVtx  += 5 + (SMOOTH_LEVEL-1);
            nTris += 3 + (SMOOTH_LEVEL-1);
            break;
        }
    }

    *out_nVtx = nVtx;
    *out_nIdx = nTris*3;
}

//*******************************************************
void MarchingSquare::priv_mesh_addTris (gos::FastArray<u16> &out_idxList, u32 firstVtx, u32 idx0, u32 idx1, u32 idx2, const VertexList3 &out_vtxList) const
{
    assert (firstVtx + idx0 <= 0xffff);
    assert (firstVtx + idx1 <= 0xffff);
    assert (firstVtx + idx2 <= 0xffff);

    assert (firstVtx + idx0 < out_vtxList.getNElem());
    assert (firstVtx + idx1 < out_vtxList.getNElem());
    assert (firstVtx + idx2 < out_vtxList.getNElem());

    out_idxList.append (static_cast<u16>(firstVtx + idx0));
    out_idxList.append (static_cast<u16>(firstVtx + idx1));
    out_idxList.append (static_cast<u16>(firstVtx + idx2));
}

//*******************************************************
void MarchingSquare::priv_mesh_buildSingleQuad (f32 spessore, const sInfo *info, const sQuad &quad, u32 firstVtxSRC, VertexList3 &out_vtxList, gos::FastArray<u16> &out_idxList) const
{
    const u32 firstVtx = out_vtxList.getNElem();

    //copio i primi (SMOOTH_LEVEL+1) vtx di SRC in DST
    sVertex3 vtx1;
    for (u32 i=0; i<SMOOTH_LEVEL+1; i++)
    {
        u32 index = firstVtxSRC+i;
        if (index >= info->numVtx)
            index -= info->numVtx;
        
        vtx1.pos.x = info->vtxList[index].pos.x;
        vtx1.pos.y = 0;
        vtx1.pos.z = info->vtxList[index].pos.y;
        
        vtx1.norm.set (info->vtxList[index].norm.x, 0, info->vtxList[index].norm.y);
        
        out_vtxList.append (vtx1);
    }

    const vec3f quadOrigin ((f32)quad.x, 0, (f32)-quad.y);
    const u8 tipoQuad = quad.quadType;
    switch (tipoQuad)
    {
    default:
        DBGBREAK;
        break;

    case 1:
    case 2:
    case 4:
    case 8:
        {
            vtx1.pos = quadOrigin;
            vtx1.norm.set(0,1,0);

            if (1 == tipoQuad)      { vtx1.pos.z -= 1.0f; }
            else if (2 == tipoQuad) { vtx1.pos.x += 1.0f; vtx1.pos.z -= 1.0f; }
            else if (4 == tipoQuad) { vtx1.pos.x += 1.0f; }
            out_vtxList.append (vtx1);

            for (u32 i=0; i<SMOOTH_LEVEL; i++)
                priv_mesh_addTris (out_idxList, firstVtx, i, 1+i, (SMOOTH_LEVEL+1), out_vtxList);
        }
        break;

    case 3:
        {
            vtx1.pos = quadOrigin;
            vtx1.pos.z -= 1.0f;
            vtx1.norm.set(0,1,0);
            out_vtxList.append (vtx1);

            for (u32 i=1; i<SMOOTH_LEVEL; i++)
            {
                vtx1.pos.x += 1.0f/(f32)SMOOTH_LEVEL;
                out_vtxList.append (vtx1);
            }

            vtx1.pos.x = quadOrigin.x +1.0f;
            out_vtxList.append (vtx1);

            u32 i2 = SMOOTH_LEVEL+1;
            for (u32 i=0; i<SMOOTH_LEVEL; i++)
            {
                priv_mesh_addTris (out_idxList, firstVtx, i, 1+i, i2, out_vtxList);
                priv_mesh_addTris (out_idxList, firstVtx, 1+i, i2+1, i2, out_vtxList);
                i2++;
            }
        }
        break;

    case 6:
        {
            vtx1.pos = quadOrigin;
            vtx1.pos.x += 1.0f;
            vtx1.pos.z -= 1.0f;
            vtx1.norm.set(0,1,0);
            out_vtxList.append (vtx1);

            for (u32 i=1; i<SMOOTH_LEVEL; i++)
            {
                vtx1.pos.z += 1.0f/(f32)SMOOTH_LEVEL;
                out_vtxList.append (vtx1);
            }

            vtx1.pos.z = quadOrigin.z;
            out_vtxList.append (vtx1);

            u32 i2 = SMOOTH_LEVEL+1;
            for (u32 i=0; i<SMOOTH_LEVEL; i++)
            {
                priv_mesh_addTris (out_idxList, firstVtx, i, 1+i, i2, out_vtxList);
                priv_mesh_addTris (out_idxList, firstVtx, 1+i, i2+1, i2, out_vtxList);
                i2++;
            }
        }
        break;     

    case 9:
        {
            vtx1.pos = quadOrigin;
            vtx1.norm.set(0,1,0);
            out_vtxList.append (vtx1);

            for (u32 i=1; i<SMOOTH_LEVEL; i++)
            {
                vtx1.pos.z -= 1.0f/(f32)SMOOTH_LEVEL;
                out_vtxList.append (vtx1);
            }

            vtx1.pos.z = quadOrigin.z -1.0f;
            out_vtxList.append (vtx1);

            u32 i2 = SMOOTH_LEVEL+1;
            for (u32 i=0; i<SMOOTH_LEVEL; i++)
            {
                priv_mesh_addTris (out_idxList, firstVtx, i, 1+i, i2, out_vtxList);
                priv_mesh_addTris (out_idxList, firstVtx, 1+i, i2+1, i2, out_vtxList);
                i2++;
            }
        }
        break;   

    case 12:
         {
            vtx1.pos = quadOrigin;
            vtx1.pos.x += 1.0f;
            vtx1.norm.set(0,1,0);
            out_vtxList.append (vtx1);

            for (u32 i=1; i<SMOOTH_LEVEL; i++)
            {
                vtx1.pos.x -= 1.0f/(f32)SMOOTH_LEVEL;
                out_vtxList.append (vtx1);
            }

            vtx1.pos.x = quadOrigin.x;
            out_vtxList.append (vtx1);

            u32 i2 = SMOOTH_LEVEL+1;
            for (u32 i=0; i<SMOOTH_LEVEL; i++)
            {
                priv_mesh_addTris (out_idxList, firstVtx, i, 1+i, i2, out_vtxList);
                priv_mesh_addTris (out_idxList, firstVtx, 1+i, i2+1, i2, out_vtxList);
                i2++;
            }
        }
        break;

    case 5:
    case 10:
        DBGBREAK;
        /* questo case non e' pie' valido perchè durante la priv_perimetro_createTemp() i type 5 e 10 li splitto
            in type 1, 2, 4, 8
        {
            v1 = out_vtxList(firstVtx);
            v1.z = floor (v1.z) + 1.0f;
            out_vtxList.append (v1);

            for (u32 i=0; i<SMOOTH_LEVEL; i++)
                priv_mesh_addTris (out_idxList, firstVtx, i, 1+i, (SMOOTH_LEVEL+1), out_vtxList);
        }
        */
        break;

    case 7:
        {
            vtx1.norm.set(0,1,0);

            const f32 x6 = quadOrigin.x;
            const f32 y6 = 0;
            const f32 z6 = quadOrigin.z -1.0f;

            vtx1.pos.set (x6+1.0f, y6, z6+1.0f);
            out_vtxList.append (vtx1);

            vtx1.pos.set (x6+1.0f, y6, z6);
            out_vtxList.append (vtx1);

            vtx1.pos.set (x6, y6, z6);
            out_vtxList.append (vtx1);

            for (u32 i=0; i<SMOOTH_LEVEL; i++)
                priv_mesh_addTris (out_idxList, firstVtx, i, 1+i, (SMOOTH_LEVEL+2), out_vtxList);
            priv_mesh_addTris (out_idxList, firstVtx, SMOOTH_LEVEL, SMOOTH_LEVEL+1, SMOOTH_LEVEL+2, out_vtxList);
            priv_mesh_addTris (out_idxList, firstVtx, SMOOTH_LEVEL+2, SMOOTH_LEVEL+3, 0, out_vtxList);
        }
        break;

    case 11:
        {
            vtx1.norm.set(0,1,0);

            const f32 x6 = quadOrigin.x;
            const f32 y6 = 0;
            const f32 z6 = quadOrigin.z;

            vtx1.pos.set (x6+1.0f, y6, z6-1.0f);
            out_vtxList.append (vtx1);

            vtx1.pos.set (x6, y6, z6-1.0f);
            out_vtxList.append (vtx1);

            vtx1.pos.set (x6, y6, z6);
            out_vtxList.append (vtx1);

            for (u32 i=0; i<SMOOTH_LEVEL; i++)
                priv_mesh_addTris (out_idxList, firstVtx, i, 1+i, (SMOOTH_LEVEL+2), out_vtxList);
            priv_mesh_addTris (out_idxList, firstVtx, SMOOTH_LEVEL, SMOOTH_LEVEL+1, SMOOTH_LEVEL+2, out_vtxList);
            priv_mesh_addTris (out_idxList, firstVtx, SMOOTH_LEVEL+2, SMOOTH_LEVEL+3, 0, out_vtxList);
        }
        break;    

    case 13:
        {
            vtx1.norm.set(0,1,0);

            const f32 x6 = quadOrigin.x + 1.0f;
            const f32 y6 = 0;
            const f32 z6 = quadOrigin.z;

            vtx1.pos.set (x6-1.0f, y6, z6-1.0f);
            out_vtxList.append (vtx1);

            vtx1.pos.set (x6-1, y6, z6);
            out_vtxList.append (vtx1);

            vtx1.pos.set (x6, y6, z6);
            out_vtxList.append (vtx1);

            for (u32 i=0; i<SMOOTH_LEVEL; i++)
                priv_mesh_addTris (out_idxList, firstVtx, i, 1+i, (SMOOTH_LEVEL+2), out_vtxList);
            priv_mesh_addTris (out_idxList, firstVtx, SMOOTH_LEVEL, SMOOTH_LEVEL+1, SMOOTH_LEVEL+2, out_vtxList);
            priv_mesh_addTris (out_idxList, firstVtx, SMOOTH_LEVEL+2, SMOOTH_LEVEL+3, 0, out_vtxList);
        }
        break;      

    case 14:
        {
            vtx1.norm.set(0,1,0);

            const f32 x6 = quadOrigin.x + 1.0f;
            const f32 y6 = 0;
            const f32 z6 = quadOrigin.z -1.0f;

            vtx1.pos.set (x6-1.0f, y6, z6+1.0f);
            out_vtxList.append (vtx1);

            vtx1.pos.set (x6, y6, z6+1.0f);
            out_vtxList.append (vtx1);

            vtx1.pos.set (x6, y6, z6);
            out_vtxList.append (vtx1);

            for (u32 i=0; i<SMOOTH_LEVEL; i++)
                priv_mesh_addTris (out_idxList, firstVtx, i, 1+i, (SMOOTH_LEVEL+2), out_vtxList);
            priv_mesh_addTris (out_idxList, firstVtx, SMOOTH_LEVEL, SMOOTH_LEVEL+1, SMOOTH_LEVEL+2, out_vtxList);
            priv_mesh_addTris (out_idxList, firstVtx, SMOOTH_LEVEL+2, SMOOTH_LEVEL+3, 0, out_vtxList);
        }
        break;  
    }

    priv_mesh_addSpessore (spessore, firstVtx, out_vtxList, out_idxList);
}

//*******************************************************
void MarchingSquare::priv_mesh_addSpessore (f32 SPESSORE, u32 firstVtx, VertexList3 &out_vtxList, gos::FastArray<u16> &out_idxList) const
{
    //static constexpr f32 SPESSORE = 1.5f;
    
    u32 i1 = out_vtxList.getNElem() - firstVtx;
    sVertex3 vtx1;
    for (u32 i=0; i<SMOOTH_LEVEL+1; i++)
    {
        vtx1 = out_vtxList[firstVtx + i];
        out_vtxList.append (vtx1);
    }

    for (u32 i=0; i<SMOOTH_LEVEL+1; i++)
    {
        vtx1 = out_vtxList[firstVtx + i];
        vtx1.pos.y -= SPESSORE;
        out_vtxList[firstVtx + i].norm.set (0,1,0);

        out_vtxList.append (vtx1);
    }


    u32 i2 = i1 + (SMOOTH_LEVEL+1);
    for (u32 i=0; i<SMOOTH_LEVEL; i++)
    {
        priv_mesh_addTris (out_idxList, firstVtx, i1, i2, i2+1, out_vtxList);
        priv_mesh_addTris (out_idxList, firstVtx, i2+1, i1+1, i1, out_vtxList);
        i1++;
        i2++;
    }

}
