#ifndef _gosAsset2MonitorClient_h_
#define _gosAsset2MonitorClient_h_
#include "gosAsset2Monitor.h"
#include "protocol/gosProtocolConsole.h"
#include "protocol/gosProtocolChSocketTCP.h"

namespace gos
{
	namespace asset2
	{
		/**************************************************
		* MonitorClient
		* 
		*/
		class MonitorClient
		{
		public:
					MonitorClient();
					~MonitorClient()		{ priv_free(); }

			bool	connect();
			bool	read (asset2::UID *out__UID);
			void	disconnect();

		private:
			void	priv_free();

		private:
			Allocator			*localAllocator;
			ProtocolChSocketTCP	*channel;
			ProtocolConsole		*protocol;
			ProtocolBuffer		bufferR;
			
		};

	} //namespace asset2
} //namespace gos


#endif //_gosAsset2MonitorClient_h_
