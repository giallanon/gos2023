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
    private:
        static constexpr u32 TEMP_MAGIC = gos::magic::_makeID (0xFF0000, 0x01);

    public:
        static constexpr u8 NAME_MAX_SIZE = 32;

    public:
        void                setDefault();

				//se [buffer] == NULL ritorna il num di byte necessari alla serializzazione
				//se [buffer] != NULL ritorna 0 in caso di errore oppure il num di byte memcpyati in [buffer]
		u32 	serialize (u8 *buffer, u32 sizeof_buffer) const;

				//ritorna 0 in caso di errore
				//altrimenti ritorna il num di byte consumati per la deserializzazione
		u32 	deserialize (const u8 *buffer, u32 sizeof_buffer);


    public:
        char                name[NAME_MAX_SIZE];
        
        eImageFormat        outputRT_fmt;
        eImageLayout        outputRT_finalLayout;
        eAttachmentLoadOp   outputRT_loadOp;
        eAttachmentStoreOp  outputRT_storeOp;
        u32                 outputRT_clearCol_ARGB;

        eImageFormat        outputDepth_fmt;
        eImageLayout outputDepth_finalLayout;
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
        
        VtxLayout           vtxLayout;
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