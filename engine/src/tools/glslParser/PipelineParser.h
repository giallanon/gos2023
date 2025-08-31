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
        void                setDefault();


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


        bool    createFromIniFile (const char *fname, PipelineDef *out);

    private:
        bool    priv_parseIniFileSection  (const char *srcFolder, gos::IniFileSection &sec, PipelineDef *out);
        bool    priv_trueOrFalse (const char *val, bool *out) const;

        /**
         * @param   firstDefine se diverso da NULL, indica una define da passare allo shader
         *          A seguire, deve esserci un elenco di ulteriori define terminato da NULL
         *          es: "pippo", NULL
         *              "pippo", "pluto=3", NULL
         */
        bool    priv_shader_compile (const char *shaderSRCFile, const char *shaderStage, const char *firstDefine=NULL, ...) const;
    };

} //namespace gos

#endif //_PipelineParser_h_