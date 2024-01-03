#ifndef _gosGPU_h_
#define _gosGPU_h_
#include "gosGPUEnumAndDefine.h"
#include "gosGPUVtxDecl.h"
#include "gosGPUShader.h"
#include "gosGPUPipeline.h"
#include "gosGPUViewport.h"
#include "gosGPUDepthStencil.h"
#include "../gos/gos.h"


namespace gos
{
    /***************************************************
     * GPU
     */
    class GPU
    {
    public:
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
    
    public:
                            GPU();
                            ~GPU();


        bool                init (u16 width, u16 height, const char *appName);
        void                deinit();

        //================ window stuff
        GLFWwindow*         getWindow()                                     { return window.win; }
        void                toggleFullscreen();

        //================ swap chain info
        //La swap chain viene creata automaticamente da GPU::init()
        //In caso di resize della main window, e' necessario chiamare swapChain_recreate() per aggiornare i buffer
        bool                swapChain_recreate ();
        u32                 swapChain_getWidth() const                      { return vulkan.swapChainInfo.imageExtent.width; }
        u32                 swapChain_getHeight() const                     { return vulkan.swapChainInfo.imageExtent.height; }
        VkFormat            swapChain_getImageFormat() const                { return vulkan.swapChainInfo.imageFormat; }
        u8                  swapChain_getImageCount() const                 { return static_cast<u8>(vulkan.swapChainInfo.imageCount); }
        VkImageView         swapChain_getImageViewHandle(u8 i) const        { assert(i < swapChain_getImageCount()); return vulkan.swapChainInfo.vkImageView[i]; }
        VkExtent2D          swapChain_getImageExten2D() const               { return vulkan.swapChainInfo.imageExtent; }
        VkResult            swapChain_acquireNextImage (u32 *out_imageIndex, u64 timeout_ns=UINT64_MAX, VkSemaphore semaphore=VK_NULL_HANDLE, VkFence fence=VK_NULL_HANDLE);
        VkResult            swapChain_present (const VkSemaphore *semaphoreHandleList, u32 semaphoreCount, u32 imageIndex);

        //================ viewport
        //E' possibile creare tante viewport
        //La viewport di default (che matcha la risoluzione della swapchain), viene creata in automatico da GUPU::init() ed e' sempre
        //accessibile tramite viewport_getDefault()
        //Le viewport vengono automaticamente ridimensionate a seguito di un swapChain_recreate()
        bool                    viewport_create (const gos::Pos2D &x,const gos::Pos2D &y, const gos::Dim2D &w, const gos::Dim2D &h, GPUViewportHandle *out_handle);
        const gpu::Viewport*    viewport_get (const GPUViewportHandle &handle) const;
        void                    viewport_delete (GPUViewportHandle &handle);

        /* ritorna la viewport di default che e' sempre garantito essere aggiornata alle attuali dimensioni della main window */
        const gpu::Viewport*    viewport_getDefault () const                { return viewport_get(defaultViewportHandle); }

        //================ oggetti di sincronizzazione 
        void                waitIdle();
        bool                semaphore_create  (VkSemaphore *out);
        void                semaphore_destroy  (VkSemaphore &in);
        
        bool                fence_create  (bool bStartAsSignaled, VkFence *out);
        void                fence_destroy  (VkFence &in);
        VkResult            fence_wait (const VkFence *fenceHandleList, u32 fenceCount=1, u64 timeout_ns = UINT64_MAX);
        void                fence_reset (const VkFence *fenceHandleList, u32 fenceCount=1);
        
        //================ vtx declaration
        VtxDeclBuilder&     vtxDecl_createNew (GPUVtxDeclHandle *out_handle);
        void                vtxDecl_delete (GPUVtxDeclHandle &handle);
        bool                vtxDecl_query (const GPUVtxDeclHandle handle, gpu::VtxDecl *out) const;

        //================ depth buffer
        bool                depthBuffer_create (const gos::Dim2D &w, const gos::Dim2D &h, bool bWithStencil, GPUDepthStencilHandle *out_handle);
        void                depthBuffer_delete (GPUDepthStencilHandle &handle);

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
        void                shader_delete (GPUShaderHandle &shaderHandle);

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

    private:
        bool                priv_initWindowSystem(u16 width, u16 height, const char *appName);
        void                priv_deinitWindowSystem();
        
        bool                priv_initVulkan ();
        void                priv_deinitVulkan();

        bool                priv_initHandleLists();
        void                priv_deinitandleLists();

        void                priv_vulkanAddDebugCallback();
        bool                priv_shader_createFromFile (const u8 *filename, eShaderType shaderType, const char *mainFnName, GPUShaderHandle *out_shaderHandle);
        bool                priv_shader_createFromMemory (const u8 *buffer, u32 bufferSize, eShaderType shaderType, const char *mainFnName, GPUShaderHandle *out_shaderHandle);
        bool                priv_shader_fromHandleToPointer (const GPUShaderHandle shaderHandle, gpu::Shader **out) const;

        bool                priv_vxtDecl_fromHandleToPointer (const GPUVtxDeclHandle handle, gpu::VtxDecl **out) const;
        void                priv_vxtDecl_onBuilderEnds(VtxDeclBuilder *builder);

        bool                priv_depthStenicl_createFromStruct (gos::gpu::DepthStencil &depthStencil);
        void                priv_depthStenicl_deleteFromStruct (gos::gpu::DepthStencil &depthStencil);

    private:
        gos::Allocator              *allocator;
        sWindow                     window;
        VkInstance                  vkInstance;
        VkSurfaceKHR                vkSurface;
        VkDebugUtilsMessengerEXT    vkDebugMessenger;
        sVkDevice                   vulkan;
        VkSurfaceCapabilitiesKHR    vkSurfCapabilities;
        VtxDeclBuilder              vtxDeclBuilder;

        GPUViewportHandle           defaultViewportHandle;

        HandleList<GPUShaderHandle, gpu::Shader>                    shaderList;
        HandleList<GPUVtxDeclHandle, gpu::VtxDecl>                  vtxDeclList;
        HandleList<GPUViewportHandle, gpu::Viewport>                viewportlList;
        gos::FastArray<GPUViewportHandle>                           viewportHandleList;
        HandleList<GPUDepthStencilHandle, gos::gpu::DepthStencil>   depthStencilList;
        gos::FastArray<GPUDepthStencilHandle>                       depthStencilHandleList;
    };
} //namespace gos


#endif //_gosGPU_h_
