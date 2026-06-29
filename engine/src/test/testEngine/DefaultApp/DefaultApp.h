#ifndef _DefaultApp_h_
#define _DefaultApp_h_

#include "gosEngine.h"
#include "renderPipe/gosEngineRenderPipe_PIPE3.h"
#include "entity/gosEntity.h"
#include "model/gosModel.h"
#include "../gosGPU/utils/gosFreeMovement.h"
#include "../gosGPU/utils/gosFPSMovement.h"

class DefaultApp
{
public:
					DefaultApp();
	virtual			~DefaultApp();

	void			run (gos::Engine *engine);

protected:
	enum class eCameraMode : u8
	{
		move_free = 0,
		move_fps = 1,
	};

protected:
	virtual void	on__setup() = 0;
	virtual void	on__handle_input (const gos::Engine::InputEvent &ev) = 0;
	virtual void	on__prepare_render() = 0;
	virtual void	on__unsetup() = 0;

protected:
	gos::Allocator					*allocator;
	gos::Engine						*engine;
	gos::GPU						*gpu;
    gos::geom::Camera3				cam;

	eCameraMode						camera_mode;
	u8 								bShowCamPos;
    gos::FPSMovement				move_fps;
	gos::FreeMovement				move_free;

private:
	const char*		enum_to_string (eCameraMode m) const;
	void			default_handle_input ();
    void    		priv_loop();

};

#endif //_DefaultApp_h_