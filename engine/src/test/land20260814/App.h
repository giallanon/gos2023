#ifndef _App_h_
#define _App_h_
#include "defaultApp/gosDefaultApp.h"
#include "renderPipe/gosEngineRenderPipe_line3d.h"
#include "land/land.h"


/******************************************
* App
*
*/
class App : public gos::engine::DefaultApp
{
public:
			App();

protected:
	void	on__setup () final;
	void	on__handle_input (const gos::Engine::InputEvent &ev) final;
	void	on__navigation_mode_changed (u8 mode_uid) final;
	void	on__update (u64 timenow_msec) final;
	void	on__render() final;
	void 	on__unsetup() final;

private:
	static constexpr u8	NAV_MODE__ENTITY = 6;
	static constexpr u8	NAV_MODE__ENTITY_FIXED_CAM = 8;

private:
	gos::engine::Renderer_PIPE3			*renderer_PIPE3;
	gos::engine::Renderer_line3d		*renderer_line3d;
	gos::engine::Renderer_line3d::Ctx	*line_ctx1;
	gos::engine::Renderer_line3d::Ctx	*line_ctx2;
	
	land::Map		map;
	land::Renderer	*renderer_land;

	gos::geom::Pos3			pos;
	gos::Ctrl3rdPersMove	ctrl_entity;
	gos::ENGModel3dInst		handle_mi_cubo1x1x1;
	gos::geom::Camera3		*query_cam;
};

#endif //_App_h_
