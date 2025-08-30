#include "gosUTF8String.h"
#include "../gos.h"


using namespace gos;

//*******************************************
void UTF8String::priv_constructor()
{
	allocator = NULL;
	buffer = NULL;
	allocatedSize = 0;
	curSize = 0;
}

//*******************************************
UTF8String::UTF8String (const UTF8String &b)				{ priv_constructor(); setAllocator(gos::getScrapAllocator()); append(b); }
UTF8String::UTF8String(const char* s)				{ priv_constructor(); setAllocator(gos::getScrapAllocator()); append(s); }


//*******************************************
UTF8String::~UTF8String()
{
	if (allocator && buffer)
	{
		GOSFREE(allocator, buffer);
	}
}

//*******************************************
void UTF8String::setAllocator (Allocator *allocIN)
{
	assert (NULL == buffer);
	allocator = allocIN;
}

//*******************************************
void UTF8String::prealloc (u32 newSizeInByte)
{
	if (newSizeInByte <= allocatedSize)
		return;

	allocatedSize = newSizeInByte;
	if (NULL == buffer)
	{
		if (NULL == allocator)
			allocator =  gos::getSysHeapAllocator();
		buffer = GOSALLOCT(char*,allocator, allocatedSize);
		buffer[0] = 0;
	}
	else
	{
		char *newstr = GOSALLOCT(char*,allocator, allocatedSize);
		if (curSize)
			memcpy (newstr, buffer, curSize);
		newstr[curSize] = 0;
		GOSFREE(allocator, buffer);
		buffer = newstr;
	}
}

//*******************************************
void UTF8String::append (const UTF8String &b, u32 lenInByte)
{
    if (u32MAX == lenInByte)
        append(b.buffer, b.curSize);
    else
        append(b.buffer, lenInByte);
}

//*******************************************
void UTF8String::append (const char *b, u32 lenInBytes)
{
	if (lenInBytes == 0)
		return;
	if (NULL == b)
		return;
	if (u32MAX == lenInBytes)
		lenInBytes = string::utf8::lengthInByte(b);
	if (0 == lenInBytes)
		return;

	prealloc (curSize + lenInBytes + 1);
	memcpy (&(buffer[curSize]), b, lenInBytes);
	curSize += lenInBytes;
	buffer[curSize] = 0;
}

//*******************************************
void UTF8String::insertNSpaces (u32 numSpaceToInsert)
{
	if (0 == numSpaceToInsert)
		return;

	prealloc (curSize + numSpaceToInsert + 1);
	memset (&(buffer[curSize]), ' ', numSpaceToInsert);
	curSize += numSpaceToInsert;
	buffer[curSize] = 0;
}

//*******************************************
i32 UTF8String::findFirst (const gos::UTF8Char &ch, u32 startIndex) const
{
	if (startIndex >= curSize)
		return -1;

	string::utf8::Iter iter;
	iter.setup (buffer, startIndex, curSize - startIndex);

	UTF8Char c;
	while (!(c = iter.getCurChar()).isEOF())
	{
		if (c == ch)
			return (i32)(startIndex + iter.getCursorPos());
		iter.advanceOneChar();
	}
	return -1;
}

//*******************************************
int UTF8String::compare (const UTF8String &b) const
{
	if (0 == lengthInByte())
	{
		if (0 == b.lengthInByte())
			return 0;
		return -1;
	}
	if (0 == b.lengthInByte())
		return 1;
	return strcmp (buffer, b.buffer);
}

//*******************************************
bool UTF8String::isEqualTo (const UTF8String &b, bool bCaseSensitive) const
{
	if (curSize != b.curSize)
		return false;
	if (curSize == 0)
		return true;
	return string::utf8::areEqualWithLen (buffer, b.buffer, bCaseSensitive, curSize);
}

//*******************************************
bool UTF8String::isEqualTo (const char *b, bool bCaseSensitive) const
{
	if (curSize == 0)
	{
		if (NULL == b || b[0] == 0x00)
			return true;
		return false;
	}

	if (NULL == b || b[0] == 0x00)
		return false;
	
	const u32 lenB = string::utf8::lengthInByte (b);
	if (lenB != curSize)
		return false;
	return string::utf8::areEqualWithLen (buffer, b, bCaseSensitive, curSize);
}

//*******************************************
bool UTF8String::isEqualToWithLen (const UTF8String &b, u32 lenInBytes, bool bCaseSensitive) const
{
	if (curSize < lenInBytes)
		return false;
	if (b.curSize < lenInBytes)
		return false;
	return string::utf8::areEqualWithLen (buffer, b.buffer, bCaseSensitive, lenInBytes);
}

//*******************************************
bool UTF8String::isEqualToWithLen (const char *b, u32 lenInBytes, bool bCaseSensitive) const
{
	if (curSize < lenInBytes)
		return false;


	const u32 lenB = string::utf8::lengthInByte (b);
	if (lenB < lenInBytes)
		return false;
	
	return string::utf8::areEqualWithLen (buffer, b, bCaseSensitive, lenInBytes);
}

//*******************************
u32 UTF8String::explode (const UTF8Char &cTofind, Array<UTF8String> &out) const
{
	if (lengthInByte() == 0)
		return 0;

	u32 nStartElem = out.getNElem();
	u32 nFound = 0;
	u32 iStartByte = 0;
	
	string::utf8::Iter src;
	src.setup (buffer, 0, lengthInByte());
	
	while (string::utf8::advanceUntil (src, &cTofind, 1))
	{
		out[nStartElem++].setFrom (&buffer[iStartByte], src.getCursorPos() - iStartByte);
		++nFound;

		src.advanceOneChar();
		iStartByte = src.getCursorPos();
	}
	

	if (iStartByte < lengthInByte())
	{
		const u32 bytesToCopy = lengthInByte() - iStartByte;
		out[nStartElem].setFrom (&buffer[iStartByte], bytesToCopy);
		++nFound;
	}

	return nFound;
}

//*******************************
void UTF8String::trimL()
{
	if (lengthInByte() == 0)
		return;

	u32 i = 0;
	while (buffer[i]==' ')
		++i;
	if (buffer[i]==0x00)
		clear();
	else
	{
		if (i>0)
		{
			memmove (buffer, &buffer[i], curSize -i);
			curSize -= i;
			buffer[curSize] = 0;
		}
	}
}

//*******************************
void UTF8String::trimR()
{
	if (lengthInByte() == 0)
		return;

	u32 i = curSize -1;
	while (i>0 && buffer[i]==' ')
		--i;
	if (buffer[i]==' ')
		curSize = i;
	else
		curSize = i+1;
	buffer[curSize] = 0;
}

//*******************************
void UTF8String::sanitizePath()
{
	if (curSize == 0)
		return;
	gos::fs::pathSanitizeInPlace (this->buffer, curSize);
	curSize = gos::string::utf8::lengthInByte(this->buffer);	
}

//*******************************
void UTF8String::escape()
{
	if (0 == curSize)
		return;

	const u32 newSize = 4 + gos::string::utf8::calcEscapedSeqLength (buffer, curSize);
	if (NULL == allocator)
		allocator =  gos::getSysHeapAllocator();

	char *newBuffer = GOSALLOCT(char*, allocator, newSize);
	curSize = gos::string::utf8::escape (newBuffer, newSize, buffer, curSize);
	GOSFREE(allocator, buffer);
	buffer = newBuffer;
}

//*******************************
void UTF8String::unescape()
{
	if (0 == curSize)
		return;

	const u32 newSize = curSize;
	if (NULL == allocator)
		allocator =  gos::getSysHeapAllocator();

	char *newBuffer = GOSALLOCT(char*, allocator, newSize);
	curSize = gos::string::utf8::unescape (newBuffer, newSize, buffer, curSize);
	GOSFREE(allocator, buffer);
	buffer = newBuffer;
}

//*******************************
void UTF8String::escapeTo (UTF8String *out) const
{
	if (0 == curSize)
	{
		out->clear();
		return;
	}

	const u32 newSize = 4 + gos::string::utf8::calcEscapedSeqLength (buffer, curSize);
	out->prealloc(newSize);
	out->curSize = gos::string::utf8::escape (out->buffer, newSize, buffer, curSize);
}

//*******************************
void UTF8String::unescapeTo (UTF8String *out) const
{
	if (0 == curSize)
	{
		out->clear();
		return;
	}

	const u32 newSize = curSize;
	out->prealloc(newSize);
	out->curSize = gos::string::utf8::unescape (out->buffer, newSize, buffer, curSize);
}

//*******************************
void UTF8String::fillRowUntilColumn (u32 column, char filler)
{
	//a partire da buffer[curSize], vado indietro fino a trovare l'inizio di questa riga
	u32 ctFirstCharOfRow = curSize;
	while (ctFirstCharOfRow)
	{
		if (buffer[ctFirstCharOfRow] == '\n')
		{	
			ctFirstCharOfRow++;
			break;
		}

		ctFirstCharOfRow--;
	}

	//il primo carattere della riga inizia in buffer[ct]
	const u32 ctlastCharOfRow = curSize;
	u32 rowSize = ctlastCharOfRow - ctFirstCharOfRow;
	while (rowSize++ < column)
	{
		append(filler);
	}
	
}