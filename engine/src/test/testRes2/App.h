#ifndef _App_h_
#define _App_h_
#include "defaultApp/gosDefaultApp.h"


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
	void 	on__unsetup() final;
	void	on__render() final;
	void	on__update (u64 timenow_msec) final;

private:
	void	test();

private:
	gos::engine::Renderer_PIPE3			*renderer_PIPE3;
	gos::engine::Renderer_line3d		*renderer_line3d;
	gos::engine::Renderer_line3d::Ctx	*line_ctx1;

	gos::GPUShapeH		shape_cubo_handle;
	gos::MaterialPBRH	material_cubo_handle;
	
	
	
	
	gos::VtxBufferH		vtxbuffer_handle;
	gos::IdxBufferH		idxbuffer_handle;
	gos::TextureH		texture_checker_handle;
	gos::MaterialPBRH	material1_handle;
	gos::MaterialPBRH	material2_handle;
	gos::PipelineH		pipe1_handle;
	
};

#endif //_App_h_
