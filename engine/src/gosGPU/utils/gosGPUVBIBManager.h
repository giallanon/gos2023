#ifndef _gosGPUVBIBManager_h_
#define _gosGPUVBIBManager_h_
#include "../gosGPUEnumAndDefine.h"
#include "../../gos/gosFastArray.h"


namespace gos
{
    class GPU;  //fwd decl

    namespace gpu
    {
        /**
         * @brief VBIBManager
         * mantiene un elenco di VB/IB e fornisce dei metodi
         * per allocare spazio all'interno di questi buffer
        */
        class VBIBManager
        {
        public:
                    VBIBManager ();
                    ~VBIBManager();
                    
            void    setup (gos::Allocator *allocator, gos::GPU *gpu, u32 sizeOfAVertex, u32 sizeInByteOfAVtxBuffer, u32 sizeInByteOfAIdxBuffer);
            void    unsetup();

            void    add (const void *vtx, u32 numVtx, const u16 *idx, u32 numIndex, 
                        GPUVtxBufferHandle *out_vbHandle,
                        GPUIdxBufferHandle *out_ibHandle,
                        u32 *out_vtxStart,
                        u32 *out_idxStart);

        private:
            struct sVB
            {
                GPUVtxBufferHandle  handle;
                u32                 numFree;
            };

            struct sIB
            {
                GPUIdxBufferHandle  handle;
                u32                 numFree;
            };

        private:
            void    priv_copyVB (const void *vtx, u32 numVtx, const GPUVtxBufferHandle &handleDST, u32 vtxStart) const;
            void    priv_copyIB (const void *vid, u32 numIdx, const GPUIdxBufferHandle &handleDST, u32 idxStart) const;
           

        private:
            gos::GPU    *gpu;
            u32         sizeOfAVertex;
            u32         numVtxPerBuffer;
            u32         numIdxPerBuffer;
            GPUStgBufferHandle  hStageBuffer;
            FastArray<sVB>  vbList;
            FastArray<sIB>  ibList;

        };
    } //namespace gpu
} //namespace gos
#endif //_gosGPUVBIBManager_h_