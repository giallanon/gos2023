#include "gosEngineRes.h"

using namespace gos;

//*************************************
const char* res::enumToString (res::eLoadMode s)
{
	switch (s)
	{
    default: DBGBREAK;                  return "!!res::eLoadMode::ERR";
    case res::eLoadMode::asap:       return "asap";
    case res::eLoadMode::onDemand:   return "onDemand";
	}
}

//*************************************
const char* res::enumToString (res::eType s)
{
	switch (s)
	{
    default: DBGBREAK;                  return "!!res::eType::ERR";
	case res::eType::_unused_zero:		return "_unused_zero";
	case res::eType::NUM_MAX:			return "NUM_MAX";
	case res::eType::vtx_buffer:		return "vtx_buffer";
	case res::eType::idx_buffer:		return "idx_buffer";
	case res::eType::vtx_shader:		return "vtx_shader";
	case res::eType::pxl_shader:		return "pxl_shader";
	case res::eType::shape:				return "shape";
	case res::eType::gpu_shape:			return "gpu_shape";
	case res::eType::texture_2d:		return "texture_2d";
	case res::eType::pipeline:			return "pipeline";
	case res::eType::skeleton:			return "skeleton";
	case res::eType::model_3d:			return "model_3d";
	case res::eType::model_instance:	return "model_inst";
	case res::eType::materialPBR:		return "materialPBR";
	}
}

//*************************************
const char* res::enumToString (res::eStatus s)
{
	switch (s)
	{
    default: DBGBREAK;				return "!!res::eStatus::ERR";
    case res::eStatus::ready:       return "ready";
    case res::eStatus::notLoaded:	return "notLoaded";
	case res::eStatus::loading:		return "loading";
	case res::eStatus::error:       return "error";
	}
}
