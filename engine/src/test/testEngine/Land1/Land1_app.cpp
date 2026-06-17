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
	land.setup (gos::getSysHeapAllocator(), engine);
}

//***************************************
void Land1_app::on__cleanup()
{
	land.unsetup();
}

//***************************************
void Land1_app::on__render()
{
	land.render();
}

