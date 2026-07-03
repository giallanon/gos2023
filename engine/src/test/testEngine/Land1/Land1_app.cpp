#include "Land1_app.h"

using namespace gos;



//***************************************
Land1_app::Land1_app()
{
	mouse_x = mouse_y = 0;
	material_index_to_apply = 1;
	num_alberi = 0;
}

//***************************************
void Land1_app::on__unsetup()
{
	engine->release (handle_model_albero);

	for (u32 i=0; i<num_alberi; i++)
		engine->release (modelinst_albero[i]);
}

//***************************************
void Land1_app::on__setup ()
{
	engine->inputCtx->
		action_add ("mouse_LB")
		.action_add ("mouse_LB+SHIFT")
		.action_add ("KB_1")
		.action_add ("KB_2")
		.action_add ("KB_3")
		.action_add ("KB_0");
		
	engine->inputCtx->action_bindToBtn ("mouse_LB", input::eOrigin::mouse, GOS_BUTTON_MOUSE_LEFT, input::eButtonStatus::pressed);
	engine->inputCtx->action_bindToBtn ("mouse_LB+SHIFT", input::eOrigin::mouse, GOS_BUTTON_MOUSE_LEFT, input::eButtonStatus::pressed, input::eButtonModifier::LSHIFT);
	engine->inputCtx->action_bindToBtn ("KB_1", input::eOrigin::keyboard, GLFW_KEY_1, input::eButtonStatus::pressed);
	engine->inputCtx->action_bindToBtn ("KB_2", input::eOrigin::keyboard, GLFW_KEY_2, input::eButtonStatus::pressed);
	engine->inputCtx->action_bindToBtn ("KB_3", input::eOrigin::keyboard, GLFW_KEY_3, input::eButtonStatus::pressed);
	engine->inputCtx->action_bindToBtn ("KB_0", input::eOrigin::keyboard, GLFW_KEY_0, input::eButtonStatus::pressed);

	renderer_PIPE3 = engine->renderPipe.add_renderer<engine::Renderer_PIPE3>();
	renderer_land = engine->renderPipe.add_renderer<Land1::Renderer>();
	renderer_line3d = engine->renderPipe.add_renderer<engine::Renderer_line3d>();
		line_ctx1 = renderer_line3d->ctx__crete_new("ctx1", 32);

	//carico un po' di texture
	{
		const gos::res::Texture2d *tex;
		engine->get_texture_bianca (&tex);

		engine->renderPipe.texture_addIfNotExitst (tex->texHandle);
		renderer_PIPE3->material_create (0, vec3f(0.3f, 1 , 0.3f));
	}

	//engine->model_createFromAsset ("model_LowPolyTree", &handle_model_albero, res::eLoadMode::asap);
	engine->model_createFromAsset ("model_gix_tree_1", &handle_model_albero, res::eLoadMode::asap);
	//engine->model_createFromAsset ("model_albero", &handle_model_albero, res::eLoadMode::asap);
	//engine->model_createFromAsset ("model_omino", &handle_model_albero, res::eLoadMode::asap);
	

	cam.pos.warp (0, 20, 0);
	cam.pos.lookAt(vec3f(0,0,0));
	cam.markUpdated();
	move_free.bind (&cam.pos);


	//creo una mappa
	const f32 EXA_WORLD_RADIUS = 20.0f;
	const u32 NUM_RINGS = 3;
	map.setup (gos::getSysHeapAllocator());
	map.map_create (EXA_WORLD_RADIUS, NUM_RINGS);
}

//***************************************
void Land1_app::priv_new_albero (const gos::vec3f &world_point)
{
	if (num_alberi >= NUM_MAX_ALBERI)
		return;

	const res::Model3d *res_model_albero;
	if (!engine->get (handle_model_albero, &res_model_albero, 4000))
	{
		DBGBREAK;
		return;
	}

	engine->modelinst_create (handle_model_albero, &modelinst_albero[num_alberi]);

	mat4x4f matW;
	mat4x4f matTr;
	mat4x4f matRot;
	matRot.buildRotationAboutY ( math::gradToRad(gos::random01()*360.0f ));
	matTr.buildTranslation ( world_point );
	matW = matTr * matRot;
	engine->modelinst_applyTransform (modelinst_albero[num_alberi], matW);

	num_alberi++;
}

//***************************************
void Land1_app::on__prepare_render()
{
	//static u8 ok = 0;
	//if (1 == ok)
	//	return;
	//ok = 1;

	Land1::Map::Result r;
	r.setup (gos::getScrapAllocator());
	map.query_visible_exa (&r);

	renderer_land->begin();
	for (u32 i = 0; i < r.get_num(); i++)
		renderer_land->add_exa (r.get_exa_by_index(i));
	renderer_land->end();



	//alberi
	renderer_PIPE3->begin();
	for (u32 i=0; i<num_alberi; i++)
		renderer_PIPE3->add (modelinst_albero[i]);
	renderer_PIPE3->end();
}

//***************************************
void Land1_app::on__handle_input (const Engine::InputEvent &ev)
{
	switch (ev.actionID)
	{
	default:
	break;

	case COMPILE_TIME_STR_CRC32("mouse_move_x"):
		mouse_x = ev.value;
		break;

	case COMPILE_TIME_STR_CRC32("mouse_move_y"):
		mouse_y = ev.value;
		break;	

	case COMPILE_TIME_STR_CRC32("KB_0"):	material_index_to_apply = 0xFF; break;
	case COMPILE_TIME_STR_CRC32("KB_1"):	material_index_to_apply = 1; break;
	case COMPILE_TIME_STR_CRC32("KB_2"):	material_index_to_apply = 2; break;
	case COMPILE_TIME_STR_CRC32("KB_3"):	material_index_to_apply = 3; break;
		
	case COMPILE_TIME_STR_CRC32("mouse_LB"):
	case COMPILE_TIME_STR_CRC32("mouse_LB+SHIFT"):
		{
			const gpu::Viewport *vp = gpu->getInfo (gpu->viewport_getDefault());
			vec2f m(mouse_x, mouse_y);
			vec3f world_dir;
			cam.unproject (vp->getW_f32(), vp->getH_f32(), &m , &world_dir, 1);

			//supponendo che hex sia sempre ad altezza y=0...
			const f32 t = -cam.pos.o.y / world_dir.y;
			const vec3f point_on_hex = cam.pos.o + world_dir * t;

			priv_draw_exa (point_on_hex, engine->inputEvent_getBtnModifier()->isLSHIFT());
		}
		break;
	}
}

//***************************************
void Land1_app::priv_draw_exa (const gos::vec3f &world_point, bool bLSHIFT)
{
	const examap::Coord coord = map.world_coord_to_exa (world_point);
	logger::log ("hex @ (%d, %d)  LSHIFT=%c\n", coord.x, coord.z, bLSHIFT?'Y':'N');
	

	const u32 N_COLORS = 7;
	const u32 colors[N_COLORS] = { 0xFFFFFFFF, 0xFFFF0000, 0xFF00FF00, 0xFF0000FF, 0xFFFFFF00, 0xFF00FFFF, 0xFFFF00FF };

	line_ctx1->clear();
	line_ctx1->enable_depth_test(false);
	line_ctx1->enable_depth_write(false);
	line_ctx1->set_line_width(2);

	//disegno l'exa <coord>
	const Land1::Exa *exa;
	if (map.exa_query(coord, &exa))
	{
		//disegno il reticolo dell'exa
		line_ctx1->
			set_color_ARGB (0xFF000000)
			.set_line_width(2);

		const u32 first_vtx = line_ctx1->vtx_get_num();
		for (u32 i = 0; i < exa->num_vtx_tot; i++)
			line_ctx1->vtx_add (vec3f(exa->vtxList[i].x, 0, exa->vtxList[i].y));
		for (u32 iVtx = 0; iVtx < exa->num_vtx_originali; iVtx++)
		{
			if (exa->vtxInfoList[iVtx].is_border_vtx)
				continue;
			const Land1::Exa::VtxInfo *vi = &exa->vtxInfoList[iVtx];

			for (u32 i=0; i<vi->num_quad; i++)
			{
				line_ctx1->line (first_vtx + vi->idx_list[0], first_vtx + vi->idx_list[1+2*i]);
			}
		}

		//diesgno i punti originali
		line_ctx1->point_set_radius(6);
		for (u32 iVtx = 0; iVtx < exa->num_vtx_originali; iVtx++)
		{
			line_ctx1->point (first_vtx + exa->vtxInfoList[iVtx].idx_list[0]);
		}

		//cerco il vtx + vicino a <world_point>
		u32 vtx_index;
		if (exa->get_closest_vtx_from_point(world_point, &vtx_index))
		{
			line_ctx1->point_set_radius(16);

			const Land1::Exa::VtxInfo *vi = &exa->vtxInfoList[vtx_index];

			//diesgno il quad composto dai quad-center
			line_ctx1->
				set_color_ARGB (0xFF00FF00)
				.line_begin();
			for (u32 i = 0; i < vi->num_quad; i++)
			{
				const u32 vtx_index = vi->idx_list[2 + i * 2];
				line_ctx1->line_add_vtx(first_vtx + vtx_index);
			}
			line_ctx1->line_add_vtx(first_vtx + vi->idx_list[2]);
			line_ctx1->line_end();

			//disegno i vtx
			for (u32 i = 0; i < vi->num_idx; i++)
			{
				const u32 vtx_index = first_vtx + vi->idx_list[i];
				line_ctx1->
					set_color_ARGB (colors[i % N_COLORS])
					.point (vtx_index);
			}


			//if (bLSHIFT)
			//{
			//	if (0xFF == material_index_to_apply)
			//	{
			//		priv_new_albero( vec3f(exa->vtxList[vtx_index].x, 0, exa->vtxList[vtx_index].y) );
			//	}
			//	else
			//	{
			//		if (0 == exa->vtxInfoList[vtx_index].material_index)
			//			exa->vtxInfoList[vtx_index].material_index = material_index_to_apply;
			//		else
			//			exa->vtxInfoList[vtx_index].material_index = 0;
			//	}
			//}
		}

	}

}