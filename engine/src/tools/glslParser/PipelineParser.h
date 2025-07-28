#ifndef _PipelineParser_h_
#define _PipelineParser_h_
#include "SPVReflect.h"
#include "../../gosImage/gosImageEnumAndDefine.h"
#include "../../gos/gosIniFile.h"
#include "../../gosGPU/gosGPUEnumAndDefine.h"

namespace gos
{
    /**
     * @brief PipelineDef
     * 
     */
    struct PipelineDef
    {
    public:
        static const u8 NAME_MAX_SIZE = 32;
    
    public:
        void    setDefault();


    public:
        char                name[NAME_MAX_SIZE];
        
        eImageFormat        outputRT_fmt;
        eImageLayout        outputRT_finalLayout;
        eAttachmentLoadOp   outputRT_loadOp;
        eAttachmentStoreOp  outputRT_storeOp;
        u32                 outputRT_clearCol_ARGB;

        eImageFormat        outputDepth_fmt;
        eDepthStencilLayout outputDepth_finalLayout;
        eAttachmentLoadOp   outputDepth_loadOp;
        eAttachmentStoreOp  outputDepth_storeOp;        
        f32                 outputDepth_zClearValue;
        f32                 outputDepth_stencilClearValue;

        bool                zbuffer_enabled;
        bool                zbuffer_write;
        eZFunc              zbuffer_cmpFn;

        bool                stencil_enabled;
        eStencilFunc        stencil_cmpFn;

        eCullMode           cullMode;
        eDrawPrimitive      drawPrimitive;        
    };



    /**
     * @brief PipelineParser
     * 
     */
    class PipelineParser
    {
    public:
                PipelineParser();
                ~PipelineParser();


        bool    parseFromFile (const char *fname, PipelineDef *out);
        bool    parseFromMemory (const u8 *buffer, u32 sizeof_buffer, PipelineDef *out);
        //se ritornano false, il mesg di errore e' recuperabile tramite gos::err:
    

        bool    parse_PipelineDef (gos::IniFileSection &sec, PipelineDef *out);


    private:
        bool    priv_trueOrFalse (const char *val, bool *out) const;
    };

} //namespace gos

#endif //_PipelineParser_h_