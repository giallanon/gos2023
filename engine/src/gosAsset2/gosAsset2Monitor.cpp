#include "gosAsset2Monitor.h"
#include "gosWaitableGrp.h"

using namespace gos;
using namespace gos::asset2;


//************************************************
Monitor::Monitor(gos::GPU *gpuIN)
{
	localAllocator = gos::getSysHeapAllocator();
	
	gpu = gpuIN;
	server = NULL;
	bufferR.setup (localAllocator, 1024);
}

//************************************************
Monitor::~Monitor()
{
	priv_unsetup_server();
}

//************************************************
bool Monitor::priv_setup_server()
{
	if (NULL != server)
		return true;

	server = GOSNEW(localAllocator, gos::ServerTCP)(8, localAllocator);
	eSocketError err = server->start(TCP_PORT);
	if (eSocketError::none == err)
	{
		logger::log ("Server started on port %d\n", TCP_PORT);
		return true;
	}

	logger::err ("Server_setup() => %s\n", utils::enumToString(err));
	return false;
}

//************************************************
void Monitor::priv_unsetup_server()
{
	if (NULL == server)
		return;
	server->close();

	GOSDELETE(localAllocator, server);
	server = NULL;
}


//************************************************
bool Monitor::priv_scan_DB_and_add_path (const char *path_to_DB, FSWatcher *fsw)
{
	fsw->begin();
	{
		//creo un elenco di folder unici da monitorare e li agiungo al monitor
		DBContext ctx;
		if (!asset2::dbcontext_open (path_to_DB, false, &ctx))
		{
			logger::err ("Can't open DB @ %s\n", path_to_DB);
			return false;
		}

		db::RST rst;
		char s[1024];
		sprintf_s (s, sizeof(s), "SELECT DISTINCT abspath FROM " GOS_ASSET2__TABLE_RES " ORDER BY abspath");
		if (db::query (ctx.db, s, &rst))
		{
			while (rst.fetchRow())
			{
				const char *asset_full_filename = rst.getVal(0);
				fs::extractFilePathWithOutSlash (asset_full_filename, s, sizeof(s));
				fsw->add_folder(s);
			}
		}
		asset2::dbcontext_close (ctx);
	}
	fsw->end();

	return true;
}

//************************************************
bool Monitor::run (const char *path_to_DB)
{
	if (!priv_setup_server())
	{
		logger::err ("Failed to setup server TCP\n");
		return false;
	}


	FSWatcher fsWatcher;
	if (!priv_scan_DB_and_add_path (path_to_DB, &fsWatcher))
		return false;

	logger::log ("Monitoring %s\n", path_to_DB);
	logger::log ("CTRL c to terminate\n", path_to_DB);
	server->fsWatcher__add_to_wait_list (&fsWatcher);



	static const u32 WAIT_BEFORE_BUILD_msec = 5000;
	u64 time_to_rebuild_msec = 0;
	while (1)
	{
		u32 max_time_wait_msec = u32MAX;
		if (0 != time_to_rebuild_msec)
		{
			const u64 timenow_msec = gos::getTimeSinceStart_msec();
			if (timenow_msec >= time_to_rebuild_msec)
			{
				server->fsWatcher__remove_from_wait_list (&fsWatcher);
				priv_build (path_to_DB);
				time_to_rebuild_msec = 0;

				//riscanno il DB
				if (!priv_scan_DB_and_add_path (path_to_DB, &fsWatcher))
					break;				
				server->fsWatcher__add_to_wait_list (&fsWatcher);
			}
			else
				max_time_wait_msec = (u32) (time_to_rebuild_msec - timenow_msec);
		}

		//wait for events
		const u8 num_events = server->wait (max_time_wait_msec);
		for (u8 iEvent=0; iEvent<num_events; iEvent++)
		{
			switch (server->event__get_type(iEvent))
			{
			default:
				DBGBREAK;

			case ServerTCP::eEventType::fsw_has_data_avail:
				if (priv_handle_fswEvents (path_to_DB, &fsWatcher))
				{
					time_to_rebuild_msec = gos::getTimeSinceStart_msec() + WAIT_BEFORE_BUILD_msec;
				}
				break;

			case ServerTCP::eEventType::new_client_connected:
				{
					HSokServerClient hClient = server->event__get_client_handle(iEvent);
					logger::log ("New client connected %08X\n", hClient.viewAsU32());
				}
				break;

			case ServerTCP::eEventType::client_has_data_avail:
				{
					HSokServerClient hClient = server->event__get_client_handle(iEvent);
					const u32 num_read = server->client_read (hClient, bufferR);
					if (u32MAX == num_read)
						logger::log ("Client diconnected %08X\n", hClient.viewAsU32());
				}
				break;
			}
		}


		if (0 != time_to_rebuild_msec)
			logger::log (eTextColor::green, "Change detected, will run a rebuild in %d of seconds...\n", WAIT_BEFORE_BUILD_msec/1000);
	}

	return true;
}

//************************************************
bool Monitor::priv_handle_fswEvents (const char *path_to_DB, gos::FSWatcher *fsw)
{
	const u32 num_events = fsw->event__get_num();
	for (u32 i=0; i<num_events; i++)
	{
		char s[1024];
		fsw->event__get_fullpath (i, s, sizeof(s));

		switch (fsw->event__get_what(i))
		{
		default:							logger::log ("??? %s\n", s); break;
		case FSWatcher::eWhat::created:		logger::log ("CREATED %s\n", s); break;
		case FSWatcher::eWhat::deleted:		logger::log ("DELETED %s\n", s); break;
		case FSWatcher::eWhat::modified:	logger::log ("MODIFIED %s\n", s); break;
		case FSWatcher::eWhat::renamed:
			logger::log ("RENAMED %s in ", s);

			fsw->event__get_renamed_fullpath (i, s, sizeof(s));
			logger::log ("%s\n", s);
			break;
		}
	}

	return (num_events != 0);
}

//************************************************
void Monitor::priv_build(const char *path_to_DB)
{
    asset2::Builder b(gpu);
    if (b.build (path_to_DB, true))
    {
        b.save_dependencies_report (path_to_DB);
        b.save_asset_manifest (path_to_DB);
        b.debug_sanityCheck(path_to_DB);


		//elenco degli UID che sono stati influenzati dalla build
		const asset2::UniqueUIDList *list = b.get_list_of_built_UID();
		if (0 == list->getNElem())
		{
			logger::log ("No asset has been updated\n");
		}
		else
		{
			logger::log ("List of updated assets:\n");
			logger::inc_indent();

			const gos::FastArray<asset2::UID> *the_list = list->_queryList();
			for (u32 i=0; i<the_list->getNElem(); i++)
			{
				asset2::UID uid = the_list->queryElem(i);
				logger::log ("%016" PRIX64 "\n", uid._uid);

				//preparo il msg per i client
				u32 ct = 0;
				u8 bufferW[32];
				bufferW[ct++] = 0x01;	//cmd
				ct += utils::bufferWriteU64 (&bufferW[ct], uid._uid);


				const u32 num_client = server->client_getNumConnected();
				for (u32 iClient=0; iClient<num_client; iClient++)
				{
        			HSokServerClient sok = server->client_getByIndex(iClient);
					server->client_writeBuffer (sok, bufferW, (u16)ct);
				}
			}
			logger::dec_indent();
		}
    }
}


