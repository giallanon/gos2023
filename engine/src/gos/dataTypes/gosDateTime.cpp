#include <time.h>
#include "gosDateTime.h"
#include <string.h>

using namespace gos;


#ifdef _DEBUG
//*****************************************************
void DateTime::debug_test_me()
{
	gos::Date::debug_test_me();

	//test relativi alla "diff" tra date
#define TEST_ONE_DAY	(i64)86400
#define TEST_ONE_HOUR	(i64)3600
#define TEST_ONE_MINUTE	(i64)60

#define TEST_DIFF1(yearA,monthA,dayA,hourA,minuteA,secondsA,  yearB,monthB,dayB,hourB,minuteB,secondsB,  expectedDiffInSeconds)\
	{\
		gos::DateTime dtA (yearA, monthA, dayA, hourA, minuteA, secondsA);\
		gos::DateTime dtB (yearB, monthB, dayB, hourB, minuteB, secondsB);\
		const i64 diff_sec = gos::DateTime::diff_seconds(dtA, dtB);\
		assert (diff_sec == (expectedDiffInSeconds));\
	}\

	TEST_DIFF1(2022,6,7,18,32,0,	2022,5,7,14,9,0,   31*TEST_ONE_DAY +4*TEST_ONE_HOUR +23*TEST_ONE_MINUTE);	//31 days, 4 hours, 23 minutes and 0 seconds
	TEST_DIFF1(1971,1,1,0,0,0,		1971,1,1,0,0,0, 0);
	TEST_DIFF1(1971,1,1,0,1,0,		1971,1,1,0,0,0, TEST_ONE_MINUTE);
	TEST_DIFF1(1971,1,1,1,0,0,		1971,1,1,0,0,0, TEST_ONE_HOUR);
	TEST_DIFF1(1971,1,2,0,0,0,		1971,1,1,0,0,0, TEST_ONE_DAY);
	TEST_DIFF1(1971,1,1,0,0,1,		1971,1,1,0,0,0, 1);
	TEST_DIFF1(2001,3,1,6,5,0,		2001,2,28,6,5,0,	   1*TEST_ONE_DAY);	//1 day
	TEST_DIFF1(2000,3,1,6,5,0,		2000,2,28,6,5,0,	   2*TEST_ONE_DAY);	//2 day (bisestile)

#undef TEST_DIFF1
#undef TEST_ONE_DAY
#undef TEST_ONE_HOUR
#undef TEST_ONE_MINUTE
	
}
#endif




//*****************************************************
void DateTime::formatAs_YYYYMMDDHHMMSS(char *out, u32 sizeOfOut, char char_between_date_and_time, char date_sep, char time_sep) const
{
	/*
	0123456789 12345678
	YYYYaMMaDDbHHcMMcSS
	*/

	out[0] = 0x00;
	date.formatAs_YYYYMMDD(out, sizeOfOut, date_sep);
	
	u32 n = (u32)strlen(out);
	if (n == 0)
		return;

	if (sizeOfOut > n)
	{
		u32 nLeft = sizeOfOut - n;
		if (char_between_date_and_time != 0x00)
		{
			out[n++] = char_between_date_and_time;
			nLeft--;
		}
		out[n] = 0;

		time.formatAs_HHMMSS(&out[n], nLeft, time_sep);
	}
}

//****************************
void DateTime::addMSec(u64 msec)
{
	u32	h = (u32)(msec / 3600000);
	msec -= h * 3600000;

	u32 m = (u32)(msec / 60000);
	msec -= m * 60000;

	u32 s = (u32)(msec / 1000);
	msec -= s * 1000;

	addYMDHIS(0, 0, 0, (i16)h, (i16)m, (i16)s);
}

//****************************
void DateTime::addYMDHIS(i16 years, i16 months, i16 days, i16 hours, i16 minutes, i16 seconds)
{
	struct tm t;
	t.tm_year = (int)date.getYear() - 1900;
	t.tm_mon = (int)date.getMonth() - 1;
	t.tm_mday = (int)date.getDay();
	t.tm_hour = (int)time.getHour();
	t.tm_min = (int)time.getMin();
	t.tm_sec = (int)time.getSec();
	t.tm_isdst = -1;

	t.tm_mday += days;
	t.tm_mon += months;
	t.tm_year += years;
	t.tm_hour += hours;
	t.tm_min += minutes;
	t.tm_sec += seconds;

	mktime(&t);

	date.setYMD(1900 + (u32)t.tm_year, 1 + (u32)t.tm_mon, (u32)t.tm_mday);
	time.setHMS((u32)t.tm_hour, (u32)t.tm_min, (u32)t.tm_sec);
}

//****************************
bool DateTime::operator< (const DateTime& b) const
{
	if (date > b.date)
		return false;
	if (date == b.date)
		return (time < b.time);
	return true; //perch� date < b.date
}

//****************************
bool DateTime::operator> (const DateTime& b) const
{
	if (date < b.date)
		return false;
	if (date == b.date)
		return (time > b.time);
	return true; //perch� date > b.date
}

//****************************
bool DateTime::operator<= (const DateTime& b) const
{
	if (date > b.date)
		return false;
	if (date == b.date)
		return (time <= b.time);
	return true; //perch� date < b.date
}

//****************************
bool DateTime::operator>= (const DateTime& b) const
{
	if (date < b.date)
		return false;
	if (date == b.date)
		return (time >= b.time);
	return true; //perch� date > b.date
}

//****************************
void DateTime::calcEuropeanSummerTimeStartAndEndDateTime (u32 year, DateTime *out_startDate, DateTime *out_endDate)
{
	/* l'ora legale (aka summertime, aka DST) in Europa inizia:
		DTS +1 on  last Sunday in March alle 02:00
	e termina:
		DST -1 on  last Sunday in October alle 03:00

	Quindi, alle 02:00 dell'ultima domenica di marzo, si porta avanti l'ora di 1 mentre
	l'ultima domenica di ottobre, alle 03:00 si porta indietro

		Start				End
		25 March 2012		28 October 2012
		31 March 2013		27 October 2013
		30 March 2014		26 October 2014
		29 March 2015		25 October 2015
		27 March 2016		30 October 2016
		26 March 2017		29 October 2017
		25 March 2018		28 October 2018
		31 March 2019		27 October 2019
		29 March 2020		25 October 2020
		28 March 2021		31 October 2021
		27 March 2022		30 October 2022
		26 March 2023		29 October 2023[note 2]
		31 March 2024 ?		27 October 2024
		30 March 2025 ?		26 October 2025
		29 March 2026 ?		25 October 2026
		28 March 2027 ?		31 October 2027
	*/

	//dato l'anno [year], devo trovare l'ultima domenica di marzo
	Date dt;
	u32 day = 31;
	while (1)
	{
		dt.setYMD (year, 3, day);
		if (dt.getDayOfWeek() == eDayOfWeek::sunday)
		{
			out_startDate->date = dt;
			out_startDate->time.setHMS (2,0,0);
			break;
		}

		day--;
	}

	day = 31;
	while (1)
	{
		dt.setYMD (year, 10, day);
		if (dt.getDayOfWeek() == eDayOfWeek::sunday)
		{
			out_endDate->date = dt;
			out_endDate->time.setHMS (3,0,0);
			break;
		}

		day--;
	}
}

//****************************
i64 DateTime::diff_seconds (const DateTime &a, const DateTime &b)
{
	struct tm timeinfoA;
	memset (&timeinfoA, 0, sizeof(timeinfoA));
	timeinfoA.tm_year = a.date.getYear() - 1900;
	timeinfoA.tm_mon = a.date.getMonth() -1;
	timeinfoA.tm_mday = a.date.getDay();
	timeinfoA.tm_hour = a.time.getHour();
	timeinfoA.tm_min = a.time.getMin();
	timeinfoA.tm_sec = a.time.getSec();
	timeinfoA.tm_isdst = 0;
	time_t rawtimeA = mktime(&timeinfoA);


	struct tm timeinfoB;
	memset (&timeinfoB, 0, sizeof(timeinfoB));
	timeinfoB.tm_year = b.date.getYear() - 1900;
	timeinfoB.tm_mon = b.date.getMonth() - 1;
	timeinfoB.tm_mday = b.date.getDay();
	timeinfoB.tm_hour = b.time.getHour();
	timeinfoB.tm_min = b.time.getMin();
	timeinfoB.tm_sec = b.time.getSec();
	timeinfoB.tm_isdst = 0;
	time_t rawtimeB = mktime(&timeinfoB);

	const double dbl_sec = difftime(rawtimeA, rawtimeB);
	const u64 sec = static_cast<i64>(dbl_sec);


	return sec;
}