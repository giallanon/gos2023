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
	bool	priv_shape_create(gos::Engine *engine, gos::ENGShape *out_cube, gos::ENGShape *out_cylinder);
	void	priv_model_setup(gos::ENGShape shapeHandle);
	gos::ENGShape 	priv_create_engineShape (GPUStgBufferHandle stgBufferHandle, GPUCmdBufferHandle cmdBufferHandle, const gos::Shape *shapeSRC);

	void	doCPUStuff ();
	bool	priv_run4 ();
	


private:
	gos::Allocator					*allocator;
	gos::ent::Registry				entRegistry;
	
	gos::Engine						*engine;
	gos::GPU						*gpu;
    gos::geom::Camera3				cam;
    gos::FPSMovement				movement;

	
	gos::Skeleton					*skeleton;
	gos::model::Model				*model;

	gos::engine::Renderer1			*renderer;
	u64 nextTimeUpdate_msec;

};