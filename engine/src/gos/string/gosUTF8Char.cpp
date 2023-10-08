#include "../gos.h"
#include "../gosString.h"

using namespace gos;

//*************************************************
bool UTF8Char::isValidUTF8Sequence() const
{
	const u8 len = length();
	if (len == 0)
		return true;

	if ((data[0] & 0x80) == 0)
	{
		if (len == 1)
			return true;
		return false;
	}

	//a questo punto il char UTF8 consiste di almeno 2 byte
	if (len < 2)
		return false;
	
	//data[1] deve essere di tipo 0x10xxxxxx
	if ((data[1] & 0xC0) != 0x80)
		return false;
	
	if ((data[0] & 0xE0) == 0xC0)
	{
		//siamo nel caso data[0]=110xxxxx data[1]=10xxxxxx
		if (len == 2)
			return true;
		return false;
	}


	else if ((data[0]  & 0xF0) == 0xE0)
	{
		//siamo nel caso data[0]=1110xxxx data[1]=10xxxxxx c2=10xxxxxx
		if (len < 3)
			return false;
		
		//c2 deve essere di tipo 0x10xxxxxx
		if ((data[2] & 0xC0) != 0x80)
			return false;	

		if (len==3)
			return true;
		return false;
	}


	else if ((data[0] & 0xF8) == 0xF0)
	{
		//siamo nel caso data[0]=11110xxx data[1]=10xxxxxx c2=10xxxxxx c3=10xxxxxx
		if (len < 4)
			return false;
		
		//c2 deve essere di tipo 0x10xxxxxx
		if ((data[2] & 0xC0) != 0x80)
			return false;
		
		//c3 deve essere di tipo 0x10xxxxxx
		if ((data[3] & 0xC0) != 0x80)
			return false;
		return true;
	}

	return false;
}


//*************************************************
u8 UTF8Char::length() const
{
	if (data[0] == 0x00) return 0;
	if (data[1] == 0x00) return 1;
	if (data[2] == 0x00) return 2;
	if (data[3] == 0x00) return 3;
	return 4;
}

//*************************************************
bool UTF8Char::setFrom4Byte (u8 a, u8 b, u8 c, u8 d)
{
	data[0] = a;
	data[1] = b;
	data[2] = c;
	data[3] = d;
	bool ret = isValidUTF8Sequence();
	assert (ret);
	return ret;
}

//*************************************************
void UTF8Char_Latin1ToUTF8 (u8 latin1, u8 *out_utf80, u8 *out_utf81)
{
	*out_utf80 = 0xc2 + (latin1 > 0xbf);
	*out_utf81 =(latin1 & 0x3f) + 0x80;
}

//*************************************************
bool UTF8Char::setFromConstChar (const char* utf8Char, u8 *out_numByteConsumed)
{
	if (NULL == utf8Char)
	{
		setEOF();
		if (out_numByteConsumed)
			*out_numByteConsumed = 0;
		return true;
	}

	data[0] = (u8)utf8Char[0];
	if (0x00 == data[0])
	{
		setEOF();
		if (out_numByteConsumed)
			*out_numByteConsumed = 0;
		return true;
	}

	if ((data[0] & 0x80) == 0)
	{
		data[1] = data[2] = data[3] = 0;
		if (out_numByteConsumed)
			*out_numByteConsumed = 1;
		return true;
	}

	//a questo punto il char UTF8 deve consistere di almeno 2 byte
	data[1] = (u8)utf8Char[1];
	if (0x00 == data[1])
	{
		//ok, quindi abbiamo un solo byte che per� � maggior di 128.
		//Assumo che sia un Latin1 e provo la conversione
		UTF8Char_Latin1ToUTF8 (data[0], &data[0], &data[1]);
		data[2] = data[3] = 0;
		if (isValidUTF8Sequence())
		{
			if (out_numByteConsumed)
				*out_numByteConsumed = 1;
			return true;
		}

		DBGBREAK;
		setEOF();
		return false;
	}
	
	//c1 deve essere di tipo 0x10xxxxxx
	if ((data[1] & 0xC0) != 0x80)
	{
		//in teoria questo � un errore per�, se data[0] era > 128, assumo che data[0] sia di per se un Latin1 e converto solo lui
		if ((data[0] & 0x80) != 0)
		{
			UTF8Char_Latin1ToUTF8 (data[0], &data[0], &data[1]);
			data[2] = data[3] = 0;
			if (isValidUTF8Sequence())
			{
				if (out_numByteConsumed)
					*out_numByteConsumed = 1;
				return true;			
			}
		}

		//se invece data[0] era un valido carattere iniziale, � sicuramente un errore
		DBGBREAK;
		setEOF();
		return false;
	}
	
	if ((data[0] & 0xE0) == 0xC0)
	{
		//siamo nel caso data[0]=110xxxxx data[1]=10xxxxxx
		data[2] = data[3] = 0;
		if (out_numByteConsumed)
			*out_numByteConsumed = 2;
		return true;
	}


	else if ((data[0]  & 0xF0) == 0xE0)
	{
		//siamo nel caso data[0]=1110xxxx data[1]=10xxxxxx c2=10xxxxxx
		data[2] = (u8)utf8Char[2];
		if (0x00 == data[2])
		{
			DBGBREAK;
			setEOF();
			return false;
		}
		
		//c2 deve essere di tipo 0x10xxxxxx
		if ((data[2] & 0xC0) != 0x80)
		{
			DBGBREAK;
			setEOF();
			return false;
		}

		if (out_numByteConsumed)
			*out_numByteConsumed = 3;
		data[3] = 0;
		return true;
	}


	else if ((data[0] & 0xF8) == 0xF0)
	{
		//siamo nel caso data[0]=11110xxx data[1]=10xxxxxx c2=10xxxxxx c3=10xxxxxx
		data[2] = (u8)utf8Char[2];
		if (0x00 == data[2])
		{
			DBGBREAK;
			setEOF();
			return false;
		}
		
		data[3] = (u8)utf8Char[3];
		if (0x00 == data[3])
		{
			DBGBREAK;
			setEOF();
			return false;
		}

		//c2 deve essere di tipo 0x10xxxxxx
		if ((data[2] & 0xC0) != 0x80)
		{
			DBGBREAK;
			setEOF();
			return false;
		}
		
		//c3 deve essere di tipo 0x10xxxxxx
		if ((data[3] & 0xC0) != 0x80)
		{
			DBGBREAK;
			setEOF();
			return false;
		}

		if (out_numByteConsumed)
			*out_numByteConsumed = 4;
		return true;
	}

	DBGBREAK;
	setEOF();
	return false;
}

