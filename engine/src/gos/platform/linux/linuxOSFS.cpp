#ifdef GOS_PLATFORM__LINUX
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include "linuxOS.h"
#include "../../gos.h"


//*********************************************
static bool linux_createFolderFromUTF8Path (const char *utf8_path, u32 nBytesToUseForPath)
{
    char path[512];
    memcpy (path, utf8_path, nBytesToUseForPath);
    path[nBytesToUseForPath] = 0x00;

    if (0 == mkdir(path, 0777))
        return true;

    if (errno == EEXIST)
        return true;
    return false;
}

//*****************************************************
bool platform::FS_folderCreate (const char *utf8_path)
{
    if (NULL == utf8_path)
        return false;
    if (utf8_path[0] == 0x00)
        return false;


    u32 n = 1;
    while (utf8_path[n] != 0x00)
    {
        if (utf8_path[n]=='\\' || utf8_path[n]=='/')
        {
            if (!linux_createFolderFromUTF8Path(utf8_path,n))
                return false;
        }
        n++;
    }

    return linux_createFolderFromUTF8Path(utf8_path, n);
}

//*****************************************************
bool platform::FS_folderDelete (const char *path)
{
    return (rmdir(path) == 0);
}

//*****************************************************
bool platform::FS_folderExists(const char *path)
{
    struct stat sb;
    if (stat(path, &sb) == 0 && S_ISDIR(sb.st_mode))
        return true;
    return false;
}

//*****************************************************
bool platform::FS_fileExists(const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (NULL == f)
        return false;
    fclose(f);
    return true;
}

//*****************************************************
bool platform::FS_fileDelete(const char *filename)
{
    return (remove(filename) == 0);
}

//*****************************************************
bool platform::FS_fileRename(const char *utf8_pathNoSlash, const char *utf8_oldFilename, const char *utf8_newFilename)
{
    char temp1[512];
    gos::string::utf8::spf (temp1, sizeof(temp1), "%s/%s", utf8_pathNoSlash, utf8_oldFilename);
    gos::fs::pathSanitizeInPlace(temp1);

    char temp2[512];
    gos::string::utf8::spf (temp2, sizeof(temp2), "%s/%s", utf8_pathNoSlash, utf8_newFilename);
    gos::fs::pathSanitizeInPlace(temp2);

    return (rename(temp1, temp2) == 0);
}

//*****************************************************
void platform::FS_fileGetCreationTime_UTC (const char *filePathAndName, gos::DateTime *out_dt)
{
    //NB; linux non ha la nozione di creationTime, quindi ritorno il last modified time
    FS_fileGetLastTimeModified_UTC (filePathAndName, out_dt);
}

//*****************************************************
void platform::FS_fileGetLastTimeModified_UTC(const char *filePathAndName, gos::DateTime *out_dt)
{
    assert(NULL != out_dt);
    assert(NULL != filePathAndName);
    
    struct stat attrib;
    stat (filePathAndName, &attrib);

    struct tm tm;
    gmtime_r(&attrib.st_mtime, &tm);
    
    out_dt->set (tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
}

//*****************************************************
void platform::FS_fileGetCreationTime_LocalTime(const char *filePathAndName, gos::DateTime *out_dt)
{
    //NB; linux non ha la nozione di creationTime, quindi ritorno il last modified time
    FS_fileGetLastTimeModified_LocalTime (filePathAndName, out_dt);
}

//*****************************************************
void platform::FS_fileGetLastTimeModified_LocalTime(const char *filePathAndName, gos::DateTime *out_dt)
{
    assert(NULL != out_dt);
    assert(NULL != filePathAndName);
    
    struct stat attrib;
    stat (filePathAndName, &attrib);

    struct tm tm;
    localtime_r(&attrib.st_mtime, &tm);
    
    out_dt->set (tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
}

//*****************************************************
bool platform::FS_fileOpen  (OSFile *out_h, const char *utf8_filePathAndName, eFileMode openMode, bool bCreateIfNotExists, bool bAppend, UNUSED_PARAM(bool bShareRead), UNUSED_PARAM(bool bShareWrite))
{
    assert (NULL != out_h);
    assert (NULL != utf8_filePathAndName);

    mode_t mode = {0};
    int flag = 0;
    switch (openMode)
    {
    default:
        DBGBREAK;
        *out_h = -1;
        return false;            

    case eFileMode::readOnly:
        flag = O_RDONLY;
        break;

    case eFileMode::writeOnly:
        flag = O_WRONLY;
        break;

    case eFileMode::readWrite:
        flag = O_RDWR;
    }

    if (bCreateIfNotExists)
    {
        flag |= O_CREAT;
        mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; //user RW, group R, other R
    }

    if (bAppend)
        flag |= O_APPEND;
    else
    {
        if (eFileMode::writeOnly == openMode)
            flag |= O_TRUNC;
    }

    flag |= O_CLOEXEC;
    *out_h = open (reinterpret_cast<const char*>(utf8_filePathAndName), flag, mode);
    if (-1 == *out_h)
    {
        DBGBREAK;
        return false;
    }

    return true;
}

//*****************************************************
void platform::FS_fileClose (OSFile &h)
{
    close(h);
    h = -1;
}

//*****************************************************
void platform::FS_fileFlush (OSFile &h)
{
    fsync(h);
}

//*****************************************************
u64 platform::FS_fileLength (OSFile &h)
{
    struct stat st;
    fstat(h, &st);
    return st.st_size;    
}

//*****************************************************
u64 platform::FS_fileLength (const char *utf8_filePathAndName)
{
    struct stat st;
    stat(reinterpret_cast<const char*>(utf8_filePathAndName), &st);
    return st.st_size;    
}

//*****************************************************
bool platform::FS_fileSeek(OSFile &h, u64 position, eSeek seekMode)
{
    int w;
    switch (seekMode)
    {
    default:
        DBGBREAK;
        return false;
        
    case eSeek::start: 
        w = SEEK_SET;
        break;
    
    case eSeek::current:
        w = SEEK_CUR;
        break;
    
    case eSeek::end: 
        w = SEEK_END;
        break;
    }

    return ((off_t)-1 != lseek (h, position, w));
}

//*****************************************************
u64 platform::FS_fileTell(OSFile &h)
{
    return lseek(h, 0, SEEK_CUR);
}

//*****************************************************
u32 platform::FS_fileRead (OSFile &h, void *buffer, u32 numMaxBytesToRead)
{
    ssize_t ret = read(h, buffer, numMaxBytesToRead);
    if (ret < 0)
        return 0;
    return ret;
}

//*****************************************************
u32 platform::FS_fileWrite (OSFile &h, const void *buffer, u32 numBytesToWrite)
{
    ssize_t ret = write(h, buffer, numBytesToWrite);
    if (ret < 0)
        return 0;
    return ret;
}

//*****************************************************
bool platform::FS_findFirst (OSFileFind *ff, const char *utf8_path, const char *utf8_jolly, eFileFindMode ffmode)
{
    assert(ff->dirp == NULL);

    switch (ffmode)
    {
    default: 
        ff->findMode = 0;
        DBGBREAK;
        break;
    case eFileFindMode::both_file_and_folder:   ff->findMode = OSFileFind::ALLOW_FILE | OSFileFind::ALLOW_FOLDER; break;
    case eFileFindMode::only_folder:            ff->findMode = OSFileFind::ALLOW_FOLDER; break;
    case eFileFindMode::only_file:              ff->findMode = OSFileFind::ALLOW_FILE; break;
    }    

    char filename[1024];
    sprintf_s(filename, sizeof(filename), "%s/%s", utf8_path, utf8_jolly);

    ff->dirp = opendir((const char*)utf8_path);
    if (NULL == ff->dirp)
        return false;

    strcpy_s (ff->strJolly, sizeof(ff->strJolly), (const char*)utf8_jolly);
    if (FS_findNext(*ff))
        return true;
    FS_findClose(*ff);
    return false;
}

//*****************************************************
bool platform::FS_findNext (OSFileFind &ff)
{
    assert(ff.dirp != NULL);

    while (1)
    {
        ff.dp = readdir (ff.dirp);
        if (NULL == ff.dp)
            return false;

        if (ff.dp->d_type == DT_DIR)
        {
            //e' una dir
			bool bSkipThisFolder = false;

            if (0 == (ff.findMode & OSFileFind::ALLOW_FOLDER))
                bSkipThisFolder = true;
            else
            {
                if (ff.dp->d_name[0] == '.')
                {
                    if (0x00 == ff.dp->d_name[1])
                        bSkipThisFolder = true;
                    else if ('.' == ff.dp->d_name[1] && 0x00 == ff.dp->d_name[2])
                        bSkipThisFolder = true;
                }
            }

            if (!bSkipThisFolder)
                return true;
        }
        else if (gos::fs::doesFileNameMatchJolly (ff.dp->d_name, ff.strJolly))
        {
            //E' un file
            if (0 != (ff.findMode & OSFileFind::ALLOW_FILE))
                return true;
        }
    }
}

//*****************************************************
void platform::FS_findClose(OSFileFind &ff)
{
    assert(ff.dirp != NULL);
    closedir(ff.dirp);
    ff.dirp = NULL;
}

//*****************************************************
bool platform::FS_findIsDirectory(const OSFileFind &ff)
{
    assert(ff.dirp != NULL);
    if (ff.dp->d_type == DT_DIR)
        return true;
    return false;
}

//*****************************************************
const char* platform::FS_findGetFileName(const OSFileFind &ff)
{
    assert(ff.dirp != NULL);
    return ff.dp->d_name;
}

//*****************************************************
void platform::FS_findGetFileName (const OSFileFind &ff, char *out, u32 sizeofOut)
{
    assert(ff.dirp != NULL);
    sprintf_s((char*)out, sizeofOut, "%s", ff.dp->d_name);
}

//*****************************************************
bool platform::FS_fileCopy (const char *src, const char *dst)
{
    int source = open (src, O_RDONLY, 0);
    int dest   = open (dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    struct stat stat_source;
    fstat (source, &stat_source);

    ssize_t result = 0;
    off_t byteTransferred = 0;
    while (byteTransferred < stat_source.st_size)
    {
        result = sendfile (dest, source, &byteTransferred, stat_source.st_size);
        if (-1 == result)
            break;

        byteTransferred += result;
    }

    close(source);
    close(dest);

    return (result != -1);    
}


#endif //GOS_PLATFORM__LINUX