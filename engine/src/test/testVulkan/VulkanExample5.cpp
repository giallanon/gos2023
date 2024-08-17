#include "VulkanExample5.h"
#include "../gosGeom/gosGeomCamera3.h"


using namespace gos;


//************************************
VulkanExample5::VulkanExample5()
{
}

//************************************
void VulkanExample5::virtual_explain()
{
    gos::logger::log ("Esperimenti con griglia di vtx da morfare\n");
}


//************************************
void VulkanExample5::virtual_onCleanup() 
{
}    

//************************************
bool VulkanExample5::virtual_onInit ()
{
    return false;
}    






/************************************
 * renderizza inviando command buffer a GPU e poi aspettando che questa
 * abbia finito il suo lavoro
 */
void VulkanExample5::virtual_onRun()
{
    while (bQuitApp == false)
    {
        handleInput();
    }
}

