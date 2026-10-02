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

//**************************************************************** 
void res2::debug__set_logger (gos::Logger *logger)
{
	res_debug_logger = logger;
}


//**************************************************************** 
static void  internal__res_debug__printInfo (const res2::ResDescr *res, const char *debug_info)
{
	if (NULL == res_debug_logger)
		return;

	 res_debug_logger->log (eTextColor::darkYellow, "res::[%-20s] [h:%08X] [ref:%03d] [st:%-12s] [%-12s] [sigUID: %016" PRIX64 "]\n",
		debug_info,
		res->res_handle.viewAsU32(),
		res->refCount,
		res2::enumToString(res->status), 
		res2::enumToString(res->get_type()),
		res->signatureUID._uid);
}

//**************************************************************** 
static void  internal__asset_debug__printInfo (const res2::AssetDescr *asset, const char *debug_info)
{
	if (NULL == res_debug_logger)
		return;

	const char *status = res2::enumToString(asset->status);
	char status_prefix=' ';
	if (0 != asset->num_child_not_ready)
		status_prefix='~';


	 res_debug_logger->log (eTextColor::darkGreen, "asset::[%-20s] [h:%08X] [ref:%03d] [st:%c%-12s] [%-12s] [assUID: %016" PRIX64 "]\n",
		debug_info,
		asset->asset_handle.viewAsU32(),
		asset->refCount,
		status_prefix, status, 
		res2::enumToString(asset->get_type()),
		asset->assetUID._uid);
}


//**************************************************************** 
static void internal__asset__on_children_become_notready (res2::AssetDescr *asset)
{
	//uno dei figli di <asset> era "ready" e ora e' diventato "not ready"
	assert (NULL != asset);

	asset->num_child_not_ready++;
	internal__asset_debug__printInfo (asset, "child unrdy");
}

//**************************************************************** 
static void internal__asset__on_children_become_ready (res2::AssetDescr *asset)
{
	//uno dei figli di <asset> era "not ready" e ora e' diventato "ready"
	assert (NULL != asset);

	assert (asset->num_child_not_ready > 0);
	asset->num_child_not_ready--;
	internal__asset_debug__printInfo (asset, "child rdy");
}





/***********************************************************************************************
* 
*							R E S O U R C E S
* 
************************************************************************************************/
void res2::res_setup (res2::ResDescr *res, asset2::UID signatureUID, res2::ResHandle res_handle)
{
	assert (NULL != res);
	res->reset();
	res->signatureUID = signatureUID;
	res->res_handle = res_handle;
}

//*************************************
bool res2::res_is_ready (const res2::ResDescr *res)
{
	assert (NULL != res);
	return (res2::eStatus::ready == res->status);
}

//*************************************
void res2::res_change_state (ResDescr *res, res2::eStatus new_status, const char *reason)
{
	assert (NULL != res);
	if (res->status == new_status)
		return;

	const bool was_ready = res_is_ready(res);
	res->status = new_status;
	const bool is_ready = res_is_ready(res);
	internal__res_debug__printInfo (res, reason);

	if (was_ready && !is_ready)
	{
		//se ero "ready" e ora non lo sono +, devo informare i miei owner
		res2::AssetHandleChain *p = res->asset_owner_list;
		while (p)
		{
			internal__asset__on_children_become_notready (p->asset);
			p = p->next;
		}
	}
	else if (is_ready && !was_ready)
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
bool res2::res_release (res2::ResDescr *res)
{
	assert (NULL != res);
	assert (res->refCount > 0);
	res->refCount--;
	if (res->refCount > 0)
		return false;
	return true;
}


/***********************************************************************************************
* 
*							A S S E T
* 
************************************************************************************************/
void res2::asset_setup (res2::AssetDescr *asset, asset2::UID assetUID, res2::AssetHandle asset_handle)
{
	assert (NULL != asset);
	asset->reset();
	asset->assetUID = assetUID;
	asset->asset_handle = asset_handle;
}

//*************************************
bool res2::asset_is_ready (const res2::AssetDescr *asset)
{
	assert (NULL != asset);
	return (res2::eStatus::ready == asset->status && 0 == asset->num_child_not_ready);
}

//*************************************
void res2::asset_change_state (res2::AssetDescr *asset, res2::eStatus new_status, const char *reason)
{
	assert (NULL != asset);
	if (asset->status == new_status)
		return;

	const bool was_ready = asset_is_ready(asset);
	asset->status = new_status;
	const bool is_ready = asset_is_ready(asset);
	internal__asset_debug__printInfo (asset, reason);

	if (was_ready && !is_ready)
	{
		//se ero "ready" e ora non lo sono +, devo informare i miei owner
		res2::AssetHandleChain *p = asset->asset_owner_list;
		while (p)
		{
			internal__asset__on_children_become_notready (p->asset);
			p = p->next;
		}
	}
	else if (is_ready && !was_ready)
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
bool res2::asset_release (res2::AssetDescr *asset)
{
	assert (NULL != asset);
	assert (asset->refCount > 0);
	asset->refCount--;
	if (asset->refCount > 0)
		return false;
	return true;
}
