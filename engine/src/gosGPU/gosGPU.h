#ifndef _gosGPU_h_
#define _gosGPU_h_
#include "gosGPUEnumAndDefine.h"
#include "vulkan/gosGPUVulkanDevice.h"
#include "vulkan/gosGPUVulkan.h"
#include "../gos/gos.h"
#include "../gos/gosFastArray.h"
#include "../gosMath/gosMath.h"
#include "../gosInput/gosInput.h"
#include "../gos/gosHashMap.h"
#include "../gosImage/gosImage.h"
#include "gosGPUDescrSetInstanceWriter.h"
#include "utils/gosGPUMainLoop.h"
#include "gosGPUResCommandBuffer.h"
#include "gosGPUResDepthStencil.h"
#include "gosGPUResDescrPool.h"
#include "gosGPUResDescrSetInstance.h"
#include "gosGPUResDescrSetLayout.h"
#include "gosGPUResPipeline2.h"
#include "gosGPUResRenderTarget.h"
#include "gosGPUResShader.h"
#include "gosGPUResBuffer.h"
#include "gosGPUResViewport.h"
#include "gosGPUResTexture.h"
#include "gosGPUResSampler.h"
#include "gosGPUUtils.h"
#include "pipe2/gosGPUPipe2_pipeline_def.h"
#include "pipe2/gosGPUPipe2_cmdBufferWriter.h"

namespace gos
{
    /***************************************************
     * GPU
     */
    class GPU
    {
    public:
        static bool shader_compile (const char *shaderSRCFile, const char *shaderStage, const char *spaceSeparateDefineList, const char *shaderDSTFile, bool bIncludeDebugInfo);

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

        //================ oggetti di sincronizzazione 
        void                waitIdle()                                                                                                          { vulkan.waitIdle(); }
        
        bool                semaphore_create  (VkSemaphore *out)                                                                                { return vulkan.semaphore_create(out); }
        void                semaphore_destroy  (VkSemaphore &in)                                                                                { vulkan.semaphore_destroy(in); }
        
        bool                fence_create  (bool bStartAsSignaled, VkFence *out)                                                                 { return vulkan.fence_create(bStartAsSignaled, out); }
        void                fence_destroy  (VkFence &in)                                                                                        { vulkan.fence_destroy(in); }

        //ritorna true se il [fence] e' segnalato, false se timeout
        bool                fence_wait (const VkFence &fenceHandle, u64 timeout_ns = UINT64_MAX)                                                { return vulkan.fence_wait (fenceHandle, timeout_ns); }
        bool                fence_waitMany (const VkFence *fenceHandleList, bool bWaitForAll, u32 fenceCount, u64 timeout_ns = UINT64_MAX)      { return vulkan.fence_waitMany (fenceHandleList, bWaitForAll, fenceCount, timeout_ns); }

        //riporta [fence] in stato non segnalato
        void                fence_reset (const VkFence &fenceHandle)                                                                            { vulkan.fence_reset(fenceHandle); }
        void                fence_resetMany (const VkFence *fenceHandleList, u32 fenceCount)                                                    { vulkan.fence_resetMany(fenceHandleList, fenceCount); }

        bool                fence_isSignaled  (const VkFence &fenceHandle)                                                                      { return vulkan.fence_isSignaled(fenceHandle); }

        //=================== supporto ai vari formati di immagine
        bool                isImage2DFmtSupported (eImageFormat fmt, eImageTiling tiling) const                                                 { return vulkan.isImage2DFmtSupported(fmt, tiling); }
        
        //=================== limits
        u32                 limits_get_maxDescriptorSetSampledImages() const            { return vulkan.limits_get_maxDescriptorSetSampledImages(); }
        u32                 limits_get_minUniformBufferOffsetAlignment() const          { return vulkan.limits_get_minUniformBufferOffsetAlignment(); }
        u32                 limits_get_minStorageBufferOffsetAlignment() const          { return vulkan.limits_get_minStorageBufferOffsetAlignment(); }

        
        //================ window stuff
        GOSWinHandle        getWindow()                                     { return mainWindow.winHandle; }
        void                toggleFullscreen();
        bool                vsync_isEnabled() const                         { return vSync; }
        void                vsync_enable (bool b);

        //================ swap chain info
        //Ammesso che una valida <mainWin> sia fornita, allora la swap chain viene creata automaticamente da GPU::init()
        bool                swapChain_acquireImage (gos::gpu::SwapchainImg *out, u64 timeout_ns=UINT64_MAX, VkSemaphore semaphore=VK_NULL_HANDLE, VkFence fence=VK_NULL_HANDLE);
        VkResult            swapChain_present (const VkSemaphore *semaphoreHandleList, u32 semaphoreCount, u32 imageIndex)              { return vulkan.swapChain_present (swapchain, semaphoreHandleList, semaphoreCount, imageIndex); }

        bool                swapChain_wasRecreated() const                  { return bSwapChainRecreatedDuringThisFrame; }
                            //ogni volta che la swapchain viene ricreata, questo id viene incrementato.
        u32                 swapChain_getCurrentAutoID() const              { return swapchainAutoID; }

        u32                 swapChain_getWidth() const                      { return swapchain.getWidth(); }
        u32                 swapChain_getHeight() const                     { return swapchain.getHeight(); }
        f32                 swapChain_calcAspectRatio() const               { return swapchain.calcAspectRatio(); }
        eImageFormat        swapChain_getImageFormat() const                { return gpu::fromVulkan(swapchain.getImageFormat()); }
        u8                  swapChain_getImageCount() const                 { return swapchain.getImageCount(); }
        VkExtent2D          swapChain_getImageExten2D() const               { return swapchain.getImageExten2D(); }

        //================ submit Q
        void                queue_waitIdle (eGPUQueueFamily whichOne)                                                                   { vulkan.queue_waitIdle(whichOne); }
        VkResult            queue_submit (eGPUQueueFamily whichOne, u32 submitCount, const VkSubmitInfo *submitInfo, VkFence fence)     { return vulkan.queue_submit (whichOne, submitCount, submitInfo, fence); }

        //================ viewport
        //E' possibile creare tante viewport
        //La viewport di default (che matcha la risoluzione della swapchain), viene creata in automatico da GUPU::init() ed e' sempre
        //accessibile tramite viewport_getDefault()
        //Le viewport vengono automaticamente ridimensionate a seguito di un swapChain_recreate()
        bool                    viewport_create (const gos::Pos2D &x,const gos::Pos2D &y, const gos::Dim2D &w, const gos::Dim2D &h, GPUViewportHandle *out_handle);
        const gpu::Viewport*    getInfo (const GPUViewportHandle &handle) const;
        void                    deleteResource (GPUViewportHandle &handle);

        /* ritorna la viewport di default che e' sempre garantito essere aggiornata alle attuali dimensioni della <mainWindow> */
        GPUViewportHandle       viewport_getDefault () const                { return defaultViewportHandle; }
        


        //================ render target
		bool				        renderTarget_create (const gos::Dim2D &dimx, const gos::Dim2D &dimy, eImageFormat fmt, GPURenderTargetHandle *out_handle)       { return renderTarget_create (dimx, dimy, fmt, eMemAccessMode::onGPU, out_handle); }
        bool				        renderTarget_create (const gos::Dim2D &dimx, const gos::Dim2D &dimy, eImageFormat fmt, eMemAccessMode memAccessMode, GPURenderTargetHandle *out_handle);
        void                        deleteResource (GPURenderTargetHandle &handle);
        const gpu::RenderTarget*    getInfo (const GPURenderTargetHandle handle) const;
        bool                        map (const GPURenderTargetHandle handle, gpu::sMappedImage *out) const;

        //================ zbuffer
        bool				        zbuffer_create (const gos::Dim2D &dimx, const gos::Dim2D &dimy, eImageFormat fmt, GPUZBufferHandle *out_handle)        { return zbuffer_create (dimx, dimy, fmt, eMemAccessMode::onGPU, out_handle); }
        bool				        zbuffer_create (const gos::Dim2D &dimx, const gos::Dim2D &dimy, eImageFormat fmt, eMemAccessMode memAccessMode, GPUZBufferHandle *out_handle);
        void                        deleteResource (GPUZBufferHandle &handle);
        const gpu::DepthStencil*    getInfo (const GPUZBufferHandle handle) const;
        eImageFormat		        zbuffer_getBestFormat() const                                                                                               { return zbuffer_bestFmt_noStencil; }
        eImageFormat		        zbuffer_getBestFormat (bool bWithStencil) const                                                                             { if (bWithStencil) return zbuffer_bestFmt_withStencil; return zbuffer_bestFmt_noStencil; }


        //================ Pipeline
        bool                        pipeline_createNew (const gpu::pipe2::Pipeline_def &rpd, GPUPipelineHandle *out_handle);
        void                        deleteResource (GPUPipelineHandle &handle);
        const gpu::Pipeline2*       getInfo (const GPUPipelineHandle handle) const;
        
        //================ descriptor pool
        DescriptorPoolBuilder&      descrPool_createNew (GPUDescrPoolHandle *out_handle);
        void                        deleteResource (GPUDescrPoolHandle &handle);
        bool                        toVulkan (const GPUDescrPoolHandle handle, VkDescriptorPool *out) const;

        //================ descriptorSet layout
        void                deleteResource (GPUDescrSetLayoutHandle &handle);
        bool                toVulkan (const GPUDescrSetLayoutHandle handle, VkDescriptorSetLayout *out) const;

        //================ descriptorSetInstance
        bool                        descrSetInstance_create (const GPUDescrPoolHandle &poolHandle, const GPUDescrSetLayoutHandle &descrSetLayoutHandle, GPUDescrSetInstanceHandle *out_handle);
        bool                        descrSetInstance_create (const GPUDescrPoolHandle &poolHandle, const GPUPipelineHandle pipelineHandle, u8 descrSetNum, GPUDescrSetInstanceHandle *out_handle);
        void                        deleteResource (GPUDescrSetInstanceHandle &handle);
        const gpu::DescrSetInstance* getInfo (const GPUDescrSetInstanceHandle handle) const;


        
        //================ command buffer
        bool                        cmdBuffer_create (eGPUQueueFamily whichQ, GPUCmdBufferHandle *out_handle, u32 threadID=gos::thread::getCurrentThreadID());
        void                        deleteResource (GPUCmdBufferHandle &handle);
        const gpu::CommandBuffer*   getInfo (const GPUCmdBufferHandle handle) const;


        //================ staging buffer
        bool                stagingBuffer_create (u32 sizeInByte, GPUStgBufferHandle *out_handle);
        void                deleteResource (GPUStgBufferHandle &handle);
        const gpu::Buffer*  getInfo (const GPUStgBufferHandle handle) const;
        
                //memcopia <dataSRC> in <&handleDST[offsetDST]>
        bool                stagingBuffer_memcpy (GPUStgBufferHandle &handleDST, u32 offsetDST, const void *dataSRC, u32 sizeof_dataSRC);

                /**
                 * @brief stagingBuffer_uploadToGPUBuffer()
                 * copia [dataSRC] in [handleDST] usando [handleSRC] come buffer di appoggio.
                 * I passaggi sono:  [datSRC] viene memcpy in [handleSRC] e poi [handleSRC] viene pushato in [handleDST]
                 */
        bool                stagingBuffer_uploadToGPUBuffer (const GPUStgBufferHandle handleSRC, const void *dataSRC, const GPUVtxBufferHandle handleDST, u32 offsetDST, u32 howManyByteToCopy);
        bool                stagingBuffer_uploadToGPUBuffer (const GPUStgBufferHandle handleSRC, const void *dataSRC, const GPUIdxBufferHandle handleDST, u32 offsetDST, u32 howManyByteToCopy);
        

        //================ buffer unmapping / manualSync
        void                buffer_unmap (gpu::sMappedBuffer &m);
        void                buffer_manualSync_cpuWrite (const gpu::sMappedBuffer *list, u32 numElemInList);
        void                buffer_manualSync_cpuRead (const gpu::sMappedBuffer *list, u32 numElemInList);

        //================ image unmapping / manualSync
        void                image_unmap (gpu::sMappedImage &m);
        void                image_manualSync_cpuRead (const gpu::sMappedImage *list, u32 numElemInList);

        //================ vertex buffer
        bool                vertexBuffer_create (u32 sizeInByte, eMemAccessMode mode, GPUVtxBufferHandle *out_handle);
        void                deleteResource (GPUVtxBufferHandle &handle)                                                             { priv_bufferDestroy (vtxBufferList, handle); }
        const gpu::Buffer*  getInfo (const GPUVtxBufferHandle handle) const;
        bool                writeAndSync (const GPUVtxBufferHandle handle, u32 offsetDST, const void *src, u32 sizeInByte) const    { return priv_bufferWriteAndSync (vtxBufferList, handle, offsetDST, src, sizeInByte); }
        bool                map (const GPUVtxBufferHandle handle, u32 offsetDST, u32 sizeInByte, gpu::sMappedBuffer *out) const     { return priv_bufferMap (vtxBufferList, handle, offsetDST, sizeInByte, out); }
        
        //================ index buffer
        bool                indexBuffer_create (u32 sizeInByte, eMemAccessMode mode, GPUIdxBufferHandle *out_handle);
        void                deleteResource (GPUIdxBufferHandle &handle)                                                             { priv_bufferDestroy (idxBufferList, handle); }
        const gpu::Buffer*  getInfo (const GPUIdxBufferHandle handle) const;
        bool                writeAndSync (const GPUIdxBufferHandle handle, u32 offsetDST, const void *src, u32 sizeInByte) const    { return priv_bufferWriteAndSync (idxBufferList, handle, offsetDST, src, sizeInByte); }
        bool                map (const GPUIdxBufferHandle handle, u32 offsetDST, u32 sizeInByte, gpu::sMappedBuffer *out) const     { return priv_bufferMap (idxBufferList, handle, offsetDST, sizeInByte, out); }

        //================ uniform buffer
        bool                uniformBuffer_create (u32 sizeInByte, eMemAccessMode mode, GPUUniformBufferHandle *out_handle);
        void                deleteResource (GPUUniformBufferHandle &handle)                                                             { priv_bufferDestroy (uniformBufferList, handle); }
        const gpu::Buffer*  getInfo (const GPUUniformBufferHandle handle) const;
        bool                writeAndSync (const GPUUniformBufferHandle handle, u32 offsetDST, const void *src, u32 sizeInByte) const    { return priv_bufferWriteAndSync (uniformBufferList, handle, offsetDST, src, sizeInByte); }
        bool                map (const GPUUniformBufferHandle handle, u32 offsetDST, u32 sizeInByte, gpu::sMappedBuffer *out) const     { return priv_bufferMap (uniformBufferList, handle, offsetDST, sizeInByte, out); }

        //================ storage buffer
        bool                storageBuffer_create (u32 sizeInByte, eMemAccessMode mode, GPUStorageBufferHandle *out_handle);
        void                deleteResource (GPUStorageBufferHandle &handle)                                                             { priv_bufferDestroy (storageBufferList, handle); }
        const gpu::Buffer*  getInfo (const GPUStorageBufferHandle handle) const;
        bool                writeAndSync (const GPUStorageBufferHandle handle, u32 offsetDST, const void *src, u32 sizeInByte) const    { return priv_bufferWriteAndSync (storageBufferList, handle, offsetDST, src, sizeInByte); }
        bool                map (const GPUStorageBufferHandle handle, u32 offsetDST, u32 sizeInByte, gpu::sMappedBuffer *out) const     { return priv_bufferMap (storageBufferList, handle, offsetDST, sizeInByte, out); }

        //================ shader
        bool                vtxshader_createFromMemory (const void *buffer, u32 bufferSize, const char *mainFnName, GPUShaderHandle *out_shaderHandle)      { return priv_shader_createFromMemory (buffer, bufferSize, eShaderType::vtxShader, mainFnName, out_shaderHandle); }
        bool                vtxshader_createFromFile (const char *filename, const char *mainFnName, GPUShaderHandle *out_shaderHandle)                      { return priv_shader_createFromFile (filename, eShaderType::vtxShader, mainFnName, out_shaderHandle); }
        
        bool                pxlshader_createFromMemory (const void *buffer, u32 bufferSize, const char *mainFnName, GPUShaderHandle *out_shaderHandle)      { return priv_shader_createFromMemory (buffer, bufferSize, eShaderType::pxlShader, mainFnName, out_shaderHandle); }
        bool                pxlshader_createFromFile (const char *filename, const char *mainFnName, GPUShaderHandle *out_shaderHandle)                      { return priv_shader_createFromFile (filename, eShaderType::pxlShader, mainFnName, out_shaderHandle); }
            
        const gpu::Shader*  getInfo (const GPUShaderHandle handle) const;
        void                deleteResource (GPUShaderHandle &shaderHandle);

        //================ texture
		bool                texture_create2D (u16 dimx, u16 dimy, u8 nMipMap, eImageFormat fmt, eMemAccessMode memAccessMode, const void *srcDATA, GPUTextureHandle *out_handle);
        bool                texture_create2D (const gos::Image *im, u8 srcTextureNum, eMemAccessMode memAccessMode, GPUTextureHandle *out_handle);
        void                deleteResource (GPUTextureHandle &handle);
        const gpu::Texture* getInfo (const GPUTextureHandle handle) const;
        bool                toVulkan (const GPUTextureHandle handle, VkImageView *out) const;

        //================ sampler
        bool                sampler_create (const gpu::SamplerDesc &desc, GPUSamplerHandle *out_handle);
        const gpu::Sampler* getInfo (const GPUSamplerHandle handle) const;
        //void                deleteResource (GPUSamplerHandle &handle);
        //                      delete resource NON esiste perche' i Sampler sono mantenuti per sempre da GPU e sharati nel caso
        //                      in cui si richiedano N sampler con le stesse caratteristiche


    public:
        VulkanDevice*       _getVulkanDevice()                                  { return &vulkan; }


    public:
        void                _internal__onWindowResized (int w, int h);

    private:
        struct sWindow
        {
        public:
            sWindow()                                                   { winHandle.setInvalid(); storedX = storedY = storedW = storedH = 0; }

            void            getCurrentSize (int *out_w, int *out_h)     { input::window_getSize (winHandle, out_w, out_h); }

            void            storeCurrentPosAndSize()
                            {
                                input::window_getPos (winHandle, &storedX, &storedY);
                                input::window_getSize (winHandle, &storedW, &storedH);
                            }

            GLFWwindow*     getGLF() const
                            {
                                GLFWwindow *glfWin;
                                input::window_getGLF (winHandle, &glfWin);
                                return glfWin;
                            }

            bool            isValid() const                             { return winHandle.isValid(); }

        public:
            GOSWinHandle winHandle;
            int storedX;
            int storedY;
            int storedW;
            int storedH;
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
            void                            setup (GPU *gpuIN, gos::eGPUQueueFamily queueTypeIN);
            void                            unsetup ();
            gpu::pipe2::CmdBufferWriter2*   begin();
            void                            end();

            VkCommandBuffer                 getVulkanCmdBufferHandle() const;

        private:
            gos::GPU                        *gpu;
            gos::eGPUQueueFamily              queueType;
            GPUCmdBufferHandle              handle_cmdBuffer;
            gpu::pipe2::CmdBufferWriter2    cw;
            
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
        bool                priv_shader_createFromMemory (const void *buffer, u32 bufferSize, eShaderType shaderType, const char *mainFnName, GPUShaderHandle *out_shaderHandle);


        bool                priv_depthStencil_createFromStruct (gos::gpu::DepthStencil &depthStencil);
        void                priv_depthStencil_deleteFromStruct (gos::gpu::DepthStencil &depthStencil);

        bool                priv_renderTarget_createFromStruct (gos::gpu::RenderTarget &rt);
        void                priv_renderTarget_deleteFromStruct (gos::gpu::RenderTarget &rt);

        bool                priv_descrPool_onBuilderEnds (DescriptorPoolBuilder *builder);

        void                priv_samplerDelete (GPUSamplerHandle &handle);

        void                priv_createHelperStagingBuffer (u32 size);
        bool                immediateTransferCmd_begin();
        bool                immediateTransferCmd_end();

        bool                priv_bufferCreate (VkBufferUsageFlags vkUsage, u32 sizeInByte, bool bCanBeUsedBy_gfxQ, bool bCanBeUsedBy_computeQ, bool bCanBeUsedBy_transferQ, eMemAccessMode mode, gpu::Buffer *out);
        bool                priv_bufferMap (const GPUVtxBufferHandle handle, u32 offsetDST, u32 sizeInByte, void **out) const;

                            template<class THandleList, class THandle>
        void                priv_bufferDestroy (THandleList &list, THandle &handle)
                            {
                                gpu::Buffer *s;
                                if (list.fromHandleToPointer (handle, &s))
                                {
                                    vulkan.buffer_delete (s->vkHandle, s->_vkMemHandle, s->memoryAllocated);
                                    s->reset();
                                    list.release (handle);
                                }
                                handle.setInvalid();
                            }

                            
        /**
         * @brief valido solo per i buffer creati con eMemAccessMode::shared_cpuW_autoSync
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

                                if (eMemAccessMode::shared_cpuW_autoSync != s->mode)
                                {
                                    gos::logger::err ("GPU::priv_bufferWriteAndSync() => invalid buffer mode [%s]\n", gpu::enumToString(s->mode));
                                    return false;
                                }

                                //i buffer eMemAccessMode::shared_cpuW_autoSync sono sempre totalmente mappati all'atto della creazione
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

                                if (eMemAccessMode::shared_cpuW_manualSync != s->mode)
                                {
                                    gos::logger::err ("GPU::priv_bufferMap() => invalid buffer mode. Buffer mode must be [shared_cpuW_manualSync], current mode is %s\n", gpu::enumToString(s->mode));
                                    return false;
                                }

                                if (NULL != s->mapped_host_pt)
                                {
                                    gos::logger::err ("GPU::priv_bufferMap(d) => buffer is already mapped\n");
                                    return false;
                                }

                                const VkResult result = vulkan.memory_map (s->_vkMemHandle, offsetDST, sizeInByte, 0, &out->host_pt);
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


        bool                priv_descrSetLayout_build_v2 (const gpu::pipe2::Pipeline_def::DescriptorSet &ds, GPUDescrSetLayoutHandle *out_handle, VkDescriptorSetLayout *out_vkHandle);
        bool                priv_pipeline2_doCreate (const gpu::pipe2::Pipeline_def &rpd, gpu::Pipeline2 *out);

    private:
        gos::Allocator              *allocator;
        sWindow                     mainWindow;
        VkInstance                  vkInstance;
        VkSurfaceKHR                vkSurfaceKHR;
        VkDebugUtilsMessengerEXT    vkDebugMessenger;
        VulkanDevice                vulkan;
        VkSurfaceCapabilitiesKHR    vkSurfCapabilities;
        sPhyDeviceInfo              physicalDevInfo;

        sSwapChainInfo              swapchain;
        u32                         currentSwapChainImageIndex;
        u64                         timeToRecreateSwapchain_msec;
        bool                        vSync;
        bool                        bSwapChainRecreatedDuringThisFrame;
        u32                         swapchainAutoID;
        
        ToBeDeletedBuilder          toBeDeletedBuilder;

        GPUViewportHandle           defaultViewportHandle;
        eImageFormat                zbuffer_bestFmt_noStencil;
        eImageFormat                zbuffer_bestFmt_withStencil;

        ImmediateTransferCmd        helperImmediateTransferCmd;
        GPUStgBufferHandle          helperStagingBuffer;

        HandleList<GPUShaderHandle, gpu::Shader>                    shaderList;
        HandleList<GPUViewportHandle, gpu::Viewport>                viewportlList;
        gos::FastArray<GPUViewportHandle>                           viewportHandleList;
        HandleList<GPUZBufferHandle, gpu::DepthStencil>        depthStencilList;
        gos::FastArray<GPUZBufferHandle>                       depthStencilHandleList;
        HandleList<GPURenderTargetHandle, gpu::RenderTarget>        renderTargetList;
        gos::FastArray<GPURenderTargetHandle>                       renderTargetHandleList;

        HandleList<GPUPipelineHandle,gpu::Pipeline2>                pipelineList;
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
