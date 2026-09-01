#ifdef GOS_PLATFORM__WINDOWS
#ifndef _winOSFSWatcher_h_
#define _winOSFSWatcher_h_
#include "winOS.h"
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
				~OSFSWatcher()									{ priv_close(); }

		void	begin ();
		void	add_folder (const char *folder_path);
		bool	end();
		
		u32		wait (u32 timeout_msec);

		u32		event__get_num () const							{ return event_list.getNElem(); }
		eWhat	event__get_what  (u32 i) const;
		void	event__get_fullpath (u32 i, char *out__fullpath, u32 sizeof_out) const;
		void	event__get_renamed_fullpath (u32 i, char *out__fullpath, u32 sizeof_out) const;
		bool	event_is_a_dir (u32 i) const;

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
		void 	priv_close();

	private:
		gos::Allocator					*localAllocator;
		gos::UniqueStringList			folder_list;
		gos::StringList					fname_list;
		gos::FastArray<sEvent>			event_list;
		gos::FastArray<HANDLE>			handle_list;

	};

} //namespace platform

#endif //_winOSFSWatcher_h_

#endif //GOS_PLATFORM__WINDOWS