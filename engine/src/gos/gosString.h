#ifndef _gosString_h_
#define _gosString_h_
#include "string/gosStringEnumAndDefine.h"
#include "string/gosCompileTimeHashedString.h"
#include "../gosMath/gosMath.h"

namespace gos
{
	namespace string
	{
		bool		strANSItoUTF8  (const char *in, char *out, u32 sizeof_out);
		bool		strANSItoUTF8  (const char *in, u32 lenOfIN, char *out,  u32 sizeof_out);
		bool		strANSItoUTF16 (const char *in, u16 *out, u32 sizeOfOutInBytes);
		bool		strUTF8toUTF16 (const char *in, u16 *out, u32 sizeOfOutInBytes);
		bool		strUTF16toUTF8 (const u16 *in, char *out, u32 sizeOfOutInBytes);

		/*==============================================================================
		 * formattazione di vari tipi in stringa
		 *=============================================================================*/
		namespace format
		{
			void	F32 (f32 val, u32 numDecimal, char thousandSep, char decimalSep, char *out, u32 sizeof_out);
			void	U32 (u32 val, char thousandSep, char *out, u32 sizeof_out);
			void	U64 (u64 val, char thousandSep, char *out, u32 sizeof_out);
			void	Hex32 (u32 hex, char *out, u32 sizeof_out);	//filla out con la rappresentazione esadecimale di hex (senza lo 0x davanti)
			void	Hex16 (u16 hex, char *out, u32 sizeof_out);	//filla out con la rappresentazione esadecimale di hex (senza lo 0x davanti)
			void	Hex8 (u8 hex, char *out, u32 sizeof_out);	//filla out con la rappresentazione esadecimale di hex (senza lo 0x davanti)
			void	timeMicroSecToHHMMSSMsUs (u64 microSec, char *out, u32 sizeof_out);

			//ritorna in [out] una stringa con qtyInByte formattata con 2 decimali e con la sua unita' di misura espressa in KB o MB o GB
			//a seconda di quanto e' grande [qtyInByte]
			void 	memoryToKB_MB_GB (u64 qtyInByte, char *out, u32 sizeof_out);


			void    currency (u16 price, u8 numDecimal, char decimalPointCharacter, char *out_s, u16 sizeof_out);

			//ritorna una stringa nel formato  "4d 8h 32m 5s".  Se numero giorni == 0, allora ritorna "12h 32m 15s".
			//Se [bIncludeSeconds]==false, allora omette i secondi nella stringa di ritorno
			void	timeToNiceDayHourMinuteSec (u64 time_sec, char *out, u32 sizeof_out, bool bIncludeSeconds=true);
		} //namespace format
		
		
		namespace ansi
		{
			u32				lengthInBytes (const char *p);
			bool			toUTF8   (const ANSIChar &in, UTF8Char *out);
			bool			toUTF16  (const ANSIChar &in, UTF16Char *out);
			bool			toUTF32  (const ANSIChar &in, UTF32Char *out);
			u8				extractAChar (const char *p, u32 lenInBytes, ANSIChar *out);

			inline	i64		toI64 (const char *s)														{ if (NULL == s) return 0; return (i64)atoll(s); }
			inline	i32		toI32 (const char *s)														{ if (NULL==s) return 0; return (i32)atoi(s); }
			inline	u64		toU64 (const char *s)														{ if (NULL == s) return 0; return (u64)atoll(s); }
			inline	u32		toU32 (const char *s)														{ if (NULL==s) return 0; return (u32)atoi(s); }
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
			bool			toUTF8  (const UTF16Char &in, UTF8Char *out);
			bool			toUTF32 (const UTF16Char &in, UTF32Char *out);
			u8				extractAChar (const u16 *utf16_str, u32 lenInBytes, UTF16Char *out);

			/* trimma a destra e ritorna la nuova lunghezza in bytes della string trimmata() */
			u32				rtrim (u16 *s);
								
			/* appende la stringa ASCII [src] alla string utf16 [dst] */
			void			concatFromASCII (u16 *dst, u32 sizeofDstInBytes, const char *src);
								
			/* prepende [strToPrepend] a [dst] */
			void			prepend (u16 *dst, u32 sizeOfDstInBytes, const u16* const strToPrepend);
								

		} //namespace utf16



		namespace utf32
		{
			u32				lengthInBytes (const u32 *utf32_str);
			bool			toUTF8  (const UTF32Char &in, UTF8Char *out);
			bool			toUTF16 (const UTF32Char &in, UTF16Char *out);
			u8				extractAChar (const u32 *p, u32 lenInBytes, UTF32Char *out);
		} //namespace utf32



		namespace utf8
		{
			extern			const UTF8Char CHAR_ARRAY_b_r_n_t[4];	// è un array di comodo che contiene i Char \b \r \n \t
			extern			const UTF8Char SECTION_SIGN;	// UTF8 per il carattere § (noto come 'section sign'), ovvero 0xc2 0xa7

			//equivalente alla strlen
			u32				lengthInByte (const char *utf8_str);

			//controlla [utf8IN] estraendo carattere per carattere e mettendolo in [out]. Se incontra dei caratteri invalidi, li sostituisce con 
			//[useThisWhenInvalidChar]. Ritorna la lunghezza in bytes di [out]
			u32				sanitize (const char *utf8IN, u32 numBytesInUT8IN, const UTF8Char &useThisWhenInvalidChar, char *out, u32 sizeof_out);

			bool			toUTF16  (const UTF8Char &in, UTF16Char *out);
			bool			toUTF32  (const UTF8Char &in, UTF32Char *out);
			
			/*	ritorna il num di bytes utilizzati per il char.
				ritorna 0 in caso di fine string o sequenza invalida*/
			u8				extractAChar (const char *utf8_str, u32 lenInBytes, UTF8Char *out);

			char*			allocStr	(Allocator *allocator, const char *src, u32 numBytesDaUtilizzare=u32MAX);
			

			//è l'equivalente della sprintf_s, solo che [dest] è di tipo u8 invece che char*
			void			spf (char *dest, u32 sizeof_dst, const char *format, ...);
								

			u32				copyStr (char *dst, u32 sizeof_dst, const char *src, u32 numBytesDaUtilizzare=u32MAX);

			// Copia in [dst] il massimo num di byte possibili di [src] e mette sempre un 0x00 alla fine di [dst]
			// Se [sizeof_dst] è troppo piccolo, non va in buffer overflow ma copia tutto quello che può da [src]
			// e aggiunge uno 0x00 alla fine di [dst]
			// Ritorna length(dst)
            u32             copyStrAsMuchAsYouCan (char *dst, u32 sizeof_dst, const char *src);

			
			// ritornano la "lengthInBytes" di dst
			u32				concatStr	(char *dst, u32 sizeof_dst, const char* src);
			u32				concat		(char *dst, u32 sizeof_dst, const UTF8Char &c);
							
			//ritorna la len di src nel caso in cui fosse applicato l'escaping
			u32				calcEscapedSeqLength (const char *src, u32 srcLenInBytes=u32MAX);

			// ritornano la "lengthInBytes" di dst								
			u32				escape (char *dst, u32 sizeof_dst, const char *src, u32 srcLenInBytes=u32MAX);
			u32				unescape (char *dst, u32 sizeof_dst, const char *src, u32 srcLenInBytes=u32MAX);
			u32				unescapeInPlace (char *src_dst, u32 srcLenInBytes=u32MAX);
							
							
					void	appendUTF8Char (char *dst, u32 sizeof_dst, const UTF8Char &ch);
					void	appendU32 (char *dst, u32 sizeof_dst, u32 num, u8 minNumOfDigit=0);
			inline	void	appendU16 (char *dst, u32 sizeof_dst, u16 num)										{ appendU32(dst, sizeof_dst, (u32)num); }
			inline	void	appendU8  (char *dst, u32 sizeof_dst, u8 num)										{ appendU32(dst, sizeof_dst, (u32)num); }
					void	appendI32 (char *dst, u32 sizeof_dst, i32 num, u8 minNumOfDigit = 0);
			inline	void	appendI16 (char *dst, u32 sizeof_dst, i16 num)										{ appendI32(dst, sizeof_dst, (i32)num); }
			inline	void	appendI8  (char *dst, u32 sizeof_dst, i8 num)										{ appendI32(dst, sizeof_dst, (i32)num); }

			bool			areEqual (const char *a, const char *b, bool bCaseSensitive);
			bool			areEqualWithLen (const char *a, const char *b, bool bCaseSensitive, u32 numBytesToCompare);

			inline	i64		toI64 (const char *s)																{ return ansi::toI64(s); }
			inline	i32		toI32 (const char *s)																{ return ansi::toI32(s); }
			inline	u64		toU64 (const char *s)																{ return ansi::toU64(s); }
			inline	u32		toU32 (const char *s)																{ return ansi::toU32(s); }
			inline	f32		toF32 (const char *s, u32 lenOfS=u32MAX)											{ return ansi::toF32(s, lenOfS); }

			//se [out_urlEncoded] == NULL, allora ritorna il num di bytes necessari a contenere l'intera [urlIN] encodata
			//altrimenti ritorna il num di byte inseriti in [out_urlEncoded]
			u32				encodeURI (const char *urlIN, char *out_urlEncoded, u32 sizeof_outURIEncoded);

			//sostituisce le sequenze %xx con la relativa rappresentazione in byte
			//Ritorna la nuova lunghezza di s
			u32				decodeURIinPlace (char *s);
						
			u32 			rtrim (char *s);

			/*=======================================================
			 * Iter
			 *
			 */
			class Iter
			{
			public:
									Iter()														{ setup(NULL); }
									Iter (const Iter &b)										{ priv_copyFrom (b); }

				void				setup (const char *utf8_src, u32 firstByte=0, u32 lenghtInBytes=u32MAX);
				void				setup (const Iter &src, u32 src_cursorStart, u32 src_cursorEnd);

				void				toStart();
				void				toLast();
				bool				advanceOneChar();	//ritorna false quando si va oltre la fine
				bool				backOneChar();		//ritorna false quando si va sotto zero
				bool				advanceNumByte (u32 howMany);//ritorna false quando si va oltre la fine
				u32 				toNextValidChar();
			
				const UTF8Char&		getCurChar() const												{ return curChar; }
				const char*			getPointerToCurrentPosition() const;
				u32					getCursorPos () const											{ return cursorPos; }
				
				
				//basandosi sulla posizione attuale di [cursor], conta in \n per determinare su quale
				//riga del file di testo siamo
				void 				deductCurrentLineAndCharPosition (u32 *out_lineNum, u32 *out_charPos) const;


				//copia in [out] tutta la stringa, a partire da utf8_seq[startingCursorPos] fino al carattere attuale (compreso se bIncludeCurrentChar==true).
				//Ritorna il num di bytes copiati (strlen) e mette uno 0x00 alla fine
				u32					copyStrFromXToCurrentPosition (u32 startingCursorPos, char *utf8_out, u32 sizeofOut, bool bIncludeCurrentChar) const;

				u32					copyStrFromCurrentPositionToEnd (char *utf8_out, u32 sizeofOut) const;
				u32					copyAllStr (char *utf8_out, u32 sizeofOut) const;
					
				u32					totalLenghtInBytes() const										{ return seq_length; }
				u32					getBytesLeft() const											{ if (curChar.isEOF()) return 0; return (seq_length - cursorPos); }

				Iter&				operator= (const Iter &b)										{ priv_copyFrom(b); return *this; }

				bool				cmp (const char *b, bool bCaseSensitive) const							
									{ 
										if (curChar.isEOF()) return false;
										return utf8::areEqualWithLen (getPointerToCurrentPosition(), b, bCaseSensitive, getBytesLeft());
									}

			private:
				bool				priv_detectCurrentChar();
				void				priv_copyFrom (const Iter &b)									{ utf8_seq = b.utf8_seq; seq_length = b.seq_length; curChar = b.curChar; cursorPos = b.cursorPos; bytesUsedForCurChar = b.bytesUsedForCurChar;  }

			private:
				const char			*utf8_seq;
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
			//Ritorna il numero di linee skippate (nel caso in cui \n e/o \r siano tra i <toBeskippedChars>
			u32				skip (Iter &src, const UTF8Char *toBeskippedChars, u32 numOfToBeskippedChars);

			//posto che il carattere attuale sia su un \n o \r, skippa il car attuale
			//e tutti i successivi \n \r
			//Ritorna il numero di linee skippate
			u32				skipEOL (Iter &src);
			
			// usa la skip() per skippare tutti o "\r\n\t\b" e ritorna l'indice del primo char buono 
			//Ritorna il numero di linee skippate
			inline	u32		toNextValidChar (Iter &src)													{ return utf8::skip (src, utf8::CHAR_ARRAY_b_r_n_t, 4); }
								
			//controlla il char corrente e compara con validTerminators. Se il char corrente è uno di quelli, ritorna true, 
			//altrimenti passa al carattere successivo e ripete.
			//Se arriviamo a fine stringa, ritorna false
			bool			advanceUntil (Iter &src, const UTF8Char *validTerminators, u32 numOfValidTerminators, u32 *out_canbeNULL_numLineSkipped = NULL);

			//controllo il char corrente e se e' un EOL termina (o lo skippa), altrimenti avanza e ripete il controllo
			//All'uscita, src punta a EOL oppure al primo char subito dopo EOL (se bskipEOL=true), oppure a fine buffer
			//Ritorna il numero di linee skippate
			u32				advanceToEOL (Iter &src, bool bskipEOL=true);

			//ritorna true se trova esattamente la stringa [whatToFind]. In questo caso, [src] punta al primo carattere dell'istanza di [whatToFind]
			//Ritorna false altrimenti. In questo caso, [src] è avanzato fino a EOF
			bool			find (Iter &src, const char *whatToFind);

			//Prende tutti i caratteri compresi tra src.getCurChar() e l'EOL e li ritorna in out_result
			//All'uscita di questa fn, src punta al primo char subito dopo EOL o a fine buffer
			//Ritorna il numero di linee skippate
			u32				extractLine (Iter &src, Iter *out_result);

			//controlla il char corrente e compara con [terminator]]. Se il char corrente e' [terminator], ritorna true, 
			//altrimenti passa al carattere successivo e ripete.
			//Se arriviamo a fine stringa, ritorna false
			//Se ritorna true, allora in [out_result] c'e' tutta la stringa compresa tra src.getCurChar() e terminator
			//[terminator] e' compreso solo se [bAlsoIncludeTerminator] == true
			//[src] e' posizionato sul carattere [terminator]
			bool			extractUntil (Iter &src, const UTF8Char &terminator, Iter *out_result, bool bAlsoIncludeTerminator = false);
			bool			extractUntil (Iter &src, char terminator, Iter *out_result, bool bAlsoIncludeTerminator = false);

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
			bool			extractCPPComment (Iter &src, Iter *out_result, u32 *out_canbeNULL_numLineSkipped);


			/***************************************************************************
			 * @brief 	String list parser
			 *  		Data una stringa composta da "parole" separate da [separator], ritorna le singole parole trimmate a destra e sinistra.
			 *  		Una "parola" e' tutto quanto sta tra l'attuale posizione di scan e il [separatore]
			 */
			class StringListParser
			{
			public:
						StringListParser() : iter() 				{ }

				void	toStart (const char *utf8_src, const UTF8Char &separatorIN, u32 firstByte=0, u32 lenghtInBytes=u32MAX)	{ iter.setup (utf8_src, firstByte, lenghtInBytes); separatore = separatorIN; }
				bool	next (char *out, u32 sizeof_out)
				{
					assert(out && sizeof_out);
					string::utf8::toNextValidChar(iter);
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


				bool 	extract_f32 (f32 *out)			{ char s[32]; if (!next(s, sizeof(s)))	return false; *out = string::utf8::toF32(s); return true; }
				bool 	extract_vec3f (gos::vec3f *out)
				{
					if (!extract_f32(&out->x)) return false;
					if (!extract_f32(&out->y)) return false;
					if (!extract_f32(&out->z)) return false;
					return true;
				}

				bool 	extract_f32Array (f32 *out, u32 num_float_to_extract)
				{
					for (u32 i=0; i<num_float_to_extract; i++)
					{
						if (!extract_f32(&out[i]))
							return false;
					}
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
