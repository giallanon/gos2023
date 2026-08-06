#include "gosEngine.h"
#include "../gosAsset2/gosAsset2Builder.h"

using namespace gos;
using namespace gos::engine;

//**************************************************************** 
void Engine::res_printInfo (const void *resIN, const char *debug_info) const
{
	 const res::Descr *res = (const res::Descr*)resIN;

	 asset_logger->log (eTextColor::darkYellow, "res::[%-20s] [%08X] [%03d] [%-12s] [%-12s] [uid: %016" PRIX64 "]\n",
		debug_info,
		res->handle.viewAsU32(),
		res->refCount,
		res::enumToString(res->_status),
		res::enumToString((res::eType)res->handle.get_value_TYPE()),
		res->uid._uid);
}

//**************************************************************** 
void Engine::res_set_status (res::Descr *res, res::eStatus new_status)
{
	assert (NULL != res);
	if (res->_status != new_status)
	{
		res->_status = new_status;
		res_printInfo (res, "status");
	}
}

//**************************************************************** 
res::Descr* Engine::res__do_createHandle (res::eType res_typeIN, res::eStatus statusIN, asset2::UID uid, res::Handle *out_handle)
{
	assert (NULL != out_handle);

	res::Descr *res = (res::Descr*)resManager.raw_reserve (res_typeIN, out_handle);
	if (NULL == res)
		return NULL;

	res->reset();
	res->handle = *out_handle;
	res->refCount = 1;
	res->_status = statusIN;
	res->uid = uid;
	
	res_bindEvents (*out_handle, res);
	
	if (NULL != res->on_afterCreate)
		(this->*res->on_afterCreate)(res);

	res_printInfo (res, "create");
	return res;
}

//**************************************************************** 
res::Descr* Engine::res_createHandle (res::eType res_type, res::Handle *out_handle)
{
	asset2::UID uid;
	uid.setInvalid();
	res::Descr *descr = res__do_createHandle (res_type, res::eStatus::ready, uid, out_handle);
	if (NULL == descr)
	{
		logger::err ("Engine::res_createHandle() => can't create handle for res type=%d\n", (u8)res_type);
	}
	return descr;
}

//**************************************************************** 
res::Descr* Engine::res_getOrCreateHandleFromAsset (const char *uid_runtimeName, res::Handle *out_handle, bool *out_bWasNew)
{
	assert (NULL != out_handle);
	asset2::UID uid;
	if (!asset2::asset_getBy_rtname (asset_ctx, uid_runtimeName, &uid))
	{
		logger::err ("Engine::res_getOrCreateHandleFromAsset(%s) => invalid runtime name\n", uid_runtimeName);
		return NULL;
	}

	return res_getOrCreateHandleFromAsset (uid, out_handle, out_bWasNew);
}

//**************************************************************** 
res::Descr* Engine::res_getOrCreateHandleFromAsset (asset2::UID uid, res::Handle *out_handle, bool *out_bWasNew)
{
	assert (uid.isValid());
	assert (NULL != out_handle);
	assert (NULL != out_bWasNew);

	res::eType res_type;
	if (!res_assetUID_to_resUID (uid, &res_type))
	{
		logger::err ("Engine::res_getOrCreateHandleFromAsset() => can't deduct res_type frome assert uid [%016]" PRIX64 "\n", uid._uid);
		return NULL;
	}


	HashListOfLoadedUID::Position pos;
	u32 handle_asU32;
	if (listof_knownUID.findWithPos (uid, &handle_asU32, &pos))
	{
		//l'asset e' gia' noto e quindi e' gia' stato associato ad un handle.
		//Ritorno quell'handle stesso
		*out_bWasNew = false;
		out_handle->setFromU32(handle_asU32);
		assert (out_handle->get_value_TYPE() == (u32)res_type);

		//incremento il ref count
		res::Descr *res = res_getDescriptor(*out_handle);
		res->refCount++;
		res_printInfo(res, "refcount++");
		return res;
	}

	//l'asset e' nuovo, devo quindi creare un nuovo handle
	*out_bWasNew = true;
	res::Descr *res = res__do_createHandle (res_type, res::eStatus::notLoaded, uid, out_handle);
	if (NULL == res)
	{
		logger::err ("Engine::res_getOrCreateHandleFromAsset() => can't create handle for res type=%d and asset uid=%016" PRIX64 "\n", (u8)res_type, uid._uid);
		return NULL;
	}

	//inserisco la coppia <uid, handle> in hashlist
	listof_knownUID.insertInPosition (pos, out_handle->viewAsU32());

	//se questo asset ha delle dipendenze runtime, recupero/creo i relativi handle
	u8 memblock[256];
	asset2::FastUIDList fastUIDList;
	fastUIDList.setupWithBase (memblock, sizeof(memblock), gos::getScrapAllocator());

	asset_logger->incIndent();
	asset2::asset_get_runtime_dependecies_list (asset_ctx, uid, false, &fastUIDList);
	for (u32 i=0; i<fastUIDList.getNElem(); i++)
	{
		const asset2::UID child_uid = fastUIDList(i);
		   
		bool bWasNew;
		res::Handle child_handle;
		res::Descr *child_res = res_getOrCreateHandleFromAsset (child_uid, &child_handle, &bWasNew);
		if (bWasNew)
		{
			res_bindEvents (child_handle, child_res);
		}
		else
		{
			child_res->refCount++;
		}

		//child_handle diventa uno dei miei figli
		res_addChild (res, child_res);
	}
	asset_logger->decIndent();
	return res;
}

//**************************************************************** 
void Engine::res_bindEvents (res::Handle handle, res::Descr *res)
{
	assert (NULL != res);
	assert (handle.isValid());
	switch ((res::eType)handle.get_value_TYPE())
	{
	default:
		DBGBREAK;
		return;

	case res::eType::_unused_zero:
		DBGBREAK;
		return;

	case res::eType::vtx_buffer:	
		res->on_afterCreate = &Engine::internal__vtxBuffer_on_afterCreate;
		res->on_destroy = &Engine::internal__vtxBuffer_on_destroy;
		return;

	case res::eType::idx_buffer:
		res->on_afterCreate = &Engine::internal__idxBuffer_on_afterCreate;
		res->on_destroy = &Engine::internal__idxBuffer_on_destroy;
		return;

	case res::eType::vtx_shader:
		res->on_afterCreate = &Engine::internal__vtxshader_on_afterCreate;
		res->on_destroy = &Engine::internal__vtxshader_on_destroy;
		return;

	case res::eType::pxl_shader:
		res->on_afterCreate = &Engine::internal__pxlshader_on_afterCreate;
		res->on_destroy = &Engine::internal__pxlshader_on_destroy;
		return;

	case res::eType::pipeline:
		res->on_afterCreate = &Engine::internal__pipeline_on_afterCreate;
		res->on_destroy = &Engine::internal__pipeline_on_destroy;
		return;

	case res::eType::texture_2d:
		res->on_afterCreate = &Engine::internal__texture2D_on_afterCreate;
		res->on_destroy = &Engine::internal__texture2D_on_destroy;
		res->on_afterLoad = &Engine::internal__texture2D_on_afterLoad;
		res->on_unload = &Engine::internal__texture2D_on_unload;
		return;

	case res::eType::shape:
		res->on_afterCreate = &Engine::internal__shape_on_afterCreate;
		res->on_destroy = &Engine::internal__shape_on_destroy;
		return;

	case res::eType::gpu_shape:
		res->on_afterCreate = &Engine::internal__GPUShape_on_afterCreate;
		res->on_destroy = &Engine::internal__GPUShape_on_destroy;
		return;

	case res::eType::skeleton:
		res->on_afterCreate = &Engine::internal__skeleton_on_afterCreate;
		res->on_destroy = &Engine::internal__skeleton_on_destroy;
		return;

	case res::eType::model_3d:
		res->on_afterCreate = &Engine::internal__model_on_afterCreate;
		res->on_destroy = &Engine::internal__model_on_destroy;
		return;

	case res::eType::model_instance:
		res->on_afterCreate = &Engine::internal__modelinst_on_afterCreate;
		res->on_destroy = &Engine::internal__modelinst_on_destroy;
		return;
	
	case res::eType::materialPBR:
		res->on_afterCreate = &Engine::internal__materialPBR_on_afterCreate;
		res->on_destroy = &Engine::internal__materialPBR_on_destroy;
		return;
	
	}
}

//**************************************************************** 
res::Descr* Engine::res_getDescriptor (res::Handle handle)
{
	return (res::Descr*)resManager.raw_get_data (handle);
}

//**************************************************************** 
bool Engine::res_release (res::Handle handle)
{
	res::Descr *res = res_getDescriptor(handle);
	if (NULL != res)
		return res_release(res);
	return false;
}

//**************************************************************** 
bool Engine::res_release (res::Descr *res)
{
	assert (NULL != res);
	assert (res->refCount > 0);
	if (1 == res->refCount)
	{
		//dobbiamo effettivamente fare il free della risorsa
		switch (res->_status)
		{
		default:
			DBGBREAK;
			break;

		case res::eStatus::loading:
			//questo non dovrebbe succedere perche' quando la risorsa viene passata
			//al loader, il suo ref-count viene incrementato
			DBGBREAK;
			break;

		case res::eStatus::hot_reload:
			//questo non dovrebbe succedere perche' quando la risorsa viene "hot-reloaded"
			//il suo ref-count viene incrementato
			DBGBREAK;
			break;

		//anche se la risorsa non e' stata nemmeno caricata, non c'e' da preoccuparsene, posso fare il "free" immediatamente
		case res::eStatus::notLoaded:
		case res::eStatus::error:
		case res::eStatus::ready:
			res_do_destroy(res);
			return true;
		}
	}
	else
	{
		res->refCount--;
		res_printInfo(res, "release");
	}

	return false;
}

//**************************************************************** 
bool Engine::res_assetUID_to_resUID (asset2::UID uid, res::eType *out_res_type) const
{
	assert (uid.isAnAsset());
	switch (uid.getAssetType())
	{
	default:
		DBGBREAK;
		return false;
	case eAssetType::vtx_shader:    *out_res_type =  res::eType::vtx_shader; return true;
	case eAssetType::pxl_shader:    *out_res_type =  res::eType::pxl_shader; return true;
	case eAssetType::pipe:          *out_res_type =  res::eType::pipeline; return true;
	case eAssetType::tex2D:         *out_res_type =  res::eType::texture_2d; return true;
	case eAssetType::shape:         *out_res_type =  res::eType::shape;     return true;
	case eAssetType::skeleton:      *out_res_type =  res::eType::skeleton;  return true;
	case eAssetType::model3d:       *out_res_type =  res::eType::model_3d;  return true;
	case eAssetType::materialPBR:	*out_res_type =  res::eType::materialPBR;  return true;
	}
}

//**************************************************************** 
void Engine::res_addChild (res::Descr *padre_res, res::Descr *child_res)
{
	//child diventa figlio di padre
	res::HandleChain *chain = res_newHandleChain();
	chain->res = child_res;

	chain->next = padre_res->figli;
	padre_res->figli = chain;

	//padre diventa "padre" di child
	chain = res_newHandleChain();
	chain->res = padre_res;
	chain->next = child_res->padri;
	child_res->padri = chain;	
}

//**************************************************************** 
res::HandleChain* Engine::res_newHandleChain ()
{
	return resHandleChainPool.alloc();
}

//**************************************************************** 
void Engine::res_freeHandleChain (res::HandleChain *p)
{
	resHandleChainPool.free(p);
}

//**************************************************************** 
bool Engine::res_getOrScheduleLoad (res::Handle handle, const res::Descr **out, u64 timeout_msec)
{
	res::Descr *res= res_getDescriptor(handle);
	if (NULL == res)
	{
		(*out) = NULL;
		return false;
	}

	(*out) = res;
	if (res::eStatus::ready == res->_status)
	{
		return true;
	}

	assert (res->uid.isValid());
	if (res::eStatus::notLoaded == res->_status)
	{
		//dato che l'asset e' notLoaded, schedulo il suo load (e degli asset da cui dipende)
		//Incremento il ref count perche' sto passando la risorsa al loader-thread e questo garantisce che 
		//la risorsa non verra' eliminata da eventuali release()
		res->refCount++;
		res_set_status (res, res::eStatus::loading);

		asset_logger->incIndent();
		res::HandleChain *p = res->figli;
		while (p)
		{
			const res::Descr *descr;
			res_getOrScheduleLoad(p->res->handle, &descr);
			p = p->next;
		}
		asset_logger->decIndent();

		
		//schedulo il load di me stesso
		thread::pushMsg (msgq_1W, MSG_FOR_LOADER_THREAD__LOAD, 0, res);

		if (0 == timeout_msec)
			return false;		
	}

	//se richiesto, attendo per un po' nella speranza di vedere l'asset "ready"
	if (timeout_msec > 0 && res::eStatus::error != res->_status)
	{
		u64 time_to_exit_msec = gos::getTimeSinceStart_msec() + timeout_msec;
		while (gos::getTimeSinceStart_msec() < time_to_exit_msec)
		{
			priv_flushLoaderThreadMsg();
			if (res::eStatus::ready == res->_status) return true;
			if (res::eStatus::error == res->_status) return false;
		}
	}
	return false;
}

//**************************************************************** 
void Engine::res_do_destroy (res::Descr *res)
{
	assert (NULL != res);
	assert (res->_status != res::eStatus::loading);
	assert (res->_status != res::eStatus::hot_reload);

	//chiamo il "distruttore" di me stesso solo se la risorsa era stata effettivamente caricata
	if (res::eStatus::ready == res->_status)
	{
		(this->*res->on_destroy)(res);
	}
	res_set_status (res, res::eStatus::notLoaded);
	res_printInfo(res, "destroy");

	//se ho dei figli, faccio il release
	asset_logger->incIndent();
	res::HandleChain *p = res->figli;
	while (p)
	{
		res::HandleChain *next = p->next;
		res_release (p->res);
		res_freeHandleChain(p);
		p = next;
	}
	asset_logger->decIndent();

	//elimino la lista dei miei padri, ma non c'e' bisogno di notificarli
	//o di rimuovermi dalla lista dei loro figli perche' essendo io refCountato,
	//io posso essere distrutto solo se tutti i miei padri sono a loro volta stati distrutti
	//nel qual caso la loro lista dei figli e' gia' stata pulita
	p = res->padri;
	while (p)
	{
		res::HandleChain *next = p->next;
		res_freeHandleChain(p);
		p = next;
	}

	//libero l'handle
	resManager.raw_release(res->handle);
}


//**************************************************************** 
bool Engine::res_hotreload (res::Handle handle)
{
	res::Descr *res = res_getDescriptor(handle);
	if (NULL == res)
	{
		DBGBREAK;
		return false;
	}
	
	//if (res->flag1.isBitSet (res::Descr::FLAG1__MARKED_FOR_RELOAD))
	if (res::eStatus::hot_reload == res->_status)
		return false;

	//Incremento il ref count per evitare che qualcuno mi elimini la risorsa intanto
	//che e' in hot-reload
	//L'hot-reload viene poi processata nella Engine::update();
	res->refCount++;
	res_set_status (res, res::eStatus::hot_reload);
	
	const sUnloadInfo info = {
		.res_handle = handle,
		.timer_msec = (u32)gos::getTimeSinceStart_msec() + 10,
	};
	list_of_res_to_be_hotreloaded.append (info);
	return true;
}
