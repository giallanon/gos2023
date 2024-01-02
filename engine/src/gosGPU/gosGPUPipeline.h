#ifndef _gosGPUPipeline_h_
#define _gosGPUPipeline_h_
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

            void        cleanUp();

            Pipeline&   begin (GPU *gpu);
            Pipeline&   addShader (const GPUShaderHandle handle)               { shaderList.append (handle); return *this; }
            Pipeline&   setDrawPrimitive (eDrawPrimitive p)                    { drawPrimitive=p; return *this; }
            Pipeline&   setVtxDecl (const GPUVtxDeclHandle handle)             { vtxDeclHandle = handle; return *this; }
            bool        end (VkRenderPass &vkRenderPassHandle);


            VkPipeline  getVkHandle() const                                     { return vkPipelineHandle; }
            
        private:
            GPU                                 *gpu;
            gos::Allocator                      *allocator;
            gos::FastArray<GPUShaderHandle>     shaderList;
            eDrawPrimitive                      drawPrimitive;
            GPUVtxDeclHandle                    vtxDeclHandle;

            VkPipelineLayout    vkPipeLayoutHandle;
            VkPipeline          vkPipelineHandle;
        };
    }//namespace gpu
} //namespace gos


#endif //_gosGPUPipeline_h_
