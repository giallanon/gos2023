#ifndef _gosProtocol_h_
#define _gosProtocol_h_
#include "../gosEnumAndDefine.h"

namespace gos
{
	namespace protocol
	{
		static const u32	RES_ERROR							= 0xF0000001;
		static const u32	RES_CHANNEL_CLOSED					= 0xF0000002;
		static const u32	RES_PROTOCOL_CLOSED					= 0xF0000003;
		static const u32	RES_PROTOCOL_WRITEBUFFER_TOOSMALL	= 0xF0000004;

	} //namespace protocol
} //namespace gos

#endif // _gosIProtocol_h_

