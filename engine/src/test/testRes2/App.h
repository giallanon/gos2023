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
	void	test1();
	void	test1_on_update(u64 timenow_msec);
	void 	test1_render();
	
	void	test2();
	void	test2_on_update(u64 timenow_msec);
	void 	test2_render();

	void	test3();
	void	test3_on_update(u64 timenow_msec);
	void 	test3_render();

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
	gos::ModelH			model1_handle;
	gos::ModelH			model2_handle;

	gos::FastArray<gos::ModelInstH>		mi_handle_list;
};

#endif //_App_h_
