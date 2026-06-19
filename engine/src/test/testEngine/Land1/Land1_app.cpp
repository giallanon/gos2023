#include "Land1_app.h"

using namespace gos;



//***************************************
Land1_app::Land1_app()
{
}

//***************************************
Land1_app::~Land1_app()
{
}

//***************************************
void Land1_app::on__handle_input ()
{
}

//***************************************
void Land1_app::on__load_assets ()
{
	land.setup (gos::getSysHeapAllocator(), engine);
}

//***************************************
void Land1_app::on__cleanup()
{
	land.unsetup();
}

//***************************************
void Land1_app::on__render (gos::gpu::SwapchainImg swapchainImg, GPUCmdBufferHandle cmdBufferHandle, gos::geom::Camera3 *cam)
{
	if (last_cam_pos != cam->pos.o)
	{
		last_cam_pos = cam->pos.o;

		logger::log (eTextColor::white, "CAM: %.2f, %.2f, %.2f\n", last_cam_pos.x, last_cam_pos.y, last_cam_pos.z);
	}
	land.render (swapchainImg, cmdBufferHandle, cam);
}

