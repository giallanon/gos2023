#ifndef _gosLoggerNull_h_
#define _gosLoggerNull_h_
#include "gosLogger.h"



namespace gos
{
    /******************************************
     * LoggerNull
     *
     * non stampa niente
     */
    class LoggerNull : public Logger
    {
    public:
                            LoggerNull()          { }
        virtual             ~LoggerNull()         { }

        void                incIndent()           { }
        void                decIndent()           { }

        void                vlog (UNUSED_PARAM(const eTextColor col), UNUSED_PARAM(const char *format), UNUSED_PARAM(va_list argptr))             { }
        void                vlog (UNUSED_PARAM(const char *format), UNUSED_PARAM(va_list argptr))                                                 { }
        void                vlogWithPrefix (UNUSED_PARAM(const char *prefix), UNUSED_PARAM(const char *format), UNUSED_PARAM(va_list argptr))     { }
        void                vlogWithPrefix (UNUSED_PARAM(const eTextColor col), UNUSED_PARAM(const char *prefix), UNUSED_PARAM(const char *format), UNUSED_PARAM(va_list argptr)) { }


    };
} //namespace gos
#endif //_gosLoggerNull_h_
