#include "linuxOSFSWatcher.h"
#include <sys/inotify.h>
#include <poll.h>

using namespace platform;

//**********************************
OSFSWatcher::OSFSWatcher()
{
	fd = -1;
	localAllocator = gos::getSysHeapAllocator();
	folder_list.setup (localAllocator, 4 * 1024 * 1024);
	notify_map.setup (localAllocator, 256);
	fname_list.setup (localAllocator, 1 * 1024 * 1024);
	event_list.setup (localAllocator, 267);
}

//**********************************
void OSFSWatcher::priv_close()
{
	if (-1 != fd)
	{
		close(fd);
		fd = -1;
	}
}

//**********************************
void OSFSWatcher::begin()
{
	priv_close();
	folder_list.reset();
}

//**********************************
void OSFSWatcher::add_folder (const char *folder_path)
{
	char s[1024];
	gos::fs::resolvePath (folder_path, s, sizeof(s));
	gos::fs::pathSanitizeInPlace (s);
	folder_list.add (s);
}

//**********************************
bool OSFSWatcher::end()
{
	return priv_create_inotify();
}

//**********************************
bool OSFSWatcher::priv_create_inotify()
{
	priv_close();

	fd = inotify_init1 (IN_CLOEXEC | IN_NONBLOCK);
	if (-1 == fd)
	{
		gos::logger::err ("OSFSWatcher::priv_create_inotify() => error inotify_init1()\n");
		return false;
	}

	//per ogni folder da monitorare, creo un nuovo watch
	const char *folder;
	u32 iter;
	folder_list.toStart(&iter);
	while ( (folder = folder_list.next(&iter)) )
	{
		//printf ("watching %s\n", folder);
		int wd = inotify_add_watch (fd, folder, IN_CREATE | IN_MODIFY | IN_DELETE | IN_DELETE_SELF | IN_MOVE);
		if (-1 == wd)
		{
			//logger::err ("error adding watch to %s\n", folder);
			continue;
		}

		notify_map.insertIfNotExists (wd, folder);
	}

	return true;
}

//**********************************
u32 OSFSWatcher::wait (u32 timeout_msec)
{
	if (-1 == fd)	priv_create_inotify();
	if (-1 == fd)	return 0;

	nfds_t nfds;
	struct pollfd fds;

	nfds = 1;
	fds.fd = fd;
	fds.events = POLLIN;

	int wait_timeout_msec = -1;
	if (timeout_msec != u32MAX)
		wait_timeout_msec = (int)timeout_msec;

	u32 num_events = 0;		
	const int n = poll (&fds, nfds, wait_timeout_msec);
	if (n > 0)
	{
		if (fds.revents & POLLIN)
			num_events = internal__on_events_fired();
	}

	return num_events;
}

//************************************************
u32 OSFSWatcher::internal__on_events_fired ()
{
	event_list.reset();
	fname_list.reset();

	char buf[4096]	__attribute__ ((aligned(__alignof__(struct inotify_event))));
	while (1)
	{
		ssize_t size = read (fd, buf, sizeof(buf));
		if (size <= 0)
			return event_list.getNElem();

		//for (char *ptr = buf; ptr < buf + size; ptr += sizeof(struct inotify_event) + event->len)
		u32 ct = 0;
		while (ct < size)
		{
			const inotify_event *event = reinterpret_cast<const inotify_event*> (&buf[ct]);
			ct += sizeof(inotify_event) + event->len;

			const char *folder;
			if (!notify_map.find (event->wd, &folder))
				continue;


			// printf ("%s\n", folder);
			// printf ("   mask: %08X\n", event->mask);
			// if (event->len)
			// 	printf ("   name: %s\n", event->name);
			// printf ("\n");


			sEvent ev;
			ev.flag.zero();

			if (event->mask & IN_ISDIR)
				ev.flag.set (FLAG__IS_DIR);

			char s[1024];
			if (event->len)
			{
				//se il filename inizia con ".", allora ignoro l'evento perch' verosimilmente e' qualche file temporaneo
				//creato, per esempio, da Kate
				if (event->name[0] == '.')
					continue;
				sprintf_s (s, sizeof(s), "%s/%s", folder, event->name);
			}
			else
			{
				sprintf_s (s, sizeof(s), "%s", folder);
			}
			ev.fname_offset = fname_list.add (s);
			ev.fname_offset_renamed = u32MAX;

			ev.what = eWhat::unknown;
			while (1)
			{
				if (event->mask & IN_MODIFY)		{ ev.what = eWhat::modified; break; }
				if (event->mask & IN_CREATE)		{ ev.what = eWhat::created; break; }

				if (event->mask & IN_MOVED_FROM)
				{
					ev.what = eWhat::deleted;
					if (ct >= size)
						break;

					//l'elemento successivo portebbe essere un MOVED_TO ad indicare che e' stato fatto il rename
					const u32 old_ct = ct;
					const inotify_event *next_event = reinterpret_cast<const inotify_event*> (&buf[ct]);
					ct += sizeof(inotify_event) + next_event->len;

					if (next_event->mask & IN_MOVED_TO)
					{
						assert (next_event->cookie == event->cookie);
						ev.what = eWhat::renamed;

						if (next_event->len)
							sprintf_s (s, sizeof(s), "%s/%s", folder, next_event->name);
						else
							sprintf_s (s, sizeof(s), "%s", folder);
						ev.fname_offset_renamed = fname_list.add (s);							
						break;
					}
					else
						ct = old_ct;
				}

				if (event->mask & IN_MOVED_TO)	{ ev.what = eWhat::created; break; }

				//non ho capito cosa e' successo
				DBGBREAK;
				break;
			}

			if (eWhat::unknown != ev.what)
				event_list.append (ev);
		}

	}

	return event_list.getNElem();
}


//************************************************
OSFSWatcher::eWhat OSFSWatcher::event__get_what (u32 i) const
{
	assert (i < event_list.getNElem());
	return event_list(i).what;
}

//************************************************
bool OSFSWatcher::event_is_a_dir (u32 i) const
{
	assert (i < event_list.getNElem());
	return event_list(i).flag.isBitSet (FLAG__IS_DIR);
}

//************************************************
void OSFSWatcher::event__get_fullpath (u32 i, char *out__fullpath, u32 sizeof_out) const
{
	assert (i < event_list.getNElem());
	const char *s = fname_list.getStringAtOffset (event_list(i).fname_offset);
	sprintf_s (out__fullpath, sizeof_out, "%s", s);
}

//************************************************
void OSFSWatcher::event__get_renamed_fullpath (u32 i, char *out__fullpath, u32 sizeof_out) const
{
	assert (i < event_list.getNElem());
	assert (event_list(i).what == eWhat::renamed);

	const char *s = fname_list.getStringAtOffset (event_list(i).fname_offset_renamed);
	sprintf_s (out__fullpath, sizeof_out, "%s", s);
}
