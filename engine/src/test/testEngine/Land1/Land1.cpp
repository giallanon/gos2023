#include "Land1.h"

using namespace gos;



//***************************************
Land1::Land1()
{
	engine = NULL;
	gpu = NULL;
}

//***************************************
void Land1::unsetup()
{
	if (NULL == engine)
		return;

	common.unsetup();
	engine->release(handle__model_tile1);
}

//***************************************
bool Land1::setup (gos::Allocator *allocatorIN, gos::Engine *engineIN)
{
	if (!common.setup(allocatorIN, engineIN, "land1_pipe"))
		return false;

	engine = engineIN;
	gpu = engine->gpu;

	//risorse
	engine->model_createFromAsset ("model_tile1", &handle__model_tile1, res::eLoadMode::asap);

	//aspetto che sia caricato
	const res::Model3d *res_model;
	engine->get (handle__model_tile1, &res_model, 4000);

	//il modello ha delle shape, voglio sapere quali
	//queste shape sono gia' bindata a VB/IB
	gos::model::Reader mr;
	mr.setup (&res_model->model);
	shape_list = mr.gpushape_get_pt_to_list();


	return true;	
}


//***************************************
void Land1::render()
{

}

