#include "gos.h"
#include "logger/gosLogger.h"
#include "logger/gosLoggerNull.h"
#include "logger/gosLoggerStdout.h"


struct sGOSGlobals
{
	u64					timeStarted_usec;
	gos::Logger			*logger;
	char				*appName;
};


static sGOSGlobals		gosGlobals;
static gos::Random		gosGlobalsRnd;


//******************************************
bool gos::init (const gos::sGOSInit &init, const char *appName)
{
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

	//logger
	switch (init._logMode)
	{
	default:
	case gos::sGOSInit::eLogMode::none:
		gosGlobals.logger = new gos::LoggerNull();
		break;

	case gos::sGOSInit::eLogMode::console:
		gosGlobals.logger = new gos::LoggerStdout();
		break;
	}
	gos::logger::log (eTextColor::white, "%s is starting...\n", appName);

	//memory	
	if (!mem::priv_init(init))
		return false;

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


	return true;
}

//******************************************
void gos::deinit()
{
	if (NULL != gosGlobals.logger)
	{
		gos::logger::log (eTextColor::white, "shutting down...\n");
		delete gosGlobals.logger;
	}

	GOSFREE(gos::getSysHeapAllocator(), gosGlobals.appName);

	console::priv_deinit();
	mem::priv_deinit();
	platform::internal_deinit();
}

//******************************************
const char* gos::getAppName()					{ return gosGlobals.appName; }
u64 gos::getTimeSinceStart_msec()				{ return (platform::getTimeNow_usec() - gosGlobals.timeStarted_usec) / 1000; }
u64 gos::getTimeSinceStart_usec()				{ return platform::getTimeNow_usec() - gosGlobals.timeStarted_usec; }
f32 gos::random01()								{ return gosGlobalsRnd.get01(); }
u32 gos::randomU32(u32 iMax)					{ return gosGlobalsRnd.getU32(iMax); }

//******************************************
void gos::logger::incIndent()																			{ gosGlobals.logger->incIndent(); }
void gos::logger::decIndent()																			{ gosGlobals.logger->decIndent(); }
void gos::logger::log (const char *format, ...)															{ va_list argptr; va_start (argptr, format); gosGlobals.logger->vlog (format, argptr); va_end (argptr); }
void gos::logger::log (const eTextColor col, const char *format, ...)                                   { va_list argptr; va_start (argptr, format); gosGlobals.logger->vlog (col, format, argptr); va_end (argptr); }
void gos::logger::logWithPrefix (const char *prefix, const char *format, ...)                           { va_list argptr; va_start (argptr, format); gosGlobals.logger->vlogWithPrefix (prefix, format, argptr); va_end (argptr); }
void gos::logger::logWithPrefix (const eTextColor col, const char *prefix, const char *format, ...)     { va_list argptr; va_start (argptr, format); gosGlobals.logger->vlogWithPrefix (col, prefix, format, argptr); va_end (argptr); }
void gos::logger::err (const char *format, ...)															{ va_list argptr; va_start (argptr, format); gosGlobals.logger->vlogWithPrefix (eTextColor::red, "ERROR=>", format, argptr); va_end (argptr); }
