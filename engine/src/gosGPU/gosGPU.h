#ifndef _gosGPU_h_
#define _gosGPU_h_
#include "gosGPUEnumAndDefine.h"
#include "../gos/gos.h"
#include "../gos/gosFastArray.h"

#include "gosGPUResDepthStencil.h"
#include "gosGPUResRenderTarget.h"
#include "gosGPUResShader.h"
#include "gosGPUResViewport.h"
#include "gosGPUResVtxDecl.h"

namespace gos
{
    /***************************************************
     * GPU
     */
    class GPU
    {
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

            /* [offset] => offset all'interno della struttra del vtx
               [bindingLocation] => bingind all'interdno del vtx shader
               [dataFormat] => data format all'interno del vtx shader */
            VtxDeclBuilder&         addDescriptor (u32 offset, u8 bindingLocation, eDataFormat dataFormat);                                    

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
            sVtxDescriptor          attributeDesc[GOSGPU__NUM_MAX_VTXDECL_ATTR];

        friend class GPU;
        }; //class VtxDeclBuilder     



        /**********************************************
         * RenderTaskLayoutBuilder
         * 
         * Classe di comodo usata da GPU per la creazione dei RenderTaskLayout
         * E' l'equivalente di un RenderPass di Vulkan
         */
        class RenderTaskLayoutBuilder : public TempBuilder
        {
        private:
            static const u8         NUM_MAX_RENDER_TARGET = 8;
            static const u8         NUM_MAX_SUBPASS = 8;

            typedef RenderTaskLayoutBuilder RTLB;   //di comodo

        public:
            /**********************************************
             * Subpass
            */
            class SubPassInfo
            {
            public:
                                SubPassInfo ()                                          { owner = NULL; }

                SubPassInfo&    useRenderTarget (u8 index);
                SubPassInfo&    useDepthStencil ()                                      { bUseDepthStencil = true; return *this; }

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
                u8      renderTargetIndexList[NUM_MAX_RENDER_TARGET];

            friend class RenderTaskLayoutBuilder;
            }; //class SubPassInfo


        public:
                            RenderTaskLayoutBuilder (GPU *gpuIN, GPURenderLayoutHandle *out_handle);
            virtual         ~RenderTaskLayoutBuilder();

            RTLB&           requireRendertarget (eRenderTargetUsage usage, VkFormat imageFormat, bool bClear, const gos::ColorHDR &clearColor = gos::ColorHDR(1.0f, 1, 1, 1) );
            RTLB&           requireDepthStencil (bool bWithStencil, bool bClear, f32 clearValue=1.0f);

            SubPassInfo&    addSubpass_GFX ();
            SubPassInfo&    addSubpass_COMPUTE ();
            bool            end();
            
            bool            anyError() const                            { return bAnyError; }

        private:
            struct sRenderTargetInfo
            {
                eRenderTargetUsage  usage;
                VkFormat            imageFormat;
                bool                bClear;
                gos::ColorHDR       clearColor;
            };

            struct sDepthBufferInfo
            {
                void    reset()                 { isRequired = false; bWithStencil=false; bClear=false; clearValue=1.0f; }

                bool    isRequired;
                bool    bWithStencil;
                bool    bClear;
                f32     clearValue;
            };

        private:
            bool                    priv_buildVulkan();

        private:
            GPURenderLayoutHandle   *out_handle;

            bool                    bAnyError;
            u8                      numRenderTargetInfo;
            sRenderTargetInfo       rtInfoList[NUM_MAX_RENDER_TARGET];
            sDepthBufferInfo        depthBuffer;
            u8                      numSubpassInfo;
            SubPassInfo             subpassInfoList[NUM_MAX_SUBPASS];

            VkRenderPass            vkRenderPassHandle;

        friend class GPU;
        }; // class RenderTaskLayoutBuilder
    
    
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
                                PipelineBuilder (GPU *gpu, const GPURenderLayoutHandle &renderLayoutHandle, GPUPipelineHandle *out_handle);
            virtual             ~PipelineBuilder();

            void                cleanUp();

            PipelineBuilder&    addShader (const GPUShaderHandle handle)                { shaderList.append (handle); return *this; }
            PipelineBuilder&    setDrawPrimitive (eDrawPrimitive p)                     { drawPrimitive=p; return *this; }
            PipelineBuilder&    setVtxDecl (const GPUVtxDeclHandle handle)              { vtxDeclHandle = handle; return *this; }
            DepthStencilParam&  depthStencil()                                          { return depthStencilParam; }
			PipelineBuilder&    setCullMode (eCullMode m)							    { cullMode = m; return *this; }
			PipelineBuilder&    setWireframe (bool b)								    { bWireframe = b; return *this; }
            bool                end ();

            bool                anyError() const                                        { return bAnyError; }

        private:
            bool                priv_buildVulkan ();
            
        private:
            bool                                bAnyError;
            gos::Allocator                      *allocator;
            gos::FastArray<GPUShaderHandle>     shaderList;
            eDrawPrimitive                      drawPrimitive;
            GPUVtxDeclHandle                    vtxDeclHandle;
            DepthStencilParam                   depthStencilParam;
            eCullMode                           cullMode;
            bool                                bWireframe;

            GPUPipelineHandle       *out_handle;
            GPURenderLayoutHandle   renderLayoutHandle;
            VkPipelineLayout        vkPipelineLayoutHandle;
            VkPipeline              vkPipelineHandle;

        friend class GPU;
        }; //PipelineBuilder



    public:
                            GPU();
                            ~GPU();


        bool                init (u16 width, u16 height, const char *appName);
        void                deinit();

        //================ window stuff
        GLFWwindow*         getWindow()                                     { return window.win; }
        void                toggleFullscreen();

        //================ rendering & presentazione
        bool                newFrame (bool *out_bSwapchainWasRecreated, u32 *out_imageIndex, u64 timeout_ns=UINT64_MAX, VkSemaphore semaphore=VK_NULL_HANDLE, VkFence fence=VK_NULL_HANDLE);
        VkResult            present (const VkSemaphore *semaphoreHandleList, u32 semaphoreCount, u32 imageIndex);

        //================ swap chain info
        //La swap chain viene creata automaticamente da GPU::init()
        //In caso di resize della main window, e' necessario chiamare swapChain_recreate() per aggiornare i buffer
        u32                 swapChain_getWidth() const                      { return vulkan.swapChainInfo.imageExtent.width; }
        u32                 swapChain_getHeight() const                     { return vulkan.swapChainInfo.imageExtent.height; }
        VkFormat            swapChain_getImageFormat() const                { return vulkan.swapChainInfo.imageFormat; }
        u8                  swapChain_getImageCount() const                 { return static_cast<u8>(vulkan.swapChainInfo.imageCount); }
        VkImageView         swapChain_getImageViewHandle(u8 i) const        { assert(i < swapChain_getImageCount()); return vulkan.swapChainInfo.vkImageListView[i]; }
        VkExtent2D          swapChain_getImageExten2D() const               { return vulkan.swapChainInfo.imageExtent; }


        //================ oggetti di sincronizzazione 
        void                waitIdle();
        bool                semaphore_create  (VkSemaphore *out);
        void                semaphore_destroy  (VkSemaphore &in);
        
        bool                fence_create  (bool bStartAsSignaled, VkFence *out);
        void                fence_destroy  (VkFence &in);
        VkResult            fence_wait (const VkFence *fenceHandleList, u32 fenceCount=1, u64 timeout_ns = UINT64_MAX);
        void                fence_reset (const VkFence *fenceHandleList, u32 fenceCount=1);


        //================ viewport
        //E' possibile creare tante viewport
        //La viewport di default (che matcha la risoluzione della swapchain), viene creata in automatico da GUPU::init() ed e' sempre
        //accessibile tramite viewport_getDefault()
        //Le viewport vengono automaticamente ridimensionate a seguito di un swapChain_recreate()
        bool                    viewport_create (const gos::Pos2D &x,const gos::Pos2D &y, const gos::Dim2D &w, const gos::Dim2D &h, GPUViewportHandle *out_handle);
        const gpu::Viewport*    viewport_get (const GPUViewportHandle &handle) const;
        void                    deleteResource (GPUViewportHandle &handle);

        /* ritorna la viewport di default che e' sempre garantito essere aggiornata alle attuali dimensioni della main window */
        const gpu::Viewport*    viewport_getDefault () const                { return viewport_get(defaultViewportHandle); }

        
        //================ vtx declaration
        VtxDeclBuilder&     vtxDecl_createNew (GPUVtxDeclHandle *out_handle);
        void                deleteResource (GPUVtxDeclHandle &handle);
        bool                vtxDecl_query (const GPUVtxDeclHandle handle, gpu::VtxDecl *out) const;

        //================ render layout
        RenderTaskLayoutBuilder&    renderLayout_createNew (GPURenderLayoutHandle *out_handle);
        void                        deleteResource (GPURenderLayoutHandle &handle);
        bool                        renderLayout_toVulkan (const GPURenderLayoutHandle handle, VkRenderPass *out) const;

        //================ Pipeline
        PipelineBuilder&    pipeline_createNew (const GPURenderLayoutHandle &enderLayoutHandle, GPUPipelineHandle *out_handle);
        void                deleteResource (GPUPipelineHandle &handle);
        bool                pipeline_toVulkan (const GPUPipelineHandle handle, VkPipeline *out, VkPipelineLayout *out_layout) const;

        //================ depth buffer
        bool                depthBuffer_create (const gos::Dim2D &w, const gos::Dim2D &h, bool bWithStencil, GPUDepthStencilHandle *out_handle);
        void                deleteResource (GPUDepthStencilHandle &handle);

        //================ shader
        bool                vtxshader_createFromMemory (const u8 *buffer, u32 bufferSize, const char *mainFnName, GPUShaderHandle *out_shaderHandle)            { return priv_shader_createFromMemory (buffer, bufferSize, eShaderType::vertexShader, mainFnName, out_shaderHandle); }
        bool                vtxshader_createFromFile (const char *filename, const char *mainFnName, GPUShaderHandle *out_shaderHandle)                          { return priv_shader_createFromFile (reinterpret_cast<const u8*>(filename), eShaderType::vertexShader, mainFnName, out_shaderHandle); }
        bool                vtxshader_createFromFile (const u8 *filename, const char *mainFnName, GPUShaderHandle *out_shaderHandle)                            { return priv_shader_createFromFile (filename, eShaderType::vertexShader, mainFnName, out_shaderHandle); }
        
        bool                fragshader_createFromMemory (const u8 *buffer, u32 bufferSize, const char *mainFnName, GPUShaderHandle *out_shaderHandle)           { return priv_shader_createFromMemory (buffer, bufferSize, eShaderType::fragmentShader, mainFnName, out_shaderHandle); }
        bool                fragshader_createFromFile (const char *filename, const char *mainFnName, GPUShaderHandle *out_shaderHandle)                         { return priv_shader_createFromFile (reinterpret_cast<const u8*>(filename), eShaderType::fragmentShader, mainFnName, out_shaderHandle); }
        bool                fragshader_createFromFile (const u8 *filename, const char *mainFnName, GPUShaderHandle *out_shaderHandle)                           { return priv_shader_createFromFile (filename, eShaderType::fragmentShader, mainFnName, out_shaderHandle); }
        
        VkShaderModule      shader_getVkHandle (const GPUShaderHandle shaderHandle) const;
        const char*         shader_getMainFnName (const GPUShaderHandle shaderHandle) const;
        eShaderType         shader_getType (const GPUShaderHandle shaderHandle) const;
        void                deleteResource (GPUShaderHandle &shaderHandle);


        //================ command buffer
        bool                createCommandBuffer (VkCommandBuffer *out);


        //================ da rimuovere
        VkDevice           REMOVE_getVkDevice() const               { return vulkan.dev; }
        VkQueue            REMOVE_getGfxQHandle() const             { return vulkan.gfxQ; }
        VkQueue            REMOVE_getComputeQHandle() const         { return vulkan.computeQ; }

    private:
        struct sWindow
        {
        public:
            sWindow()                                                   { win = NULL; storedX = storedY = storedW = storedH = 0; }
            void getCurrentPos  (int *outX, int *outY)                  { glfwGetWindowPos (win, outX, outY); }
            void getCurrentSize (int *dimX, int *dimY)                  { glfwGetWindowSize (win, dimX, dimY); }
            void storeCurrentPosAndSize()                               { glfwGetWindowPos (win, &storedX, &storedY); glfwGetWindowSize (win, &storedW, &storedH); }

        public:
            GLFWwindow *win;
            int storedX;
            int storedY;
            int storedW;
            int storedH;
        };
        
        struct sRenderLayout
        {
            void    reset ()                        { vkRenderPassHandle = VK_NULL_HANDLE; }
            VkRenderPass    vkRenderPassHandle;
        };

        struct sPipeline
        {
            void    reset ()                        { vkPipelineLayoutHandle = VK_NULL_HANDLE; vkPipelineHandle = VK_NULL_HANDLE; }

            VkPipelineLayout    vkPipelineLayoutHandle;
            VkPipeline          vkPipelineHandle;

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

    private:
        bool                priv_initWindowSystem(u16 width, u16 height, const char *appName);
        void                priv_deinitWindowSystem();
        
        bool                priv_initVulkan ();
        void                priv_deinitVulkan();

        bool                priv_initHandleLists();
        void                priv_deinitandleLists();

        void                priv_vulkanAddDebugCallback();
        bool                priv_swapChain_recreate ();

        bool                priv_shader_createFromFile (const u8 *filename, eShaderType shaderType, const char *mainFnName, GPUShaderHandle *out_shaderHandle);
        bool                priv_shader_createFromMemory (const u8 *buffer, u32 bufferSize, eShaderType shaderType, const char *mainFnName, GPUShaderHandle *out_shaderHandle);
        bool                priv_shader_fromHandleToPointer (const GPUShaderHandle shaderHandle, gpu::Shader **out) const;

        bool                priv_vxtDecl_fromHandleToPointer (const GPUVtxDeclHandle handle, gpu::VtxDecl **out) const;
        void                priv_vxtDecl_onBuilderEnds(VtxDeclBuilder *builder);

        bool                priv_renderLayout_onBuilderEnds(RenderTaskLayoutBuilder *builder);
        bool                priv_pipeline_onBuilderEnds (PipelineBuilder *builder);

        bool                priv_depthStenicl_createFromStruct (gos::gpu::DepthStencil &depthStencil);
        void                priv_depthStenicl_deleteFromStruct (gos::gpu::DepthStencil &depthStencil);

        bool                priv_renderTarget_fromHandleToPointer (const GPURenderTargetHandle handle, gpu::RenderTarget **out) const;

    private:
        gos::Allocator              *allocator;
        sWindow                     window;
        VkInstance                  vkInstance;
        VkSurfaceKHR                vkSurface;
        VkDebugUtilsMessengerEXT    vkDebugMessenger;
        sVkDevice                   vulkan;
        VkSurfaceCapabilitiesKHR    vkSurfCapabilities;
        VtxDeclBuilder              vtxDeclBuilder;
        u32                         currentSwapChainImageIndex;
        bool                        bNeedToRecreateSwapchain;
        ToBeDeletedBuilder          toBeDeletedBuilder;

        GPUViewportHandle           defaultViewportHandle;
        GPURenderTargetHandle       *defaultRTHandleList;

        HandleList<GPUShaderHandle, gpu::Shader>                    shaderList;
        HandleList<GPUVtxDeclHandle, gpu::VtxDecl>                  vtxDeclList;
        HandleList<GPUViewportHandle, gpu::Viewport>                viewportlList;
        gos::FastArray<GPUViewportHandle>                           viewportHandleList;
        HandleList<GPUDepthStencilHandle, gpu::DepthStencil>        depthStencilList;
        gos::FastArray<GPUDepthStencilHandle>                       depthStencilHandleList;
        HandleList<GPURenderTargetHandle, gpu::RenderTarget>        renderTargetList;
        HandleList<GPURenderLayoutHandle,sRenderLayout>             renderLayoutList;
        HandleList<GPUPipelineHandle,sPipeline>                     pipelineList;
        
    };
} //namespace gos


#endif //_gosGPU_h_
