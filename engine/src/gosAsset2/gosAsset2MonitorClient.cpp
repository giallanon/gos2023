#include "gosAsset2MonitorClient.h"

using namespace gos;
using namespace gos::asset2;


//************************************ 
MonitorClient::MonitorClient()
{
	localAllocator = gos::getSysHeapAllocator();
	channel = NULL;
	protocol = NULL;
	bufferR.setup (localAllocator, 1024);
}

//************************************ 
void MonitorClient::priv_free()
{
	if (NULL != protocol)
	{
		GOSDELETE(localAllocator, protocol);
		protocol = NULL;
	}

	if (NULL != channel)
	{
		GOSDELETE(localAllocator, channel);
		channel = NULL;
	}	
}

//************************************ 
bool MonitorClient::connect()
{
	priv_free();

	gos::Socket sok;
	socket::init (&sok);
	eSocketError err = socket::openAsTCPClient (&sok, "127.0.0.1", Monitor::TCP_PORT);
	if (eSocketError::none != err)
	{
		logger::err ("error connecting to Monitor Server: %s\n", utils::enumToString(err));
		priv_free();
		return false;
	}

	channel = GOSNEW(localAllocator, gos::ProtocolChSocketTCP)(localAllocator, 1024, 4096);
	channel->bindSocket(sok);


	protocol = GOSNEW(localAllocator, gos::ProtocolConsole) (localAllocator, 128);
	if (!protocol->handshake_clientSend (channel, logger::get_system_logger()))
	{
		logger::err ("failed to handshake with Monitor Server\n");
		priv_free();
		return false;
	}

	return true;
}

//************************************ 
void MonitorClient::disconnect()
{
	if (NULL == protocol)
		return;

	protocol->close (channel);
	priv_free();
}

//************************************ 
bool MonitorClient::read(asset2::UID *out__UID)
{
	if (NULL == protocol)
		return false;

	bufferR.reset();
	const u32 n = protocol->read (channel, 0, bufferR);
	if (0 == n)
		return false;
	if (n >= protocol::RES_ERROR)
	{
		priv_free();
		return false;
	}

	const u8 *p = bufferR._getPointer(0);
	const u8 op = p[0];
	switch (op)
	{
	default:
		logger::warn ("MonitorClient::Read() => invalid OP rcv [%d]\n", op);
		break;

	case 0x01:
		out__UID->_uid = utils::bufferReadU64(&p[1]);
		//logger::log ("MonitorClient::Read() => UID %016" PRIX64 "\n", out__UID->_uid);
		return true;
	}

	return false;
}