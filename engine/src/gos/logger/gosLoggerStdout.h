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
            
            void    open ();
            void    close();

            bool    isOpen() const                                              { return bIsOpen; }

            gos::File   _f;

        private:
            static const u8     LOGFILE_CHECK_SIZE_COUNTER = 200;
            static const u64    LOGFILE_MAX_FILE_SIZE_BYTES = 1024*1024;
            static const u64    LOGFILE_MAX_NUM_LOGFILE_IN_FOLDER = 10;

        private:
            void                priv_getLogFileList (gos::FastArray<u64> &elenco) const;
            void                priv_createNewLogFile();     
            void                priv_clearLogFolder();

        private:
            u8          *filename;
            u8          *fullFolderPathAndName;
            bool        bIsOpen;
            u8          checkLogFileSize;
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
