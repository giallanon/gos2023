#ifndef _gosProtocolConsole_h_
#define _gosProtocolConsole_h_
#include "gosIProtocol.h"


namespace gos
{
    /*************************************************++
     * ProtocolConsole
     *
     */
    class ProtocolConsole : public IProtocol
    {
    public:
        /* il server si aspetta che il client inizi la connessione con uno specifico handshake.
         * Se il primi nByte di bufferIN corrispondono ad un valido handshake iniziato dal client con la handshake_clientSend()
         * allora questa fn ritorna true */
        static bool          server_isAValidHandshake (const void *bufferIN, u32 sizeOfBuffer);

    public:
                            ProtocolConsole (Allocator *allocatorIN, u32 startingSizeOfWriteBuffer) : IProtocol(allocatorIN, startingSizeOfWriteBuffer)		{ }
		virtual             ~ProtocolConsole()																										        { }

        /* invia l'handshake al server e aspetta la risposta.
         * Ritorna true se tutto ok, false altrimenti*/
		bool				handshake_clientSend (IProtocolChannel *ch, gos::Logger *logger = NULL);

		bool				handshake_serverAnswer(IProtocolChannel *ch, gos::Logger *logger = NULL);

	protected:
		void				virt_sendCloseMessage (IProtocolChannel *ch);
		u32					virt_decodeBuffer (IProtocolChannel *ch, const u8 *bufferIN, u32 nBytesInBufferIN, ProtocolBuffer &out_result, u32 *out_nBytesInseritiInOutResult);
		u32					virt_encodeBuffer (const u8 *bufferToEncode, u32 nBytesToEncode, ProtocolBuffer &out_buffer);


    private:
        static const u8     MAGIC_CODE_1 = 0x72;
        static const u8     MAGIC_CODE_2 = 0xC1;

    private:
        enum class eOpcode : u8
        {
            msg						= 0x01,
            close					= 0x02,
			internal_simpleMsg		= 0xfd,
			internal_extendedMsg	= 0xfe,
            none					= 0xff
        };

        struct sDecodeResult
        {
            eOpcode             what;
            const u8            *payload;
            u16                 payloadLen;
        };

	private:
		u32					priv_decodeOneMessage(const u8 *buffer, u32 nBytesInBuffer, sDecodeResult *out_result) const;
		u32                 priv_encodeAMessage(eOpcode opcode, const void *payloadToSend, u32 payloadLen, u8 *out_buffer, u32 sizeOfOutBuffer);

    };

} //namespace gos
#endif // _gosProtocolConsole_h_
