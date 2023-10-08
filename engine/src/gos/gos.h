#ifndef _gos_h_
#define _gos_h_
#include "platform/gosPlatform.h"
#include "gosEnumAndDefine.h"
#include "memory/gosMemory.h"
#include "gosString.h"
#include "dataTypes/gosDateTime.h"
#include "gosRandom.h"


namespace gos
{
						//============== init/deinit ======================
	bool            	init (const sGOSInit &init, const char *appName);
	void				deinit();

	const char*			getAppName();
    inline const u8* 	getAppPathNoSlash ()					{ return platform::getAppPathNoSlash(); }
						//ritorna il path assoluto dell'applicazione, senza slash finale

    inline const u8*    getPhysicalPathToWritableFolder()		{ return platform::getPhysicalPathToWritableFolder(); }
						//ritorna il path di una cartella nella quale è sicuramente possibile scrivere
						//Sui sistemi windows per es, ritorna una cosa del tipo "C:\Users\NOME_UTENTE".
						//Sui sistemi linux, ritorna generalmente lo stesso path dell'applicazione

    u64        			getTimeSinceStart_msec();
    u64 				getTimeSinceStart_usec();
	inline void         sleep_msec (size_t msec)				{ return platform::sleep_msec(msec); }
					
	f32					random01();
							//ritorna un num compreso tra 0 e 1 inclusi
	u32					randomU32(u32 iMax);
							//ritorna un u32 compreso tra 0 e iMax incluso

	inline Allocator*	getSysHeapAllocator()					{ return mem::getSysHeapAllocator(); }
	inline Allocator*	getScrapAllocator()						{ return mem::getScrapAllocator(); }


	/************************************************************************************************************
	 *
	 * systeminfo
	 *
	 */
	namespace systeminfo
	{
		inline u32			getNumOfCPUCore()					{ return platform::systeminfo_getNumOfCPUCore(); }
		inline u32 			getPageSizeInByte() 				{ return platform::memory_getPageSizeInByte(); }
	} //namespace systeminfo


	/************************************************************************************************************
	 *
	 * logger
	 *
	 */
	namespace logger
	{
        void	incIndent();
		void	decIndent();

        void	log (const char *format, ...);
        void	log (const eTextColor col, const char *format, ...);
		void	err (const char *format, ...);
        void	logWithPrefix (const char *prefix, const char *format, ...);
        void	logWithPrefix (const eTextColor col, const char *prefix, const char *format, ...);
	} //namespace logger



	/************************************************************************************************************
	 *
	 * fs
	 *
	 */
	namespace fs
	{
		void 			pathSanitize (const u8 *utf8_path, u8 *out_utf8sanitizedPath, u32 sizeOfOutSanitzed);
		void			pathSanitizeInPlace (u8 *utf8_path, u32 nBytesToCheck = u32MAX);
		void			pathGoBack (const u8 *pathSenzaSlashIN, u8 *out, u32 sizeofout);
		bool			doesFileNameMatchJolly (const u8 *utf8_filename, const u8 *utf8_strJolly);

		void			extractFileExt (const u8 *utf8_filename, u8 *out, u32 sizeofout);
		void			extractFileNameWithExt (const u8 *utf8_filename, u8 *out, u32 sizeofout);
		void			extractFileNameWithoutExt (const u8 *utf8_filename, u8 *out, u32 sizeofout);
		void			extractFilePathWithSlash (const u8 *utf8_filename, u8 *out, u32 sizeofout);
		void			extractFilePathWithOutSlash (const u8 *utf8_filename, u8 *out, u32 sizeofout);

		inline bool		folderExists (const u8 *pathSenzaSlash)																{ return platform::FS_folderExists(pathSenzaSlash); }
		inline bool		folderDelete (const u8 *pathSenzaSlash)																{ return platform::FS_folderDelete(pathSenzaSlash); }
		inline bool		folderCreate (const u8 *pathSenzaSlash)																{ return platform::FS_folderCreate(pathSenzaSlash); }
								//crea anche percorsi complessi. Es create("pippo/pluto/paperino), se necessario
								//prima crea pippo, poi pippo/pluto e infine pippo/pluto/paperino
		bool			folderDeleteAllFileRecursively(const u8 *pathSenzaSlash, eFolderDeleteMode folderDeleteMode);
		void			folderDeleteAllFileWithJolly  (const u8 *pathSenzaSlash, const u8 *utf8_jolly);
		inline void		folderDeleteAllFileWithJolly  (const u8 *pathSenzaSlash, const char *jolly)							{ folderDeleteAllFileWithJolly  (pathSenzaSlash, reinterpret_cast<const u8*>(jolly)); }

		inline bool		fileExists(const u8 *utf8_fullFileNameAndPath)														{ return platform::FS_fileExists(utf8_fullFileNameAndPath); }
		inline bool   	fileDelete(const u8 *utf8_fullFileNameAndPath)														{ return platform::FS_fileDelete(utf8_fullFileNameAndPath); }
		inline bool		fileRename(const u8 *utf8_pathNoSlash, const u8 *utf8_oldFilename, const u8 *utf8_newFilename)		{ return platform::FS_fileRename(utf8_pathNoSlash, utf8_oldFilename, utf8_newFilename); }
		inline void     fileGetCreationTime_UTC(const u8 *utf8_pathNoSlash, gos::DateTime *out_dt)							{ platform::FS_fileGetCreationTime_UTC(utf8_pathNoSlash, out_dt); }
		inline void     fileGetLastTimeModified_UTC(const u8 *utf8_pathNoSlash, gos::DateTime *out_dt)						{ platform::FS_fileGetLastTimeModified_UTC(utf8_pathNoSlash, out_dt); }
    	inline void     fileGetCreationTime_LocalTime (const u8 *utf8_filePathAndName, gos::DateTime *out_dt)				{ platform::FS_fileGetCreationTime_LocalTime (utf8_filePathAndName, out_dt); }
    	inline void     fileGetLastTimeModified_LocalTime (const u8 *utf8_filePathAndName, gos::DateTime *out_dt)			{ platform::FS_fileGetLastTimeModified_LocalTime (utf8_filePathAndName, out_dt); }


		inline bool		fileOpen  (gos::File *out_h, const u8 *utf8_filePathAndName, eFileMode mode, bool bCreateIfNotExists, bool bAppend, bool bShareRead, bool bShareWrite)		{ return platform::FS_fileOpen (&out_h->osFile, utf8_filePathAndName, mode, bCreateIfNotExists, bAppend, bShareRead, bShareWrite); }
		inline bool		fileOpenForR (gos::File *out_h, const u8 *utf8_filePathAndName)										{ return fs::fileOpen (out_h, utf8_filePathAndName, eFileMode::readOnly, false, false, true, true); }
		bool			fileOpenForW (gos::File *out_h, const u8 *utf8_filePathAndName, bool bAutoCreateFolders=false);
		bool			fileOpenForAppend (gos::File *out_h, const u8 *utf8_filePathAndName, bool bAutoCreateFolders=false);
		inline u32		fileRead (const gos::File &h, void *buffer, u32 numMaxBytesToRead)									{ return platform::FS_fileRead(h.osFile, buffer, numMaxBytesToRead); }
		inline u32		fileWrite (const gos::File &h, const void *buffer, u32 numBytesToWrite)								{ return platform::FS_fileWrite(h.osFile, buffer, numBytesToWrite); }
		void			fpf (const gos::File &h, const char *format, ...);
		inline void		fileClose (gos::File  &h)																			{ platform::FS_fileClose(h.osFile); }
		inline void		fileFlush (gos::File &h) 																			{ platform::FS_fileFlush(h.osFile); }

		inline u64		fileLength (gos::File  &h)																			{ return platform::FS_fileLength(h.osFile); }
		inline u64		fileLength (const u8 *utf8_filePathAndName)															{ return platform::FS_fileLength(utf8_filePathAndName); }
		inline u64		fileLength (const char *filePathAndName)															{ return platform::FS_fileLength(reinterpret_cast<const u8*>(filePathAndName)); }
	    inline void		fileSeek(gos::File &h, u64 position, eSeek seekMode)												{ platform::FS_fileSeek(h.osFile, position, seekMode); }
    	inline u64		fileTell(gos::File &h) 																				{ return platform::FS_fileTell(h.osFile); }

		u8*				fileLoadInMemory (Allocator *allocator, const u8* filePathAndName, u32 *out_fileSize=NULL);

		inline bool			findFirst (gos::FileFind *ff, const u8 *utf8_path, const u8 *utf8_jolly)						{ return platform::FS_findFirst (&ff->osFF, utf8_path, utf8_jolly); }
		inline bool			findFirst (gos::FileFind *ff, const u8 *utf8_path, const char *jolly)							{ return platform::FS_findFirst (&ff->osFF, utf8_path, reinterpret_cast<const u8*>(jolly)); }
		inline bool         findNext (gos::FileFind &ff)																	{ return platform::FS_findNext(ff.osFF); }
		inline void         findClose(gos::FileFind &ff)																	{ platform::FS_findClose(ff.osFF); }
		inline bool         findIsDirectory(const gos::FileFind &ff)														{ return platform::FS_findIsDirectory(ff.osFF); }
		inline const u8*	findGetFileName(const gos::FileFind &ff)														{ return platform::FS_findGetFileName(ff.osFF); }
		inline void         findGetFileName (const gos::FileFind &ff, u8 *out, u32 sizeofOut)								{ platform::FS_findGetFileName(ff.osFF, out, sizeofOut); }
		void 				findComposeFullFilePathAndName(const gos::FileFind &ff, const u8 *pathNoSlash, u8 *out, u32 sizeofOut);
	} //namespace fs



	/************************************************************************************************************
	 *
	 * thread e syncronization
	 *
	 */
    namespace thread
    {
        inline void    mutexCreate (gos::Mutex &m)      { platform::mutexCreate(&m.osm); }
        inline void    mutexDestroy (gos::Mutex &m)     { platform::mutexDestroy(&m.osm); }
        inline bool    mutexTryLock (gos::Mutex &m)     { return platform::mutexTryLock(&m.osm); }
        inline bool    mutexLock (gos::Mutex &m)        { return platform::mutexLock(&m.osm); }
        inline void    mutexUnlock (gos::Mutex &m)      { platform::mutexUnlock(&m.osm); }
        
    } // namespace thread



	/************************************************************************************************************
	 *
	 * console
	 *
	 *	Utility per la programmazione di applicazioni console
	 */
	namespace console
	{
		bool			priv_init ();
		void			priv_deinit ();

		inline void		trap_CTRL_C (ConsoleTrap_CTRL_C trapFn, void *userParam)						{ platform::console_trap_CTRL_C (trapFn, userParam); }
							//nelle applicazioni console, questo handler viene invocato quando l'utente preme CTRL C

		void			setWH (u16 w, u16 h);	//imposta width e height della finestra. Questa potrebbe non funzionare, il supporto a questa fn dipende dall'OS
		void			setWidthTo80Cols();		//Questa e la successiva (132col) sono uno standard che funziona su ogni terminale, a differenza della
		void			setWidthTo132Cols();	//precedente setWH() il cui supporto è ballerino

		inline void		setTitle (const char *title)													{ platform::console_setTitle (title); }
		eTextColor		setTextColor (const eTextColor c);
		eBgColor		setBgColor (const eBgColor c);

		void			clear();
		void			clearLine();			//cancella la riga dove e' attualmente posizionato il cursore
		void			cursorMove (u16 x, u16 y);
		void			cursorMoveX (u16 x);
		void			cursorMoveY (u16 y);

	} //namespace console


} //namespace gos

#endif //_gos_h_