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
                VkDescriptorSetLayoutCreateFlags flag;
                sDescriptor descriptorList[GOSGPU__NUM_MAX_DESCRIPTOR_PER_SET];
            };

            struct sPushConst
            {
                VkShaderStageFlags  stageFlags;
                u16                 offset;
                u16                 sizeInByte;
            };

            struct sClearColor
            {
                u32 argb;
            };
            struct sZBClearValue
            {
                f32 depth;
                f32 stencil;
            };
            union uClearCol
            {
                sClearColor     asColoBuffer;
                sZBClearValue   asZBuffer;
            };

            struct sSubpass
            {
            public:
                void    reset()     
                        {
                            type = eSubpassType::unknown;
                            num_rt = num_inputAttachment = num_preserveAttachment = 0;
                            memset(rtList, 0, sizeof(rtList));
                            memset(inputList, 0xFF, sizeof(inputList));
                            memset(preserveList, 0xFF, sizeof(preserveList));

                            zbIndex = 0xff; zbuffer_write=true; zbuffer_cmpFn = eZFunc::LESS;
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

                void    setType_gfx()                                   { type = eSubpassType::gfx; }
                void    setType_compute()                               { type = eSubpassType::compute; }
                
                void    add_rt (u8 attachmentIndex)                     { assert(attachmentIndex < GOSGPU__NUM_MAX_ATTACHMENT); assert (num_rt < GOSGPU__NUM_MAX_ATTACHMENT); rtList[num_rt++] = attachmentIndex; }
                void    add_inputAttachment (u8 attachmentIndex)        { assert(attachmentIndex < GOSGPU__NUM_MAX_ATTACHMENT); assert (num_inputAttachment < GOSGPU__NUM_MAX_ATTACHMENT); inputList[num_inputAttachment++] = attachmentIndex; }
                void    add_preserveAttachment (u8 attachmentIndex)     { assert(attachmentIndex < GOSGPU__NUM_MAX_ATTACHMENT); assert (num_preserveAttachment < GOSGPU__NUM_MAX_ATTACHMENT); preserveList[num_preserveAttachment++] = attachmentIndex; }

                void    set_zbuffer (u8 attachmentIndex, bool zwriteIN, eZFunc zfuncIN, bool stencilEnabledIN, eStencilFunc stencilFnIN)        { zbIndex = attachmentIndex; zbuffer_write=zwriteIN; zbuffer_cmpFn=zfuncIN; stencil_enabled=stencilEnabledIN; stencil_cmpFn=stencilFnIN; }

                void    set_cullMode (eCullMode m)                      { cullMode = m; }
                void    set_drawPrimitive (eDrawPrimitive p)            { drawPrimitive = p; }

                void    add_descriptorSet (u8 set, VkDescriptorSetLayoutCreateFlags flag)
                {
                    assert (set < GOSGPU__NUM_MAX_DESCRIPTOR_SETS);
                    assert (descriptorSetList[set].numDescriptor == 0);
                    descriptorSetList[set].flag = flag;
                }

                void    add_descriptor (u8 set, u8 binding, VkDescriptorType descrType, VkShaderStageFlags stageFlags, u32 count)
                {
                    assert (set < GOSGPU__NUM_MAX_DESCRIPTOR_SETS);
                    assert (descriptorSetList[set].numDescriptor < GOSGPU__NUM_MAX_DESCRIPTOR_PER_SET-1);
                    
                    u32 n = descriptorSetList[set].numDescriptor++;
                    descriptorSetList[set].descriptorList[n].binding = binding;
                    descriptorSetList[set].descriptorList[n].descrType = descrType;
                    descriptorSetList[set].descriptorList[n].stageFlags = stageFlags;
                    descriptorSetList[set].descriptorList[n].count = count;
                }

            public:
                eSubpassType        type;

                u8                  num_rt;
                u8                  num_inputAttachment;
                u8                  num_preserveAttachment;
                u8                  rtList[GOSGPU__NUM_MAX_ATTACHMENT];         //terminano con 0xff e sono degli indici per Framebuffer_def->attachment
                u8                  inputList[GOSGPU__NUM_MAX_ATTACHMENT];      //terminano con 0xff e sono degli indici per Framebuffer_def->attachment
                u8                  preserveList[GOSGPU__NUM_MAX_ATTACHMENT];   //terminano con 0xff e sono degli indici per Framebuffer_def->attachment

                u8                  zbIndex;                                    //0xff oppure in indice per Framebuffer_def->attachment
                bool                zbuffer_write;                              //valido solo se zbIndex != 0xff
                eZFunc              zbuffer_cmpFn;                              //valido solo se zbIndex != 0xff
                bool                stencil_enabled;                            //valido solo se zbIndex != 0xff. Se true, allora zbIndex deve essere != 0xff e il formato di zb deve includere lo stencil
                eStencilFunc        stencil_cmpFn;                              //valido solo se zbIndex != 0xff

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
            void    reset()
            {
                framebuffer_def = NULL; 
                for (u32 i=0; i<GOSGPU__NUM_MAX_SUBPASSES; i++)
                    subpassList[i].reset();

                memset (clearColList, 0, sizeof (clearColList));
            }




        public:
            const Framebuffer_def   *framebuffer_def;
            uClearCol               clearColList[GOSGPU__NUM_MAX_ATTACHMENT];
            sSubpass                subpassList[GOSGPU__NUM_MAX_SUBPASSES];

        };


    } //namespace gpu
} //namespace gos

#endif //_gosGPURenderPassDesc_h_
