#include "gosEngine.h"
#include "gosEngine_renderer.h"
#include "entity/gosEntityRegistry.h"
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
	bool	priv_shape_create(gos::Engine *engine, gos::ENGShape *out);
	void	priv_model_setup(gos::ENGShape shapeHandle);

	void	doCPUStuff ();
	void	priv_run1 ();
	bool	priv_run2 ();
	bool	priv_run3 ();
	


private:
	gos::Allocator					*allocator;
	gos::ent::Registry				entRegistry;
	gos::FastArray<gos::Entity>		entList;
	
	gos::Engine						*engine;
	gos::GPU						*gpu;
    gos::geom::Camera3				cam;
    gos::FPSMovement				movement;

	
	gos::Skeleton					*skeleton;
	gos::model::Model				*model;

	gos::engine::Renderer			*renderer;
	f32								obj0_roty;
};