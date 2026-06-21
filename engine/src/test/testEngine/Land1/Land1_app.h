#ifndef _Land1_app_h_
#define _Land1_app_h_
#include "../DefaultApp/DefaultApp.h"
#include "Land1.h"
#include "renderPipe/gosEngineRenderPipe_line3d.h"


/******************************************
* Land1_app 
*
*/
class Land1_app : public DefaultApp
{
public:
			Land1_app();
			~Land1_app();

protected:
	void	on__load_assets () final;
	void	on__handle_input (const gos::Engine::InputEvent &ev) final;
	void 	on__cleanup() final;

private:
	static const u32 NUM_ALBERI = 0; //128;

private:
	i16	mouse_x;
	i16	mouse_y;

	gos::engine::Renderer1			*renderer1;
	gos::engine::Renderer_line3d	*renderer_line3d;
	gos::engine::Renderer_line3d::Ctx	*line_ctx1;
	Land1							*renderer_land;
	
	gos::ENGTexture			handle_texBianca;
	gos::ENGModel3d 		handle_model_albero;
	gos::ENGModel3dInst		modelinst_albero[NUM_ALBERI];
};

#endif //_Land1_app_h_
