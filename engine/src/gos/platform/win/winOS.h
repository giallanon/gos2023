#ifdef GOS_PLATFORM__WINDOWS
#ifndef _winos_h_
#define _winos_h_
#include "winOSInclude.h"
#include "../../gosEnumAndDefine.h"

namespace gos
{
    class DateTime; //fwd decl
}

namespace platform
{
	bool            internal_init (const char *appName);
	void            internal_deinit();

    u32             systeminfo_getNumOfCPUCore();
    u32             memory_getPageSizeInByte();
    void*           memory_alignedAlloc (size_t size, size_t alignmentPowerOfTwo);
    void            memory_alignedFree (void *p);

    const u8*       getAppPathNoSlash ();
    const u8*       getPhysicalPathToWritableFolder();

    void            sleep_msec (size_t msec);

    u64             getTimeNow_usec();
    void            getDateNow (u16 *out_year, u16 *out_month, u16 *out_day);
    void            getTimeNow (u8 *out_hour, u8 *out_min, u8 *out_sec);


    /******************************************************
     * console stuff
     * 
     */
	bool            console_internal_init();
	void			console_internal_deinit();

	void			console_trap_CTRL_C (ConsoleTrap_CTRL_C trapFn, void *userParam);
	void			console_setTitle (const char *title);


    /******************************************************
     * thread stuff
     * 
     */
    inline void     mutexCreate (OSMutex *m)                            { ::InitializeCriticalSection(&m->cs); }
    inline void     mutexDestroy (OSMutex *m)                           { ::DeleteCriticalSection(&m->cs); }
    inline bool     mutexLock (OSMutex *m)                              { ::EnterCriticalSection(&m->cs); return true; }
    inline void     mutexUnlock (OSMutex *m)                            { ::LeaveCriticalSection(&m->cs); }
    inline bool     mutexTryLock (OSMutex *m)                           { return (TryEnterCriticalSection(&m->cs) == TRUE); }

				
    /******************************************************
     * File system
     * 
     */
    bool			FS_folderCreate (const u8 *utf8_path);
    bool			FS_folderDelete (const u8 *utf8_path);
    bool			FS_folderExists (const u8 *utf8_path);
    
    bool			FS_fileExists(const u8 *utf8_filename);
    bool			FS_fileDelete(const u8 *utf8_filename);
    bool			FS_fileRename(const u8 *utf8_path, const u8 *utf8_oldFilename, const u8 *utf8_newFilename);
    
    void            FS_fileGetCreationTime_UTC (const u8 *utf8_filePathAndName, gos::DateTime *out_dt);
    void            FS_fileGetCreationTime_UTC (const char *filePathAndName, gos::DateTime *out_dt);
    
    void            FS_fileGetLastTimeModified_UTC (const u8 *utf8_filePathAndName, gos::DateTime *out_dt);
    void            FS_fileGetLastTimeModified_UTC (const char *filePathAndName, gos::DateTime *out_dt);
    
    void            FS_fileGetCreationTime_LocalTime (const u8 *utf8_filePathAndName, gos::DateTime *out_dt);
    void            FS_fileGetCreationTime_LocalTime (const char *filePathAndName, gos::DateTime *out_dt);

    void            FS_fileGetLastTimeModified_LocalTime (const u8 *utf8_filePathAndName, gos::DateTime *out_dt);
    void            FS_fileGetLastTimeModified_LocalTime (const char *filePathAndName, gos::DateTime *out_dt);

    bool			FS_fileOpen  (OSFile *out_h, const u8 *utf8_filePathAndName, eFileMode mode, bool bCreateIfNotExists, bool bAppend, bool bShareRead, bool bShareWrite);
    u32				FS_fileRead (OSFile &h, void *buffer, u32 numMaxBytesToRead);
    u32				FS_fileWrite (OSFile &h, const void *buffer, u32 numBytesToWrite);
    void			FS_fileClose (OSFile &h);
    void            FS_fileFlush (OSFile &h);

    u64				FS_fileLength (OSFile &h);
    u64				FS_fileLength (const u8 *utf8_filePathAndName);
    void			FS_fileSeek(OSFile &h, u64 position, eSeek seekMode);
    u64				FS_fileTell(OSFile &h);

    bool            FS_findFirst (OSFileFind *ff, const u8 *utf8_path, const u8 *utf8_jolly);
    bool            FS_findNext (OSFileFind &ff);
    void            FS_findClose(OSFileFind &ff);
    bool            FS_findIsDirectory(const OSFileFind &ff);
    const u8*       FS_findGetFileName(const OSFileFind &ff);
    void            FS_findGetFileName (const OSFileFind &ff, u8 *out, u32 sizeofOut);

}   //namespace platform


#endif //_winos_h_
#endif //GOS_PLATFORM__WINDOWS