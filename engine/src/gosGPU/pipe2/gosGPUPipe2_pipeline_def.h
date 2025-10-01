#ifndef _gosGPUPipe2_pipeline_def_h_
#define _gosGPUPipe2_pipeline_def_h_
#include "../gosGPUEnumAndDefine.h"
#include "../vulkan/gosGPUVulkanEnumAndDefine.h"
#include "../vulkan/gosGPUVulkan.h"
#include "../../gos/gosUtils.h"

namespace gos
{
    class GPU; //fwd

    namespace gpu
    {
        namespace pipe2
        {
            struct Pipeline_def; //fwd

            /****************************************
             * @brief 
             * 
             */
            struct PushConst
            {
            public:
                void    reset()     { offset = sizeInByte = 0; shaderTypeBitmask.zero(); }

            public:
                u16                 offset;
                u16                 sizeInByte;
                eShaderTypeBitmask  shaderTypeBitmask;
            };
            
            /****************************************
             * @brief 
             * 
             */
            struct Descriptor
            {
            public:
                void    reset() { binding=0; descrType=eGPUDescriptrorType::UNKNOWN; usageBitmask.zero(); count = 0; }

            public:
                u32                         binding;
                eGPUDescriptrorType         descrType;
                eGPUDescriptrorUsageBitmask usageBitmask; 
                u32                         count;
            };

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

                struct VtxStream
                {
                public:
                    VtxStream&      add (u8 bindingLocation, u8 offset, eDataFormat fmt)        { assert (numLayout < GOSGPU__NUM_MAX_VTXDECL_ATTR); list[numLayout].bindingLocation = bindingLocation; list[numLayout].offset = offset; list[numLayout].format = fmt; numLayout++; return (*this); }
                    Pipeline_def&   endVtxStream()                                                 { return *def; }


                    u32             calcStride() const
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

                private:
                                    VtxStream()                         { def = NULL; }
                                    ~VtxStream()                        { }
                    void            priv_setup (Pipeline_def *defIN, u8 streamIndexIN, eVtxStreamInputRate inputRateIN)    { def = defIN; streamIndex = streamIndexIN; inputRate=inputRateIN; numLayout=0; }

                private:
                    Pipeline_def    *def;

                friend Pipeline_def;
                };                

                struct DescriptorSet
                {
                public:
                                    //per <usageFlags> vedi eGPUDescriptrorUsageFlag
                    DescriptorSet&  add (u32 binding, eGPUDescriptrorType type, u32 count, eGPUDescriptrorUsageBitmask usageBitmaskIN)
                    { 
                        assert(numDescriptor<GOSGPU__NUM_MAX_DESCRIPTOR_PER_SET);
                        list[numDescriptor].binding = binding;
                        list[numDescriptor].descrType = type;
                        list[numDescriptor].count = count;
                        list[numDescriptor].usageBitmask = usageBitmaskIN;
                        numDescriptor++;
                        return *this;
                    }
                    Pipeline_def&   endDescriptorSet()                                                          { return *def; }


                public:
                    VkDescriptorSetLayoutCreateFlags flag;
                    u32             numDescriptor;
                    Descriptor      list[GOSGPU__NUM_MAX_DESCRIPTOR_PER_SET];

                private:
                                    DescriptorSet()                                                                 { }
                                    ~DescriptorSet()                                                                { }
                    void            priv_setup(Pipeline_def *defIN, VkDescriptorSetLayoutCreateFlags flagIN)        { def=defIN; flag=flagIN; numDescriptor=0; for (u32 i=0;i<GOSGPU__NUM_MAX_DESCRIPTOR_PER_SET;i++) { list[i].reset(); } }

                private:
                    Pipeline_def    *def;

                friend Pipeline_def;
                };

            public:
                Pipeline_def&   reset()
                {
                    numPushConst = 0;
                    for (u32 i=0; i<GOSGPU__NUM_MAX_PUSH_CONSTANT_PER_PIPELINE; i++)
                        pushConstList[i].reset();

                    numDescrSet = 0;
                    numShader = 0;
                    numVtxStream = 0;
                    cullMode = eCullMode::CCW;
                    drawPrimitive = eDrawPrimitive::trisList;
                    bWireframe = false;

                    zbuffer_enabled = false; 
                    zbuffer_format = eImageFormat::_DEPTH_BEST;
                    zbuffer_write=false;
                    zbuffer_cmpFn = eZFunc::LESS;
                    zbuffer_clearCol.depth = 1; zbuffer_clearCol.stencil = 0;
                    
                    numRT = 0;
                    return *this;
               }


                Pipeline_def&   set_cullMode (eCullMode m)                                                          { cullMode = m; return *this; }
                Pipeline_def&   set_drawPrimitive (eDrawPrimitive p)                                                { drawPrimitive = p; return *this; }
                Pipeline_def&   enable_wireframe()                                                                  { bWireframe = true; return *this; }

                Pipeline_def&   set_zbuffer (eImageFormat fmt, bool zwriteIN=true, eZFunc zfuncIN=eZFunc::LESS)     { assert (gos::utils::isFormatWithDepth(fmt) || fmt==eImageFormat::_DEPTH_BEST); zbuffer_enabled = true; zbuffer_format=fmt; zbuffer_write=zwriteIN; zbuffer_cmpFn=zfuncIN; return *this; }

                Pipeline_def&   add_rt (eImageFormat fmt)                                                           { assert (numRT < GOSGPU__NUM_MAX_ATTACHMENT); renderTargetFormat[numRT] = fmt; numRT++; return *this; }

                Pipeline_def&   shader_add (const GPUShaderHandle &handle)                                          { assert (numShader < GOSGPU__NUM_MAX_SHADER_PER_PIPELINE); shaderHandleList[numShader++] = handle; return *this; }

                Pipeline_def&   pushConst_add (u16 offset, u16 sizeInByte, eShaderTypeBitmask shaderTypeBitmask)
                {
                    assert (numPushConst < GOSGPU__NUM_MAX_PUSH_CONSTANT_PER_PIPELINE);
                    pushConstList[numPushConst].offset = offset;
                    pushConstList[numPushConst].sizeInByte = sizeInByte;
                    pushConstList[numPushConst].shaderTypeBitmask = shaderTypeBitmask;
                    numPushConst++;
                    return *this;
                }

                DescriptorSet&  descriptorset_add (VkDescriptorSetLayoutCreateFlags flag=0)                         { assert (numDescrSet < GOSGPU__NUM_MAX_DESCRIPTOR_SETS); descriptorSetList[numDescrSet].priv_setup (this, flag); return descriptorSetList[numDescrSet++]; }
                        
                VtxStream&      vtxStream_add (eVtxStreamInputRate inputRateIN)                                     { assert (numVtxStream<GOSGPU__NUM_MAX_VXTDECL_STREAM); vtxStreamList[numVtxStream].priv_setup(this, numVtxStream, inputRateIN); return vtxStreamList[numVtxStream++]; }



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

                bool            zbuffer_enabled;
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

#endif //#define _gosGPUPipe2_pipeline_def_h_

