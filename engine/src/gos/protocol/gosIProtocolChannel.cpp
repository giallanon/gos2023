#include "gosIProtocolChannel.h"
#include "../gos.h"

using namespace gos;


//****************************************************
IProtocolChannel::IProtocolChannel(Allocator *allocatorIN, u32 startingSizeOfReadBufferInBytes, u32 maxSizeOfReadBufferIN)
{
	assert (maxSizeOfReadBufferIN >= startingSizeOfReadBufferInBytes);
	maxSizeOfReadBuffer = maxSizeOfReadBufferIN;
	if (startingSizeOfReadBufferInBytes > maxSizeOfReadBuffer)
		startingSizeOfReadBufferInBytes = maxSizeOfReadBuffer;

	bufferR.setup (allocatorIN, startingSizeOfReadBufferInBytes);
}

//****************************************************
IProtocolChannel::~IProtocolChannel()
{
	bufferR.unsetup();
}

//******************************************************
u32 IProtocolChannel::read (u32 timeoutMSec)
{
	//se necessario, allargo il buffer di lettura fino al max consentito
	u32 nByteFreeInBuffer = bufferR.getTotalSizeAllocated() - bufferR.getCursor();
	if (nByteFreeInBuffer < 1024)
	{
		u32 newBufferSize = bufferR.getTotalSizeAllocated() * 2;
		if (newBufferSize > maxSizeOfReadBuffer)
			newBufferSize = maxSizeOfReadBuffer;
		bufferR.growUpTo (newBufferSize);
		nByteFreeInBuffer = bufferR.getTotalSizeAllocated() - bufferR.getCursor();
	}

	if (nByteFreeInBuffer > 0xFFFF)
		nByteFreeInBuffer = 0xFFFF;
	if (nByteFreeInBuffer == 0)
		return 0;
	const u32 nRead = virt_read (bufferR._getPointer(bufferR.getCursor()), nByteFreeInBuffer, timeoutMSec);
	if (nRead < protocol::RES_ERROR)
	    bufferR.advanceCursor (nRead);
	return nRead;

}

//******************************************************
u32 IProtocolChannel::write (const u8 *bufferToWrite, u32 nBytesToWrite, u32 timeoutMSec)
{ 
	const u64 timeToExitMSec = gos::getTimeSinceStart_msec() + timeoutMSec;
	
	u32 nWrittenSoFar = virt_write (bufferToWrite, nBytesToWrite);
	
	//if (nWrittenSoFar >= PROTORES_ERROR) return nWrittenSoFar;
	if (nWrittenSoFar >= nBytesToWrite) // l'if qui sopra non serve perch� in caso di errore, sicuramente nWrittenSoFar � >= di nBytesToWrite e quindi questa
		return nWrittenSoFar;			// condizione da sola le copre entrambe

	do
	{
		gos::sleep_msec (50);

		assert ( (nBytesToWrite - nWrittenSoFar) <= 0xFFFF);
		const u32 n = virt_write (&bufferToWrite[nWrittenSoFar], nBytesToWrite - nWrittenSoFar);
		if (n >= protocol::RES_ERROR)
			return n;

		if (n > 0)
		{
			nWrittenSoFar += n;
			if (nWrittenSoFar >= nBytesToWrite)
				return nWrittenSoFar;
		}
	
	} while (gos::getTimeSinceStart_msec() < timeToExitMSec);

	//se arriviamo qui, vuol dire che non siamo riusciti a spedire tutto quanto
	//DBGBREAK;
	return nWrittenSoFar;
}
