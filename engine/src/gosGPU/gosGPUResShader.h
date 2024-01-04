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
            void            reset()                         { memset (_mainFnName, 0, sizeof(_mainFnName)); _vkHandle = VK_NULL_HANDLE; _shaderType = eShaderType::unknown; }

        public:
            char            _mainFnName[32]; 
            VkShaderModule  _vkHandle;
            eShaderType     _shaderType;
        };

    } //namespace gpu
} //namespace gos

#endif //_gosResGPUShader_h_
