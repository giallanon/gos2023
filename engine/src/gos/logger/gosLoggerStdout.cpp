#include "gosLoggerStdout.h"
#include "../gos.h"


using namespace gos;



bool LoggerStdout_sortFn_ascendente (const u64 &a, const u64 &b)            { return a > b; }
bool LoggerStdout_sortFn_discendente (const u64 &a, const u64 &b)           { return b > a; }



//*************************************************
LoggerStdout::LoggerStdout()
{
    gos::thread::mutexCreate(&mutex);
    bShoudLogToStdout = true;
	indent = 0;
    isANewLine = 1;
    logToFile = NULL;
    priv_buildIndentStr();
}

//*************************************************
LoggerStdout::~LoggerStdout() 
{
    gos::thread::mutexDestroy(mutex);
    if (NULL != logToFile)
        GOSDELETE(gos::getSysHeapAllocator(), logToFile);
}

//*************************************************
void LoggerStdout::enableFileLogging (const u8 *fullFolderPathAndName)
{
    logToFile = GOSNEW(gos::getSysHeapAllocator(), LogToFile) (fullFolderPathAndName);
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

        if (bShoudLogToStdout)
            fprintf (stdout, "%s %s", hhmmss, strIndent);

        if (logToFile && logToFile->isOpen())
            gos::fs::fpf (logToFile->_f, "%s %s", hhmmss, strIndent);

        isANewLine = 0;
    }

    if (bShoudLogToStdout)
    {
        fprintf (stdout, "%s", what);
        fflush(stdout);
    }

    if (logToFile && logToFile->isOpen())
        gos::fs::fpf (logToFile->_f, "%s", what);
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

    if (NULL != logToFile)
    {
        logToFile->open();
    }

	u32 i = 0;
    if (buffer[0] == '\n')
    {
        if (bShoudLogToStdout)
            fprintf (stdout, "\n");
        
        if (logToFile && logToFile->isOpen())
            gos::fs::fpf (logToFile->_f, "\n");

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
            
            if (bShoudLogToStdout)
                fprintf (stdout, "\n");
            
            if (logToFile && logToFile->isOpen())
                gos::fs::fpf (logToFile->_f, "\n");

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

    if (logToFile)
        logToFile->close();

}


/***************************************************************************
 * class LogToFile
 * 
 * 
 * 
 ****************************************************************************/
LoggerStdout::LogToFile::LogToFile(const u8 *fullFolderPathAndNameIN)
{
    bIsOpen = false;
    filename = NULL;
    checkLogFileSize = LOGFILE_CHECK_SIZE_COUNTER;
    fullFolderPathAndName = gos::string::utf8::allocStr (gos::getSysHeapAllocator(), fullFolderPathAndNameIN);

    gos::fs::folderCreate(fullFolderPathAndName);
    priv_clearLogFolder();

    //apro un nuovo file oppure uso l'ultimo gia' esistente
    bool bCreateNew = true;
    gos::FastArray<u64> elenco (gos::getScrapAllocator(), 20);
    priv_getLogFileList (elenco);
    if (elenco.getNElem())
    {
        elenco.bubbleSort(LoggerStdout_sortFn_discendente);

        u8 s[1024];
        gos::string::utf8::spf (s, sizeof(s), "%s/%" PRIu64 ".goslog", fullFolderPathAndName, elenco(0));
        if (gos::fs::fileLength (s) < ((LOGFILE_MAX_FILE_SIZE_BYTES * 80) / 100) )
        {
            bCreateNew = false;
            filename = gos::string::utf8::allocStr (gos::getSysHeapAllocator(), s);
        }
    }

    if (bCreateNew)
        priv_createNewLogFile();    


    //intestazione iniziale
    char ymdhms[64];
    gos::DateTime dt;
    dt.setNow();
    dt.formatAs_YYYYMMDDHHMMSS (ymdhms, sizeof(ymdhms));

    open();
    gos::fs::fpf (_f, "\n\n\n\n============================================================================\n");
    gos::fs::fpf (_f, "LOG BEGIN @ %s\n", ymdhms);

    close();

}

LoggerStdout::LogToFile::~LogToFile()
{ 
    close();

    if (NULL != fullFolderPathAndName)
    {
        Allocator *localAllocator = gos::getSysHeapAllocator();
        GOSFREE (localAllocator, fullFolderPathAndName);
        fullFolderPathAndName = NULL;

        if (NULL != filename)
        {
            GOSFREE (localAllocator, filename);
            filename = NULL;
        }
    }    
}

void LoggerStdout::LogToFile::open ()
{ 
    if (bIsOpen) 
        close(); 
    if (NULL == filename)
    {
        DBGBREAK;
        return;
    }        
    bIsOpen = gos::fs::fileOpenForAppend (&_f, filename); 
}

void LoggerStdout::LogToFile::close()
{
    if (!bIsOpen) 
        return;
    gos::fs::fileFlush(_f);
    gos::fs::fileClose (_f);
    bIsOpen = false;

    //ogni [LOGFILE_CHECK_SIZE_COUNTER] righe di log su file, verifico la dimensione del file
    //Se supera una certa soglia, passo ad usare un nuovo file
    if (checkLogFileSize-- == 0)
    {
        checkLogFileSize = LOGFILE_CHECK_SIZE_COUNTER;
        if (gos::fs::fileLength (filename) >= LOGFILE_MAX_FILE_SIZE_BYTES)
            priv_createNewLogFile();
    }
}

void LoggerStdout::LogToFile::priv_createNewLogFile()
{
    gos::Allocator *localAllocator = gos::getSysHeapAllocator();
    if (NULL != filename)
    {
        GOSFREE (localAllocator, filename);
        filename = NULL;
    }

    char yyyymmddhhmmss[64];
    gos::DateTime dt;
    dt.setNow();
    dt.formatAs_YYYYMMDDHHMMSS (yyyymmddhhmmss, sizeof(yyyymmddhhmmss), 0x00, 0x00, 0x00);

    u8 s[1204];
    gos::string::utf8::spf (s, sizeof(s), "%s/%s.goslog", fullFolderPathAndName, yyyymmddhhmmss);

    gos::File f;
    if (!gos::fs::fileOpenForW (&f, s, true))
    {
        DBGBREAK;
        return;
    }
    gos::fs::fileClose(f);

    filename = gos::string::utf8::allocStr (localAllocator, s);
    priv_clearLogFolder();
}

void LoggerStdout::LogToFile::priv_getLogFileList (gos::FastArray<u64> &elenco) const
{
    //elenco dei file di log nella directory
    gos::FileFind ff;
    if (gos::fs::findFirst (&ff, fullFolderPathAndName, "*.goslog"))
    {
        //elenco dei file di log nella directory
        u8 s[1024];
        do
        {
            if (!gos::fs::findIsDirectory(ff))
            {
                gos::fs::extractFileNameWithoutExt (gos::fs::findGetFileName(ff), s, sizeof(s));
                elenco.append (gos::string::utf8::toU64(s));
            }
        } while (gos::fs::findNext(ff));
        gos::fs::findClose(ff);
    }
}

/*************************************************
* controlla il numero di file presenti nella cartella di log
* Se questo numero e' maggiore di una certa soglia, cancella i piu' vecchi
*/
void LoggerStdout::LogToFile::priv_clearLogFolder()
{
    gos::FastArray<u64> elenco (gos::getScrapAllocator(), 20);
    priv_getLogFileList (elenco);
        

    //se sono troppi, ne butto via un po'
    if (elenco.getNElem() > LOGFILE_MAX_NUM_LOGFILE_IN_FOLDER)
    {
        //sorto dal piu' piccolo al piu' grande
        elenco.bubbleSort(LoggerStdout_sortFn_ascendente);
        
        //elimino i file
        const u32 numFileToDelete = elenco.getNElem() - LOGFILE_MAX_NUM_LOGFILE_IN_FOLDER;
        for (u32 i = 0; i < numFileToDelete; i++)
        {
            u8 s[1024];
            gos::string::utf8::spf (s, sizeof(s), "%s/%" PRIu64 ".goslog", fullFolderPathAndName, elenco(i));
            gos::fs::fileDelete(s);
        }
    }

    elenco.unsetup();
}

