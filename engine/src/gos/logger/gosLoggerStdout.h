#ifndef _LoggerStdout_h_
#define _LoggerStdout_h_
#include "gosLogger.h"
#include "../gosFastArray.h"
#include "../gosBit.h"


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
					LoggerStdout();
		virtual     ~LoggerStdout();

		void        enableStdouLogging()                                            { flag.set (FLAG__SHOULD_LOG_TO_STDOUT); }
		void        disableStdouLogging()                                           { flag.clear (FLAG__SHOULD_LOG_TO_STDOUT); }
		
					//accetta il path di un folder dentro al quale crea un certo numero di file di log in totale autonomia
		void        enableFileLogging (const char *fullFolderPathAndName, bool bClearFolder=false);
		void        disableFileLogging();

		void        inc_indent();
		void        dec_indent();

		void        vlog (u8 level, const char *format, va_list argptr);
		void        vlog (u8 level, const eTextColor col, const char *format, va_list argptr);
		void        vlog_with_prefix (u8 level, const char *prefix, const char *format, va_list argptr);
		void        vlog_with_prefix (u8 level, const eTextColor col, const char *prefix, const char *format, va_list argptr);


	private:
		static constexpr u16    MAX_INDENT_CHAR = 31;
		static constexpr u16    INTERNAL_BUFFER_SIZE = 4096;
		
		static constexpr u8     FLAG__SHOULD_LOG_TO_STDOUT = 0;
		static constexpr u8     FLAG__USE_HHMMSSMsec = 1;


	private:
		class LogToFile
		{
		public:
					LogToFile (const char *fullFolderPathAndName, bool bClearFolder);
					~LogToFile();
			
			void    log (const char *format, ...);

		private:
			static const u16    FLUSH_COUNTER = 200;                //ogni quanti "output" devo chiudere e riaprire il file nella speranza di flusharlo su disco per davvero?
			static const u16    CHECK_SIZE_COUNTER = 70;            //ogni quanti "output" devo chiudere verificare che il file non sia diventato troppo grande?
			static const u64    MAX_FILE_SIZE_BYTE = 1024*1024;
			static const u64    MAX_NUM_LOGFILE_IN_FOLDER = 10;

		private:
			char*               priv_allocString (const char *strIN) const;
			void                priv_openForAppend ();
			void                priv_closeAndFlush();
			void                priv_getLogFileList (gos::FastArray<u64> &elenco) const;
			void                priv_createNewLogFileAndOpenForAppend();     
			void                priv_clearLogFolder();
			void                priv_logIntestazione();

		private:
			gos::File   f;
			char        *filename;
			char        *fullFolderPathAndName;
			bool        bIsOpen;
			u16         flushCounter;
			u16         checkFileSizeCounter;
		};

	private:
		void                priv_buildIndentStr();
		void                priv_out (u8 level, const char *what);
		void                priv_log (u8 level, const char *prefix, const char *format, va_list argptr);
		
		void                priv_logToFileClearLogFolder();
		

	private:
		gos::Flag8			flag;
		u16                 indent;
		char                strIndent[MAX_INDENT_CHAR+1];
		char                buffer[INTERNAL_BUFFER_SIZE];
		u8                  isANewLine;
		Mutex	            mutex;
		LogToFile           *logToFile;
	};
} //namespace gos
#endif //_LoggerStdout_h_
