#include "../gosString.h"
#include "../gos.h"
#include <stdarg.h>

using namespace gos;

namespace gos
{
	namespace string
	{
		namespace utf8
		{
			const UTF8Char	CHAR_ARRAY_b_r_n_t[4] = { UTF8Char(" "), UTF8Char("\r"), UTF8Char("\n"), UTF8Char("\t") };		
			const UTF8Char	SECTION_SIGN (0xc2, 0xa7);
		}
	}
}

//***************************************************************
bool string::utf8::toUTF16  (const UTF8Char &in, UTF16Char *out)
{ 
	UTF32Char temp; 
	if (utf8::toUTF32(in, &temp)) 
		return utf32::toUTF16(temp, out);
	return false; 
}

//***************************************************************
bool string::utf8::toUTF32 (const UTF8Char &in, UTF32Char *out)
{
	if (in.data[0] == 0)
	{
		out->data = 0;
		return true;
	}

	// The first byte is a special case.
	if ((in.data[0] & 0x80) == 0)
	{
		out->data = in.data[0];
		return true;
	}

	// We are in a multi-byte sequence.  get bits from the top, starting from the second bit.
	u32 ret = in.data[0];
	u32 I = 0x20;
	u32 ui32Len = 2;
	u32 ui32Mask = 0xC0;
	while (ret & I)
	{
		// Add this bit to the mask to be removed later.
		ui32Mask |= I;
		I >>= 1;
		++ui32Len;

		if (I == 0)
		{
			// Invalid sequence.
			DBGBREAK;
			return false;
		}
	}

	// Mask out the leading bits.
	ret &= ~ui32Mask;

	// For every trailing bit, add it to the final value.
	u8 iNext = 1;
	for (I = ui32Len - 1UL; I--; )
	{
		if (in.data[iNext] == 0x00)
		{
			//apparentemente mi aspettavo che la codifica utf8 necessitasse di pi� byte di quanti ne ho ricevuti in input
			DBGBREAK;
			return false;
		}
		ret <<= 6UL;
		ret |= (in.data[iNext++] & 0x3F);
	}


	out->data = ret;
	return true;
}

//**************************************************
u32 string::utf8::lengthInByte (const u8 *utf8_str)
{
	if (NULL == utf8_str)
		return 0;

	//il primo 0x00 che trovo...
	u32 i = 0;
	while (utf8_str[i] != 0x00)
	{
		i++;
#ifdef _DEBUG
		//se siamo arrivati cos� avanti, prob � un errore
		if (i >= 16384)
			DBGBREAK;
#endif
	}
	return i;
}

//**************************************************
u32 string::utf8::sanitize (const u8 *utf8IN, u32 numBytesInUT8IN, const UTF8Char &useThisWhenInvalidChar, u8 *out, u32 sizeof_out)
{
	assert (NULL != utf8IN);
	assert (NULL != out);
	assert (sizeof_out > 0);

	if (numBytesInUT8IN == 0)
		return 0;

	u32 i = 0;
	u32 iOut = 0;

	while (i < numBytesInUT8IN)
	{
		if (0 == utf8IN[i])
		{
			out[iOut++] = 0x00;
			break;
		}

		UTF8Char ch;
		const u8 nConsumed = string::utf8::extractAChar (&utf8IN[i], numBytesInUT8IN, &ch);
		if (0 == nConsumed)
		{
			//ch invalido
			i++;
			ch = useThisWhenInvalidChar;
		}
		else
		{
			i+=nConsumed;
		}

		//appendo a out
		const u8 n = ch.length();
		if (iOut + n > sizeof_out)
		{
			break;
		}
		else
		{
			memcpy (&out[iOut], ch.data, n);
			iOut += n;
		}
	}

	return iOut;
}

//**************************************************
u8 string::utf8::extractAChar (const u8 *p, u32 lenInBytes, UTF8Char *out)
{
	if (NULL == p || lenInBytes<1)
		return 0;
	
	out->data[0] = out->data[1] = out->data[2] = out->data[3] = 0;
	if (p[0] == 0x00)
		return 0;

	if ((p[0] & 0x80) == 0)
	{
		out->data[0] = p[0];
		return 1;
	}

	//a questo punto il char UTF8 consiste di almeno 2 byte
	if (lenInBytes < 2)
	{
		//DBGBREAK; //seq invalida
		return 0;
	}
	

	//c1 deve essere di tipo 0x10xxxxxx
	if ((p[1] & 0xC0) != 0x80)
	{
		//DBGBREAK; //seq invalida
		return 0;
	}
	
	if ((p[0]  & 0xE0) == 0xC0)
	{
		//siamo nel caso c0=110xxxxx c1=10xxxxxx
		out->data[0] = p[0];
		out->data[1] = p[1];
		return 2;
	}


	else if ((p[0]  & 0xF0) == 0xE0)
	{
		//siamo nel caso p[0]=1110xxxx p[1]=10xxxxxx p[2]=10xxxxxx
		if (lenInBytes < 3)
		{
			//DBGBREAK; //seq invalida
			return 0;
		}
		
		//p[2] deve essere di tipo 0x10xxxxxx
		if ((p[2] & 0xC0) != 0x80)
		{
			//DBGBREAK; //seq invalida
			return 0;
		}

		out->data[0] = p[0];
		out->data[1] = p[1];
		out->data[2] = p[2];
		return 3;
	}


	else if ((p[0]  & 0xF8) == 0xF0)
	{
		//siamo nel caso p[0]=11110xxx p[1]=10xxxxxx p[2]=10xxxxxx p[3]=10xxxxxx
		if (lenInBytes < 4)
		{
			//DBGBREAK; //seq invalida
			return 0;
		}		
		//p[2] deve essere di tipo 0x10xxxxxx
		if ((p[2] & 0xC0) != 0x80)
		{
			//DBGBREAK; //seq invalida
			return 0;
		}		
		//p[3] deve essere di tipo 0x10xxxxxx
		if ((p[3] & 0xC0) != 0x80)
		{
			//DBGBREAK; //seq invalida
			return 0;
		}

		out->data[0] = p[0];
		out->data[1] = p[1];
		out->data[2] = p[2];
		out->data[3] = p[3];
		return 0;
	}

	//DBGBREAK; //seq invalida
	return 0;
}


//**************************************************
u8* string::utf8::allocStr (Allocator *allocator, const char* src, u32 numBytesDaUtilizzare)
{
    return utf8::allocStr (allocator, reinterpret_cast<const u8 *>(src), numBytesDaUtilizzare);
}


//**************************************************
u8* string::utf8::allocStr (Allocator *allocator, const u8 *src, u32 numBytesDaUtilizzare)
{
	assert (allocator && src);

	if (u32MAX == numBytesDaUtilizzare)
        numBytesDaUtilizzare = string::utf8::lengthInByte (src);
	if (0 == numBytesDaUtilizzare)
		return NULL;
	
    u8 *ret = GOSALLOCT (u8*, allocator, numBytesDaUtilizzare+1);
	memcpy (ret, src, numBytesDaUtilizzare);
	ret[numBytesDaUtilizzare] = 0x00;
	return ret;
}


//**************************************************
u32 string::utf8::makeStr (u8 *dst, u32 sizeofDst, const char* src)
{
	assert (dst && sizeofDst);
	dst[0] = 0;

	u32 i = 0;
	u32 n = 0;
	while (1)
	{
		gos::UTF8Char utf8char;
		u8 nBytesConsumed;
		
		if (!utf8char.setFromConstChar (&src[i], &nBytesConsumed))
		{
			//sequenza invalida
			DBGBREAK;
			dst[n] = 0;
			return n;
		}

		const u8 len = utf8char.length();
		if (len == 0)
			break;
		if (n+len >=sizeofDst)
		{
			DBGBREAK;
			dst[n] = 0;
			return n;
		}

		memcpy (&dst[n], utf8char.data, len);
		n += len;
		i += nBytesConsumed;
	}

	dst[n] = 0x00;
	return n;
}

//**************************************************
u32 string::utf8::copyStr (u8 *dst, u32 sizeofDst, const u8 *src, u32 numBytesDaUtilizzare)
{
	assert (dst && sizeofDst);

	if (NULL == src)
	{
		dst[0] = 0;
		return 0;
	}
	if (src[0] == 0x00)
	{
		dst[0] = 0;
		return 0;
	}

	if (u32MAX == numBytesDaUtilizzare)
		numBytesDaUtilizzare = utf8::lengthInByte (src);
	
	if (sizeofDst > numBytesDaUtilizzare)
	{
		memcpy (dst, src, numBytesDaUtilizzare);
		dst[numBytesDaUtilizzare] = 0;
		return numBytesDaUtilizzare;
	}

	DBGBREAK;
	return 0;
}

//**************************************************
u32 string::utf8::copyStrAsMuchAsYouCan (u8 *dst, u32 sizeOfDest, const u8 *src)
{
    if (NULL == dst)
        return 0;
    if (0 == sizeOfDest)
        return 0;

    if (NULL == src)
    {
        dst[0] = 0;
        return 0;
    }

    u32 srcLen = string::utf8::lengthInByte(src);
    if (0 == srcLen)
    {
        dst[0] = 0;
        return 0;
    }

    if (sizeOfDest >= (srcLen+1))
    {
        memcpy (dst, src, srcLen+1);
        return srcLen;
    }

    sizeOfDest--;
    if (sizeOfDest)
        memcpy (dst, src, sizeOfDest);
    dst[sizeOfDest] = 0;
    return sizeOfDest;
}

//**************************************************
u32 string::utf8::concatStr (u8 *dst, u32 sizeofDst, const char* src)
{
	u32 n = string::utf8::lengthInByte(dst);
	return n + utf8::makeStr (&dst[n], sizeofDst - n, src);
}

//**************************************************
u32 string::utf8::concat (u8 *dst, u32 sizeofDst, const UTF8Char &c)
{
	u32 n = string::utf8::lengthInByte(dst);
	if (n + c.length() >= sizeofDst)
	{
		DBGBREAK;
		return n;
	}

	memcpy (&dst[n], c.data, c.length());
	n += c.length();
	dst[n] = 0;
	return n;
}

//**************************************************
bool string::utf8::areEqual (const u8 *a, const char *b, bool bCaseSensitive)										{ return string::utf8::areEqual (a, reinterpret_cast<const u8*>(b), bCaseSensitive); }
bool string::utf8::areEqual (const u8 *a, const u8 *b, bool bCaseSensitive)							
{ 
	assert (NULL != a && NULL != b);
	
	const u32 lenA = string::utf8::lengthInByte (a);
	if (lenA != string::utf8::lengthInByte (b))
		return false;
	return string::utf8::areEqualWithLen (a, b, bCaseSensitive, lenA);
}

//**************************************************
bool string::utf8::areEqualWithLen (const u8 *a, const char *b, bool bCaseSensitive, u32 numBytesToCompare)			{ return string::utf8::areEqualWithLen (a, reinterpret_cast<const u8*>(b), bCaseSensitive, numBytesToCompare); }
bool string::utf8::areEqualWithLen (const u8 *a, const u8 *b, bool bCaseSensitive, u32 numBytesToCompare)
{
	if (bCaseSensitive) 
        return ( strncmp (reinterpret_cast<const char*>(a), reinterpret_cast<const char*>(b), numBytesToCompare) == 0);

    return (strncasecmp (reinterpret_cast<const char*>(a), reinterpret_cast<const char*>(b), numBytesToCompare) == 0);
}

//**************************************************
bool string::utf8::isCharMaiuscolo (const UTF8Char &c)
{
	if (c.length() == 1 && c.data[0] >= 'A' && c.data[0] <= 'Z')
		return true;
	return false;
}

//**************************************************
bool string::utf8::isCharMinuscolo (const UTF8Char &c)
{
	if (c.length() == 1 && c.data[0] >= 'a' && c.data[0] <= 'z')
		return true;
	return false;
}

//**************************************************
bool string::utf8::isANumber (const UTF8Char &c)
{
	if (c.length() == 1 && c.data[0] >= '0' && c.data[0] <= '9')
		return true;
	return false;
}

//**************************************************
bool string::utf8::isALetter (const UTF8Char &c)
{
    if (c.length() == 1 && ((c.data[0] >= 'A' && c.data[0] <= 'Z') || (c.data[0] >= 'a' && c.data[0] <= 'z')))
		return true;
	return false;
}

//**************************************************
bool string::utf8::isOneOfThis (const UTF8Char &c, const UTF8Char *validChars, u32 numOfValidChars)
{
	if (numOfValidChars == 0 || NULL == validChars)
		return false;
	for (u32 i2=0; i2<numOfValidChars; i2++)
	{
		if (c == validChars[i2])
			return true;
	}
	return false;
}

//**************************************************
void string::utf8::skip (Iter &src, const UTF8Char *toBeskippedChars, u32 numOfToBeskippedChars)
{
	while (!src.getCurChar().isEOF())
	{
		if (utf8::isOneOfThis (src.getCurChar(), toBeskippedChars, numOfToBeskippedChars))
			src.advanceOneChar();
		else
			return;
	}
}

//**************************************************
void string::utf8::skipEOL (Iter &src)
{
	while (!src.getCurChar().isEOF())
	{
		if (src.getCurChar() == '\n' || src.getCurChar() == '\r')
			src.advanceOneChar();
		else
			return;
	}
}

//**************************************************
bool string::utf8::advanceUntil (Iter &src, const UTF8Char *validTerminators, u32 numOfValidTerminators)
{
	while (!src.getCurChar().isEOF())
	{
		if (utf8::isOneOfThis (src.getCurChar(), validTerminators, numOfValidTerminators))
			return true;
		src.advanceOneChar ();
	}
	return false;
}

//*****************************************
void string::utf8::advanceToEOL (Iter &src, bool bskipEOL)
{
	gos::UTF8Char c;
	while (!(c = src.getCurChar()).isEOF())
	{
		if (c == '\n' || c == '\r')
			break;
		src.advanceOneChar();
	}

	if (bskipEOL)
	{
		while (!(c = src.getCurChar()).isEOF())
		{
			if (c == '\n' || c == '\r')
				src.advanceOneChar();
			else

				break;
		}
	}
}

//**************************************************
bool string::utf8::find (Iter &src, const char *whatToFind)		{ return utf8::find (src, reinterpret_cast<const u8*>(whatToFind)); }
bool string::utf8::find (Iter &src, const u8 *whatToFind)
{
	if (NULL == whatToFind)
		return false;
	const u32 whatToFindLEN = utf8::lengthInByte(whatToFind);
	if (0 == whatToFindLEN)
		return false;


	Iter iterWhat;
	iterWhat.setup (whatToFind, 0, whatToFindLEN);

	gos::UTF8Char c;
	while (!(c = src.getCurChar()).isEOF())
	{
		if (c != iterWhat.getCurChar())
		{
			src.advanceOneChar();
			continue;
		}

		//ho trovato un ch di [src] che � == al primo ch di [whatToFind].
		//Ora faccio un memcmp
		const u32 bytesLeftSRC = src.getBytesLeft();
		if (bytesLeftSRC >= whatToFindLEN)
		{
			if (0 == memcmp (src.getPointerToCurrentPosition(), whatToFind, whatToFindLEN))
				return true;
		}
		
		src.advanceOneChar();
	}

	return false;
}

//**************************************************
void string::utf8::extractLine (Iter &srcIN, Iter *out_result)
{
	assert (out_result);
	Iter src = srcIN;
	
	gos::UTF8Char c;
	while ( !(c=src.getCurChar()).isEOF() )
	{
		if (c == '\n' || c == '\r')
			break;
		src.advanceOneChar();
	}

	//metto in out la linea che ho trovato
	out_result->setup (srcIN.getPointerToCurrentPosition(), 0, src.getCursorPos() - srcIN.getCursorPos());
	
	//avanzo per skippare i \n e\o \r
	while ( !(c=src.getCurChar()).isEOF() )
	{
		if (c == '\n' || c == '\r')
			src.advanceOneChar();
		else
			break;
	}

	//aggiorno srcIN
	srcIN = src;
}

//**************************************************
bool string::utf8::extractValue (string::utf8::Iter &srcIN, string::utf8::Iter *out_result, const gos::UTF8Char *validClosingChars, u32 numOfValidClosingChars)
{
	string::utf8::Iter src = srcIN;
	if (src.getCurChar()=='"' || src.getCurChar()=='\'')
	{
		//il value � racchiuso tra apici. Prendo tutto fino a che non trovo un'altro apice uguale all'apice di apertura
		UTF8Char opening = src.getCurChar();
		src.advanceOneChar();

		while (1)
		{
			const u32 cursorPos = src.getCursorPos();
			if (!utf8::advanceUntil (src, &opening, 1))
				//non ho trovato l'opening finale
				return false;

			//ho trovato l'opening, verifico che non sia una escape sequence
			if (src.getCursorPos() > cursorPos + 1)
			{
				src.backOneChar();
				if (src.getCurChar().isEqual ('\\'))
				{
					src.advanceOneChar();
					src.advanceOneChar();
					continue;
				}
				src.advanceOneChar();
			}
		
			//tutto ok
			src.advanceOneChar();
			break;
		}

		srcIN.advanceOneChar(); //per skippare l'apice iniziale
		out_result->setup (srcIN.getPointerToCurrentPosition (), 0, src.getCursorPos () - srcIN.getCursorPos () - 1);
		srcIN = src;
		return true;
	}
	else
	{
		// l'identificatore non � racchiuso tra apici per cui prendo tutto fino a che trovo un validClosingChars o fine buffer

		//il primo char per� non deve essere uno spazio
		if (utf8::isOneOfThis (src.getCurChar(), CHAR_ARRAY_b_r_n_t, 4))
			return false;

		if (!utf8::advanceUntil (src, validClosingChars, numOfValidClosingChars))
		{
			//se non ho trovato un valido sep, ma sono a fine buffer, va bene lo stesso
			if (!src.getCurChar().isEOF())
				return false;
		}

		out_result->setup (srcIN.getPointerToCurrentPosition (), 0, src.getCursorPos () - srcIN.getCursorPos ());
		srcIN = src;
		return true;
	}
}

//**************************************************
bool string::utf8::extractIdentifier (Iter &srcIN, Iter *out_result, const UTF8Char *otherValidChars, u32 numOfOtherValidChars)
{
	Iter	src = srcIN;

	//il primo char potrebbe essere "_"
	if (src.getCurChar() == '_')
		src.advanceOneChar();

	//qui deve esserci una lettera
	if (!utf8::isALetter(src.getCurChar()))
		return false;
	src.advanceOneChar();

	while (!src.getCurChar().isEOF())
	{
		if (src.getCurChar()=='_' || utf8::isALetterOrANumber(src.getCurChar()) || utf8::isOneOfThis (src.getCurChar(), otherValidChars, numOfOtherValidChars))
		{
			src.advanceOneChar();
			continue;
		}
		break;
	}

	//ho trovato fine buffer o un char invalido, quindi va tutto bene
	out_result->setup (srcIN.getPointerToCurrentPosition (), 0, src.getCursorPos () - srcIN.getCursorPos ());
	srcIN = src;
	return true;
}

//**************************************************
bool string::utf8::extractFloat (Iter &srcIN, f32 *out, const UTF8Char &sepDecimale, const UTF8Char *validClosingChars, u32 numOfValidClosingChars)
{
	assert (sepDecimale != ' ');

	Iter	src = srcIN;
	bool	sepDecimaleFound = false;
	bool	eFound = false;

	//il primo char potrebbe essere il sepDecimale
	if (src.getCurChar() == sepDecimale)
	{
		sepDecimaleFound = true;
		src.advanceOneChar();
	}
	// oppure potrebbe essere un +/-
	else if (src.getCurChar() == '+' || src.getCurChar() == '-')
		src.advanceOneChar();

	//qui deve esserci un numero
	if (!utf8::isANumber(src.getCurChar()))
		return false;
	src.advanceOneChar();


	//da ora in poi, devono essere tutti numeri, sono ammessi anche un solo sepDecimale, e un "e" o "E" se siamo nella parte intera
	UTF8Char c;
	while ( !(c=src.getCurChar()).isEOF() )
	{
		if (utf8::isANumber(c))
		{
			src.advanceOneChar();
			continue;
		}
		if (c == sepDecimale)
		{
			if (sepDecimaleFound)
				return false;
			if (eFound)
				return false;
			sepDecimaleFound = true;
			src.advanceOneChar();
			continue;
		}
		if (utf8::isOneOfThis (src.getCurChar(), validClosingChars, numOfValidClosingChars))
			break;

		if (c == 'e' || c=='E')
		{
			if (eFound)
				return false;
			eFound = true;

			//subito dopo potrebbe esserci un +/-
			src.advanceOneChar();
			if (src.getCurChar() == '+' || src.getCurChar() == '-')
				src.advanceOneChar();

			//qui deve esserci un numero
			if (!utf8::isANumber(src.getCurChar()))
				return false;
			src.advanceOneChar();
			continue;
		}
		break;
	}

	//ho trovato fine buffer o un char invalido.
	//se il char � un valido separatore, ok, altrimenti � un errore
	if (!src.getCurChar().isEOF() && !utf8::isOneOfThis (src.getCurChar(), validClosingChars, numOfValidClosingChars))
		return false;

	//se arrivo qui vuol dire che la stringa conteneva un num valido e curChar() punta al separatore o a fine buffer
	const u32 MAXTEMP = 64;
	u8 temp[MAXTEMP];
    src.copyStrFromXToCurrentPosition (srcIN.getCursorPos(), temp, MAXTEMP, false);

	//converto in float
	*out = string::utf8::toF32 (temp);
	srcIN = src;
	return true;
}

//*****************************************
bool string::utf8::extractFloatArray (Iter &srcIN, f32 *out, u32 *maxFloatIN_Out, const UTF8Char &sepDecimale, const UTF8Char &arraySeparator)
{
	assert (*maxFloatIN_Out > 0);
	assert (sepDecimale!=' ' && arraySeparator!=sepDecimale);
	
	u32 nOUT = 0;
	Iter src = srcIN;

	//parso
	UTF8Char floatEndingChar[5] = { UTF8Char(' '), UTF8Char('\t'), arraySeparator, UTF8Char('\r'), UTF8Char('\n')};
	UTF8Char eol[3] = { UTF8Char('\r'), UTF8Char('\n') };
	UTF8Char array_b_t[2] = { UTF8Char(' '), UTF8Char('\t') };
	
	while (nOUT < *maxFloatIN_Out)
	{
		utf8::skip (src, array_b_t, 2);
		if (!utf8::extractFloat (src, &out[nOUT], sepDecimale, floatEndingChar, 5))
			break;
		++nOUT;

		//cerco il prossimo array separator
		if (src.getCurChar() != arraySeparator)
			utf8::skip (src, array_b_t, 2);

		if (src.getCurChar().isEOF() || utf8::isOneOfThis (src.getCurChar(), eol, 2) )
			break;
		if (src.getCurChar() != arraySeparator)
			return false;
		src.advanceOneChar();

		if (nOUT >= *maxFloatIN_Out)
			break;
	}

	*maxFloatIN_Out = nOUT;
	if (nOUT == 0)
		return false;
	srcIN = src;
	return true;

}

//*****************************************
bool string::utf8::extractI32 (Iter &srcIN, i32 *out, const UTF8Char *validClosingChars, u32 numOfValidClosingChars)
{
	Iter src = srcIN;
	
	utf8::toNextValidChar( src );

	//il primo char potrebbe essere +/-
	if (src.getCurChar() == '+' || src.getCurChar() == '-')
		src.advanceOneChar();

	//qui deve esserci un numero
	if (!utf8::isANumber(src.getCurChar()))
		return false;
	src.advanceOneChar();


	//da ora in poi, devono essere tutti numeri
	UTF8Char c;
	while ( !(c=src.getCurChar()).isEOF() )
	{
		if (utf8::isANumber(c))
		{
			src.advanceOneChar();
			continue;
		}
		
		//se non � un numero ma � uno dei validi separatori, va bene lo stesso
		if (utf8::isOneOfThis (src.getCurChar(), validClosingChars, numOfValidClosingChars))
			break;

		return false;
	}


	//se arrivo qui vuol dire che la stringa conteneva un num valido e curChar() punta al separatore o a fine buffer
	const u32 MAXTEMP = 64;
	u8 temp[MAXTEMP];
    src.copyStrFromXToCurrentPosition (srcIN.getCursorPos(), temp, MAXTEMP, false);

	//converto in float
	const i64 v = string::utf8::toI64 (temp);
	if (v < i32MIN)
	{
		*out = i32MIN;
		DBGBREAK;
	}
	else if (v > i32MAX)
	{
		*out = i32MAX;
		DBGBREAK;
	}
	else
		*out = static_cast<i32>(v);
	srcIN = src;
	return true;
}

//*****************************************
bool string::utf8::extractU32 (Iter &srcIN, u32 *out, const UTF8Char *validClosingChars, u32 numOfValidClosingChars)
{
	Iter src = srcIN;
	
	utf8::toNextValidChar( src );

	//il primo char potrebbe essere +/-
	if (src.getCurChar() == '+' || src.getCurChar() == '-')
		src.advanceOneChar();

	//qui deve esserci un numero
	if (!utf8::isANumber(src.getCurChar()))
		return false;
	src.advanceOneChar();


	//da ora in poi, devono essere tutti numeri
	UTF8Char c;
	while ( !(c=src.getCurChar()).isEOF() )
	{
		if (utf8::isANumber(c))
		{
			src.advanceOneChar();
			continue;
		}
		
		//se non � un numero ma � uno dei validi separatori, va bene lo stesso
		if (utf8::isOneOfThis (src.getCurChar(), validClosingChars, numOfValidClosingChars))
			break;

		return false;
	}


	//se arrivo qui vuol dire che la stringa conteneva un num valido e curChar() punta al separatore o a fine buffer
	const u32 MAXTEMP = 64;
	u8 temp[MAXTEMP];
    src.copyStrFromXToCurrentPosition (srcIN.getCursorPos(), temp, MAXTEMP, false);

	//converto in float
	const i64 v = string::utf8::toI64 (temp);
	if (v < 0)
	{
		*out = 0;
		DBGBREAK;
	}
	else if (v > u32MAX)
	{
		*out = u32MAX;
		DBGBREAK;
	}
	else
		*out = static_cast<u32>(v);
	srcIN = src;
	return true;
}

//*****************************************
bool string::utf8::extractI32Array (Iter &srcIN, i32 *out, u32 *maxIntIN_Out, const UTF8Char &arraySeparator)
{
	assert (*maxIntIN_Out > 0);
	assert (arraySeparator!=' ');
	
	u32 nOUT = 0;
	Iter src = srcIN;

	//parso
	UTF8Char intEndingChar[3] = { UTF8Char(' '), UTF8Char('\t'), arraySeparator };
	UTF8Char array_b_t[2] = { UTF8Char(' '), UTF8Char('\t') };

	while (nOUT < *maxIntIN_Out)
	{
		utf8::skip (src, array_b_t, 2);
		if (!utf8::extractI32 (src, &out[nOUT], intEndingChar, 3))
			break;
		++nOUT;

		//cerco il prossimo array separator
		if (src.getCurChar() != arraySeparator)
			utf8::skip (src, array_b_t, 2);

		if (src.getCurChar().isEOF())
			break;
		if (src.getCurChar() != arraySeparator)
			return false;
		src.advanceOneChar();

		if (nOUT >= *maxIntIN_Out)
			break;
	}

	*maxIntIN_Out = nOUT;
	if (nOUT == 0)
		return false;
	srcIN = src;
	return true;
}

//*****************************************
bool string::utf8::extractU32Array (Iter &srcIN, u32 *out, u32 *maxIntIN_Out, const UTF8Char &arraySeparator)
{
	assert (*maxIntIN_Out > 0);
	assert (arraySeparator!=' ');
	
	u32 nOUT = 0;
	Iter src = srcIN;

	//parso
	UTF8Char intEndingChar[3] = { UTF8Char(' '), UTF8Char('\t'), arraySeparator };
	UTF8Char array_b_t[2] = { UTF8Char(' '), UTF8Char('\t') };

	while (nOUT < *maxIntIN_Out)
	{
		utf8::skip (src, array_b_t, 2);
		if (!utf8::extractU32 (src, &out[nOUT], intEndingChar, 3))
			break;
		++nOUT;

		//cerco il prossimo array separator
		if (src.getCurChar() != arraySeparator)
			utf8::skip (src, array_b_t, 2);

		if (src.getCurChar().isEOF())
			break;
		if (src.getCurChar() != arraySeparator)
			return false;
		src.advanceOneChar();

		if (nOUT >= *maxIntIN_Out)
			break;
	}

	*maxIntIN_Out = nOUT;
	if (nOUT == 0)
		return false;
	srcIN = src;
	return true;
}

//*****************************************
bool string::utf8::extractCPPComment (Iter &srcIN, Iter *result)
{
	Iter src = srcIN;
	if (src.getCurChar() != '/')
		return false;
	src.advanceOneChar();

	//se il secondo char � un'altro /, leggo fino a fine riga
	if (src.getCurChar() == '/')
	{
		string::utf8::extractLine (srcIN, result);
		return true;
	}
	
	//se il secondo char � * leggo fino a che non trovo * /
	if (src.getCurChar() == '*')
	{
		const UTF8Char star('*');
		while (1)
		{
			if (!advanceUntil (src, &star, 1))
				return false;
			src.advanceOneChar();
			if (src.getCurChar() == '/')
			{
				src.advanceOneChar();
				//result->setup (&(srcIN.s[srcIN.iNow]), 0, src.iNow - srcIN.iNow );
                result->setup (srcIN.getPointerToCurrentPosition(), 0, static_cast<u32>(src.getPointerToCurrentPosition() - srcIN.getPointerToCurrentPosition()) );
				srcIN = src;
				return true;
			}
		}
	}

	return false;
}

//*******************************************************************
u32 string::utf8::decodeURIinPlace (u8 *s)
{
	if (NULL == s)
		return 0;
    u8 *pIN = s;
	u8 *pOUT = pIN;
	u32 ct = 0;
	u32 i = 0;
	while (pIN[i] != 0x00)
	{
		if (pIN[i] != '%')
			pOUT[ct++] = pIN[i++];
		else
		{
			u8 c2 = pIN[i + 1];
			if ((c2 >= 'A' && c2 <= 'F') || (c2 >= '0' && c2 <= '9'))
			{
				u8 c3 = pIN[i + 2];
				if ((c3 >= 'A' && c3 <= 'F') || (c3 >= '0' && c3 <= '9'))
				{
					u32 b = 0;
                    ansi::hexToInt (reinterpret_cast<const char*>(&s[i + 1]), &b, 2);
                    pOUT[ct++] = static_cast<u8>(b);
					i += 2;
				}
			}
			else
				pOUT[ct++] = '%';
			++i;
		}
	}
	pOUT[ct] = 0;
	return ct;
}

//*********************************************************
void string::utf8::appendUTF8Char (u8 *dst, u32 sizeOfDest, const UTF8Char &ch)
{
	u32 currentDSTLen = string::utf8::lengthInByte(dst);
	const u32 nBytes = ch.length();
	if (currentDSTLen + nBytes >= sizeOfDest)
	{
		DBGBREAK;
		return;
	}

	for (u8 i=0; i < nBytes; i++)
		dst[currentDSTLen++] = ch.data[i];
	dst[currentDSTLen++] = 0;
}

//*********************************************************
void string::utf8::appendU32 (u8 *dst, u32 sizeOfDest, u32 num, u8 minNumOfDigit)
{ 
	char s[16];
	if (minNumOfDigit==0)	
		sprintf_s(s, sizeof(s), "%d", num); 
	else
		sprintf_s(s, sizeof(s), "%0*d", minNumOfDigit, num);
	
	utf8::concatStr (dst, sizeOfDest, s);
}

//*********************************************************
void string::utf8::appendI32 (u8 *dst, u32 sizeOfDest, i32 num, u8 minNumOfDigit)
{
	char s[16];
	if (minNumOfDigit == 0)
		sprintf_s(s, sizeof(s), "%d", num);
	else
		sprintf_s(s, sizeof(s), "%0*d", minNumOfDigit, num);

	utf8::concatStr (dst, sizeOfDest, s);
}


//*********************************************************
void string::utf8::spf (u8 *dest, u32 sizeOfDest, const char *format, ...)
{
	va_list argptr;
	va_start(argptr, format);
	vsnprintf (reinterpret_cast<char*>(dest), sizeOfDest, format, argptr);
	va_end(argptr);
}

//*******************************************************************
u32 string::utf8::encodeURI (const u8 *urlIN, u8 *out_urlEncoded, u32 sizeof_outURIEncoded)
{
	utf8::Iter iter;
	iter.setup (urlIN);

	u32 ct = 0;
	UTF8Char ch;
	while (!(ch = iter.getCurChar()).isEOF())
	{
		if (ch.isEqual (' '))
			ct+=3;
		else
			ct += ch.length();
		iter.advanceOneChar();
	}
	ct++; //per lo 0x00 finale

	if (NULL == out_urlEncoded)
		return ct;
	if (sizeof_outURIEncoded < ct)
	{
		DBGBREAK;
		out_urlEncoded[0] = 0x00;
		return 0;
	}

	ct = 0;
	iter.toStart();
	while (!(ch = iter.getCurChar()).isEOF())
	{
		if (ch.isEqual (' '))
		{
			out_urlEncoded[ct++] = '%';
			out_urlEncoded[ct++] = '2';
			out_urlEncoded[ct++] = '0';
		}
		else
		{
			memcpy (&out_urlEncoded[ct], ch.data, ch.length());
			ct += ch.length();
		}
		iter.advanceOneChar();
	}

	out_urlEncoded[ct++] = 0;
	assert (ct <= sizeof_outURIEncoded);
	return ct;

}

//*******************************************************************
u32 string::utf8::calcEscapedSeqLength (const u8 *src, u32 srcLenInBytes)
{
	u32 ret = 0;
	if (u32MAX == srcLenInBytes)
	{
		u32 i = 0;
		while (src[i] != 0x00)
		{
			if (src[i] == '\"' || src[i] == '\\')
				ret++;

			++i;
		}
		ret += i;
	}
	else
	{
		ret = srcLenInBytes;
		for (u32 i = 0; i < srcLenInBytes; i++)
		{
			if (src[i] == '\"' || src[i] == '\\')
				ret++;
		}
	}
	return ret;
}

//*******************************************************************
u32 string::utf8::escape (u8 *dst, u32 sizeofDst, const u8 *src, u32 srcLenInBytes)
{
	utf8::Iter i1;
	i1.setup (src, 0, srcLenInBytes);

	u32 lenDST = 0;
	const UTF8Char cSlash('\\');
	const UTF8Char cApiceDoppio('\"');
	UTF8Char ch;
	while (! (ch = i1.getCurChar()) .isEOF())
	{
		if (ch == cApiceDoppio)
		{
			if (lenDST + 2 >= sizeofDst)
				break;
			dst[lenDST++] = '\\';
			dst[lenDST++] = '\"';
		}
		else if (ch == cSlash)
		{
			if (lenDST + 2 >= sizeofDst)
				break;
			dst[lenDST++] = '\\';
			dst[lenDST++] = '\\';
		}
		else
		{
			if (lenDST + ch.length() >= sizeofDst)
				break;
			memcpy (&dst[lenDST], ch.data, ch.length());
			lenDST += ch.length();
		}

		i1.advanceOneChar();
	}
	dst[lenDST] = 0x00;
	return lenDST;
}

//*******************************************************************
u32 string::utf8::unescape(u8 *dst, u32 sizeofDst, const u8 *src, u32 srcLenInBytes)
{
	utf8::Iter i1;
	i1.setup (src, 0, srcLenInBytes);

	u32 lenDST = 0;
	const UTF8Char cSlash('\\');
	const UTF8Char cApiceDoppio('\"');
	UTF8Char ch;
	while (! (ch = i1.getCurChar()) .isEOF())
	{
		if (ch == cSlash)
		{
			i1.advanceOneChar();
			dst[lenDST++] = i1.getCurChar().data[0];
		}
		else
		{
			if (lenDST + ch.length() >= sizeofDst)
				break;
			memcpy (&dst[lenDST], ch.data, ch.length());
			lenDST += ch.length();
		}

		i1.advanceOneChar();
	}
	dst[lenDST] = 0x00;
	return lenDST;
}

//*******************************************************************
u32 string::utf8::unescapeInPlace (u8 *src_dst, u32 srcLenInBytes)
{
	u32 ret = 0;
	if (u32MAX == srcLenInBytes)
	{
		u32 i = 0;
		while (src_dst[i] != 0x00)
		{
			if (src_dst[i] == '\\')
			{
				if (src_dst[i + 1] == '\"' || src_dst[i + 1] == '\\')
				{
					++i;
					src_dst[ret++] = src_dst[i];
				}
			}
			else
				src_dst[ret++] = src_dst[i];
			i++;
		}
	}
	else
	{
		for (u32 i=0; i<srcLenInBytes; i++)
		{
			if (src_dst[i] == '\\')
			{
				if (src_dst[i + 1] == '\"' || src_dst[i + 1] == '\\')
				{
					++i;
					src_dst[ret++] = src_dst[i];
				}
			}
			else
				src_dst[ret++] = src_dst[i];
		}
	}
	src_dst[ret] = 0;
	return ret;
}

//***************************************************************
u32 string::utf8::rtrim(u8 *s)
{
	u32 n = utf8::lengthInByte(s);
	if (n == 0)
		return 0;

	u32 i = n -1;
	while (i>0 && s[i]==0x00)
		--i;
	if (s[i] == 0x00)
	{
		assert (i == 0);
		return 0;
	}

	while (i>0 && s[i]==' ')
		--i;
	
	if (s[i] == ' ')
	{
		assert (i == 0);
		s[i] = 0x00;
		return 0;
	}
	
	i++;
	assert (i <= n);
	s[i] = 0;
	return i;
}

