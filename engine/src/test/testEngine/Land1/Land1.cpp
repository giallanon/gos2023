#include "Land1.h"

using namespace gos;



//***************************************
Land1::Land1()
{
}

//***************************************
Land1::~Land1()
{
}

//***************************************
void Land1::on__handle_input ()
{
}

//***************************************
void Land1::on__load_assets ()
{
	engine->model_createFromAsset ("model_tile1", &handle__model_tile1, res::eLoadMode::asap);

	//aspetto che sia caricato
	const res::Model3d *res_model;
	engine->get (handle__model_tile1, &res_model, 4000);

	for (u32 i=0; i<SQUARE_SIZE*SQUARE_SIZE; i++)
		engine->modelinst_create (handle__model_tile1, &handle__modelinst_tile1[i]);


	u32 ct = 0;
	f32 zz = 1.0f * (SQUARE_SIZE/2);
	for (u32 z=0; z<SQUARE_SIZE; z++)
	{
		f32 xx = -1.0f * (SQUARE_SIZE/2);
		for (u32 x=0; x<SQUARE_SIZE; x++)
		{
			f32 yy = 0;
			switch (gos::randomU32(2))
			{
			default: break;
			case 1: yy = 0.04f; break;
			case 2: yy = -0.04f; break;
			}

			mat4x4f matW;
			matW.buildTranslation (vec3f(xx, yy, zz));
			engine->modelinst_applyTransform (handle__modelinst_tile1[ct++], matW);
			xx += 1.0f;
		}
		zz -= 1.0f;
	}



	engine->release(handle__model_tile1);
}

//***************************************
void Land1::on__cleanup()
{
	for (u32 i=0; i<SQUARE_SIZE*SQUARE_SIZE; i++)
		engine->release(handle__modelinst_tile1[i]);
}

//***************************************
void Land1::on__render()
{
	for (u32 i=0; i<SQUARE_SIZE*SQUARE_SIZE; i++)
		renderer->add (handle__modelinst_tile1[i]);
}

