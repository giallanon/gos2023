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
	engine->release (model1_handle);
	engine->release (model2_handle);
	engine->release (model3_handle);

	for (u32 i=0; i<mi_handle_list.getNElem(); i++)
	{
		ModelInstH h = mi_handle_list(i);
		engine->release(h);
	}
}

//***************************************
void App::on__setup ()
{
	mi_handle_list.setup (gos::getSysHeapAllocator(), 1000);

	//test1();
	//test2();
	test3();
}

//***************************************
void App::on__update (u64 timenow_msec)
{
	//test1_on_update (timenow_msec);
	//test2_on_update (timenow_msec);
	test3_on_update (timenow_msec);

}

//***************************************
void App::on__render()
{
	// test1_render();
	// test2_render();
	test3_render();
}


//***************************************
void App::test1()
{
	engine->vtxBuffer_create (1024, eMemAccessMode::onGPU, &vtxbuffer_handle);
	engine->idxBuffer_create (1024, eMemAccessMode::onGPU, &idxbuffer_handle);

	engine->texture2D_createFromAsset ("checker1", &texture_checker_handle, res2::eLoadMode::onDemand);
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
	engine->materialPBR_createFromAsset ("material01", &material1_handle, res2::eLoadMode::asap);
	{
		const MaterialPBRData *material_data;
		GOS_DEBUG_ASSERT( engine->get(material1_handle, &material_data, 4000) );

		MaterialPBRDataUpd upd;
		engine->get_for_update (material1_handle, &upd);
		engine->materialPBR_update_texture_diffuse (&upd, engine->get_handle_texture_bianca());
	}

	engine->pipeline_createFromAsset ("gosengine_PIPE3", &pipe1_handle);
}
void App::test1_on_update(u64 timenow_msec)		{ }	
void App::test1_render()
{
	const MaterialPBRData *materia_data;
	engine->get (material1_handle, &materia_data);
	engine->get (material2_handle, &materia_data);
}


//***************************************
void App::test2()
{
	renderer_PIPE3 = engine->renderPipe.add_renderer<engine::Renderer_PIPE3>();
	renderer_line3d = engine->renderPipe.add_renderer<engine::Renderer_line3d>();
		line_ctx1 = renderer_line3d->ctx__create_new("ctx1", 1024);


	engine->texture2D_createFromAsset ("checker1", &texture_checker_handle, res2::eLoadMode::onDemand);
	engine->GPUShape_createFromAsset ("model_cubo1x1x1.Cube", &shape_cubo_handle);
	engine->materialPBR_createFromAsset ("model_cubo1x1x1.Material", &material_cubo_handle);
}
void App::test2_on_update(u64 timenow_msec)
{
	static u64 next_time_upd__msec = 0;
	static f32 incr = 0.02f;
	static u8 which_texture = 0;


	if (timenow_msec < next_time_upd__msec)
		return;

	next_time_upd__msec = timenow_msec + 33;

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
void App::test2_render()
{
	renderer_PIPE3->begin();
	{
		mat4x4f	m;

		m.identity();
		renderer_PIPE3->add (shape_cubo_handle, &m, material_cubo_handle);

		m.buildTranslation (0, 2, 2);
		renderer_PIPE3->add (shape_cubo_handle, &m, material_cubo_handle);
	}
	renderer_PIPE3->end();

	line_ctx1->clear();
}

//***************************************
void App::test3()
{
	static constexpr i32 NUM_PER_LATO = 52;
	static constexpr f32 SX = 2.0f;
	static constexpr f32 SY = 2.0f;
	static constexpr f32 SZ = 2.0f;

	renderer_PIPE3 = engine->renderPipe.add_renderer<engine::Renderer_PIPE3>();
	engine->model_createFromAsset ("gix_tree_1", &model1_handle);
	engine->model_createFromAsset ("model_cubo1x1x1", &model2_handle);
	
	engine->materialPBR_createFromAsset ("material01", &material1_handle, res2::eLoadMode::asap);
	engine->materialPBR_createFromAsset ("material02", &material2_handle, res2::eLoadMode::asap);

	//clono il modello e gli cambio materiale
	if (engine->model_clone (model1_handle, &model3_handle, 2000))
	{
		ModelDataUpd upd;
		GOS_DEBUG_ASSERT( engine->get_for_update(model3_handle, &upd) );
		engine->model_update_material (&upd, 1, material1_handle);
	}



	u32 ct = 0;
	for (i32 y = 0; y <= NUM_PER_LATO; y++)
	{
		for (i32 z = 0; z <= NUM_PER_LATO; z++)
		{
			for (i32 x = -NUM_PER_LATO/2; x <NUM_PER_LATO/2; x++)
			{
				ModelInstH handle;
				gos::mat4x4f m;
				m.buildTranslation (x*SX, y*SY, z*SZ);
				switch (ct%3)
				{
				case 0:	engine->modelinst_create (model1_handle, &handle); break;
				case 1:	engine->modelinst_create (model2_handle, &handle); break;
				case 2:	engine->modelinst_create (model3_handle, &handle); break;
				}

				engine->modelinst_applyTransform (handle, m);
				mi_handle_list.append(handle);
				ct++;
			}
		}
	}

	engine->vtxBuffer_create (1024, eMemAccessMode::onGPU, &vtxbuffer_handle);
}

void App::test3_on_update(u64 timenow_msec)		{ }	
void App::test3_render()
{
	renderer_PIPE3->begin();
	{
		static gos::TimerFPS debug_t2;
		debug_t2.onFrameBegin();
		const u32 N = mi_handle_list.getNElem();
		for (u32 i=0; i<N; i++)
		{
			renderer_PIPE3->add (mi_handle_list(i));
		}
		if (debug_t2.onFrameEnd())
			printf ("**** (%d) debug_t2 %.2f ms\n", mi_handle_list.getNElem(), debug_t2.getAvgFrameTime_ms());
	}
	renderer_PIPE3->end();
}



