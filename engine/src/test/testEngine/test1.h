#include "gosEngine.h"
#include "gosEngine_renderer.h"
#include "entity/gosEntity.h"
#include "model/gosModel.h"
#include "../gosGPU/utils/gosFreeMovement.h"
#include "../gosGPU/utils/gosFPSMovement.h"


class Test1
{
public:
			Test1();
			~Test1();
	void	run (gos::Engine *engine);


private:


private:
	bool	priv_shape_create(gos::Engine *engine, gos::ENGGPUShape *out_cube, gos::ENGGPUShape *out_cylinder);
	void	priv_model_setup(gos::ENGGPUShape shape_cube, gos::ENGGPUShape shape_cylinder);

	void	doCPUStuff ();
	bool	priv_run4 ();
	


private:
	gos::Allocator					*allocator;
	gos::ent::Registry				entRegistry;
	
	gos::Engine						*engine;
	gos::GPU						*gpu;
    gos::geom::Camera3				cam;
    gos::FPSMovement				movement;

	
	gos::ENGSkeleton				handle_skeleton;
	gos::ENGModel3d					handle_model;

	gos::engine::Renderer1			*renderer;
	u64 nextTimeUpdate_msec;

};