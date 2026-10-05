#include "App.h"
using namespace gos;



//***************************************
App::App()
{
}

//***************************************
void App::on__unsetup()
{
	engine->release (idxbuffer_handle);
	engine->release (vtxbuffer_handle);
	engine->release (texture1_handle);
}

//***************************************
void App::on__setup ()
{
	engine->vtxBuffer_create (1024, eMemAccessMode::onGPU, &vtxbuffer_handle);
	engine->idxBuffer_create (1024, eMemAccessMode::onGPU, &idxbuffer_handle);

	engine->texture2D_createFromAsset ("checker1", &texture1_handle, res::eLoadMode::onDemand);


	const TextureData *texure_data;
	GOS_DEBUG_ASSERT( engine->get_texture_bianca(&texure_data) );
	GOS_DEBUG_ASSERT( engine->get(texture1_handle, &texure_data, 4000) );

	//renderer_PIPE3 = engine->renderPipe.add_renderer<engine::Renderer_PIPE3>();
}

//***************************************
void App::on__render()
{
	//renderer_PIPE3->begin();
	//renderer_PIPE3->end();
}

