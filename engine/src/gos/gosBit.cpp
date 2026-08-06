#include "gosBit.h"
#include "gos.h"

using namespace gos;

//******************************************
void gos::bitZERO  (void *dst, u32 sizeof_dst)
{
	assert (NULL != dst);
	assert (sizeof_dst);
	memset (dst, 0, sizeof_dst);
}

//******************************************
void gos::bitSET (void *dst, u32 sizeof_dst, u32 pos)
{
	assert (NULL != dst);
	assert (sizeof_dst);
	assert (pos < 8*sizeof_dst);

	const u32	byte = (pos >> 3); //pos / 8;
	const u32	bit  = (pos & 0x07); //pos % 8;
	const u8 mask = 0x01 << bit;

	u8 *p = reinterpret_cast<u8*>(dst);
	p[byte] |= mask;
}

//******************************************
void gos::bitCLEAR (void *dst, u32 sizeof_dst, u32 pos)
{
	assert (NULL != dst);
	assert (sizeof_dst);
	assert (pos < 8*sizeof_dst);

	const u32	byte = (pos >> 3); //pos / 8;
	const u32	bit  = (pos & 0x07); //pos % 8;
	const u8 mask = 0x01 << bit;

	u8 *p = reinterpret_cast<u8*>(dst);
	p[byte] &= ~mask;
}

//******************************************
bool gos::isBitSET (const void *dst, u32 sizeof_dst, u32 pos)
{
	assert (NULL != dst);
	assert (sizeof_dst);
	assert (pos < 8*sizeof_dst);

	const u32	byte = (pos >> 3); //pos / 8;
	const u32	bit  = (pos & 0x07); //pos % 8;
	const u8 mask = 0x01 << bit;

	const u8 *p = reinterpret_cast<const u8*>(dst);
	return ((p[byte] & mask) != 0);
}

//******************************************
void gos::bit32SET (u32 *dst, u32 pos)
{
	assert (NULL != dst);
	assert (pos < 32);
	(*dst) |= (0x00000001 << pos);
}	

//******************************************
void gos::bit32CLEAR (u32 *dst, u32 pos)
{
	assert (NULL != dst);
	assert (pos < 32);
	(*dst) &= ~(0x00000001 << pos);
}

//******************************************
bool gos::isBit32SET (u32 dst, u32 pos)
{
	assert (pos < 32);
	return ( ( dst & (0x00000001 << pos) ) != 0);
}


//******************************************
void gos::byte32SET (u32 *dst, u8 value, u32 pos)
{
	assert (NULL != dst); 
	assert (pos < 4);

	const u32 shift = (pos<<3);
	(*dst) &= ~(0x000000FF << shift );
	(*dst) |= (((u32)value) << shift);
}

//******************************************
u8 gos::byte32GET (u32 src, u32 pos)
{
	assert (pos < 4);

	const u32 shift = (pos<<3);
	return static_cast<u8>( (src >> shift) & 0x000000FF );
}

/************************************************************************************
 * 
 *       B i t f i e l d
 * 
 *************************************************************************************/
void Bitfield::setup (gos::Allocator *allocator, u32 numBitIN)
{
	assert (NULL == p);
	numBit = numBitIN;
	numU64Allocati = numBit / 64;
	if (numU64Allocati * 64 < numBit)
		numU64Allocati++;

	p = GOSALLOCT(u64*, allocator, numU64Allocati * sizeof(u64));
}

void Bitfield::unsetup (gos::Allocator *allocator)
{
	if (NULL != p)
	{
		GOSFREE(allocator, p);
		p = NULL;
		numU64Allocati = 0;
	}
}

void Bitfield::zero()
{
	assert (NULL != p);
	memset (p, 0, numU64Allocati * sizeof(u64));
}

void Bitfield::set (u32 pos)
{
	assert (NULL != p);
	assert (pos < numBit);

	const u32	byte = (pos >> 6); //pos / 64;
	const u32	bit  = (pos & 63); //pos % 64;
	const u64 	mask = (u64)1 << bit;
	p[byte] |= mask;
}

void Bitfield::clear (u32 pos)
{
	assert (NULL != p);
	assert (pos < numBit);

	const u32	byte = (pos >> 6); //pos / 64;
	const u32	bit  = (pos & 63); //pos % 64;
	const u64 	mask = (u64)1 << bit;
	p[byte] &= ~mask;
}

bool Bitfield::isBitSet (u32 pos) const
{
	assert (NULL != p);
	assert (pos < numBit);

	const u32	byte = (pos >> 6); //pos / 64;
	const u32	bit  = (pos & 63); //pos % 64;
	const u64 	mask = (u64)1 << bit;
	return ((p[byte] & mask) != 0);
}

bool Bitfield::findAndSetFirstFreeBit (u32 *out_pos) const
{
	assert (NULL != out_pos);
	for (u32 i=0; i<numU64Allocati; i++)
	{
		if (p[i] == u64MAX)
			continue;

		u64 mask = (u64)1;
		for (u32 t=0; t<64; t++)
		{
			if ((p[i] & mask) == 0)
			{
				p[i] |= mask;

				*out_pos = 64*i + t;
				if ((*out_pos) >= numBit)
					return false;

				return true;
			}
			mask <<= 1;
		}
	}

	return false;
}

bool Bitfield::findFirstFreeBit (u32 startBit, u32 *out_pos) const
{
	assert (NULL != out_pos);

	u32 byte = (startBit >> 6);
	u32 bit  = (startBit & 63);

	while (byte <numU64Allocati)
	{
		if (p[byte] != u64MAX)
		{
			u64 mask = (u64)1 << bit;
			while (bit < 64)
			{
				if ((p[byte] & mask) == 0)
				{
					*out_pos = 64*byte + bit;
					if ((*out_pos) >= numBit)
						return false;

					return true;
				}

				mask <<= 1;
				bit++;
			}
		}

		byte++;
		bit = 0;
	}

	return false;
}

bool Bitfield::findFirstSetBit (u32 startBit, u32 *out_pos) const
{
	assert (NULL != out_pos);

	u32 byte = (startBit >> 6);
	u32 bit  = (startBit & 63);

	while (byte <numU64Allocati)
	{
		if (p[byte] != 0)
		{
			u64 mask = (u64)1 << bit;
			while (bit < 64)
			{
				if ((p[byte] & mask) != 0)
				{
					*out_pos = 64*byte + bit;
					if ((*out_pos) >= numBit)
						return false;

					return true;
				}

				mask <<= 1;
				bit++;
			}
		}

		byte++;
		bit = 0;
	}

	return false;
}


