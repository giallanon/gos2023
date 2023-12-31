#ifndef _gosPipeline_h_
#define _gosPipeline_h_
#include "gosGPUEnumAndDefine.h"
#include "gosGPUShader.h"
#include "../gos/gosFastArray.h"

namespace gos
{
    class GPU; //fwd decl

    namespace gpu
    {
        /*****************************************************
         * Pipeline
         * 
         */
        class Pipeline
        {
        public:
            Pipeline();
            ~Pipeline();

            void begin (GPU *gpu);
                void addShader (const GPUShaderHandle handle)               { shaderList.append (handle); }
                void setDrawPrimitive (eDrawPrimitive p)                    { drawPrimitive=p; }
                void setVtxDecl (const GPUVtxDeclHandle handle)             { vtxDeclHandle = handle; }
            bool end ();

        private:
            void    priv_deleteVkHandle();
            bool    REMOVE_priv_createRenderPass();

        private:
            GPU                                 *gpu;
            gos::Allocator                      *allocator;
            gos::FastArray<GPUShaderHandle>     shaderList;
            eDrawPrimitive                      drawPrimitive;
            GPUVtxDeclHandle                    vtxDeclHandle;

            VkPipeline          vkPipelineHandle;
            VkPipelineLayout    vkLayoutHandle;
            VkRenderPass        REMOVE_vkRenderPassHandle;
        };
    }//namespace gpu
} //namespace gos


#endif //_gosPipeline_h_
