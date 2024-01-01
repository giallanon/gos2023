#include "gosProtocolChSocketTCP.h"

using namespace gos;

//**************************************************
ProtocolChSocketTCP::ProtocolChSocketTCP (Allocator *allocatorIN, u32 startingSizeOfReadBufferInBytes, u32 maxSizeOfReadBufferInBytes) :
	IProtocolChannel(allocatorIN, startingSizeOfReadBufferInBytes, maxSizeOfReadBufferInBytes)
{
	gos::socket::init(&sok);
}

//**************************************************
u32 ProtocolChSocketTCP::virt_read (u8 *buffer, u32 nMaxBytesToRead, u32 timeoutMSec)
{
	assert (nMaxBytesToRead <= 0xFFFF);
	i32 nBytesLetti = gos::socket::read (sok, buffer, static_cast<u16>(nMaxBytesToRead), timeoutMSec);
	if (nBytesLetti == 0)
		return protocol::RES_CHANNEL_CLOSED;

	if (nBytesLetti < 0)
		return 0;

	assert(static_cast<u32>(nBytesLetti) <= nMaxBytesToRead);
	return static_cast<u32>(nBytesLetti);
}

//**************************************************
u32 ProtocolChSocketTCP::virt_write (const u8 *bufferToWrite, u32 nBytesToWrite)
{
	assert (nBytesToWrite <= 0xFFFF);
	i32 n = gos::socket::write (sok, bufferToWrite, static_cast<u16>(nBytesToWrite));
	if (n < 0)
		return protocol::RES_CHANNEL_CLOSED;

//	assert (n>0);
	return static_cast<u32>(n);
}