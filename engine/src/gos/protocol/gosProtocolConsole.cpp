#include "gosProtocolConsole.h"
#include "../gosUtils.h"
#include "../gosRandom.h"
//#include <stdio.h>
#include <time.h>

using namespace gos;

/****************************************************
 * client_sendHandshake
 *
 * un client connesso ad un server deve mandare questo messaggio
 * per farsi riconoscere come console
 */
bool ProtocolConsole::handshake_clientSend(IProtocolChannel *ch, gos::Logger *logger)
{
    if (logger)
    {
        logger->log ("handshake..\n");
        logger->incIndent();
    }

    gos::Random random((u32)time(NULL));
    const u8 key = static_cast<u8>(random.getU32(255));

    char handshake[32];
    sprintf_s (handshake, sizeof(handshake), "GOSCONSOLE");
    handshake[11] = key;

    if (logger)
        logger->log ("sending with key=%d\n", key);
    ch->write ((const u8 *)handshake, 12, 2000);


	//aspetta la risposta dal server
    const u32 n = ch->read (5000);
	if (n >= protocol::RES_ERROR)
	{
		if (logger)
			logger->log("error while waiting. Error code is [0x%08X]\n", n);
		return false;
	}

    if (logger)
        logger->log ("response length is %d bytes\n", ch->getNumBytesInReadBuffer());
    if (ch->getNumBytesInReadBuffer() < 12)
    {
		ch->consumeReadBuffer(ch->getNumBytesInReadBuffer());
        if (logger)
        {
            logger->log("FAIL\n");
            logger->decIndent();
        }
        return false;
    }

	const u8 *p = ch->getReadBuffer();
    const u8 expectedKey = 0xff - key;
    if (memcmp (p, "GOSCONSOLE", 11) != 0 || (u8)p[11] != expectedKey)
    {
        if (logger)
        {
			memcpy(handshake, p, 12);
            u8 receivedKey = handshake[11];
            handshake[11] = 0;
            logger->log("Invalid answer: [%s], received key=%d, expected=%d\n", handshake, receivedKey, expectedKey);
            logger->decIndent();
			ch->consumeReadBuffer(12);
        }
        return false;
    }
	

	ch->consumeReadBuffer(12);
	if (logger)
    {
        logger->log("Done!\n");
        logger->decIndent();
    }
    return true;
}


//****************************************************
bool ProtocolConsole::server_isAValidHandshake (const void *bufferIN, u32 sizeOfBuffer)
{
    if (sizeOfBuffer < 12)
        return false;

    if (memcmp (bufferIN, "GOSCONSOLE", 11) != 0)
        return false;
    else
        return true;
}

//****************************************************
bool ProtocolConsole::handshake_serverAnswer(IProtocolChannel *ch, UNUSED_PARAM(gos::Logger *logger))
{
	if (!server_isAValidHandshake(ch->getReadBuffer(), ch->getNumBytesInReadBuffer()))
		return false;

	const u8 *buffer = ch->getReadBuffer();
	const u8 key = 0xff - buffer[11];
	ch->consumeReadBuffer(12);

	char answer[16];
	sprintf(answer, "GOSCONSOLE");
	answer[11] = key;
	const u32 n = ch->write ((const u8 *)answer, 12, 3000);

	if (n != 12)
		return false;
	return true;
}



/****************************************************
 * vedi IProtocol.h
 */
u32 ProtocolConsole::virt_decodeBuffer (IProtocolChannel *ch, const u8 *buffer, u32 nBytesInBuffer, ProtocolBuffer &out_result, u32 *out_nBytesInseritiInOutResult)
{
	*out_nBytesInseritiInOutResult = 0;

	//prova a decodificare i dati che sono nel buffer di lettura per vedere se riesce a tirarci fuori un frame
	sDecodeResult decoded;
	const u32 nBytesConsumed = priv_decodeOneMessage (buffer, nBytesInBuffer, &decoded);
	if (nBytesConsumed == 0 || nBytesConsumed >= protocol::RES_ERROR)
		return nBytesConsumed;

	//in decoded c'è un messaggio buono, vediamo di cosa si tratta
	switch (decoded.what)
	{
		case eOpcode::none:
			//non ho trovato un valido messaggio nel buffer, ma potrei aver parsato e scartato un po' di byte.
			//Ritorno quindi il num di byte eventualmente consumati
			return nBytesConsumed;

		case eOpcode::msg:
			//copio il payload appena ricevuto nel buffer utente
			if (decoded.payloadLen)
			{
				out_result.append (decoded.payload, decoded.payloadLen);
				*out_nBytesInseritiInOutResult += decoded.payloadLen;
			}
			return nBytesConsumed;

		case eOpcode::close:
			//rispondoa a mia volta con close e chiudo
			virt_sendCloseMessage(ch);
			return protocol::RES_PROTOCOL_CLOSED;

		default:
			//messaggio invalido, chiudo il protocollo
			DBGBREAK;
			virt_sendCloseMessage(ch);
			return protocol::RES_PROTOCOL_CLOSED;
	}
}

/****************************************************
 * Prova ad estrarre un valido messaggio dal buffer e ritorna il numero di bytes "consumati" durante il processo.
 * 
 */
u32 ProtocolConsole::priv_decodeOneMessage (const u8 *buffer, u32 nBytesInBuffer, sDecodeResult *out_result) const
{
	out_result->what = eOpcode::none;
	out_result->payloadLen = 0;

	u32 ct = 0;
	while (ct < nBytesInBuffer)
	{
		//il primo char buono deve essere MAGIC_CODE_1
		const u32 startOfThisMessage = ct;
		if (buffer[ct++] != MAGIC_CODE_1)
			continue;		

		//a seguire ci deve essere MAGIC_CODE2 e opcode
		if (ct+2 > nBytesInBuffer)
			return startOfThisMessage;

		if (buffer[ct] != MAGIC_CODE_2)
			continue;
		ct++;

		const eOpcode opcode = (eOpcode)buffer[ct++];
		switch (opcode)
		{
			default:
				//Errore, opcode non riconosciuto, riparto a scannare
				DBGBREAK;
				ct = startOfThisMessage + 1;
				break;

			case eOpcode::close:
				//ho ricevuto una esplicita richiesta di chiudere il canale
				//MAGIC1 MAGIC2 [close] 
				out_result->what = opcode;
				return protocol::RES_PROTOCOL_CLOSED;

			case eOpcode::internal_simpleMsg:
			{
				/*  messaggio semplice, payloadLen <=255 byte, ck8 semplice a fine messaggio:
						MAGIC1 MAGIC2 WHAT [payloadLen] [data] ... [data] [payloadCK]
				*/
				
				//se non ci sono abbastanza bytes per il payloadLen, playload e la ck, ritorno "startOfThisMessage" perchè vuol dire che il buffer non ha ricevuto ancora abbastanza dati
				//per completare il msg, ma i dati ricevuti fino ad ora sembrano validi
				if (ct >= nBytesInBuffer)
					return startOfThisMessage;
				out_result->payloadLen = buffer[ct++];

				if (startOfThisMessage +5 +out_result->payloadLen > nBytesInBuffer)
					return startOfThisMessage;
				out_result->payload = &buffer[ct];
				ct += out_result->payloadLen;

				//verifichiamo CK sul payload
				const u8 ck = buffer[ct++];
				const u8 calc_ck = gos::utils::simpleChecksum8_calc (out_result->payload, out_result->payloadLen);
				if (ck == calc_ck)
				{
					out_result->what = eOpcode::msg;
					return ct;
				}

				//errore nella ck, butto via tutto e riparto a scannare
				//printf("ERR ProtocolConsole::decodeBuffer() => bad CK [%d] expected [%d]\n", ck, calc_ck);
				DBGBREAK;
				ct = startOfThisMessage + 1;
			}
			break;

			case eOpcode::internal_extendedMsg:
			{
				//MAGIC1 MAGIC2 WHAT [payloadLen_MSB] [payloadLen_LSB] [payload] ... [payload] [payloadCK(u16)]

				//se non ci sono abbastanza bytes per il payloadLen, playload e la ck, ritorno "startOfThisMessage" perchè vuol dire che il buffer non ha ricevuto ancora abbastanza dati
				//per completare il msg, ma i dati ricevuti fino ad ora sembrano validi
				if (ct+1 >= nBytesInBuffer)
					return startOfThisMessage;
				out_result->payloadLen = gos::utils::bufferReadU16 (&buffer[ct]);
				ct+=2;

				if (startOfThisMessage + 7 + out_result->payloadLen > nBytesInBuffer)
					return startOfThisMessage;
				out_result->payload = &buffer[ct];
				ct += out_result->payloadLen;

				//verifichiamo CK
				const u16 ck = gos::utils::bufferReadU16 (&buffer[ct]);
				ct+=2;

				if (ck == gos::utils::simpleChecksum16_calc(out_result->payload, out_result->payloadLen))
				{
					out_result->what = eOpcode::msg;
					return ct;
				}

				//printf("ERR ProtocolConsole::decodeBuffer() => bad CK [%d] expected [%d]\n", ck, calc_ck);
				DBGBREAK;
				ct = startOfThisMessage + 1;
			}
			break;
		}
	}
	return nBytesInBuffer;
}



/****************************************************
 * vedi IProtocol.h
 */
u32 ProtocolConsole::virt_encodeBuffer (const u8 *bufferToEncode, u32 nBytesToEncode, ProtocolBuffer &out_buffer)
{
	const u32 bytesNeeded = 10 + nBytesToEncode;
	
	//alloco memoria se necessario
	{
		const u32 bytesFreeInOutBuffer = out_buffer.getTotalSizeAllocated() - out_buffer.getCursor();
		if (bytesFreeInOutBuffer < bytesNeeded)
			out_buffer.growIncremental (bytesNeeded - bytesFreeInOutBuffer);
	}

	//encodo
	u8 *wBuffer = out_buffer._getPointer(out_buffer.getCursor());
	const u32 sizeOfOutWBuffer = out_buffer.getTotalSizeAllocated() - out_buffer.getCursor();
	return priv_encodeAMessage(eOpcode::msg, bufferToEncode, nBytesToEncode, wBuffer, sizeOfOutWBuffer);
}

/****************************************************
 * Prepara un valido messaggio e lo mette in wBuffer a partire dal byte 0.
 * Ritorna la lunghezza in bytes del messaggio
 */
u32 ProtocolConsole::priv_encodeAMessage (eOpcode opcode, const void *payloadToSend, u32 payloadLen, u8 *wBuffer, u32 sizeOfOutBuffer)
{
	assert (payloadLen <= 65000);

	if (sizeOfOutBuffer < 3)
	{
		DBGBREAK;
		return 0;
	}

	if (opcode == eOpcode::msg)
	{
		if (payloadLen == 0)
			return 0;
		if (payloadLen <= 0xff)
			opcode = eOpcode::internal_simpleMsg;
		else
			opcode = eOpcode::internal_extendedMsg;
	}

    u32 ct = 0;
	wBuffer[ct++] = MAGIC_CODE_1;
	wBuffer[ct++] = MAGIC_CODE_2;
	wBuffer[ct++] = (u8)opcode;

	switch (opcode)
	{
		default:
			DBGBREAK;
			return 0;

		case eOpcode::close:
			return ct;

		case eOpcode::internal_simpleMsg:
			//MAGIC1 MAGIC2 WHAT LEN(byte) payload...payload CK(byte)
			if (sizeOfOutBuffer < 5 + payloadLen)
				return protocol::RES_PROTOCOL_WRITEBUFFER_TOOSMALL;


			wBuffer[ct++] = (u8)payloadLen;
			memcpy (&wBuffer[ct], payloadToSend, payloadLen);
			ct += payloadLen;
			wBuffer[ct++] = gos::utils::simpleChecksum8_calc (payloadToSend, payloadLen);
			return ct;
    

		case eOpcode::internal_extendedMsg:
			if (payloadLen == 0)
				return 0;

			//MAGIC1 MAGIC2 WHAT LEN_MSB LEN_LSB payload...payload CK(u16)
			if (sizeOfOutBuffer < 7 + payloadLen)
				return protocol::RES_PROTOCOL_WRITEBUFFER_TOOSMALL;

			gos::utils::bufferWriteU16 (&wBuffer[ct], static_cast<u16>(payloadLen));
			ct+=2;
			
			memcpy(&wBuffer[ct], payloadToSend, payloadLen);
			ct += payloadLen;

			u16 ck = gos::utils::simpleChecksum16_calc (payloadToSend, payloadLen);
			gos::utils::bufferWriteU16 (&wBuffer[ct], ck);
			ct+=2;
			return ct;
	}
}



//****************************************************
void ProtocolConsole::virt_sendCloseMessage(IProtocolChannel *ch)
{
	u8 wBuffer[32];
	const u32 n = priv_encodeAMessage (eOpcode::close, NULL, 0, wBuffer, sizeof(wBuffer));

	assert (n <= 0xFFFF);
	ch->write (wBuffer, static_cast<u16>(n), 1000);
}


