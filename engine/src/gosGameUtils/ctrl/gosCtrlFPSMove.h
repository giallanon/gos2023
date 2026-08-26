#ifndef _gosCtrlFPSMove_h_
#define _gosCtrlFPSMove_h_
#include "gosCtrlAction.h"

namespace gos
{
    /**************************************************
     * @brief 	CtrlFPSMove
	 * 			muove <pos> lungo gli assi X e Z e ruota solo attorno a Y
     * 
    */
    class CtrlFPSMove
    {
    public:
                CtrlFPSMove ();

        void    bind (geom::Pos3 *posIN);

        void    set_linear_speed__m_sec (f32 m_sec)			{ lin_speed__m_sec = m_sec; }
        void    set_rot_speed__grad_s (f32 grad_s)			{ rot_speed__rad_s = gos::math::gradToRad(grad_s); }

		void 	update (u64 timenow_msec, CtrlAction &ctrl);

	public:
		geom::Pos3 	*pos;

	
	private:
		u64		last_time_updated__msec;
		f32		lin_speed__m_sec;
		f32		rot_speed__rad_s;
		f32		rot_x__rad; 
		f32		rot_y__rad;

	};	
} //namespace gos

#endif // _gosCtrlFPSMove_h_

