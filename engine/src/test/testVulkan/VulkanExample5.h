#ifndef _VulkanExample5_h_
#define _VulkanExample5_h_
#include "VulkanApp.h"


/************************************
 *  VulkanExample5
 */
class VulkanExample5 : public VulkanApp
{
public:
    
                VulkanExample5();

    bool        virtual_onInit ();
    void        virtual_explain();
    void        virtual_onRun();
    void        virtual_onCleanup();

private:

};


#endif //_VulkanExample5_h_