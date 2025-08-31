#ifndef _gosGPURenderPassDesc_h_
#define _gosGPURenderPassDesc_h_
#include "gosGPUFramebuffer_def.h"


namespace gos
{
    namespace gpu
    {
        /**
         * @brief   RenderPassDesc
         * 
         */
        struct RenderPassDesc
        {
        public:
            enum eSubpassType
            {
                unknown = 0,
                gfx = 1,
                compute = 2
            };

            struct sDescriptor
            {
                u32                 binding;
                VkDescriptorType    descrType;
                VkShaderStageFlags  stageFlags;
                u32                 count;
            };

            struct sDescriptorSet
            {
                u32 numDescriptor;
                VkDescriptorSetLayoutCreateInfo flag;
                sDescriptor descriptorList[GOSGPU__NUM_MAX_DESCRIPTOR_PER_SET];
            };

            struct sPushConst
            {
                VkShaderStageFlags  stageFlags;
                u16                 offset;
                u16                 sizeInByte;
            };


            struct sSubpass
            {
            public:
                void    reset()     
                        {
                            type = eSubpassType::unknown;
                            memset(rtList, 0xFF, sizeof(rtList));
                            memset(zbList, 0xFF, sizeof(zbList));
                            memset(inputList, 0xFF, sizeof(inputList));
                            memset(preserveList, 0xFF, sizeof(preserveList));

                            zbuffer_enabled = true; zbuffer_write=true; zbuffer_cmpFn = eZFunc::LESS;
                            stencil_enabled = false; stencil_cmpFn = eStencilFunc::NEVER;
                            cullMode = eCullMode::CCW;
                            drawPrimitive = eDrawPrimitive::trisList;

                            numDescrSet = 0;
                            memset (descriptorSetList, 0, sizeof(descriptorSetList));

                            numPushConst = 0;
                            memset (pushConstList, 0, sizeof(pushConstList));

                            vtxDeclHandle.setInvalid();
                            vtxShaderHandle.setInvalid();
                            pxlShaderHandle.setInvalid();
                        }

            public:
                eSubpassType        type;
                u8                  rtList[GOSGPU__NUM_MAX_ATTACHMENT];         //terminano con 0xff
                u8                  zbList[GOSGPU__NUM_MAX_ATTACHMENT];         //terminano con 0xff
                u8                  inputList[GOSGPU__NUM_MAX_ATTACHMENT];      //terminano con 0xff
                u8                  preserveList[GOSGPU__NUM_MAX_ATTACHMENT];   //terminano con 0xff

                bool                zbuffer_enabled;
                bool                zbuffer_write;
                eZFunc              zbuffer_cmpFn;

                bool                stencil_enabled;
                eStencilFunc        stencil_cmpFn;

                eCullMode           cullMode;
                eDrawPrimitive      drawPrimitive;                

                u32                 numDescrSet;
                sDescriptorSet      descriptorSetList[GOSGPU__NUM_MAX_DESCRIPTOR_SETS];
                
                u32                 numPushConst;
                sPushConst          pushConstList[GOSGPU__NUM_MAX_PUSH_CONSTANT_PER_PIPELINE];

                GPUVtxDeclHandle    vtxDeclHandle;
                GPUShaderHandle     vtxShaderHandle;
                GPUShaderHandle     pxlShaderHandle;                
            };

        public:
            void    reset()         { framebuffer_def = NULL; for (u32 i=0; i<GOSGPU__NUM_MAX_SUBPASSES; i++) { subpassList[i].reset(); } }

        public:
            const Framebuffer_def   *framebuffer_def;
            sSubpass                subpassList[GOSGPU__NUM_MAX_SUBPASSES];

        };


    } //namespace gpu
} //namespace gos

#endif //_gosGPURenderPassDesc_h_
