#include "gosCtrlFPSMove.h"

using namespace gos;

//*******************************
CtrlFPSMove::CtrlFPSMove ()
{
	set_linear_speed__m_sec (2);
	set_rot_speed__grad_s(180);
	pos = NULL;
}

//*******************************
void CtrlFPSMove::bind (geom::Pos3 *posIN)
{
	pos = posIN;
	last_time_updated__msec = 0;
	rot_x__rad = rot_y__rad = 0;

	f32 z;
	pos->getEulerAngles_YXZ (&rot_y__rad, &rot_x__rad, &z);
	pos->setFromEulerAngles_YXZ (rot_y__rad, rot_x__rad, 0);
}

//*******************************
void CtrlFPSMove::update (u64 timenow_msec, CtrlAction &ctrl)
{
	if (NULL == pos)
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
		if (bm.is_set (eCtrlAction::forward))
			pos->move_rel_along_z (lin_dist);
		else if (bm.is_set (eCtrlAction::backward))
			pos->move_rel_along_z (-lin_dist);		

		if (bm.is_set (eCtrlAction::strafe_right))
			pos->move_rel_along_x (lin_dist);
		else if (bm.is_set (eCtrlAction::strafe_left))
			pos->move_rel_along_x (-lin_dist);


		//rotazione
		if (bm.is_set (eCtrlAction::rot_y_clock))
			rot_y__rad += rot_dist;
		else if (bm.is_set (eCtrlAction::rot_y_counterclock))
			rot_y__rad -= rot_dist;

		while (rot_y__rad > math::DUEPI)			rot_y__rad -= math::DUEPI;
		while (rot_y__rad < -math::DUEPI)			rot_y__rad += math::DUEPI;
				
		pos->setFromEulerAngles_YXZ(rot_y__rad, rot_x__rad, 0);
	}
	ctrl.update_end();
}

