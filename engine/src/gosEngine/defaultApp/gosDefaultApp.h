#ifndef _gosDefaultApp_h_
#define _gosDefaultApp_h_
#include "../gosEngine.h"
#include "../../gosAsset2/gosAsset2MonitorClient.h"
#include "../renderPipe/gosEngineRenderPipe_PIPE3.h"
#include "../../gosGameUtils/ctrl/gosCtrlFreeMove.h"
#include "../../gosGameUtils/ctrl/gosCtrlFPSMove.h"
#include "../../gosGameUtils/ctrl/gosCtrl3rdPersMove.h"

namespace gos
{
	class Engine;

	namespace engine
	{	
		/************************************
		 * @brief 	DefaultApp
		 * 
		 */
		class DefaultApp
		{
		public:
							DefaultApp();
			virtual			~DefaultApp();

			void			enable_asset_monitor()									{ bEnableAssetMonitor = true; }
			void			run (gos::Engine *engine);

		protected:
			static constexpr u8	NAV_MODE__DEFAULT_CAMERA = 0;

		protected:
			virtual void	on__setup()												{ }
			virtual void	on__handle_input (const gos::Engine::InputEvent &ev)	{ }
			virtual void	on__navigation_mode_changed (u8 mode_uid)				{ }
			virtual void	on__update(u64 timenow_msec)							{ }
			virtual void	on__render()											{ }
			virtual void	on__unsetup()											{ }

			geom::Camera3*	camera__create (u32 index, f32 fov_grad, f32 near_plane, f32 far_plane);
			geom::Camera3*	camera__get (u32 index);
			void			camera__set_render_camera (u32 index)					{ render_cam = camera__get (index); }
			geom::Camera3*	camera__get_render_camera()								{ return render_cam; }

			void			navigation__create_mode (u8 mode_uid);
			void 			navigation__set_mode (u8 mode_uid);
			u8				navigation__get_mode() const							{ return nav.uid[nav.current_index]; }

		protected:
			gos::Allocator	*allocator;
			gos::Engine		*engine;
			gos::GPU		*gpu;

			CtrlAction		ctrl_action;
			CtrlFreeMove	ctrl_default_cam;

		private:
			static constexpr u8	CAMERA__NUM_MAX = 8;
			static constexpr u8	NAVIGATION__NUM_MAX_MODE = 8;

		private:
			struct CameraInfo
			{
				static const u8 	FLAG_CREATED = 0;

				gos::geom::Camera3	cam;
				gos::Flag8			flag;
			};

			struct Navigation
			{
				u8	uid[NAVIGATION__NUM_MAX_MODE];
				u8	num;
				u8	current_index;
			};

		private:
			void			default_handle_input ();
			void    		priv_loop();

		private:
			bool			bEnableAssetMonitor;
			CameraInfo		cam_list[CAMERA__NUM_MAX];
			geom::Camera3	*render_cam;
			u8				bShowCamPos;

			Navigation		nav;
		};

	} //namespace engine
} //namespace gos

#endif //_gosDefaultApp_h_

