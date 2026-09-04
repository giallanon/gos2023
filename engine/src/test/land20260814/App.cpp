#include "App.h"
using namespace gos;



//***************************************
App::App()
{
	query_cam = NULL;
	ccList.setup (gos::getScrapAllocator(), 1024);

	ctrl_entity.set_zoom_limits (0.1f, 100000.0f);
}

//***************************************
void App::on__unsetup()
{
	engine->release(handle_mi_cubo1x1x1);
}

//***************************************
void App::on__setup ()
{
	material_list.add (MATERIAL_ID__DEEP_WATER, 0x000d233a);
	material_list.add (MATERIAL_ID__SHALLOW_WATER, 0x00285973);
	material_list.add (MATERIAL_ID__SAND, 0x00d2b48c);
	material_list.add (MATERIAL_ID__LUSH_GRASS, 0x004a703b);
	material_list.add (MATERIAL_ID__FOREST, 0x002d4a22);
	material_list.add (MATERIAL_ID__DIRT, 0x005a4d41);
	material_list.add (MATERIAL_ID__ROCK, 0x007a7a7a);
	material_list.add (MATERIAL_ID__SNOW, 0x00f0f4f7);


	// land::Map::CreateData create_4096;
	// land::Map::create ("@w/assets/asset_src/heightmap/ms_4096", create_4096);
	// map.open ("@w/assets/asset_src/heightmap/ms_4096");

	// land::Map::CreateData create_1024;
	// create_1024.default_map__border_size__point = 1024;
	// create_1024.default_height__m = 10;
	// land::Map::create ("@w/assets/asset_src/heightmap/ms_1024", create_1024);
	map.open ("@w/assets/asset_src/heightmap/ms_1024");
	map.apply_heightmap ("@w/assets/asset_src/heightmap/radial.png", land::Resol::_1m, 0.2f);

	
	
//	land::Map::create_from_hmap ("@w/assets/asset_src/heightmap/anorway_30m.png", 0.06f);
	
	
	renderer_PIPE3 = engine->renderPipe.add_renderer<engine::Renderer_PIPE3>();
	renderer_land = engine->renderPipe.add_renderer<land::Renderer>();
	renderer_line3d = engine->renderPipe.add_renderer<engine::Renderer_line3d>();
		line_ctx1 = renderer_line3d->ctx__create_new("ctx1", 1024);
		line_ctx2 = renderer_line3d->ctx__create_new("ctx2", 32);

	renderer_land->bind_map (&map);
	
	gos::ENGModel3d		handle_model_cubo1x1x1;
	engine->model_createFromAsset ("model_cubo1x1x1", &handle_model_cubo1x1x1, res::eLoadMode::asap);
	engine->modelinst_create (handle_model_cubo1x1x1, &handle_mi_cubo1x1x1);
	engine->release(handle_model_cubo1x1x1);

	//posiziono il player
	{
		mat4x4f matW;
		matW.buildTranslation (0, 0.5f, 0);
		//matW.buildTranslation (-451.97, 10.47, 567.51);
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
		cam = camera__create (1, math::gradToRad(45), 0.1f, 100000.0f); //land::LAND__VIEW_DISTANCE_m);
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
	map.qtree__calc_visibility (query_cam, &ccList);
	renderer_land->begin();
	renderer_land->add (ccList);
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
	switch (ev.actionID)
	{
	default:
		break;

	case COMPILE_TIME_STR_CRC32("speed++"):
		switch (navigation__get_mode())
		{
		default:
			break;

		case 0:
			ctrl_default_cam.multiply_linear_speed (1.1f);
			logger::log ("CAM SPEED++ = %.2f m/s\n", ctrl_default_cam.get_linear_speed__m_sec());
			break;

		case NAV_MODE__ENTITY:
		case NAV_MODE__ENTITY_FIXED_CAM:
			ctrl_entity.multiply_linear_speed (1.1f);
			logger::log ("CAM SPEED++ = %.2f m/s\n", ctrl_entity.get_linear_speed__m_sec());
			break;
		}
		break;

	case COMPILE_TIME_STR_CRC32("speed--"):
		switch (navigation__get_mode())
		{
		default:
			break;

		case 0:
			ctrl_default_cam.multiply_linear_speed (0.9f);
			logger::log ("CAM SPEED-- = %.2f m/s\n", ctrl_default_cam.get_linear_speed__m_sec());
			break;

		case NAV_MODE__ENTITY:
		case NAV_MODE__ENTITY_FIXED_CAM:
			ctrl_entity.multiply_linear_speed (0.9f);
			logger::log ("CAM SPEED-- = %.2f m/s\n", ctrl_entity.get_linear_speed__m_sec());
			break;
		}
		break;		
	}
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
