#ifndef _gosCtrl3rdPersMove_h_
#define _gosCtrl3rdPersMove_h_
#include "gosCtrlAction.h"

namespace gos
{
    /**************************************************
     * @brief 	Ctrl3rdPersMove
	 * 			muove <entity_pos> lungo gli assi X e Z e ruota solo attorno a Y
	 * 			muove di conseguenza anche <camera_pos> mantenendola ad una distanza di <zoom>.
	 * 			Per <camera_pos> sono attivi anche i controlli zoom-in e soom-out
     * 
    */
    class Ctrl3rdPersMove
    {
    public:
                Ctrl3rdPersMove ();

        void    bind (geom::Pos3 *entity_pos, geom::Camera3 *camera);

        void    set_linear_speed__m_sec (f32 m_sec)				{ lin_speed__m_sec = m_sec; }
        void    set_rot_speed__grad_s (f32 grad_s)				{ rot_speed__rad_s = gos::math::gradToRad(grad_s); }
		
		void 	set_zoom_limits (f32 min_zoom, f32 max_zoom)	{ this->zoom_min=zoom_min; this->zoom_max=zoom_max; set_zoom(zoom); }
		void 	set_zoom (f32 zoom);
		void	camera__set_POV_offset (f32 y)					{ camera_offset_y = y; }
		void	camera__set_rotation_limits (f32 min_grad, f32 max_grad);

		void 	update (u64 timenow_msec, CtrlAction &ctrl);

	public:
		geom::Pos3 	*entity_pos;
	
	private:
		u64		last_time_updated__msec;
		f32		lin_speed__m_sec;
		f32		rot_speed__rad_s;
		f32		rot_x__rad; 
		f32		rot_y__rad;
		f32 	zoom_min, zoom_max, zoom;
		f32		camera_offset_y;
		f32		camera_rot_x__rad;
		f32		camera_min_rot_x__rad, camera_max_rot_x__rad;
		geom::Camera3 	*camera;

	};	
} //namespace gos

#endif // _gosCtrl3rdPersMove_h_

