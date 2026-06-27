#include "Land1_app.h"

using namespace gos;



//***************************************
Land1_app::Land1_app()
{
	renderer1 = NULL;
	mouse_x = mouse_y = 0;
	material_index_to_apply = 1;
	num_alberi = 0;
}

//***************************************
void Land1_app::on__unsetup()
{
	engine->release (handle_texBianca);
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

	renderer_land = renderPipe.add_renderer<Land1::Renderer>();
	renderer1 = renderPipe.add_renderer<engine::Renderer1>();
	renderer_line3d = renderPipe.add_renderer<engine::Renderer_line3d>();
		line_ctx1 = renderer_line3d->ctx__crete_new("ctx1", 32);

	//carico un po' di texture
	engine->texture2D_createFromAsset ("tex_bianca", &handle_texBianca);
	{
		const gos::res::Texture2d *tex;
		engine->get (handle_texBianca, &tex, 5000);

		renderPipe.texture_addIfNotExitst (tex->texHandle);
		renderer1->material_create (0, vec3f(0.3f, 1 , 0.3f));
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
	renderer1->begin();
	for (u32 i=0; i<num_alberi; i++)
		renderer1->add (modelinst_albero[i]);
	renderer1->end();
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
	

	line_ctx1->clear();
	line_ctx1->enable_depth_test(false);
	line_ctx1->enable_depth_write(false);
	line_ctx1->set_line_width(2);

	//disegno tutti i quad dell'exa <coord>
	const Land1::Exa *exa;
	if (map.exa_query(coord, &exa))
	{
		//disegno il reticolo dell'exa
		line_ctx1->set_color_ARGB (0xFF000000);
		const u32 first_vtx = line_ctx1->vtx_get_num();
		for (u32 i = 0; i < exa->num_vtx; i++)
			line_ctx1->vtx_add (vec3f(exa->vtxList[i].x, 0, exa->vtxList[i].y));
		for (u32 i = 0; i < exa->num_quad; i++)
		{
			line_ctx1->
				set_line_width(2)
				.line_begin()
				.line_add_vtx (first_vtx + exa->quadList[i].idx[0])
				.line_add_vtx (first_vtx + exa->quadList[i].idx[1])
				.line_add_vtx (first_vtx + exa->quadList[i].idx[2])
				.line_add_vtx (first_vtx + exa->quadList[i].idx[3])
				.line_add_vtx (first_vtx + exa->quadList[i].idx[0])
				.line_end();
		}

		//evidenzio il quad selezionato
		//u32 quad_index;
		//if (exa->get_quad_from_point (world_point, &quad_index))
		//{
		//	line_ctx1->
		//		set_line_width (4)
		//		.set_color_ARGB (0xFFFF0000)
		//		.line_begin()
		//		.line_add_vtx (first_vtx + exa->quadList[quad_index].idx[0])
		//		.line_add_vtx (first_vtx + exa->quadList[quad_index].idx[1])
		//		.line_add_vtx (first_vtx + exa->quadList[quad_index].idx[2])
		//		.line_add_vtx (first_vtx + exa->quadList[quad_index].idx[3])
		//		.line_add_vtx (first_vtx + exa->quadList[quad_index].idx[0])
		//		.line_end();

		//	if (0xFF == material_index_to_apply)
		//	{
		//		const vec2f v = exa->utils__calc_quad_center (quad_index);
		//		priv_new_albero( vec3f(v.x, 0, v.y) );
		//	}
		//	else
		//	{
		//		exa->quadList[quad_index].material_index = material_index_to_apply;
		//	}
		//}

		//cerco il vtx + vicino
		u32 vtx_index;
		if (exa->get_closest_vtx_from_point(world_point, &vtx_index))
		{
			const u32 i = line_ctx1->vtx_add (vec3f (exa->vtxList[vtx_index].x, 0, exa->vtxList[vtx_index].y) );
			line_ctx1->
				set_color_ARGB (0xFFFFFFFF)
				.point_set_radius(8)
				.point (i);

			//renderizzo i quad che sharano il vtx
			line_ctx1->
				set_color_ARGB (0xFF00FF00)
				.set_line_width(2);

			u32 quads[8];
			const u32 nquad = exa->get_quad_from_vtx (vtx_index, quads, 8);
			for (u32 i = 0; i < nquad; i++)
			{
				const Land1::Exa::Quad *q = &exa->quadList[quads[i]];

				line_ctx1->
					line_begin()
						.line_add_vtx(first_vtx + q->idx[0])
						.line_add_vtx(first_vtx + q->idx[1])
						.line_add_vtx(first_vtx + q->idx[2])
						.line_add_vtx(first_vtx + q->idx[3])
						.line_add_vtx(first_vtx + q->idx[0])
					.line_end();
			}

			if (bLSHIFT)
			{
				if (0xFF == material_index_to_apply)
				{
					priv_new_albero( vec3f(exa->vtxList[vtx_index].x, 0, exa->vtxList[vtx_index].y) );
				}
				else
				{
					if (0 == exa->vtxInfoList[vtx_index].material_index)
						exa->vtxInfoList[vtx_index].material_index = material_index_to_apply;
					else
						exa->vtxInfoList[vtx_index].material_index = 0;
				}
			}
		}

	}

	//disegno il perimetro dell'esagono centrato su <coord>
	vec3f vv1 = map.exa_coord_to_world (coord);
	vec3f vv6[6];
	examap::coord_hexagon (vv1, map.get_exa_world_radius(), vv6, sizeof(vv6));

	line_ctx1->
		set_color_ARGB (0xFFFFFF80)
		.set_line_width(2)
		.line_begin()
			.line_add_vtx(vv6[0])
			.line_add_vtx(vv6[1])
			.line_add_vtx(vv6[2])
			.line_add_vtx(vv6[3])
			.line_add_vtx(vv6[4])
			.line_add_vtx(vv6[5])
			.line_add_vtx(vv6[0])
		.line_end();

}