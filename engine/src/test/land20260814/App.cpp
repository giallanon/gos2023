#include "App.h"
using namespace gos;



//***************************************
App::App()
{
	query_cam = NULL;
}

//***************************************
void App::on__unsetup()
{
	engine->release(handle_mi_cubo1x1x1);
}

//***************************************
void App::on__setup ()
{
	land::Map::CreateData data;
	land::Map::create ("@w/assets/asset_src/heightmap/map2_1", data);

	map.load ("@w/assets/asset_src/heightmap/map2_1");

	// land::Map::create ("@w/assets/asset_src/heightmap/Hills_01.png", 129, 0.5f, 0.1f);
	// map.load ("@w/assets/asset_src//heightmap/Hills_01");

	// land::Map::create ("@w/assets/asset_src/heightmap/radial.png", 129, 0.5f, 0.1f);
	// map.load ("@w/assets/asset_src//heightmap/radial");

	// land::Map::create ("@w/assets/asset_src/heightmap/anorway_30m.png", 129, 1.0f, 0.1f);
	// map.load ("@w/assets/asset_src//heightmap/anorway_30m");
	
	


	renderer_PIPE3 = engine->renderPipe.add_renderer<engine::Renderer_PIPE3>();
	renderer_land = engine->renderPipe.add_renderer<land::Renderer>();
	renderer_line3d = engine->renderPipe.add_renderer<engine::Renderer_line3d>();
		line_ctx1 = renderer_line3d->ctx__create_new("ctx1", 1024);
		line_ctx2 = renderer_line3d->ctx__create_new("ctx2", 32);

	renderer_land->map__bind (&map);
	
	gos::ENGModel3d		handle_model_cubo1x1x1;
	engine->model_createFromAsset ("model_cubo1x1x1", &handle_model_cubo1x1x1, res::eLoadMode::asap);
	engine->modelinst_create (handle_model_cubo1x1x1, &handle_mi_cubo1x1x1);
	engine->release(handle_model_cubo1x1x1);

	//posiziono il player
	{
		mat4x4f matW;
		matW.buildTranslation (0, 0.5f, 0);
		matW.buildTranslation (-451.97, 10.47, 567.51);
		engine->modelinst_applyTransform (handle_mi_cubo1x1x1, matW);
	}
	
	

	//default camera, la ricreo perche' voglio una far-distance molto grande
	geom::Camera3 *cam = camera__create (0, math::gradToRad(45), 0.1f, land::LAND__VIEW_DISTANCE_m);
	//cam->pos.warp (0, 716, 0);
	cam->pos.warp (0, 2, 0);
	cam->pos.lookAt(vec3f(0,2,10));
	cam->mark_updated();
	ctrl_default_cam.set_linear_speed__m_sec(20);

	//camera 1 per il movimento dell'entity
	navigation__create_mode(NAV_MODE__ENTITY);
	{
		cam = camera__create (1, math::gradToRad(45), 0.1f, land::LAND__VIEW_DISTANCE_m);
	}

	//camera 1 per il movimento dell'entity ma con camera di default
	navigation__create_mode(NAV_MODE__ENTITY_FIXED_CAM);
	{
		cam = camera__create (2, math::gradToRad(45), 0.1f, 1500);
	}
	
}

//***************************************
void App::on__render()
{
	//addo dei chunk
	FastArray<land::ChunkCoord> chunk_list(gos::getScrapAllocator(), 32);
	map.calc_visible_chunk (query_cam, &chunk_list);
	renderer_land->begin();
	renderer_land->add (chunk_list);
	renderer_land->end();
	
	
	renderer_PIPE3->begin();
	renderer_PIPE3->add (handle_mi_cubo1x1x1);
	renderer_PIPE3->end();

	line_ctx1->clear();
	if (NAV_MODE__ENTITY == navigation__get_mode())
	{
		geom::Frustum3	fr = query_cam->get_frustumWC();

		vec3f vv[8];
		fr.calc_8points(vv);
		for (u32 i=0; i<8; i++)
			line_ctx1->vtx_add(vv[i]);

		line_ctx1->set_color_ARGB (0xFFFF00FF)	
			.line_begin().line_add_vtx(0).line_add_vtx(1).line_add_vtx(2).line_add_vtx(3).line_add_vtx(0).line_end()
			//.line_begin().line_add_vtx(4).line_add_vtx(5).line_add_vtx(6).line_add_vtx(7).line_add_vtx(4).line_end()
			;
	}

	if (NAV_MODE__ENTITY_FIXED_CAM == navigation__get_mode())
	{
		geom::Frustum3	fr = query_cam->get_frustumWC();

		vec3f vv[8];
		fr.calc_8points(vv);
		for (u32 i=0; i<8; i++)
			line_ctx1->vtx_add(vv[i]);

		line_ctx1->set_color_ARGB (0xFFFF00FF)	
			.line (0, 4)
			.line (1, 5)
			.line (2, 6)
			.line (3, 7);
	}
}

//***************************************
void App::on__handle_input (const Engine::InputEvent &ev)
{
}

//***************************************
void App::on__navigation_mode_changed (u8 mode_uid)
{
	switch (mode_uid)
	{
	default:
		query_cam = camera__get(0);
		camera__set_render_camera (0);
		break;

	case NAV_MODE__ENTITY:
		//muovo entity e faccio query/renderizzo in base alla sua camera
		{
			ctrl_entity.set_linear_speed__m_sec (15);

			mat4x4f matW;
			engine->modelinst_getWMatrix (handle_mi_cubo1x1x1, &matW);
			pos.setFromMatrix4x4(matW);
			
			ctrl_entity.bind (&pos, camera__get(1));
			query_cam = camera__get(1);
			camera__set_render_camera (1);
		}
		break;

	case NAV_MODE__ENTITY_FIXED_CAM:
		//muovo entity e faccio query in base alla sua camera
		//renderizzo dalla camera posta sopra la sua testa
		{
			mat4x4f matW;
			engine->modelinst_getWMatrix (handle_mi_cubo1x1x1, &matW);
			pos.setFromMatrix4x4(matW);

			ctrl_entity.bind (&pos, camera__get(1));
			query_cam = camera__get(1);
			camera__set_render_camera (2);

			//accelero l'entity
			ctrl_entity.set_linear_speed__m_sec (20);
		}
		break;

	}
}

//***************************************
void App::on__update (u64 timenow_msec)
{
	switch (navigation__get_mode())
	{
	default:
		break;

	case NAV_MODE__ENTITY:
	case NAV_MODE__ENTITY_FIXED_CAM:
		{
			ctrl_entity.update (gos::getTimeSinceStart_msec(), ctrl_action);
			mat4x4f matW;
			ctrl_entity.entity_pos->getMatrix4x4(&matW);
			engine->modelinst_applyTransform (handle_mi_cubo1x1x1, matW);

			if (NAV_MODE__ENTITY_FIXED_CAM == navigation__get_mode())
			{
				//sto muovendo entity e cam(1) tramite il ctrl.
				//Voglio pero' avere cam(2) sempre sopra a entity a mo' di bird-view
				geom::Camera3 *cam = camera__get(2);
				cam->pos.identity();
				cam->pos.o = ctrl_entity.entity_pos->o;
				cam->pos.o.y += 1450;
				cam->pos.lookAt (ctrl_entity.entity_pos->o);
				cam->mark_updated();
			}
		}
		break;

	}

}
