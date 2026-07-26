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
	void	priv_draw_exa (const Land1::GVC &gvc, bool bLSHIFT);
	u32		priv_do_draw_exa (const gos::examap::Coord exa_coord, gos::FastArray<Land1::Map2::Vtx> *out_list);
	void	priv_new_albero (const gos::vec3f &world_point);
	void	priv_mouse_to_GVC ();
	void 	priv_on_mouse_click (bool bLB);

private:
	i16	mouse_x;
	i16	mouse_y;

	gos::engine::Renderer_PIPE3			*renderer_PIPE3;
	gos::engine::Renderer_line3d		*renderer_line3d;
	gos::engine::Renderer_line3d::Ctx	*line_ctx1;
	gos::engine::Renderer_line3d::Ctx	*line_ctx2;
	
	Land1::Map2							map;
	Land1::Renderer						*renderer_land;
	
	gos::ENGModel3d 		handle_model_albero;
	gos::ENGModel3dInst		modelinst_albero[NUM_MAX_ALBERI];
	u32						num_alberi;


	Land1::GVC				last_mouseover_gvc;
	u64						next_time_calc_mouseover_ms;
	u8						material_index_to_apply;
};

#endif //_Land1_app2_h_


