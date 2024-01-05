#ifndef _LoggerStdout_h_
#define _LoggerStdout_h_
#include "gosLogger.h"
#include "../gos.h"
#include "../gosFastArray.h"


namespace gos
{
    /******************************************
     * LoggerStdout
     *
     * Semplice log con funzionalità di indentazione che butta
     * tutto sullo stdout
     */
    class LoggerStdout : public Logger
    {
    public:

    public:
                            LoggerStdout();
		virtual             ~LoggerStdout();

        void                disableStdouLogging()                                           { bShoudLogToStdout=false; }
        void                enableFileLogging (const u8 *fullFolderPathAndName);
                                //accetta il path di un folder dentro al quale crea un certo numero di file di log in totale autonomia

		void                incIndent();
		void                decIndent();

        void                vlog (const char *format, va_list argptr);
        void                vlog (const eTextColor col, const char *format, va_list argptr);
        void                vlogWithPrefix (const char *prefix, const char *format, va_list argptr);
        void                vlogWithPrefix (const eTextColor col, const char *prefix, const char *format, va_list argptr);


    private:
        static const u16    MAX_INDENT_CHAR = 31;
        static const u16    INTERNAL_BUFFER_SIZE = 1024;

    private:
        class LogToFile
        {
        public:
                    LogToFile (const u8 *fullFolderPathAndName);
                    ~LogToFile();
            
            void    log (const char *format, ...);

        private:
            static const u16    FLUSH_COUNTER = 200;                //ogni quanti "output" devo chiudere e riaprire il file nella speranza di flusharlo su disco per davvero?
            static const u16    CHECK_SIZE_COUNTER = 70;            //ogni quanti "output" devo chiudere verificare che il file non sia diventato troppo grande?
            static const u64    MAX_FILE_SIZE_BYTE = 1024*1024;
            static const u64    MAX_NUM_LOGFILE_IN_FOLDER = 10;

        private:
            u8*                 priv_allocString (const u8 *strIN) const;
            void                priv_openForAppend ();
            void                priv_closeAndFlush();
            void                priv_getLogFileList (gos::FastArray<u64> &elenco) const;
            void                priv_createNewLogFileAndOpenForAppend();     
            void                priv_clearLogFolder();
            void                priv_logIntestazione();

        private:
            gos::File   f;
            u8          *filename;
            u8          *fullFolderPathAndName;
            bool        bIsOpen;
            u16         flushCounter;
            u16         checkFileSizeCounter;
        };

    private:
        void                priv_buildIndentStr();
        void                priv_out (const char *what);
        void                priv_log (const char *prefix, const char *format, va_list argptr);
        
        void                priv_logToFileClearLogFolder();
        
    private:
        bool                bShoudLogToStdout;
        u16                 indent;
        char                strIndent[MAX_INDENT_CHAR+1];
        char                buffer[INTERNAL_BUFFER_SIZE];
        u8                  isANewLine;
		Mutex	            mutex;
        LogToFile           *logToFile;
    };
} //namespace gos
#endif //_LoggerStdout_h_
