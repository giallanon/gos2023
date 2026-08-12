#ifndef _linuxOSFSWatcher_h_
#define _linuxOSFSWatcher_h_
#include "../../gosFSWatcherInterface.h"
#include "../../string/gosUniqueStringList.h"
#include "../../string/gosStringList.h"
#include "../../gosHashMap.h"
#include "../../gosBit.h"
#include "../../gos.h"

namespace platform
{
	/********************************************************************
	 * OSFSWatcher
	 *
	 * Implementazione dell'interfaccia gos::FSWatcherInterface
	 */
	class OSFSWatcher : public gos::FSWatcherInterface
    {
    public:
				OSFSWatcher();
				~OSFSWatcher()							{ priv_close(); }

		void	begin ();
		void	add_folder (const char *folder_path);
		bool	end();
		
		u32		wait (u32 timeout_msec);

		u32		event__get_num () const							{ return event_list.getNElem(); }
		eWhat	event__get_what  (u32 i) const;
		void	event__get_fullpath (u32 i, char *out__fullpath, u32 sizeof_out) const;
		void	event__get_renamed_fullpath (u32 i, char *out__fullpath, u32 sizeof_out) const;
		bool	event_is_a_dir (u32 i) const;


		//usate da WaitableGrp
		int		internal__open_fd() 							{ if (-1 == fd) priv_create_inotify(); return fd; }
		void	internal__close_fd()							{ priv_close(); }
		u32 	internal__on_events_fired();
		
	private:
		static constexpr u8	FLAG__IS_DIR	= 0;

		struct sEvent
		{
			u32			fname_offset;
			u32			fname_offset_renamed;	//valido solo in caso di what==renamed
			eWhat		what;
			gos::Flag8	flag;
		};

	private:
		bool	priv_create_inotify();
		
		void 	priv_close();

	private:
		gos::Allocator						*localAllocator;
		gos::UniqueStringList				folder_list;
		gos::FastHashMap<int, const char*> 	notify_map;
		gos::StringList						fname_list;
		gos::FastArray<sEvent>				event_list;
		int									fd;

	};

} //namespace platform

#endif //_linuxOSFSWatcher_h_


