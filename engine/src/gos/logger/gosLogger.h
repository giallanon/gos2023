#ifndef _gosLogger_h_
#define _gosLogger_h_
#include "../gosEnumAndDefine.h"


namespace gos
{
    /******************************************
     * Logger
     *
     */
    class Logger
    {
	public:
		static constexpr u8	LEVEL__DEFAULT = 5;
		static constexpr u8	LEVEL__MIN_PRIORITY = 0;
		static constexpr u8	LEVEL__MAX_PRIORITY = 9;
		
		static constexpr u8	LEVEL__ERR = LEVEL__MAX_PRIORITY;
		static constexpr u8	LEVEL__WARN = (LEVEL__DEFAULT);
		static constexpr u8	LEVEL__VERBOSE = (LEVEL__DEFAULT - 1);

    public:
                            Logger()			{ min_visible_level = LEVEL__MIN_PRIORITY; }
        virtual             ~Logger()			{ }

        virtual void        inc_indent() = 0;
        virtual void        dec_indent() = 0;

		void 				set_visible_level (u8 min_visible_levelIN)												{ min_visible_level = min_visible_levelIN; }

        void                log (const char *format, ...)															{ va_list argptr; va_start (argptr, format); vlog (LEVEL__DEFAULT, format, argptr); va_end (argptr); }
        void                log (const eTextColor col, const char *format, ...)										{ va_list argptr; va_start (argptr, format); vlog (LEVEL__DEFAULT, col, format, argptr); va_end (argptr); }
        void                log_with_prefix (const char *prefix, const char *format, ...)							{ va_list argptr; va_start (argptr, format); vlog_with_prefix (LEVEL__DEFAULT, prefix, format, argptr); va_end (argptr); }
        void                log_with_prefix (const eTextColor col, const char *prefix, const char *format, ...)		{ va_list argptr; va_start (argptr, format); vlog_with_prefix (LEVEL__DEFAULT, col, prefix, format, argptr); va_end (argptr); }

        void                verbose (const char *format, ...)														{ va_list argptr; va_start (argptr, format); vlog_with_prefix (LEVEL__VERBOSE, eTextColor::darkYellow, "VERBOSE=>", format, argptr); va_end (argptr); }
        void                warn (const char *format, ...)															{ va_list argptr; va_start (argptr, format); vlog_with_prefix (LEVEL__WARN, eTextColor::magenta, "WARNING=>", format, argptr); va_end (argptr); }
        void                err (const char *format, ...)
                            {
	                            va_list argptr; 
	                            va_start (argptr, format); 
	                            vlog_with_prefix (LEVEL__ERR, eTextColor::red, "ERROR=>", format, argptr); 
	                            va_end (argptr); 
	                            DBGBREAK;
                            }

        void                log_wl (u8 level, const char *format, ...)															{ va_list argptr; va_start (argptr, format); vlog (level, format, argptr); va_end (argptr); }
        void                log_wl (u8 level, const eTextColor col, const char *format, ...)									{ va_list argptr; va_start (argptr, format); vlog (level, col, format, argptr); va_end (argptr); }
        void                log_with_prefix_wl (u8 level, const char *prefix, const char *format, ...)							{ va_list argptr; va_start (argptr, format); vlog_with_prefix (level, prefix, format, argptr); va_end (argptr); }
        void                log_with_prefix_wl (u8 level, const eTextColor col, const char *prefix, const char *format, ...)	{ va_list argptr; va_start (argptr, format); vlog_with_prefix (level, col, prefix, format, argptr); va_end (argptr); }


        virtual void        vlog (u8 level, const char *format, va_list argptr) = 0;
        virtual void        vlog (u8 level, const eTextColor col, const char *format, va_list argptr) = 0;
        virtual void        vlog_with_prefix (u8 level, const char *prefix, const char *format, va_list argptr) = 0;
        virtual void        vlog_with_prefix (u8 level, const eTextColor col, const char *prefix, const char *format, va_list argptr) = 0;

	protected:
		bool 				chklvl(u8 level) const 																				{ return level >= min_visible_level; }
		
	private:
		u8	min_visible_level;
    };
} //namespace gos

#endif // _gosLogger_h_


