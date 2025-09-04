#ifndef _gosGPU_h_
#define _gosGPU_h_
#include "gosGPUEnumAndDefine.h"
#include "vulkan/gosGPUVulkanEnumAndDefine.h"
#include "vulkan/gosGPUVulkan.h"
#include "../gos/gos.h"
#include "../gos/gosFastArray.h"
#include "../gosMath/gosMath.h"
#include "../gosInput/gosInput.h"
#include "../gos/gosHashMap.h"
#include "../gosImage/gosImage.h"
#include "gosGPUDescrSetInstanceWriter.h"
#include "gosGPUCmdBufferWriter.h"
#include "utils/gosGPUMainLoop.h"
#include "gosGPUResCommandBuffer.h"
#include "gosGPUResDepthStencil.h"
#include "gosGPUResDescrPool.h"
#include "gosGPUResDescrSetInstance.h"
#include "gosGPUResDescrSetLayout.h"
#include "gosGPUResFrameBuffer.h"
#include "gosGPUResRenderLayout.h"
#include "gosGPUResRenderTarget.h"
#include "gosGPUResShader.h"
#include "gosGPUResBuffer.h"
#include "gosGPUResViewport.h"
#include "gosGPUResVtxDecl.h"
#include "gosGPUResTexture.h"
#include "gosGPUResSampler.h"

namespace gos
{
    /***************************************************
     * GPU
     */
    class GPU
    {
    public:
        //vulkan extensions
        static PFN_vkCmdPushDescriptorSetKHR   vkCmdPushDescriptorSetKHR;


    public:
        /*****************************************************
         * TempBuilder
         * 
         * Utility usata dai vari builder
         */
        class TempBuilder
        {
        public:
                    TempBuilder (GPU *gpuIN)            { gpu = gpuIN; }
            virtual ~TempBuilder ()             { }

        protected:
            GPU     *gpu;
        }; //class TempBuilder



        /*****************************************************
         * VtxDeclBuilder
         * 
         * Classe di comodo usata da GPU per la creazione delle VtxDecl
         */
        class VtxDeclBuilder
        {
        public:
                                    VtxDeclBuilder()                                        { }


            /*  Aggiunge un nuovo stream con [inputRate] indicato.
                Le successive chiamate a addDescriptor() aggiungono descriptor allo stream appena creato */
            VtxDeclBuilder&         addStream (eVtxStreamInputRate inputRate = eVtxStreamInputRate::perVertex);

            /* [bindingLocation] => bingind all'interno del vtx shader (ovvero il parametro location della dichiarazione layout
               [offsetInBuffer]  => offset in byte all'interno della struttra del vtx
               [dataFormat]      => data format all'interno del vtx shader */
            VtxDeclBuilder&         addLayout (u8 bindingLocation, u32 offsetInBuffer, eDataFormat dataFormat);

            void                    end();

        private:
            void                    priv_begin(GPU *gpuIN, GPUVtxDeclHandle *out_handle)    { this->gpu = gpuIN; handle = out_handle; numStreamIndex = 0; numAttributeDesc = 0; }
            void                    priv_markAsInvalid()                                    { numStreamIndex = 0xff; } 
            bool                    priv_isValid() const                                    { return (0xFF!=numStreamIndex); }


        private:
            GPU                     *gpu;
            GPUVtxDeclHandle        *handle;
            u8                      numStreamIndex;
            u8                      numAttributeDesc;
            eVtxStreamInputRate     inputRatePerStream[GOSGPU__NUM_MAX_VXTDECL_STREAM];
            gpu::sVtxDescriptor     attributeDesc[GOSGPU__NUM_MAX_VTXDECL_ATTR];

        friend class GPU;
        }; //class VtxDeclBuilder     


       /**************************************
         * FrameBuffersBuilder
         * 
         */
        class FrameBuffersBuilder : public TempBuilder
        {
        public:
                                    FrameBuffersBuilder (GPU *gpu, const GPURenderPassHandle &renderPassHandle, GPUFrameBufferHandle *out_handle);
            virtual                 ~FrameBuffersBuilder();

            FrameBuffersBuilder&    setRenderAreaSize (const gos::Dim2D &w, const gos::Dim2D &h);
            FrameBuffersBuilder&    bindRenderTarget (const GPURenderTargetHandle &handle);
            FrameBuffersBuilder&    bindDepthStencil (const GPUDepthStencilHandle &handle);

            bool                    end();


            bool                    anyError() const        { return bAnyError; }

        private:
            bool                    bAnyError;
            gos::Dim2D              width;
            gos::Dim2D              height;
            u32                     numRenderTarget;
            GPURenderTargetHandle   renderTargetHandleList[GOSGPU__NUM_MAX_ATTACHMENT];
            GPUDepthStencilHandle   depthStencilHandle;


            GPURenderPassHandle   renderPassHandle;
            GPUFrameBufferHandle    *out_handle;

        friend class GPU;
        }; //class FrameBuffersBuilder





        /**********************************************
         * RenderPassBuilder
         * 
         * Classe di comodo usata da GPU per la creazione dei RenderPass
         */
        class RenderPassBuilder : public TempBuilder
        {
        private:
            static const u8 NUM_MAX_SUBPASS = 8;

            typedef RenderPassBuilder RTLB;   //di comodo

        public:
            /**********************************************
             * Subpass
            */
            class SubPassInfo
            {
            public:
                                SubPassInfo ()                                          { owner = NULL; }

                SubPassInfo&    writeToRenderTarget (u8 index);
                SubPassInfo&    writeToDepthStencil ()                                  { bUseDepthStencil = true; return *this; }

                RTLB&           end()                                                   { return *owner; }

            private:
                enum class eMode : u8
                {
                    gfx = 0,
                    compute = 1
                };

            private:
                void                priv_begin (RTLB *ownerIN, eMode modeIN)    { owner = ownerIN; mode=modeIN; bUseDepthStencil = false; nRenderTarget=0; }
            
            private:
                RTLB    *owner;
                eMode   mode;
                bool    bUseDepthStencil;
                u8      nRenderTarget;
                u8      renderTargetIndexList[GOSGPU__NUM_MAX_ATTACHMENT];

            friend class RenderPassBuilder;
            }; //class SubPassInfo


        public:
                            RenderPassBuilder (GPU *gpuIN, GPURenderPassHandle *out_handle);
            virtual         ~RenderPassBuilder();

            RTLB&           requireRendertarget (const eImageFormat imageFormat, const eImageLayout initialLayout, const eImageLayout finalLayout, eAttachmentLoadOp loadOp, eAttachmentStoreOp storeOp);
            RTLB&           requireZBuffer (const eImageFormat imageFormat, const eImageLayout initialLayout, const eImageLayout finalLayout, eAttachmentLoadOp loadOp, eAttachmentStoreOp storeOp);
            
            SubPassInfo&    addSubpass_GFX ();
            SubPassInfo&    addSubpass_COMPUTE ();
            bool            end();
            
            bool            anyError() const                            { return bAnyError; }

        private:
            struct sRenderTargetInfo
            {
                VkFormat                imageFormat;
                VkImageLayout           initialLayout;
                VkImageLayout           finalLayout;
                VkAttachmentLoadOp      loadOp;
                VkAttachmentStoreOp     storeOp;
            };

            struct sDepthBufferInfo
            {
                void    reset()                 { isRequired = false; indexOfDepthStencilAttachment=0xFF; }

                bool    isRequired;
                u8      indexOfDepthStencilAttachment;
                eImageFormat            imageFormat;
                eImageLayout     initialLayout;
                eImageLayout     finalLayout;
                eAttachmentLoadOp       loadOp;
                eAttachmentStoreOp      storeOp;
            };

        private:
            bool                    priv_buildVulkan();

        private:
            GPURenderPassHandle     *out_handle;

            bool                    bAnyError;
            u8                      numRenderTargetInfo;
            sRenderTargetInfo       rtInfoList[GOSGPU__NUM_MAX_ATTACHMENT];
            sDepthBufferInfo        depthBuffer;
            u8                      numSubpassInfo;
            SubPassInfo             subpassInfoList[NUM_MAX_SUBPASS];

            VkRenderPass            vkRenderPassHandle;

        friend class GPU;
        }; // class RenderPassBuilder
    
    
        /*****************************************************
         * PipelineBuilder
         * 
         * Classe di comodo usata da GPU per la creazione delle Pipeline
         */
        class PipelineBuilder : public TempBuilder
        {
        public:
            /*****************************************************
             * DepthStencilParam
             * 
             */
            class DepthStencilParam
            {
            public:
                struct sZBufferParams
                {
                    bool    enabled;
                    bool    writeEnabled;
                    eZFunc  compareFn;

                    void    setDefault()    { enabled=true; writeEnabled=true; compareFn=eZFunc::LESS; }
                };

                struct sStencilFace
                {
                    eStencilOp      ifStencilFail;
                    eStencilOp      ifStencilSuccessAndDepthFail;
                    eStencilOp      ifStencilSuccessAndDepthSuccess;
                    eStencilFunc    compareFn;
                    u32             compareMask;
                    u32             writeMask;
                    u32             referenceValue;                

                    void    setDefault()
                            {
                                ifStencilFail = eStencilOp::KEEP;
                                ifStencilSuccessAndDepthFail = eStencilOp::KEEP;
                                ifStencilSuccessAndDepthSuccess = eStencilOp::KEEP;
                                compareFn = eStencilFunc::ALWAYS;
                                compareMask = writeMask = referenceValue = 0;
                            }
                };

                struct sStencilParams
                {
                    bool            enabled;
                    sStencilFace    frontFace;
                    sStencilFace    backFace;            

                    void    setDefault()    { enabled=false; frontFace.setDefault(); backFace.setDefault(); }
                };

            public:
                                    DepthStencilParam ()                                                { }

                void                setDefault()                                                        { zbp.setDefault(); stp.setDefault(); }
                PipelineBuilder&    end()                                                               { return *owner; }

                //============================== zbuffer param
                DepthStencilParam&  zbuffer_enable (bool b)                                             { zbp.enabled = b; return *this; }
                DepthStencilParam&  zbuffer_enableWrite (bool b)                                        { zbp.writeEnabled = b; return *this; }
                DepthStencilParam&  zbuffer_setFn (eZFunc fn)                                           { zbp.compareFn = fn; return *this; }

                //============================== stencil param
                DepthStencilParam&  stencil_enable (bool b)                                             { stp.enabled = b; return *this; }

                DepthStencilParam&  stencil_FrontFace_IfStencilFail (eStencilOp m)                      { stp.frontFace.ifStencilFail = m; return *this; }
                DepthStencilParam&  stencil_FrontFace_IfStencilSuccesAndDepthFail  (eStencilOp m)       { stp.frontFace.ifStencilSuccessAndDepthFail = m; return *this; }
                DepthStencilParam&  stencil_FrontFace_IfStencilSuccesAndDepthSuccess  (eStencilOp m)    { stp.frontFace.ifStencilSuccessAndDepthSuccess = m; return *this; }
                DepthStencilParam&  stencil_FrontFace_compareFn (eStencilFunc f)                        { stp.frontFace.compareFn = f; return *this; }
                DepthStencilParam&  stencil_FrontFace_compareMask (u32 v)                               { stp.frontFace.compareMask = v; return *this; }
                DepthStencilParam&  stencil_FrontFace_writeMask (u32 v)                                 { stp.frontFace.writeMask = v; return *this; }
                DepthStencilParam&  stencil_FrontFace_referenceValue (u32 v)                            { stp.frontFace.referenceValue = v; return *this; }

                DepthStencilParam&  stencil_BackFace_IfStencilFail (eStencilOp m)                       { stp.backFace.ifStencilFail = m; return *this; }
                DepthStencilParam&  stencil_BackFace_IfStencilSuccesAndDepthFail  (eStencilOp m)        { stp.backFace.ifStencilSuccessAndDepthFail = m; return *this; }
                DepthStencilParam&  stencil_BackFace_IfStencilSuccesAndDepthSuccess  (eStencilOp m)     { stp.backFace.ifStencilSuccessAndDepthSuccess = m; return *this; }
                DepthStencilParam&  stencil_BackFace_compareFn (eStencilFunc f)                         { stp.backFace.compareFn = f; return *this; }
                DepthStencilParam&  stencil_BackFace_compareMask (u32 v)                                { stp.backFace.compareMask = v; return *this; }
                DepthStencilParam&  stencil_BackFace_writeMask (u32 v)                                  { stp.backFace.writeMask = v; return *this; }
                DepthStencilParam&  stencil_BackFace_referenceValue (u32 v)                             { stp.backFace.referenceValue = v; return *this; }

                //============================== get
                const sZBufferParams&   getZBufferParams() const                                        { return zbp; }
                const sStencilParams&   getStencilParams() const                                        { return stp; }

            private:
                void            priv_bind (PipelineBuilder *ownerIN)                                    { owner = ownerIN; }

            private:
                PipelineBuilder *owner;
                sZBufferParams  zbp;
                sStencilParams  stp;

            friend PipelineBuilder;
            }; //class DepthStencilParam        
        
        public:
                                PipelineBuilder (GPU *gpu, const GPURenderPassHandle &renderPassHandle, GPUPipelineHandle *out_handle);
            virtual             ~PipelineBuilder();

            void                cleanUp();

            PipelineBuilder&    addShader (const GPUShaderHandle handle)                        { shaderList.append (handle); return *this; }
            PipelineBuilder&    setDrawPrimitive (eDrawPrimitive p)                             { drawPrimitive=p; return *this; }
            PipelineBuilder&    setVtxDecl (const GPUVtxDeclHandle handle)                      { vtxDeclHandle = handle; return *this; }
            DepthStencilParam&  depthStencil()                                                  { return depthStencilParam; }
			PipelineBuilder&    setCullMode (eCullMode m)							            { cullMode = m; return *this; }
			PipelineBuilder&    setWireframe (bool b)								            { bWireframe = b; return *this; }
            PipelineBuilder&    descriptor_add (const GPUDescrSetLayoutHandle handle)           { descrSetLayoutList.append (handle); return *this; }
            PipelineBuilder&    pushConstant_add (VkShaderStageFlags stageFlags, u16 offset, u16 sizeInByte, u8 *out_whichOne);

            bool                end ();

            bool                anyError() const                                                { return bAnyError; }

        private:
            struct sPushConstant
            {
                eShaderType whichShader;
                u16 offset;
                u16 size;
            };

        private:
            bool                priv_buildVulkan ();
            
        private:
            bool                                bAnyError;
            gos::Allocator                      *allocator;
            gos::FastArray<GPUShaderHandle>     shaderList;
            gos::FastArray<GPUDescrSetLayoutHandle> descrSetLayoutList;
            gos::FastArray<VkPushConstantRange> pushConstantList;
            eDrawPrimitive                      drawPrimitive;
            GPUVtxDeclHandle                    vtxDeclHandle;
            DepthStencilParam                   depthStencilParam;
            eCullMode                           cullMode;
            bool                                bWireframe;

            GPUPipelineHandle       *out_handle;
            GPURenderPassHandle   renderPassHandle;
            VkPipelineLayout        vkPipelineLayoutHandle;
            VkPipeline              vkPipelineHandle;

        friend class GPU;
        }; //PipelineBuilder


 
        /**************************************
         * DescriptorSetLayoutBuilder
         * 
         */
        class DescriptorSetLayoutBuilder : public TempBuilder
        {
        public:
                                            DescriptorSetLayoutBuilder (GPU *gpu, VkDescriptorSetLayoutCreateFlags createFlag, GPUDescrSetLayoutHandle *out_handle);
            virtual                         ~DescriptorSetLayoutBuilder();

                                            //per <usageFlags> vedi eGPUDescriptrorUsageFlag
            DescriptorSetLayoutBuilder&     add (eGPUDescriptrorType descrType, u32 usageFlags, u32 count=1);

            //aggiunge un descriptor al set.
            //  [stageFlags], vedi anche gos::ShaderStageFlag che contiene i flag utilizzabili
            DescriptorSetLayoutBuilder&     add_uniformBuffer (VkShaderStageFlags stageFlags, u32 count=1)                  { return priv_add (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, stageFlags, count); }
            DescriptorSetLayoutBuilder&     add_dynamicUniformBuffer (VkShaderStageFlags stageFlags, u32 count=1)           { return priv_add (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, stageFlags, count); }
            DescriptorSetLayoutBuilder&     add_storageBuffer (VkShaderStageFlags stageFlags, u32 count=1)                  { return priv_add (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlags, count); }
            DescriptorSetLayoutBuilder&     add_dynamicStorageBuffer (VkShaderStageFlags stageFlags, u32 count=1)           { return priv_add (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, stageFlags, count); }
            DescriptorSetLayoutBuilder&     add_combinedTextureAndSampler (VkShaderStageFlags stageFlags, u32 count=1)      { return priv_add (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stageFlags, count); }
            DescriptorSetLayoutBuilder&     add_sampler (VkShaderStageFlags stageFlags, u32 count=1)                        { return priv_add (VK_DESCRIPTOR_TYPE_SAMPLER, stageFlags, count); }
            DescriptorSetLayoutBuilder&     add_texture (VkShaderStageFlags stageFlags, u32 count=1)                        { return priv_add (VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, stageFlags, count); }
            bool                            end();

            bool                            anyError() const        { return bAnyError; }

        private:
            DescriptorSetLayoutBuilder&     priv_add (VkDescriptorType descrType, VkShaderStageFlags stageFlags, u32 count=1);

        private:
            bool    bAnyError;
            VkDescriptorSetLayoutCreateFlags createFlag;
            u32     nextBindingNumber;
            u32     numDescriptor;
            VkDescriptorSetLayoutBinding   list[GOSGPU__NUM_MAX_DESCRIPTOR_PER_SET];

            GPUDescrSetLayoutHandle    *out_handle;

        friend class GPU;
        }; //class DescriptorSetLayoutBuilder


   
        /**************************************
         * DescriptorPoolBuilder
         * 
         */
        class DescriptorPoolBuilder : public TempBuilder
        {
        public:
                                        DescriptorPoolBuilder (GPU *gpu, GPUDescrPoolHandle *out_handle);
            virtual                     ~DescriptorPoolBuilder();

            DescriptorPoolBuilder&      setMaxNumDescriptorSet (u32 n)              { numMaxDescriptorSets = n; return *this; }
            DescriptorPoolBuilder&      addPool_uniformBuffer (u32 howMany = 8);
            DescriptorPoolBuilder&      addPool_storageBuffer (u32 howMany = 8);
            DescriptorPoolBuilder&      addPool_combinedTextureAndSampler(u32 howMany = 8);
            DescriptorPoolBuilder&      addPool_sampler(u32 howMany = 8);
            DescriptorPoolBuilder&      addPool_texture(u32 howMany = 8);
            bool                        end();

            bool                        anyError() const                            { return bAnyError; }

        private:
            VkDescriptorPoolSize*       priv_findOrAddByDescrType (VkDescriptorType descrType, u32 howMany);

        private:
            bool    bAnyError;
            u32     numMaxDescriptorSets;
            u32     numPool;
            VkDescriptorPoolCreateFlags  vkPoolFlags;
            VkDescriptorPoolSize   list[GOSGPU__NUM_MAX_DESCRIPTOR_POOL_SIZE_PER_POOL];

            GPUDescrPoolHandle    *out_handle;

        friend class GPU;
        }; //class DescriptorPoolBuilder

   

    public:
                            GPU();
                            ~GPU();


        bool                init (GOSWinHandle mainWin, bool vSync);
        void                deinit();

        //================ window stuff
        GOSWinHandle        getWindow()                                     { return window.winH; }
        void                toggleFullscreen();
        bool                vsync_isEnabled() const                         { return vSync; }
        void                vsync_enable (bool b);


        //================ rendering & presentazione
        bool                swapChain_acquireImage (u64 timeout_ns=UINT64_MAX, VkSemaphore semaphore=VK_NULL_HANDLE, VkFence fence=VK_NULL_HANDLE);
        bool                swapChain_wasRecreated() const                  { return bSwapChainRecreatedDuringThisFrame; }
        VkImage             swapChain_getCurImage() const                   { return vulkan.swapChainInfo.vkImageList[currentSwapChainImageIndex]; }
        VkResult            swapChain_present (const VkSemaphore *semaphoreHandleList, u32 semaphoreCount);

        //================ swap chain info
        //La swap chain viene creata automaticamente da GPU::init()
        u32                 swapChain_getWidth() const                      { return vulkan.swapChainInfo.imageExtent.width; }
        u32                 swapChain_getHeight() const                     { return vulkan.swapChainInfo.imageExtent.height; }
        f32                 swapChain_calcAspectRatio() const               { return (f32)swapChain_getWidth() / (f32)swapChain_getHeight(); }
        eImageFormat        swapChain_getImageFormat() const;
        u8                  swapChain_getImageCount() const                 { return static_cast<u8>(vulkan.swapChainInfo.imageCount); }
        VkImageView         swapChain_getImageView(u8 i) const              { assert(i < swapChain_getImageCount()); return vulkan.swapChainInfo.vkImageListView[i]; }
        VkExtent2D          swapChain_getImageExten2D() const               { return vulkan.swapChainInfo.imageExtent; }


        //================ oggetti di sincronizzazione 
        void                waitIdle();
        void                waitIdle(eGPUQueueType q);
        bool                semaphore_create  (VkSemaphore *out);
        void                semaphore_destroy  (VkSemaphore &in);
        
        bool                fence_create  (bool bStartAsSignaled, VkFence *out);
        void                fence_destroy  (VkFence &in);

        //ritorna true se il [fence] e' segnalato, false se timeout
        bool                fence_wait (const VkFence &fenceHandle, u64 timeout_ns = UINT64_MAX);
        bool                fence_waitMany (const VkFence *fenceHandleList, bool bWaitForAll, u32 fenceCount, u64 timeout_ns = UINT64_MAX);

        //riporta [fence] in stato non segnalato
        void                fence_reset (const VkFence &fenceHandle);
        void                fence_resetMany (const VkFence *fenceHandleList, u32 fenceCount);

        bool                fence_isSignaled  (const VkFence &fenceHandle);


        //================ viewport
        //E' possibile creare tante viewport
        //La viewport di default (che matcha la risoluzione della swapchain), viene creata in automatico da GUPU::init() ed e' sempre
        //accessibile tramite viewport_getDefault()
        //Le viewport vengono automaticamente ridimensionate a seguito di un swapChain_recreate()
        bool                    viewport_create (const gos::Pos2D &x,const gos::Pos2D &y, const gos::Dim2D &w, const gos::Dim2D &h, GPUViewportHandle *out_handle);
        const gpu::Viewport*    viewport_get (const GPUViewportHandle &handle) const;
        void                    deleteResource (GPUViewportHandle &handle);

        /* ritorna la viewport di default che e' sempre garantito essere aggiornata alle attuali dimensioni della main window */
        GPUViewportHandle       viewport_getDefault () const                { return defaultViewportHandle; }
        

        //================ vtx declaration
        VtxDeclBuilder&     vtxDecl_createNew (GPUVtxDeclHandle *out_handle);
        void                deleteResource (GPUVtxDeclHandle &handle);
        bool                vtxDecl_query (const GPUVtxDeclHandle handle, gpu::VtxDecl *out) const;



        //================ render layout
        RenderPassBuilder&    renderPass_createNew (GPURenderPassHandle *out_handle);
        void                        deleteResource (GPURenderPassHandle &handle);
        bool                        toVulkan (const GPURenderPassHandle handle, VkRenderPass *out) const;
        const gpu::RenderLayout*    getInfo (const GPURenderPassHandle handle) const;


        //================ Frame buffer
        FrameBuffersBuilder&    frameBuffer_createNew (const GPURenderPassHandle &renderPassHandle, GPUFrameBufferHandle *out_handle);
        void                    deleteResource (GPUFrameBufferHandle &handle);
        bool                    toVulkan (const GPUFrameBufferHandle handle, VkFramebuffer *out, u32 *out_renderAreaW, u32 *out_renderAreaH) const;



        //================ Pipeline
        PipelineBuilder&            pipeline_createNew (const GPURenderPassHandle &enderLayoutHandle, GPUPipelineHandle *out_handle);
        void                        deleteResource (GPUPipelineHandle &handle);
        bool                        toVulkan (const GPUPipelineHandle handle, const gpu::sPipeline **out) const;


        //================ depth buffer
        GPUDepthStencilHandle       depthStencil_getDefault() const                         { return defaultDepthStencil.handle; }
        eImageFormat                depthStencil_getDefaultFormat() const                   { return defaultDepthStencil.gosFormat; }
        bool                        depthStencil_create (const eImageFormat fmt, const gos::Dim2D &w, const gos::Dim2D &h, bool bWithStencil, GPUDepthStencilHandle *out_handle);
        void                        deleteResource (GPUDepthStencilHandle &handle);
        const gpu::DepthStencil*    getInfo (const GPUDepthStencilHandle handle) const;

        //================ render target
        GPURenderTargetHandle       renderTarget_getDefault() const                         { return defaultRTHandle; }
		bool				        renderTarget_create (const gos::Dim2D &dimx, const gos::Dim2D &dimy, eImageFormat fmt, GPURenderTargetHandle *out_handle);
        void                        deleteResource (GPURenderTargetHandle &handle);
        const gpu::RenderTarget*    getInfo (const GPURenderTargetHandle handle) const;

        //================ command buffer
        bool                        cmdBuffer_create (eGPUQueueType whichQ, GPUCmdBufferHandle *out_handle);
        void                        deleteResource (GPUCmdBufferHandle &handle);
        bool                        toVulkan (const GPUCmdBufferHandle handle, VkCommandBuffer *out) const;


        //================ staging buffer
        bool                        stagingBuffer_create (u32 sizeInByte, GPUStgBufferHandle *out_handle);
        void                        deleteResource (GPUStgBufferHandle &handle);
        
        /**
         * @brief stagingBuffer_uploadToGPUBuffer()
         * copia [dataSRC] in [handleDST] usando [handleSRC] come buffer di appoggio.
         * I passaggi sono:  [datSRC] viene memcpy in [handleSRC] e poi [handleSRC] viene pushato in [handleDST]
         */
        bool                stagingBuffer_uploadToGPUBuffer (const GPUStgBufferHandle handleSRC, const void *dataSRC, const GPUVtxBufferHandle handleDST, u32 offsetDST, u32 howManyByteToCopy);
        bool                stagingBuffer_uploadToGPUBuffer (const GPUStgBufferHandle handleSRC, const void *dataSRC, const GPUIdxBufferHandle handleDST, u32 offsetDST, u32 howManyByteToCopy);

        //================ buffer unmapping / manualSync
        void                buffer_unmap (gpu::sMappedBuffer &m);
        void                buffer_manualSync (const gpu::sMappedBuffer *list, u32 numElemInList);



        //================ vertex buffer
        bool                vertexBuffer_create (u32 sizeInByte, eVIBufferMode mode, GPUVtxBufferHandle *out_handle);
        void                deleteResource (GPUVtxBufferHandle &handle)                                                             { priv_bufferDestroy (vtxBufferList, handle); }
        bool                toVulkan (const GPUVtxBufferHandle handle, VkBuffer *out) const;
        bool                writeAndSync (const GPUVtxBufferHandle handle, u32 offsetDST, const void *src, u32 sizeInByte) const    { return priv_bufferWriteAndSync (vtxBufferList, handle, offsetDST, src, sizeInByte); }
        bool                map (const GPUVtxBufferHandle handle, u32 offsetDST, u32 sizeInByte, gpu::sMappedBuffer *out) const     { return priv_bufferMap (vtxBufferList, handle, offsetDST, sizeInByte, out); }

        
        //================ index buffer
        bool                indexBuffer_create (u32 sizeInByte, eVIBufferMode mode, GPUIdxBufferHandle *out_handle);
        void                deleteResource (GPUIdxBufferHandle &handle)                                                             { priv_bufferDestroy (idxBufferList, handle); }
        bool                toVulkan (const GPUIdxBufferHandle handle, VkBuffer *out) const;
        bool                writeAndSync (const GPUIdxBufferHandle handle, u32 offsetDST, const void *src, u32 sizeInByte) const    { return priv_bufferWriteAndSync (idxBufferList, handle, offsetDST, src, sizeInByte); }
        bool                map (const GPUIdxBufferHandle handle, u32 offsetDST, u32 sizeInByte, gpu::sMappedBuffer *out) const     { return priv_bufferMap (idxBufferList, handle, offsetDST, sizeInByte, out); }

        //================ uniform buffer
        bool                uniformBuffer_create (u32 sizeInByte, eVIBufferMode mode, GPUUniformBufferHandle *out_handle);
        void                deleteResource (GPUUniformBufferHandle &handle)                                                             { priv_bufferDestroy (uniformBufferList, handle); }
        bool                toVulkan (const GPUUniformBufferHandle handle, VkBuffer *out, u32 *out_bufferSize) const;
        bool                writeAndSync (const GPUUniformBufferHandle handle, u32 offsetDST, const void *src, u32 sizeInByte) const    { return priv_bufferWriteAndSync (uniformBufferList, handle, offsetDST, src, sizeInByte); }
        bool                map (const GPUUniformBufferHandle handle, u32 offsetDST, u32 sizeInByte, gpu::sMappedBuffer *out) const     { return priv_bufferMap (uniformBufferList, handle, offsetDST, sizeInByte, out); }

        //================ storage buffer
        bool                storageBuffer_create (u32 sizeInByte, eVIBufferMode mode, GPUStorageBufferHandle *out_handle);
        void                deleteResource (GPUStorageBufferHandle &handle)                                                             { priv_bufferDestroy (storageBufferList, handle); }
        bool                toVulkan (const GPUStorageBufferHandle handle, VkBuffer *out, u32 *out_bufferSize) const;
        bool                writeAndSync (const GPUStorageBufferHandle handle, u32 offsetDST, const void *src, u32 sizeInByte) const    { return priv_bufferWriteAndSync (storageBufferList, handle, offsetDST, src, sizeInByte); }
        bool                map (const GPUStorageBufferHandle handle, u32 offsetDST, u32 sizeInByte, gpu::sMappedBuffer *out) const     { return priv_bufferMap (storageBufferList, handle, offsetDST, sizeInByte, out); }




        //================ shader
        bool                vtxshader_createFromMemory (const u8 *buffer, u32 bufferSize, const char *mainFnName, GPUShaderHandle *out_shaderHandle)            { return priv_shader_createFromMemory (buffer, bufferSize, eShaderType::vertexShader, mainFnName, out_shaderHandle); }
        bool                vtxshader_createFromFile (const char *filename, const char *mainFnName, GPUShaderHandle *out_shaderHandle)                          { return priv_shader_createFromFile (filename, eShaderType::vertexShader, mainFnName, out_shaderHandle); }
        
        bool                fragshader_createFromMemory (const u8 *buffer, u32 bufferSize, const char *mainFnName, GPUShaderHandle *out_shaderHandle)           { return priv_shader_createFromMemory (buffer, bufferSize, eShaderType::fragmentShader, mainFnName, out_shaderHandle); }
        bool                fragshader_createFromFile (const char *filename, const char *mainFnName, GPUShaderHandle *out_shaderHandle)                         { return priv_shader_createFromFile (filename, eShaderType::fragmentShader, mainFnName, out_shaderHandle); }
        
        VkShaderModule      shader_getVkHandle (const GPUShaderHandle shaderHandle) const;
        const char*         shader_getMainFnName (const GPUShaderHandle shaderHandle) const;
        eShaderType         shader_getType (const GPUShaderHandle shaderHandle) const;
        void                deleteResource (GPUShaderHandle &shaderHandle);

        //================ descriptor pool
        DescriptorPoolBuilder& descrPool_createNew (GPUDescrPoolHandle *out_handle);
        void                deleteResource (GPUDescrPoolHandle &handle);
        bool                toVulkan (const GPUDescrPoolHandle handle, VkDescriptorPool *out) const;


        //================ descriptorSet layout
        DescriptorSetLayoutBuilder&    descrSetLayout_create (GPUDescrSetLayoutHandle *out_handle);
        DescriptorSetLayoutBuilder&    descrSetLayout_create_updAfterBind (GPUDescrSetLayoutHandle *out_handle);
        DescriptorSetLayoutBuilder&    descrSetLayout_create_pushable (GPUDescrSetLayoutHandle *out_handle);
        void                deleteResource (GPUDescrSetLayoutHandle &handle);
        bool                toVulkan (const GPUDescrSetLayoutHandle handle, VkDescriptorSetLayout *out) const;

        //================ descriptorSetInstance
        bool                descrSetInstance_createNew (const GPUDescrPoolHandle &poolHandle, const GPUDescrSetLayoutHandle &descrSetLayoutHandle, GPUDescrSetInstanceHandle *out_handle);
        void                deleteResource (GPUDescrSetInstanceHandle &handle);
        bool                toVulkan (const GPUDescrSetInstanceHandle handle, VkDescriptorSet *out) const;

							
        //================ texture
		bool				texture_create2D (u16 dimx, u16 dimy, u8 nMipMap, eImageFormat fmt, const void *srcDATA, GPUTextureHandle *out_handle);
        bool				texture_create2D (const gos::Image *im, u8 srcTextureNum, GPUTextureHandle *out_handle);
        void                deleteResource (GPUTextureHandle &handle);
        bool                toVulkan (const GPUTextureHandle handle, VkImageView *out) const;

        //================ sampler
        bool                sampler_create (const gpu::SamplerDesc &desc, GPUSamplerHandle *out_handle);
        bool                toVulkan (const GPUSamplerHandle handle, VkSampler *out) const;
        //void                deleteResource (GPUSamplerHandle &handle);
        //                      delete resource NON esiste perche' i Sampler sono mantenuti per sempre da GPU e sharati nel caso
        //                      in cui si richiedano N sampler con le stesse caratteristiche

        //================ da rimuovere
        VkDevice           REMOVE_getVkDevice() const               { return vulkan.dev; }
        VkQueue            REMOVE_getGfxQHandle()                   { return vulkan.getQueueInfo(eGPUQueueType::gfx)->vkQueueHandle; }
        VkQueue            REMOVE_getComputeQHandle()               { return vulkan.getQueueInfo(eGPUQueueType::compute)->vkQueueHandle; }
        VkQueue            REMOVE_getTransferQHandle()              { return vulkan.getQueueInfo(eGPUQueueType::transfer)->vkQueueHandle; }

    private:
        struct sWindow
        {
        public:
            sWindow()                                                   { winH.setInvalid(); storedX = storedY = storedW = storedH = 0; }

            void            getCurrentSize (int *out_w, int *out_h)     { input::window_getSize (winH, out_w, out_h); }

            void            storeCurrentPosAndSize()
                            {
                                input::window_getPos (winH, &storedX, &storedY);
                                input::window_getSize (winH, &storedW, &storedH);
                            }

            GLFWwindow*     getGLF() const
                            {
                                GLFWwindow *glfWin;
                                input::window_getGLF (winH, &glfWin);
                                return glfWin;
                            }
        public:
            GOSWinHandle winH;
            int storedX;
            int storedY;
            int storedW;
            int storedH;
        };

        
        struct sDefaultDepthStencil
        {
            GPUDepthStencilHandle   handle;
            VkFormat                vkFormat;
            eImageFormat            gosFormat;
        };


        class ToBeDeletedBuilder
        {
        public:
                    ToBeDeletedBuilder ()           { timeToCheckIfPurgeIsNeeded_msec=u64MAX; }

            void    setup()                         { list.setup (gos::getSysHeapAllocator(), 32); }
            void    unsetup()                       { list.unsetup(); }

            void    add (TempBuilder *b)            { list.append(b); timeToCheckIfPurgeIsNeeded_msec = gos::getTimeSinceStart_msec() + 5000; }
            void    check (u64 timeNow_msec)        { if (timeNow_msec >= timeToCheckIfPurgeIsNeeded_msec) deleteAll(); }
            void    deleteAll()                     
                    { 
                        timeToCheckIfPurgeIsNeeded_msec=u64MAX; 
                        for (u32 i=0; i<list.getNElem(); i++)
                        { 
                            TempBuilder *b = list[i];
                            GOSDELETE(gos::getScrapAllocator(), b); 
                        } 
                        list.reset();
                    }

        private:
            gos::FastArray<TempBuilder*>    list;
            u64 timeToCheckIfPurgeIsNeeded_msec;
        };

    
        /**
         * @brief classe di comodo per effettuare comandi di trasferimento generalmente da staging buffer
         * ad altri tipi di buffer.
         * Laddove possibile, usa una transferQ invece che una gfxQ
         */
        class ImmediateTransferCmd
        {
        public:
                    ImmediateTransferCmd();

            void    setup (sVkDevice *vkDevice, gos::eGPUQueueType queueType);
            void    unsetup ();
            void    begin();
            void    copyBuffer (const VkBuffer srcBuffer, const VkBuffer dstBuffer, u32 offsetSRC, u32 offsetDST, u32 howManyByteToCopy);
            void    transitionImageLayout (VkImage image, u8 numMipMap, VkImageLayout oldLayout, VkImageLayout newLayout);
            void    end();


        public:
            VkCommandBuffer vkCmdBuffer;

        private:
            gos::eGPUQueueType  queueType;
            sVkDevice           *vkDevice;
        };

    private:
        bool                priv_initVulkan (eVulkanVersion vulkanVersion);
        void                priv_deinitVulkan();

        bool                priv_initHandleLists();
        void                priv_deinitandleLists();

        void                priv_vulkanAddDebugCallback();
        bool                priv_swapChain_recreate ();


                            template<typename THANDLE, typename TSTRUCT>
        bool                priv_fromHandleToPointer (const HandleList<THANDLE,TSTRUCT> &handleList, const THANDLE handle, TSTRUCT **out) const
                            {
                                assert (NULL != out);
                                return (handleList.fromHandleToPointer (handle, out));
                            }

        bool                priv_shader_createFromFile (const char *filename, eShaderType shaderType, const char *mainFnName, GPUShaderHandle *out_shaderHandle);
        bool                priv_shader_createFromMemory (const u8 *buffer, u32 bufferSize, eShaderType shaderType, const char *mainFnName, GPUShaderHandle *out_shaderHandle);

        void                priv_vxtDecl_onBuilderEnds (VtxDeclBuilder *builder);

        bool                priv_renderLayout_onBuilderEnds (RenderPassBuilder *builder);

        bool                priv_pipeline_onBuilderEnds (PipelineBuilder *builder);

        bool                priv_depthStencil_createFromStruct (gos::gpu::DepthStencil &depthStencil);
        void                priv_depthStencil_deleteFromStruct (gos::gpu::DepthStencil &depthStencil);

        bool                priv_renderTarget_createFromStruct (gos::gpu::RenderTarget &rt);
        void                priv_renderTarget_deleteFromStruct (gos::gpu::RenderTarget &rt);

        bool                priv_frameBuffer_onBuilderEnds (FrameBuffersBuilder *builder);
        void                priv_frameBuffer_deleteFromStruct (gpu::FrameBuffer *s);
        bool                priv_frameBuffer_recreate (gpu::FrameBuffer *s);
        bool                priv_frameBuffer_do_recreate (gpu::FrameBuffer *s);
        
        bool                priv_descrSetLayout_onBuilderEnds (DescriptorSetLayoutBuilder *builder);
        bool                priv_descrPool_onBuilderEnds (DescriptorPoolBuilder *builder);

        void                priv_samplerDelete (GPUSamplerHandle &handle);

        void                priv_createHelperStagingBuffer (u32 size);
        bool                immediateTransferCmd_begin();
        bool                immediateTransferCmd_end();

        bool                priv_bufferCreate (VkBufferUsageFlags vkUsage, u32 sizeInByte, bool bCanBeUsedBy_gfxQ, bool bCanBeUsedBy_computeQ, bool bCanBeUsedBy_transferQ, eVIBufferMode mode, gpu::Buffer *out);
        bool                priv_bufferMap (const GPUVtxBufferHandle handle, u32 offsetDST, u32 sizeInByte, void **out) const;

                            template<class THandleList, class THandle>
        void                priv_bufferDestroy (THandleList &list, THandle &handle)
                            {
                                gpu::Buffer *s;
                                if (list.fromHandleToPointer (handle, &s))
                                {
                                    vkDestroyBuffer (vulkan.dev, s->vkHandle, nullptr);
                                    gos::vulkanFreeMemory (vulkan, s->_vkMemHandle, nullptr, s->memoryAllocated);
                                    s->reset();
                                    list.release (handle);
                                }
                                handle.setInvalid();
                            }

                            
        /**
         * @brief valido solo per i buffer creati con eVIBufferMode::shared_cpuW_autoSync
         * <out> viene memcpiato nel buffer a partire da <offsetDST> per un totale di <sizeInByte> byte.
         * La sincronizzazione con GPU e' automatica
         */
                            template<class THandleList, class THandle>
        bool                priv_bufferWriteAndSync (const THandleList &list, const THandle &handle, u32 offsetDST, const void *src, u32 sizeInByte) const
                            {
                                assert (NULL != src);
                                assert (sizeInByte > 0);

                                gpu::Buffer *s;
                                if (!priv_fromHandleToPointer(list, handle, &s))
                                {
                                    gos::logger::err ("GPU::priv_bufferWriteAndSync() => invalid handle\n");
                                    return false;
                                }

                                if (eVIBufferMode::shared_cpuW_autoSync != s->mode)
                                {
                                    gos::logger::err ("GPU::priv_bufferWriteAndSync() => invalid buffer mode [%s]\n", gpu::enumToString(s->mode));
                                    return false;
                                }

                                //i buffer eVIBufferMode::shared_cpuW_autoSync sono sempre totalmente mappati all'atto della creazione
                                if (sizeInByte > s->bufferSize)
                                {
                                    gos::logger::err ("GPU::priv_bufferWriteAndSync() => invalid params1 (%d, %d). Buffer size is %d\n", offsetDST, sizeInByte, s->bufferSize);
                                    return false;
                                }

                                if (offsetDST + sizeInByte > s->bufferSize)
                                {
                                    gos::logger::err ("GPU::priv_bufferWriteAndSync() => invalid params2 (%d, %d). Buffer size is %d, mapped from %d\n", offsetDST, sizeInByte, s->bufferSize, s->mapped_offset);
                                    return false;
                                }

                                memcpy (&s->mapped_host_pt[offsetDST], src, sizeInByte);
                                return true;
                            }    

                            
                            template<class THandleList, class THandle>
        bool                priv_bufferMap (const THandleList &list, const THandle handle, u32 offsetDST, u32 sizeInByte, gpu::sMappedBuffer *out) const
                            {
                                memset (out, 0, sizeof(gpu::sMappedBuffer));

                                gpu::Buffer *s;
                                if (!priv_fromHandleToPointer(list, handle, &s))
                                {
                                    gos::logger::err ("GPU::priv_bufferMap() => invalid handle\n");
                                    return false;
                                }

                                if (eVIBufferMode::shared_cpuW_manualSync != s->mode)
                                {
                                    gos::logger::err ("GPU::priv_bufferMap() => invalid buffer mode. Buffer mode must be [shared_cpuW_manualSync], current mode is %s\n", gpu::enumToString(s->mode));
                                    return false;
                                }

                                if (NULL != s->mapped_host_pt)
                                {
                                    gos::logger::err ("GPU::priv_bufferMap(d) => buffer is already mapped\n");
                                    return false;
                                }

                                //size deve essere un multipo di out->deviceProperties.limits.nonCoherentAtomSize
                                const u32 minSize = static_cast<u32>(vulkan.phyDevInfo.deviceProperties.limits.nonCoherentAtomSize);
                                const u32 aa = sizeInByte % minSize;
                                sizeInByte += minSize - aa;                                

                                VkResult result = vkMapMemory (vulkan.dev, s->_vkMemHandle, offsetDST, sizeInByte, 0, &out->host_pt);
                                if (VK_SUCCESS != result)
                                {
                                    out->host_pt = NULL;
                                    gos::logger::err ("GPU::priv_bufferMap(d) => vkMapMemory() => %s\n", string_VkResult(result));
                                    return false;
                                }

                                out->offset = offsetDST;
                                out->size = sizeInByte;
                                out->_vkMemHandle = s->_vkMemHandle;
                                return true;
                            }

    private:
        gos::Allocator              *allocator;
        sWindow                     window;
        VkInstance                  vkInstance;
        VkSurfaceKHR                vkSurface;
        VkDebugUtilsMessengerEXT    vkDebugMessenger;
        sVkDevice                   vulkan;
        VkSurfaceCapabilitiesKHR    vkSurfCapabilities;
        sPhyDeviceInfo vkPhysicalDevInfo;
        VtxDeclBuilder              vtxDeclBuilder;
        u32                         currentSwapChainImageIndex;
        bool                        bRecreateSwapChainOnNextFrame;
        bool                        vSync;
        bool                        bSwapChainRecreatedDuringThisFrame;
        ToBeDeletedBuilder          toBeDeletedBuilder;

        GPUViewportHandle           defaultViewportHandle;
        GPURenderTargetHandle       defaultRTHandle;
        sDefaultDepthStencil        defaultDepthStencil;

        ImmediateTransferCmd        helperImmediateTransferCmd;
        GPUStgBufferHandle          helperStagingBuffer;

        HandleList<GPUShaderHandle, gpu::Shader>                    shaderList;
        HandleList<GPUVtxDeclHandle, gpu::VtxDecl>                  vtxDeclList;
        HandleList<GPUViewportHandle, gpu::Viewport>                viewportlList;
        gos::FastArray<GPUViewportHandle>                           viewportHandleList;
        HandleList<GPUDepthStencilHandle, gpu::DepthStencil>        depthStencilList;
        gos::FastArray<GPUDepthStencilHandle>                       depthStencilHandleList;
        HandleList<GPURenderTargetHandle, gpu::RenderTarget>        renderTargetList;
        gos::FastArray<GPURenderTargetHandle>                       renderTargetHandleList;

        HandleList<GPURenderPassHandle,gpu::RenderLayout>         renderLayoutList;
        HandleList<GPUPipelineHandle,gpu::sPipeline>                pipelineList;
        HandleList<GPUFrameBufferHandle, gpu::FrameBuffer>          frameBufferList;
        gos::FastArray<GPUFrameBufferHandle>                        frameBufferDependentOnSwapChainList;
        HandleList<GPUVtxBufferHandle,gpu::Buffer>                  vtxBufferList;
        HandleList<GPUStgBufferHandle,gpu::Buffer>                  staginBufferList;
        HandleList<GPUIdxBufferHandle,gpu::Buffer>                  idxBufferList;
        HandleList<GPUUniformBufferHandle, gpu::Buffer>             uniformBufferList;
        HandleList<GPUStorageBufferHandle, gpu::Buffer>             storageBufferList;
        HandleList<GPUDescrSetLayoutHandle, gpu::DescrSetLayout>    descrSetLayoutList;
        HandleList<GPUDescrPoolHandle, gpu::DescrPool>              descrPoolList;
        HandleList<GPUDescrSetInstanceHandle, gpu::DescrSetInstance> descrSetInstanceList;
        HandleList<GPUCmdBufferHandle, gpu::CommandBuffer>          cmdBufferList;
        HandleList<GPUTextureHandle,gpu::Texture>                   textureList;
        HandleList<GPUSamplerHandle, gpu::Sampler>                  samplerList;
        gos::HashMap<u32, GPUSamplerHandle>                         samplerDescrHashMap;
        
    };
} //namespace gos


#endif //_gosGPU_h_
