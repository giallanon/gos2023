#ifndef _Land1_app_h_
#define _Land1_app_h_
#include "../DefaultApp/DefaultApp.h"
#include "renderPipe/gosEngineRenderPipe_line3d.h"
#include "Land1_renderer.h"
#include "Land1_map.h"



/******************************************
* Land1_app 
*
*/
class Land1_app : public DefaultApp
{
public:
			Land1_app();

protected:
	void	on__setup () final;
	void	on__handle_input (const gos::Engine::InputEvent &ev) final;
	void	on__prepare_render() final;
	void 	on__unsetup() final;

private:
	static const u32 NUM_ALBERI = 1; //128;

private:
	void	priv_draw_exa (const gos::vec3f &world_point, bool bToggle);

private:
	i16	mouse_x;
	i16	mouse_y;

	gos::engine::Renderer1				*renderer1;
	gos::engine::Renderer_line3d		*renderer_line3d;
	gos::engine::Renderer_line3d::Ctx	*line_ctx1;
	
	Land1::Map							map;
	Land1::Renderer						*renderer_land;
	
	gos::ENGTexture			handle_texBianca;
	gos::ENGModel3d 		handle_model_albero;
	gos::ENGModel3dInst		modelinst_albero[NUM_ALBERI];
};

#endif //_Land1_app_h_
