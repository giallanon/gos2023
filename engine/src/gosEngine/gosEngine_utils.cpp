#include "gosEngineEnumAndDefine.h"

using namespace gos;


//*************************************
const char* engine::enumToString (engine::eLoadMode s)
{
	switch (s)
	{
    default: DBGBREAK;                  return "!!engine::eLoadMode::ERR";
    case engine::eLoadMode::asap:       return "asap";
    case engine::eLoadMode::onDemand:   return "onDemand";
	}
}