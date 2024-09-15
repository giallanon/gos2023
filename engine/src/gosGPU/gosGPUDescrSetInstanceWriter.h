#ifndef _gosGPUDescrSetInstanceWriter_h_
#define _gosGPUDescrSetInstanceWriter_h_
#include "gosGPUEnumAndDefine.h"


namespace gos
{
    class GPU; //fwd decl


    namespace gpu
    {
        /**********************************************
         * DescrSetInstanceWriter
         * 
         * classe di comodo per aggiornare i DescrSetInstance
         * 
        */
        class DescrSetInstanceWriter
        {
        public:
                                        DescrSetInstanceWriter();

            DescrSetInstanceWriter&     begin (gos::GPU *gpu, const GPUDescrSetInstanceHandle &descrSetInstanceHandle);
            DescrSetInstanceWriter&     bindUniformBuffer (u32 binding, const GPUUniformBufferHandle &handle);
            DescrSetInstanceWriter&     bindTextureAndSampler (u32 binding, const GPUTextureHandle &texHandle, const GPUSamplerHandle &samplerHandle);
            bool                        end();


        private:
            struct sBufferInfo
            {
                u32                             num;
                VkDescriptorBufferInfo          list[GOSGPU__NUM_MAX_DESCRIPTOR_PER_SET];
            };

            struct sImageInfo
            {
                u32                             num;
                VkDescriptorImageInfo          list[GOSGPU__NUM_MAX_DESCRIPTOR_PER_SET];
            };            

        private:
            gos::GPU                        *gpu;
            VkDescriptorSet                 vkDescrSetHandle;

            sBufferInfo                     bufferList;
            sImageInfo                      imageList;

            u32                             numWriteDescr;
            VkWriteDescriptorSet            writeDescrList[GOSGPU__NUM_MAX_DESCRIPTOR_PER_SET];

            bool                            bAnyError;

        }; //DescrSetInstanceWriter
    } //namespace gpu
} //namespace gos

#endif //_gosGPUDescrSetInstanceWriter_h_