#include "../gos.h"
#include "../gosString.h"
#include "gosTime24.h"


using namespace gos;


//******************************
void Time24::setNow_local ()
{
    u8 h,m,s;
	platform::getTimeNow_local(&h, &m, &s);
    setHMS (h, m, s, 0);
}

//******************************
void Time24::setNow_UTC ()
{
    u8 h,m,s;
	platform::getTimeNow_UTC(&h, &m, &s);
    setHMS (h, m, s, 0);
}


//****************************
void Time24::getTimeNow_local(u8 *out_hour, u8 *out_min, u8 *out_sec)
{
	platform::getTimeNow_local(out_hour, out_min, out_sec);
}

//****************************
void Time24::getTimeNow_UTC(u8 *out_hour, u8 *out_min, u8 *out_sec)
{
	platform::getTimeNow_UTC(out_hour, out_min, out_sec);
}

//****************************
void Time24::setFromHHMMSS (const char *hhmmss)
{
    if (NULL == hhmmss)
        return;
    if (string::utf8::lengthInByte(hhmmss) < 6)
        return;

    char s[4] = { 0,0,0,0 };
    
    s[0] = hhmmss[0];   s[1] = hhmmss[1];   setHour (string::utf8::toU32(s));
    s[0] = hhmmss[2];   s[1] = hhmmss[3];   setMin (string::utf8::toU32(s));
    s[0] = hhmmss[4];   s[1] = hhmmss[5];   setSec (string::utf8::toU32(s));
}


//****************************
u32 Time24::formatAs_HHMMSS() const
{
	char s[8];
	formatAs_HHMMSS(s, sizeof(s), 0x00);
	return gos::string::utf8::toU32(s);
}

//****************************
void Time24::formatAs_HHMMSS(char *out, u32 sizeof_out, char time_sep) const
{
	if (time_sep == 0x00)
	{
		assert(sizeof_out >= 7);
		if (sizeof_out >= 7)
			sprintf_s(out, sizeof_out, "%02d%02d%02d", getHour(), getMin(), getSec());
		else
			out[0] = 0x00;
	}
	else
	{
		assert(sizeof_out >= 9);
		if (sizeof_out >= 9)
			sprintf_s(out, sizeof_out, "%02d%c%02d%c%02d", getHour(), time_sep, getMin(), time_sep, getSec());
		else
			out[0] = 0x00;
	}
}

//******************************
void Time24::add (u32 h, u32 m, u32 s, u32 ms)
{
    const u64 mNow = calcTimeInMSec();
    const u64 toAdd = ms + 1000*s + 60000*m +3600000*h;
    setFromMSec (mNow + toAdd);
}

//******************************
void Time24::sub (u32 h, u32 m, u32 s, u32 ms)
{
    u64 mNow = calcTimeInMSec();
    const u64 mToSub = ms + 1000*s + 60000*m +3600000*h;

    if (mNow > mToSub)
        mNow -= mToSub;
    else
        mNow = 0;

    setFromMSec (mNow);
}

//******************************
void Time24::setFromMSec (u64 msec)
{
    u32	h = (u32)(msec / 3600000);
    msec -= h *3600000;

    u32 m = (u32)(msec / 60000);
    msec -= m*60000;

    u32 s = (u32)(msec / 1000);
    msec -= s*1000;

    setHMS (h, m, s, (u32)msec);
}

