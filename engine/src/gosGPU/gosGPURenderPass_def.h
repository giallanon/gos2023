#ifndef _gosGPURenderPass_def_h_
#define _gosGPURenderPass_def_h_
#include "gosGPUFramebuffer_def.h"


namespace gos
{
    namespace gpu
    {
        /**
         * @brief   RenderPass_def
         * 
         */
        struct RenderPass_def
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
                eGPUDescriptrorType descrType;
                u32                 usageFlags;
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


            struct sVtxLayout
            {
			    u8              bindingLocation;
			    eDataFormat     format;
			    u8              offset;
            };

            struct sVtxStream
            {
                u8                  streamIndex;
                eVtxStreamInputRate inputRate;
                u8                  numLayout;
                sVtxLayout          layoutList[GOSGPU__NUM_MAX_VTXDECL_ATTR];
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
                            numVtxStream = 0;
                            memset (vtxStreamList, 0, sizeof(vtxStreamList));

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


                void    descriptor_addSet (VkDescriptorSetLayoutCreateFlags flag)
                {
                    assert (numDescrSet < GOSGPU__NUM_MAX_DESCRIPTOR_SETS-1);
                    const u32 n = numDescrSet++;
                    memset (&descriptorSetList[n], 0, sizeof(sDescriptorSet));
                    descriptorSetList[n].flag = flag;
                }
                        //per <usageFlags> vedi eGPUDescriptrorUsageFlag
                void    descriptor_add (u8 binding, eGPUDescriptrorType descrType, u32 usageFlags, u32 count = 1)
                {
                    assert (numDescrSet > 0);
                    const u32 set = numDescrSet-1;

                    const u32 n = descriptorSetList[set].numDescriptor++;
                    assert (n < GOSGPU__NUM_MAX_DESCRIPTOR_PER_SET);
                    
                    descriptorSetList[set].descriptorList[n].binding = binding;
                    descriptorSetList[set].descriptorList[n].descrType = descrType;
                    descriptorSetList[set].descriptorList[n].count = count;
                    descriptorSetList[set].descriptorList[n].usageFlags = usageFlags;
                }


                void    vtxdecl_addStream (eVtxStreamInputRate inputRate = eVtxStreamInputRate::perVertex)
                        {
                            assert (numVtxStream < GOSGPU__NUM_MAX_VXTDECL_STREAM-1);
                            vtxStreamList[numVtxStream].streamIndex = static_cast<u8>(numVtxStream);
                            vtxStreamList[numVtxStream].inputRate = inputRate;
                            vtxStreamList[numVtxStream].numLayout = 0;
                            memset (vtxStreamList[numVtxStream].layoutList, 0, sizeof(vtxStreamList[numVtxStream].layoutList));
                            numVtxStream++;
                        }
                void    vtxdecl_addLayout (u8 bindingLocation, u32 offsetInBuffer, eDataFormat dataFormat)
                {
                    assert (numVtxStream > 0);
                    const u32 n = numVtxStream-1;
                    const u32 i = vtxStreamList[n].numLayout++;
                    assert (i < GOSGPU__NUM_MAX_VTXDECL_ATTR);
                    vtxStreamList[n].layoutList[i].bindingLocation = bindingLocation;
                    vtxStreamList[n].layoutList[i].offset = offsetInBuffer;
                    vtxStreamList[n].layoutList[i].format = dataFormat;
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
                u32                 numVtxStream;
                sVtxStream          vtxStreamList[GOSGPU__NUM_MAX_VXTDECL_STREAM];
                
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

#endif //_gosGPURenderPass_def_h_
