#ifdef GOS_PLATFORM__LINUX
#include "linuxOS.h"
#include <errno.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "../../gos.h"


//*************************************************
void platform::socket_init (OSSocket *sok)
{
    assert (sok != NULL);
    sok->socketID = -1;
    sok->readTimeoutMSec = 10000;
}

//*************************************************
void platform::socket_close (OSSocket &sok)
{
    if (platform::socket_isOpen(sok))
    {
        ::close(sok.socketID);
        sok.socketID = -1;
    }
}

//*************************************************
eSocketError socket_openAsTCP (platform::OSSocket *sok)
{
    platform::socket_init(sok);

    //creo la socket
    sok->socketID = ::socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sok->socketID == -1)
    {
        switch (errno)
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
    int enable = 1;
    setsockopt (sok->socketID, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    setsockopt (sok->socketID, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int));

    int value = 64*1024;
    setsockopt (sok->socketID, SOL_SOCKET, SO_RCVBUF, &value, sizeof(int));
    setsockopt (sok->socketID, SOL_SOCKET, SO_SNDBUF, &value, sizeof(int));

    //setsockopt (sok->socketID, IPPROTO_TCP, TCP_NODELAY, &value, sizeof(int));


    return eSocketError::none;
}

//*************************************************
eSocketError platform::socket_openAsTCPClient (OSSocket *sok, const char *connectToIP, u32 portNumber)
{
    eSocketError sokErr = socket_openAsTCP (sok);
    if (sokErr != eSocketError::none)
        return sokErr;


    struct hostent *server = ::gethostbyname(connectToIP);
    if (server == NULL)
    {
        ::close(sok->socketID);
        sok->socketID = -1;
        return eSocketError::no_such_host;
    }

    /*struct sockaddr_in serv_addr;
    memset (&serv_addr, 0x00, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy (server->h_addr, &serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portNumber);
*/
    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(connectToIP);
    serv_addr.sin_port = htons(portNumber);

    if (0 != connect (sok->socketID, (struct sockaddr *)&serv_addr, sizeof(serv_addr)))
    {
        ::close(sok->socketID);
        sok->socketID = -1;

        switch (errno)
        {
        case EACCES:
        case EPERM:         return eSocketError::addressProtected;
        case EADDRINUSE:    return eSocketError::addressInUse;
        case EINVAL:        return eSocketError::alreadyBound;
        case ENOTSOCK:      return eSocketError::invalidDescriptor;
        case ENOMEM:        return eSocketError::noMem;
        case ECONNREFUSED:  return eSocketError::connRefused;
        case ETIMEDOUT:     return eSocketError::timedOut;
        default:            return eSocketError::unknown;
        }
    }

    return eSocketError::none;
}

//*************************************************
eSocketError platform::socket_openAsTCPServer (OSSocket *sok, int portNumber)
{
    eSocketError sokErr = socket_openAsTCP (sok);
    if (sokErr != eSocketError::none)
        return sokErr;


    //la bindo
    struct sockaddr_in socket_config;
    socket_config.sin_family = AF_INET;
    socket_config.sin_addr.s_addr = INADDR_ANY;
    socket_config.sin_port = htons(portNumber);

    if (0 != bind (sok->socketID, (struct sockaddr *)&socket_config, sizeof(socket_config)))
    {
        ::close(sok->socketID);
        sok->socketID = -1;
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

    u32 timeoutMSec = timeoutMSecIN;
    if (timeoutMSec == u32MAX)
        timeoutMSec = 0;    //socket sempre bloccante
    else if (timeoutMSec == 0)
        timeoutMSec = 1;    //socket con il minimo possibile tempo di wait


    timeval recv_timeout_ms;
    recv_timeout_ms.tv_sec = 0;
    recv_timeout_ms.tv_usec = timeoutMSec * 1000;
    while (recv_timeout_ms.tv_usec >= 1000000)
    {
        recv_timeout_ms.tv_usec -= 1000000;
        recv_timeout_ms.tv_sec++;
    }

    if (0 == setsockopt (sok.socketID, SOL_SOCKET, SO_RCVTIMEO, (void*)&recv_timeout_ms, sizeof(struct timeval)))
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
bool platform::socket_setWriteTimeoutMSec (OSSocket &sok, u32 timeoutMSec)
{
    if (!platform::socket_isOpen(sok))
        return false;


    if (timeoutMSec == u32MAX)
        timeoutMSec = 0;    //socket sempre bloccante
    else if (timeoutMSec == 0)
        timeoutMSec = 1;    //socket con il minimo possibile tempo di wait

    timeval send_timeout_ms;
    send_timeout_ms.tv_sec = 0;
    send_timeout_ms.tv_usec = timeoutMSec * 1000;
    while (send_timeout_ms.tv_usec >= 1000000)
    {
        send_timeout_ms.tv_usec -= 1000000;
        send_timeout_ms.tv_sec++;
    }

    if (0 == setsockopt (sok.socketID, SOL_SOCKET, SO_SNDTIMEO, (void*)&send_timeout_ms, sizeof(struct timeval)))
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

    int clientSocketID = ::accept(sok.socketID, NULL, NULL);
    if (clientSocketID == -1)
        return false;

    out_clientSocket->socketID = clientSocketID;
    platform::socket_setReadTimeoutMSec (*out_clientSocket, out_clientSocket->readTimeoutMSec);
    return true;
}


//*************************************************
i32 platform::socket_read (OSSocket &sok, void *buffer, u16 bufferSizeInBytes, u32 timeoutMSec, bool bPeekMSG)
{
	u32 readFLAG = 0;
	if (bPeekMSG)
		readFLAG = MSG_PEEK;

    if (timeoutMSec != sok.readTimeoutMSec)
        platform::socket_setReadTimeoutMSec(sok, timeoutMSec);

    i32 ret = ::recv(sok.socketID, buffer, bufferSizeInBytes, readFLAG);
    int myerrno = errno;
    if (ret == 0)
        return 0;
    if (ret > 0)
        return ret;


    if (myerrno==EAGAIN || myerrno==EWOULDBLOCK || myerrno==EINTR)
        return -1;  //timeout


    /*printf ("errno:%d\n", myerrno);
    switch (errno)
    {
    case ENOTCONN: return 0;

    case ENOTSOCK: return 0;
    }
*/
    return 0;
}

//*************************************************
i32  platform::socket_write(OSSocket &sok, const void *buffer, u16 nByteToSend)
{
    i32 ret = ::send(sok.socketID, buffer, nByteToSend, 0);
    if (ret <= 0)
        return 0;
    return ret;
}


//*************************************************
eSocketError platform::socket_openAsUDP(OSSocket *sok)
{
    platform::socket_init(sok);

    //creo la socket
    sok->socketID = socket(AF_INET, SOCK_DGRAM, 0);
    if (sok->socketID == -1)
    {
        switch (errno)
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
    int enable = 1;
    setsockopt (sok->socketID, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    setsockopt (sok->socketID, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int));
    //setsockopt (sok->socketID, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(int));

    return eSocketError::none;
}

//*************************************************
eSocketError platform::socket_UDPbind (OSSocket &sok, int portNumber)
{
    sockaddr_in		saAddress;
    memset(&saAddress, 0, sizeof(saAddress));
    saAddress.sin_family = AF_INET;
    saAddress.sin_port = htons(static_cast<unsigned short>(portNumber));
    saAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    if (-1 == bind(sok.socketID, reinterpret_cast<const struct sockaddr*>(&saAddress), sizeof(saAddress)))
    {
        ::close(sok.socketID);
        sok.socketID = -1;
        switch (errno)
        {
        case EACCES:        return eSocketError::addressProtected;
        case EADDRINUSE:    return eSocketError::addressInUse;
        case EINVAL:        return eSocketError::alreadyBound;
        case ENOTSOCK:      return eSocketError::invalidDescriptor;
        case ENOMEM:        return eSocketError::noMem;

        case EBADF: printf ("socket_UDPbind error EBADF\n"); return eSocketError::unknown;
        case EADDRNOTAVAIL: printf ("socket_UDPbind error EBADF\n"); return eSocketError::unknown;
        case EFAULT: printf ("socket_UDPbind error EBADF\n"); return eSocketError::unknown;
        case ELOOP: printf ("socket_UDPbind error EBADF\n"); return eSocketError::unknown;
        case ENAMETOOLONG: printf ("socket_UDPbind error EBADF\n"); return eSocketError::unknown;
        case ENOENT: printf ("socket_UDPbind error EBADF\n"); return eSocketError::unknown;
        case ENOTDIR: printf ("socket_UDPbind error EBADF\n"); return eSocketError::unknown;
        case EROFS: printf ("socket_UDPbind error EBADF\n"); return eSocketError::unknown;
        default:
            printf ("socket_UDPbind error [%d]\n", errno);
            return eSocketError::unknown;
        }
    }

    return eSocketError::none;
}

//***************************************************
u32 platform::socket_UDPSendTo (OSSocket &sok, const u8 *buffer, u32 nByteToSend, const gos::NetAddr &addrTo)
{
    int ret = sendto (sok.socketID, reinterpret_cast<const char*>(buffer), nByteToSend, 0, gos::netaddr::getSockAddr(addrTo), gos::netaddr::getSockAddrLen(addrTo));
    if (-1 == ret)
        return 0;
    return static_cast<u32>(ret);
}

//***************************************************
u32 platform::socket_UDPReceiveFrom (OSSocket &sok, u8 *buffer, u32 nMaxBytesToRead, gos::NetAddr *out_addrFrom)
{
    int	addrLen = gos::netaddr::getSockAddrLen(*out_addrFrom);
    int ret = recvfrom(sok.socketID, buffer, nMaxBytesToRead, 0, gos::netaddr::getSockAddr(*out_addrFrom), (socklen_t*)&addrLen);
    if (-1 == ret)
        return 0;
    return static_cast<u32>(ret);
}

#endif //GOS_PLATFORM__LINUX

