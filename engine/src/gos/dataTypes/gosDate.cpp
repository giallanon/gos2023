#include <time.h>
#include "gosDate.h"
#include "../gos.h"
#include "../gosString.h"

using namespace gos;

#ifdef _DEBUG
//****************************
void Date::debug_test_me()
{
	gos::Date dt;
	dt.setYMD(2019, 9, 17);	dt.addYMD(1, 0, 0); assert(dt.formatAs_YYYYMMDD() == 20200917);
	dt.setYMD(2019, 9, 17);	dt.addYMD(0, 1, 0); assert(dt.formatAs_YYYYMMDD() == 20191017);
	dt.setYMD(2019, 9, 17);	dt.addYMD(0, 0, 1); assert(dt.formatAs_YYYYMMDD() == 20190918);
	dt.setYMD(2019, 9, 17);	dt.addYMD(0, 0, 15); assert(dt.formatAs_YYYYMMDD() == 20191002);
}
#endif

//****************************
void Date::setNow ()
{
    u16	d,m,y;
	platform::getDateNow (&y, &m, &d);
    setYMD (y, m, d);
}

//****************************
void Date::getDateNow(u16 *out_year, u16 *out_month, u16 *out_day)
{
	platform::getDateNow(out_year, out_month, out_day);
}

//****************************
void Date::setFromYYYYMMDD (const char *yyyymmdd)
{
	setFromYYYYMMDD (reinterpret_cast<const u8*>(yyyymmdd));
}

//****************************
void Date::setFromYYYYMMDD (const u8 *yyyymmdd)
{
    if (NULL == yyyymmdd)
        return;
    if (string::utf8::lengthInBytes(yyyymmdd) < 8)
        return;

    u8 s[8] = { 0,0,0,0,0,0,0,0 };
    
    s[0] = yyyymmdd[4];   s[1] = yyyymmdd[5];   setMonth (string::utf8::toU32(s));
    s[0] = yyyymmdd[6];   s[1] = yyyymmdd[7];   setDay (string::utf8::toU32(s));
    s[0] = yyyymmdd[0];   s[1] = yyyymmdd[1];   s[2] = yyyymmdd[2];   s[3] = yyyymmdd[3]; setYear (string::utf8::toU32(s));
}

//****************************
u32 Date::formatAs_YYYYMMDD() const
{
	char s[16];
	formatAs_YYYYMMDD(s, sizeof(s), 0x00);
	return gos::string::utf8::toU32((const u8 *)s);
}

//****************************
void Date::formatAs_YYYYMMDD(char *out, u32 sizeofout, char date_sep) const
{
	if (date_sep == 0x00)
	{
		assert(sizeofout >= 9);
		if (sizeofout >= 9)
			sprintf_s(out, sizeofout, "%04d%02d%02d", getYear(), getMonth(), getDay());
		else
			out[0] = 0;
	}
	else
	{
		assert(sizeofout >= 11);
		if (sizeofout >= 11)
			sprintf_s(out, sizeofout, "%04d%c%02d%c%02d", getYear(), date_sep, getMonth(), date_sep, getDay());
		else
			out[0] = 0;
	}
}

//****************************
void Date::addYMD (u32 years, u32 months, u32 days)
{
	struct tm t;
	t.tm_year = (int)getYear() - 1900;
	t.tm_mon = (int)getMonth() - 1;
	t.tm_mday = (int)getDay();
	t.tm_hour= 1;
	t.tm_min = 1;
	t.tm_sec = 1;
	t.tm_isdst = -1;

	t.tm_mday += (int)days;
	t.tm_mon += (int)months;
	t.tm_year += (int)years;
	
	mktime (&t);

	setYMD(1900 + (u32)t.tm_year, 1 + (u32)t.tm_mon, (u32)t.tm_mday);
}

//*******************************************************************
eDayOfWeek Date::getDayOfWeek () const
{
	//vedi Michael Keith and Tom Craver algo from Wikipedia https://en.wikipedia.org/wiki/Determination_of_the_day_of_the_week#Implementation-dependent_methods
	i32	d = getDay();
	i32 m = getMonth();
	i32 y = getYear();
	switch ((d += m < 3 ? y-- : y - 2, 23 * m / 9 + d + 4 + y / 4 - y / 100 + y / 400) % 7)
	{
	default:
	case 0: return eDayOfWeek::sunday;
	case 1: return eDayOfWeek::monday;
	case 2: return eDayOfWeek::tuesday;
	case 3: return eDayOfWeek::wednesday;
	case 4: return eDayOfWeek::thursday;
	case 5: return eDayOfWeek::friday;
	case 6: return eDayOfWeek::saturday;
	}
}
