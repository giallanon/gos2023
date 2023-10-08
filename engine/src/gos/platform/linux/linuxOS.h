#ifdef GOS_PLATFORM__LINUX
#ifndef _linuxos_h_
#define _linuxos_h_
#include "linuxOSInclude.h"
#include "../../gosEnumAndDefine.h"

namespace gos
{
    class DateTime; //fwd decl
}

namespace platform
{
    bool            internal_init (const char *appName);
    void            internal_deinit ();

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
    inline void     mutexCreate (OSMutex *m)                            { pthread_mutex_init (m, NULL); }
    inline void     mutexDestroy (OSMutex *m)                           { pthread_mutex_destroy(m); }
    inline bool     mutexLock (OSMutex *m)                              { return (pthread_mutex_lock(m) == 0); }
    inline void     mutexUnlock (OSMutex *m)                            { pthread_mutex_unlock(m); }
    inline bool     mutexTryLock (OSMutex *m)                           { return (pthread_mutex_trylock(m) == 0); }

    /*eThreadError    createThread (OSThread &out_handle, OSThreadFunction threadFunction, size_t stackSizeInKb, void *userParam);
    void            killThread (OSThread &handle);
    void            waitThreadEnd(OSThread &handle);

	inline void     event_setInvalid(OSEvent &ev)													{ ev.evfd = -1; }
	inline bool		event_isInvalid(const OSEvent &ev)												{ return (ev.evfd == -1); }
    bool            event_open (OSEvent *out_ev);
	inline bool		event_compare (const OSEvent &a, const OSEvent &b)								{ return (a.evfd == b.evfd); }
    void            event_close (OSEvent &ev);
    void            event_fire (const OSEvent &ev);
    bool            event_wait (const OSEvent &ev, size_t timeoutMSec);
    */

                    //====================================== file system
    bool			FS_folderCreate (const u8 *utf8_path);
    bool			FS_folderDelete (const u8 *utf8_path);
    bool			FS_folderExists (const u8 *utf8_path);
    
    bool			FS_fileExists(const u8 *utf8_filename);
    bool			FS_fileDelete(const u8 *utf8_filename);
    bool			FS_fileRename(const u8 *utf8_path, const u8 *utf8_oldFilename, const u8 *utf8_newFilename);
    void            FS_fileGetCreationTime_UTC (const u8 *utf8_filePathAndName, gos::DateTime *out_dt);
    void            FS_fileGetCreationTime_UTC (const char *filePathAndName, gos::DateTime *out_dt);
    void            FS_fileGetLastTimeModified_UTC (const u8 *utf8_filePathAndName, gos::DateTime *out_dt);
    void            FS_fileGetLastTimeModified_UTC (const u8 *filePathAndName, gos::DateTime *out_dt);
    void            FS_fileGetCreationTime_LocalTime (const u8 *filePathAndName, gos::DateTime *out_dt);
    void            FS_fileGetLastTimeModified_LocalTime (const u8 *filePathAndName, gos::DateTime *out_dt);

    bool			FS_fileOpen  (OSFile *out_h, const u8 *utf8_filePathAndName, eFileMode mode, bool bCreateIfNotExists, bool bAppend, bool bShareRead, bool bShareWrite);
    u32				FS_fileRead (const OSFile &h, void *buffer, u32 numMaxBytesToRead);
    u32				FS_fileWrite (const OSFile &h, const void *buffer, u32 numBytesToWrite);
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
    /*

    void			FS_fileFlushAll();


    bool			FS_findFirst (OSFileFind *h, const u8 *utf8_path, const u8 *utf8_jolly);
    bool			FS_findNext (OSFileFind &h);
    bool			FS_findIsDirectory (const OSFileFind &ff);
    void			FS_findGetFileName (const OSFileFind &ff, u8 *out, u32 sizeofOut);
    const u8 *		FS_findGetFileName(const OSFileFind &ff);
    void			FS_findGetCreationTime (const OSFileFind &ff, gos::DateTime *out);
    void			FS_findGetLastTimeModified (const OSFileFind &ff, gos::DateTime *out);
    void			FS_findClose(OSFileFind &ff);

	bool			FS_findFirstHardDrive(OSDriveEnumerator *h, gosFindHardDriveResult *out);
	bool			FS_findNextHardDrive(OSDriveEnumerator &h, gosFindHardDriveResult *out);
	void			FS_findCloseHardDrive(OSDriveEnumerator &h);

	bool			FS_getDestkopPath(u8* out_path, u32 sizeof_out_path);
*/

}   //namespace platform

#endif //_linuxos_h_
#endif
