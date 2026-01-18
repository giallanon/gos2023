#include "gosPosDim2D.h"


using namespace gos;

//***********************************************
static i16	PosDim2D_priv_numFromStr (const char *s, char *out_lastChar)
{
	assert(s && s[0]!=0);

	char tmp[16];
	u32 n = 0;

	if (s[0] == '-' || s[0]=='+')
		tmp[n++] = s[0];
	while (s[n]>='0' && s[n]<='9')
	{
		tmp[n] = s[n];
		++n;
	}
	tmp[n] = 0;

	*out_lastChar = s[n];
	assert (s[n] == 0x00 || s[n] == '-' || s[n] == '%');
	return (i16)atoi(tmp);
}

//***********************************************
void Pos2D::setFromString (const char *s)
{
	assert(s && s[0]!=0);
	
	if (s[0] == '!')
	{
		char lastChar;
		value = PosDim2D_priv_numFromStr (&s[1], &lastChar);
		switch (lastChar)
		{
		default:
			DBGBREAK; //simbolo non consentito
			mode = eMode::somePixelAfterCenter;
			break;
		case '-':	mode = eMode::somePixelBeforeCenter; break;
		case 0x00:	mode = eMode::somePixelAfterCenter; break;
		}
	}
	else
	{
		char lastChar;
		value = PosDim2D_priv_numFromStr (s, &lastChar);
		switch (lastChar)
		{
		default:
			DBGBREAK; //simbolo non consentito
			mode = eMode::absolute;
			break;

		case '-':
			mode = eMode::somePixelFromRight;
			break;

		case 0x00:
			assert (s[0] >= '0' && s[0] <= '9');
			mode = eMode::absolute;
			break;

		case '%':
			assert (value > 0);
			mode = eMode::perc;
			break;
		}
	}
}

//***********************************************
i16	Pos2D::resolve (i16 w) const
{
	switch (mode)
	{
	default:							DBGBREAK; return 0;
	case eMode::absolute:				return value;
	case eMode::somePixelFromRight:		return w - value;
	case eMode::somePixelAfterCenter:	return (w/2) + value;
	case eMode::somePixelBeforeCenter:	return (w/2) - value;
	case eMode::perc:					return (w*value) / 100;
	}
}





//***********************************************
void Dim2D::setFromString (const char *s)
{
	assert(s && s[0]!=0);
		
	if (s[0] == '!')
	{
		char lastChar;
		value = PosDim2D_priv_numFromStr (&s[1], &lastChar);
		switch (lastChar)
		{
		default:
			DBGBREAK; //simbolo non consentito
			mode = eMode::upToSomePixelAfterCenter;
			break;
		case '-':	mode = eMode::somePixelBeforeCenter; break;
		case 0x00:	mode = eMode::upToSomePixelAfterCenter; break;
		}
	}
	else
	{
		char lastChar;
		value = PosDim2D_priv_numFromStr (s, &lastChar);
		switch (lastChar)
		{
		default:
			DBGBREAK; //simbolo non consentito
			mode = eMode::absolute;
			break;

		case '-':
			mode = eMode::upToSomePixelFromRight;
			break;

		case 0x00:
			assert (s[0] >= '0' && s[0] <= '9');
			mode = eMode::absolute;
			break;

		case '%':
			assert (value > 0);
			mode = eMode::perc;
			break;
		}
	}
}

//***********************************************
i16	Dim2D::resolve (i16 x1, i16 w) const
{
	i16 x2;
	switch (mode)
	{
	default:									DBGBREAK; return 0;
	case eMode::absolute:						return value;
	case eMode::upToSomePixelFromRight:			x2 = w - value;			if (x2 > x1) return (x2-x1); return 0;
	case eMode::upToSomePixelAfterCenter:		x2 = (w/2) + value;		if (x2 > x1) return (x2-x1); return 0;
	case eMode::somePixelBeforeCenter:			x2 = (w/2) - value;		if (x2 > x1) return (x2-x1); return 0;
	case eMode::perc:							return (w*value) / 100;
	}
}
