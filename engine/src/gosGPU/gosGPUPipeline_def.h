#ifndef _gosGPURenderPass_def_h_
#define _gosGPURenderPass_def_h_
#include "gosGPUEnumAndDefine.h"
#include "vulkan/gosGPUVulkanEnumAndDefine.h"
#include "vulkan/gosGPUVulkan.h"
#include "../gos/gosUtils.h"

namespace gos
{
    namespace gpu
    {
        namespace pipe2
        {
            /****************************************
             * @brief 
             * 
             */
            struct PushConst
            {
                VkShaderStageFlags  stageFlags;
                u16                 offset;
                u16                 sizeInByte;
            };

            /****************************************
             * @brief 
             * 
             */
            struct Descriptor
            {
            public:
                u32                 binding;
                eGPUDescriptrorType descrType;
                u32                 usageFlags; 
                u32                 count;
            };

            /****************************************
             * @brief 
             * 
             */
            struct DescriptorSet
            {
            public:
                void            reset(VkDescriptorSetLayoutCreateFlags flagIN)      { flag=flagIN; numDescriptor=0; memset(list, 0, sizeof(list)); }

                                //per <usageFlags> vedi eGPUDescriptrorUsageFlag
                DescriptorSet&  add (u32 binding, eGPUDescriptrorType type, u32 count, u32 usageFlagsIN)
                { 
                    assert(numDescriptor<GOSGPU__NUM_MAX_DESCRIPTOR_PER_SET);
                    list[numDescriptor].binding = binding;
                    list[numDescriptor].descrType = type;
                    list[numDescriptor].count = count;
                    list[numDescriptor].usageFlags = usageFlagsIN;
                    numDescriptor++;
                    return *this;
                }

            public:
                VkDescriptorSetLayoutCreateFlags flag;
                u32             numDescriptor;
                Descriptor      list[GOSGPU__NUM_MAX_DESCRIPTOR_PER_SET];
            }; //DescriptorSet


            /****************************************
             * @brief 
             * 
             */
            struct VtxLayout
            {
			    u8              bindingLocation;
			    eDataFormat     format;
			    u8              offset;
            };

            /****************************************
             * @brief 
             * 
             */
            struct VtxStream
            {
            public:
                void        reset(u8 streamIndexIN, eVtxStreamInputRate inputRateIN)    { streamIndex = streamIndexIN; inputRate=inputRateIN; numLayout=0; }
                VtxStream&  add (u8 bindingLocation, u8 offset, eDataFormat fmt)        { assert (numLayout < GOSGPU__NUM_MAX_VTXDECL_ATTR); list[numLayout].bindingLocation = bindingLocation; list[numLayout].offset = offset; list[numLayout].format = fmt; numLayout++; return (*this); }

                u32     calcStride() const
                {
                    u32 ret = 0;
                    for (u8 i=0; i<numLayout; i++)
                    {
                        const u32 n = list[i].offset + gos::dataformat::getSize (list[i].format);
                        if (n > ret)
                            ret = n;
                    }
                    return ret;
                }                    
                
            public:
                u8                  streamIndex;
                eVtxStreamInputRate inputRate;
                u8                  numLayout;
                VtxLayout           list[GOSGPU__NUM_MAX_VTXDECL_ATTR];
            };


            /****************************************
             * @brief   Pipeline
             * 
             */
            struct Pipeline
            {
            public:
                u32                         descrset_num;
                GPUDescrSetLayoutHandle     descrset_handle_defList[GOSGPU__NUM_MAX_DESCRIPTOR_SETS];
                GPUPipelineHandle           pipeline_handle;

            public:
                void reset()
                {
                    descrset_num = 0;
                    for (u32 i=0; i<GOSGPU__NUM_MAX_DESCRIPTOR_SETS; i++)
                        descrset_handle_defList[i].setInvalid();
                    pipeline_handle.setInvalid();
                }
            }; //Pipeline


            /****************************************
             * @brief   Pipeline_def
             * 
             */
            struct Pipeline_def
            {
            public:
                struct sZBClearValue
                {
                    f32 depth;
                    f32 stencil;
                };


            public:
                void reset()
                {
                    numPushConst = 0;
                    memset (pushConstList, 0, sizeof(pushConstList));

                    numDescrSet = 0;
                    numShader = 0;
                    numVtxStream = 0;
                    cullMode = eCullMode::CCW;
                    drawPrimitive = eDrawPrimitive::trisList;
                    bWireframe = false;

                    zbuffer_enabled = true; 
                    zbuffer_format = eImageFormat::_DEPTH_BEST;
                    zbuffer_write=true; zbuffer_cmpFn = eZFunc::LESS;
                    zbuffer_clearCol.depth = 1; zbuffer_clearCol.stencil = 0;
                    
                    numRT = 0;
               }


                void                set_cullMode (eCullMode m)                                                          { cullMode = m; }
                void                set_drawPrimitive (eDrawPrimitive p)                                                { drawPrimitive = p; }

                void                set_zbuffer (eImageFormat fmt, bool zwriteIN=true, eZFunc zfuncIN=eZFunc::LESS)     { assert (gos::utils::isFormatWithDepth(fmt) || fmt==eImageFormat::_DEPTH_BEST); zbuffer_enabled = true; zbuffer_format=fmt; zbuffer_write=zwriteIN; zbuffer_cmpFn=zfuncIN; }

                void                add_rt (eImageFormat fmt)                                                           { assert (numRT < GOSGPU__NUM_MAX_ATTACHMENT); renderTargetFormat[numRT] = fmt; numRT++; }

                void                shader_add (GPUShaderHandle &handle)                                                { assert (numShader < GOSGPU__NUM_MAX_SHADER_PER_PIPELINE); shaderHandleList[numShader++] = handle; }

                void                pushConst_add (u16 offset, u16 sizeInByte, VkShaderStageFlags stageFlags)
                {
                    assert (numPushConst < GOSGPU__NUM_MAX_PUSH_CONSTANT_PER_PIPELINE);
                    pushConstList[numPushConst].offset = offset;
                    pushConstList[numPushConst].sizeInByte = sizeInByte;
                    pushConstList[numPushConst].stageFlags = stageFlags;
                    numPushConst++;
                }

                DescriptorSet&      descriptorset_add (VkDescriptorSetLayoutCreateFlags flag=0)                         { assert (numDescrSet < GOSGPU__NUM_MAX_DESCRIPTOR_SETS); descriptorSetList[numDescrSet].reset (flag); return descriptorSetList[numDescrSet++]; }
                        
                VtxStream&          vtxStream_add (eVtxStreamInputRate inputRateIN)                                     { assert (numVtxStream<GOSGPU__NUM_MAX_VXTDECL_STREAM); vtxStreamList[numVtxStream].reset(numVtxStream, inputRateIN); return vtxStreamList[numVtxStream++]; }



            public:
                u32             numPushConst;
                PushConst       pushConstList[GOSGPU__NUM_MAX_PUSH_CONSTANT_PER_PIPELINE];

                u32             numDescrSet;
                DescriptorSet   descriptorSetList[GOSGPU__NUM_MAX_DESCRIPTOR_SETS];
                
                u32             numShader;
                GPUShaderHandle shaderHandleList[GOSGPU__NUM_MAX_SHADER_PER_PIPELINE];

                u32             numVtxStream;
                VtxStream       vtxStreamList[GOSGPU__NUM_MAX_VXTDECL_STREAM];

                eCullMode       cullMode;
                eDrawPrimitive  drawPrimitive;
                bool            bWireframe;

                u8              zbuffer_enabled;
                eImageFormat    zbuffer_format;
                bool            zbuffer_write;                              //valido solo se zbIndex != 0xff
                eZFunc          zbuffer_cmpFn;                              //valido solo se zbIndex != 0xff
                sZBClearValue   zbuffer_clearCol;

                u32             numRT;
                eImageFormat    renderTargetFormat[GOSGPU__NUM_MAX_ATTACHMENT];

            }; //Pipeline_def

        }; //namespace pipe2



    } //namespace gpu
} //namespace gos

#endif //_gosGPURenderPass_def_h_
