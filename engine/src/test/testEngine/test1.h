#include "gosEngine.h"
#include "entity/gosEntityRegistry.h"
#include "model/gosModel.h"

class Test1
{
public:
			Test1();
			~Test1();
	void	run (gos::Engine *engine);


private:


private:
	void 	priv_model_setup(gos::ENGShape shapeHandle);

private:
	gos::Allocator					*allocator;
	gos::ent::Registry				entRegistry;
	gos::FastArray<gos::Entity>		entList;
	
	gos::Skeleton					*skeleton;
	gos::model::Model				*model;
	gos::model::ModelInstance		*modelInstance;
};