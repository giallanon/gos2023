#ifndef _gosGPUViewport_h_
#define _gosGPUViewport_h_
#include "gosGPUEnumAndDefine.h"
#include "../gos/dataTypes/gosPosDim2D.h"

namespace gos
{
    class GPU; //fwd decl

    namespace gpu
    {
        /****************************************************
         * Viewport
         * 
         * struttura interna accessibile tramite i metodi di GPU utilizzando uno GPUViewportHandle
         * Questa classe accetta Pos2D/Dim2D come parametri per il dimensionamento della finestra.
         * E' cura di GPU assicurarsi che il rect definito da questa classe sia sempre costantemente aggiornato rispetto
         * all'attuale dimensione della finestra di rendering (vuol dire che se la finenstra di rendering cambia dimensioni, questa classe
         * viene automaticamente aggiornata da GPU)
         */
        struct Viewport
        {
        public:
                            Viewport()                                  { }

            u32             getX() const                                { return resolvedX; }
            u32             getY() const                                { return resolvedY; }
            u32             getW() const                                { return resolvedW; }
            u32             getH() const                                { return resolvedH; }

            f32             getX_f32() const                            { return resolvedX_f32; }
            f32             getY_f32() const                            { return resolvedY_f32; }
            f32             getW_f32() const                            { return resolvedW_f32; }
            f32             getH_f32() const                            { return resolvedH_f32; }

        private:
            void            resolve (i16 w, i16 h)                     
                            {
                                resolvedX = static_cast<u32>(x.resolve(w));
                                resolvedY = static_cast<u32>(y.resolve(h));
                                resolvedW = static_cast<u32>(width.resolve ((i16)resolvedX, (i16)w));
                                resolvedH = static_cast<u32>(height.resolve ((i16)resolvedY, (i16)h));

                                resolvedX_f32 = static_cast<f32>(resolvedX);
                                resolvedY_f32 = static_cast<f32>(resolvedY);
                                resolvedW_f32 = static_cast<f32>(resolvedW);
                                resolvedH_f32 = static_cast<f32>(resolvedH);
                            }

        private:
            gos::Pos2D  x;
            gos::Pos2D  y;
            gos::Dim2D  width;
            gos::Dim2D  height;
            u32         resolvedX;
            u32         resolvedY;
            u32         resolvedW;
            u32         resolvedH;
            f32         resolvedX_f32;
            f32         resolvedY_f32;
            f32         resolvedW_f32;
            f32         resolvedH_f32;

        friend class gos::GPU;
        };

    } //namespace gpu
} //namespace gos

#endif // _gosGPUViewport_h_