#include "gosIniFile.h"
#include "gos.h"

using namespace gos;

//********************************************
IniFileArrayHelper::IniFileArrayHelper()
{
	list.setup (gos::getScrapAllocator(), 4);
}

//********************************************
u32 IniFileArrayHelper::acquireNetxArrayIndex (const char *name)
{
	const u32 n = list.getNElem();
	for (u32 i=0; i<n; i++)
	{
		if (strcasecmp(name, list(i).name) == 0)
			return list[i].nextID++;
	}

	strcpy_s (list[n].name, sizeof(list[n].name), name);
	list[n].nextID = 1;
	return 0;
}

//********************************************
IniFile::IniFile()
{
	allocator = NULL;
	root = NULL;
//	loadBuffer = NULL;
}

//********************************************
IniFile::~IniFile()
{
	unsetup();
}

//********************************************
void IniFile::unsetup()
{
	if (NULL == allocator)
		return;
	GOSDELETE(allocator, root);
	allocator = NULL;
}

//********************************************
void IniFile::reset()
{
	GOSDELETE(allocator, root);
	filename = "";
}

//********************************************
void IniFile::priv_errorMessageNear (const UTF8String &msg, const string::utf8::Iter &src) const
{
	char temp[256];
	src.copyStrFromCurrentPositionToEnd (temp, sizeof(temp));
	gos::logger::err ("%s was expected near %s", msg.getBuffer(), temp);
}

//********************************************
void IniFile::setSaveFilename (const char* filenameIN)
{
	this->filename = filenameIN;
}

//********************************************
void IniFile::save() const
{
	saveAs(filename.getBuffer());
}

//********************************************
void IniFile::saveAs (const char* filenameIN) const
{
	gos::File f;
	if (!fs::fileOpenForW (&f, filenameIN))
	{
		DBGBREAK;
		return;
	}

	root->save (f, 0, 0);
    fs::fileClose(f);
}


//********************************************
bool IniFile::priv_Parse_separator_Value (string::utf8::Iter &src, string::utf8::Iter *result, char separator) const
{
	string::utf8::toNextValidChar(src);

	// mi aspetto un carattere separatore
	if (src.getCurChar() != separator)
	{
		UTF8String temp;
		temp = "IniFile::priv_Parse_Identifier_separator_Value () -> '";
		temp.append (&separator, 1);
		temp.append ("'", 1);
		priv_errorMessageNear (temp, src);
		return false;
	}
	src.advanceOneChar();

	//mi aspetto un value
	string::utf8::toNextValidChar(src);

	UTF8Char closer[2] = { UTF8Char('\r'), UTF8Char('\n') };
	if (!string::utf8::extractValue (src, result, closer, 2))
	{
		priv_errorMessageNear ("IniFile::Parse () -> A valid string", src);
		return false;
	}
	return true;
}

//********************************************
void IniFile::createEmpty (const char* filenameIN)
{
	reset();
	filename = filenameIN;

	if (!allocator)
		allocator = gos::getSysHeapAllocator();
	root = GOSNEW (allocator, IniFileSection)(allocator);
}

//********************************************
bool IniFile::loadAndParse (const char *filenameIN)
{
	reset();
	u32 fileSize;
	u8 *buffer = fs::fileLoadInMemory (gos::getScrapAllocator(), filenameIN, &fileSize);
	if (NULL == buffer)
		return false;

	//se c'e' il BOM, lo elimino
	if (fileSize > 3)
	{
		if (buffer[0] == 0xEF && buffer[1] == 0xBB && buffer[2] == 0xBF)
		{
			fileSize -= 3;
			memcpy (buffer, &buffer[3], fileSize);
		}
	}

	bool ret = parseFromMemory (reinterpret_cast<const char*>(buffer), fileSize);
	GOSFREE(gos::getScrapAllocator(), buffer);

	if (ret)
		filename = filenameIN;
	return ret;	
}

//********************************************
bool IniFile::parseFromMemory (const void *buffer, u32 sizeOfBuffer)
{
	if (!allocator)
		allocator = gos::getSysHeapAllocator();

	GOSDELETE(allocator, root);
	root = GOSNEW (allocator, IniFileSection)(allocator);

	string::utf8::Iter src;
	src.setup (reinterpret_cast<const char *>(buffer), 0, sizeOfBuffer);
	if (!priv_Parse_Section (root, src))
	{
		//gos::logErr ("IniFile::parseFromMemory() -> filename: [%s]\n", filename);
		return false;
	}
	return true;
}


//********************************************
void IniFile::priv_toNextValidChar (IniFileSection *section, string::utf8::Iter &src) const
{
	const UTF8Char	cTabAndBlank[2] = { UTF8Char(' '), UTF8Char('\t') };

	u32 nLine = 0;
	while (!src.getCurChar().isEOF())
	{
		//mi porto sul primo char buono
		string::utf8::skip (src, cTabAndBlank, 2);

		if (src.getCurChar() == '\n' || src.getCurChar() == '\r')
		{
			string::utf8::skipEOL (src);
			++nLine;
		}
		else
			break;
	}

	if (nLine)
	{
		if (nLine > 32)
			nLine = 32;
		char eol[32];
		memset (eol, '\n', 32);
		section->addBlob (eol, nLine);
	}
}

//********************************************
bool IniFile::priv_Parse_Section (IniFileSection *section, string::utf8::Iter &src)
{
	IniFileArrayHelper	arrayHelper;

	string::utf8::Iter result;
	while (!src.getCurChar().isEOF())
	{
		//mi porto sul primo char buono
		priv_toNextValidChar (section, src);
		if (src.getCurChar().isEOF())
			break;

		//qui puo' esserci un commento
		if (string::utf8::extractCPPComment (src, &result))
		{
			section->addComment (result.getPointerToCurrentPosition(), result.getBytesLeft());
			string::utf8::skipEOL(src);
			continue;
		}

		//oppure la fine della sezione
		if (src.getCurChar() == '}')
		{
			string::utf8::advanceToEOL (src, true);
			break;
		}


		//mi aspetto un identifier
		char identifierName[128];
		if (string::utf8::extractIdentifier (src, &result))
			result.copyAllStr (identifierName, sizeof(identifierName));
		else
		{
			//ok, in generale questo o' un errore ma c'o' un caso particolare. Consento alle sezioni di chiamarsi [nomeSezione] ovvero
			//con le parentesi quadre attorno al nome. Questa sintassi vuol dire che posso avere nel file n sezioni con lo stesso nome
			//e che d'ufficio io appendo un numero univoco al nome della sezione durante il parsing
			
			const UTF8Char parentesiQuadraAperta('[');
			const UTF8Char parentesiQuadraChiusa(']');
			bool isErr = true;
			while (1)
			{
				if (src.getCurChar() != parentesiQuadraAperta)
					break;

				src.advanceOneChar();
				if (!string::utf8::extractIdentifier (src, &result, &parentesiQuadraChiusa, 1))
					break;

				//mi assicuro che la ] sia l'ultimo char
				result.copyAllStr (identifierName, sizeof(identifierName));
				u32 n = string::utf8::lengthInByte(identifierName);
				if (identifierName[n - 1] != ']')
					break;
				identifierName[n - 1] = 0;

				//trasformo il nome eliminando [] e aggiungendo @arrayIndex
				{
					const u32 arrayIndex = arrayHelper.acquireNetxArrayIndex(identifierName);
					char num[32];
					sprintf_s (num, sizeof(num), "@%d@", arrayIndex);
					string::utf8::concatStr (identifierName, sizeof(identifierName), num);
				}

				isErr = false;
				break;
			}
			
			if (isErr)
			{
				priv_errorMessageNear ("IniFile::Parse () -> A valid identifier", src);
				return false;
			}
		}
		

		//mi porto sul primo char buono
		string::utf8::toNextValidChar (src);

		//a questo punto o c'e' un valore (identifier : valore) oppure l'inizio di una sezione ({)
		if (src.getCurChar() == '{')
		{
			IniFileSection *subSection = section->addSubsection (identifierName);
			src.advanceOneChar();
			string::utf8::toNextValidChar(src);
			string::utf8::skipEOL(src);
			if (!priv_Parse_Section (subSection, src))
				return false;
			continue;
		}
		
		//dato che non e' iniziata una sezione, mi aspetto identifier : value
		if (!priv_Parse_separator_Value (src, &result, ':'))
			return false;
		
		//section->priv_set (identifierName, result.getPointerToCurrentPosition(), result.getBytesLeft());
		
		//unescape del valore
		char temp[512];
		result.copyStrFromCurrentPositionToEnd (temp, sizeof(temp));
		const u32 len = string::utf8::unescapeInPlace (temp, u32MAX);
		section->priv_set (identifierName, temp, len);

		string::utf8::advanceToEOL(src, true);
	}
	return true;
}


//********************************************
bool IniFile::exists  (const char *identifier) const
{
	if (NULL == root)
		return false;
	return root->exists (identifier);
}

//********************************************
bool IniFile::get (const char *identifier, UTF8String &out) const
{
	if (NULL == root)
		return false;
	return root->get (identifier, out);
}

//********************************************
bool IniFile::get (const char *identifier, char *out, u32 sizeofout) const
{
	if (NULL == root)
		return false;
	return root->get (identifier, out, sizeofout);
}

//********************************************
void IniFile::getOrDefault (const char *identifier, const char *defaultValue, UTF8String &out) const
{
	if (NULL == root)
		out = defaultValue;
	else
		root->getOrDefault (identifier, defaultValue, out);
}

//*******************************************
void IniFile::getOrDefault (const char *identifier, const char *defaultValue, char *out, u32 sizeofout) const
{
	if (NULL == root)
	{
		assert (NULL != defaultValue);
		u32 n = string::utf8::lengthInByte(defaultValue);
		if (n>=sizeofout)
		{
			n = sizeofout-1;
			DBGBREAK;
		}
		memcpy (out, defaultValue, n);
		out[n] = 0;
	}
	else
		root->getOrDefault (identifier, defaultValue, out, sizeofout);
}

//********************************************
bool IniFile::checkString (const char *identifier, const char *valueToCmp, bool bCaseSens) const
{
	if (NULL == root)
		return false;
	return root->checkString (identifier, valueToCmp, bCaseSens);
}

//********************************************
f32 IniFile::getOrDefaultAsF32 (const char *identifier, f32 defaultValue) const
{
	if (NULL == root)
		return defaultValue;
	return root->getOrDefaultAsF32 (identifier, defaultValue);
}

//********************************************
u64 IniFile::getOrDefaultAsU64 (const char *identifier, u64 defaultValue) const
{
	if (NULL == root)
		return defaultValue;
	return root->getOrDefaultAsU64 (identifier, defaultValue);
}

//********************************************
u32 IniFile::getOrDefaultAsU32 (const char *identifier, u32 defaultValue) const
{
	if (NULL == root)
		return defaultValue;
	return root->getOrDefaultAsU32 (identifier, defaultValue);
}

//********************************************
u16 IniFile::getOrDefaultAsU16 (const char *identifier, u16 defaultValue) const
{
	if (NULL == root)
		return defaultValue;
	return root->getOrDefaultAsU16 (identifier, defaultValue);
}

//********************************************
u8 IniFile::getOrDefaultAsU8 (const char *identifier, u8 defaultValue) const
{
	if (NULL == root)
		return defaultValue;
	return root->getOrDefaultAsU8 (identifier, defaultValue);
}


//********************************************
bool IniFile::getOrDefaultAsBool (const char *identifier, bool defaultValue) const
{
	if (NULL == root)
		return defaultValue;
	return root->getOrDefaultAsBool (identifier, defaultValue);
}

//********************************************
i64 IniFile::getOrDefaultAsI64 (const char *identifier, i64 defaultValue) const
{
	if (NULL == root)
		return defaultValue;
	return root->getOrDefaultAsI64 (identifier, defaultValue);
}

//********************************************
i32 IniFile::getOrDefaultAsI32 (const char *identifier, i32 defaultValue) const
{
	if (NULL == root)
		return defaultValue;
	return root->getOrDefaultAsI32 (identifier, defaultValue);
}

//********************************************
i16 IniFile::getOrDefaultAsI16 (const char *identifier, i16 defaultValue) const
{
	if (NULL == root)
		return defaultValue;
	return root->getOrDefaultAsI16 (identifier, defaultValue);
}

//********************************************
i8 IniFile::getOrDefaultAsI8 (const char *identifier, i8 defaultValue) const
{
	if (NULL == root)
		return defaultValue;
	return root->getOrDefaultAsI8 (identifier, defaultValue);
}


//********************************************
i32 IniFile::getOrDefaultHexToI32 (const char *identifier, const char *defaultValue) const
{
	if (NULL == root)
	{
		u32 out;
		string::ansi::hexToInt ((const char*)defaultValue, &out);
		return (i32)out;
	}
	return root->getOrDefaultHexToI32 (identifier, defaultValue);
}

//********************************************
void IniFile::set (const char *identifier, const char* value, bool bCreateIfNotFound)
{
	if (NULL == root)
		root = GOSNEW(allocator, IniFileSection)(allocator);
	root->set (identifier, value, bCreateIfNotFound);
}

//********************************************
IniFileSection*	IniFile::getOrCreateSubsection (const char *name)			{ if (NULL == root) return NULL; return root->getOrCreateSubsection(name); }
IniFileSection*	IniFile::getSubsection (const char *name) const				{ if (NULL == root) return NULL; return root->getSubsection(name); }
u32				IniFile::getNSubsection () const							{ if (NULL == root) return 0; return root->getNSubsection(); }
IniFileSection*	IniFile::getSubsectionByIndex (u32 i) const					{ if (NULL == root) return NULL; return root->getSubsectionByIndex(i); }

//********************************************
char* IniFile::toJSon (Allocator *allocatorIN, u32 *out_allocatedSize) const
{
	if (NULL == root)
	{
		*out_allocatedSize = 0;
		return NULL;
	}

	const u32 nSubsection = root->subSection.getNElem();

	BufferLinear buffer;
	buffer.setup (allocatorIN, 2 + nSubsection);


	u32 ct = 0;
	char c;
	
	c='{'; buffer.write (&c, ct, 1); ct++;
		for (u32 i = 0; i < nSubsection; i++)
		{
			root->subSection(i)->toJSon (buffer, ct);
			if (i != nSubsection - 1)
			{
				c = ',';
				buffer.write (&c, ct, 1);
				ct++;
			}
		}
	c='}'; buffer.write (&c, ct, 1); ct++;

	char *ret = GOSALLOCT(char*, allocatorIN, ct);
	memcpy (ret, buffer._getPointer(0), ct);
	*out_allocatedSize = ct;
	return ret;	
}

//********************************************
bool IniFile::fromJSon (const char *jsonSRC, u32 sizeOfJSonSRC)
{
	reset();
	filename = "";
	if (!allocator)
		allocator = gos::getSysHeapAllocator();
	root = GOSNEW (allocator, IniFileSection)(allocator);

	gos::string::utf8::Iter iter;
	iter.setup (jsonSRC, 0, sizeOfJSonSRC);

	return root->fromJSon (iter);
}

//********************************************
void IniFile::mergeWith (const IniFile &otherIniFile, bool onConflictUseValueFromOtherIniFile)
{
	const u32 numSubsectionB = otherIniFile.getNSubsection();
	for (u32 iSecB = 0; iSecB < numSubsectionB; iSecB++)
	{
		const IniFileSection *secB = otherIniFile.getSubsectionByIndex(iSecB);
		if (NULL == secB)
		{
			DBGBREAK;
			continue;
		}

		//se la sezione di otherIniFile non esiste in this, la creo
		IniFileSection *sec = getOrCreateSubsection(secB->name.getBuffer());

		//per ogni identificatore esistente nella sezione di otherIniFile...
		const u32 numIdentifierB = secB->getNIdentifier();
		for (u32 i = 0; i < numIdentifierB; i++)
		{
			//recupero il nome dell'identificatore
			const char* nameB = secB->getIdentifierByIndex(i);
			if (NULL == nameB)
			{
				DBGBREAK;
				continue;
			}

			//se l'identificatore non esiste in this, allora lo creo e uso il valore id otherIniFile
			if (u32MAX == sec->identifierExists(nameB))
			{
				sec->set (nameB, secB->getValueByIndex(i), true);
			}
			else
			{
				//se invece esiste, sovrascrivo il valore solo se onConflictUseValueFromOtherIniFile==true
				if (onConflictUseValueFromOtherIniFile == true)
					sec->set (nameB, secB->getValueByIndex(i));
			}
		}
	}
}

//*************************************************************
bool IniFile::_resolveInplace_identifierThatMayHaveArrayIndexing (char *in_out_name, u32 lenof_name)
{
	//nomi del tipo   pippo[2] vengono trasfomati in pippo@3@

	if (lenof_name < 2)
		return false;
	
	lenof_name--;
	if (in_out_name[lenof_name] != ']')
		return false;

	in_out_name[lenof_name] = '@';
	while (lenof_name--)
	{
		if (in_out_name[lenof_name] == '[')
		{
			in_out_name[lenof_name] = '@';
			return true;
		}
	}

	//qualcosa e' andato storto.
	//il noma aveva una quadra chiusa alla fine ma non ho trovato la quadra aperta
	DBGBREAK;
	return false;
}