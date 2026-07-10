#ifndef _Land1_app2_h_
#define _Land1_app2_h_
#include "../DefaultApp/DefaultApp.h"
#include "renderPipe/gosEngineRenderPipe_line3d.h"
#include "Land1_renderer.h"
#include "Land1_map2.h"



/******************************************
* Land1_app2
*
*/
class Land1_app2 : public DefaultApp
{
public:
			Land1_app2();

protected:
	void	on__setup () final;
	void	on__handle_input (const gos::Engine::InputEvent &ev) final;
	void	on__prepare_render() final;
	void 	on__unsetup() final;

private:
	static const u32 NUM_MAX_ALBERI = 128;

private:
	void	priv_draw_exa (const gos::vec3f &world_point, bool bLSHIFT);
	void	priv_new_albero (const gos::vec3f &world_point);

private:
	i16	mouse_x;
	i16	mouse_y;

	gos::engine::Renderer_PIPE3			*renderer_PIPE3;
	gos::engine::Renderer_line3d		*renderer_line3d;
	gos::engine::Renderer_line3d::Ctx	*line_ctx1;
	
	Land1::Map2							map;
	Land1::Renderer						*renderer_land;
	
	gos::ENGModel3d 		handle_model_albero;
	gos::ENGModel3dInst		modelinst_albero[NUM_MAX_ALBERI];
	u32						num_alberi;

	gos::ENGModel3d 		handle__model_exa;
	gos::ENGModel3dInst		modelinst_exa[1024];
	u32						num_modelinst_exa;

	u8	material_index_to_apply;
};

#endif //_Land1_app2_h_


