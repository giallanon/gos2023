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
                            Logger()                                     { }
        virtual             ~Logger()                                    { }

        virtual void        incIndent()     = 0;
        virtual void        decIndent()     = 0;

        void                log (const char *format, ...)                                                           { va_list argptr; va_start (argptr, format); vlog (format, argptr); va_end (argptr); }
        void                log (const eTextColor col, const char *format, ...)                                     { va_list argptr; va_start (argptr, format); vlog (col, format, argptr); va_end (argptr); }
        void                logWithPrefix (const char *prefix, const char *format, ...)                             { va_list argptr; va_start (argptr, format); vlogWithPrefix (prefix, format, argptr); va_end (argptr); }
        void                logWithPrefix (const eTextColor col, const char *prefix, const char *format, ...)       { va_list argptr; va_start (argptr, format); vlogWithPrefix (col, prefix, format, argptr); va_end (argptr); }

        virtual void        vlog (const char *format, va_list argptr) = 0;
        virtual void        vlog (const eTextColor col, const char *format, va_list argptr) = 0;
        virtual void        vlogWithPrefix (const char *prefix, const char *format, va_list argptr) = 0;
        virtual void        vlogWithPrefix (const eTextColor col, const char *prefix, const char *format, va_list argptr) = 0;

    };
} //namespace gos

#endif // _gosLogger_h_


