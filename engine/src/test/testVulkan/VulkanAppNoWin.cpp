#include "VulkanAppNoWin.h"

using namespace gos;

//************************************
VulkanAppNoWin::VulkanAppNoWin()
{ 
    gpu = NULL; 
}

//************************************
bool VulkanAppNoWin::init (gos::GPU *gpuIN)
{
    gpu = gpuIN;

    if (!virtual_onInit())
        return false;
    return true;
}    

