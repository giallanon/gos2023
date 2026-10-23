#include "res2.h"
#include "../gosEngine.h"
#include "../../gos/gosObjectPool.h"

#undef ENABLE_DEBUG_PRINT_INFO


using namespace gos;

static gos::Logger 								*res_debug_logger = NULL;
static gos::ObjectPool<res2::ResHandleChain>	res_handleChainPool;
static gos::Engine 								*the_engine = NULL;

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
	case res2::eStatus::unloading:	return "hot_reload";
	case res2::eStatus::deleting:	return "deleting";
	case res2::eStatus::deleted:	return "deleted";
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
	case res2::eResType::materialPBR:		return "materialPBR";
	case res2::eResType::model_3d:			return "model_3d";
	case res2::eResType::model_instance:	return "model_instance";
	}
}


//***********************************
void res2::ResHandle::make_handle (u8 guard, u32 start_slot, u32 num_slot, u8 type)
{
	/*
	start_slot 	max = 4.194.304 > 22 bit
	num_slot	max = 8			> 3 bit
	res_type	max = 16		> 4 bit
	guard						> 3 bit
	*/		
	assert (start_slot < 0x00400000);
	assert (num_slot < 8);
	assert (type < 16);

	_handle = start_slot;
	_handle |= (num_slot << 22);
	_handle |= (type << 25);
	_handle |= ( (guard & 0x07) << 29);

	assert (get_start_slot() == start_slot);
	assert (get_num_slot() == num_slot);
	assert (get_type() == type);
	assert (get_gard() == (guard & 0x07));
}



//**************************************************************** 
void res2::setup (gos::Engine *engIN, gos::Allocator *allocator, gos::Logger *logger)
{
	res_debug_logger = logger;
	res_handleChainPool.setup (allocator, 8192);
	the_engine = engIN;
}

//**************************************************************** 
void res2::unsetup()
{
	res_handleChainPool.unsetup();
}


//****************************************************************
#ifdef ENABLE_DEBUG_PRINT_INFO
static void  internal__debug__printInfo (const res2::ResDescr *res, const char *debug_info)
{
	if (NULL == res_debug_logger)
		return;

	char tipo_prefix[4] = { 'r', 'e', 's', 0x00 };
	if (res->UID.isValid())
	{
		if (res->UID.isAnAsset())	
			sprintf_s (tipo_prefix, sizeof(tipo_prefix), "ass");
	}

	char status_prefix=' ';
	if (res->num_child_not_ready != 0)
		status_prefix='~';
	
	res_debug_logger->log (eTextColor::darkYellow, "%s::[%-12s] [%-20s] [ref:%03d] [st:%c%-12s/%03d] [h:%016" PRIX32 "] ",
		tipo_prefix,
		res2::enumToString(res->get_type()),
		debug_info,
		res->refCount,
		status_prefix, res2::enumToString(res->status), res->num_child_not_ready,
		res->res_handle.viewAsU32());

	
	if (res->UID.isValid())
	{
		if (res->UID.isAnAsset())
			res_debug_logger->log (eTextColor::darkYellow, "[assUID: %016" PRIX64 "]\n", res->UID._uid);
		else
			res_debug_logger->log (eTextColor::darkYellow, "[sigUID: %016" PRIX64 "]\n", res->UID._uid);
	}
	else
		res_debug_logger->log ("\n");
}
#else
	static void  internal__debug__printInfo (UNUSED_PARAM( res2::ResDescr *res), UNUSED_PARAM(const char *debug_info))	{}
#endif

//**************************************************************** 
static void internal__on_children_become_notready (res2::ResDescr *padre)
{
	//uno dei figli di <padre> era "ready" e ora e' diventato "not ready"
	assert (NULL != padre);

	const bool was_ready = is_ready(padre);
	padre->num_child_not_ready++;
	const bool now_ready = is_ready(padre);
	internal__debug__printInfo (padre, "child unrdy");

	if (was_ready && !now_ready)
	{
		//se ero "ready" e ora non lo sono +, devo informare i miei padri
		res2::ResHandleChain *p = padre->list_padri;
		while (p)
		{
			internal__on_children_become_notready (p->descr);
			p = p->next;
		}		
	}	
}

//**************************************************************** 
static void internal__on_children_become_ready (res2::ResDescr *padre)
{
	//uno dei figli di <padre> era "not ready" e ora e' diventato "ready"
	assert (NULL != padre);

	assert (padre->num_child_not_ready > 0);

	const bool was_ready = is_ready(padre);
	padre->num_child_not_ready--;
	const bool now_ready = is_ready(padre);

	internal__debug__printInfo (padre, "child rdy");

	if (!was_ready && now_ready)
	{
		//se ero "non ready" e adesso invece sono "ready", devo informare i miei padri
		the_engine->res2__callbak_on_resource_become_rdy (padre);

		res2::ResHandleChain *p = padre->list_padri;
		while (p)
		{
			internal__on_children_become_ready (p->descr);
			p = p->next;
		}		
	}
}

//**************************************************************** 
static res2::ResHandleChain* new_handleChain ()
{
	return res_handleChainPool.alloc();
}

//**************************************************************** 
static void free_handleChain (res2::ResHandleChain *p)
{
	res_handleChainPool.free(p);
}

//**************************************************************** 
bool res2::is_ready (const res2::ResDescr *res)
{
	assert (NULL != res);
	return (res2::eStatus::ready == res->status && 0 == res->num_child_not_ready);
}

//*************************************
void res2::change_state (res2::ResDescr *res, res2::eStatus new_status, const char *reason)
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
		//se ero "ready" e ora non lo sono +, devo informare i miei padri
		res2::ResHandleChain *p = res->list_padri;
		while (p)
		{
			internal__on_children_become_notready (p->descr);
			p = p->next;
		}
	}
	else if (now_ready && !was_ready)
	{
		//se ero "non ready" e adesso invece sono "ready", devo informare i miei padri
		res2::ResHandleChain *p = res->list_padri;
		while (p)
		{
			internal__on_children_become_ready (p->descr);
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
	internal__debug__printInfo (res, "ref--");
	if (res->refCount > 0)
		return false;
	return true;
}

/*************************************
 * a seguito di un release che ha ritornato true, 
 * chiamare questa per eliminare del tutto la risorsa
 */
void res2::destroy (ResDescr *res)
{
	assert (NULL != res);
	assert (res->refCount == 0);

	res2::ResHandleChain *p;

	p = res->list_figli;
	while (p)
	{
		res2::ResHandleChain *q = p;
		p = p->next;

		free_handleChain(q);
	}
	change_state (res, res2::eStatus::deleted, "deleted");
}

//*************************************
void res2::inc_refCount (res2::ResDescr *res)
{
	assert (NULL != res);
	assert (res->refCount < u16MAX);
	res->refCount++;
	internal__debug__printInfo (res, "ref++");
}

//*************************************
void res2::add_child (res2::ResDescr *padre, res2::ResDescr *child)
{
	//child diventa figlio di padre
	res2::ResHandleChain *chain = new_handleChain();
	chain->descr = child;
	chain->next = padre->list_figli;
	padre->list_figli = chain;

	//padre diventa "padre" di child
	chain = new_handleChain();
	chain->descr = padre;
	chain->next = child->list_padri;
	child->list_padri = chain;

	//se child e' not-ready, lo comunico al mio nuovo padre
	if (!is_ready(child))
		internal__on_children_become_notready (padre);
}

//*************************************
void res2::remove_child (res2::ResDescr *padre, res2::ResDescr *child)
{
	res2::ResHandleChain *figlio = padre->list_figli;
	res2::ResHandleChain *q = NULL;
	while (figlio)
	{
		if (figlio->descr == child)
		{
			if (NULL == q)
				padre->list_figli = figlio->next;
			else
				q->next = figlio->next;

			//se child era "non ready", decremento il ct di padre visto
			//che sto eliminando il figlio
			if (!is_ready(child))
			{
				assert (padre->num_child_not_ready>0);
				padre->num_child_not_ready--;
			}

			//rimuovo padre dalla lista dei padri di child
			res2::ResHandleChain *p = child->list_padri;
			q = NULL;
			while (p)
			{
				if (p->descr == padre)
				{
					if (NULL == q)
						child->list_padri = p->next;
					else
						q->next = p->next;
					break;
				}
				p = p->next;
			}
			
			
			return;
		}

		q = figlio;
		figlio = figlio->next;
	}
}

