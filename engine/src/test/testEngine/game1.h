#ifndef _game1_h_
#define _game1_h_

#include "gosEngine.h"
#include "renderPipe/gosEngineRenderPipe_PIPE3.h"
#include "renderPipe/gosEngineRenderPipe_line3d.h"
#include "entity/gosEntity.h"
#include "model/gosModel.h"
#include "../gosGPU/utils/gosFreeMovement.h"
#include "../gosGPU/utils/gosFPSMovement.h"
#include "characterController.h"

class Game1
{
public:
					Game1();
					~Game1();
	void			run (gos::Engine *engine);

private:
	static constexpr u8 NUM_MAX_MISSILE = 12;

private:
	enum class eCameraMode : u8
	{
		third_person = 0,
		free_cam = 1
	};


private:
	void			doCPUStuff ();
    bool    		priv_loadAssets();
    void    		priv_loop();
	bool 			priv_createShapes();
	void 			priv_createModel_mainPlayer();
	void 			priv_createModel_pavimento();
	void			priv_spawnMissile (const gos::vec3f &o, const gos::vec3f dir);


private:
	gos::Allocator					*allocator;
	gos::Engine						*engine;
	gos::GPU						*gpu;
	gos::ent::Registry				entRegistry;
    gos::geom::Camera3				cam;

	gos::engine::Renderer_PIPE3		*renderer_PIPE3;
	gos::engine::Renderer_line3d	*renderer_line3d;

	gos::ENGGPUShape				handle_gpushape_cube;
	gos::ENGGPUShape				handle_gpushape_cyl;
	gos::ENGTexture					handle_texChecker;
    u32								material_indices[4];

	eCameraMode						cameraMode;
	gos::ENGSkeleton				handle_skeleton1;
	gos::ENGSkeleton				handle_skeleton2;
	gos::ENGModel3d					handle_model_player;
	gos::ENGModel3d					handle_model_pavimento;
    gos::FPSMovement				movement;
	CharacterController				charCtrl;
	gos::Entity						ent_mainPlayer;
	gos::Entity						ent_missile[NUM_MAX_MISSILE];
	gos::Entity						ent_cubi[3];
	u8								num_missile_alive;

	gos::ENGModel3d handle_model_albero;
};

#endif //_game1_h_