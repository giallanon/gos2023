#include "gosLoggerStdout.h"
#include "../gos.h"
#include "../gosFastArray.h"


using namespace gos;


//*************************************************
LoggerStdout::LoggerStdout()
{
    gos::thread::mutexCreate(mutex);
	indent = 0;
    isANewLine = 1;
    logfile_filename = NULL;
    logFile_fullFolderPathAndName = NULL;
    checkLogFileSize = LOGFILE_CHECK_SIZE_COUNTER;
    priv_buildIndentStr();
}

//*************************************************
LoggerStdout::~LoggerStdout() 
{
    gos::thread::mutexDestroy(mutex);

    if (NULL != logFile_fullFolderPathAndName)
    {
        Allocator *localAllocator = gos::getSysHeapAllocator();
        GOSFREE(localAllocator, logFile_fullFolderPathAndName);
        logFile_fullFolderPathAndName = NULL;

        if (NULL != logfile_filename)
        {
            GOSFREE(localAllocator, logfile_filename);
            logfile_filename = NULL;
        }

    }
}

//*************************************************
void LoggerStdout::enableFileLogging (const u8 *fullFolderPathAndName)
{
    Allocator *localAllocator = gos::getSysHeapAllocator();
    if (NULL != logFile_fullFolderPathAndName)
        GOSFREE(localAllocator, logFile_fullFolderPathAndName);
    
    gos::fs::folderCreate(fullFolderPathAndName);
    logFile_fullFolderPathAndName = gos::string::utf8::allocStr (localAllocator, fullFolderPathAndName);
    checkLogFileSize = LOGFILE_CHECK_SIZE_COUNTER;

    priv_createNewLogFile();
}

//*************************************************
void LoggerStdout::priv_createNewLogFile()
{
    gos::Allocator *localAllocator = gos::getSysHeapAllocator();
    if (NULL != logfile_filename)
    {
        GOSFREE(localAllocator, logfile_filename);
        logfile_filename = NULL;
    }

    char yyyymmddhhmmss[64];
    gos::DateTime dt;
    dt.setNow();
    dt.formatAs_YYYYMMDDHHMMSS (yyyymmddhhmmss, sizeof(yyyymmddhhmmss), 0x00, 0x00, 0x00);

    u8 s[1204];
    gos::string::utf8::spf (s, sizeof(s), "%s/%s.goslog", logFile_fullFolderPathAndName, yyyymmddhhmmss);

    gos::File f;
    if (!gos::fs::fileOpenForW (&f, s, true))
    {
        DBGBREAK;
        return;
    }
    gos::fs::fileClose(f);
    logfile_filename = gos::string::utf8::allocStr (localAllocator, s);

    priv_logToFileClearLogFolder();
}

//*************************************************
bool LoggerStdout_sortFn (const u64 &a, const u64 &b)
{
    return a > b;
}

/*************************************************
* controlla il numero di file presenti nella cartella di log
* Se questo numero e' maggiore di una certa soglia, cancella i pi� vecchi
*/
void LoggerStdout::priv_logToFileClearLogFolder()
{
    gos::FileFind ff;
    if (gos::fs::findFirst (&ff, logFile_fullFolderPathAndName, "*.goslog"))
    {
        gos::Allocator *localAllocator = gos::getScrapAllocator();
        gos::FastArray<u64> elenco;
        elenco.setup (localAllocator, 20);

        u8 s[1024];
        do
        {
            if (!gos::fs::findIsDirectory(ff))
            {
                //il nome del file � in formato YYMMDDhhmm quindi ci sta in un u32
                gos::fs::extractFileNameWithoutExt (gos::fs::findGetFileName(ff), s, sizeof(s));
                elenco.append (gos::string::utf8::toU64(s));
            }
        } while (gos::fs::findNext(ff));
        gos::fs::findClose(ff);


        if (elenco.getNElem() > LOGFILE_MAX_NUM_LOGFILE_IN_FOLDER)
        {
            //sorto dal pi� piccolo al piu' grande
            elenco.bubbleSort(LoggerStdout_sortFn);
            
            //elimino i file
            const u32 numFileToDelete = elenco.getNElem() - LOGFILE_MAX_NUM_LOGFILE_IN_FOLDER;
            for (u32 i = 0; i < numFileToDelete; i++)
            {
                gos::string::utf8::spf (s, sizeof(s), "%s/%llu.goslog", logFile_fullFolderPathAndName, elenco(i));
                gos::fs::fileDelete(s);
            }
        }

        elenco.unsetup();
    }
}

//*************************************************
void LoggerStdout::priv_buildIndentStr()
{
    u32 n = indent*2;
    if (n > MAX_INDENT_CHAR)
        n = MAX_INDENT_CHAR;

    memset (strIndent, ' ', n);
    strIndent[n] = 0;
}

//*************************************************
void LoggerStdout::priv_out (const char *what)
{
    if (isANewLine)
    {
        char hhmmss[16];
        gos::Time24 dt;
        dt.setNow();
        dt.formatAs_HHMMSS (hhmmss, sizeof(hhmmss), ':');
        fprintf (stdout, "%s %s", hhmmss, strIndent);
        isANewLine = 0;
    }

    fprintf (stdout, "%s", what);
    fflush(stdout);
}

//*************************************************
void LoggerStdout::incIndent() 
{ 
    gos::thread::mutexLock(mutex);
	++indent; 
	priv_buildIndentStr();
	gos::thread::mutexUnlock(mutex);
}

//*************************************************
void LoggerStdout::decIndent() 
{
	gos::thread::mutexLock(mutex);
	if (indent) 
		--indent; 
	priv_buildIndentStr();
	gos::thread::mutexUnlock(mutex);
}

//*************************************************
void LoggerStdout::vlog (const eTextColor col, const char *format, va_list argptr)
{
    gos::thread::mutexLock(mutex);
    {
        const eTextColor prevColor = console::setTextColor(col);
        priv_log (NULL, format, argptr);
        console::setTextColor(prevColor);
    }
    gos::thread::mutexUnlock(mutex);
}

//*************************************************
void LoggerStdout::vlog (const char *format, va_list argptr)
{
    gos::thread::mutexLock(mutex);
    priv_log (NULL, format, argptr);
    gos::thread::mutexUnlock(mutex);
}

//*************************************************
void LoggerStdout::vlogWithPrefix (const char *prefix, const char *format, va_list argptr)
{
	gos::thread::mutexLock(mutex);
    priv_log (prefix, format, argptr);
    gos::thread::mutexUnlock(mutex);
}

//*************************************************
void LoggerStdout::vlogWithPrefix (const eTextColor col, const char *prefix, const char *format, va_list argptr)
{
    gos::thread::mutexLock(mutex);
    {
        const eTextColor prevColor = console::setTextColor(col);
        priv_log (prefix, format, argptr);
        console::setTextColor(prevColor);
    }
    gos::thread::mutexUnlock(mutex);
}

//*************************************************
void LoggerStdout::priv_log (const char *prefix, const char *format, va_list argptr) 
{
    if (NULL == prefix)
        vsnprintf (buffer, INTERNAL_BUFFER_SIZE, format, argptr);
    else
    {
        const size_t n = strlen(prefix);
        memcpy (buffer, prefix, n);
        vsnprintf (&buffer[n], INTERNAL_BUFFER_SIZE - n, format, argptr);
    }

    u32 n = (u32)strlen(buffer);
	if (n == 0)
		return;

    if (NULL != logfile_filename)
    {
        priv_logToFile(buffer);    
    }

	u32 i = 0;
    if (buffer[0] == '\n')
    {
        fprintf (stdout, "\n");
        isANewLine = 1;
        i++;
    }

    u32 iStart = i;
    while (1)
    {
        if (buffer[i] == '\n')
        {
            buffer[i] = 0;
            if (i-iStart)
                priv_out (&buffer[iStart]);
            fprintf (stdout, "\n");

            isANewLine = 1;
            i++;
            iStart = i;
            continue;
        }

        if (buffer[i] == 0x00)
        {
            if (i-iStart)
                priv_out (&buffer[iStart]);
            break;
        }

        ++i;
    }
}

//*************************************************
void LoggerStdout::priv_logToFile (const char *what)
{
    if (NULL == logfile_filename)
    {
        DBGBREAK;
        return;
    }

    char hhmmss[16];
    gos::Time24 dt;
    dt.setNow();
    dt.formatAs_HHMMSS (hhmmss, sizeof(hhmmss), ':');

    gos::File f;
    gos::fs::fileOpenForAppend (&f, logfile_filename);
    gos::fs::fpf (f, "%s %s %s", hhmmss, strIndent, what);
    gos::fs::fileFlush(f);
    gos::fs::fileClose(f);

    //ogni [LOGFILE_CHECK_SIZE_COUNTER] righe di log su file, verifico la dimensione del file
    //Se supera una certa soglia, passo ad usare un nuovo file
    if (checkLogFileSize-- == 0)
    {
        checkLogFileSize = LOGFILE_CHECK_SIZE_COUNTER;
        if (gos::fs::fileLength(logfile_filename) >= LOGFILE_MAX_FILE_SIZE_BYTES)
            priv_createNewLogFile();
    }
}
