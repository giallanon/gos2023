#include "gos.h"
#include "logger/gosLogger.h"
#include "logger/gosLoggerNull.h"
#include "logger/gosLoggerStdout.h"
#include "gosUtils.h"
#include "gosThreadMsgQ.h"

struct sGOSGlobals
{
	u64					timeStarted_usec;
	gos::Logger			*logger;
	char				*appName;
	u8					*pathToWritableFolder;
};

struct ThreadInfo
{
    platform::OSThread      	osThreadHandle;
    GOS_ThreadMainFunction     	threadMainFn;
    void                    	*userParam;
};

static sGOSGlobals		gosGlobals;
static gos::Random		gosGlobalsRnd;


//******************************************
bool gos::init (const gos::sGOSInit &init, const char *appName)
{
	u8 s[1024];

	//assert sulla dimensione dei datatype
	assert (sizeof(i8) == 1);
	assert (sizeof(i16) == 2);
	assert (sizeof(i32) == 4);
	assert (sizeof(i64) == 8);

	assert (sizeof(u8) == 1);
	assert (sizeof(u16) == 2);
	assert (sizeof(u32) == 4);
	assert (sizeof(u64) == 8);

	assert (sizeof(f32) == 4);
	assert (sizeof(f64) == 8);

	memset (&gosGlobals, 0, sizeof(gosGlobals));

	if (!platform::internal_init(appName))
		return false;

	gosGlobals.timeStarted_usec = platform::getTimeNow_usec();

	//console stuff
	if (!console::priv_init())
		return false;

	//memory	
	if (!mem::priv_init(init))
		return false;


	//determino il path alla writable folder
	switch (init._writableFolder.mode)
	{
	case gos::sGOSInit::eWritableFolder::inTheAppFolder:
		if (0x00 == init._writableFolder.suffix[0] )
			gos::string::utf8::spf (s, sizeof(s), "%s/writable", platform::getAppPathNoSlash());
		else
			gos::string::utf8::spf (s, sizeof(s), "%s/writable%s", platform::getAppPathNoSlash(), init._writableFolder.suffix);
		break;

	case gos::sGOSInit::eWritableFolder::inUserFolder:
		if (0x00 == init._writableFolder.suffix[0] )
			gos::string::utf8::spf (s, sizeof(s), "%s/%s/writable", platform::getPhysicalPathToUserFolder(), appName);
		else
			gos::string::utf8::spf (s, sizeof(s), "%s/%s/writable%s", platform::getPhysicalPathToUserFolder(), appName, init._writableFolder.suffix);
		break;
	}
	gosGlobals.pathToWritableFolder = gos::string::utf8::allocStr (gos::getSysHeapAllocator(), s);
	gos::fs::folderCreate (gosGlobals.pathToWritableFolder);

	//logger
	switch (init._logMode)
	{
	default:
	case gos::sGOSInit::eLogMode::none:
		gosGlobals.logger = new gos::LoggerNull();
		break;

	case gos::sGOSInit::eLogMode::only_console:
		gosGlobals.logger = new gos::LoggerStdout();
		break;

	case gos::sGOSInit::eLogMode::only_file:
		{
			LoggerStdout *logger = new gos::LoggerStdout();
			logger->disableStdouLogging();
			string::utf8::spf (s, sizeof(s), "%s/log", gos::getPhysicalPathToWritableFolder());
			logger->enableFileLogging (s);
			gosGlobals.logger = logger;
		}
		break;

	case gos::sGOSInit::eLogMode::both_console_and_file:
		{
			LoggerStdout *logger = new gos::LoggerStdout();
			string::utf8::spf (s, sizeof(s), "%s/log", gos::getPhysicalPathToWritableFolder());
			logger->enableFileLogging (s);
			gosGlobals.logger = logger;
		}
		break;
	}

	gos::logger::log (eTextColor::white, "%s is starting...\n", appName);

	gosGlobals.appName = reinterpret_cast<char*>(gos::string::utf8::allocStr (gos::getSysHeapAllocator(), appName));

	//generatore random
	{
		u16 y, m, d;
		u8 hh, mm, ss;
		gos::Date::getDateNow(&y, &m, &d);
		gos::Time24::getTimeNow(&hh, &mm, &ss);
        const u32 s = (y * 365 + m * 31 + d) * 24 * 3600 + hh * 3600 + mm * 60 + ss +static_cast<u32>(gosGlobals.timeStarted_usec);
		gosGlobalsRnd.seed(s);
	}

	if (!gos::thread::internal_init())
		return false;

	return true;
}

//******************************************
void gos::deinit()
{

	if (NULL != gosGlobals.logger)
	{
		gos::logger::log (eTextColor::white, "shutting down...\n\n\n\n");
		delete gosGlobals.logger;
	}

	thread::internal_deinit();
	GOSFREE(gos::getSysHeapAllocator(), gosGlobals.appName);
	GOSFREE(gos::getSysHeapAllocator(), gosGlobals.pathToWritableFolder);

	console::priv_deinit();
	mem::priv_deinit();
	platform::internal_deinit();
}

//******************************************
const char* gos::getAppName()						{ return gosGlobals.appName; }
const u8* gos::getPhysicalPathToWritableFolder()	{ return gosGlobals.pathToWritableFolder; }
u64 gos::getTimeSinceStart_msec()					{ return (platform::getTimeNow_usec() - gosGlobals.timeStarted_usec) / 1000; }
u64 gos::getTimeSinceStart_usec()					{ return platform::getTimeNow_usec() - gosGlobals.timeStarted_usec; }
f32 gos::random01()									{ return gosGlobalsRnd.get01(); }
u32 gos::randomU32(u32 iMax)						{ return gosGlobalsRnd.getU32(iMax); }

//******************************************
gos::Logger* gos::logger::getSystemLogger()																{ return gosGlobals.logger; }
void gos::logger::incIndent()																			{ gosGlobals.logger->incIndent(); }
void gos::logger::decIndent()																			{ gosGlobals.logger->decIndent(); }
void gos::logger::log (const char *format, ...)															{ va_list argptr; va_start (argptr, format); gosGlobals.logger->vlog (format, argptr); va_end (argptr); }
void gos::logger::log (const eTextColor col, const char *format, ...)                                   { va_list argptr; va_start (argptr, format); gosGlobals.logger->vlog (col, format, argptr); va_end (argptr); }
void gos::logger::logWithPrefix (const char *prefix, const char *format, ...)                           { va_list argptr; va_start (argptr, format); gosGlobals.logger->vlogWithPrefix (prefix, format, argptr); va_end (argptr); }
void gos::logger::logWithPrefix (const eTextColor col, const char *prefix, const char *format, ...)     { va_list argptr; va_start (argptr, format); gosGlobals.logger->vlogWithPrefix (col, prefix, format, argptr); va_end (argptr); }
void gos::logger::err (const char *format, ...)															{ va_list argptr; va_start (argptr, format); gosGlobals.logger->vlogWithPrefix (eTextColor::red, "ERROR=>", format, argptr); va_end (argptr); }

//***************************************************
bool gos::netaddr::getMACAddressAsString (const gos::MacAddress &mac, char *out_macAddress, u32 sizeOfMacAddress, char optionalSeparator)
{
	assert (NULL != out_macAddress);
	if (0x00 == optionalSeparator)
	{
		if (sizeOfMacAddress < 13)
		{
			out_macAddress[0] = 0;
			return false;
		}
		sprintf_s (out_macAddress, sizeOfMacAddress, "%02X%02X%02X%02X%02X%02X", mac.b[0], mac.b[1], mac.b[2], mac.b[3], mac.b[4], mac.b[5]);
	}
	else
	{
		if (sizeOfMacAddress < 18)
		{
			out_macAddress[0] = 0;
			return false;
		}
		sprintf_s (out_macAddress, sizeOfMacAddress, "%02X%c%02X%c%02X%c%02X%c%02X%c%02X", mac.b[0], optionalSeparator, 
			mac.b[1], optionalSeparator, 
			mac.b[2], optionalSeparator, 
			mac.b[3], optionalSeparator, 
			mac.b[4], optionalSeparator
			, mac.b[5]);
	}
	return true;
}

//***************************************************
i8 gos::netaddr::compare (const gos::MacAddress &m1, const gos::MacAddress &m2)
{
	for (u8 i = 0; i < 6; i++)
	{
		if (m1.b[i] < m2.b[i])
			return -1;
		if (m1.b[i] > m2.b[i])
			return 1;
	}
	return 0;
}

//***************************************************
void gos::netaddr::ipstrToIPv4  (const char *ip, gos::IPv4 *out)
{
	assert (NULL != out);
	memset (out->ips, 0, 4);

	gos::string::utf8::Iter iter;
    iter.setup (reinterpret_cast<const u8 *>(ip));

	const UTF8Char chPunto(".");
	i32 n = 0;
	if (string::utf8::extractI32 (iter, &n, &chPunto, 1))
	{
        out->ips[0] = static_cast<u8>(n);
		iter.advanceOneChar();
		if (string::utf8::extractI32 (iter, &n, &chPunto, 1))
		{
            out->ips[1] = static_cast<u8>(n);
			iter.advanceOneChar();
			if (string::utf8::extractI32 (iter, &n, &chPunto, 1))
			{
                out->ips[2] = static_cast<u8>(n);
				iter.advanceOneChar();
				if (string::utf8::extractI32 (iter, &n, &chPunto, 1))
                    out->ips[3] = static_cast<u8>(n);
			}
		}
	}
}

//***************************************************
void gos::netaddr::setInvalid (gos::MacAddress &me)															{ memset(me.b, 0, 6); }
bool gos::netaddr::isInvalid (const gos::MacAddress &me)														{ return (me.b[0]==0 && me.b[1]==0 && me.b[2]==0 && me.b[3]==0 && me.b[4]==0 && me.b[5]==0); }
bool gos::netaddr::isValid (const gos::MacAddress &me)														{ return !netaddr::isInvalid(me); }
bool gos::netaddr::setFromMACString (gos::MacAddress &me, const u8 *macString, bool bStringHasSeparator)		{ return netaddr::setFromMACString (me, reinterpret_cast<const char*>(macString), bStringHasSeparator); }
bool gos::netaddr::setFromMACString (gos::MacAddress &me, const char *macString, bool bStringHasSeparator)
{
	if (NULL == macString)
	{
		DBGBREAK;
		netaddr::setInvalid(me);
		return false;
	}

	const size_t len = strlen(macString);
	u8 step = 0;
	if (bStringHasSeparator)
	{
		if (len >= 17)
			step = 3;
	}
	else
	{
		if (len >= 12)
			step = 2;
	}

	if (0 == step)
	{
		//verifica [len], non e' valida!
		DBGBREAK;
		netaddr::setInvalid(me);
		return false;
	}

	char hex[3];
	hex[2] = 0x00;

	u32 ct = 0;
	for (u8 i = 0; i < 6; i++)
	{
		hex[0] = macString[ct];
		hex[1] = macString[ct + 1];
		u32 num;
		if (!gos::string::ansi::hexToInt (hex, &num, 2))
		{
			DBGBREAK;
			netaddr::setInvalid(me);
			return false;
		}
		me.b[i] = static_cast<u8>(num);
		ct += step;
	}
	return true;
}
bool gos::MacAddress::operator== (const gos::MacAddress &z) const											{ return gos::netaddr::compare (*this, z) == 0; }
bool gos::MacAddress::operator!= (const gos::MacAddress &z) const											{ return gos::netaddr::compare (*this, z) != 0; }

//***************************************************
void gos::netaddr::setInvalid (gos::NetAddr &me)											{ setPort(me, 0); }
bool gos::netaddr::isInvalid (const gos::NetAddr &me)										{ return (getPort(me) == 0); }
bool gos::netaddr::isValid (const gos::NetAddr &me)											{ return !netaddr::isInvalid(me); }
void gos::netaddr::setFromSockAddr(gos::NetAddr &me, const sockaddr_in &addrIN)				{ memcpy(&me.addr, &addrIN, sizeof(sockaddr_in)); }
void gos::netaddr::setFromAddr(gos::NetAddr &me, const gos::NetAddr &addrIN)				{ memcpy(&me.addr, &addrIN.addr, sizeof(sockaddr_in)); }
void gos::netaddr::setIPv4(gos::NetAddr &me, const char *ip)								{ me.addr.sin_family = AF_INET; inet_pton (AF_INET, ip, &me.addr.sin_addr.s_addr); }
void gos::netaddr::setPort(gos::NetAddr &me, u16 port)										{ me.addr.sin_family = AF_INET; me.addr.sin_port = htons(static_cast<unsigned short>(port)); }
u16 gos::netaddr::getPort (const gos::NetAddr &me)											{ return static_cast<u16>(ntohs(me.addr.sin_port)); }
sockaddr* gos::netaddr::getSockAddr(const gos::NetAddr &me)									{ return (sockaddr*)(&me.addr); }
int gos::netaddr::getSockAddrLen(const gos::NetAddr &me)									{ return sizeof(me.addr); }

u8 gos::netaddr::serializeToBuffer (const gos::NetAddr &me, u8 *dst, u32 sizeof_dst, bool bIncludePort)
{
	if ( (sizeof_dst < 4) || (bIncludePort && sizeof_dst < 6))
	{
		DBGBREAK;
		return 0;
	}

	gos::IPv4 ipv4;
	netaddr::getIPv4 (me, &ipv4);

	ipv4.serializeToBuffer (dst, sizeof_dst);
	if (!bIncludePort)
		return 4;
	gos::utils::bufferWriteU16 (&dst[4], netaddr::getPort(me));
	return 6;
}

u8 gos::netaddr::deserializeFromBuffer (gos::NetAddr &me, const u8 *src, u32 sizeof_src, bool bIncludePort)
{
	if ( (sizeof_src < 4) || (bIncludePort && sizeof_src < 6))
	{
		DBGBREAK;
		return 0;
	}

	gos::IPv4 ipv4;
	ipv4.deserializeFromBuffer (src, sizeof_src);
	netaddr::setIPv4 (me, ipv4);

	if (!bIncludePort)
		return 4;
	netaddr::setPort (me, gos::utils::bufferReadU16 (&src[4]));
	return 6;
}

void gos::netaddr::setIPv4 (gos::NetAddr &me, const gos::IPv4 &ip)
{
	char s[32];
	sprintf_s (s, sizeof(s), "%d.%d.%d.%d", ip.ips[0], ip.ips[1], ip.ips[2], ip.ips[3]);
	setIPv4 (me, s);
}

void gos::netaddr::getIPv4 (const gos::NetAddr &me, gos::IPv4 *out)							{ assert (NULL != out); char s[32]; getIPv4 (me, s, sizeof(s)); ipstrToIPv4 (s, out); }

void gos::netaddr::getIPv4 (const gos::NetAddr &me, char *out, u32 sizeof_out)
{
	out[0] = 0x00;
	//const char *ip = inet_ntoa(me.addr.sin_addr);

	char ip[32];
	memset (ip, 0, sizeof(ip));
	inet_ntop (AF_INET, &me.addr.sin_addr, ip, sizeof(ip));
	if (ip[0] != 0x00)
		sprintf_s (out, sizeof_out, "%s", ip);
}

bool gos::netaddr::compare(const gos::NetAddr &a, const gos::NetAddr &b)
{
	if (gos::netaddr::getPort(a) != netaddr::getPort(b))
		return false;
	char ipA[32], ipB[32];
	netaddr::getIPv4(a, ipA, sizeof(ipA));
	netaddr::getIPv4(b, ipB, sizeof(ipB));
	if (strcasecmp(ipA, ipB) != 0)
		return false;
	return true;
}

//*************************************************** 
eSocketError gos::socket::openAsTCPClient (Socket *out_sok, const gos::NetAddr &ipAndPort)
{
	char ip[32];
	gos::netaddr::getIPv4 (ipAndPort, ip, sizeof(ip));
	return openAsTCPClient(out_sok, ip, gos::netaddr::getPort(ipAndPort));
}

//*************************************************** 
void gos::socket::UDPSendBroadcast(Socket &sok, const u8 *buffer, u32 nByteToSend, const char *ip, int portNumber, const char *subnetMask)
{
	// Abilita il broadcast
	int i = 1;
    setsockopt(sok.osSok.socketID, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<char*>(&i), sizeof(i));

	// Broadcasta il messaggio
	//const unsigned long	host_addr = inet_addr(ip);
	unsigned long	host_addr;
	inet_pton (AF_INET, ip, &host_addr);

	//const unsigned long	net_mask = inet_addr(subnetMask);
	unsigned long	net_mask;
	inet_pton (AF_INET, subnetMask, &net_mask);

	const unsigned long	net_addr = host_addr & net_mask;
	const unsigned long	dir_bcast_addr = net_addr | (~net_mask);

	sockaddr_in		saAddress;
	saAddress.sin_family = AF_INET;
    saAddress.sin_port = static_cast<unsigned short>(htons(static_cast<unsigned short>(portNumber)));
    saAddress.sin_addr.s_addr = static_cast<unsigned int>(dir_bcast_addr);
    sendto(sok.osSok.socketID, reinterpret_cast<const char*>(buffer), nByteToSend, 0, reinterpret_cast<sockaddr*>(&saAddress), sizeof(saAddress));


	// Disabilita il broadcast
	i = 0;
    setsockopt (sok.osSok.socketID, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<char*>(&i), sizeof(i));
}




//************************************************************************
void* GOS_threadFunctionWrapper (void *userParam)
{
    ThreadInfo *th = reinterpret_cast<ThreadInfo*>(userParam);
    GOS_ThreadMainFunction mainFn = th->threadMainFn;

    //int retCode = (*mainFn)(th->userParam);
    (*mainFn)(th->userParam);

	platform::destroyThread(th->osThreadHandle);
	
	gos::Allocator *allocator = gos::getSysHeapAllocator();
    GOSFREE(allocator, th);
    
    return NULL;
}

//************************************************************************
eThreadError gos::thread::create (gos::Thread *out_hThread, GOS_ThreadMainFunction threadFunction, void *userParam, u16 stackSizeInKb)
{
    out_hThread->p = NULL;

    Allocator *allocator = gos::getSysHeapAllocator();
    
    ThreadInfo *th = GOSALLOCSTRUCT(allocator, ThreadInfo);
    memset (th, 0x00, sizeof(ThreadInfo));
    th->threadMainFn = threadFunction;
    th->userParam = userParam;

    eThreadError err = platform::createThread (th->osThreadHandle, GOS_threadFunctionWrapper, stackSizeInKb, th);

    if (eThreadError::none == err)
        out_hThread->p = th;
    else
    {
        GOSFREE(allocator, th);
    }
    return err;
}


//************************************************************************
void gos::thread::waitEnd (const gos::Thread &hThread)
{
    ThreadInfo *th = reinterpret_cast<ThreadInfo*>(hThread.p);
    if (NULL == th)
        return;
    platform::waitThreadEnd(th->osThreadHandle);
}
