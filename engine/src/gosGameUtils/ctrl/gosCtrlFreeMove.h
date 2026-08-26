#ifndef _gosCtrlFreeMove_h_
#define _gosCtrlFreeMove_h_
#include "gosCtrlAction.h"

namespace gos
{
    /**************************************************
     * @brief 	CtrlFreeMove
	 * 			muove <pos> lungo tutti gli assi e ruota attorno x e y
     * 
    */
    class CtrlFreeMove
    {
    public:
                CtrlFreeMove ();

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

#endif // _gosCtrlFreeMove_h_
