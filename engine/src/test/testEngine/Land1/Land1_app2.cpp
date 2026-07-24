#include "Land1_app2.h"

using namespace gos;



//***************************************
Land1_app2::Land1_app2()
{
	mouse_x = mouse_y = 0;
	material_index_to_apply = 1;
	num_alberi = 0;
}

//***************************************
void Land1_app2::on__unsetup()
{
	engine->release (handle_model_albero);

	for (u32 i=0; i<num_alberi; i++)
		engine->release (modelinst_albero[i]);
}

//***************************************
void Land1_app2::on__setup ()
{
	engine->inputCtx->
		action_add ("mouse_LB")
		.action_add ("mouse_LB+SHIFT")
		.action_add ("KB_1")
		.action_add ("KB_2")
		.action_add ("KB_3")
		.action_add ("KB_0")
		.action_add ("Height++")
		.action_add ("Height--");
		
	engine->inputCtx->action_bindToBtn ("mouse_LB", input::eOrigin::mouse, GOS_BUTTON_MOUSE_LEFT, input::eButtonStatus::pressed);
	engine->inputCtx->action_bindToBtn ("mouse_LB+SHIFT", input::eOrigin::mouse, GOS_BUTTON_MOUSE_LEFT, input::eButtonStatus::pressed, input::eButtonModifier::LSHIFT);
	engine->inputCtx->action_bindToBtn ("KB_1", input::eOrigin::keyboard, GLFW_KEY_1, input::eButtonStatus::pressed);
	engine->inputCtx->action_bindToBtn ("KB_2", input::eOrigin::keyboard, GLFW_KEY_2, input::eButtonStatus::pressed);
	engine->inputCtx->action_bindToBtn ("KB_3", input::eOrigin::keyboard, GLFW_KEY_3, input::eButtonStatus::pressed);
	engine->inputCtx->action_bindToBtn ("KB_0", input::eOrigin::keyboard, GLFW_KEY_0, input::eButtonStatus::pressed);
	engine->inputCtx->action_bindToBtn ("Height++", input::eOrigin::keyboard, GLFW_KEY_KP_ADD, input::eButtonStatus::pressed);
	engine->inputCtx->action_bindToBtn ("Height--", input::eOrigin::keyboard, GLFW_KEY_KP_SUBTRACT, input::eButtonStatus::pressed);


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

	engine->model_createFromAsset ("model_gix_tree_1", &handle_model_albero, res::eLoadMode::asap);
	

	cam.pos.warp (0, 20, 0);
	cam.pos.lookAt(vec3f(0,0,0));
	cam.markUpdated();
	move_free.bind (&cam.pos);

	//creo una mappa
	const f32 EXA_WORLD_RADIUS = 10.0f; //50.0f;
	map.setup (gos::getSysHeapAllocator());
	map.map_create (EXA_WORLD_RADIUS, 187);
	map.exa__add_with_radius ( examap::Coord(0, 0), 4);



	renderer_land->map_attach (&map);
}

//***************************************
void Land1_app2::priv_new_albero (const gos::vec3f &world_point)
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
void Land1_app2::on__prepare_render()
{
	renderer_land->begin();
	{
		renderer_land->add_exa (examap::Coord(0,0));
	
		const u32 MAX_RADIUS = 128;
		const u32 MAX_NUM_COORD = MAX_RADIUS * 6;
		examap::Coord coordList[MAX_NUM_COORD];
		for (u32 ring = 1; ring <= 2; ring++)
		{
			u32 n = examap::coord_ring (examap::Coord(0,0), ring, coordList, MAX_NUM_COORD);
			for (u32 i = 0; i < n; i++)
				renderer_land->add_exa (coordList[i]);
		}
	}
	renderer_land->end();

	//alberi
	renderer_PIPE3->begin();
	{
		for (u32 i=0; i<num_alberi; i++)
			renderer_PIPE3->add (modelinst_albero[i]);
	}
	renderer_PIPE3->end();
}

//***************************************
void Land1_app2::on__handle_input (const Engine::InputEvent &ev)
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

	case COMPILE_TIME_STR_CRC32("Height++"):	material_index_to_apply = 0xFE; break;
	case COMPILE_TIME_STR_CRC32("Height--"):	material_index_to_apply = 0xFD; break;

	}
}

//***************************************
u32 Land1_app2::priv_do_draw_exa (const examap::Coord exa_coord, FastArray<Land1::Map2::Vtx> *out_list)
{
	out_list->reset();
	const u32 num_vtx = map.get_exa_vtxList (exa_coord, *out_list, true);
	if (0 == num_vtx)
		return 0;

	const u32 START_IDX = line_ctx1->vtx_get_num();
	for (u32 i=0; i<out_list->getNElem(); i++)
	{
		//line_ctx1->vtx_add ( vec3f((*out_list)(i).pos.x, 0, (*out_list)(i).pos.y) );
		line_ctx1->vtx_add ( (*out_list)(i).pos );
	}

	for (u32 iVtx=0; iVtx<num_vtx; iVtx++)
	{
		const Land1::Map2::Vtx *vv = &(*out_list)(iVtx);

		for (u32 i=0; i<vv->num_adj_vtx; i++)
		{
			line_ctx1->line (START_IDX + iVtx, START_IDX + vv->adj_vtx_list[i]);
		}
	}

	return num_vtx;
}

//***************************************
void Land1_app2::priv_draw_exa (const gos::vec3f &world_point, bool bLSHIFT)
{
	line_ctx1->clear();
	Land1::GVC gvc;
	if (!map.world_coord_to_GVC (world_point, &gvc))
		return;
	Land1::Map2::Node node;
	map.GVC_to_node (gvc, &node);
	const examap::Coord exa_coord = gvc.get_exa_coord();
	logger::log ("(%d, %d, %d) num_adjc=%d", exa_coord.x, exa_coord.z, gvc.get_vertex_idx(), node.num_adj_vtx);
	for (u32 i=0; i<node.num_adj_vtx; i++)
		logger::log ("  (%d, %d, %d)", node.connected_vtx[i].get_exa_coord().x, node.connected_vtx[i].get_exa_coord().z, node.connected_vtx[i].get_vertex_idx());
	logger::log ("\n");
	

	FastArray<Land1::Map2::Vtx> vtxList (gos::getScrapAllocator(), 1024);

	const u32 N_COLORS = 8;
	const u32 colors[N_COLORS] = { 0xFFFFFFFF, 0xFFFF0000, 0xFF00FF00, 0xFF0000FF, 0xFFFFFF00, 0xFF00FFFF, 0xFFFF00FF, 0xFFa889B1 };

	line_ctx1->enable_depth_test(true);
	line_ctx1->enable_depth_write(false);
	line_ctx1->set_line_width(1);
	line_ctx1->set_color_ARGB (0xFF404040);

	//disegno tutti i vicini dell'exa selezionato
	{
		examap::Coord coord_list[32];
		const u32 n = examap::coord_ring (exa_coord, 1, coord_list, 32);
		for (u32 i = 0; i < n; i++)
			priv_do_draw_exa (coord_list[i], &vtxList);
	}


	//solo per l'exa selezionato....
	line_ctx1->set_color_ARGB (0xFF000000);
	const u32 START_IDX = line_ctx1->vtx_get_num();
	const u32 num_vtx = priv_do_draw_exa (exa_coord, &vtxList);
	

	//disegno i vtx
	line_ctx1
		->point_set_radius(6)
		.set_color_ARGB (0xFF303030);
	for (u32 iVtx=0; iVtx<num_vtx; iVtx++)
	{
		line_ctx1->point (START_IDX + iVtx);	
	}

	//disegno il vtx selezionato
	{
		line_ctx1
			->point_set_radius(20)
			.set_color_ARGB (0xFFFF00FF);
		
		vec3f p;
		map.GVC_to_world_coord (gvc, &p);

		const u32 ii = line_ctx1->vtx_add(p);
		line_ctx1->point (ii);
	}

	//disegno i vtx adiacenti
	line_ctx1->point_set_radius(10);
	for (u32 i = 0; i < node.num_adj_vtx; i++)
	{
		vec3f p;
		map.GVC_to_world_coord (node.connected_vtx[i], &p);

		const u32 ii = line_ctx1->vtx_add(p);
		line_ctx1->set_color_ARGB (colors[i % N_COLORS])
			.point (ii);
	}

	//disegno i quad-center e il vtx "other" che apaprtiene al quad i-esimo
	{
		line_ctx1->point_set_radius(4);
		for (u32 i = 0; i < node.num_adj_vtx; i++)
		{
			const vec3f p (node.quad_center[i].x, (f32)node.height * Land1::Map2::EXA_HEIGHT_MUL, node.quad_center[i].y);
			const u32 ii = line_ctx1->vtx_add(p);
			
			vec3f p2;
			map.GVC_to_world_coord (node.other_vtx[i], &p2);
			const u32 ii2 = line_ctx1->vtx_add(p2);
			
			line_ctx1->set_color_ARGB (colors[i % N_COLORS])
				.point (ii)
				.point (ii2);
			
		}
	}





	if (bLSHIFT)
	{
		switch (material_index_to_apply)
		{
		case 0xFF:
			{
				const vec3f world( node.pos.x, 0, node.pos.y );
				priv_new_albero( world );
			}
			break;

		case 0xFE:
			map.inc_node_height (node.gvc, 100);
			break;

		case 0xFD:
			map.dec_node_height (node.gvc, 100);
			break;

		default:
			map.set_node_material_index (node.gvc, material_index_to_apply);
			break;
		}
	}	

}