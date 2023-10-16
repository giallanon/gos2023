#ifdef GOS_PLATFORM__LINUX
#include <sys/stat.h>
#include <fcntl.h>
#include "linuxOS.h"
#include "../../gos.h"


//*********************************************
static bool linux_createFolderFromUTF8Path (const u8 *utf8_path, u32 nBytesToUseForPath)
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
bool platform::FS_folderCreate (const u8 *utf8_path)
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
bool platform::FS_folderDelete (const u8 *path)
{
    return (rmdir((const char*)path) == 0);
}

//*****************************************************
bool platform::FS_folderExists(const u8 *path)
{
    struct stat sb;
    if (stat((const char*)path, &sb) == 0 && S_ISDIR(sb.st_mode))
        return true;
    return false;
}

//*****************************************************
bool platform::FS_fileExists(const u8 *filename)
{
    FILE *f = fopen((const char*)filename, "r");
    if (NULL == f)
        return false;
    fclose(f);
    return true;
}

//*****************************************************
bool platform::FS_fileDelete(const u8 *filename)
{
    return (remove((const char*)filename) == 0);
}

//*****************************************************
bool platform::FS_fileRename(const u8 *utf8_pathNoSlash, const u8 *utf8_oldFilename, const u8 *utf8_newFilename)
{
    u8 temp1[512];
    gos::string::utf8::spf (temp1, sizeof(temp1), "%s/%s", utf8_pathNoSlash, utf8_oldFilename);
    gos::fs::pathSanitizeInPlace(temp1);

    u8 temp2[512];
    gos::string::utf8::spf (temp2, sizeof(temp2), "%s/%s", utf8_pathNoSlash, utf8_newFilename);
    gos::fs::pathSanitizeInPlace(temp2);

    return (rename(reinterpret_cast<const char*>(temp1), reinterpret_cast<const char*>(temp2)) == 0);
}

//*****************************************************
void platform::FS_fileGetLastTimeModified_UTC (const char *filePathAndName, gos::DateTime *out_dt)
{
    //NB; linux non ha la nozione di creationTime, quindi ritorno il last modified time
    FS_fileGetLastTimeModified_UTC (filePathAndName, out_dt);
}
void platform::FS_fileGetCreationTime_UTC(const u8 *utf8_filePathAndName, gos::DateTime *out_dt)
{
    //NB; linux non ha la nozione di creationTime, quindi ritorno il last modified time
    FS_fileGetLastTimeModified_UTC (utf8_filePathAndName, out_dt);
}

//*****************************************************
void platform::FS_fileGetLastTimeModified_UTC(const u8 *utf8_filePathAndName, gos::DateTime *out_dt)
{
    platform::FS_fileGetLastTimeModified_UTC (reinterpret_cast<const char*>(utf8_filePathAndName), out_dt);
}
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
void platform::FS_fileGetCreationTime_LocalTime(const u8 *utf8_filePathAndName, gos::DateTime *out_dt)
{
    FS_fileGetCreationTime_LocalTime (reinterpret_cast<const char*>(utf8_filePathAndName), out_dt);
}
void platform::FS_fileGetCreationTime_LocalTime(const char *filePathAndName, gos::DateTime *out_dt)
{
    //NB; linux non ha la nozione di creationTime, quindi ritorno il last modified time
    FS_fileGetLastTimeModified_LocalTime (filePathAndName, out_dt);
}

//*****************************************************
void platform::FS_fileGetLastTimeModified_LocalTime(const u8 *utf8_filePathAndName, gos::DateTime *out_dt)
{
    FS_fileGetLastTimeModified_LocalTime (reinterpret_cast<const char*>(utf8_filePathAndName), out_dt);
}
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
bool platform::FS_fileOpen  (OSFile *out_h, const u8 *utf8_filePathAndName, eFileMode openMode, bool bCreateIfNotExists, bool bAppend, UNUSED_PARAM(bool bShareRead), UNUSED_PARAM(bool bShareWrite))
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
u64 platform::FS_fileLength (const u8 *utf8_filePathAndName)
{
    struct stat st;
    stat(reinterpret_cast<const char*>(utf8_filePathAndName), &st);
    return st.st_size;    
}

//*****************************************************
void platform::FS_fileSeek(OSFile &h, u64 position, eSeek seekMode)
{
    int w;
    switch (seekMode)
    {
    default:
        DBGBREAK;
        return;
        
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

    lseek (h, position, w);
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
bool platform::FS_findFirst (OSFileFind *ff, const u8 *utf8_path, const u8 *utf8_jolly)
{
    assert(ff->dirp == NULL);

    char filename[1024];
    sprintf_s(filename, sizeof(filename), "%s/%s", utf8_path, utf8_jolly);

    ff->dirp = opendir((const char*)utf8_path);
    if (NULL == ff->dirp)
        return false;

    strcpy_s (ff->strJolly, sizeof(ff->strJolly), (const char*)utf8_jolly);
    return FS_findNext(*ff);
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

        //se � una dir...
        if (ff.dp->d_type == DT_DIR)
            return true;

        if (gos::fs::doesFileNameMatchJolly ((const u8 *)ff.dp->d_name, (const u8 *)ff.strJolly))
            return true;
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
const u8* platform::FS_findGetFileName(const OSFileFind &ff)
{
    assert(ff.dirp != NULL);
    return (const u8 *)ff.dp->d_name;
}

//*****************************************************
void platform::FS_findGetFileName (const OSFileFind &ff, u8 *out, u32 sizeofOut)
{
    assert(ff.dirp != NULL);
    sprintf_s((char*)out, sizeofOut, "%s", ff.dp->d_name);
}



#endif //GOS_PLATFORM__LINUX