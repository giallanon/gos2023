#ifndef _gosDateTime_h_
#define _gosDateTime_h_
#include "gosDate.h"
#include "gosTime24.h"

namespace gos
{
    /**********************************************************
    *	DateTime
    **********************************************************/
    class DateTime
    {
	public:
#ifdef _DEBUG
		static void			debug_test_me();
#endif

	public:
		static DateTime		Now()																					{ DateTime t; t.setNow(); return t; }
		static void			getDateNow(u16 *out_year, u16 *out_month, u16 *out_day)									{ gos::Date::getDateNow(out_year, out_month, out_day); }
		static void			getTimeNow(u8 *out_hour, u8 *out_min, u8 *out_sec)										{ gos::Time24::getTimeNow(out_hour, out_min, out_sec); }
		static void			calcEuropeanSummerTimeStartAndEndDateTime (u32 year, DateTime *out_startDate, DateTime *out_endDate);
		static i64			diff_seconds (const DateTime &a, const DateTime &b);
								//ritorna (a - b) in secondi

    public:
							DateTime()																				{ }
							DateTime(u32 year, u32 month, u32 day)													{ date.setYMD(year, month, day); time.setHMS(0,0,0,0);}
							DateTime(u32 year, u32 month, u32 day, u32 h, u32 m, u32 s, u32 ms=0)					{ date.setYMD(year, month, day); time.setHMS(h,m,s,ms); }

		DateTime&			operator= (const DateTime& b)															{ date = b.date; time = b.time; return *this; }

        bool        		operator== (const DateTime& b) const													{ return (date==b.date && time==b.time); }
        bool        		operator!= (const DateTime& b) const													{ return (date!=b.date || time!=b.time); }
        bool        		operator< (const DateTime& b) const;
        bool        		operator> (const DateTime& b) const;
        bool        		operator<= (const DateTime& b) const;
        bool        		operator>= (const DateTime& b) const;

        void				setNow()																				{ date.setNow(); time.setNow(); }
		void				set (u32 year, u32 month, u32 day, u32 h, u32 m, u32 s, u32 ms = 0)						{ date.setYMD (year, month, day); time.setHMS (h, m, s, ms); }

        u64					getInternalRappresentation() const														{ u64 d = date.getInternalRappresentation(); u64 t=time.getInternalRappresentation(); return ((d<<32) | t); }
        void				setFromInternalRappresentation(u64 u)													{ date.setFromInternalRappresentation ((u32)((u >> 32) & 0x00000000FFFFFFFF)); time.setFromInternalRappresentation ((u32)(u&0x00000000FFFFFFFF)); }

		void				formatAs_YYYYMMDDHHMMSS (char *out, u32 sizeOfOut, char char_between_date_and_time=' ', char date_sep='-', char time_sep=':') const;
							/* ritorna in out una stringa cosi' composta YYYYaMMaDDbHHcMMcSS dove:
								YYYY = anno 4 cifre
								a = carattere di separazione (date_Sep, default ='-')
								MM = mese 2 cifre
								a = carattere di separazione (date_Sep, default ='-')
								DD = giorno 2 cifre
								b = carattere di separazione (char_between_date_and_time, default =' ')
								HH = ora, 2 cifre
								c = carattere di separazione (time_sep, default =':')
								MM = minuti, 2 cifre
								c = carattere di separazione (time_sep, default =':')
								SS = secondi, 2 cifre

								a, b, c possono eventualmente essere 0x00 se non si desidera includerli nella stringa finale
							*/



	void					addYMDHIS (i16 years, i16 months, i16 days, i16 hours, i16 minutes, i16 seconds);
	void					addMSec(u64 msec);

	public:
        Date				date;
        Time24				time;

        
    };
} //namespace gos
#endif // _gosDateTime_h_
