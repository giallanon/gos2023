#ifndef _gosCtrlAction_h_
#define _gosCtrlAction_h_
#include "../gosMath/gosMath.h"
#include "../gosGeom/gosGeomCamera3.h"

namespace gos
{
	enum class eCtrlAction : u16
	{
		forward 			= 0x0001,
		backward 			= 0x0002,
		strafe_left 		= 0x0004,
		strafe_right 		= 0x0008,
		strafe_up 			= 0x0010,
		strafe_down			= 0x0020,
		rot_x_clock			= 0x0040,
		rot_x_counterclock	= 0x0080,
		rot_y_clock			= 0x0100,
		rot_y_counterclock	= 0x0200,
		zoom_in				= 0x0400,
		zoom_out			= 0x0800,
	};

	GOS_DECL_ENUM_BITMASK_CLASS(eCtrlAction);

    /**************************************************
     * @brief 	CtrlAction
	 * 			tramite set() e clear() si impostano le azioni che derivano dai vari dispositivi di input (kb & mouse per esempio).
	 * 			Altre classi, tipo CtrlFreeMove() prendono in input questa classe e la usano per muovere fisicamente un oggetto
     * 
    */
    class CtrlAction
    {
    public:
					CtrlAction()										{ zero(); }

		void 		zero()												{ bm.zero(); bm_oneshot.zero(); }
		void 		set (eCtrlAction action, bool one_shot = false)		{ bm.bit_set (action); if (one_shot) bm_oneshot.bit_set (action); }

		void 		clear (eCtrlAction action)							{ bm.bit_clear (action); }

        eCtrlActionBitmask	update_begin() const 						{ return bm; }
		void 		update_end()										{ const u32 mask = bm_oneshot.asU32(); u32 m = bm.asU32(); m &= (~mask); bm.set_from_U32 (m); bm_oneshot.zero(); }		

	private:
		eCtrlActionBitmask bm;
		eCtrlActionBitmask bm_oneshot;
	};


} //namespace gos

#endif //_gosCtrlAction_h_