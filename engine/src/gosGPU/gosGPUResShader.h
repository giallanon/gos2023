#ifndef _gosResGPUShader_h_
#define _gosResGPUShader_h_
#include "gosGPUEnumAndDefine.h"

namespace gos
{
    namespace gpu
    {
        /****************************************************
         * Shader
         * 
         * struttura interna accessibile tramite i metodi di GPU utilizzando uno GPUShaderHandle
         */
        struct Shader
        {
        public:
                            Shader()                        { }
            void            reset()                         { memset (mainFnName, 0, sizeof(mainFnName)); vkHandle = VK_NULL_HANDLE; shaderType = eShaderType::none; }

        public:
            char            mainFnName[32]; 
            VkShaderModule  vkHandle;
            eShaderType     shaderType;
        };

    } //namespace gpu
} //namespace gos

#endif //_gosResGPUShader_h_
