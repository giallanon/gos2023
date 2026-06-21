#include "Land1_app.h"

using namespace gos;



//***************************************
Land1_app::Land1_app()
{
	renderer1 = NULL;
	mouse_x = mouse_y = 0;
}

//***************************************
Land1_app::~Land1_app()
{
}

//***************************************
void Land1_app::on__load_assets ()
{
	engine->inputCtx->
		action_add ("mouse_LB")
		.action_bindToBtn ("mouse_LB", input::eOrigin::mouse, GOS_BUTTON_MOUSE_LEFT, input::eButtonStatus::pressed);



	renderer_land = renderPipe.add_renderer<Land1>();
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

	engine->model_createFromAsset ("model_LowPolyTree", &handle_model_albero, res::eLoadMode::asap);
	{
		const res::Model3d *res_model_albero;
		if (!engine->get (handle_model_albero, &res_model_albero, 4000))
		{
			DBGBREAK;
			return;
		}

		renderer1->begin();
		{
			for (u32 i=0; i<NUM_ALBERI; i++)
			{
				engine->modelinst_create (handle_model_albero, &modelinst_albero[i]);

				mat4x4f matW;

				const f32 AA = 60.0f;
				matW.buildTranslation ( gos::random(-AA, AA), 0, gos::random(-AA, AA));
				engine->modelinst_applyTransform (modelinst_albero[i], matW);
				renderer1->add (modelinst_albero[i]);
			}
		}
		renderer1->end();
		
	}


	cam.pos.warp (0, 20, 0);
	cam.pos.lookAt(vec3f(0,0,0));
	cam.markUpdated();
	move_free.bind (&cam.pos);
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

	case COMPILE_TIME_STR_CRC32("mouse_LB"):
		{
			const gpu::Viewport *vp = gpu->getInfo (gpu->viewport_getDefault());
			vec2f m(mouse_x, mouse_y);
			vec3f world_dir;
			cam.unproject (vp->getW_f32(), vp->getH_f32(), &m , &world_dir, 1);

			line_ctx1->clear();
			line_ctx1->enable_depth_test(true);
			line_ctx1->enable_depth_write(false);
			line_ctx1->set_line_width(2);

			u32 v0 = line_ctx1->vtx_add (cam.pos.o);
			u32 v1 = line_ctx1->vtx_add (cam.pos.o + cam.pos.getAsseZ() * 100.0f);
			u32 v2 = line_ctx1->vtx_add (cam.pos.o - cam.pos.getAsseY() * 100.0f);
			u32 v3 = line_ctx1->vtx_add (cam.pos.o + world_dir * 100.0f);

			line_ctx1->set_color_ARGB (0xFFFF00FF);
			line_ctx1->line (v0, v1);
			line_ctx1->line (v0, v2);

			line_ctx1->set_color_ARGB (0xFFFFFF00);
			line_ctx1->line (v0, v3);

			//supponendo che hex sia sempre ad altezza y=0...
			const f32 t = -cam.pos.o.y / world_dir.y;
			vec3f point_on_hex = cam.pos.o + world_dir * t;

			u32 v4 = line_ctx1->vtx_add (point_on_hex);
			line_ctx1->point_set_radius(6);
			line_ctx1->enable_depth_test(false);
			line_ctx1->point (v4);

			logger::log (eTextColor::white, "coord on hex: %.2f, %.2f, %.2f\n", point_on_hex.x, point_on_hex.y, point_on_hex.z);

			HexMap hexmap;
			{
				hexmap.world__set_information (vec3f(0,0,0), Land1::HEX_RADIUS);
			
				HexMap::Coord c = hexmap.world_coord_to_hex (point_on_hex);

				logger::log (eTextColor::white, "hex: %d, %d\n", c.x, c.z);


				vec3f vv1 = hexmap.hex_coord_to_world (c);

				vec3f vv6[6];
				hexmap.coord_hexagon (vv1, Land1::HEX_RADIUS, vv6, sizeof(vv6));

				line_ctx1->
					set_color_ARGB (0xFFFF00)
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
		}
		break;
	}
}


//***************************************
void Land1_app::on__cleanup()
{
	engine->release (handle_texBianca);
	engine->release (handle_model_albero);

	for (u32 i=0; i<NUM_ALBERI; i++)
		engine->release (modelinst_albero[i]);
}
