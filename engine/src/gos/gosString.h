#ifndef _gosString_h_
#define _gosString_h_
#include "string/gosStringEnumAndDefine.h"
#include "gosEnumAndDefine.h"

namespace gos
{
	namespace string
	{
		bool		strANSItoUTF8  (const char* in, u8* out,  u32 sizeOfOut);
		bool		strANSItoUTF8  (const char* in, u32 lenOfIN, u8* out,  u32 sizeOfOut);
		bool		strANSItoUTF16 (const char* in, u16* out, u32 sizeOfOutInBytes);
		bool		strUTF8toUTF16 (const u8 *in, u16* out, u32 sizeOfOutInBytes);
		bool		strUTF16toUTF8 (const u16* in, u8* out, u32 sizeOfOutInBytes);

		namespace ansi
		{
			u32				lengthInBytes (const char* p);
			bool			toUTF8   (const ANSIChar& in, UTF8Char* out);
			bool			toUTF16  (const ANSIChar& in, UTF16Char* out);
			bool			toUTF32  (const ANSIChar& in, UTF32Char* out);
			u8				extractAChar (const char* p, u32 lenInBytes, ANSIChar *out);

			inline	i64		toI64 (const char *s)														{ if (NULL == s) return 0; return (i64)atoll((const char*)s); }
			inline	i32		toI32 (const char *s)														{ if (NULL==s) return 0; return (i32)atoi((const char*)s); }
			inline	u64		toU64 (const char *s)														{ if (NULL == s) return 0; return (u64)atoll((const char*)s); }
			inline	u32		toU32 (const char *s)														{ if (NULL==s) return 0; return (u32)atoi((const char*)s); }
					f32		toF32 (const char *s, u32 lenOfS=u32MAX);

			/* s deve essere un hex valido, il che vuol dire:
					- inizia con un numero o con ABCDEF o abcdef
					- tutti i char successivi sono numeri o ABCDEF o abcdef fino a che non si trova 0x00 o nBytes==0
			*/
			bool			hexToInt (const char *s, u32 *out, u32 lenInByteOf_s = u32MAX);
			
		} // namespace ansi



		namespace utf16
		{
			u32				lengthInBytes (const u16 *utf16_str);
			bool			toUTF8  (const UTF16Char& in, UTF8Char* out);
			bool			toUTF32 (const UTF16Char &in, UTF32Char *out);
			u8				extractAChar (const u16* utf16_str, u32 lenInBytes, UTF16Char *out);

			/* trimma a destra e ritorna la nuova lunghezza in bytes della string trimmata() */
			u32				rtrim(u16 *s);
								
			/* appende la stringa ASCII [src] alla string utf16 [dst] */
			void			concatFromASCII(u16 *dst, u32 sizeofDstInBytes, const char* src);
								
			/* prepende [strToPrepend] a [dst] */
			void			prepend(u16 *dst, u32 sizeOfDstInBytes, const u16* const strToPrepend);
								

		} //namespace utf16



		namespace utf32
		{
			u32				lengthInBytes (const u32 *utf32_str);
			bool			toUTF8  (const UTF32Char& in, UTF8Char* out);
			bool			toUTF16 (const UTF32Char& in, UTF16Char* out);
			u8				extractAChar (const u32* p, u32 lenInBytes, UTF32Char *out);
		} //namespace utf32


		/*==============================================================================
		 * formattazione di vari tipi in stringa
		 *=============================================================================*/
		namespace format
		{
			void	F32 (f32 val, u32 numDecimal, char thousandSep, char decimalSep, char *out, u32 sizeof_out);
			void	U32 (u32 val, char thousandSep, char *out, u32 sizeof_out);
			void	U64 (u64 val, char thousandSep, char *out, u32 sizeof_out);
			void	Hex32 (u32 hex, char *out, u32 sizeofout);	//filla out con la rappresentazione esadecimale di hex (senza lo 0x davanti)
			void	Hex16 (u16 hex, char *out, u32 sizeofout);	//filla out con la rappresentazione esadecimale di hex (senza lo 0x davanti)
			void	Hex8 (u8 hex, char *out, u32 sizeofout);	//filla out con la rappresentazione esadecimale di hex (senza lo 0x davanti)
			void	timeMicroSecToHHMMSSMsUs (u64 microSec, char *out, u32 sizeof_out);

			//ritorna in [out] una stringa con qtyInByte formattata con 2 decimali e con la sua unita' di misura espressa in KB o MB o GB
			//a seconda di quanto e' grande [qtyInByte]
			void 	memoryToKB_MB_GB (u64 qtyInByte, char *out, u32 sizeof_out);


			void    currency (u16 price, u8 numDecimal, char decimalPointCharacter, char *out_s, u16 sizeOfOut);

			//ritorna una stringa nel formato  "4d 8h 32m 5s".  Se numero giorni == 0, allora ritorna "12h 32m 15s".
			//Se [bIncludeSeconds]==false, allora omette i secondi nella stringa di ritorno
			void	timeToNiceDayHourMinuteSec (u64 time_sec, char *out, u32 sizeof_out, bool bIncludeSeconds=true);
		} //namespace format



		namespace utf8
		{
			extern			const UTF8Char CHAR_ARRAY_b_r_n_t[4];	// è un array di comodo che contiene i Char \b \r \n \t
			extern			const UTF8Char SECTION_SIGN;	// UTF8 per il carattere § (noto come 'section sign'), ovvero 0xc2 0xa7

			//equivalente alla strlen
			u32				lengthInByte (const u8 *utf8_str);

			//controlla [utf8IN] estraendo carattere per carattere e mettendolo in [out]. Se incontra dei caratteri invalidi, li sostituisce con 
			//[useThisWhenInvalidChar]. Ritorna la lunghezza in bytes di [out]
			u32				sanitize (const u8 *utf8IN, u32 numBytesInUT8IN, const UTF8Char &useThisWhenInvalidChar, u8 *out, u32 sizeof_out);

			bool			toUTF16  (const UTF8Char &in, UTF16Char *out);
			bool			toUTF32  (const UTF8Char &in, UTF32Char *out);
			
			/*	ritorna il num di bytes utilizzati per il char.
				ritorna 0 in caso di fine string o sequenza invalida*/
			u8				extractAChar (const u8 *utf8_str, u32 lenInBytes, UTF8Char *out);

			u8*				allocStr	(Allocator *allocator, const char* src, u32 numBytesDaUtilizzare=u32MAX);
			u8*				allocStr	(Allocator *allocator, const u8 *src, u32 numBytesDaUtilizzare=u32MAX);
			
			// ritorna la "lengthInBytes" di dst
			u32				makeStr		(u8 *dst, u32 sizeofDst, const char* src);	
										

			//è l'equivalente della sprintf_s, solo che [dest] è di tipo u8 invece che char*
			void			spf (u8 *dest, u32 sizeOfDest, const char *format, ...);
								


			u32				copyStr		(u8 *dst, u32 sizeofDst, const u8 *src, u32 numBytesDaUtilizzare=u32MAX);

			// Copia in [dst] il massimo num di byte possibili di [src] e mette sempre un 0x00 alla fine di [dst]
			// Se [sizeOfDest] è troppo piccolo, non va in buffer overflow ma copia tutto quello che può da [src]
			// e aggiunge uno 0x00 alla fine di [dst]
			// Ritorna length(dst)
            u32             copyStrAsMuchAsYouCan (u8 *dst, u32 sizeOfDest, const u8 *src);

			
			// ritornano la "lengthInBytes" di dst
			u32				concatStr	(u8 *dst, u32 sizeofDst, const char* src);
			inline u32		concatStr	(u8 *dst, u32 sizeofDst, const u8 *src)								{ return utf8::concatStr(dst, sizeofDst, (const char*)src);  }
			u32				concat		(u8 *dst, u32 sizeofDst, const UTF8Char &c);
							
			//ritorna la len di src nel caso in cui fosse applicato l'escaping
			u32				calcEscapedSeqLength (const u8 *src, u32 srcLenInBytes=u32MAX);

			// ritornano la "lengthInBytes" di dst								
			u32				escape (u8 *dst, u32 sizeofDst, const u8 *src, u32 srcLenInBytes=u32MAX);
			u32				unescape (u8 *dst, u32 sizeofDst, const u8 *src, u32 srcLenInBytes=u32MAX);
			u32				unescapeInPlace (u8 *src_dst, u32 srcLenInBytes=u32MAX);
							
							
					void	appendUTF8Char (u8 *dst, u32 sizeOfDest, const UTF8Char &ch);
					void	appendU32 (u8 *dst, u32 sizeOfDest, u32 num, u8 minNumOfDigit=0);
			inline	void	appendU16 (u8 *dst, u32 sizeOfDest, u16 num)									{ appendU32(dst, sizeOfDest, (u32)num); }
			inline	void	appendU8  (u8 *dst, u32 sizeOfDest, u8 num)										{ appendU32(dst, sizeOfDest, (u32)num); }
					void	appendI32 (u8 *dst, u32 sizeOfDest, i32 num, u8 minNumOfDigit = 0);
			inline	void	appendI16 (u8 *dst, u32 sizeOfDest, i16 num)									{ appendI32(dst, sizeOfDest, (i32)num); }
			inline	void	appendI8  (u8 *dst, u32 sizeOfDest, i8 num)										{ appendI32(dst, sizeOfDest, (i32)num); }

			bool			areEqual (const u8 *a, const u8 *b, bool bCaseSensitive);
			bool			areEqual (const u8 *a, const char *b, bool bCaseSensitive);
			bool			areEqualWithLen (const u8 *a, const u8 *b, bool bCaseSensitive, u32 numBytesToCompare);
			bool			areEqualWithLen (const u8 *a, const char *b, bool bCaseSensitive, u32 numBytesToCompare);

			inline	i64		toI64 (const u8 *s)																{ return ansi::toI64((const char*)s); }
			inline	i32		toI32 (const u8 *s)																{ return ansi::toI32((const char*)s); }
			inline	u64		toU64 (const u8 *s)																{ return ansi::toU64((const char*)s); }
			inline	u32		toU32 (const u8 *s)																{ return ansi::toU32((const char*)s); }
			inline	f32		toF32 (const u8 *s, u32 lenOfS=u32MAX)											{ return ansi::toF32((const char*)s, lenOfS); }

			//se [out_urlEncoded] == NULL, allora ritorna il num di bytes necessari a contenere l'intera [urlIN] encodata
			//altrimenti ritorna il num di byte inseriti in [out_urlEncoded]
			u32				encodeURI (const u8 *urlIN, u8 *out_urlEncoded, u32 sizeof_outURIEncoded);

			//sostituisce le sequenze %xx con la relativa rappresentazione in byte
			//Ritorna la nuova lunghezza di s
			u32				decodeURIinPlace(u8 *s);
						
			u32 			rtrim(u8 *s);

			/*=======================================================
			 * Iter
			 *
			 */
			class Iter
			{
			public:
									Iter()														{ setup((const u8 *)NULL); }
									Iter (const Iter &b)										{ priv_copyFrom (b); }

				void				setup (const char *src, u32 firstByte=0, u32 lenghtInBytes=u32MAX);
				void				setup (const u8 *utf8_src, u32 firstByte=0, u32 lenghtInBytes=u32MAX);

				void				toStart();
				void				toLast();
				bool				advanceOneChar();	//ritorna false quando si va oltre la fine
				bool				backOneChar();		//ritorna false quando si va sotto zero
			
				const UTF8Char&		getCurChar() const												{ return curChar; }
				const u8 *			getPointerToCurrentPosition() const;
				u32					getCursorPos () const											{ return cursorPos; }

				//copia in [out] tutta la stringa, a partire da utf8_seq[startingCursorPos] fino al carattere attuale (compreso se bIncludeCurrentChar==true).
				//Ritorna il num di bytes copiati (strlen) e mette uno 0x00 alla fine
				u32					copyStrFromXToCurrentPosition (u32 startingCursorPos, u8 *utf8_out, u32 sizeofOut, bool bIncludeCurrentChar) const;

				u32					copyStrFromCurrentPositionToEnd (u8 *utf8_out, u32 sizeofOut) const;
				u32					copyAllStr(u8 *utf8_out, u32 sizeofOut) const;
					
				u32					totalLenghtInBytes() const										{ return seq_length; }
				u32					getBytesLeft() const											{ if (curChar.isEOF()) return 0; return (seq_length - cursorPos); }

				Iter&				operator= (const Iter &b)										{ priv_copyFrom(b); return *this; }

				bool				cmp (const u8 *b, bool bCaseSensitive) const							
									{ 
										if (curChar.isEOF()) return false;
										return utf8::areEqualWithLen (getPointerToCurrentPosition(), b, bCaseSensitive, getBytesLeft());
									}

			private:
				bool				priv_detectCurrentChar();
				void				priv_copyFrom (const Iter &b)									{ utf8_seq = b.utf8_seq; seq_length = b.seq_length; curChar = b.curChar; cursorPos = b.cursorPos; bytesUsedForCurChar = b.bytesUsedForCurChar;  }

			private:
				const u8			*utf8_seq;
				u32					seq_length;
				UTF8Char			curChar;
				u32					cursorPos;				//posizione attuale all'interno di [utf8_seq]. Il curChar parte da &utf8_seq[cursorPos]
				u8					bytesUsedForCurChar;	//num di bytes di [utf8_seq] che servono al curChar
			};


			
			bool			isCharMaiuscolo (const UTF8Char &c);
			bool			isCharMinuscolo (const UTF8Char &c);
			bool			isANumber (const UTF8Char &c);
			bool			isALetter (const UTF8Char &c);
			inline	bool	isALetterOrANumber (const UTF8Char &c)										{ return (isANumber(c) || isALetter(c)); }
			
			// true se [c] == uno dei caratteri in [validChars]
			bool			isOneOfThis (const UTF8Char &c, const UTF8Char *validChars, u32 numOfValidChars);
							
			//avanza e si ferma quando trova char != da quelli da skipppare o a fine buffer
			//Se trova un char != dai toBeskippedChars, src punta al primo char trovato
			void			skip (Iter &src, const UTF8Char *toBeskippedChars, u32 numOfToBeskippedChars);

			//posto che il carattere attuale sia su un \n o \r, skippa il car attuale
			//e tutti i successivi \n \r
			void			skipEOL (Iter &src);
			
			// usa la skip() per skippare tutti o "\r\n\t\b" e ritorna l'indice del primo char buono 
			inline	void	toNextValidChar (Iter &src)											{ utf8::skip (src, utf8::CHAR_ARRAY_b_r_n_t, 4); }
								
			//controlla il char corrente e compara con validTerminators. Se il char corrente è uno di quelli, ritorna true, 
			//altrimenti passa al carattere successivo e ripete.
			//Se arriviamo a fine stringa, ritorna false
			bool			advanceUntil (Iter &src, const UTF8Char *validTerminators, u32 numOfValidTerminators);

			//controllo il char corrente e se è un EOL termina (o lo skippa), altrimenti avanza e ripete il controllo
			//All'uscita, src punta a EOL oppure al primo char subito dopo EOL (se bskipEOL=true), oppure a fine buffer
			void			advanceToEOL (Iter &src, bool bskipEOL=true);

			//ritorna true se trova esattamente la stringa [whatToFind]. In questo caso, [src] punta al primo carattere dell'istanza di [whatToFind]
			//Ritorna false altrimenti. In questo caso, [src] è avanzato fino a EOF
			bool			find (Iter &src, const u8 *whatToFind);
			bool			find (Iter &src, const char *whatToFind);

			//Prende tutti i caratteri compresti tra src.getCurChar() e l'EOL e li ritorna in out_result
			//All'uscita di questa fn, src punta al primo char subito dopo EOL o a fine buffer
			void			extractLine (Iter &src, Iter *out_result);

			//Ritorna true se trova un valido value, nel qual caso src punta al primo char subito dopo il value trovato e
			//out_result punta al value
			//Ritorna false se non ha trovato un valido value, nel qual caso src rimane immodificato
			//Un "value" valido è una o più parola che:
			//	1- se il primo carattere è un apice singolo (') o doppio (") allora "value" è tutti i caratteri compresi all'interno degli apici, ignorando i *validClosingChars
			//	2- altrimenti "value" deve iniziare con un "non spazio" e comprende tutti i caratteri fino a che non si trova un *validClosingChars
			bool			extractValue (Iter &src, Iter *out_result, const UTF8Char *validClosingChars=utf8::CHAR_ARRAY_b_r_n_t, u32 numOfValidClosingChars=4);

			//Ritorna true se trova un valido identificatore, nel qual caso src punta al primo char subito dopo l'identificatore trovato e
			//out_result punta all'identificatore.
			//Ritorna false se non ha trovato un valido identifier, nel qual caso src rimane immodificato
			//Un identificatore valido è una parola che:
			//	1- inizia con una lettera, oppure inizia con _ ed è seguito da una lettera
			//	2- tutti i caratteri oltre al primo sono lettere, numeri, _ oppure fanno parte di "*otherValidChars"
			//	3- termina non appena si trova un non numero, non lettera, non *otherValidChars
			bool			extractIdentifier (Iter &src, Iter *out_result, const UTF8Char *otherValidChars=NULL, u32 numOfOtherValidChars=0);

			//Ritorna true se trova un valido float, nel qual caso src punta al primo char subito dopo l'intero trovato
			//Ritorna false se non ha trovato un float, nel qual caso src rimane immodificato
			bool			extractFloat (Iter &src, f32 *out, const UTF8Char &sepDecimale=".", const UTF8Char *validClosingChars = utf8::CHAR_ARRAY_b_r_n_t, u32 numOfValidClosingChars=4);

			//Ritorna true se trova almeno 1 float valido.
			//maxFloatIN_Out indica il max num di float da inserire in *out e, in caso di successo, indica anche il num di float
			//insertiti in out.
			//La stringa deve contenere un float, oppure una serie di float separati da arraySeparator.										
			//	Ritorna false se non trova un array di float, nel qual caso src rimane invariato.
			bool			extractFloatArray (Iter &src, f32 *out, u32 *maxFloatIN_Out, const UTF8Char &sepDecimale, const UTF8Char &arraySeparator);

			//Ritorna true se trova un valido intero, nel qual caso src punta al primo char subito dopo l'intero trovato
			//Ritorna false se non ha trovato un intero, nel qual caso src rimane immodificato
			//Un intero è valido se:
			//	- inizia con un '+' o un '-', seguiti da un numero, e tutti gli altri char sono numeri fino a fine buffer o fino a che non si incontra uno dei validClosingChars
			//	- inizia un numero, e tutti gli altri char sono numeri fino a fine buffer o fino a che non si incontra uno dei validClosingChars
			bool			extractI32 (Iter &src, i32 *out, const UTF8Char *validClosingChars = utf8::CHAR_ARRAY_b_r_n_t, u32 numOfValidClosingChars=4);
			bool			extractU32 (Iter &src, u32 *out, const UTF8Char *validClosingChars = utf8::CHAR_ARRAY_b_r_n_t, u32 numOfValidClosingChars=4);

			//Ritorna true se trova almeno 1 int valido.
			//maxIntIN_Out indica il max num di int da inserire in *out e, in caso di successo, indica anche il num di int
			//insertiti in out.
			//La stringa deve contenere un int, oppure una serie di int separati da arraySeparator.										
			//Ritorna false se non trova un array di int, nel qual caso src rimane invariato.
			bool			extractI32Array (Iter &src, i32 *out, u32 *maxIntIN_Out, const UTF8Char &arraySeparator=',');
			bool			extractU32Array (Iter &src, u32 *out, u32 *maxIntIN_Out, const UTF8Char &arraySeparator=',');

			//Ritorna true se trova un valido commento,nel qual caso src punta al primo char subito dopo il commento trovato e
			//out_result punta al commento, comprensivo di / * * / o //
			//Ritorna false se non ha trovato un valido value, nel qual caso src rimane immodificato
			//Un commento è valido se:
			//	1- inizia con "//", allora è lungo fino alla fine della riga (\n\r o fine buffer)
			//	oppure
			//	2- inizia con / *, allora e finisce quando trova * /
			bool			extractCPPComment (Iter &src, Iter *out_result);


			/*=======================================================
			 * String list parser
			 *
			 *  Data una stringa composta da "parole" separate da [separator], ritorna le singole parole trimmate a destra e sinistra.
			 *  Una "parola" e' tutto quanto sta tra l'attuale posizione di scan e il [separatore]
			 */
			class StringListParser
			{
			public:
						StringListParser() : iter() 				{ }

				void	toStart (const char *src, const UTF8Char &separator, u32 firstByte=0, u32 lenghtInBytes=u32MAX)			{ toStart (reinterpret_cast<const u8*>(src), separator, firstByte, lenghtInBytes); }
				void	toStart (const u8 *utf8_src, const UTF8Char &separatorIN, u32 firstByte=0, u32 lenghtInBytes=u32MAX)	{ iter.setup (utf8_src, firstByte, lenghtInBytes); separatore = separatorIN; }
				bool	next (u8 *out, u32 sizeof_out)
				{
					assert(out && sizeof_out);
					if (iter.getCurChar().isEOF())
					{
						out[0] = 0;
						return false;
					}

					Iter result;
					if (!string::utf8::extractValue (iter, &result, &separatore, 1))
					{
						out[0] = 0;
						return false;
					}

					result.copyAllStr(out, sizeof_out);
					string::utf8::rtrim(out);
					iter.advanceOneChar();
					string::utf8::toNextValidChar(iter);
					return true;
				}


			private:
				Iter		iter;
				UTF8Char 	separatore;		
			};

		} //namespace utf8

    } //namespace string
} //namespace gos

#endif //_gosString_h_
