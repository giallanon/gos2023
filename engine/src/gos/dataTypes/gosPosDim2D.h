#ifndef _gosPosDim2D_h_
#define _gosPosDim2D_h_
#include "../gosEnumAndDefine.h"

namespace gos
{
	/*=============================================================
	* Pos2D<W>
	*	Rappresenta una posizione relativa ad una dimensione W.
	*	Le 4 modalita' possibili sono:
	*
	*	pos: 	number  	-> n			//mode=0, n e' un valore assoluto (es : pos = 5)
	*			number- 	-> W - n		//mode=1, n pixel da W (es: pos = "5-")
	*			!number  	-> W/2 + n		//mode=2, n pixel dopo il centro (es: pos = "!5")
	*			!number- 	-> W/2 - n		//mode=3, n pixel prima del centro	(es: pos = "!5-")
	*============================================================*/
	class Pos2D
	{
	public:
		enum class eMode : u8
		{
			absolute = 0,
			somePixelFromRight = 1,
			somePixelAfterCenter = 2,
			someBeforeAfterCenter = 3,
		};

	public:
					Pos2D()										{ mode=eMode::absolute; value=0; }
					Pos2D(const char *s)						{ setFromString(s); }
					Pos2D(int i)								{ setFromNumber(i); }

		bool		operator== (const Pos2D &b) const			{ return (mode==b.mode && value==b.value); }
		bool		operator!= (const Pos2D &b) const			{ return (mode!=b.mode || value!=b.value); }

		//Pos2D&		operator= (const Pos2D &b)					{ mode=b.mode; value=b.value; return *this; }
		Pos2D&		operator= (int i)							{ setFromNumber(i); return *this; }
		Pos2D&		operator= (const char *s)					{ setFromString(s); return *this; }

		void		setFromString (const char *s);
		void		setFromNumber (int i)						{ mode=eMode::absolute; value=(i16)i; }
			
		i16			resolve (i16 w) const;
		eMode		getMode() const								{ return mode; }
		bool		isAbsolute() const 							{ return (mode == eMode::absolute); }
		bool 		isRelative() const 							{ return (mode != eMode::absolute); }

	private:
		i16			value;
		eMode		mode;
	};

	/*=============================================================
	* Dim2D<from, W>
	*	Rappresenta una dimensione, fa coppia con Pos<W>
	*
	*	dim: 	number  	-> n			//mode=0, n e' un valore assoluto (es : dim = 5)
	*			number- 	-> W - n		//mode=1, fino a W-n (es: dim = "5-")
	*			!number  	-> W/2 + n		//mode=2, fino a (W/2) + n (es: pos = "!5")
	*			!number- 	-> W/2 - n		//mode=3, fino a (W/2) - n (es: pos = "!5-")
	*============================================================*/
	class Dim2D
	{
	public:
		enum class eMode : u8
		{
			absolute = 0,
			upToSomePixelFromRight = 1,
			upToSomePixelAfterCenter = 2,
			upToSomeBeforeAfterCenter = 3,
		};

	public:
					Dim2D()										{ mode=eMode::absolute; value=0; }
					Dim2D(const char *s)						{ setFromString(s); }
					Dim2D(int i)								{ setFromNumber(i); }

		bool		operator== (const Dim2D &b) const			{ return (mode==b.mode && value==b.value); }
		bool		operator!= (const Dim2D &b) const			{ return (mode!=b.mode || value!=b.value); }

		Dim2D&		operator= (const Dim2D &b)					{ mode=b.mode; value=b.value; return *this; }
		Dim2D&		operator= (int i)							{ setFromNumber(i); return *this; }
		Dim2D&		operator= (const char *s)					{ setFromString(s); return *this; }

		void		setFromString (const char *s);
		void		setFromNumber (int i)						{ mode=eMode::absolute; value=(i16)i; }
			
		i16			resolve (i16 from, i16 w) const;
		eMode		getMode() const								{ return mode; }
		bool		isAbsolute() const 							{ return (mode == eMode::absolute); }
		bool 		isRelative() const 							{ return (mode != eMode::absolute); }
		
	private:
		i16			value;
		eMode		mode;
	};
    
} // namespace gos

#endif //_gosPosDim2D_h_
