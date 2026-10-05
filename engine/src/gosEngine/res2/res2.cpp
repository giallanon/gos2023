#include "res2.h"
#include "../gosEngine.h"

using namespace gos;

static gos::Logger *res_debug_logger = NULL;

//*************************************
const char* res2::enumToString (res2::eLoadMode s)
{
	switch (s)
	{
    default: DBGBREAK;                  return "!!res2::eLoadMode::ERR";
    case res2::eLoadMode::asap:       return "asap";
    case res2::eLoadMode::onDemand:   return "onDemand";
	}
}

//*************************************
const char* res2::enumToString (res2::eStatus s)
{
	switch (s)
	{
    default: DBGBREAK;				return "!!res2::eStatus::ERR";
    case res2::eStatus::ready:		return "ready";
    case res2::eStatus::unloaded:	return "unloaded";
	case res2::eStatus::loading:	return "loading";
	case res2::eStatus::loaded:		return "loaded";
	case res2::eStatus::unloading:	return "hot_reload";
	case res2::eStatus::error:      return "error";
	}
}

//*************************************
const char* res2::enumToString (res2::eResType s)
{
	switch (s)
	{
    default: DBGBREAK;						return "!!res2::eResType::ERR";
	case res2::eResType::_unused_zero:		return "_unused_zero";
	case res2::eResType::NUM_MAX:			return "NUM_MAX";

	case res2::eResType::vtx_buffer:		return "vtx_buffer";
	case res2::eResType::idx_buffer:		return "idx_buffer";
	case res2::eResType::vtx_shader:		return "vtx_shader";
	case res2::eResType::pxl_shader:		return "pxl_shader";
	case res2::eResType::pipeline:			return "pipeline";
	case res2::eResType::texture_2d:		return "texture_2d";
	case res2::eResType::shape:				return "shape";
	case res2::eResType::gpu_shape:			return "gpu_shape";
	case res2::eResType::skeleton:			return "skeleton";
	case res2::eResType::model_3d:			return "model_3d";
	}
}

//*************************************
const char* res2::enumToString (res2::eAssetType s)
{
	switch (s)
	{
    default: DBGBREAK;						return "!!res2::eAssetType::ERR";
	case res2::eAssetType::_unused_zero:	return "_unused_zero";
	case res2::eAssetType::NUM_MAX:			return "NUM_MAX";
	
	case res2::eAssetType::vtx_shader:		return "vtx_shader";
	case res2::eAssetType::pxl_shader:		return "pxl_shader";
	case res2::eAssetType::pipeline:		return "pipeline";
	case res2::eAssetType::texture_2d:		return "texture_2d";
	case res2::eAssetType::shape:			return "shape";
	case res2::eAssetType::skeleton:		return "skeleton";
	case res2::eAssetType::materialPBR:		return "materialPBR";
	case res2::eAssetType::model_3d:		return "model_3d";
	case res2::eAssetType::model_instance:	return "model_instance";
	}
}

//***********************************
void res2::BaseHandle::make_handle (u8 guard, u32 page_index, u32 offset, u32 num_slot, u8 type, bool isResource)
{
	assert (num_slot <= 0x7FF);
	_handle = offset;									//32 bit
	_handle |= (((u64)page_index) & 0xFF) << 32;		//8 bit per page_index
	_handle |= (((u64)num_slot) & 0x07FF) << 40;		//11 bit per <num_slot>  == 64 * 2048 = 128 MB allocazione max
	_handle |= (((u64)type) & 0xFF) << 51;				//8 bit per <type>
	_handle |= (((u64)guard) & 0x0F) << 59;				//4 bit per <guard>
	if (isResource)	_handle |= 0x8000000000000000;	//1 bit (MSB) per <is_a_resource>

	assert (get_offset() == offset);
	assert (get_page_index() == page_index);
	assert (get_num_slot() == num_slot);
	assert (get_type() == type);
	assert ( (isResource && is_a_resource()) || (!isResource && is_an_asset()) );
}



//**************************************************************** 
void res2::debug__set_logger (gos::Logger *logger)
{
	res_debug_logger = logger;
}


//**************************************************************** 
static void  internal__debug__printInfo (const res2::ResDescr *res, const char *debug_info)
{
	if (NULL == res_debug_logger)
		return;

	 res_debug_logger->log (eTextColor::darkYellow, "res2::[%-12s] [%-20s] [st:%-12s] [ref:%03d] [h:%016" PRIX64 "] [sigUID: %016" PRIX64 "]\n",
		res2::enumToString(res->get_type()),
		debug_info,
		res2::enumToString(res->status), 
		res->refCount,
		res->res_handle.viewAsU64(),
		res->signatureUID._uid);
}

//**************************************************************** 
static void  internal__debug__printInfo (const res2::AssetDescr *asset, const char *debug_info)
{
	if (NULL == res_debug_logger)
		return;

	const char *status = res2::enumToString(asset->status);
	char status_prefix=' ';
	if (0 != asset->num_child_not_ready)
		status_prefix='~';


	 res_debug_logger->log (eTextColor::darkGreen, "asset2::[%-12s] [%-20s] [st:%c%-12s] [ref:%03d] [h:%016" PRIX64 "] [assUID: %016" PRIX64 "]\n",
		res2::enumToString(asset->get_type()),
		debug_info,
		status_prefix, status, 
		asset->refCount,
		asset->asset_handle.viewAsU64(),
		asset->assetUID._uid);
}


//**************************************************************** 
static void internal__asset__on_children_become_notready (res2::AssetDescr *asset)
{
	//uno dei figli di <asset> era "ready" e ora e' diventato "not ready"
	assert (NULL != asset);

	asset->num_child_not_ready++;
	internal__debug__printInfo (asset, "child unrdy");
}

//**************************************************************** 
static void internal__asset__on_children_become_ready (res2::AssetDescr *asset)
{
	//uno dei figli di <asset> era "not ready" e ora e' diventato "ready"
	assert (NULL != asset);

	assert (asset->num_child_not_ready > 0);
	asset->num_child_not_ready--;
	internal__debug__printInfo (asset, "child rdy");
}





/***********************************************************************************************
* 
*							R E S O U R C E S
* 
************************************************************************************************/
bool res2::is_ready (const res2::ResDescr *res)
{
	assert (NULL != res);
	return (res2::eStatus::ready == res->status);
}

//*************************************
void res2::change_state (ResDescr *res, res2::eStatus new_status, const char *reason)
{
	assert (NULL != res);
	if (res->status == new_status)
		return;

	const bool was_ready = is_ready(res);
	res->status = new_status;
	const bool now_ready = is_ready(res);
	internal__debug__printInfo (res, reason);

	if (was_ready && !now_ready)
	{
		//se ero "ready" e ora non lo sono +, devo informare i miei owner
		res2::AssetHandleChain *p = res->asset_owner_list;
		while (p)
		{
			internal__asset__on_children_become_notready (p->asset);
			p = p->next;
		}
	}
	else if (now_ready && !was_ready)
	{
		//se ero "non ready" e adesso invece sono "ready", devo informare i miei owner
		res2::AssetHandleChain *p = res->asset_owner_list;
		while (p)
		{
			internal__asset__on_children_become_ready (p->asset);
			p = p->next;
		}
	}
}

/*************************************
* Ritorna true se il ref_count e' andato a 0 e quindi la risorsa e' stata freed.
* Eventuali operazioni di free da farsi su risorse allocate dall'engine, le deve fare l'engine stesso
*/
bool res2::release (res2::ResDescr *res)
{
	assert (NULL != res);
	assert (res->refCount > 0);
	res->refCount--;	
	if (res->refCount > 0)
	{
		internal__debug__printInfo (res, "ref--");
		return false;
	}

	internal__debug__printInfo (res, "destroyed");
	return true;
}

//*************************************
void res2::inc_refCount (ResDescr *res)
{
	assert (NULL != res);
	assert (res->refCount < u16MAX);
	res->refCount++;
	internal__debug__printInfo (res, "ref++");
}


/***********************************************************************************************
* 
*							A S S E T
* 
************************************************************************************************/
bool res2::is_ready (const res2::AssetDescr *asset)
{
	assert (NULL != asset);
	return (res2::eStatus::ready == asset->status && 0 == asset->num_child_not_ready);
}

//*************************************
void res2::change_state (res2::AssetDescr *asset, res2::eStatus new_status, const char *reason)
{
	assert (NULL != asset);
	if (asset->status == new_status)
		return;

	const bool was_ready = is_ready(asset);
	asset->status = new_status;
	const bool now_ready = is_ready(asset);
	internal__debug__printInfo (asset, reason);

	if (was_ready && !now_ready)
	{
		//se ero "ready" e ora non lo sono +, devo informare i miei owner
		res2::AssetHandleChain *p = asset->asset_owner_list;
		while (p)
		{
			internal__asset__on_children_become_notready (p->asset);
			p = p->next;
		}
	}
	else if (now_ready && !was_ready)
	{
		//se ero "non ready" e adesso invece sono "ready", devo informare i miei owner
		res2::AssetHandleChain *p = asset->asset_owner_list;
		while (p)
		{
			internal__asset__on_children_become_ready (p->asset);
			p = p->next;
		}
	}
}

/*************************************
* Ritorna true se il ref_count e' andato a 0 e quindi la risorsa e' stata freed.
* Eventuali operazioni di free da farsi su risorse allocate dall'engine, le deve fare l'engine stesso
*/
bool res2::release (res2::AssetDescr *asset)
{
	assert (NULL != asset);
	assert (asset->refCount > 0);
	asset->refCount--;
	if (asset->refCount > 0)
	{
		internal__debug__printInfo (asset, "ref--");
		return false;
	}

	internal__debug__printInfo (asset, "destroyed");
	return true;
}

//*************************************
void res2::inc_refCount (AssetDescr *asset)
{
	assert (NULL != asset);
	assert (asset->refCount < u16MAX);
	asset->refCount++;
	internal__debug__printInfo (asset, "ref++");
}
