#include "../gosString.h"
#include "../gos.h"

using namespace gos;


//**************************************************
void string::utf8::Iter::setup (const char *src, u32 firstByte, u32 lenghtInBytes)
{
	setup(reinterpret_cast<const u8*>(src), firstByte, lenghtInBytes);
}
void string::utf8::Iter::setup (const u8 *utf8_src, u32 firstByteIN, u32 lenghtInBytesIN)
{
	utf8_seq = NULL;
	seq_length = 0;
	curChar.setEOF();
	cursorPos = 0;
	bytesUsedForCurChar = 0;

	if (NULL == utf8_src)
		return;
	if (0x00 == utf8_src[firstByteIN])
		return;

	utf8_seq = &utf8_src[firstByteIN];
	if (lenghtInBytesIN == u32MAX)
		seq_length = string::utf8::lengthInBytes(utf8_seq);
	else
		seq_length = lenghtInBytesIN;

	toStart();
}

//**************************************************
void string::utf8::Iter::toStart()
{
	cursorPos = 0;
	priv_detectCurrentChar();
}

//**************************************************
bool string::utf8::Iter::priv_detectCurrentChar()
{
	if (cursorPos >= seq_length)
	{
		curChar.setEOF();
		cursorPos = seq_length;
		bytesUsedForCurChar = 0;
		return false;
	}

	if (!curChar.setFromConstChar ((const char*)&utf8_seq[cursorPos], &bytesUsedForCurChar))
	{
		curChar.setEOF();
		bytesUsedForCurChar = 0;
		return false;
	}
	return true;
}


//**************************************************
bool string::utf8::Iter::advanceOneChar()
{
	if (cursorPos + bytesUsedForCurChar >= seq_length)
	{
		curChar.setEOF();
		cursorPos = seq_length;
		bytesUsedForCurChar = 0;
		return false;
	}

	cursorPos += bytesUsedForCurChar;
	return priv_detectCurrentChar();
}

//**************************************************
bool string::utf8::Iter::backOneChar()
{
	if (cursorPos == 0 || seq_length==0)
	{
		curChar.setEOF();
		cursorPos = 0;
		bytesUsedForCurChar = 0;
		return false;
	}

	/* ci sono 4 possibili codifiche per un utf8
		0xxxxxxx
		110xxxxx 10xxxxxx
		1110xxxx 10xxxxxx 10xxxxxx
		11110xxx 10xxxxxx 10xxxxxx 10xxxxxx

		In pratica quindi, un utf8 char inizia da un byte che NON deve avere 10xxxxxx
	*/
	u32 p = cursorPos;

	for (u8 i=0; i<4; i++)
	{
		if (p > 0)
		{
			p--;
			if ((utf8_seq[p] & 0xC0) != 0x80)
			{
				//ho trovato un valido char d'inizio
				if (curChar.setFromConstChar ((const char*)&utf8_seq[p], &bytesUsedForCurChar))
				{
					cursorPos = p;
					return true;
				}
			}
		}
	}

	//se arrivo qui, vuol dire che non ho trovato una valida seq utf8 esaminando fino a 4 byte all'indietro
	//Potrebbe essere il caso che c'� un Latin1
	p = cursorPos;
	if (p>0)
	{
		p--;
		if (curChar.setFromConstChar ((const char*)&utf8_seq[p], &bytesUsedForCurChar))
		{
			cursorPos = p;
			return true;
		}
	}


	cursorPos = 0;
	return priv_detectCurrentChar();
}

//**************************************************
const u8 *string::utf8::Iter::getPointerToCurrentPosition() const
{
	if (curChar.isEOF())
		return NULL;
	return &utf8_seq[cursorPos];
}

//**************************************************
u32 string::utf8::Iter::copyAllStr(u8 *out, u32 sizeofOut) const
{
	assert (NULL != out && sizeofOut > 0);
	if (seq_length +1  >= sizeofOut)
	{
		DBGBREAK;
		return 0;
	}

	memcpy (out, utf8_seq, seq_length);
	out[seq_length] = 0;
	return seq_length;
}

//**************************************************
u32 string::utf8::Iter::copyStrFromCurrentPositionToEnd (u8 *utf8_out, u32 sizeofOut) const
{
	assert (NULL != utf8_out && sizeofOut > 0);
	u32 nToCopy = getBytesLeft();
	if (0 == nToCopy)
	{
		utf8_out[0] = 0;
		return 0;
	}

	if (nToCopy+1 >= sizeofOut)
	{
		DBGBREAK;
		utf8_out[0] = 0;
		return 0;
	}
	memcpy (utf8_out, getPointerToCurrentPosition(), nToCopy);
	utf8_out[nToCopy] = 0;
	return nToCopy;
}

//**************************************************
u32 string::utf8::Iter::copyStrFromXToCurrentPosition(u32 startingCursorPos, u8 *out, u32 sizeofOut, bool bIncludeCurrentChar) const
{
	assert (NULL != out && sizeofOut > 0);

	u32 nToCopy;
	if (bIncludeCurrentChar)
		nToCopy = (cursorPos + bytesUsedForCurChar) - startingCursorPos;
	else
		nToCopy = cursorPos - startingCursorPos;
	if (nToCopy+1 >= sizeofOut)
	{
		DBGBREAK;
		return 0;
	}
	memcpy (out, &utf8_seq[startingCursorPos], nToCopy);
	out[nToCopy] = 0;
	return nToCopy;
}

//**************************************************
void string::utf8::Iter::toLast()
{
	curChar.setEOF();
	cursorPos = seq_length;
	bytesUsedForCurChar = 0;
	backOneChar();
}