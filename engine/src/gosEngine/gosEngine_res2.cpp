#include "gosEngine.h"


using namespace gos;
using namespace gos::engine;


//******************************** 
void Engine::res2__flushLoaderThreadMsg()
{
	static constexpr u8 LOADER_THREAD__NUM_MAX_MESSAGES_TO_READ = 16;
	thread::sMsg        msgList[LOADER_THREAD__NUM_MAX_MESSAGES_TO_READ];

	const u32 nMsg = thread::popMultipleMsg (res2__msgq_2R, msgList, LOADER_THREAD__NUM_MAX_MESSAGES_TO_READ);
	for (u32 i=0; i<nMsg; i++)
	{
		switch (msgList[i].what)
		{
		default:
			DBGBREAK;
			break;
		
		case RES2__MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_OK:
			{
				res2::CallbackData_onLoad *callback_data = reinterpret_cast<res2::CallbackData_onLoad*>( msgList[i].buffer );
				res2__callbak_on_file_loaded (callback_data);
				res2::release (callback_data->descr);
			}
			break;

		case RES2__MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_OK_WANT_CALLBACK:
			{
				res2::CallbackData_onLoad *callback_data = reinterpret_cast<res2::CallbackData_onLoad*>( msgList[i].buffer );
				const eRes2LoadResult result = res2__callbak_on_file_loaded (callback_data);
				switch (result)
				{
				default:
					DBGBREAK;
					break;

				case eRes2LoadResult::success:
				case eRes2LoadResult::fail:
					res2::release (callback_data->descr);
					thread::pushMsg (res2__msgq_1W, RES2__MSG_FOR_LOADER_THREAD__LOAD_FINISHED, 0, callback_data, sizeof(res2::CallbackData_onLoad));
					break;

				case eRes2LoadResult::pending:
					thread::pushMsg (res2__msgq_1W, RES2__MSG_FOR_LOADER_THREAD__LOAD_CONTINUED, 0, callback_data, sizeof(res2::CallbackData_onLoad));
					break;
				}
			}
			break;

		case RES2__MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_KO:
			DBGBREAK
			break;

		}

		thread::deleteMsg (msgList[i]);
	}
}


//********************************************************
void Engine::res2__register_resource (res2::eResType type, FN_res2__on_file_loaded on_file_loaded, FN_res2__on_destroy on_destroy, FN_res2__on_hot_reload on_hot_reload)
{
	const u8 index = (u8)type;
	
	assert (NULL == res2FNList_onFileLoaded[index]);	res2FNList_onFileLoaded[index] = on_file_loaded;
	assert (NULL == res2FNList_onDestroy[index]);		res2FNList_onDestroy[index] = on_destroy;
	assert (NULL == res2FNList_onHotReload[index]);		res2FNList_onHotReload[index] = on_hot_reload;
}

//********************************************************
eRes2LoadResult Engine::res2__callbak_on_file_loaded (res2::CallbackData_onLoad *data)
{
	FN_res2__on_file_loaded fn = res2FNList_onFileLoaded[(u8)data->descr->get_type()];
	if (NULL == fn)
	{
		DBGBREAK;
		return eRes2LoadResult::fail;
	}
	const eRes2LoadResult result = (this->*fn) (data);
	if (eRes2LoadResult::success == result)
		res2::change_state (data->descr, res2::eStatus::ready, "just load");
	else if (eRes2LoadResult::fail == result)
		res2::change_state (data->descr, res2::eStatus::error, "load err");

	return result;
}

//********************************************************
void Engine::res2__callbak_on_destroy (res2::ResDescr *descr)
{
	FN_res2__on_destroy fn = res2FNList_onDestroy[ (u8)descr->get_type() ];
	if (NULL != fn)
		(this->*fn) (descr);
}

//********************************************************
void Engine::res2__callbak_on_hot_reload (res2::ResDescr *asset_descr, asset2::UID new_res_signatureUID)
{
	FN_res2__on_hot_reload fn = res2FNList_onHotReload[ (u8)asset_descr->get_type() ];
	if (NULL != fn)
		(this->*fn) (asset_descr, new_res_signatureUID);
}

//********************************************************
void Engine::res2__release (res2::ResHandle handle)
{
	if (handle.is_invalid()) return;
	res2::ResDescr *descr = res2MemManager.get(handle);
	if (NULL == descr)
		return;

	if (res2::release (descr))
	{
		asset_logger->inc_indent();
		{
			res2::change_state (descr, res2::eStatus::deleting);

			//rimuovo UID dalla lista degli asset/signature noti
			bool bIsAnAsset = false;
			if (descr->UID.isValid())
			{
				if (descr->UID.isAnAsset())	
				{
					bIsAnAsset = true;
					res2_known_assetUID_HashMap.remove (descr->UID);
				}
				else
					res2_known_signatureUID_HashMap.remove (descr->UID);
			}

			asset_logger->inc_indent();
			{
				if (!bIsAnAsset)
					res2__callbak_on_destroy (descr);

				//rimuovo i miei figli e ne faccio il release
				while (descr->list_figli)
				{
					res2::ResDescr *figlio = descr->list_figli->descr;
					res2::remove_child (descr, figlio);
					res2__release (figlio->res_handle);
				}

				//mi rimuovo dalla lista dei miei padri
				while (descr->list_padri)
				{
					res2::ResDescr *padre = descr->list_padri->descr;
					res2::remove_child (padre, descr);
				}
			}
			asset_logger->dec_indent();
		}
		asset_logger->dec_indent();

		//destroy definitivo
		res2::destroy(descr);
		res2MemManager.release(handle);
	}	
}

/********************************************************
* Ritorna true solo se la risorsa e' in stato ready.
* Se non lo e', ne schedula il load se possibile
*/
bool Engine::res2__get_or_schedule_load (res2::ResDescr *descr, u32 timeout_msec)
{
	if (res2::is_ready(descr))
		return true;

	//se la risorsa non e' associata ad un signatureUID, non posso schedularne il load, quindi ritorna false
	if (descr->UID.isInvalid())
		return false;

	if (res2::eStatus::error == descr->status)
		return false;


	//se e' un asset, devo scatenare il load della risorsa sua figlia
	if (descr->isAnAsset())
	{
		//gli asset hanno un solo figlio, che e' la risorsa vera e propria
		assert (NULL == descr->list_figli->next);
		res2__get_or_schedule_load (descr->list_figli->descr, 0);
	}
	else
	{
		if (!res2::is_ready(descr))
		{
			//dato che la risorsa e' <not ready>, dovrebbe significare che io o uno o + dei miei figli siamo
			//not-ready il che possibilmente significa che siamo non-loaded

			asset_logger->inc_indent();
			res2::ResHandleChain *p = descr->list_figli;
			while (p)
			{
				res2::ResDescr *child_descr = p->descr;
				p = p->next;
				if (!res2::is_ready(child_descr))
					res2__get_or_schedule_load (child_descr, 0);
			}
			asset_logger->dec_indent();

			//schedulo il load di me stesso se necessario
			//Incremento il ref count perche' sto passando la risorsa al loader-thread e questo garantisce che 
			//la risorsa non verra' eliminata da eventuali release()
			if (res2::eStatus::unloaded == descr->status)
			{
				change_state (descr, res2::eStatus::loading, "loading");
				res2::inc_refCount(descr);
				thread::pushMsg (res2__msgq_1W, RES2__MSG_FOR_LOADER_THREAD__LOAD, 0, descr);
			}

			if (0 == timeout_msec)
				return false;		
		}
	}

	if (res2::eStatus::unloaded == descr->status)
	{
		//NB: 	qui sono sicuro che ci entro solo se non sono un asset dato che gli asset per
		//		definizione sono sempre ready (e' il figlio dell'asset che potrebbe non essere ready)
		assert (!descr->isAnAsset());

		//dato che l'asset e' notLoaded, schedulo il load suo e dei suoi figli
		//Incremento il ref count perche' sto passando la risorsa al loader-thread e questo garantisce che 
		//la risorsa non verra' eliminata da eventuali release()
		change_state (descr, res2::eStatus::loading, "loading");
		res2::inc_refCount(descr);

		asset_logger->inc_indent();
		res2::ResHandleChain *p = descr->list_figli;
		while (p)
		{
			res2::ResDescr *child_descr = p->descr;
			p = p->next;
			res2__get_or_schedule_load (child_descr, 0);
		}
		asset_logger->dec_indent();

		//schedulo il load di me stesso
		thread::pushMsg (res2__msgq_1W, RES2__MSG_FOR_LOADER_THREAD__LOAD, 0, descr);

		if (0 == timeout_msec)
			return false;		
	}

	//se richiesto, attendo per un po' nella speranza di vedere l'asset "ready"
	if (timeout_msec > 0 && res2::eStatus::error != descr->status)
	{
		u64 time_to_exit_msec = gos::getTimeSinceStart_msec() + timeout_msec;
		while (gos::getTimeSinceStart_msec() < time_to_exit_msec)
		{
			res2__flushLoaderThreadMsg();
			if (res2::is_ready(descr))
				return true;

			if (res2::eStatus::error == descr->status)
				return false;
		}
	}
	return false;	
}	