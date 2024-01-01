#include "gosIProtocol.h"

using namespace gos;

//*************************************************
IProtocol::IProtocol (Allocator * allocatorIN, u32 startingSizeOfWriteBuffer)
{
	bufferW.setup  (allocatorIN, startingSizeOfWriteBuffer);
}

//*************************************************
IProtocol::~IProtocol()
{
	bufferW.unsetup();
}

//*************************************************
u32 IProtocol::read (IProtocolChannel *ch, u32 timeoutMSec, ProtocolBuffer &out_result)
{
	//legge dal canale di comunicazione il quale bufferizza l'input ricevuto
	u32 err = ch->read(timeoutMSec);
	if (err >= protocol::RES_ERROR)
		return err;

	const u32 nBytesInBuffer = ch->getNumBytesInReadBuffer();
	if (nBytesInBuffer == 0)
		return 0;

	//prova a parsare il buffer
	u32 nBytesInseritiInOutResult = 0;
	const u32 nBytesConsumati = virt_decodeBuffer (ch, ch->getReadBuffer(), ch->getNumBytesInReadBuffer(), out_result, &nBytesInseritiInOutResult);
	if (nBytesConsumati == 0 || nBytesConsumati >= protocol::RES_ERROR)
		return nBytesConsumati;

	ch->consumeReadBuffer(nBytesConsumati);
	return nBytesInseritiInOutResult;
}

//*************************************************
u32 IProtocol::write (IProtocolChannel *ch, const u8 *bufferToWrite, u16 nBytesToWrite, u32 timeoutMSec)
{
	if (nBytesToWrite == 0 || NULL == bufferToWrite)
		return 0;

	bufferW.reset();
	const u32 nByteToSend = virt_encodeBuffer (bufferToWrite, nBytesToWrite, bufferW);

	if (nByteToSend >= protocol::RES_ERROR)
	{
		DBGBREAK;
		return 0;
	}

	if (nByteToSend > 0)
	{
		assert (nByteToSend <= 0xFFFF);
		return ch->write(bufferW._getPointer(0), static_cast<u16>(nByteToSend), timeoutMSec);
	}
	return nByteToSend;
}
