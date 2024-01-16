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

            DescrSetInstanceWriter&     begin (gos::GPU *gpu, const GPUDescrSetInstancerHandle &descrSetInstanceHandle);
            DescrSetInstanceWriter&     updateUniformBuffer (u32 binding, const GPUUniformBufferHandle &handle);
            bool                        end();


        private:
            gos::GPU                        *gpu;
            VkDescriptorSet                 vkDescrSetHandle;
            u32                             numDescrBufferInfo;
            VkDescriptorBufferInfo          descrBufferInfoList[GOSGPU__NUM_MAX_DESCRIPTOR_PER_SET];

            u32                             numWriteDescr;
            VkWriteDescriptorSet            writeDescrList[GOSGPU__NUM_MAX_DESCRIPTOR_PER_SET];

            bool                            bAnyError;

        }; //DescrSetInstanceWriter
    } //namespace gpu
} //namespace gos

#endif //_gosGPUDescrSetInstanceWriter_h_