#include "App.h"
using namespace gos;



//***************************************
App::App()
{
}

//***************************************
void App::on__unsetup()
{
	engine->release (shape_cubo_handle);
	engine->release (material_cubo_handle);

	engine->release (idxbuffer_handle);
	engine->release (vtxbuffer_handle);
	engine->release (texture_checker_handle);
	engine->release (material2_handle);
	engine->release (material1_handle);
	engine->release (pipe1_handle);
}

//***************************************
void App::test()
{
	engine->vtxBuffer_create (1024, eMemAccessMode::onGPU, &vtxbuffer_handle);
	engine->idxBuffer_create (1024, eMemAccessMode::onGPU, &idxbuffer_handle);

	engine->texture2D_createFromAsset ("checker1", &texture_checker_handle, res::eLoadMode::onDemand);
	{
		const TextureData *texture_data;
		GOS_DEBUG_ASSERT( engine->get_texture_bianca(&texture_data) );
		GOS_DEBUG_ASSERT( engine->get(texture_checker_handle, &texture_data, 4000) );

		//renderer_PIPE3 = engine->renderPipe.add_renderer<engine::Renderer_PIPE3>();
		const TextureData *texure_data2;
		engine->get(texture_checker_handle, &texure_data2);
		assert(texure_data2 == texture_data);
	}

	//creo un materiale,
	//De default, le texture diffuse e normal vengono associate a texture-bianca
	engine->materialPBR_create (&material2_handle);
	{
		const TextureData *texture_bianca_data;
		GOS_DEBUG_ASSERT( engine->get_texture_bianca(&texture_bianca_data) );

		MaterialPBRDataUpd upd;
		engine->get_for_update (material2_handle, &upd);
		{
			assert( upd.data->diffuse_texture == texture_bianca_data);
			assert( upd.data->normal_texture == texture_bianca_data);

			//cambio <diffuse-tex> con un'altra texture
			engine->materialPBR_update_texture_normal (&upd, texture_checker_handle);
		}

		//verifico che la modifica abbia avuto seguito
		const MaterialPBRData *material_data;
		GOS_DEBUG_ASSERT( engine->get(material2_handle, &material_data, 4000) );

		const TextureData *texture_checker_data;
		GOS_DEBUG_ASSERT( engine->get(texture_checker_handle, &texture_checker_data, 4000) );

		assert( material_data->diffuse_texture == texture_bianca_data);
		assert( material_data->normal_texture == texture_checker_data);
	}

	//carico un materiale
	engine->materialPBR_createFromAsset ("material01", &material1_handle, res::eLoadMode::asap);
	{
		const MaterialPBRData *material_data;
		GOS_DEBUG_ASSERT( engine->get(material1_handle, &material_data, 4000) );

		MaterialPBRDataUpd upd;
		engine->get_for_update (material1_handle, &upd);
		engine->materialPBR_update_texture_diffuse (&upd, engine->get_handle_texture_bianca());
	}

	engine->pipeline_createFromAsset ("gosengine_PIPE3", &pipe1_handle);
}

//***************************************
void App::on__setup ()
{
	renderer_PIPE3 = engine->renderPipe.add_renderer<engine::Renderer_PIPE3>();
	renderer_line3d = engine->renderPipe.add_renderer<engine::Renderer_line3d>();
		line_ctx1 = renderer_line3d->ctx__create_new("ctx1", 1024);


	engine->texture2D_createFromAsset ("checker1", &texture_checker_handle, res::eLoadMode::onDemand);
	engine->GPUShape_createFromAsset ("model_cubo1x1x1.Cube", &shape_cubo_handle);
	engine->materialPBR_createFromAsset ("model_cubo1x1x1.Material", &material_cubo_handle);
}

//***************************************
void App::on__update (u64 timenow_msec)
{
	static u64 next_time_upd__msec = 0;
	static f32 incr = 0.1f;
	static u8 which_texture = 0;


	if (timenow_msec < next_time_upd__msec)
		return;

	next_time_upd__msec = timenow_msec + 100;

	gos::MaterialPBRDataUpd upd;
	if (!engine->get_for_update(material_cubo_handle, &upd))
		return;
	
	f32 red = upd.data->diffuse_col_HDR_RGBA.values[0];
	{
		red += incr;
		if (red >= 1.0f)
		{
			red = 1.0f;
			incr = -incr;
		}
		if (red <= 0)
		{
			red = 0;
			incr = -incr;

			if (0 == which_texture)
			{
				which_texture = 1;
				engine->materialPBR_update_texture_diffuse (&upd, texture_checker_handle);
			}
			else
			{
				which_texture = 0;
				engine->materialPBR_update_texture_diffuse (&upd, engine->get_handle_texture_bianca());
			}			
		}
	}
	upd.data->diffuse_col_HDR_RGBA.values[0] = red;
	upd.data->diffuse_col_HDR_RGBA.values[1] = red;
	upd.data->diffuse_col_HDR_RGBA.values[2] = red;

}

//***************************************
void App::on__render()
{
	renderer_PIPE3->begin();
	{
		mat4x4f	m;

		m.identity();
		renderer_PIPE3->add (shape_cubo_handle, m, material_cubo_handle);

		m.buildTranslation (0, 2, 2);
		renderer_PIPE3->add (shape_cubo_handle, m, material_cubo_handle);
	}
	renderer_PIPE3->end();

	line_ctx1->clear();

	// const MaterialPBRData *materia_data;
	// engine->get (material1_handle, &materia_data);
	// engine->get (material2_handle, &materia_data);

	// const PipelineData *pipe_data;
	// engine->get (pipe1_handle, &pipe_data);
}

