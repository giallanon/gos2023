#include "land.h"

using namespace gos;


//**********************************
f32 land::resolution_to_m (Resol res)
{
	switch (res)
	{
	default:	DBGBREAK; return 0;
	case land::Resol::_0125m:	return 0.125f;
	case land::Resol::_025m:	return 0.25f;
	case land::Resol::_05m:		return 0.5f;
	case land::Resol::_1m:		return 1.0f;
	case land::Resol::_2m:		return 2.0f;
	case land::Resol::_4m:		return 4.0f;
	case land::Resol::_8m:		return 8.0f;
	case land::Resol::_16m:		return 16.0f;
	case land::Resol::_32m:		return 32.0f;
	case land::Resol::_64m:		return 64.0f;
	case land::Resol::_128m:	return 128.0f;
	case land::Resol::_256m:	return 256.0f;
	}
}