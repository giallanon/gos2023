#include "gosProtocolWebsocket.h"
#include "../gosUtils.h"
#include "../gosString.h"

using namespace gos;

// WebSocket Universally Unique IDentifier
static const char WS_UUID[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
static const char HEADER_UPGRADE[] = "Upgrade: websocket";
static const char HEADER_CONNECTION[] = "Connection: Upgrade";
static const char HEADER_CONNECTION2[] = "Connection: keep-alive, Upgrade";
static const char HEADER_HOST[] = "Host: ";
static const char HEADER_KEY[] = "Sec-WebSocket-Key: ";
static const char HEADER_VERSION[] = "Sec-WebSocket-Version: ";
static const char HEADER_EXTENSION[] = "Sec-WebSocket-Extensions: ";
static const char HEADER_PROTOCOL[] = "Sec-WebSocket-Protocol: ";


/****************************************************
 * client_sendHandshake
 *
 *	Dovrebbe iniziare l'handshake lato client, ma non lo implemento
 *	per ora perchè non mi serve. 
 */
bool ProtocolWebsocket::handshake_clientSend (UNUSED_PARAM(IProtocolChannel *ch), gos::Logger *logger)
{
	if (logger)
	{
		logger->log("handshake..\n");
		logger->incIndent();
	}

	if (logger)
		logger->log("error, function non implemented\n");

	if (logger)
		logger->decIndent();

	return false;
}


//****************************************************
bool ProtocolServer_handshake_check_header (const char *src, const char *header)
{
    return (strncasecmp(src, header, strlen(header)) == 0);
}

//****************************************************
void ProtocolServer_handshake_copy_header (char *out, const char *src, u16 maxSizeOfOutInBytes)
{
    maxSizeOfOutInBytes--;
    u16 i=0;
    while (i<maxSizeOfOutInBytes)
    {
        char c = src[i];
        if (c==0x00 || c=='\r' || c=='\n')
            break;
        out[i++] = c;
    }
    out[i] = 0x00;
}

//****************************************************
bool ProtocolServer_handshake_make_accept (const char *received_key, char *acceptKey, u32 sizeOfAcceptKeyInBytes)
{
    if (sizeOfAcceptKeyInBytes < 32)
        return false;

    //concat di received_key & WS_UUID
    char concat_key[256+64];
	u32 lenOfReceivedKey = (u32)strlen(received_key);
	u32 lenOfWS_UUID = (u32)strlen(WS_UUID);
    if (lenOfReceivedKey + lenOfWS_UUID >= sizeof(concat_key)-1)
        return false;

#ifdef GOS_COMPILER__GCC
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wsizeof-pointer-memaccess"
#endif
#ifdef GOS_COMPILER__MSVC
	//#pragma warning(disable:4201)
	#pragma warning(disable:4996)
#endif

    memset (concat_key, 0, sizeof(concat_key));
    strncpy (concat_key, received_key, sizeof(concat_key));
	strncat (concat_key, WS_UUID, sizeof(WS_UUID));

#ifdef GOS_COMPILER__GCC
	#pragma GCC diagnostic pop
#endif
#ifdef GOS_COMPILER__MSVC
	//#pragma warning(default:4201)
	#pragma warning(default:4996)
#endif


    


    //sha-1 della key concatenata
    u8 sha1_key[20];
    utils::sha1(sha1_key, sizeof(sha1_key), concat_key, (u32)strlen(concat_key));

    //converto la sha_key in base 64
    //Mi servono almeno 31 bytes (vedi gos::base64_howManyBytesNeededForEncoding())
    return utils::base64_encode ((char*)acceptKey, sizeOfAcceptKeyInBytes, sha1_key, sizeof(sha1_key));
}

//****************************************************
bool ProtocolWebsocket::server_isAValidHandshake(const void *bufferIN, u32 sizeOfBuffer)
{
	Handshake hs;
	return priv_server_isAValidHandshake(bufferIN, sizeOfBuffer, &hs);
}

//****************************************************
bool ProtocolWebsocket::priv_server_isAValidHandshake(const void *bufferIN, u32 sizeOfBufferIN, Handshake *out)
{
	const u16 MAX_BUFFER_SIZE = 1024;
	char buffer[MAX_BUFFER_SIZE];

	if (sizeOfBufferIN >= MAX_BUFFER_SIZE - 1)
		return false;
	memcpy(buffer, bufferIN, sizeOfBufferIN);
	buffer[sizeOfBufferIN] = 0;

	memset(out, 0x00, sizeof(Handshake));

	//il buffer di handshake deve iniziare con la classica richiesta HTTP
	if (sscanf(buffer, "GET %s HTTP/1.1", out->resource) != 1)
	{
		//printf ("Invalid HTTP GET request.\n");
		return false;
	}

	//parsing dell'handshake ricevuto
	char *token = NULL;
	char *tokState;
	token = strtok_r(buffer, "\r\n", &tokState);
	while (token)
	{
		if (ProtocolServer_handshake_check_header(token, HEADER_UPGRADE))
			out->upgrade = 1;

		else if (ProtocolServer_handshake_check_header(token, HEADER_CONNECTION))
			out->connection = 1;

		else if (ProtocolServer_handshake_check_header(token, HEADER_CONNECTION2))
			out->connection = 2;

		else if (ProtocolServer_handshake_check_header(token, HEADER_HOST))
			ProtocolServer_handshake_copy_header(out->host, &token[strlen(HEADER_HOST)], sizeof(out->host));

		else if (ProtocolServer_handshake_check_header(token, HEADER_KEY))
			ProtocolServer_handshake_copy_header(out->received_key, &token[strlen(HEADER_KEY)], sizeof(out->received_key));

		else if (ProtocolServer_handshake_check_header(token, HEADER_VERSION))
			out->version = gos::string::utf8::toU32((const u8 *)&token[strlen(HEADER_VERSION)]);
		else if (ProtocolServer_handshake_check_header(token, HEADER_EXTENSION))
			ProtocolServer_handshake_copy_header(out->extension, &token[strlen(HEADER_EXTENSION)], sizeof(out->extension));

		else if (ProtocolServer_handshake_check_header(token, HEADER_PROTOCOL))
			ProtocolServer_handshake_copy_header(out->protocol, &token[strlen(HEADER_PROTOCOL)], sizeof(out->protocol));


		token = strtok_r(NULL, "\r\n", &tokState);
	}

	//se tutto ok, preparo l'handshake di risposta
	if (out->upgrade && out->connection && out->host[0] && out->received_key[0] && out->version)
		return true;

	return false;
}


//****************************************************
bool ProtocolWebsocket::handshake_serverAnswer(IProtocolChannel *ch, UNUSED_PARAM(gos::Logger *logger))
{
	Handshake hs;
	if (!priv_server_isAValidHandshake(ch->getReadBuffer(), ch->getNumBytesInReadBuffer(), &hs))
		return false;
	ch->consumeReadBuffer(ch->getNumBytesInReadBuffer());

    //calcolo la key di risposta
    char    send_key[32];
	if (!ProtocolServer_handshake_make_accept(hs.received_key, send_key, sizeof(send_key)))
		return false;

    //printf ("Handshake request from %s:\n\tReceived key: \t%s\n\tSend key: \t%s\n", hs.host, hs.received_key, send_key);

    //fillo il buffer con la risposta da rimandare indietro
    const char *connection = HEADER_CONNECTION;
    if (hs.connection == 2)
    {
        connection = HEADER_CONNECTION2;
    }
    char answer[300];
    sprintf_s(answer, sizeof(answer),   "HTTP/1.1 101 Switching Protocols\r\n"\
                        "Upgrade: websocket\r\n"\
                        "%s\r\n"\
                        "Sec-WebSocket-Accept: %s\r\n"\
                        "Sec-WebSocket-Protocol: %s\r\n\r\n",
                        connection, send_key, hs.protocol);
    //printf ("Handshake response: %s\n", out);


	const u32 n = ch->write((const u8 *)answer, (u16)strlen(answer), 2000);
    if (n == 0 || n >= protocol::RES_ERROR)
        return false;
    return true;
}




/****************************************************
 * vedi IProtocol.h
 */
u32 ProtocolWebsocket::virt_decodeBuffer (IProtocolChannel *ch, const u8 *buffer, u32 nBytesInBuffer, ProtocolBuffer &out_result, u32 *out_nBytesInseritiInOutResult)
{
	*out_nBytesInseritiInOutResult = 0;

	//prova a decodificare i dati che sono nel buffer di lettura per vedere
	//se riesce a tirarci fuori un frame
	u32 totalBytesConsumed = 0;
	while (nBytesInBuffer)
	{
		sDecodeResult decoded;
		const u32 nBytesConsumed = priv_decodeOneMessage (&buffer[totalBytesConsumed], nBytesInBuffer, &decoded);
		if (nBytesConsumed == 0)
			return totalBytesConsumed;
		if (nBytesConsumed >= protocol::RES_ERROR)
			return nBytesConsumed;
		totalBytesConsumed += nBytesConsumed;
		nBytesInBuffer -= nBytesConsumed;


		//in decoded c'e' un messaggio buono, vediamo di cosa si tratta.
		//Se � un messaggio TEXT o BINARY, allora ne copio il payload in out_result
		//Se invece sono messaggi interni di protocollom, li gestisco e riparto la scansione del buffer in cerca di ulteriori messaggi
		switch (decoded.opcode)
		{
		default:
			//messaggio invalido, chiudo il canale
			sendClose(ch);
			return protocol::RES_PROTOCOL_CLOSED;

		case eWebSocketOpcode::TEXT:
		case eWebSocketOpcode::BINARY:
			//copio il payload appena ricevuto nel buffer utente
			if (decoded.payloadLen)
			{
				const u32 curBufferCursor = out_result.getCursor();
				out_result.append (decoded.payload, decoded.payloadLen);
				*out_nBytesInseritiInOutResult += decoded.payloadLen;

				if (decoded.isMasked)
				{
					//unmask del payload se necessario
					u8 *p = out_result._getPointer(curBufferCursor);
					for (u32 i = 0; i < decoded.payloadLen; i++)
						p[i] ^= decoded.keys[i % 4];
				}
			}
			return totalBytesConsumed;

		case eWebSocketOpcode::PING:
		{
			//unmask del payload se necessario
			if (decoded.isMasked)
			{
				DBGBREAK;
				/*u8 *p = decoded.payload;
				for (u16 i = 0; i < decoded.payloadLen; i++)
					p[i] ^= decoded.keys[i % 4];*/
			}

			//rispondo con pong
			u8 wBuffer[32];
			u32 n = priv_encodeAMessage(true, eWebSocketOpcode::PONG, decoded.payload, decoded.payloadLen, wBuffer, sizeof(wBuffer));
			ch->write(wBuffer, (u16)n, 500);
		}
		break;

		case eWebSocketOpcode::PONG:
			//ho ricevuto un pong
			break;

		case eWebSocketOpcode::CLOSE:
			//rispondo a mia volta con close e chiudo
			sendClose(ch);
			return protocol::RES_PROTOCOL_CLOSED;
		}
	}

	return totalBytesConsumed;
}

/****************************************************
 * Prova ad estrarre un valido messaggio websocket dal buffer e ritorna il numero di bytes "consumati" durante il processo.
 * Se non ci sono abbastanza bytes per un valido completo messaggio, ritorna 0 in quanto non consuma alcun bytes. Si suppone che
 * qualcuno all'esterno continuera'  ad appendere bytes al buffer fino a quando questo non conterra'  un valido messaggio consumabile da
 * questa fn
 */
u32 ProtocolWebsocket::priv_decodeOneMessage(const u8 *buffer, u32 nBytesInBuffer, sDecodeResult *out_result) const
{
    //ci devono essere almeno 2 bytes, questi sono obbligatori, il protocollo Websocket non ammette msg lunghi meno di 2 bytes
    if (nBytesInBuffer < 2)
        return 0;

    u32 ct=0;

    //dal primo byte si ricava fin, RSV1, RSV2, RSV3, opcode
    u8 b = buffer[ct++];
    {
        out_result->bIsLastFrame = ((b & 0x80) != 0);

        //MUST be 0 unless an extension is negotiated that defines meanings for non-zero values.
        //u8 RSV1 = (b & 0x40);
        //u8 RSV2 = (b & 0x20);
        //u8 RSV3 = (b & 0x10);

        //opcode
        switch ((b & 0x0F))
        {
        case 0x00:  out_result->opcode = eWebSocketOpcode::CONTINUATION;	break;
        case 0x01:  out_result->opcode = eWebSocketOpcode::TEXT;			break;
        case 0x02:  out_result->opcode = eWebSocketOpcode::BINARY;			break;
        case 0x08:  out_result->opcode = eWebSocketOpcode::CLOSE;			break;
        case 0x09:  out_result->opcode = eWebSocketOpcode::PING;			break;
        case 0x0A:  out_result->opcode = eWebSocketOpcode::PONG;			break;
        default:    out_result->opcode = eWebSocketOpcode::UNKNOWN;			break;
        }
    }

    //dal secondo byte si ricava "isMasked" e una indicazione sulla lunghezza del payload
    b = buffer[ct++];
	out_result->isMasked = ((b & 0x80) != 0);

    //payload length
	out_result->payloadLen = (b & 0x7f);


    //la lunghezza del payload potrebbe essere indicata da ulteriori bytes.
    if (out_result->payloadLen == 126)
    {
        if (nBytesInBuffer < 4)
            return 0;
		out_result->payloadLen = buffer[ct++];
		out_result->payloadLen <<= 8;
		out_result->payloadLen += buffer[ct++];
    }
    //If payloadLen == 127, the following 8 bytes interpreted as a 64-bit unsigned integer
    else if (out_result->payloadLen == 127)
    {
        //non lo sto nemmeno ad implementare. Se voglio mandare più di 65K di roba in un messaggio è certamente un errore
		DBGBREAK;
        out_result->opcode = eWebSocketOpcode::UNKNOWN;
        return protocol::RES_PROTOCOL_CLOSED;
    }


    //se il messaggio è cifrato, a seguire ci sono 4 bytes con la chiave
    out_result->keys[0] = out_result->keys[1] = out_result->keys[2] = out_result->keys[3] = 0x00;
    if (out_result->isMasked)
    {
        if (nBytesInBuffer < ct+4)
            return 0;
		out_result->keys[0] = buffer[ct++];
		out_result->keys[1] = buffer[ct++];
		out_result->keys[2] = buffer[ct++];
		out_result->keys[3] = buffer[ct++];
    }

    //payload
    if (nBytesInBuffer < ct + out_result->payloadLen)
        return 0;
    out_result->payload = &buffer[ct];
	ct += out_result->payloadLen;

    return ct;
}

/****************************************************
 * vedi IProtocol.h
 */
u32 ProtocolWebsocket::virt_encodeBuffer(const u8 *bufferToEncode, u32 nBytesToEncode, ProtocolBuffer &out_buffer)
{
	const u32 wBufferCursor = out_buffer.getCursor();

	while (1)
	{
		u8 *wBuffer = out_buffer._getPointer(wBufferCursor);
		const u32 sizeOfWBuffer = out_buffer.getTotalSizeAllocated() - out_buffer.getCursor();
		const u32 ret = priv_encodeAMessage (true, eWebSocketOpcode::BINARY, bufferToEncode, nBytesToEncode, wBuffer, sizeOfWBuffer);
	
		if (ret == protocol::RES_PROTOCOL_WRITEBUFFER_TOOSMALL)
		{
			//se necessario, allargo out_buffer per ospitare tutti i dati e riparto
			out_buffer.growDouble();
			continue;
		}

		return ret;
	}
}

/****************************************************
 * Prepara un valido messaggio Websocket e lo mette in wBuffer a partire dal byte 0.
 * Ritorna la lunghezza in bytes del messaggio
 */
u32 ProtocolWebsocket::priv_encodeAMessage(bool bFin, eWebSocketOpcode opcode, const void *payloadToSend, u32 payloadLen, u8 *wBuffer, u32 sizeOfOutBuffer)
{
	const u32 nBytesRequired = payloadLen + 5;
	if (sizeOfOutBuffer < nBytesRequired)
		return protocol::RES_PROTOCOL_WRITEBUFFER_TOOSMALL;

	u32 ct = 0;

	//primo byte : fin, RSV1, RSV2, RSV3, opcode
	wBuffer[ct] = (u8)opcode;
	if (bFin)
		wBuffer[ct] |= 0x80;
	++ct;

	//secondo byte: isMasked | payloadLen
	if (payloadLen < 126)
		wBuffer[ct++] = (u8)payloadLen;
	else if (payloadLen < 0xffff)
	{
		wBuffer[ct++] = 126;
		wBuffer[ct++] = (u8)((payloadLen & 0xff00) >> 8);
		wBuffer[ct++] = (u8)(payloadLen & 0x00ff);
	}
	else
	{
		//i messaggi a lunghezza superiore ai 65k non li supporto
		DBGBREAK;
		return 0;
	}

	//payload
	if (payloadLen > 0)
	{
		assert(ct + payloadLen <= sizeOfOutBuffer);
		memcpy(&wBuffer[ct], payloadToSend, payloadLen);
		ct += payloadLen;
	}

	return ct;
}




//****************************************************
u32 ProtocolWebsocket::writeText (IProtocolChannel *ch, const char *strIN)
{
    if (NULL == strIN)
        return 0;
    u32 n = static_cast<u32>(strlen(strIN));
    if (n < 1)
    {
        return 0;
    }

	while (1)
	{
		bufferW.reset();
        const u32 nToWrite = priv_encodeAMessage (true, eWebSocketOpcode::TEXT, strIN, n, bufferW._getPointer(0), bufferW.getTotalSizeAllocated());
		if (protocol::RES_PROTOCOL_WRITEBUFFER_TOOSMALL == nToWrite)
		{
			bufferW.growDouble();
			continue;
		}

		if (nToWrite)
		{
			assert (nToWrite <= 0xFFFF);
			ch->write (bufferW._getPointer(0), static_cast<u16>(nToWrite), 1000);
            return nToWrite;
		}

		return 0;
	}
}

//****************************************************
u32 ProtocolWebsocket::writeBuffer (IProtocolChannel *ch, const void *bufferIN, u16 nBytesToWrite)
{
	if (NULL == bufferIN || nBytesToWrite==0)
		return 0;

	while (1)
	{
		bufferW.reset();
		const u32 nToWrite = priv_encodeAMessage (true, eWebSocketOpcode::BINARY, bufferIN, nBytesToWrite, bufferW._getPointer(0), bufferW.getTotalSizeAllocated());
		if (protocol::RES_PROTOCOL_WRITEBUFFER_TOOSMALL == nToWrite)
		{
			bufferW.growDouble();
			continue;
		}

		if (nToWrite)
		{
			assert (nToWrite <= 0xFFFF);
			ch->write (bufferW._getPointer(0), static_cast<u16>(nToWrite), 1000);
            return nToWrite;
		}

		return 0;
	}
}

//****************************************************
void ProtocolWebsocket::sendPing (IProtocolChannel *ch)
{
	u8 wBuffer[32];
	const u32 n = priv_encodeAMessage(true, eWebSocketOpcode::PING, NULL, 0, wBuffer, sizeof(wBuffer));
	assert (n <= 0xFFFF);
	ch->write(wBuffer, static_cast<u16>(n), 1000);
}

//****************************************************
void ProtocolWebsocket::sendClose(IProtocolChannel *ch)
{
	u8 wBuffer[32];
	const u32 n = priv_encodeAMessage(true, eWebSocketOpcode::CLOSE, NULL, 0, wBuffer, sizeof(wBuffer));
	assert (n <= 0xFFFF);
	ch->write(wBuffer, static_cast<u16>(n), 200);
}
