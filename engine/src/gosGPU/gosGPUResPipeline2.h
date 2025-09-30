#ifndef _gosGPUResPipeline2_h_
#define _gosGPUResPipeline2_h_
#include "gosGPUEnumAndDefine.h"
#include "../gos/dataTypes/gosPosDim2D.h"

namespace gos
{
    namespace gpu
    {
        /****************************************
         * @brief   Pipeline2
         * 
         */
        struct Pipeline2
        {
        public:
            struct sPushConst
            {
                u8  offset;
                u8  size;
                u8  whichRange;
            };

        public:
			VkPipelineLayout            vkPipelineLayoutHandle;
			VkPipeline                  vkPipelineHandle;

            u16                         pcRange_num;
			VkPushConstantRange         pcRange_list[GOSGPU__NUM_MAX_PUSH_CONSTANT_RANGE_PER_PIPELINE];
            sPushConst                  pcList[GOSGPU__NUM_MAX_PUSH_CONSTANT_PER_PIPELINE];

            u32                         descrset_num;
            GPUDescrSetLayoutHandle     descrset_handle_defList[GOSGPU__NUM_MAX_DESCRIPTOR_SETS];

            u8                          vtx_numStream;
            u8                          vtx_stridePerStream[GOSGPU__NUM_MAX_VXTDECL_STREAM];
            

        public:
            void reset()
            {
                vkPipelineLayoutHandle = VK_NULL_HANDLE;
                vkPipelineHandle = VK_NULL_HANDLE;
                
                pcRange_num = 0;
                memset (&pcList, 0xff, sizeof(pcList));
                
                descrset_num = 0;
                for (u32 i=0; i<GOSGPU__NUM_MAX_DESCRIPTOR_SETS; i++)
                    descrset_handle_defList[i].setInvalid();
                //pipeline_handle.setInvalid();
                
                vtx_numStream = 0;
                memset (vtx_stridePerStream, 0, sizeof(vtx_stridePerStream));
            }

            //void deleteResources (gos::GPU *gpu);

        }; //Pipeline

    } //namespace gpu
} //namespace gos

#endif //_gosGPUResPipeline2_h_
