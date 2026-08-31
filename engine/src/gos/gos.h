#ifndef _gos_h_
#define _gos_h_
#include "platform/gosPlatform.h"
#include "gosEnumAndDefine.h"
#include "memory/gosMemory.h"
#include "logger/gosLoggerNull.h"
#include "logger/gosLoggerStdout.h"
#include "gosString.h"
#include "dataTypes/gosDateTime.h"
#include "gosRandom.h"
#include "gosHandle.h"
#include "helpers/gosErr.h"


//A per "num max di handle", B per "num di chunk", C per "counter"
GOS_DECL_HANDLE(1024,32, GOSThreadHandle);			//1024 => num totale di oggetti, divisi in chunk 32
GOS_DECL_HANDLE(1024,64, GOSThreadMsgHandle);		//1024 => num totale di oggetti, divisi in chunk 64

namespace gos
{
						//============== init/deinit ======================
	bool            	init (const sGOSInit &init, const char *appName);
	void				deinit();

	const char*			getAppName();

	//ritorna il path assoluto dell'applicazione, senza slash finale
    inline const char* 	getAppPathNoSlash ()					{ return platform::getAppPathNoSlash(); }
	u32					getLengthOfAppPathNoSlash ();

	//ritorna il path di una cartella nella quale è sicuramente possibile scrivere
	//Il path creato dipende dalle impostazioni di "writableFolder) dei paraemtri di sGOSInit
    const char*    		getPhysicalPathToWritableFolder();
	u32					getLengthOfPhysicalPathToWritableFolder();

    u64        			getTimeSinceStart_msec();
    u64 				getTimeSinceStart_usec();
	inline void         sleep_msec (size_t msec)				{ return platform::sleep_msec(msec); }
					
	//ritorna un num compreso tra 0 e 1 inclusi
	f32					random01();

	//ritorna un u32 compreso tra vMin e vMax inclusi
	f32					random (f32 vMin, f32 vMax);

	//ritorna un u32 compreso tra 0 e iMax incluso							
	u32					randomU32(u32 iMax);

	inline Allocator*	getSysHeapAllocator()					{ return mem::getSysHeapAllocator(); }
	inline Allocator*	getScrapAllocator()						{ return mem::getScrapAllocator(); }

	//esegue [cmdLine] e memorizza l'output in [out_result]
	//Ritorna false se [cmdLine] non esiste
	bool				runShellScriptAndStoreResult (const char *cmdLine, Allocator *allocator, char **out_result, u32 *out_resultLen);


	/************************************************************************************************************
	 *
	 * global error
	 * 
	 * Queste funzioni sono thread safe in quanto per ogni thread esiste una istanza specifica di err.
	 * Ogni thread quindi e' libero di utilizzare le fn di questo namespace sicuro che non andra' ad interferire
	 * con l'err di altri thread
	 *
	 */
	namespace err
	{
		void 			clear();
		void 			add (const char *format, ...);
		u32				anyError();
		const char*		getErrByIndex (u32 i);
	};


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
		Logger*	get_system_logger();
        void	inc_indent();
		void	dec_indent();

		//(DEFAULT) log con livello 5
        void	log (const char *format, ...);		void log (const eTextColor col, const char *format, ...);

		//log con altri livelli
		void	log_0 (const char *format, ...);	void log_0 (const eTextColor col, const char *format, ...);
		void	log_1 (const char *format, ...);	void log_1 (const eTextColor col, const char *format, ...);
		void	log_3 (const char *format, ...);	void log_3 (const eTextColor col, const char *format, ...);
		void	log_7 (const char *format, ...);	void log_7 (const eTextColor col, const char *format, ...);
		
		void	err (const char *format, ...);
				// err() setta anche il global error chiamando gos::err::add()

		void	verbose (const char *format, ...);
		void	warn (const char *format, ...);
        void	log_with_prefix (u8 level, const char *prefix, const char *format, ...);
        void	log_with_prefix (u8 level, const eTextColor col, const char *prefix, const char *format, ...);
	} //namespace logger



	/************************************************************************************************************
	 *
	 * fs
	 *
	 * Tutte le fn che accettano un path sono in grado di gestire alcuni casi speciali:
	 * 	- se il path inizia con @w, allora al posto di @w viene automaticamente inserito il path alla "writable folder"
	 * 		es: @w/pippo.txt, diventa [pathWritable]/pippo.txt
	 *  - se il path e' relativo (ovvero NON inizia con /), allora gli viene automaticamente prefisso il "path dell'app" piu' lo slash
	 * 		es: pippo/pluto.txt, diventa [pathApp]/pippo/pluto
	 */
	namespace fs
	{
		bool			priv_init ();
		void			priv_deinit ();

		
		//addAlias consente di aggiungere degli shortcut utilizzabili dai path delle fn di fs.
		//Gli shortcut iniziano sempre con @
		//Ad esempio:
		//	addAlias ("@pippo", "/usr/bin");
		//  implica che posso usare l'alias "@pippo" nelle chimate di fn
		//  Per aprire il file /usr/bin/pluto.txt posso usare fileOpen ("@pippo/pluto.txt")
		//  Per aprire il file /usr/bin/sottocartella/pluto.txt posso usare fileOpen ("@pippo/sottocartella/pluto.txt")
		//
		//Di default, gos aggiunge d'ufficio i senguenti alias:
		//	@w => punta alla directory "writable".  Quindi "@w/ciao.txt" punta a ".../writable/ciato.txt"
		//
		//	[aliasNoChioccola] e' l'alias da aggiungere
		//	[realPathNoSlash] e' il path reale, non deve terminare con /
		bool 			addAlias (const char *alias, const char *realPathNoSlash, eAliasPathMode mode, bool bReplaceAliasIfAlreadyExists = false);
		void			removeAlias (const char *alias);

						//path che comprendono degli alias, o che hanno dei percorsi relativi
						//vengono completamente espansi
		void 			resolvePath (const char *pathIN, char *out, u32 sizeof_out);

						//<origin_absFilename> deve essere un path assoluto mentre <rel_or_abs_path> puo' essere relativo (a origin)
						//o assoluto.
						//Ritorna in out il path assoluto di <rel_or_abs_path> relativamente a <origin_absFilename> gia'
						//sanitizzato
		void 			makeABSPathFromABSFilename (const char *origin_absFilename, const char *rel_or_abs_path, char *out, u32 sizeof_out);

		bool			isPathAbsolute (const char *path);
		void 			pathSanitize (const char *utf8_path, char *out_utf8sanitizedPath, u32 sizeOfOutSanitzed);
		void			pathSanitizeInPlace (char *utf8_path, u32 nBytesToCheck = u32MAX);
		void			pathGoBack (const char *pathSenzaSlashIN, char *out, u32 sizeof_out);
		bool			doesFileNameMatchJolly (const char *utf8_filename, const char *utf8_strJolly);

		void			extractFileExt (const char *utf8_filename, char *out, u32 sizeof_out);
		void			extractFileNameWithExt (const char *utf8_filename, char *out, u32 sizeof_out);
		void			extractFileNameWithoutExt (const char *utf8_filename, char *out, u32 sizeof_out);
		void			extractFilePathWithSlash (const char *utf8_filename, char *out, u32 sizeof_out);
		void			extractFilePathWithOutSlash (const char *utf8_filename, char *out, u32 sizeof_out);
		void			remove_ext_in_place (char *utf8_filename);

		bool			folderExists (const char *utf8_pathSenzaSlashRESOLVABLE);
		bool			folderDelete (const char *utf8_pathSenzaSlashRESOLVABLE);
		bool			folderCreate (const char *utf8_pathSenzaSlashRESOLVABLE);

		//crea anche percorsi complessi. Es create("pippo/pluto/paperino), se necessario
		//prima crea pippo, poi pippo/pluto e infine pippo/pluto/paperino
		bool			folderDeleteAllFileRecursively(const char *utf8_pathSenzaSlashRESOLVABLE, eFolderDeleteMode folderDeleteMode);
		void			folderDeleteAllFileWithJolly  (const char *utf8_pathSenzaSlashRESOLVABLE, const char *utf8_jolly);

		bool			fileExists (const char *utf8_filePathAndNameRESOLVABLE);
		bool   			fileDelete (const char *utf8_filePathAndNameRESOLVABLE);
		bool			fileRename (const char *utf8_pathNoSlashRESOLVABLE, const char *utf8_oldFilename, const char *utf8_newFilename);

		void     		fileGetCreationTime_UTC (const char *utf8_filePathAndNameRESOLVABLE, gos::DateTime *out_dt);
		void     		fileGetLastTimeModified_UTC (const char *utf8_filePathAndNameRESOLVABLE, gos::DateTime *out_dt);
		u64     		fileGetLastTimeModified_UTC_niceu64 (const char *utf8_filePathAndNameRESOLVABLE);
    	void     		fileGetCreationTime_LocalTime (const char *utf8_filePathAndNameRESOLVABLE, gos::DateTime *out_dt);
    	void     		fileGetLastTimeModified_LocalTime (const char *utf8_filePathAndNameRESOLVABLE, gos::DateTime *out_dt);

		bool			fileOpen  (gos::File *out_h,
									const char *utf8_filePathAndNameRESOLVABLE, 
									eFileMode mode, 
									bool bCreateIfNotExists,
									bool bAppend, 
									bool bShareRead,
									bool bShareWrite);

		inline bool		fileOpenForR (gos::File *out_h, const char *utf8_filePathAndNameRESOLVABLE)							{ return fs::fileOpen (out_h, utf8_filePathAndNameRESOLVABLE, eFileMode::readOnly, false, false, true, true); }
		bool			fileOpenForW (gos::File *out_h, const char *utf8_filePathAndNameRESOLVABLE, bool bAutoCreateFolders=false);
		bool			fileOpenForWriteAppend (gos::File *out_h, const char *utf8_filePathAndNameRESOLVABLE, bool bAutoCreateFolders=false);
		bool			fileOpenForRW  (gos::File *out_h, const char *utf8_filePathAndNameRESOLVABLE, bool bAutoCreateFolders=false);
		
		inline u32		fileRead (gos::File &h, void *buffer, u32 numMaxBytesToRead)										{ return platform::FS_fileRead(h.osFile, buffer, numMaxBytesToRead); }
		inline u32		fileReadU8 (gos::File &h, u8 *out)																	{ return fileRead (h, out, 1); }
			   u32		fileReadU16 (gos::File &h, u16 *out);
			   u32		fileReadU32 (gos::File &h, u32 *out);
			   u32		fileReadU64 (gos::File &h, u64 *out);

		inline u32		fileWrite (gos::File &h, const void *buffer, u32 numBytesToWrite)									{ return platform::FS_fileWrite(h.osFile, buffer, numBytesToWrite); }
		inline u32 		fileWriteU8 (gos::File &h, u8 data)																	{ return fileWrite (h, &data, 1); }
			   u32 		fileWriteU16 (gos::File &h, u16 data);
			   u32 		fileWriteU32 (gos::File &h, u32 data);
			   u32 		fileWriteU64 (gos::File &h, u64 data);
		
		void			fpf (gos::File &h, const char *format, ...);
		void			fpf_valist (gos::File &h, const char *format, va_list argptr);
		inline void		fileClose (gos::File  &h)																			{ platform::FS_fileClose(h.osFile); }
		inline void		fileFlush (gos::File &h) 																			{ platform::FS_fileFlush(h.osFile); }

		inline u64		fileLength (gos::File &h)																			{ return platform::FS_fileLength(h.osFile); }
		u64				fileLength (const char *utf8_filePathAndNameRESOLVABLE);
	    inline bool		fileSeek(gos::File &h, u64 position, eSeek seekMode)												{ return platform::FS_fileSeek(h.osFile, position, seekMode); }
    	inline u64		fileTell(gos::File &h) 																				{ return platform::FS_fileTell(h.osFile); }

		u8*				fileLoadInMemory (Allocator *allocator, const char* utf8_filePathAndNameRESOLVABLE, u32 *out_fileSize=NULL);
		bool 			fileSaveBuffer (const char* utf8_filePathAndNameRESOLVABLE, const void *buffer, u32 sizeof_buffer);
		bool			fileCopy (const char *src, const char *dst);

		bool			findFirst (gos::FileFind *ff, const char *utf8_pathRESOLVABLE, const char *utf8_jolly, eFileFindMode ffmode = eFileFindMode::both_file_and_folder);
		inline bool     findNext (gos::FileFind &ff)																		{ return platform::FS_findNext(ff.osFF); }
		inline void     findClose(gos::FileFind &ff)																		{ platform::FS_findClose(ff.osFF); }
		inline bool     findIsDirectory(const gos::FileFind &ff)															{ return platform::FS_findIsDirectory(ff.osFF); }
		inline const char* findGetFileName(const gos::FileFind &ff)															{ return platform::FS_findGetFileName(ff.osFF); }
		inline void     findGetFileName (const gos::FileFind &ff, char *out, u32 sizeofOut)									{ platform::FS_findGetFileName(ff.osFF, out, sizeofOut); }
		void 			findComposeFullFilePathAndName (const gos::FileFind &ff, const char *pathNoSlash, char *out, u32 sizeofOut);
		void 			findGetLastTimeModified_UTC (const gos::FileFind &ff, const char *pathNoSlash, gos::DateTime *out);
		u64 			findGetLastTimeModified_UTC_niceu64 (const gos::FileFind &ff, const char *pathNoSlash);
	} //namespace fs



	/************************************************************************************************************
	 *
	 * thread e syncronization
	 *
	 */
    namespace thread
    {
		//queste 2 macro sono solo degli alias. Mi piacciono perche' nel codice sono molto piu' evidenti rispetto ad usare la normale chiamata alla fn
		#define MUTEX_LOCK(mutex) 	gos::thread::mutex_lock(mutex); 
		#define MUTEX_UNLOCK(mutex) gos::thread::mutex_unlock(mutex); 

        inline void    mutex_create (gos::Mutex *m)      										{ platform::mutex_create (&m->osm); }
        inline void    mutex_destroy (gos::Mutex &m)     										{ platform::mutex_destroy (&m.osm); }
        inline bool    mutex_try_lock (gos::Mutex &m)     										{ return platform::mutex_try_lock (&m.osm); }
        inline bool    mutex_lock (gos::Mutex &m)        										{ return platform::mutex_lock (&m.osm); }
        inline void    mutex_unlock (gos::Mutex &m)      										{ platform::mutex_unlock (&m.osm); }

        
		inline bool     signal_create (gos::Signal *out_ev)										{ return platform::signal_create (&out_ev->osEvt); }
		inline void     signal_destroy (gos::Signal &ev)											{ platform::signal_destroy (ev.osEvt); }
		inline bool		signal_compare (const gos::Signal &a, const gos::Signal &b)				{ return platform::signal_compare(a.osEvt, b.osEvt); }
		inline void     signal_fire (const gos::Signal &ev)										{ platform::signal_fire (ev.osEvt); }
		inline bool     signal_wait (const gos::Signal &ev, u32 timeoutMSec)						{ return platform::signal_wait (ev.osEvt, timeoutMSec); }
		inline void     signal_set_invalid (gos::Signal &ev)										{ platform::signal_set_invalid (ev.osEvt); }
		inline bool		signal_is_invalid (const gos::Signal &ev)								{ return platform::signal_is_invalid (ev.osEvt); }
		inline bool		signal_is_valid (const gos::Signal &ev)									{ return !platform::signal_is_invalid (ev.osEvt); }

        eThreadError    create (GOSThreadHandle *out_hThread, GOS_ThreadMainFunction threadFunction, void *userParam, u16 stackSizeInKb=2048);
        void            wait_end (GOSThreadHandle &hThread);
		inline u32		get_current_threadID()													{ return platform::get_current_threadID(); }

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

		inline void		trap_CTRL_C (GOS_ConsoleTrap_CTRL_C trapFn, void *userParam)					{ platform::console_trap_CTRL_C (trapFn, userParam); }
							//nelle applicazioni console, questo handler viene invocato quando l'utente preme CTRL C

		void			setWH (u16 w, u16 h);	//imposta width e height della finestra. Questa potrebbe non funzionare, il supporto a questa fn dipende dall'OS
		void			setWidthTo80Cols();		//Questa e la successiva (132col) sono uno standard che funziona su ogni terminale, a differenza della
		void			setWidthTo132Cols();	//precedente setWH() il cui supporto è ballerino

		inline void		setTitle (const char *title)													{ platform::console_setTitle (title); }
		eTextColor		setTextColor (const eTextColor c);
		eBgColor		setBgColor (const eBgColor c);

		void			clear();
		void			clearLine();				//cancella la riga dove e' attualmente posizionato il cursore
		void			clearEndOfLine();			//cancella a partire dall'attuale posizione del cursore fino a fine riga
		void			clearStartOfLine();			//cancella a partire dall'attuale posizione del cursore fino a inizio riga
		void			clearDown();				//cancella a partire dall'attuale riga fino a fondo schermo
		void			clearUp();					//cancella a partire dall'attuale riga fino a inizio schermo
		
		void			cursorMove (u16 x, u16 y);
		void			cursorMoveX (u16 x);
		void			cursorMoveY (u16 y);

		void 			setScrollingArea (u16 rowStart, u16 rowEnd);

	} //namespace console


	/************************************************************************************************************
	 *
	 * network & socket
	 *
	 */
	namespace netaddr
	{
		//alloca un array di [sNetworkAdapterInfo] e lo ritorna.
		//L'array contiene le info su nome, IP e netmask degli adattatori di rete disponibili al sistema
		inline  NetworkAdapterInfo*	getListOfAllNerworkAdpaterIPAndNetmask (gos::Allocator *allocator, u32 *out_numFound)		{ return platform::NET_getListOfAllNerworkAdpaterIPAndNetmask(allocator, out_numFound); }

		void				ipstrToIPv4  (const char *ip, IPv4 *out);

		//================================== MAC ADDRESS
		void				setInvalid (MacAddress &me);
		bool				isValid (const MacAddress &me);
		bool				isInvalid (const MacAddress &me);

		//scanna tutte le interfacce di rete esistenti nel PC e ritorna il MAC e l'indirizzo IP dell'interfaccia
		//in questione. Predilige interfacce LAN rispetto a WIFI (nel caso esistano entrambe)
		inline bool			findMACAddress (MacAddress *outMAC, IPv4 *outIP)														{ return platform::NET_getMACAddress(outMAC, outIP); }

		//se [bStringHasSeparator]==true, allora [macString] è nel formato AA:BB:CC:DD:EE:FF 
		//altrimenti è nel formato AABBCCDDEEFF 
		bool				setFromMACString (MacAddress &me, const char *macString, bool bStringHasSeparator);

		//ritorna il MAC address di eth0 se esiste.
		//[out_macAddress] deve essere di almeno 16 char
		bool				getMACAddressAsString (const MacAddress &mac, char *out_macAddress, u32 sizeOfMacAddress, char optionalSeparator=0x00);

		i8					compare (const MacAddress &m1, const MacAddress &m2);

		//=============================== NETWORK ADDRESS
		void				setInvalid (NetAddr &me);
		bool				isValid (const NetAddr &me);
		bool				isInvalid (const NetAddr &me);
		void				setFromSockAddr (NetAddr &me, const sockaddr_in &addrIN);
		void				setFromAddr (NetAddr &me, const NetAddr &addrIN);
		void				setIPv4 (NetAddr &me, const char* constip);
		void				setIPv4 (NetAddr &me, const IPv4 &ip);
		void				setPort (NetAddr &me, u16 port);
		
		bool				compare (const NetAddr &a, const NetAddr &b);

		u8					serializeToBuffer (const NetAddr &me, u8 *dst, u32 sizeof_dst, bool bIncludePort);
								//se [bIncludePort] == false, serializza solo l'indirizzo IP, altrimenti IP e porta
		u8					deserializeFromBuffer (NetAddr &me, const u8 *src, u32 sizeof_src, bool bIncludePort);
		
		void				getIPv4 (const NetAddr &me, char *out, u32 sizeof_out);
		void				getIPv4 (const NetAddr &me, IPv4 *out);
		u16					getPort (const NetAddr &me);
        sockaddr*           getSockAddr (const NetAddr &me);
		int					getSockAddrLen (const NetAddr &me);
	} //namespace netaddr


	namespace socket
	{
		inline void					init (Socket *sok)																		{ platform::socket_init(&sok->osSok); }

		//=============================================== TCP
		inline eSocketError         openAsTCPServer(Socket *out_sok, int portNumber)										{ return platform::socket_openAsTCPServer(&out_sok->osSok, portNumber); }
		inline eSocketError         openAsTCPClient(Socket *out_sok, const char* connectToIP, u32 portNumber)				{ return platform::socket_openAsTCPClient(&out_sok->osSok, connectToIP, portNumber); }
		eSocketError				openAsTCPClient(Socket *out_sok, const NetAddr &ipAndPort);

		inline void                 close(Socket &sok)																		{ platform::socket_close(sok.osSok); }

		/* false se la socket non è open.
		 * False anche a seguito di una chiamata a close() (in quanto la sok viene chiusa)
		 */
		inline bool                 isOpen(const Socket &sok)																{ return platform::socket_isOpen(sok.osSok); }

		/* true se "puntano" alla stessa socket
		*/
		inline bool                 compare(const Socket &a, const Socket &b)												{ return platform::socket_compare(a.osSok, b.osSok); }


		/* Per specificare un tempo di wait "infinito" (ie: socket sempre bloccante), usare timeoutMSec=u32MAX
		 * Per indicare il tempo di wait minimo possibile, usare timeoutMSec=0
		 * Tutti gli altri valori sono comunque validi ma non assumono significati particolari
		 */
		inline bool                 setReadTimeoutMSec(Socket &sok, u32 timeoutMSec)										{ return platform::socket_setReadTimeoutMSec(sok.osSok, timeoutMSec); }

		/* Per specificare un tempo di wait "infinito" (ie: socket sempre bloccante), usare timeoutMSec=u32MAX
		 * Per indicare il tempo di wait minimo possibile, usare timeoutMSec=0
		 * Tutti gli altri valori sono comunque validi ma non assumono significati particolari
		 */
		inline bool                 setWriteTimeoutMSec(Socket &sok, u32 timeoutMSec)										{ return platform::socket_setWriteTimeoutMSec(sok.osSok, timeoutMSec); }


		inline bool					listen(const Socket &sok, u16 maxIncomingConnectionQueueLength = u16MAX)				{ return platform::socket_listen(sok.osSok, maxIncomingConnectionQueueLength); }
		inline bool					accept(const Socket &sok, Socket *out_clientSocket)										{ return platform::socket_accept(sok.osSok, &out_clientSocket->osSok); }

		/* prova a leggere dalla socket. La chiamata è bloccante per un massimo di timeoutMSec.
		 * Riguardo [timeoutMSec], valgono le stesse considerazioni indicate in setReadTimeoutMSec()
		 *
		 * Ritorna:
		 *      0   se la socket si è disconnessa
		 *      -1  se la chiamata avrebbe bloccato il processo (quindi devi ripetere la chiamata fra un po')
		 *      >0  se ha letto qualcosa e ha quindi fillato [buffer] con il num di bytes ritornato
		 */
		inline i32					read(Socket &sok, void *buffer, u16 bufferSizeInBytes, u32 timeoutMSec, bool bPeekMsg=false)	{ return platform::socket_read(sok.osSok, buffer, bufferSizeInBytes, timeoutMSec, bPeekMsg); }

		/*	Ritorna il numero di btye scritti sulla socket.
		 *	Se ritorna 0, vuol dire che la chiamata sarebbe stata bloccante e quindi l'ha evitata
		 */
		inline i32                  write (Socket &sok, const void *buffer, u16 nByteToSend)								{ return platform::socket_write(sok.osSok, buffer, nByteToSend); }

		 //=============================================== UDP
		inline	eSocketError		openAsUDP (Socket *out_sok)																{ return platform::socket_openAsUDP(&out_sok->osSok); }
		inline	eSocketError		UDPbind (Socket &sok, int portNumber)													{ return platform::socket_UDPbind(sok.osSok, portNumber); }
		inline	u32					UDPSendTo (Socket &sok, const u8 *buffer, u32 nByteToSend, const NetAddr &addrTo)		{ return platform::socket_UDPSendTo(sok.osSok, buffer, nByteToSend, addrTo); }
		inline	u32					UDPReceiveFrom (Socket &sok, u8 *buffer, u32 nMaxBytesToRead, NetAddr *out_from)		{ return platform::socket_UDPReceiveFrom(sok.osSok, buffer, nMaxBytesToRead, out_from); }
		void						UDPSendBroadcast (Socket &sok, const u8 *buffer, u32 nByteToSend, const char *ip, int portNumber, const char *subnetMask);
	}
	

} //namespace gos

#endif //_gos_h_