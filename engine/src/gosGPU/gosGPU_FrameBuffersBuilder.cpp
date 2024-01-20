#include "gosGPU.h"

using namespace gos;


typedef GPU::FrameBuffersBuilder  GPUFBB;   //di comodo

//*********************************************** 
GPU::FrameBuffersBuilder::FrameBuffersBuilder (GPU *gpuIN, const GPURenderLayoutHandle &renderLayoutHandleIN, GPUFrameBufferHandle *out_handleIN) :
    GPU::TempBuilder(gpuIN)
{
    renderLayoutHandle = renderLayoutHandleIN;
    out_handle = out_handleIN;

    bAnyError = false;
    numRenderTarget = 0;
    depthStencilHandle.setInvalid();
    width.setFromString("0-");
    height.setFromString("0-");
}

//*********************************************** 
GPU::FrameBuffersBuilder::~FrameBuffersBuilder()
{
}

//*********************************************** 
GPUFBB& GPU::FrameBuffersBuilder::setRenderAreaSize (const gos::Dim2D &w, const gos::Dim2D &h)
{
    width = w;
    height = h;
    return *this;
}

//*********************************************** 
GPUFBB& GPU::FrameBuffersBuilder::bindRenderTarget (const GPURenderTargetHandle &handle)
{
    if (numRenderTarget >= GOSGPU__NUM_MAX_ATTACHMENT)
    {
        bAnyError = true;
        gos::logger::err ("GPU::FrameBuffersBuilder::bindRenderTarget() => too many RenderTarget. Max is %d\n", GOSGPU__NUM_MAX_ATTACHMENT);
    }
    else
    {
        //verifichiamo di non stare ripedente uno stesso RT
        for (u32 i=0; i<numRenderTarget; i++)
        {
            if (renderTargetHandleList[i] == handle)
            {
                bAnyError = true;
                gos::logger::err ("GPU::FrameBuffersBuilder::bindRenderTarget() => render target already exist at index %d\n", i);
            }
        }

        if (!bAnyError)
            renderTargetHandleList[numRenderTarget++] = handle;
    }

    return *this;
}

//*********************************************** 
GPUFBB& GPU::FrameBuffersBuilder::bindDepthStencil (const GPUDepthStencilHandle &handle)
{
    if (depthStencilHandle.isInvalid())
        depthStencilHandle = handle;
    else
    {
        bAnyError = true;
        gos::logger::err ("GPU::FrameBuffersBuilder::bindDepthStencil() => depth stencil was already bound\n");
    }

    return *this;
}

//*********************************************** 
bool GPU::FrameBuffersBuilder::end()
{
    return gpu->priv_frameBuffer_onBuilderEnds (this);
}


