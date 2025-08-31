#ifndef _gosGPUFramebuffer_def_h_
#define _gosGPUFramebuffer_def_h_
#include "gosGPUEnumAndDefine.h"
#include "vulkan/gosGPUVulkanEnumAndDefine.h"

namespace gos
{
    namespace gpu
    {
        /**
         * @brief   Framebuffer_def
         * 
         */
        struct Framebuffer_def
        {
        public:
            struct sAttachment
            {
                eImageFormat        fmt;
                eImageLayout        initialLayout;
                eImageLayout        finalLayout;
                eAttachmentLoadOp   loadOp;
                eAttachmentStoreOp  storeOp;
            };

        public:
            void    reset()         { memset (this, 0, sizeof(Framebuffer_def)); }

        public:
            u32         numAttachment;
            sAttachment attachment[GOSGPU__NUM_MAX_ATTACHMENT];
        };

				//se [buffer] == NULL ritorna il num di byte necessari alla serializzazione
				//se [buffer] != NULL ritorna 0 in caso di errore oppure il num di byte memcpyati in [buffer]
        u32     serialize (const Framebuffer_def &def, u8 *buffer, u32 sizeof_buffer);

				//ritorna 0 in caso di errore
				//altrimenti ritorna il num di byte consumati per la deserializzazione
        u32     deserialize (const u8 *buffer, u32 sizeof_buffer, Framebuffer_def *out);
        

    } //namespace gpu
} //namespace gos

#endif //_gosGPUFramebuffer_def_h_