#include "gosGPU.h"

using namespace gos;

//************************************************************
GPU::VtxDeclBuilder& GPU::VtxDeclBuilder::addStream (eVtxStreamInputRate inputRate)
{
    if (!priv_isValid())
        return (*this);
    if (numStreamIndex >= GOSGPU__NUM_MAX_VXTDECL_STREAM)
        priv_markAsInvalid();
    else
        inputRatePerStream[numStreamIndex++] = inputRate;
    return (*this);
}

//************************************************************
GPU::VtxDeclBuilder& GPU::VtxDeclBuilder::addDescriptor (u32 offset, u8 bindingLocation, eDataFormat dataFormat)
{
    assert (offset <= 0xFF);
    if (!priv_isValid())
        return (*this);

    const u8 streamIndex = numStreamIndex-1;
    if (streamIndex >= numStreamIndex)
    {
        priv_markAsInvalid();
        return (*this);
    }

    if (numAttributeDesc >= GOSGPU__NUM_MAX_VTXDECL_ATTR)
    {
        priv_markAsInvalid();
        return (*this);
    }

    //verifico che questa dichiarazione sia valida rispetto alla dichiarazione precedente
    const u8 i = numAttributeDesc++;
    if (i > 0)
    {
        const u8 iPrev = i -1;
        if (attributeDesc[iPrev].streamIndex == streamIndex)
        {
            const u32 nextValidOffset = attributeDesc[iPrev].offset + gos::utils::getFormatSize(attributeDesc[iPrev].format);
            if (offset < nextValidOffset)
            {
                priv_markAsInvalid();
                return (*this);
            }
            if (bindingLocation <= attributeDesc[iPrev].bindingLocation)
            {
                priv_markAsInvalid();
                return (*this);
            }                            
        }
    }

    attributeDesc[i].streamIndex = streamIndex;
    attributeDesc[i].bindingLocation = bindingLocation;
    attributeDesc[i].format = dataFormat;
    attributeDesc[i].offset = static_cast<u8>(offset);                        
    return (*this);
}

//************************************************************
void GPU::VtxDeclBuilder::end()
{ 
    gpu->priv_vxtDecl_onBuilderEnds (this); 
}
