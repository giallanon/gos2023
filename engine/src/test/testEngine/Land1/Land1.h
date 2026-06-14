#ifndef _Land1_h_
#define _Land1_h_
#include "../DefaultApp/DefaultApp.h"


class Land1 : public DefaultApp
{
public:
					Land1();
					~Land1();

protected:
	void	on__load_assets () final;
	void	on__handle_input () final;
	void	on__render() final;
	void 	on__cleanup() final;


private:
	static const u32 SQUARE_SIZE = 16;

private:
	gos::ENGModel3d 	handle__model_tile1;
	gos::ENGModel3dInst	handle__modelinst_tile1[SQUARE_SIZE*SQUARE_SIZE];
};

#endif //_Land1_h_
