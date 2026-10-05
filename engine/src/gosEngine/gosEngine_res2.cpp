#include "gosEngine.h"


using namespace gos;
using namespace gos::engine;


//******************************** 
void Engine::res2__flushLoaderThreadMsg()
{
	const u32 nMsg = thread::popMultipleMsg (res2__msgq_2R, res2__loaderMsgList, LOADER_THREAD__NUM_MAX_MESSAGES_TO_READ);
	for (u32 i=0; i<nMsg; i++)
	{
		switch (res2__loaderMsgList[i].what)
		{
		default:
			DBGBREAK;
			break;
		
		case RES2__MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_OK:
			{
				res2::CallbackData_onLoad *callback_data = reinterpret_cast<res2::CallbackData_onLoad*>( res2__loaderMsgList[i].buffer );
				res2__callbak_on_file_loaded (callback_data);
				res2::release (callback_data->descr);
				thread::pushMsg (res2__msgq_1W, RES2__MSG_FOR_LOADER_THREAD__RESOURCE_LOAD_FINISHED, 0, callback_data);
			}
			break;

		case RES2__MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_KO:
			DBGBREAK
			break;

		}

		thread::deleteMsg (res2__loaderMsgList[i]);
	}
}


//********************************************************
void Engine::res2__register_resource (res2::eResType type, FN_res2__on_file_loaded on_file_loaded, FN_res2__on_unload on_unload, FN_res2__on_destroy on_destroy)
{
	const u8 index = (u8)type;
	
	assert (NULL == res2FNList_resFileLoaded[index]);	res2FNList_resFileLoaded[index] = on_file_loaded;
	assert (NULL == res2FNList_resUnload[index]);		res2FNList_resUnload[index] = on_unload;
	assert (NULL == res2FNList_resDestroy[index]);		res2FNList_resDestroy[index] = on_destroy;
}

//********************************************************
void Engine::res2__callbak_on_file_loaded (res2::CallbackData_onLoad *data)
{
	FN_res2__on_file_loaded fn = res2FNList_resFileLoaded[(u8)data->descr->res_handle.get_type()];
	if (NULL == fn)
		return;
	const bool result = (this->*fn) (data);
	if (result)
		res2::change_state (data->descr, res2::eStatus::ready, "just load");
	else
		res2::change_state (data->descr, res2::eStatus::error, "load err");
}

//********************************************************
void Engine::res2__callbak_on_destroy (res2::ResDescr *descr)
{
	FN_res2__on_destroy fn = res2FNList_resDestroy[ (u8)descr->res_handle.get_type() ];
	if (NULL != fn)
		(this->*fn) (descr);
}

//********************************************************
void Engine::res2__release (res2::ResHandle &handle)
{
	if (handle.is_invalid()) return;
	res2::ResDescr *descr = res2MemManager.get(handle);
	if (NULL != descr)
	{
		if (res2::release (descr))
		{
			res2__callbak_on_destroy (descr);
			res2MemManager.release(handle);
		}
	}
	handle.set_invalid();
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
	if (descr->signatureUID.isInvalid())
		return false;							


	if (res2::eStatus::unloaded == descr->status)
	{
		//dato che la risorsa e' notLoaded, schedulo il suo load
		//Incremento il ref count perche' sto passando la risorsa al loader-thread e questo garantisce che 
		//la risorsa non verra' eliminata da eventuali release()
		res2::inc_refCount(descr);
		change_state (descr, res2::eStatus::loading, "loading");

		//schedulo il load di me stesso
		thread::pushMsg (res2__msgq_1W, RES2__MSG_FOR_LOADER_THREAD__RESOURCE_LOAD, 0, descr);

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