#ifndef _gosProtocolWebsocket_h_
#define _gosProtocolWebsocket_h_
#include "gosIProtocol.h"

namespace gos
{
    /*************************************************++
     * ProtocolWebsocket
     *
     */
    class ProtocolWebsocket : public IProtocol
    {
    public:
        /* il server si aspetta che il client inizi la connessione con uno specifico handshake.
         * Se il primi nByte di bufferIN corrispondono ad un valido handshake iniziato dal client con la handshake_clientSend()
         * allora questa fn ritorna true */
        static bool          server_isAValidHandshake(const void *bufferIN, u32 sizeOfBuffer);

    public:
							ProtocolWebsocket (gos::Allocator *allocatorIN, u32 startingSizeOfWriteBuffer) : IProtocol(allocatorIN, startingSizeOfWriteBuffer)		{ }
		virtual             ~ProtocolWebsocket()																													{ }

		bool				handshake_clientSend (IProtocolChannel *ch, gos::Logger *logger);
		bool				handshake_serverAnswer(IProtocolChannel *ch, gos::Logger *logger);

        // fn specifiche del protocollo websocket, non ereditate da IProtocol
		u32                 writeBuffer (IProtocolChannel *ch, const void *bufferIN, u16 nBytesToWrite);
		u32                 writeText (IProtocolChannel *ch, const char *strIN);
		void                sendPing (IProtocolChannel *ch);
		void				sendClose(IProtocolChannel *ch);


	protected:
		void				virt_sendCloseMessage(IProtocolChannel *ch)																							    { sendClose(ch); }
		u32					virt_decodeBuffer (IProtocolChannel *ch, const u8 *bufferIN, u32 nBytesInBufferIN, ProtocolBuffer &out_result, u32 *out_nBytesInseritiInOutResult);
		u32					virt_encodeBuffer (const u8 *bufferToEncode, u32 nBytesToEncode, ProtocolBuffer &out_buffer);

    private:
        enum class eWebSocketOpcode : u8
        {
            CONTINUATION = 0x0,
            TEXT = 0x1,
            BINARY = 0x2,
            CLOSE = 0x8,
            PING = 0x9,
            PONG = 0x0A,
            UNKNOWN = 0xff
        };

        struct sDecodeResult
        {
            const u8            *payload;
            u16                 payloadLen;
            eWebSocketOpcode    opcode;
            u8	                bIsLastFrame;
			u8					isMasked;
			u8					keys[4];
        };

		struct Handshake
		{
			char    resource[32];
			char    host[64];
			char    received_key[256];
			char    extension[256];
			char    protocol[128];
			u8      upgrade;
			u8      connection;
			u32     version;
		};

    private:
		u32					priv_decodeOneMessage (const u8 *buffer, u32 nBytesInBuffer, sDecodeResult *out_result) const;
        u32                 priv_encodeAMessage (bool bFin, eWebSocketOpcode opcode, const void *payloadToSend, u32 payloadLen, u8 *wBuffer, u32 sizeOfOutBuffer);

		static bool         priv_server_isAValidHandshake(const void *bufferIN, u32 sizeOfBuffer, Handshake *out);
	};

} //namespace gos
#endif // _gosProtocolWebsocket_h_
