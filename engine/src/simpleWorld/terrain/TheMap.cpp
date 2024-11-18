#include "TheMap.h"
#include "../gosImage/loader/stb_image.h"

using namespace gos;


//********************************
TheMap::TheMap()
{
    localAllocator = NULL;
}

//********************************
TheMap::~TheMap()
{
    priv_free();
}

//********************************
void TheMap::priv_free()
{
    if (NULL == localAllocator)
        return;

    layersInfo.numLayer = 0;
    GOSFREE(localAllocator, layersBitmask);
    GOSFREE(localAllocator, layersColorIndex);

    localAllocator = NULL;
}

//********************************
bool TheMap::loadTGA (const char *mapFile, u8 firstLayerHeight, u8 numLayerIN, f32 worldY, f32 spessore)
{
    u32 fsize;
	u8 *buffer = fs::fileLoadInMemory (gos::getScrapAllocator(), mapFile, &fsize);
	if (NULL == buffer)
	{
		gos::logger::err ("TheMap::load => file '%s' not found\n", mapFile);
		return false;
	}

    int width, height, comp;
    u8 *rgba = stbi_load_from_memory (buffer, fsize, &width, &height, &comp, 4);
    GOSFREE(gos::getScrapAllocator(), buffer);
    if (NULL == rgba)
    {
        gos::logger::err ("TheMap::load => invalid file format '%s'\n");
        return false;
    }

    //creo una hmap usando solo il canale red
    u8 *hmap = GOSALLOCT(u8*, gos::getScrapAllocator(), width*height);
    u32 ct=0;
    u32 ctTGA=0;
    for (u32 i=0; i<static_cast<u32>(width*height); i++)
    {
        hmap[ct++] = rgba[ctTGA];
        ctTGA+=4;
    }
    stbi_image_free (rgba);

    const bool ret = loadFromSingleChannellHeightmap (hmap, static_cast<u32>(width), static_cast<u32>(height), firstLayerHeight, numLayerIN, worldY, spessore);
    GOSFREE(gos::getScrapAllocator(), hmap);
    return ret;
}

//********************************
bool TheMap::loadFromSingleChannellHeightmap (const u8 *hmap, u32 hmapDimx, u32 hmapDimy, u8 firstLayerHeight, u8 numLayerIN, f32 worldY, f32 spessore)
{
    assert (numLayerIN>0);
    assert ((u32)firstLayerHeight + (u32)numLayerIN <= 0xff);
    assert (hmapDimx % 8 == 0);

    priv_free();

    localAllocator = gos::getSysHeapAllocator();
    this->dimx = hmapDimx;
    this->dimy = hmapDimy;


    //Alloco i layer
    layersInfo.numLayer = numLayerIN;
    layersInfo.spessore = spessore;
    layersInfo.firstLayerWorldY = worldY;
    layersInfo.sizeOfARow = dimx/8;
    layersInfo.sizeOfASingleLayerInByte = (layersInfo.sizeOfARow * dimy);
    layersBitmask = GOSALLOCT (u8*, localAllocator, layersInfo.sizeOfASingleLayerInByte * layersInfo.numLayer);
    memset (layersBitmask, 0, layersInfo.sizeOfASingleLayerInByte * layersInfo.numLayer);

    layersColorIndex = GOSALLOCT (u8*, localAllocator, layersInfo.numLayer);
    memset (layersColorIndex, 0, layersInfo.numLayer);

    u8 colorIndex = 0;
    for (u32 y=0; y<layersInfo.numLayer; y++)
    {
        priv_fillLayer (y, hmap, firstLayerHeight);
        firstLayerHeight++;

        layersColorIndex[y] = colorIndex++;
        if (colorIndex >= NUM_COLORS_IN_PALETTE)
            colorIndex = NUM_COLORS_IN_PALETTE-1;
    }
    


    return true;
}


//********************************
void TheMap::priv_fillLayer (u32 iLayer, const u8 *hmap, u8 layerHeight)
{
    assert (iLayer < layersInfo.numLayer);

    u32 ctHMAP = 0;
    u32 ctROW = layersInfo.sizeOfASingleLayerInByte * iLayer;
    for (u32 y=0; y<dimy; y++)   
    {
        for (u32 x=0; x<dimx; x++)
        {
            const u8 h = hmap[ctHMAP++];
            if (h >= layerHeight)
            {
                const u32 bit = (x % 8);
                const u32 byte = x / 8;

                const u8 mask = 0x80 >> bit;
                layersBitmask[ctROW + byte] |= mask;
            }
        }

        ctROW += layersInfo.sizeOfARow;
    }

    //per come funziona il marching square, i bordi della mappa devono essere vuoti
    ctROW = layersInfo.sizeOfASingleLayerInByte * iLayer;
    memset (&layersBitmask[ctROW], 0, layersInfo.sizeOfARow);
    memset (&layersBitmask[ctROW + layersInfo.sizeOfASingleLayerInByte - layersInfo.sizeOfARow], 0, layersInfo.sizeOfARow);
    for (u32 y=0; y<dimy; y++)   
    {
        layersBitmask[ctROW] &= 0x7F;
        layersBitmask[ctROW + layersInfo.sizeOfARow-1] &= 0XFE;
        ctROW += layersInfo.sizeOfARow;
    }

}

//********************************
void TheMap::queryLayer (u32 iLayer, LayerView *out) const
{
    assert (iLayer < layersInfo.numLayer);

    out->dimx = dimx;
    out->dimy = dimy;
    out->sizeOfARow = layersInfo.sizeOfARow;
    out->data = &layersBitmask[layersInfo.sizeOfASingleLayerInByte * iLayer];
    out->worldY = layersInfo.firstLayerWorldY + iLayer * layersInfo.spessore;
    out->spessore = layersInfo.spessore;
    out->colorRGBA = COLOR_PALETTE[layersColorIndex[iLayer]];

    out->dataNextLayer = NULL;
    if (iLayer < layersInfo.numLayer-1)
        out->dataNextLayer = &layersBitmask[layersInfo.sizeOfASingleLayerInByte * (iLayer+1)];
}