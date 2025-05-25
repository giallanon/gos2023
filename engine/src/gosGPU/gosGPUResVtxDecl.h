#ifndef _gosGPUResVtxDecl_h_
#define _gosGPUResVtxDecl_h_
#include "gosGPUEnumAndDefine.h"
#include "gosGPUUtils.h"
#include "../gos/gosBit.h"
#include "../gos/gosUtils.h"

namespace gos
{
    class GPU; //fwd decl

    namespace gpu
    {
        /*****************************************************
         * VtxDecl
         * 
         * Struttura interna accessibile tramite i metodi di GPU utilizzando uno GPUVtxDeclHandle
         * Impacchetta in una manciata di 32bit tutte le info sulla VtxDecl
         */
        class VtxDecl
        {
        public:
            u8                      stream_getNum() const                                                   { return gos::byte32GET (streamInfo, 3); }
            eVtxStreamInputRate     stream_getInputRate (u8 streamIndex) const
                                    {
                                        if (streamIndex < stream_getNum())
                                        {
                                            if (gos::isBit32SET (streamInfo, streamIndex))
                                                return eVtxStreamInputRate::perInstance;
                                            return eVtxStreamInputRate::perVertex;
                                        }

                                        DBGBREAK;
                                        return eVtxStreamInputRate::perVertex;
                                    }
            u32                     stream_getStrideInByte (u8 streamIndex) const
                                    {
                                        u8 i;
                                        u8 n;
                                        if (!attr_getList (streamIndex, &i, &n))
                                            return 0;
                                        const u8 pos = i+n-1;
                                        return attr_getOffset(pos) + gos::dataformat::getSize (attr_getDataFormat(pos));
                                    }

            u8                      attr_getNum() const                                                     { return gos::byte32GET (streamInfo, 2); }
            u8                      attr_getStreamIndex (u8 declNum) const                                  { assert(declNum < attr_getNum()); return gos::byte32GET (packedAttrDescrList[declNum], 3); }
            u8                      attr_getBindingLocation (u8 declNum) const                              { assert(declNum < attr_getNum()); return gos::byte32GET (packedAttrDescrList[declNum], 2); }
            eDataFormat             attr_getDataFormat (u8 declNum) const                                   { assert(declNum < attr_getNum()); return static_cast<eDataFormat>(gos::byte32GET (packedAttrDescrList[declNum], 1)); }
            u8                      attr_getOffset (u8 declNum) const                                       { assert(declNum < attr_getNum()); return gos::byte32GET (packedAttrDescrList[declNum], 0); }
            bool                    attr_getList (u8 streamIndex, u8 *out_firstIndex, u8 *out_numDecl) const
                                    {
                                        assert (NULL != out_firstIndex);
                                        assert (NULL != out_numDecl);
                                        if (streamIndex < stream_getNum())
                                        {
                                            for (u8 i=0; i<attr_getNum(); i++)
                                            {
                                                if (attr_getStreamIndex(i) == streamIndex)
                                                {
                                                    *out_numDecl = 1;
                                                    *out_firstIndex = i++;
                                                    while (i < attr_getNum())
                                                    {
                                                        if (attr_getStreamIndex(i++) == streamIndex)
                                                            (*out_numDecl)++;
                                                        else
                                                            break;
                                                    }

                                                    return true;
                                                }
                                            }
                                        }

                                        *out_firstIndex = 0;
                                        *out_numDecl = 0;
                                        return false;
                                    }

        private:
            void                    reset()                                                                 { streamInfo=0; memset(packedAttrDescrList, 0, sizeof(packedAttrDescrList)); }
            void                    stream_setNum (u8 n)                                                    { gos::byte32SET (&streamInfo, n, 3); }
            void                    stream_setInputRate (u8 streamIndex, eVtxStreamInputRate inputRate)
                                    {
                                        assert (streamIndex < stream_getNum());
                                        assert (streamIndex < 16);
                                        
                                        if (inputRate == eVtxStreamInputRate::perInstance)
                                            gos::bit32SET (&streamInfo, streamIndex);
                                        else
                                            gos::bit32CLEAR (&streamInfo, streamIndex);
                                    }

            void                    attr_setNum (u8 n)                                                      { assert(n<GOSGPU__NUM_MAX_VTXDECL_ATTR); gos::byte32SET (&streamInfo, n, 2); }
            void                    attr_setStreamIndex (u8 declNum, u8 streamIndex)                        { assert(declNum < attr_getNum()); gos::byte32SET (&packedAttrDescrList[declNum], streamIndex, 3); }
            void                    attr_setBindingLocation(u8 declNum, u8 loc)                             { assert(declNum < attr_getNum()); gos::byte32SET (&packedAttrDescrList[declNum], loc, 2); }
            void                    attr_setDataFormat(u8 declNum, eDataFormat df)                          { assert(declNum < attr_getNum()); gos::byte32SET (&packedAttrDescrList[declNum], static_cast<u8>(df), 1); }
            void                    attr_setOffset(u8 declNum, u8 offset)                                   { assert(declNum < attr_getNum()); gos::byte32SET (&packedAttrDescrList[declNum], offset, 0); }


        private:
            u32     streamInfo; //qui ci sono [numStream], [numVtxDecl], [inputRate per ogni stream]
            u32     packedAttrDescrList[GOSGPU__NUM_MAX_VTXDECL_ATTR];   //per ogni vtxDecl, un 32bit che include tutte le info necessarie

        friend class gos::GPU;
        };        



        

    } //namespace gpu
} //namespace gos


#endif //_gosGPUResVtxDecl_h_
