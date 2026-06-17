#ifndef _DefaultApp_h_
#define _DefaultApp_h_

#include "gosEngine.h"
#include "gosEngine_renderer1.h"
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
	virtual void	on__load_assets() = 0;
	virtual void	on__handle_input () = 0;
	virtual void	on__render() = 0;
	virtual void	on__cleanup() = 0;

protected:
	gos::Allocator					*allocator;
	gos::Engine						*engine;
	gos::GPU						*gpu;
    gos::geom::Camera3				cam;
	gos::engine::Renderer1			*renderer;

	eCameraMode						camera_mode;
    gos::FPSMovement				move_fps;
	gos::FreeMovement				move_free;

	gos::ENGTexture					handle_texBianca;
	u32								default_material_indices[4];


private:
	const char*		enum_to_string (eCameraMode m) const;
	void			default_handle_input ();
	bool			default_load_material();
    void    		priv_loop();
};

#endif //_DefaultApp_h_