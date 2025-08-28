#ifndef _gosIniFile_h_
#define _gosIniFile_h_
#include "gosArray.h"
#include "gosFastArray.h"
#include "gosString.h"
#include "string/gosUTF8String.h"

#ifdef GOS_COMPILER__MSVC
	#pragma warning(disable:4458)
#endif

namespace gos
{
	class IniFileSection;
	class IniFileArrayHelper;

	/*===============================================
	 * IniFile
	 *
	 */
	class IniFile
	{
	public:
								IniFile ();
								~IniFile();

								//============================ 
		void					setup (Allocator *alloc)												{ assert(NULL==allocator); allocator=alloc; }
		void					unsetup();
		void					reset();

								//============================ load / save
		void					setSaveFilename (const char* filename);
		void					save () const;
		void					saveAs (const char* filename) const;


								//============================ merge
		void					mergeWith (const IniFile &otherIniFile, bool onConflictUseValueFromOtherIniFile);
									//Scorre tutte le sezioni di [otherIniFile] e le aggiunge a this se gia' non esistevano.
									//Per ogni voce di ogni sezione:
									//		- la aggiunge a this se gia' non esisteva.
									//		- se la voce esisteva gia', la sovrascrive con il valore di [otherIniFile] se [onConflictUseValueFromOtherIniFile]==true, altrimenti
									//		  mantiene il valore precedente
									

								//============================ new
		void					createEmpty (const char* filename);	//filename viene usato successivamente durante la save()

								//============================ parse
		bool					loadAndParse (const char *filename);
		bool					parseFromMemory (const void *buffer, u32 sizeOfBuffer);

								//============================ set / get
								// identifier usa la notazione "." per indicare le sottosezioni
		void					set (const char *identifier, const char* value, bool bCreateIfNotFound = true);
		
		bool 					exists  (const char *identifier) const;
		bool					get (const char *identifier, UTF8String &out) const;
		bool					get (const char *identifier, char *out, u32 sizeof_out) const;
		void					getOrDefault (const char *identifier, const char *defaultValue, UTF8String &out) const;
		void					getOrDefault (const char *identifier, const char *defaultValue, char *out, u32 sizeof_out) const;
		bool					checkString (const char *identifier, const char *valueToCmp, bool bSaseSens=false) const;
									//ritorna true se identifier esiste ed e' == a valueToCmp
		f32						getOrDefaultAsF32 (const char *identifier, f32 defaultValue) const;

		u64						getOrDefaultAsU64 (const char *identifier, u64 defaultValue) const;
		u32						getOrDefaultAsU32 (const char *identifier, u32 defaultValue) const;
		u16						getOrDefaultAsU16 (const char *identifier, u16 defaultValue) const;
		u8						getOrDefaultAsU8 (const char *identifier, u8 defaultValue) const;

		i64						getOrDefaultAsI64 (const char *identifier, i64 defaultValue) const;
		i32						getOrDefaultAsI32 (const char *identifier, i32 defaultValue) const;
		i16						getOrDefaultAsI16 (const char *identifier, i16 defaultValue) const;
		i8						getOrDefaultAsI8 (const char *identifier, i8 defaultValue) const;

		i32						getOrDefaultHexToI32 (const char *identifier, const char *defaultValue) const;
									//legge una stringa in hex e ritorna i32 (l'hex NON deve iniziare con 0x)
		
		bool					getOrDefaultAsBool (const char *identifier, bool defaultValue) const;
									//ritorna false se [identifier]==0, true altrimenti

								//============================ section
		IniFileSection*			getRoot() const																	{ return root; }
		IniFileSection*			getOrCreateSubsection (const char *name);
		IniFileSection*			getSubsection (const char *name) const;
									// name usa la notazione "." per indicare le sottosezioni
		u32						getNSubsection () const;
		IniFileSection*			getSubsectionByIndex (u32 i) const;

								//============================ json
		char*					toJSon (gos::Allocator *allocator, u32 *out_allocatedSize) const;
		bool					fromJSon (const u8 *jsonSRC, u32 sizeOfJSonSRC)									{ return fromJSon (reinterpret_cast<const char*>(jsonSRC), sizeOfJSonSRC); }
		bool					fromJSon (const char *jsonSRC, u32 sizeOfJSonSRC);

								//============================ utils
		void 					debug_print (gos::UTF8String &out) const;

	public:
		static bool 			_resolveInplace_identifierThatMayHaveArrayIndexing (char *in_out_name, u32 lenof_name);

	private:
		void					priv_errorMessageNear (u32 linuNumber, const UTF8String &msg, const string::utf8::Iter &src) const;
		bool					priv_Parse_separator_Value (u32 linuNumber, string::utf8::Iter &src, string::utf8::Iter *result, char separator) const;
		bool					priv_Parse_Section (IniFileSection *section, string::utf8::Iter &src, u32 &in_out_curLineNumber);
		u32						priv_toNextValidChar (IniFileSection *section, string::utf8::Iter &src) const;
		

	private:
		Allocator				*allocator;
		UTF8String				filename;
		IniFileSection			*root;
	};







	/*===============================================
	 * IniFileSection
	 *
	 */
	class IniFileSection
	{
	public:
								IniFileSection (Allocator *alloc);
								~IniFileSection();

								//============================= set / get
								// identifier usa la notazione "." per indicare le sottosezioni
		void					set (const char *identifier, const char* value, bool bCreateIfNotFound = true);
		void					set (const char *identifierIN, u32 value, bool bCreateIfNotFound)					{ char s[32]; sprintf_s (s, sizeof(s), "%d", value); set (identifierIN, s, bCreateIfNotFound); }
		void					set (const char *identifierIN, i32 value, bool bCreateIfNotFound)					{ char s[32]; sprintf_s (s, sizeof(s), "%d", value); set (identifierIN, s, bCreateIfNotFound); }
		void					set (const char *identifierIN, f32 value, bool bCreateIfNotFound)					{ char s[32]; sprintf_s (s, sizeof(s), "%f", value); set (identifierIN, s, bCreateIfNotFound); }
		void					set (const char *identifierIN, bool value, bool bCreateIfNotFound)					{ if (true == value) set (identifierIN, "1", bCreateIfNotFound); else set (identifierIN, "0", bCreateIfNotFound); }

		u32 					getLineStarted() const 																{ return startAtLine; }

		bool 					exists  (const char *identifier) const;
		bool					get (const char *identifier, char *out, u32 sizeof_out) const;
		bool					get (const char *identifierIN, UTF8String &out) const								{ const char *pstr = priv_get (identifierIN); if (NULL == pstr) return false; out = pstr; return true; }

		bool					checkString (const char *identifier, const char *valueToCmp, bool bCaseSensitive=false) const;
									//ritorna true se identifier esiste ed � = a valueToCmp

		void					getOrDefault (const char *identifier, const char *defaultValue, UTF8String &out) const;
		void					getOrDefault (const char *identifier, const char *defaultValue, char *out, u32 sizeof_out) const;
		f32						getOrDefaultAsF32 (const char *identifier, f32 defaultValue) const;
		u64						getOrDefaultAsU64 (const char *identifier, u64 defaultValue) const;
		u32						getOrDefaultAsU32 (const char *identifier, u32 defaultValue) const;
		u16						getOrDefaultAsU16 (const char *identifier, u16 defaultValue) const;
		u8						getOrDefaultAsU8 (const char *identifier, u8 defaultValue) const;
		i64						getOrDefaultAsI64 (const char *identifier, i64 defaultValue) const;
		i32						getOrDefaultAsI32 (const char *identifier, i32 defaultValue) const;
		i16						getOrDefaultAsI16 (const char *identifier, i16 defaultValue) const;
		i8						getOrDefaultAsI8 (const char *identifier, i8 defaultValue) const;

		i32						getOrDefaultHexToI32 (const char *identifier, const char *defaultValue) const;
		
		bool					getOrDefaultAsBool (const char *identifier, bool defaultValue) const;
									//ritorna false se [identifier]==0, true altrimenti


								//============================= add
		IniFileSection*			addSubsection (const char *name);
		void					addComment (const char *s, u32 len);
		void					addBlob (const char *s, u32 len);
		
								//============================= query
		u32						getNSubsection () const									{ return subSection.getNElem(); }
		IniFileSection*			getSubsectionByIndex (u32 i) const						{ assert (i<getNSubsection()); return subSection.getElem(i); }
		IniFileSection*			getOrCreateSubsection (const char *name);
		IniFileSection*			getSubsection (const char* name) const;
									// name usa la notazione "." per indicare le sottosezioni
		
		u32						getNIdentifier() const									{ return identifier.getNElem(); }
		u32						identifierExists (const char *name) const;
		const char*				getValueByIndex (u32 index) const;
		const char*				getIdentifierByIndex (u32 index) const;

								//============================= Save
		void					save (gos::File &f, u32 tabCount, u32 level) const;
		void 					debug_print (gos::UTF8String &out, u32 indent=0) const;
		
								//============================= json
		void					toJSon (BufferLinear &buffer, u32 &ct) const;
		bool					fromJSon (gos::string::utf8::Iter &iter);

								//============================ var
		UTF8String				name;

	private:
		enum class eElem : u8
		{
			comment			= 0,
			subsection		= 1,
			identifierValue	= 2,
			blob			= 3
		};

		class sElem
		{
		public:
					sElem()	{}
			sElem	operator= (const sElem &b)		{ what=b.what; index=b.index; return *this; }

			eElem	what;
			u32		index;
		};

	private:
		IniFileSection*						priv_doAddaddSubsection (const char *nameIN);
		const char*							priv_get (const char *identifier) const;
		void								priv_set (const char *identifierName, const char *valueIN, u32 valuelen);
		IniFileSection*						priv_simpleSubsectionExists (const char *name) const;
												//cerca una subsection di this, senza calcolare evenutali "."

		void								priv_toJSon_writeIdentifierValue (u32 i, BufferLinear &buffer, u32 &ct) const;
		u32									priv_simpleIdentifierExists (const char *name) const;

	private:
		Allocator							*allocator;
		u32									startAtLine;
		Array<sElem>						elements;
		Array<UTF8String>					blob;
		Array<UTF8String>					comments;
		Array<UTF8String>					identifier;
		Array<UTF8String>					value;
		FastArray<IniFileSection*>			subSection;

	friend IniFile;
	};


	/******************************************
	 * IniFileArrayHelper
	 */
	class IniFileArrayHelper
	{
	public:
				IniFileArrayHelper();
				~IniFileArrayHelper()						{ list.unsetup(); }
		u32 	acquireNetxArrayIndex (const char *name);

	private:
		struct sElem
		{
			char	name[128];
			u32		nextID;
		};

	private:
		FastArray<sElem> list;
	};
};
#endif //_gosIniFile_h_