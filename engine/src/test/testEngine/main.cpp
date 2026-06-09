#include "test1.h"
#include "game1.h"
#include "test_exa1.h"


//******************************** 
void test_res_handle_1()
{
	gos::res::Handle h;
	h.setInvalid();

	printf ("MAX_NUM_TYPE = %d\n", gos::res::Handle::MAX_NUM_TYPE);
	printf ("MAX_NUM_INDEX = %d\n", gos::res::Handle::MAX_NUM_INDEX);
	printf ("MAX_NUM_PAGE = %d\n", gos::res::Handle::MAX_NUM_PAGE);
	printf ("MAX_NUM_COUNTER = %d\n", gos::res::Handle::MAX_NUM_COUNTER);

	for (u32 t=0; t<gos::res::Handle::MAX_NUM_TYPE; t++)
	{
		h.set_value_TYPE(t);

		for (u32 index=0; index<gos::res::Handle::MAX_NUM_INDEX; index++)
		{
			h.set_value_INDEX(index);

			for (u32 page=0; page<gos::res::Handle::MAX_NUM_PAGE; page++)
			{
				h.set_value_PAGE(page);
				for (u32 counter=0; counter<gos::res::Handle::MAX_NUM_COUNTER; counter++)
				{
					h.set_value_COUNTER(counter);

					assert (h.get_value_TYPE() == t);
					assert (h.get_value_INDEX() == index);
					assert (h.get_value_COUNTER() == counter);
					assert (h.get_value_PAGE() == page);
				}
			}
		}
	}
}

//******************************** 
int main()
{
    gos::sGOSInit init;
    init.memory_setDefaultForGame();
    
    init.setLogMode (gos::sGOSInit::eLogMode::both_console_and_file);
	if (!gos::init (init, "gosEngine"))
		return -1;

	//test_res_handle_1(); return 0;

    {
        gos::Engine engine;
        if (!engine.setup (1024, 768, "test engine"))
            return -2;

        if (!engine.asset_build())  return -3;
        //if (!engine.asset_rebuildAll())  return -3;
        {
			// gos::ENGVtxBuffer handle_pippo;
			// engine.vtxBuffer_create (128, eMemAccessMode::onGPU, &handle_pippo);
			// engine.release (handle_pippo);


            //Test1 test;		test.run (&engine);
            //Game1 game;		game.run (&engine);
			Test_exa1 test;		test.run(&engine);
        }
    }
    

    gos::deinit();
    return 0;
}

