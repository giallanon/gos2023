#include "Land1.h"

using namespace gos;



//***************************************
Land1::Land1()
{
	engine = NULL;
	gpu = NULL;
	renderer = NULL;
}

//***************************************
Land1::~Land1()
{
}

//***************************************
void Land1::setup (gos::Engine *engineIN, gos::GPU *gpuIN, gos::engine::Renderer1 *rendererIN)
{
	engine = engineIN;
	gpu = gpuIN;
	renderer = rendererIN;
}

//***************************************
void Land1::load_assets ()
{
	engine->model_createFromAsset ("model_tile1", &handle__model_tile1, res::eLoadMode::asap);

	//aspetto che sia caricato
	const res::Model3d *res_model;
	engine->get (handle__model_tile1, &res_model, 4000);

	//il modello ha delle shape, voglio sapere quali
	//queste shape sono gia' bindata a VB/IB
	gos::model::Reader mr;
	mr.setup (&res_model->model);
	shape_list = mr.gpushape_get_pt_to_list();





	
}

//***************************************
void Land1::cleanup()
{
	engine->release(handle__model_tile1);
}

//***************************************
void Land1::render()
{
	mat4x4f matW;

	matW.identity();
	renderer->add (shape_list[0], matW, 0);
	renderer->add (shape_list[1], matW, 0);

	matW.buildTranslation (0, 3, 0);
	renderer->add (shape_list[0], matW, 0);
	renderer->add (shape_list[1], matW, 0);
}

