#ifdef GOS_PLATFORM__WINDOWS
#include "winOS.h"
#include "../../gosEnumAndDefine.h"
#include <ws2tcpip.h>


//*************************************************
void platform::socket_init (OSSocket *sok)
{
	assert(sok != NULL);
	sok->socketID = INVALID_SOCKET;
	sok->readTimeoutMSec = 10000;
	//sok->hEventNotify = INVALID_HANDLE_VALUE;
}

//*************************************************
void platform::socket_close (OSSocket &sok)
{
	if (platform::socket_isOpen(sok))
	{
		closesocket(sok.socketID);
		sok.socketID = INVALID_SOCKET;
	}
}

//*************************************************
bool setBlockingMode (OSSocket &sok, bool bBlocking)
{
	u_long iMode = 0;
	if (!bBlocking)
		iMode = 1;

	if (ioctlsocket(sok.socketID, FIONBIO, &iMode) != 0)
	{
		//printf("ioctlsocket failed with error: %ld\n", WSAGetLastError());
		return false;
	}
	return true;
}

//*************************************************
eSocketError socket_openAsTCP(OSSocket *sok)
{
	platform::socket_init(sok);

	//creo la socket
	sok->socketID = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sok->socketID == INVALID_SOCKET)
	{
		switch (WSAGetLastError())
		{
		case EACCES:            return eSocketError::denied;

		case EPROTONOSUPPORT:
		case EINVAL:
		case EAFNOSUPPORT:      return eSocketError::unsupported;

		case EMFILE:
		case ENFILE:            return eSocketError::tooMany;

		case ENOBUFS:
		case ENOMEM:            return eSocketError::noMem;

		default:                return eSocketError::unknown;
		}
	}

	//abilito delle opzioni di socket
	BOOL enable = TRUE;
	setsockopt(sok->socketID, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(BOOL));
	//setsockopt(sok->socketID, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(BOOL));

	return eSocketError::none;
}

//*************************************************
eSocketError platform::socket_openAsTCPClient(OSSocket* sok, const char* connectToIP, u32 portNumber)
{
	eSocketError sockErr = socket_openAsTCP(sok);

	if(sockErr == eSocketError::none)
	{
		sockaddr_in clientService;
		clientService.sin_family = AF_INET;
		clientService.sin_addr.s_addr = inet_addr(connectToIP);
		clientService.sin_port = htons(static_cast<USHORT>(portNumber));

		if (0 != connect(sok->socketID, (SOCKADDR*)&clientService, sizeof(clientService)))
		{
			switch (WSAGetLastError())
			{
			case WSA_INVALID_HANDLE:
				sockErr = eSocketError::invalidDescriptor;
				break;
			case WSA_NOT_ENOUGH_MEMORY:
				sockErr = eSocketError::noMem;
				break;
			case WSA_INVALID_PARAMETER:
				sockErr = eSocketError::invalidParameter;
				break;
			case WSAEACCES:
				sockErr = eSocketError::addressProtected;
				break;
			case WSAEADDRINUSE:
				sockErr = eSocketError::addressInUse;
				break;
			case WSAEINVAL:
				sockErr = eSocketError::alreadyBound;
				break;
			case WSAECONNREFUSED:
				sockErr = eSocketError::connRefused;
				break;
			case WSAETIMEDOUT:
				sockErr = eSocketError::timedOut;
				break;
			default:
				sockErr = eSocketError::unknown;
				break;
			}

			::closesocket(sok->socketID);
			sok->socketID = static_cast<SOCKET>(-1);
		}
	}

	return sockErr;
}

//*************************************************
eSocketError platform::socket_openAsTCPServer (OSSocket *sok, int portNumber)
{
	eSocketError sokErr = socket_openAsTCP(sok);
	if (sokErr != eSocketError::none)
		return sokErr;


	//blocking
	setBlockingMode(*sok, true);

	//la bindo
	struct sockaddr_in socket_config;
	socket_config.sin_family = AF_INET;
	socket_config.sin_addr.s_addr = INADDR_ANY;
	socket_config.sin_port = htons(static_cast<USHORT>(portNumber));

	if (0 != bind(sok->socketID, (struct sockaddr *)&socket_config, sizeof(socket_config)))
	{
		::closesocket(sok->socketID);
		sok->socketID = INVALID_SOCKET;
		switch (errno)
		{
		case EACCES:        return eSocketError::addressProtected;
		case EADDRINUSE:    return eSocketError::addressInUse;
		case EINVAL:        return eSocketError::alreadyBound;
		case ENOTSOCK:      return eSocketError::invalidDescriptor;
		case ENOMEM:        return eSocketError::noMem;
		default:            return eSocketError::unknown;
		}
	}


	//read timeout di default
	platform::socket_setReadTimeoutMSec(*sok, sok->readTimeoutMSec);

	//tutto ok
	return eSocketError::none;
}


//*************************************************
bool platform::socket_setReadTimeoutMSec  (OSSocket &sok, u32 timeoutMSecIN)
{
	if (!platform::socket_isOpen(sok))
		return false;

	DWORD timeoutMSec = timeoutMSecIN;
	if (timeoutMSecIN == u32MAX)
		timeoutMSec = 0;    //socket sempre bloccante
	else if (timeoutMSec == 0)
		timeoutMSec = 1;    //socket con il minimo possibile tempo di wait

	if (0 == setsockopt (sok.socketID, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeoutMSec, sizeof(DWORD)))
	{
		sok.readTimeoutMSec = timeoutMSecIN;
		return true;
	}

	/*switch (errno)
	{
	case EBADF:         printf ("Error code=EBADF\n"); break;
	case EFAULT:        printf ("Error code=EFAULT\n"); break;
	case EINVAL:        printf ("Error code=EINVAL\n"); break;
	case ENOPROTOOPT:   printf ("Error code=ENOPROTOOPT\n"); break;
	case ENOTSOCK:      printf ("Error code=ENOTSOCK\n"); break;

	default:
		printf ("Error code=%d\n", errno);
		break;
	}
	*/
	return false;
}

//*************************************************
bool platform::socket_setWriteTimeoutMSec (OSSocket &sok, u32 timeoutMSecIN)
{
	if (!platform::socket_isOpen(sok))
		return false;
	
	DWORD timeoutMSec = (DWORD)timeoutMSecIN;
	if (timeoutMSecIN == u32MAX)
		timeoutMSec = 0;    //socket sempre bloccante
	else if (timeoutMSec == 0)
		timeoutMSec = 1;    //socket con il minimo possibile tempo di wait

	
	if (0 == setsockopt(sok.socketID, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeoutMSec, sizeof(DWORD)))
		return true;

	/*switch (errno)
	{
	case EBADF:         printf ("Error code=EBADF\n"); break;
	case EFAULT:        printf ("Error code=EFAULT\n"); break;
	case EINVAL:        printf ("Error code=EINVAL\n"); break;
	case ENOPROTOOPT:   printf ("Error code=ENOPROTOOPT\n"); break;
	case ENOTSOCK:      printf ("Error code=ENOTSOCK\n"); break;

	default:
		printf ("Error code=%d\n", errno);
		break;
	}
	*/
	return false;
}

//*************************************************
bool platform::socket_listen (const OSSocket &sok, u16 maxIncomingConnectionQueueLength)
{
	if (!platform::socket_isOpen(sok))
		return false;

	int err;
	if (u16MAX == maxIncomingConnectionQueueLength)
		err = ::listen(sok.socketID, SOMAXCONN);
	else
		err = ::listen(sok.socketID, (int)maxIncomingConnectionQueueLength);

	return (err == 0);
}

//*************************************************
bool platform::socket_accept (const OSSocket &sok, OSSocket *out_clientSocket)
{
	if (!platform::socket_isOpen(sok))
		return false;

	socket_init(out_clientSocket);

	SOCKET clientSocketID = ::accept(sok.socketID, NULL, NULL);
	if (clientSocketID == INVALID_SOCKET)
		return false;

	out_clientSocket->socketID = clientSocketID;
	setBlockingMode(*out_clientSocket, true);
	platform::socket_setReadTimeoutMSec(*out_clientSocket, out_clientSocket->readTimeoutMSec);
	return true;
}


//*************************************************
i32 platform::socket_read(OSSocket &sok, void *buffer, u16 bufferSizeInBytes, u32 timeoutMSec, bool bPeekMSG)
{
	u32 readFLAG = 0;
	if (bPeekMSG)
		readFLAG = MSG_PEEK;

	if (timeoutMSec != sok.readTimeoutMSec)
		platform::socket_setReadTimeoutMSec(sok, timeoutMSec);
		

	//purtroppo devo pollare.... in teoria socket_setReadTimeoutMSec dovrebbe settare
	//la socket in modalit� bloccante con il timeout deciso, ma di fatto non funziona quindi mi tocca pollare
	u64 timeToExitMSec = gos::getTimeNowMSec() + timeoutMSec;
	i32 ret = -1;
	do
	{
		ret = ::recv(sok.socketID, (char*)buffer, bufferSizeInBytes, readFLAG);
		if (ret == 0)
			return 0;	//socket closed
		if (ret != SOCKET_ERROR)
			return ret;

		assert(ret == SOCKET_ERROR);
		int myerrno = WSAGetLastError();
		if (myerrno == WSAEINPROGRESS || myerrno == WSAEWOULDBLOCK || myerrno == WSAETIMEDOUT)
		{
			platform::sleepMSec(100);
			continue;
		}

		//DBGBREAK;
		switch (myerrno)
		{
		case WSANOTINITIALISED:
			return 0;
		case WSAENETDOWN:
			return 0;
		case WSAEFAULT:
			return 0;
		case WSAENOTCONN:
			return 0;
		case WSAEINTR:
			return 0;
		case WSAENETRESET:
			return 0;
		case WSAENOTSOCK:
			return 0;
		case WSAEOPNOTSUPP:
			return 0;
		case MSG_OOB:
			return 0;
		case WSAESHUTDOWN:
			return 0;
		case WSAEMSGSIZE:
			return 0;
		
		case WSAECONNRESET:
			return 0;

		case WSAEINVAL:
			return 0;

		case WSAECONNABORTED:
			return 0;
		}
	} while (gos::getTimeNowMSec() < timeToExitMSec);

	return -1;  //timeout
}

//*************************************************
i32  platform::socket_write(OSSocket &sok, const void *buffer, u16 nByteToSend)
{
	i32 ret = ::send(sok.socketID, (const char*)buffer, nByteToSend, 0);
	if (ret == SOCKET_ERROR)
	{
		int myerrno = WSAGetLastError();
		if (myerrno == WSAEINPROGRESS || myerrno == WSAEWOULDBLOCK || myerrno == WSAETIMEDOUT)
			return 0;

		/*DBGBREAK;
		switch (myerrno)
		{
		case WSANOTINITIALISED:
			return 0;
		case WSAENETDOWN:
			return 0;
		case WSAEFAULT:
			return 0;
		case WSAENOTCONN:
			return 0;
		case WSAEINTR:
			return 0;
		case WSAENETRESET:
			return 0;
		case WSAENOTSOCK:
			return 0;
		case WSAEOPNOTSUPP:
			return 0;
		case MSG_OOB:
			return 0;
		case WSAESHUTDOWN:
			return 0;
		case WSAEMSGSIZE:
			return 0;

		case WSAECONNRESET:
			return 0;

		case WSAEINVAL:
			return 0;

		case WSAECONNABORTED:
			return 0;
		}
		*/
		return 0;
	}
	
	assert(ret <= nByteToSend);
	return ret;
}



//*************************************************
eSocketError platform::socket_openAsUDP(OSSocket *sok)
{
	platform::socket_init(sok);

	//creo la socket
	sok->socketID = socket(AF_INET, SOCK_DGRAM, 0);
	if (sok->socketID == INVALID_SOCKET)
	{
		switch (WSAGetLastError())
		{
		case EACCES:            return eSocketError::denied;

		case EPROTONOSUPPORT:
		case EINVAL:
		case EAFNOSUPPORT:      return eSocketError::unsupported;

		case EMFILE:
		case ENFILE:            return eSocketError::tooMany;

		case ENOBUFS:
		case ENOMEM:            return eSocketError::noMem;

		default:                return eSocketError::unknown;
		}
	}

	//abilito delle opzioni di socket
	BOOL enable = TRUE;
	setsockopt(sok->socketID, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(BOOL));
	//setsockopt(sok->socketID, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(BOOL));

	// non bloccante
	unsigned long lNonBlocking = 1;
	ioctlsocket(sok->socketID, FIONBIO, &lNonBlocking);


	return eSocketError::none;
}

//*************************************************
eSocketError platform::socket_UDPbind (OSSocket &sok, int portNumber)
{
	sockaddr_in		saAddress;
	saAddress.sin_family = AF_INET;
	saAddress.sin_port = htons(static_cast<USHORT>(portNumber));
	saAddress.sin_addr.s_addr = htonl(static_cast<USHORT>(INADDR_ANY));
	if (SOCKET_ERROR == bind(sok.socketID, (LPSOCKADDR)&saAddress, sizeof(saAddress)))
		return eSocketError::unknown;

	return eSocketError::none;
}

//*************************************************** 
u32 platform::socket_UDPSendTo (OSSocket &sok, const u8 *buffer, u32 nByteToSend, const OSNetAddr &addrTo)
{
	int ret = sendto (sok.socketID, (const char*)buffer, nByteToSend, 0, gos::netaddr::getSockAddr(addrTo), gos::netaddr::getSockAddrLen(addrTo));
	if (SOCKET_ERROR == ret)
		return 0;
	return (u32)ret;
}

//*************************************************** 
u32 platform::socket_UDPReceiveFrom (OSSocket &sok, u8 *buffer, u32 nMaxBytesToRead, OSNetAddr *out_addrFrom)
{
	int	addrLen = gos::netaddr::getSockAddrLen(*out_addrFrom);
	int ret = recvfrom(sok.socketID, (char*)buffer, nMaxBytesToRead, 0, gos::netaddr::getSockAddr(*out_addrFrom), &addrLen);
	if (SOCKET_ERROR == ret)
		return 0;
	return (u32)ret;
}



#endif // GOS_PLATFORM__WINDOWS
