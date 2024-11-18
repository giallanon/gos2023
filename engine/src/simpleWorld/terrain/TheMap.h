#ifndef _TheMap_h_
#define _TheMap_h_
#include "../gos/gos.h"


/**
 * @brief TheMap
 *  
 */
class TheMap
{
public:
    class LayerView
    {
    public:
                LayerView()                                     { data=dataNextLayer=NULL; dimx=dimy=sizeOfARow=0; }

        u32     getDimX() const                                 { return dimx; }
        u32     getDimY() const                                 { return dimy; }
        bool    isON (u32 x, u32 y) const                       { assert(y<dimy);  return layer_isON (x, y, data, dimx, sizeOfARow); }

        f32     calcAO(u32 x, u32 y) const
        {
            //AO == 1   => massima luce
            //AO == 0   => massima ombra
            if (NULL == dataNextLayer)
                return 1.0f;

            assert(y<dimy);
            static constexpr f32 INC = 1.0f / 18.0f;
            f32 ao = 1.0f;
            

            //y-1
            if (y > 0)
            {
                if (x>0 && layer_isON (x-1, y-1, dataNextLayer, dimx, sizeOfARow))
                    ao -= INC;

                if (layer_isON (x, y-1, dataNextLayer, dimx, sizeOfARow))
                    ao -= INC;

                if (x < dimx-1 && layer_isON (x+1, y-1, dataNextLayer, dimx, sizeOfARow))
                    ao -= INC;
            }            
            
            //y
            if (x>0 && layer_isON (x-1, y, dataNextLayer, dimx, sizeOfARow))
                ao -= INC;

            if (layer_isON (x, y, dataNextLayer, dimx, sizeOfARow))
                ao -= INC;

            if (x < dimx-1 && layer_isON (x+1, y, dataNextLayer, dimx, sizeOfARow))
                ao -= INC;
            
            
            //y+1
            if (y < dimy-1)
            {
                if (x>0 && layer_isON (x-1, y+1, dataNextLayer, dimx, sizeOfARow))
                    ao -= INC;

                if (layer_isON (x, y+1, dataNextLayer, dimx, sizeOfARow))
                    ao -= INC;

                if (x < dimx-1 && layer_isON (x+1, y+1, dataNextLayer, dimx, sizeOfARow))
                    ao -= INC;
            }            


            assert (ao >= 0 && ao <=1.0f);
            return ao;
        }
        
        f32     getWorldY() const                               { return worldY; }
        f32     getSpessore() const                             { return spessore; }
        u32     getColorRGBA() const                            { return colorRGBA; }
        

    private:
        const u8    *data;
        const u8    *dataNextLayer;
        u32         sizeOfARow;
        u32         dimx;
        u32         dimy;
        f32         worldY;
        f32         spessore;
        u32         colorRGBA;

        friend TheMap;
    };


public:
    static  bool    layer_isON (u32 x, u32 y, const u8 *layer, u32 dimx, u32 sizeOfARow)
    {
        assert(x<dimx);        
        const u32 byte = (x >> 3); //x / 8
        const u32 bit = (x & 0x07); // x % 8
        const u8 mask = 0x80 >> bit;
        return ((layer[y*sizeOfARow + byte] & mask) != 0);        
    }

public:
            TheMap();
            ~TheMap();

    bool    loadTGA (const char *mapFile, u8 firstLayerHeight, u8 numLayer, f32 worldY, f32 spessore);
    bool    loadFromSingleChannellHeightmap (const u8 *hmap, u32 hmapDimx, u32 hmapDimy, u8 firstLayerHeight, u8 numLayer, f32 worldY, f32 spessore);

    u32     getDimX() const                                 { return dimx; }
    u32     getDimY() const                                 { return dimy; }

    f32     getSpessore() const                             { return layersInfo.spessore; }
    u32     getNumLayer() const                             { return layersInfo.numLayer; }
    void    queryLayer (u32 iLayer, LayerView *out) const;


private:
    static constexpr u32 NUM_COLORS_IN_PALETTE = 8;
    static constexpr u32 COLOR_PALETTE[NUM_COLORS_IN_PALETTE] = {
        0x55503d,
        0x767a26,
        0xa59629,
        0xb8b947,
        0x809323,
        0x6b8521,
        0x4b5d31,
        0x2d532f
    };
        

private:
    struct sLayersInfo
    {
        u32     numLayer;
        u32     sizeOfASingleLayerInByte;
        f32     spessore;
        f32     firstLayerWorldY;
        u16     sizeOfARow;
    };

private:
    void    priv_free();
    void    priv_fillLayer (u32 iLayer, const u8 *rgba, u8 layerHeight);

private:
    gos::Allocator  *localAllocator;
    u32     dimx;
    u32     dimy;

    sLayersInfo layersInfo;
    u8          *layersBitmask;     //bitmask con l'elenco dei blocchi esistenti
    u8          *layersColorIndex;  //per ogni layer, un u8 che punta ad un colore in COLOR_PALETTE
};



#endif //_TheMap_h_
