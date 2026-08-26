#include "gosCtrl3rdPersMove.h"

using namespace gos;

//*******************************
Ctrl3rdPersMove::Ctrl3rdPersMove ()
{
	set_linear_speed__m_sec (2);
	set_rot_speed__grad_s(180);
	
	camera__set_rotation_limits (2, 80);
	camera_rot_x__rad = -math::gradToRad(45);
	camera_offset_y = 0;
	zoom_min = 0.1f;
	zoom_max = 1000;
	zoom = 10;
	entity_pos = NULL;
	camera = NULL;
}

//*******************************
void Ctrl3rdPersMove::bind (geom::Pos3 *entity_posIN, geom::Camera3 *cameraIN)
{
	assert (NULL != entity_posIN);
	assert (NULL != cameraIN);

	entity_pos = entity_posIN;
	camera = cameraIN;
	last_time_updated__msec = 0;

	f32 z;
	entity_pos->getEulerAngles_YXZ (&rot_y__rad, &rot_x__rad, &z);
	entity_pos->setFromEulerAngles_YXZ (rot_y__rad, rot_x__rad, 0);
}

//*******************************
void Ctrl3rdPersMove::camera__set_rotation_limits (f32 min_grad, f32 max_grad)
{
	camera_min_rot_x__rad = -math::gradToRad(min_grad);
	camera_max_rot_x__rad = -math::gradToRad(max_grad);
	if (camera_min_rot_x__rad > camera_max_rot_x__rad)
		GOSSWAP(camera_min_rot_x__rad, camera_max_rot_x__rad);
}

//*******************************
void Ctrl3rdPersMove::set_zoom (f32 zoomIN)
{
	zoom = zoomIN;
	if (zoom < zoom_min)	zoom = zoom_min;
	if (zoom > zoom_max)	zoom = zoom_max;
}

//*******************************
void Ctrl3rdPersMove::update (u64 timenow_msec, CtrlAction &ctrl)
{
	if (NULL == entity_pos)
		return;
	if (0 == last_time_updated__msec)
		last_time_updated__msec = timenow_msec;
    
	const u64 time_elapsed__msec = (timenow_msec - last_time_updated__msec);
	if (time_elapsed__msec < 10)
		return;

	const f32 time_elapsed__sec = (f32)time_elapsed__msec / 1000.0f;
	const f32 lin_dist = lin_speed__m_sec * time_elapsed__sec;
	const f32 rot_dist = rot_speed__rad_s * time_elapsed__sec;

	last_time_updated__msec = timenow_msec;

	const eCtrlActionBitmask bm = ctrl.update_begin();
	{
		//movimento entity
		if (bm.is_set (eCtrlAction::forward))
			entity_pos->move_rel_along_z (lin_dist);
		else if (bm.is_set (eCtrlAction::backward))
			entity_pos->move_rel_along_z (-lin_dist);		

		if (bm.is_set (eCtrlAction::strafe_right))
			entity_pos->move_rel_along_x (lin_dist);
		else if (bm.is_set (eCtrlAction::strafe_left))
			entity_pos->move_rel_along_x (-lin_dist);

		if (bm.is_set (eCtrlAction::strafe_up))
			entity_pos->move_rel_along_y (lin_dist);
		else if (bm.is_set (eCtrlAction::strafe_down))
			entity_pos->move_rel_along_y (-lin_dist);

		//rotazione entity
		if (bm.is_set (eCtrlAction::rot_y_clock))
			rot_y__rad += rot_dist;
		else if (bm.is_set (eCtrlAction::rot_y_counterclock))
			rot_y__rad -= rot_dist;

		while (rot_y__rad > math::DUEPI)			rot_y__rad -= math::DUEPI;
		while (rot_y__rad < -math::DUEPI)			rot_y__rad += math::DUEPI;
				
		entity_pos->setFromEulerAngles_YXZ(rot_y__rad, rot_x__rad, 0);



		//movimento camera
		if (bm.is_set (eCtrlAction::zoom_in))
			set_zoom (zoom * 0.9f);
		else if (bm.is_set (eCtrlAction::zoom_out))
			set_zoom (zoom * 1.1f);

		//rotazione camera
		if (bm.is_set (eCtrlAction::rot_x_clock))
			camera_rot_x__rad += rot_dist;
		else if (bm.is_set (eCtrlAction::rot_x_counterclock))
			camera_rot_x__rad -= rot_dist;
		if (camera_rot_x__rad < camera_min_rot_x__rad)		camera_rot_x__rad = camera_min_rot_x__rad;
		else if (camera_rot_x__rad > camera_max_rot_x__rad)	camera_rot_x__rad = camera_max_rot_x__rad;

		camera->pos = *entity_pos;
		camera->pos.move_rel_along_y (camera_offset_y);
		camera->pos.rotate_me_about_my_x (camera_rot_x__rad);
		camera->pos.move_rel_along_z (-zoom);
		camera->mark_updated();

	}
	ctrl.update_end();
}

