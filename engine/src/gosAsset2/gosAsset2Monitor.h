#ifndef _gosAsset2Monitor_h_
#define _gosAsset2Monitor_h_
#include "gosAsset2Builder.h"
#include "gosFSWatcher.h"
#include "gosServerTCP.h"

namespace gos
{
	namespace asset2
	{
		/**************************************************
		* Monitor
		* 
		*/
		class Monitor
		{
		public:
			static constexpr u16 TCP_PORT = 12374;

        public:
					Monitor (gos::GPU *gpuIN);
					~Monitor();

			bool	run (const char *path_to_DB);

		private:
			bool	priv_setup_server();
			void	priv_unsetup_server();

			bool	priv_scan_DB_and_add_path (const char *path_to_DB, FSWatcher *fsw);
			bool	priv_do_monitor();
			void	priv_build (const char *path_to_DB);
			bool	priv_handle_fswEvents (const char *path_to_DB, gos::FSWatcher *fsw);

		private:
			gos::Allocator	*localAllocator;
			gos::GPU			*gpu;
			ServerTCP			*server;
			gos::ProtocolBuffer	bufferR;
		};
	} //namespace asset2
} //namespace gos

#endif //_gosAsset2Monitor_h_

