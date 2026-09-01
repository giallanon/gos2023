#ifdef GOS_PLATFORM__WINDOWS
#include "winOSFSWatcher.h"


using namespace platform;

//**********************************
OSFSWatcher::OSFSWatcher()
{
	localAllocator = gos::getSysHeapAllocator();
	folder_list.setup (localAllocator, 4 * 1024 * 1024);
	handle_list.setup (localAllocator, 256);
	fname_list.setup (localAllocator, 1 * 1024 * 1024);
	event_list.setup (localAllocator, 256);
}

//**********************************
void OSFSWatcher::priv_close()
{
	for (u32 i=0; i<handle_list.getNElem(); i++)
	{
		::FindCloseChangeNotification (handle_list(i));
	}
	handle_list.reset();
	folder_list.reset();
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
	priv_close();


	//per ogni folder da monitorare, creo un nuovo handle
	const char *folder;
	u32 iter;
	folder_list.toStart(&iter);
	while ( (folder = folder_list.next(&iter)) )
	{
		//printf ("watching %s\n", folder);
		
		wchar_t the_folder[512];
		win32::utf8_towchar (folder, u32MAX, the_folder, sizeof(the_folder));
		HANDLE handle = ::FindFirstChangeNotification (the_folder, TRUE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE);
		if (INVALID_HANDLE_VALUE == handle)
		{
			gos::logger::err ("error adding watch to %s\n", folder);
			continue;
		}

		handle_list.append (handle);
	}

	return true;
}

//**********************************
u32 OSFSWatcher::wait (u32 timeout_msecIN)
{
	const DWORD num_handle = (DWORD)handle_list.getNElem();
	const HANDLE *handle_array = handle_list._queryTypedPointer();
	DWORD timeout_msec = (DWORD)timeout_msecIN;
	if (u32MAX == timeout_msecIN)
		timeout_msec = INFINITE;

	DWORD result = ::WaitForMultipleObjects (num_handle, handle_array, FALSE, timeout_msec); 

	if (WAIT_TIMEOUT == result)
		return 0;
	
	if (WAIT_FAILED == result)
	{
		DBGBREAK;
		return 0;
	}
	
	DWORD index;
	if (result >= WAIT_ABANDONED_0)
		index = result - WAIT_ABANDONED_0;
	else
		index = result - WAIT_OBJECT_0;

	//riarmo l'handle
	HANDLE handle = handle_array[index];
	::FindNextChangeNotification (handle);

	return 1;
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


#endif //#ifdef GOS_PLATFORM__WINDOWS