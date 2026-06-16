#include "Land1_app.h"

using namespace gos;



//***************************************
Land1_app::Land1_app()
{
}

//***************************************
Land1_app::~Land1_app()
{
}

//***************************************
void Land1_app::on__handle_input ()
{
}

//***************************************
void Land1_app::on__load_assets ()
{
	land.setup (engine, gpu, renderer);
	land.load_assets();
}

//***************************************
void Land1_app::on__cleanup()
{
	land.cleanup();
}

//***************************************
void Land1_app::on__render()
{
	land.render();
}

