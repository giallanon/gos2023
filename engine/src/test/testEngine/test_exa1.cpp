#include "test_exa1.h"
#include "gosGeomUtils.h"

using namespace gos;


//***************************************
Test_exa1::Test_exa1()
{
	allocator = gos::getSysHeapAllocator();
	rend_line3d = NULL;

	vtxList.setup (allocator, 1024);
	trisList.setup (allocator, 1024);
	quadList.setup (allocator, 1024);

	line_ctx1.setup (allocator, 512);
	line_ctx2.setup (allocator, 16);
	line_ctx3.setup (allocator, 16);
}

//***************************************
Test_exa1::~Test_exa1()
{
	GOSDELETE(allocator, rend_line3d);
	gpu->deleteResource (handle_rt0);
	gpu->deleteResource (handle_zb);

	vtxList.unsetup ();
	trisList.unsetup ();
	quadList.unsetup ();
}

//***************************************
void Test_exa1::run (gos::Engine *engineIN)
{
	engine = engineIN;
	gpu = engine->gpu;

	//input
	engine->inputCtx->
		action_add ("select_edge_to_remove")
		.action_add ("try_remove_edge")
		.action_add ("simplify_90")
		.action_add ("subdivide")
		.action_add ("restart")
		.action_add ("relax");
		// .action_add ("mouse-LB")
		// .action_add ("toggle_cam_mode");

	//engine->inputCtx->action_bindToAxleREL ("mouse-wheel", input::eOrigin::mouse, input::eAxle::z, input::eAxleDirection::both);
	// engine->inputCtx->action_bindToBtn ("mouse-LB", input::eOrigin::mouse, 0, input::eButtonStatus::pressed);
	engine->inputCtx->action_bindToBtn ("select_edge_to_remove", input::eOrigin::keyboard, GLFW_KEY_SPACE, input::eButtonStatus::pressed);
	engine->inputCtx->action_bindToBtn ("try_remove_edge", input::eOrigin::keyboard, GLFW_KEY_ENTER, input::eButtonStatus::pressed);
	engine->inputCtx->action_bindToBtn ("simplify_90", input::eOrigin::keyboard, GLFW_KEY_F1, input::eButtonStatus::pressed);
	engine->inputCtx->action_bindToBtn ("subdivide", input::eOrigin::keyboard, GLFW_KEY_F2, input::eButtonStatus::pressed);
	engine->inputCtx->action_bindToBtn ("restart", input::eOrigin::keyboard, GLFW_KEY_F3, input::eButtonStatus::pressed);
	engine->inputCtx->action_bindToBtn ("relax", input::eOrigin::keyboard, GLFW_KEY_F4, input::eButtonStatus::pressed);


	//render target & zbuffer
	gpu->renderTarget_create ("100%", "100%", eImageFormat::U8_RGBA, &handle_rt0);
	gpu->zbuffer_create ("0-", "0-", eImageFormat::_DEPTH_BEST, &handle_zb);


	//renderer
	rend_line3d = GOSNEW(allocator, gos::engine::Rend_line3d)();
	rend_line3d->setup (allocator, engine);

	//setup camera
    cam.setPerspectiveFovLH(gpu->swapChain_calcAspectRatio(),  math::gradToRad(45), 0.1f, 250.0f);
    cam.pos.identity();
    cam.pos.warp (0, 0, -10);
	cam.pos.lookAt (vec3f(0,0,0));
	cam.markUpdated();

	//movement
    movement.bind (&cam.pos);

    priv_loop();
}

//**********************************
void Test_exa1::doCPUStuff ()
{
    const u64 timeNow_msec = gos::getTimeSinceStart_msec();

	Engine::InputEvent ev;
	while (engine->inputEvent_getNext(&ev))
	{
		switch (ev.actionID)
		{
		case COMPILE_TIME_STR_CRC32("move_forward"):
            movement.moveForward ((ev.value == 1));
			break;

		case COMPILE_TIME_STR_CRC32("move_backward"):
            movement.moveBackward ((ev.value == 1));
			break;

		case COMPILE_TIME_STR_CRC32("strafe_left"):
			movement.strafeLeft ((ev.value == 1));  
			break;

		case COMPILE_TIME_STR_CRC32("strafe_right"):
			movement.strafeRight ((ev.value == 1));
			break;

		case COMPILE_TIME_STR_CRC32("rotateY"):
			movement.rotateY ((ev.value < 0));
			break;

		case COMPILE_TIME_STR_CRC32("rotateX"):
			movement.rotateX ((ev.value < 0));
			break;

		case COMPILE_TIME_STR_CRC32("strafe_up"):
			movement.strafeUp ((ev.value == 1));
			break;

		case COMPILE_TIME_STR_CRC32("strafe_down"):
			movement.strafeDown ((ev.value == 1));
			break;

		case COMPILE_TIME_STR_CRC32("select_edge_to_remove"):
			select_edge_to_remove(&edge_to_remove);
			break;

		case COMPILE_TIME_STR_CRC32("try_remove_edge"):
			try_remove_edge(edge_to_remove);
			break;

		case COMPILE_TIME_STR_CRC32("simplify_90"):
			{
				logger::log ("tris:%d, quad:%d\n", trisList.getNElem(), quadList.getNElem());
				const u32 tris_limit = (trisList.getNElem() * 15) / 100;
				const u32 max_consecutive_fail = trisList.getNElem() / 2;

				u32 numFail = 0;
				while (trisList.getNElem() > tris_limit)
				{
					sEdgeToRemove edge;
					select_edge_to_remove(&edge);
					if (try_remove_edge(edge))
						numFail = 0;
					else
						numFail++;

					if (numFail > max_consecutive_fail)
						break;
				}

				logger::log ("tris:%d, quad:%d\n", trisList.getNElem(), quadList.getNElem());
			}
			break;

		case COMPILE_TIME_STR_CRC32("subdivide"):
			line_ctx2.clear();
			line_ctx3.clear();
			subdivide();
			break;

		case COMPILE_TIME_STR_CRC32("restart"):
			build_exa();
			build_line_ctx();
			break;			

		case COMPILE_TIME_STR_CRC32("relax"):
			relax();
			build_line_ctx();
			break;

		}
	}

	movement.update(timeNow_msec);
    cam.markUpdated();
}

//***************************************
void Test_exa1::build_exa ()
{
	const vec3f	center(0,0,0);

	PointList exaVtx (allocator, 16);
	FastArray<sRing> ringIndexStartList(allocator, 32);

	vtxList.reset();
	trisList.reset();
	quadList.reset();

	static constexpr u32 NUM_RINGS = 5; //16
	f32 radius = 1;

	//il ring 0 ha solo il vtx centrale
	vtxList.append (center);
	ringIndexStartList.append( sRing{0, 1} );

	//gli altri ring..
	for (u32 ringLevel=1; ringLevel<=NUM_RINGS; ringLevel++)
	{
		//creo un anello di vertixi
		const u32 ring_start_at_index = vtxList.getNElem();
		ringIndexStartList.append( sRing {
				.first_vtx_idx = ring_start_at_index,
				.num_vtx = 6 * ringLevel} );

		exaVtx.reset();
		gos::geom::circle (&exaVtx, center, radius, 6);
		
		//devo aggiungere dei vtx a seconda di quanto esterno e' il ring
		if (1 == ringLevel)
		{
			for (u32 i=0; i<6; i++)
			{
				vtxList.append(exaVtx(i));

				const u32 vtx_idx1 = ring_start_at_index + i;
				u32 vtx_idx2 = vtx_idx1 + 1;
				if (i == 5)
					vtx_idx2 = ring_start_at_index;
				trisList.append (sTris{ (u16)(vtx_idx1), 0, (u16)(vtx_idx2)} );
			}
		}
		else
		{
			const sRing *internal_ring_info = &ringIndexStartList(ringLevel-1);
			const sRing *external_ring_info = &ringIndexStartList(ringLevel);
			exaVtx.append (exaVtx(0));
			for (u32 i=0; i<6; i++)
			{
				//per ogni lato dell'exa, deve generare vtx addizionali in base al ringLevel
				const vec3f vtx1 = exaVtx(i);
				const vec3f vtx2 = exaVtx(i+1);

				vtxList.append(vtx1);
				{
					const f32 tIncr = 1.0f / (f32)ringLevel;
					f32 t = 0;
					for (u32 i2=0; i2< (ringLevel-1); i2++)
					{
						t+=tIncr;
						const vec3f mid = vtx1 + (vtx2-vtx1) * t;
						vtxList.append(mid);
					}
				}
			}
				
			//genero i tris per ogni lato dell'exa
			u32 idxA = external_ring_info->first_vtx_idx;;
			u32 idxB = internal_ring_info->first_vtx_idx;
			for (u32 i=0; i<6; i++)
			{
				u32 idxA1;
				u32 idxB1;
				for (u32 i3=0; i3<ringLevel-1; i3++)
				{
					assert (idxA < external_ring_info->first_vtx_idx + external_ring_info->num_vtx);
					assert (idxB < internal_ring_info->first_vtx_idx + internal_ring_info->num_vtx);

					idxA1 = idxA +1;
					if (idxA1 >= external_ring_info->first_vtx_idx + external_ring_info->num_vtx)
						idxA1 = external_ring_info->first_vtx_idx;

					idxB1 = idxB +1;
					if (idxB1 >= internal_ring_info->first_vtx_idx + internal_ring_info->num_vtx)
						idxB1 = internal_ring_info->first_vtx_idx;

					trisList.append (sTris{ (u16)(idxA), (u16)(idxB), (u16)(idxA1)} );
					trisList.append (sTris{ (u16)(idxA1), (u16)(idxB), (u16)(idxB1)} );
					idxA = idxA1;
					idxB = idxB1;
				}

				if (i == 5)
				{
					idxA1 = external_ring_info->first_vtx_idx;
				}
				else
				{
					idxA1 = idxA +1;
				}

				trisList.append (sTris{ (u16)(idxA), (u16)(idxB), (u16)(idxA1)} );
				idxA = idxA1;
			}
		}

		
		

		radius += 1.0f;
	}


}

//***************************************
vec3f Test_exa1::calc_tris_center(u32 tris_index) const
{
	const vec3f v0 = vtxList(trisList(tris_index).vtx_idx0);
	const vec3f v1 = vtxList(trisList(tris_index).vtx_idx1);
	const vec3f v2 = vtxList(trisList(tris_index).vtx_idx2);

	vec3f ret = (v0 + v1 + v2) / 3.0f;
	return ret;
}

//***************************************
void Test_exa1::select_edge_to_remove (sEdgeToRemove *out)
{
	//logger::log (eTextColor::grey, "select_edge_to_remove... ");

	const u32 nTris = trisList.getNElem();
	out->tris_index = gos::randomU32(nTris - 1);

	switch (gos::randomU32(2))
	{
	default:
		DBGBREAK;
		break;

	case 0:	//edge 0-1
		//logger::log (eTextColor::grey, "edge 0-1\n");
		out->which_edge = 0;
		out->edge_vtx0 = trisList(out->tris_index).vtx_idx0;
		out->edge_vtx1 = trisList(out->tris_index).vtx_idx1;
		break;

	case 1:	//edge 1-2
		//logger::log (eTextColor::grey, "edge 1-2\n");
		out->which_edge = 1;
		out->edge_vtx0 = trisList(out->tris_index).vtx_idx1;
		out->edge_vtx1 = trisList(out->tris_index).vtx_idx2;
		break;

	case 2:	//edge 2-0
		//logger::log (eTextColor::grey, "edge 2-0\n");
		out->which_edge = 2;
		out->edge_vtx0 = trisList(out->tris_index).vtx_idx2;
		out->edge_vtx1 = trisList(out->tris_index).vtx_idx0;
		break;
	}

	line_ctx2.clear();
	line_ctx2.set_color_ARGB (0xFF0000FF);
	line_ctx2.set_line_width(4);
	line_ctx2.line (vtxList(out->edge_vtx0), vtxList(out->edge_vtx1));


	const vec3f c = calc_tris_center(out->tris_index);
	const vec3f cix = vec3f(0.1f, 0, 0);
	const vec3f ciy = vec3f(0, 0.1f, 0);

	line_ctx2.set_line_width(2);
	line_ctx2.line (c, c + cix);
	line_ctx2.line (c, c - cix);
	line_ctx2.line (c, c + ciy);
	line_ctx2.line (c, c - ciy);

}

//***************************************
bool Test_exa1::try_remove_edge (const sEdgeToRemove &edge)
{
	//logger::log (eTextColor::grey, "try_remove_edge...\n");
	line_ctx3.clear();

	//so per certo che e' coinvolto il tris <edge.tris_index>
	//deve essere coinvolto anche un altro tris che condivide lo stesso edge
	const u32 nTris = trisList.getNElem();

	sEdgeToRemove edge2;
	edge2.tris_index = u32MAX;
	u32 vtx_idx_to_add = 0;
	for (u32 i=0; i<nTris; i++)
	{
		//cero un tris con edge <edge.edge_vtx1> <edge.edge_vtx0>
		if (trisList(i).vtx_idx0 == edge.edge_vtx1 && trisList(i).vtx_idx1 == edge.edge_vtx0)
		{
			edge2.tris_index = i;
			edge2.which_edge = 0;
			edge2.edge_vtx0 = trisList(i).vtx_idx0;
			edge2.edge_vtx1 = trisList(i).vtx_idx1;
			vtx_idx_to_add = trisList(i).vtx_idx2;
			break;
		}
		else if (trisList(i).vtx_idx1 == edge.edge_vtx1 && trisList(i).vtx_idx2 == edge.edge_vtx0)
		{
			edge2.tris_index = i;
			edge2.which_edge = 1;
			edge2.edge_vtx0 = trisList(i).vtx_idx1;
			edge2.edge_vtx1 = trisList(i).vtx_idx2;
			vtx_idx_to_add = trisList(i).vtx_idx0;
			break;
		}		
		else if (trisList(i).vtx_idx2 == edge.edge_vtx1 && trisList(i).vtx_idx0 == edge.edge_vtx0)
		{
			edge2.tris_index = i;
			edge2.which_edge = 2;
			edge2.edge_vtx0 = trisList(i).vtx_idx2;
			edge2.edge_vtx1 = trisList(i).vtx_idx0;
			vtx_idx_to_add = trisList(i).vtx_idx1;
			break;
		}		
	}

	if (u32MAX == edge2.tris_index)
	{
		//logger::log ("second tris not found\n");
		return false;
	}

	//logger::log ("edge src:%d, edge dst:%d\n", edge.which_edge, edge2.which_edge);

	//in base all'edge che ho eliminato, creo un quad al posto dei 2 trix
	switch (edge.which_edge)
	{
	default:
		DBGBREAK;
		return false;

	case 0:
		quadList.append (sQuad{
				.vtx_idx0 = trisList(edge.tris_index).vtx_idx1,
				.vtx_idx1 = trisList(edge.tris_index).vtx_idx2,
				.vtx_idx2 = trisList(edge.tris_index).vtx_idx0,
				.vtx_idx3 = (u16)vtx_idx_to_add,
			});
		break;

	case 1:
		quadList.append (sQuad{
				.vtx_idx0 = trisList(edge.tris_index).vtx_idx2,
				.vtx_idx1 = trisList(edge.tris_index).vtx_idx0,
				.vtx_idx2 = trisList(edge.tris_index).vtx_idx1,
				.vtx_idx3 = (u16)vtx_idx_to_add
			});
		break;
		
	case 2:
		quadList.append (sQuad{
				.vtx_idx0 = trisList(edge.tris_index).vtx_idx0,
				.vtx_idx1 = trisList(edge.tris_index).vtx_idx1,
				.vtx_idx2 = trisList(edge.tris_index).vtx_idx2,
				.vtx_idx3 = (u16)vtx_idx_to_add
			});
		break;
	}		


	const vec3f c = calc_tris_center(edge2.tris_index);
	const vec3f cix = vec3f(0.1f, 0, 0);
	const vec3f ciy = vec3f(0, 0.1f, 0);

	line_ctx3.set_color_ARGB (0xFF00FFFF);
	line_ctx3.set_line_width(2);
	line_ctx3.line (c, c + cix);
	line_ctx3.line (c, c - cix);
	line_ctx3.line (c, c + ciy);
	line_ctx3.line (c, c - ciy);	



	if (edge.tris_index > edge2.tris_index)
	{
		trisList.removeAndSwapWithLast(edge.tris_index);
		trisList.removeAndSwapWithLast(edge2.tris_index);
	}
	else
	{
		trisList.removeAndSwapWithLast(edge2.tris_index);
		trisList.removeAndSwapWithLast(edge.tris_index);
	}	

	build_line_ctx();
	line_ctx2.clear();

	return true;
}

//***************************************
u32 Test_exa1::find_in_pointList (const PointList &list, u32 index_start, const vec3f &v_to_be_found) const
{
	static constexpr f32 MAX_DIST = 0.1f * 0.1f;
	const u32 n = list.getNElem();
	while (index_start < n)
	{
		const f32 d = math::distance2(list(index_start), v_to_be_found);
		if (d < MAX_DIST)
			return index_start;
		index_start++;
	}
	return u32MAX;
}

//***************************************
void Test_exa1::subdivide()
{
	if (0 == trisList.getNElem())
		return;

	QuadList 	newQuadList (allocator, quadList.getNElem()*4 + trisList.getNElem()*3);
	
	const u32 num_vtx_before_subdivide = vtxList.getNElem();

	//subdivide dei tris
	u32 n = trisList.getNElem();
	for (u32 i=0; i<n; i++)
	{
		const vec3f v0 = vtxList(trisList(i).vtx_idx0);
		const vec3f v1 = vtxList(trisList(i).vtx_idx1);
		const vec3f v2 = vtxList(trisList(i).vtx_idx2);

		const vec3f v_center = (v0 + v1 + v2) / 3.0f;
		const vec3f v_01 = v0 + (v1-v0) * 0.5f;
		const vec3f v_12 = v1 + (v2-v1) * 0.5f;
		const vec3f v_20 = v2 + (v0-v2) * 0.5f;

		const u32 idx_center = vtxList.getNElem();
		vtxList.append(v_center);

		//riguardo agli altri 3 vtx, potrebbe gia' esistere in quanto creati da altri split
		u32 idx_v01 = find_in_pointList (vtxList, num_vtx_before_subdivide, v_01);
		if (u32MAX == idx_v01)
		{
			idx_v01 = vtxList.getNElem();
			vtxList.append(v_01);
		}

		u32 idx_v12 = find_in_pointList (vtxList, num_vtx_before_subdivide, v_12);
		if (u32MAX == idx_v12)
		{
			idx_v12 = vtxList.getNElem();
			vtxList.append(v_12);
		}
		
		u32 idx_v20 = find_in_pointList (vtxList, num_vtx_before_subdivide, v_20);
		if (u32MAX == idx_v20)
		{
			idx_v20 = vtxList.getNElem();
			vtxList.append(v_20);
		}

		newQuadList.append ( sQuad{
			.vtx_idx0 = (u16)trisList(i).vtx_idx0,
			.vtx_idx1 = (u16)idx_v01,
			.vtx_idx2 = (u16)idx_center,
			.vtx_idx3 = (u16)idx_v20
		});

		newQuadList.append ( sQuad{
			.vtx_idx0 = (u16)idx_v01,
			.vtx_idx1 = (u16)trisList(i).vtx_idx1,
			.vtx_idx2 = (u16)idx_v12,
			.vtx_idx3 = (u16)idx_center,

		});	
		
		newQuadList.append ( sQuad{
			.vtx_idx0 = (u16)idx_v12,
			.vtx_idx1 = (u16)trisList(i).vtx_idx2,
			.vtx_idx2 = (u16)idx_v20,
			.vtx_idx3 = (u16)idx_center
		});			
	}
	trisList.reset();

	//subdivide dei quad
	n = quadList.getNElem();
	for (u32 i=0; i<n; i++)
	{
		const vec3f v0 = vtxList(quadList(i).vtx_idx0);
		const vec3f v1 = vtxList(quadList(i).vtx_idx1);
		const vec3f v2 = vtxList(quadList(i).vtx_idx2);
		const vec3f v3 = vtxList(quadList(i).vtx_idx3);

		const vec3f v_center = (v0 + v1 + v2 + v3) / 4.0f;
		const vec3f v_01 = v0 + (v1-v0) * 0.5f;
		const vec3f v_12 = v1 + (v2-v1) * 0.5f;
		const vec3f v_23 = v2 + (v3-v2) * 0.5f;
		const vec3f v_30 = v3 + (v0-v3) * 0.5f;

		const u32 idx_center = vtxList.getNElem();
		vtxList.append(v_center);

		//riguardo agli altri vtx, potrebbe gia' esistere in quanto creati da altri split
		u32 idx_v01 = find_in_pointList (vtxList, num_vtx_before_subdivide, v_01);
		if (u32MAX == idx_v01)
		{
			idx_v01 = vtxList.getNElem();
			vtxList.append(v_01);
		}

		u32 idx_v12 = find_in_pointList (vtxList, num_vtx_before_subdivide, v_12);
		if (u32MAX == idx_v12)
		{
			idx_v12 = vtxList.getNElem();
			vtxList.append(v_12);
		}
		
		u32 idx_v23 = find_in_pointList (vtxList, num_vtx_before_subdivide, v_23);
		if (u32MAX == idx_v23)
		{
			idx_v23 = vtxList.getNElem();
			vtxList.append(v_23);
		}

		u32 idx_v30 = find_in_pointList (vtxList, num_vtx_before_subdivide, v_30);
		if (u32MAX == idx_v30)
		{
			idx_v30 = vtxList.getNElem();
			vtxList.append(v_30);
		}		

		newQuadList.append ( sQuad{
			.vtx_idx0 = (u16)quadList(i).vtx_idx0,
			.vtx_idx1 = (u16)idx_v01,
			.vtx_idx2 = (u16)idx_center,
			.vtx_idx3 = (u16)idx_v30
		});

		newQuadList.append ( sQuad{
			.vtx_idx0 = (u16)idx_v01,
			.vtx_idx1 = (u16)quadList(i).vtx_idx1,
			.vtx_idx2 = (u16)idx_v12,
			.vtx_idx3 = (u16)idx_center,

		});	
		
		newQuadList.append ( sQuad{
			.vtx_idx0 = (u16)idx_v12,
			.vtx_idx1 = (u16)quadList(i).vtx_idx2,
			.vtx_idx2 = (u16)idx_v23,
			.vtx_idx3 = (u16)idx_center
		});			

		newQuadList.append ( sQuad{
			.vtx_idx0 = (u16)idx_v23,
			.vtx_idx1 = (u16)quadList(i).vtx_idx3,
			.vtx_idx2 = (u16)idx_v30,
			.vtx_idx3 = (u16)idx_center
		});			

	}
	quadList.reset();



	quadList.copyFrom (newQuadList);
	build_line_ctx();
}

//***************************************
void Test_exa1::quad_get_vertex (u32 quadIndex, vec3f *out) const
{
	out[0] = vtxList(quadList(quadIndex).vtx_idx0);
	out[1] = vtxList(quadList(quadIndex).vtx_idx1);
	out[2] = vtxList(quadList(quadIndex).vtx_idx2);
	out[3] = vtxList(quadList(quadIndex).vtx_idx3);
}

//***************************************
void Test_exa1::relax()
{
	logger::log (eTextColor::white, "relaxing...\n");
	struct Info
	{
		u32		vtx_index[4];
		f32 	side_len[4];	//lunghezza dei 4 lati
		vec3f	center;
		f32 	angles_rad[4];
		u8		indexof_worst_angle;
		f32 	worst_dist_from_PIMEZZI;
		f32 	score;
	};

	FastArray<Info> infoList(allocator, quadList.getNElem());
	const u32 n = quadList.getNElem();
	for (u32 i=0; i<n; i++)
	{
		vec3f	vtx[4];
		quad_get_vertex (i, vtx);

		const vec3f A = vtx[1] - vtx[0];
		const vec3f B = vtx[2] - vtx[1];
		const vec3f C = vtx[3] - vtx[2];
		const vec3f D = vtx[0] - vtx[3];

		Info info;
		info.vtx_index[0] = quadList(i).vtx_idx0;
		info.vtx_index[1] = quadList(i).vtx_idx1;
		info.vtx_index[2] = quadList(i).vtx_idx2;
		info.vtx_index[3] = quadList(i).vtx_idx3;
		info.center = (vtx[0] + vtx[1] + vtx[2] + vtx[3]) / 4.0f;
		info.side_len[0] = A.length();
		info.side_len[1] = B.length();
		info.side_len[2] = C.length();
		info.side_len[3] = D.length();

		info.angles_rad[0] = acosf( math::dot(A,D) / (info.side_len[0] * info.side_len[3]) );
		info.angles_rad[1] = acosf(math::dot(A,B) / (info.side_len[0] * info.side_len[1]) );
		info.angles_rad[2] = acosf(math::dot(B,C) / (info.side_len[1] * info.side_len[2]) );
		info.angles_rad[3] = acosf(math::dot(C,D) / (info.side_len[2] * info.side_len[3]) );


		//distanza dei singoli angoli dai 90 gradi
		info.indexof_worst_angle = 0;
		info.worst_dist_from_PIMEZZI = fabsf (math::PIMEZZI - info.angles_rad[0]);

		info.score = info.worst_dist_from_PIMEZZI;
		for (u32 i2=1; i2<4; i2++)
		{
			const f32 d = fabsf (math::PIMEZZI - info.angles_rad[i2]);
			info.score += d;
			if (d > info.worst_dist_from_PIMEZZI)
			{
				info.worst_dist_from_PIMEZZI = d;
				info.indexof_worst_angle = i2;
			}
		}

		infoList.append(info);

		logger::log ("%03d (%.2f %.2f: %.2f) => %.2f %.2f %.2f %.2f, worst=%d\n", 
						i, info.center.x, info.center.y, info.score, 
						math::radToGrad(info.angles_rad[0]), 
						math::radToGrad(info.angles_rad[1]), 
						math::radToGrad(info.angles_rad[2]), 
						math::radToGrad(info.angles_rad[3]),
					info.indexof_worst_angle);
	}


	//cero un candidato
	// u32 candidato = gos::randomU32(n-1);
	// for (u32 i=0; i<n; i++)
	// {
	// 	const Info *info_candidato = &infoList(candidato);

	// 	const Info *info = &infoList(i);
	// 	if (info->score > info_candidato->score)
	// 		candidato = i;
	// }


	// const Info *info = &infoList(candidato);
	// logger::log ("candidato %d\n", candidato);


	for (u32 candidato=0; candidato<n; candidato++)
	{
		const Info *info = &infoList(candidato);

		u32 idx_vtx_centrale;
		u32 idx_vtx_da_muovere1;
		u32 idx_vtx_da_muovere2;
		switch (info->indexof_worst_angle)
		{
		default: DBGBREAK; return;
		case 0:	idx_vtx_centrale = 2; idx_vtx_da_muovere1=1; idx_vtx_da_muovere2 = 3; break;
		case 1:	idx_vtx_centrale = 3; idx_vtx_da_muovere1=0; idx_vtx_da_muovere2 = 2; break;
		case 2:	idx_vtx_centrale = 0; idx_vtx_da_muovere1=1; idx_vtx_da_muovere2 = 3; break;
		case 3:	idx_vtx_centrale = 1; idx_vtx_da_muovere1=0; idx_vtx_da_muovere2 = 2; break;
		}

		const vec3f vv0 = vtxList(info->vtx_index[idx_vtx_centrale]);
		const vec3f vv1 = vtxList(info->vtx_index[idx_vtx_da_muovere1]);
		const vec3f vv2 = vtxList(info->vtx_index[idx_vtx_da_muovere2]);
		if (info->angles_rad[info->indexof_worst_angle] > math::PIMEZZI)
		{
			//devo incrementare l'angolo
			vtxList[info->vtx_index[idx_vtx_da_muovere1]] = vv0 + (vv1-vv0) * 1.05f;
			vtxList[info->vtx_index[idx_vtx_da_muovere2]] = vv0 + (vv2-vv0) * 1.05f;
		}
		else
		{
			//devo diminuire l'angolo
			vtxList[info->vtx_index[idx_vtx_da_muovere1]] = vv0 + (vv1-vv0) * 0.95f;
			vtxList[info->vtx_index[idx_vtx_da_muovere2]] = vv0 + (vv2-vv0) * 0.95f;
		}
	}

	logger::log ("done\n");
}

//***************************************
void Test_exa1::build_line_ctx ()
{
	line_ctx1.clear();

	vtxList.forEach( [&line_ctx1=this->line_ctx1](u32 index, const vec3f &v){
		line_ctx1.vtx_add(v);
		return true;
	});

	line_ctx1.set_line_width(2);
	trisList.forEach ( [&line_ctx1=this->line_ctx1, &vtxList=this->vtxList](u32 index, const sTris &tris){
		line_ctx1.line_begin();
		line_ctx1.line_add_vtx (tris.vtx_idx0);
		line_ctx1.line_add_vtx (tris.vtx_idx1);
		line_ctx1.line_add_vtx (tris.vtx_idx2);
		line_ctx1.line_add_vtx (tris.vtx_idx0);
		line_ctx1.line_end();
		return true;
	});

	line_ctx1.set_line_width(3);
	line_ctx1.set_color_ARGB (0xFF00FF00);
	quadList.forEach ( [&line_ctx1=this->line_ctx1, &vtxList=this->vtxList](u32 index, const sQuad &quad){
		line_ctx1.line_begin();
		line_ctx1.line_add_vtx (quad.vtx_idx0);
		line_ctx1.line_add_vtx (quad.vtx_idx1);
		line_ctx1.line_add_vtx (quad.vtx_idx2);
		line_ctx1.line_add_vtx (quad.vtx_idx3);
		line_ctx1.line_add_vtx (quad.vtx_idx0);
		line_ctx1.line_end();
		return true;
	});


	logger::log (eTextColor::white, "cur graph: vtx=%d, tris=%d, quad=%d\n", vtxList.getNElem(), trisList.getNElem(), quadList.getNElem());
}

//***************************************
void Test_exa1::priv_loop ()
{
	build_exa();
	build_line_ctx();


	logger::log (eTextColor::magenta, "num vtx=%d, num tris=%d\n", vtxList.getNElem(), trisList.getNElem());


    //loop
    gpu::MainLoop2 mainLoop;
    mainLoop.setup (gpu);
	mainLoop.stat_setPrintReportEvery(u32MAX);

    GPUCmdBufferHandle  cmdBufferHandle;
    gpu->cmdBuffer_create (eGPUQueueFamily::gfx, &cmdBufferHandle);
	

	bool bQuit = false;
	while (false == bQuit)
	{
		if (!engine->update())
		{
			bQuit = true;
			continue;
		}

		mainLoop.run();

        //CPU jobs
		mainLoop.stat_onCPUFrameBegin();
		{
			doCPUStuff();
        }
		mainLoop.stat_onCPUFrameEnd();		


		//rendering
        if (gpu->swapChain_wasRecreated())
            cam.changeAspectRatioPerspectiveFovLH (gpu->swapChain_calcAspectRatio());

        //se il job precedente e' stato presentato, posso schedularne uno nuovo
        gpu::SwapchainImg swapchainImg;
        if (mainLoop.gfxJob_canSubmit(&swapchainImg))
        {
			gos::gpu::CmdBufferWriter2 cw;
			cw	.begin (gpu, cmdBufferHandle)
				.setViewport (gpu->viewport_getDefault())
				.imageTransition (handle_rt0, eImageLayout::undefined, eImageLayout::color_attachment_optimal)
				.imageTransition (handle_zb, eImageLayout::undefined, eImageLayout::depth_attachment_optimal);

			mainLoop.stat_onCommandBufferBegin();
			{
				gpu::RenderCtx rctx;
				cw	.renderCtx_define_begin(&rctx)
					.withRenderArea (handle_rt0)
					.withRT (handle_rt0, eAttachmentLoadOp::clear, eAttachmentStoreOp::dont_care, gos::ColorHDR(0, 0.1f, 0.1f))
					.withZB (handle_zb, eAttachmentLoadOp::clear, eAttachmentStoreOp::dont_care)
					.define_end();
			
				rend_line3d->begin (&cam, &rctx);
					rend_line3d->appendToCommandBuffer (line_ctx1);
					rend_line3d->appendToCommandBuffer (line_ctx2);
					rend_line3d->appendToCommandBuffer (line_ctx3);
				rend_line3d->end();

				rctx.end_render_ctx();
			}
			mainLoop.stat_onCommandBufferEnd();


			//present
			cw	.imageTransition (handle_rt0, eImageLayout::color_attachment_optimal, eImageLayout::transfer_src)
				.imageTransition (swapchainImg.image, eImageLayout::undefined, eImageLayout::transfer_dst)
				.copyImageToImage (handle_rt0, swapchainImg.image, gpu->swapChain_getImageExten2D(), gpu->swapChain_getImageExten2D())
				.imageTransition (swapchainImg.image, eImageLayout::transfer_dst, eImageLayout::presentation)
				.end();

			mainLoop.gfxJob_submitAndPresent (cmdBufferHandle, swapchainImg);
        }		
	}

	//free
	gpu->waitIdle();
	mainLoop.unsetup();

	gpu->deleteResource (cmdBufferHandle);
}

