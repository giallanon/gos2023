#include "gosPosDim2D.h"


using namespace gos;

//***********************************************
i16	gos_priv_numFromStr (const char *s, u8 *out_haveMinusAtEnd)
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

	if (s[n] == '-')
		*out_haveMinusAtEnd = 1;
	else
		*out_haveMinusAtEnd = 0;
	return (i16)atoi(tmp);
}

//***********************************************
void Pos2D::setFromString (const char *s)
{
	assert(s && s[0]!=0);
	
	if (s[0] == '!')
	{
		u8 haveMinus;
		value = gos_priv_numFromStr (&s[1], &haveMinus);
		if (haveMinus)
			mode = eMode::someBeforeAfterCenter;
		else
			mode = eMode::somePixelAfterCenter;
	}
	else
	{
		u8 haveMinus;
		value = gos_priv_numFromStr (s, &haveMinus);
		if (haveMinus)
			mode = eMode::somePixelFromRight;
		else
		{
			assert (s[0] >='0' && s[0]<='9');
			mode = eMode::absolute;
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
	case eMode::someBeforeAfterCenter:	return (w/2) - value;
	}
}





//***********************************************
void Dim2D::setFromString (const char *s)
{
	assert(s && s[0]!=0);
		
	if (s[0] == '!')
	{
		u8 haveMinus;
		value = gos_priv_numFromStr (&s[1], &haveMinus);
		if (haveMinus)
			mode = eMode::upToSomeBeforeAfterCenter;
		else
			mode = eMode::upToSomePixelAfterCenter;
	}
	else
	{
		u8 haveMinus;
		value = gos_priv_numFromStr (s, &haveMinus);
		if (haveMinus)
			mode = eMode::upToSomePixelFromRight;
		else
		{
			assert (s[0] >='0' && s[0]<='9');
			mode = eMode::absolute;
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
	case eMode::upToSomeBeforeAfterCenter:		x2 = (w/2) - value;		if (x2 > x1) return (x2-x1); return 0;
	}
}
