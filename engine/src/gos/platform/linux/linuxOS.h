#ifdef GOS_PLATFORM__LINUX
#ifndef _linuxos_h_
#define _linuxos_h_
#include "linuxOSInclude.h"
#include "../../gosEnumAndDefine.h"

namespace gos
{
    class DateTime; //fwd decl
    class Allocator; //fwd decl
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
    const u8*       getPhysicalPathToUserFolder();

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

	void			console_trap_CTRL_C (GOS_ConsoleTrap_CTRL_C trapFn, void *userParam);
	void			console_setTitle (const char *title);


    /******************************************************
     * thread stuff
     * 
     */
    inline void     mutexCreate (OSMutex *m)                                            { pthread_mutex_init (m, NULL); }
    inline void     mutexDestroy (OSMutex *m)                                           { pthread_mutex_destroy(m); }
    inline bool     mutexLock (OSMutex *m)                                              { return (pthread_mutex_lock(m) == 0); }
    inline void     mutexUnlock (OSMutex *m)                                            { pthread_mutex_unlock(m); }
    inline bool     mutexTryLock (OSMutex *m)                                           { return (pthread_mutex_trylock(m) == 0); }

    bool            eventCreate (OSEvent *out_ev);
    void            eventDestroy (OSEvent &ev);
    void            eventFire (const OSEvent &ev);
    bool            eventWait (const OSEvent &ev, size_t timeoutMSec);
	inline void     eventSetInvalid (OSEvent &ev)										{ ev.evfd = -1; }
	inline bool		eventIsInvalid (const OSEvent &ev)									{ return (ev.evfd == -1); }
	inline bool		eventCompare (const OSEvent &a, const OSEvent &b)					{ return (a.evfd == b.evfd); }

    eThreadError    createThread  (OSThread *out_handle, GOS_ThreadMainFunction threadFunction, u32 stackSizeInKb, void *userParam);
    void            waitThreadEnd (OSThread &handle);
    

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


    /******************************************************
     * Network & Socket
     * 
     */
    void				socket_init (OSSocket *sok);

    //=============================== TCP
    eSocketError        socket_openAsTCPServer (OSSocket *out_sok, int portNumber);
    eSocketError        socket_openAsTCPClient (OSSocket *out_sok, const char *connectToIP, u32 portNumber);

    void                socket_close (OSSocket &sok);

    inline bool         socket_isOpen (const OSSocket &sok)                                                   { return (sok.socketID > 0); }

    inline bool         socket_compare (const OSSocket &a, const OSSocket &b)                                 { return (a.socketID == b.socketID); }

    bool                socket_setReadTimeoutMSec  (OSSocket &sok, u32 timeoutMSec);

    bool                socket_setWriteTimeoutMSec (OSSocket &sok, u32 timeoutMSec);

    bool                socket_listen (const OSSocket &sok, u16 maxIncomingConnectionQueueLength = u16MAX);
    bool                socket_accept (const OSSocket &sok, OSSocket *out_clientSocket);

    i32                 socket_read (OSSocket &sok, void *buffer, u16 bufferSizeInBytes, u32 timeoutMSec, bool bPeekMS);

    i32                 socket_write(OSSocket &sok, const void *buffer, u16 nByteToSend);

    //=============================== UDP
    eSocketError        socket_openAsUDP (OSSocket *out_sok);
    eSocketError        socket_UDPbind (OSSocket &sok, int portNumber);
    u32					socket_UDPSendTo (OSSocket &sok, const u8 *buffer, u32 nByteToSend, const gos::NetAddr &addrTo);
    u32					socket_UDPReceiveFrom (OSSocket &sok, u8 *buffer, u32 nMaxBytesToRead, gos::NetAddr *out_addrFrom);

    //====================================== networking
	gos::NetworkAdapterInfo*    NET_getListOfAllNerworkAdpaterIPAndNetmask (gos::Allocator *allocator, u32 *out_numFound);
    bool					    NET_getMACAddress (gos::MacAddress *outMAC, gos::IPv4 *outIP);

}   //namespace platform

#endif //_linuxos_h_
#endif
