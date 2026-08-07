#include "gosEngine.h"
#include "../gosAsset2/gosAsset2Builder.h"

using namespace gos;
using namespace gos::engine;

//**************************************************************** 
void Engine::res__printInfo (const void *resIN, const char *debug_info) const
{
	 const res::Descr *res = (const res::Descr*)resIN;

	 asset_logger->log (eTextColor::darkYellow, "res::[%-20s] [%08X] [%03d] [%02d/%-12s] [%-12s] [uid: %016" PRIX64 "]\n",
		debug_info,
		res->handle.viewAsU32(),
		res->refCount,
		res->_num_child_not_ready, res::enumToString(res->_status), 
		res::enumToString((res::eType)res->handle.get_value_TYPE()),
		res->uid._uid);
}

//**************************************************************** 
void Engine::res__set_status (res::Descr *res, res::eStatus new_status)
{
	assert (NULL != res);
	if (res->_status == new_status)
	{
		return;
	}

	const res::eStatus old_status = res->_status;
	res->_status = new_status;
	res__printInfo (res, "status");

	if (res::eStatus::aready == old_status)
	{
		//se ero "ready" e ora non lo sono +, devo informare i miei padri
		res::HandleChain *p = res->padri;
		while (p)
		{
			res__on_children_become_notready (p->res);
			p = p->next;
		}
	}
	else if (res::eStatus::aready == new_status)
	{
		//se ero "non ready" e adesso invece sono "ready", devo informare i miei padri		
		res::HandleChain *p = res->padri;
		while (p)
		{
			res__on_children_become_ready (p->res);
			p = p->next;
		}
	}
}

//**************************************************************** 
void Engine::res__on_children_become_ready (res::Descr *res_padre)
{
	assert (NULL != res_padre);

	//uno dei miei figli era "not ready" e ora e' diventato ready
	assert (res_padre->_num_child_not_ready > 0);
	res_padre->_num_child_not_ready--;
	res__printInfo (res_padre, "child rdy");

	if (0 == res_padre->_num_child_not_ready)
	{
		if (res::eStatus::loaded == res_padre->_status)
		{
			//io ero "loaded" che vuol dire che ero pronto, ma avevo dei figli not-ready.
			//Ora che tutti anche i miei figli sono diventati ready, cambio di stato e divento "ready".
			//Il mio cambio di stato viene propagato ai miei padri
			res__set_status (res_padre, res::eStatus::aready);
		}
	}
}

//**************************************************************** 
void Engine::res__on_children_become_notready (res::Descr *res_padre)
{
	assert (NULL != res_padre);

	//uno dei miei figli era "ready" e ora e' diventato "not ready"
	res_padre->_num_child_not_ready++;
	res__printInfo (res_padre, "child unrdy");

	if (res::eStatus::aready == res_padre->_status)
	{
		//io ero "ready" in quanto anche tutti i miei figli erano ready.
		//Ora che almeno uno dei miei figli e' diventato not-ready, cambio il mio stato.
		//Il mio cambio di stato viene propagato ai miei padri
		res__set_status (res_padre, res::eStatus::loaded);	
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
	
	res__bindEvents (*out_handle, res);
	
	res__printInfo (res, "create");

	if (NULL != res->on_afterCreate)
		(this->*res->on_afterCreate)(res);
	
	return res;
}

//**************************************************************** 
res::Descr* Engine::res__createHandle (res::eType res_type, res::Handle *out_handle)
{
	asset2::UID uid;
	uid.setInvalid();
	res::Descr *descr = res__do_createHandle (res_type, res::eStatus::aready, uid, out_handle);
	if (NULL == descr)
	{
		logger::err ("Engine::res__createHandle() => can't create handle for res type=%d\n", (u8)res_type);
	}
	return descr;
}

//**************************************************************** 
res::Descr* Engine::res__getOrCreateHandleFromAsset (const char *uid_runtimeName, res::Handle *out_handle, bool *out_bWasNew)
{
	assert (NULL != out_handle);
	asset2::UID uid;
	if (!asset2::asset_getBy_rtname (asset_ctx, uid_runtimeName, &uid))
	{
		logger::err ("Engine::res__getOrCreateHandleFromAsset(%s) => invalid runtime name\n", uid_runtimeName);
		return NULL;
	}

	return res__getOrCreateHandleFromAsset (uid, out_handle, out_bWasNew);
}

res::Descr* Engine::res__getOrCreateHandleFromAsset (asset2::UID uid, res::Handle *out_handle, bool *out_bWasNew)
{
	assert (uid.isValid());
	assert (NULL != out_handle);
	assert (NULL != out_bWasNew);

	res::eType res_type;
	if (!res__assetUID_to_resUID (uid, &res_type))
	{
		logger::err ("Engine::res__getOrCreateHandleFromAsset() => can't deduct res_type frome assert uid [%016]" PRIX64 "\n", uid._uid);
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
		res::Descr *res = res__getDescriptor(*out_handle);
		res->refCount++;
		res__printInfo(res, "refcount++");
		return res;
	}

	//l'asset e' nuovo, devo quindi creare un nuovo handle
	*out_bWasNew = true;
	res::Descr *res = res__do_createHandle (res_type, res::eStatus::notLoaded, uid, out_handle);
	if (NULL == res)
	{
		logger::err ("Engine::res__getOrCreateHandleFromAsset() => can't create handle for res type=%d and asset uid=%016" PRIX64 "\n", (u8)res_type, uid._uid);
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
		res::Descr *child_res = res__getOrCreateHandleFromAsset (child_uid, &child_handle, &bWasNew);
		if (!bWasNew)
			child_res->refCount++;

		//child_handle diventa uno dei miei figli
		res__addChild (res, child_res);
	}
	asset_logger->decIndent();
	return res;
}

//**************************************************************** 
void Engine::res__bindEvents (res::Handle handle, res::Descr *res)
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
		res->on_unload = &Engine::internal__vtxshader_on_unload;
		return;

	case res::eType::pxl_shader:
		res->on_afterCreate = &Engine::internal__pxlshader_on_afterCreate;
		res->on_destroy = &Engine::internal__pxlshader_on_destroy;
		res->on_unload = &Engine::internal__pxlshader_on_unload;
		return;

	case res::eType::pipeline:
		res->on_afterCreate = &Engine::internal__pipeline_on_afterCreate;
		res->on_destroy = &Engine::internal__pipeline_on_destroy;
		res->on_unload = &Engine::internal__pipeline_on_unload;
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
		res->on_unload = &Engine::internal__shape_on_unload;
		return;

	case res::eType::gpu_shape:
		res->on_afterCreate = &Engine::internal__GPUShape_on_afterCreate;
		res->on_destroy = &Engine::internal__GPUShape_on_destroy;
		res->on_destroy = &Engine::internal__GPUShape_on_unload;
		return;

	case res::eType::skeleton:
		res->on_afterCreate = &Engine::internal__skeleton_on_afterCreate;
		res->on_destroy = &Engine::internal__skeleton_on_destroy;
		res->on_unload = &Engine::internal__skeleton_on_unload;
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
res::Descr* Engine::res__getDescriptor (res::Handle handle)
{
	return (res::Descr*)resManager.raw_get_data (handle);
}

//**************************************************************** 
bool Engine::res__release (res::Handle handle)
{
	res::Descr *res = res__getDescriptor(handle);
	if (NULL != res)
		return res__release(res);
	return false;
}

bool Engine::res__release (res::Descr *res)
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
			if (bQuitEngine)
			{
				//caso molto particolare dell'engine in fase di distruzione ma con in canna degli hot reload
				res->_status = res::eStatus::loaded;
				res__do_destroy(res);
			}
			else
			{
				DBGBREAK;
			}
			break;

		//anche se la risorsa non e' stata nemmeno caricata, non c'e' da preoccuparsene, posso fare il "free" immediatamente
		case res::eStatus::notLoaded:
		case res::eStatus::error:
		case res::eStatus::loaded:
		case res::eStatus::aready:
			res__do_destroy(res);
			return true;
		}
	}
	else
	{
		res->refCount--;
		res__printInfo(res, "release");
	}

	return false;
}

//**************************************************************** 
bool Engine::res__assetUID_to_resUID (asset2::UID uid, res::eType *out_res_type) const
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
void Engine::res__addChild (res::Descr *padre_res, res::Descr *child_res)
{
	//child diventa figlio di padre
	res::HandleChain *chain = res__newHandleChain();
	chain->res = child_res;
	chain->next = padre_res->figli;
	padre_res->figli = chain;

	//padre diventa "padre" di child
	chain = res__newHandleChain();
	chain->res = padre_res;
	chain->next = child_res->padri;
	child_res->padri = chain;	

	//se child e' not-ready, lo comunico al mio nuovo padre
	if (res::eStatus::aready != child_res->_status)
		res__on_children_become_notready (padre_res);
}

//**************************************************************** 
res::HandleChain* Engine::res__newHandleChain ()
{
	return resHandleChainPool.alloc();
}

//**************************************************************** 
void Engine::res__freeHandleChain (res::HandleChain *p)
{
	resHandleChainPool.free(p);
}

/**************************************************************** 
* Ritorna true solo se <handle> punta ad una valida risorsa che al momento e':
*	- in stato eReady
*	- tutti i suoi figli sono in stato eRerady
*/
bool Engine::res__getOrScheduleLoad (res::Handle handle, const res::Descr **out, u64 timeout_msec)
{
	res::Descr *res= res__getDescriptor(handle);
	if (NULL == res)
	{
		(*out) = NULL;
		return false;
	}

	(*out) = res;
	if (res::eStatus::aready == res->_status)
	{
		assert (0 == res->_num_child_not_ready);
		return true;
	}

	//se la risorsa non e' associata ad un UID, non posso schedularne il load, quindi
	//ritorna false
	if (!res->uid.isValid())
		return false;

	if (res::eStatus::notLoaded == res->_status)
	{
		//dato che l'asset e' notLoaded, schedulo il suo load (e degli asset da cui dipende)
		//Incremento il ref count perche' sto passando la risorsa al loader-thread e questo garantisce che 
		//la risorsa non verra' eliminata da eventuali release()
		res->refCount++;
		res__set_status (res, res::eStatus::loading);

		asset_logger->incIndent();
		res::HandleChain *p = res->figli;
		while (p)
		{
			const res::Descr *descr;
			res__getOrScheduleLoad(p->res->handle, &descr);
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
			if (res::eStatus::aready == res->_status)
			{
				assert (0 == res->_num_child_not_ready);
				return true;
			}

			if (res::eStatus::error == res->_status) return false;
		}
	}
	return false;
}

//**************************************************************** 
void Engine::res__do_destroy (res::Descr *res)
{
	assert (NULL != res);
	assert (res->_status != res::eStatus::loading);
	assert (res->_status != res::eStatus::hot_reload);
	assert (res->refCount == 1);

	res->refCount = 0;

	//chiamo il "distruttore" di me stesso solo se la risorsa era stata effettivamente caricata
	if (res::eStatus::aready == res->_status || res::eStatus::loaded == res->_status)
	{
		(this->*res->on_destroy)(res);
	}
	res__set_status (res, res::eStatus::notLoaded);

	//se ho dei figli, faccio il release
	asset_logger->incIndent();
	res::HandleChain *p = res->figli;
	while (p)
	{
		res::HandleChain *next = p->next;
		res__release (p->res);
		res__freeHandleChain(p);
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
		res__freeHandleChain(p);
		p = next;
	}

	//libero l'handle
	resManager.raw_release(res->handle);
}

//**************************************************************** 
bool Engine::res__hotreload (res::Handle handle)
{
	res::Descr *res = res__getDescriptor(handle);
	if (NULL == res)
	{
		DBGBREAK;
		return false;
	}

	//if (res->flag1.isBitSet (res::Descr::FLAG1__MARKED_FOR_RELOAD))
	if (res::eStatus::hot_reload == res->_status)
		return false;

	//Incremento il ref count per evitare che qualcuno mi elimini la risorsa intanto che e' in hot-reload
	//L'hot-reload effettivo viene poi processata nella Engine::update();
	res->refCount++;
	res__set_status (res, res::eStatus::hot_reload);


	//se questa risorsa ha dei figli, devo farne l'hot reload
	res::HandleChain *p = res->figli;
	while (p)
	{
		res__hotreload (p->res->handle);
		p = p->next;
	}


	//aggiungo la risorsa alle lista delle risorsa di cui fare l'hot-reload
	const sUnloadInfo info = {
		.res_handle = handle,
		.timer_msec = (u32)gos::getTimeSinceStart_msec() + 1000,
	};
	list_of_res_to_be_hotreloaded.append (info);
	return true;
}
