#ifndef _ThePipeline_h_
#define _ThePipeline_h_
#include "ThePipelineEnumAndDefine.h"
#include "DynamicTextureArray.h"
#include "VBIBSTBuffer.h"

/**
 * @brief ThePipeline
 *  
 */
class ThePipeline
{
private:
    typedef gos::AllocatorHeap<gos::AllocPolicy_Track_simple, gos::AllocPolicy_Thread_Unsafe>		LocalAllocator;

public:
            ThePipeline();
            ~ThePipeline();

    bool    setup (gos::GPU *gpu);
    void    unsetup();

    bool    createDescriptorInstance (const GPUDescrSetLayoutHandle &layout, GPUDescrSetInstanceHandle *out_instance);
    bool    createDescriptorInstance (tpp::sDescriptor *in_out)                                                             { return createDescriptorInstance(in_out->layout, &in_out->instance); }

    /**
     * @brief il descriptor0 (aka descriptroBase) contiene tutti i sampler e tutte le texture
     * Per aggiungere una texture usare addTextureIfNotExitst()
     * I sampler invece vengono creati da this e non sono modificabili
     */
    const tpp::sDescriptor*     descriptorBase_get() const                                                   { return &descriptorBase; }
    bool                        decriptorBase_addTextureIfNotExitst (const GPUTextureHandle &hTexture, u16 *out_index);

    /**
     * @brief il descriptor1 (aka descriptroScene) contiene le info globali di scena quali
     * la matrice della camera e la direzione della luce del sole
     */
    const tpp::sDescriptor*     descriptorScene_get() const                                                   { return &descriptorScene.descr; }
    void                        descritproScene_update (gos::geom::Camera3 *cam);


    /**
     * @brief mette la <shape> in un VB/IB e ritorna le info sul binding
     */
    bool    shape_uploadToVBIB (const gos::Shape *shape, tpp::sBoundShapeInfo *out_info);

public:
    //vertex declaration
    //Tutti i renderer di questa pipe hanno lo stesso vtx format
    struct sVertex
    {
        gos::vec3f  pos;
        gos::vec3f  norm;
        gos::vec2f  tutv0;
    };

    GPUVtxDeclHandle            vtxDeclHandle;
    gos::VtxLayout              vtxLayout;

public:
    gos::GPU                    *gpu;
    LocalAllocator              *localAllocator;
    GPURenderLayoutHandle       hRenderLayoutClearBuffer;
    GPURenderLayoutHandle       hRenderLayout;
    GPUFrameBufferHandle        hFrameBuffer;


private:
    static constexpr u32        NUM_MAX_TEXTURE                         = 1024;

private:
    struct sSceneData
    {
        gos::mat4x4f    camVP;
        gos::vec4f      lightDir;
    };

    struct DescrScene
    {
        tpp::sDescriptor            descr;
        GPUUniformBufferHandle      uboHandle;
        sSceneData                  sceneData;
    };

private:
    bool    priv_setupVertexDecl();
    bool    priv_createDescriptorBase();
    bool    priv_createDescriptorScene();

private:
    GPUDescrPoolHandle          hDescrPool;

    //descriptor set 0: texture & sample
    tpp::sDescriptor            descriptorBase;
    DynamicTextureArray         textureList;
    GPUSamplerHandle            hSampler0_bilinearFiltering;
    GPUSamplerHandle            hSampler1_pointFiltering;

    //descriptor set 1: scene data
    DescrScene                  descriptorScene;

    //gestore di VB/IB
    VBIBSTBuffer                vbibstBuffer;

};



#endif //_ThePipeline_h_
