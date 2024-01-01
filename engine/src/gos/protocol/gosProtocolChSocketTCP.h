#ifndef _gosProtocolSocketIO_h_
#define _gosProtocolSocketIO_h_
#include "gosIProtocolChannel.h"
#include "../gos.h"

namespace gos
{
	//Attiva questa define per dumpare su file tutto il traffico lungo in canale (viene creato un file diverso per ogni istanza di canale)
	#undef DUMP_CProtocolChSocketTCP_TO_FILE

	//se non siamo in DEBUG, disabilito il DUMP d'ufficio
#ifndef _DEBUG
	#ifdef DUMP_CProtocolChSocketTCP_TO_FILE
		#undef DUMP_CProtocolChSocketTCP_TO_FILE
	#endif
#endif

	/************************************************
	 * ProtocolSocketTCP
	 * Implementa un canale di comunicazione basato su socket TCP
	 *
	 * Per i dettagli, vedi IProtocolChannel.h
	 */
	class ProtocolChSocketTCP : public IProtocolChannel
	{
	public:
							ProtocolChSocketTCP (Allocator *allocatorIN, u32 startingSizeOfReadBufferInBytes, u32 maxSizeOfReadBufferInBytes);

		void				bindSocket (gos::Socket &sokIN)													{ sok = sokIN; }
		gos::Socket&        getSocket ()																	{ return sok; }

	protected:
		bool				virt_isOpen () const															{ return gos::socket::isOpen(sok); }
		void				virt_close ()																	{ gos::socket::close(sok); }
		u32					virt_read (u8 *buffer, u32 nMaxBytesToRead, u32 timeoutMSec);
		u32					virt_write (const u8 *bufferToWrite, u32 nBytesToWrite);

	private:
		gos::Socket			sok;
	};
} // namespace gos
#endif // _gosProtocolSocketIO_h_
