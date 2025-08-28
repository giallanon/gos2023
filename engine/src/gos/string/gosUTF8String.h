#ifndef _gosUTF8String_h_
#define _gosUTF8String_h_
#include <stdio.h>
#include "../gosString.h"
#include "../gosArray.h"

namespace gos
{
	class STRFMT
	{
	public:
		STRFMT (const char *format, ...)
		{
			va_list argptr;
			va_start (argptr, format); 
			vsnprintf (temp, sizeof(temp), format, argptr);
			va_end (argptr);
		}

	public:
		char temp[128];		
	};


	/*==============================================
	 * UTF8String
	 *
	 */
	class UTF8String
	{
	public:
							UTF8String ()																	{ priv_constructor(); }
							UTF8String (const UTF8String &b);
							UTF8String (const char* s);
							~UTF8String();

		void				setAllocator (Allocator *allocIN);
		void				prealloc (u32 newSizeInByte);

							//================================================ assign
		void				clear()																		{ curSize = 0; if (buffer) buffer[0] = 0; }
		void				setFrom (const UTF8String &b, u32 lenInByte=u32MAX)							{ clear(); append(b, lenInByte); }
		void				setFrom (const char *b, u32 lenInByte=u32MAX)								{ clear(); append(b, lenInByte); }
		UTF8String&			operator= (const UTF8String &b)												{ if (this != &b) { clear(); append(b); } return *this; }
		UTF8String&			operator= (const char *b)													{ if (this->buffer != b) { clear(); append(b); } return *this; }

							//================================================ append
		void				append (const UTF8String &b, u32 lenInByte = u32MAX);
		void				append (const char *b, u32 lenInByte = u32MAX);
		void				append (const UTF8Char &b)													{ append ((const char*)b.data, b.length()); }
		void				append (const char c)														{ const char cc[2] = { c, 0 }; append(cc, 1); }
		void				append (unsigned char c)													{ char buf[4];  sprintf_s (buf, sizeof(buf), "%u", c); append (buf, (u32)strlen(buf)); }
		void				append (int c)																{ char buf[32]; sprintf_s (buf, sizeof(buf), "%d", c); append (buf, (u32)strlen(buf)); }
		void				append (unsigned int c)														{ char buf[32]; sprintf_s (buf, sizeof(buf), "%u", c); append (buf, (u32)strlen(buf)); }
#ifdef GOS_PLATFORM__WINDOWS
		void				append (long c)																{ char buf[32]; sprintf_s (buf, sizeof(buf), "%d", c); append (buf, (u32)strlen(buf)); }
		void				append (unsigned long c)													{ char buf[32]; sprintf_s (buf, sizeof(buf), "%u", c); append (buf, (u32)strlen(buf)); }
		void				append(u64 i)																{ char buf[64]; sprintf_s(buf, sizeof(buf), "%I64u", i); append(buf, (u32)strlen(buf)); }
		void				append(i64 i)																{ char buf[64]; sprintf_s(buf, sizeof(buf), "%I64i", i); append(buf, (u32)strlen(buf)); }
#else
		void				append (u64 i)																{ char buf[64]; sprintf_s (buf, sizeof(buf),"%" PRIu64,i); append (buf, (u32)strlen(buf)); }
		void				append (i64 i)																{ char buf[64]; sprintf_s (buf, sizeof(buf),"%" PRIi64,i); append (buf, (u32)strlen(buf)); }
		void 				append (const STRFMT &num)													{ append (num.temp); }
#endif

		friend	UTF8String&		operator<<  (UTF8String &me, const UTF8String &b)								{ me.append (b); return me; }
		friend	UTF8String&		operator<<  (UTF8String &me, const char *b)										{ me.append (b); return me; }
		friend	UTF8String&		operator<<  (UTF8String &me, const UTF8Char &b)									{ me.append (b); return me; }
		friend	UTF8String&		operator<<  (UTF8String &me, char b)											{ me.append (b); return me; }
		friend	UTF8String&		operator<<  (UTF8String &me, unsigned char b)									{ me.append (b); return me; }
		friend	UTF8String&		operator<<  (UTF8String &me, int b)												{ me.append (b); return me; }
		friend	UTF8String&		operator<<  (UTF8String &me, unsigned int b)									{ me.append (b); return me; }
#ifdef GOS_PLATFORM__WINDOWS
		friend	UTF8String&		operator<<  (UTF8String &me, long b)											{ me.append (b); return me; }
		friend	UTF8String&		operator<<  (UTF8String &me, unsigned long b)									{ me.append (b); return me; }
#endif
		friend	UTF8String&		operator<<  (UTF8String &me, u64 b)												{ me.append (b); return me; }
		friend	UTF8String&		operator<<  (UTF8String &me, i64 b)												{ me.append (b); return me; }
		friend	UTF8String&		operator<<  (UTF8String &me, const STRFMT &b)									{ me.append (b.temp); return me; }

							//================================================ concat
		static	UTF8String		concat (const UTF8String &a, const UTF8String &b)								{ UTF8String ret; ret.prealloc (a.lengthInByte() + b.lengthInByte()+1); ret = a; ret.append(b); return ret; }
		friend	UTF8String		operator&  (const UTF8String &a, const UTF8String &b)							{ return UTF8String::concat (a, b); }
		friend	UTF8String		operator&  (const UTF8String &a, const UTF8Char &b)								{ UTF8String ret; ret.prealloc (a.lengthInByte() + 2); ret = a; ret.append(b); return ret; }
		friend	UTF8String		operator&  (const UTF8String &a, const char *b)									{ UTF8String ret; u32 n=(u32)strlen(b); ret.prealloc (a.lengthInByte() + n +1); ret = a; ret.append(b, n); return ret; }
		friend	UTF8String		operator&  (const UTF8String &a, char b)										{ UTF8String ret; ret.prealloc (a.lengthInByte() + 2); ret = a; ret.append(b); return ret; }
		friend	UTF8String		operator&  (const UTF8String &a, unsigned char b)								{ UTF8String ret; ret.prealloc (a.lengthInByte() + 16); ret = a; ret.append(b); return ret; }
		friend	UTF8String		operator&  (const UTF8String &a, int b)											{ UTF8String ret; ret.prealloc (a.lengthInByte() + 16); ret = a; ret.append(b); return ret; }
		friend	UTF8String		operator&  (const UTF8String &a, unsigned int b)								{ UTF8String ret; ret.prealloc (a.lengthInByte() + 16); ret = a; ret.append(b); return ret; }
#ifdef GOS_PLATFORM__WINDOWS
		friend	UTF8String		operator&  (const UTF8String &a, long b)										{ UTF8String ret; ret.prealloc (a.lengthInByte() + 16); ret = a; ret.append(b); return ret; }
		friend	UTF8String		operator&  (const UTF8String &a, unsigned long b)								{ UTF8String ret; ret.prealloc (a.lengthInByte() + 16); ret = a; ret.append(b); return ret; }
#endif
		friend	UTF8String		operator&  (const UTF8String &a, u64 b)                                         { UTF8String ret; ret.prealloc (a.lengthInByte() + 16); ret = a; ret.append(b); return ret; }
		friend	UTF8String		operator&  (const UTF8String &a, i64 b)                                         { UTF8String ret; ret.prealloc (a.lengthInByte() + 16); ret = a; ret.append(b); return ret; }


							//================================================ query
		u32					lengthInByte() const																{ return curSize; }
		const char*			getBuffer() const																	{ return buffer; }
		i32					findFirst (const gos::UTF8Char &ch, u32 startIndex = 0) const;
		bool				isEqualTo (const UTF8String &b, bool bCaseSensitive) const;
		bool				isEqualTo (const char* b, bool bCaseSensitive) const;
		bool				isEqualToWithLen (const UTF8String &b, u32 lenInBytes, bool bCaseSensitive) const;
		bool				isEqualToWithLen (const char *b, u32 lenInBytes, bool bCaseSensitive) const;

							//================================================ utils
		u32					explode (const UTF8Char &cTofind, Array<UTF8String> &out) const;
		void				trim()																				{ trimR(); trimL(); }
		void				trimL();
		void				trimR();
		void				sanitizePath();
		void				escape();
		void				unescape();
		void				escapeTo (UTF8String *out) const;
		void				unescapeTo (UTF8String *out) const;
		void 				insertNSpaces (u32 numSpaceToInsert);
		
							//aggiunge N <filler> fino a che la riga corrente non raggiunge la dimensione <column>
		void 				fillRowUntilColumn (u32 column, char filler=' ');

	private:
		void				priv_constructor();

	private:
		Allocator			*allocator;
		char				*buffer;
		u32					allocatedSize;
		u32					curSize;
		
	};

}//namespace gos
#endif //_gosUTF8String_h_