#include "gosIniFile.h"
#include "gos.h"

#ifdef GOS_COMPILER__MSVC
	#pragma warning(disable:4456)
#endif

using namespace gos;


//********************************************
IniFileSection::IniFileSection (Allocator *alloc)
{
	assert (alloc);
	allocator = alloc;
	name.setAllocator (allocator);
	subSection.setup (alloc, 4);
	identifier.setup (alloc, 32);
	comments.setup (alloc, 32);
	blob.setup (alloc, 128);
	elements.setup (alloc, 128);
	value.setup (alloc, 64);
}

//********************************************
IniFileSection::~IniFileSection()
{
	u32 n = subSection.getNElem();
	for (u32 i=0; i<n; i++)
		GOSDELETE(allocator, subSection[i]);
}


//********************************************
IniFileSection* IniFileSection::priv_doAddaddSubsection (const char *nameIN)
{
	IniFileSection *ret = GOSNEW(allocator, IniFileSection)(allocator);
	ret->name = nameIN;
	u32 n = subSection.getNElem();
	subSection[n] = ret;

	u32 n2 = elements.getNElem();
	elements[n2].what = eElem::subsection;
	elements[n2].index = n;

	return ret;
}

//********************************************
IniFileSection* IniFileSection::addSubsection (const char *nameIN)
{
	IniFileSection *ret;

	//potrebbe essere il nome di un array
	if (nameIN[0] != '[')
	{
		ret = priv_simpleSubsectionExists (nameIN);
		if (NULL == ret)
			ret = priv_doAddaddSubsection (nameIN);
		return ret;
	}

	//lo e'... devo convertire  [name] in name@index@
	char arrayName[128];
	sprintf_s (arrayName, sizeof(arrayName), "%s", &nameIN[1]);
	arrayName[strlen(arrayName)-1] = 0x00;

	u32 index = 0;
	while (1)
	{
		char s[256];
		sprintf_s (s, sizeof(s), "%s@%d@", arrayName, index++);
		if (NULL == priv_simpleSubsectionExists (s))
			return priv_doAddaddSubsection(s);
	}
}

//********************************************
u32 IniFileSection::priv_simpleIdentifierExists (const char *name) const
{
	u32 n = identifier.getNElem();
	for (u32 i=0; i<n; i++)
	{
		if (identifier(i).isEqualTo (name, false))
			return i;
	}
	return u32MAX;
}

//********************************************
u32 IniFileSection::identifierExists (const char *nameIN) const
{
	//identificatori del tipo pippo[3] sono equivalenti a pippo@3@
	u32 len = string::utf8::lengthInByte(nameIN);
	if (nameIN[len-1] != ']')
		return priv_simpleIdentifierExists (nameIN);

	if (nameIN[0] == '[')
		return u32MAX;

	char name[256];
	memcpy (name, nameIN, len-1);
	name[len-1] = '@';
	name[len] = 0x00;

	while (len--)
	{
		if (name[len] == '[')
		{
			name[len] ='@';
			return priv_simpleIdentifierExists (name);
		}
	}

	return priv_simpleIdentifierExists (nameIN);
}

//********************************************
IniFileSection* IniFileSection::priv_simpleSubsectionExists (const char *name) const
{
	u32 n = subSection.getNElem();
	for (u32 i=0; i<n; i++)
	{
		if (subSection(i)->name.isEqualTo (name, false))
			return subSection.getElem(i);
	}
	return NULL;
}


//********************************************
void IniFileSection::set (const char *identifierName, const char *valueIN, bool bCreateIfNotFound)
{
	assert (NULL != identifierName && NULL != allocator);
	
	const UTF8Char cPunto('.');
	gos::string::utf8::Iter src;
	src.setup (identifierName, 0, (u32)string::utf8::lengthInByte(identifierName));
	
	if (!string::utf8::advanceUntil (src, &cPunto, 1))
	{
		//non ho trovato il "."
		u32 index = identifierExists (identifierName);
		if (u32MAX != index)
			value[index].setFrom (valueIN, (u32)string::utf8::lengthInByte(valueIN));
		else
		{
			if (bCreateIfNotFound)
				priv_set (identifierName, valueIN, string::utf8::lengthInByte(valueIN));
		}
		return;
	}
	else
	{
		//ho trovato il "."
		//estraggo il nome della sezione
		char subSectionName[128];
		src.copyStrFromXToCurrentPosition (0, subSectionName, sizeof(subSectionName), false);
		src.advanceOneChar(); //skippo il "."
		
		//la cerco (eventualmente la creo)
		IniFileSection *subSection = priv_simpleSubsectionExists (subSectionName);
		if (NULL == subSection)
		{
			if (bCreateIfNotFound)
				subSection = addSubsection (subSectionName);
			else
				return;
		}

		subSection->set (src.getPointerToCurrentPosition(), valueIN, bCreateIfNotFound);
	}

}

//********************************************
void IniFileSection::priv_set (const char *identifierNameIN, const char *valueIN, u32 valuelen)
{	
	const char *identifierName= identifierNameIN;
	char s[256];

	//potrebbe essere il nome di un array
	if (identifierNameIN[0] == '[')
	{
		//lo e'... devo convertire  [name] in name@index@
		char arrayName[128];
		sprintf_s (arrayName, sizeof(arrayName), "%s", &identifierNameIN[1]);
		arrayName[strlen(arrayName)-1] = 0x00;

		u32 index = 0;
		while (1)
		{
			sprintf_s (s, sizeof(s), "%s@%d@", arrayName, index++);
			if (u32MAX == priv_simpleIdentifierExists (s))
			{
				identifierName = s;
				break;
			}
		}
	}

	u32 i = identifierExists (identifierName);
	if (u32MAX == i)
	{
		i = identifier.getNElem();
		identifier[i] = identifierName;

		u32 n2 = elements.getNElem();
		elements[n2].what = eElem::identifierValue;
		elements[n2].index = i;
	}

	value[i].setFrom (valueIN, valuelen);
}

//********************************************
const char* IniFileSection::priv_get (const char *identifierName) const
{
	assert (NULL != identifierName);
	
	gos::string::utf8::Iter src;
	src.setup (identifierName, 0, (u32)string::utf8::lengthInByte(identifierName));
		
	const UTF8Char cPunto('.');
	if (!string::utf8::advanceUntil (src, &cPunto, 1))
	{
		//non ho trovato il "."
		u32 index = identifierExists (identifierName);
		if (u32MAX == index)
		{
			//potrebbe trattarsi di un identifierName che indicizza un array
			char s[128];
			strcpy_s (s, sizeof(s), identifierName);
			if (!IniFile::_resolveInplace_identifierThatMayHaveArrayIndexing (s, string::utf8::lengthInByte(s)))
				return NULL;

			index = identifierExists (s);
			if (u32MAX == index)
				return NULL;
		}
		const char *ret = getValueByIndex (index);
		return ret;
	}

	//ho trovato il "."
	char subSectionName[128];
	src.copyStrFromXToCurrentPosition(0, subSectionName, sizeof(subSectionName), false);
	src.advanceOneChar(); //skippo il "."
	
	//potrebbe trattarsi di un identifierName che indicizza un array
	IniFile::_resolveInplace_identifierThatMayHaveArrayIndexing (subSectionName, string::utf8::lengthInByte(subSectionName));


	//la cerco
	IniFileSection *subSection = priv_simpleSubsectionExists (subSectionName);
	if (NULL == subSection)
		return NULL;

	return subSection->priv_get (src.getPointerToCurrentPosition());
}

//********************************************
void IniFileSection::addComment (const char *c, u32 len)
{
	if (NULL == c || len == 0)
		return;

	u32 n = comments.getNElem();
	comments[n].setFrom (c, len);
		
	u32 n2 = elements.getNElem();
	elements[n2].what = eElem::comment;
	elements[n2].index = n;
}

//********************************************
void IniFileSection::addBlob (const char *c, u32 len)
{
	if (NULL == c || len == 0)
		return;

	u32 n = blob.getNElem();
	blob[n].setFrom (c, len);
		
	u32 n2 = elements.getNElem();
	elements[n2].what = eElem::blob;
	elements[n2].index = n;
}

//********************************************
void IniFileSection::save (gos::File &f, u32 tabCount, u32 level) const
{
	const char oneTab[2] = { "\t" };
	const char oneGraffaOpen[2] = { "{" };
	const char oneGraffaClose[2] = { "}" };
	const char spazioDuepuntiSpazio[4] = {" : "};
	const char EOL[4] = { "\r\n" };
	const UTF8Char apiciDoppi("\"");
	char s[512];
	gos::UTF8String stemp;
	stemp.setAllocator (gos::getScrapAllocator());

	char tabs[128];
	memset (tabs, oneTab[0], sizeof(tabs));
	
#define BW_WRITE_EOL						gos::fs::fileWrite (f, EOL, 2);
#define BW_WRITE_TABS						gos::fs::fileWrite (f, tabs, tabCount);
#define BW_WRITE_GRAFFA_OPEN				gos::fs::fileWrite (f, oneGraffaOpen, 1);
#define BW_WRITE_GRAFFA_CLOSE				gos::fs::fileWrite (f, oneGraffaClose, 1);
#define BW_WRITE_GRAFFA_TAB					gos::fs::fileWrite (f, oneTab, 1);
#define BW_WRITE_SPAZIO_DUEPUNTI_SPAZIO		gos::fs::fileWrite (f, spazioDuepuntiSpazio, 3);
	
	u32 nameLen = name.lengthInByte();
	if (nameLen)
	{
		BW_WRITE_TABS

		const char *p = name.getBuffer();
		if (p[nameLen-1] == '@')
		{
			//e' un elemento di un array
			sprintf_s (s, sizeof(s), "[%s", p);
			
			while (nameLen--)
			{
				if (s[nameLen] == '@')
				{
					s[nameLen++] = ']';
					s[nameLen] = 0x00;
					p = s;
					break;
				}
			}
		}
		gos::fs::fileWrite (f, p, nameLen);
		BW_WRITE_EOL
		BW_WRITE_TABS
		BW_WRITE_GRAFFA_OPEN
		BW_WRITE_EOL
		++tabCount;
	}
		
	u32 n2 = elements.getNElem();
	for (u32 i2=0; i2<n2; i2++)
	{
		switch (elements(i2).what)
		{
		default:
			DBGBREAK;
			break;

		case eElem::blob:
			gos::fs::fileWrite (f, blob(elements(i2).index).getBuffer(), blob(elements(i2).index).lengthInByte());
			break;

		case eElem::comment:
			BW_WRITE_TABS
			gos::fs::fileWrite (f, comments(elements(i2).index).getBuffer(), comments(elements(i2).index).lengthInByte());
			BW_WRITE_EOL
			break;

		case eElem::subsection:
			subSection(elements(i2).index)->save (f, tabCount, level+1);
			break;

		case eElem::identifierValue:
			{
				u32 i = elements(i2).index;

				BW_WRITE_TABS
				{
					const char *p = identifier(i).getBuffer();
					u32 nameLen = identifier(i).lengthInByte();
					if (p[nameLen-1] == '@')
					{
						//e' un elemento di un array
						sprintf_s (s, sizeof(s), "[%s", p);
						
						while (nameLen--)
						{
							if (s[nameLen] == '@')
							{
								s[nameLen++] = ']';
								s[nameLen] = 0x00;
								p = s;
								break;
							}
						}
					}
					gos::fs::fileWrite (f, p, nameLen);
				}
				BW_WRITE_SPAZIO_DUEPUNTI_SPAZIO

				if (value(i).lengthInByte() == 0)
				{
					//se value e' vuoto, salvo ""
					char c[2] = { '\"', '\"' };
					gos::fs::fileWrite (f, c, 2);
				}
				else if (-1 == value(i).findFirst(' '))
				{
					//se in value non ci sono spazi, lo salvo cosi' com'e'
					gos::fs::fileWrite (f, value(i).getBuffer(), value(i).lengthInByte());
				}
				else
				{
					//se ci sono spazi, lo salvo racchiuso tra doppi apici o singoli apici

					/*
					char c[2] = { '\'', 0 };
					if (-1 == value(i).findFirst(apiciDoppi))
						c[0] = '"';
					gos::fs::fileWrite (f, c, 1);
					gos::fs::fileWrite (f, value(i).getBuffer(), value(i).lengthInByte());
					gos::fs::fileWrite (f, c, 1);
					*/

					value(i).escapeTo (&stemp);

					const char c[2] = { '\"', 0 };
					gos::fs::fileWrite (f, c, 1);
					gos::fs::fileWrite (f, stemp.getBuffer(), stemp.lengthInByte());
					gos::fs::fileWrite (f, c, 1);
					
				}
				BW_WRITE_EOL
			}
			break;
		}
	}
	

	if (name.lengthInByte())
	{
		gos::fs::fileWrite (f, tabs, tabCount-1);
		BW_WRITE_GRAFFA_CLOSE
		BW_WRITE_EOL
		
		if (1 == level)
		{
			BW_WRITE_EOL
			BW_WRITE_EOL
		}
	}

#undef BW_WRITE_EOL
#undef BW_WRITE_TABS
#undef BW_WRITE_GRAFFA_OPEN
#undef BW_WRITE_GRAFFA_CLOSE
#undef BW_WRITE_GRAFFA_TAB
#undef BW_WRITE_SPAZIO_DUEPUNTI_SPAZIO
}

//********************************************
const char* IniFileSection::getValueByIndex (u32 index) const
{
	if (index < value.getNElem())
		return value(index).getBuffer();
	else
	{
		DBGBREAK;
		return NULL;
	}	
}

//********************************************
const char* IniFileSection::getIdentifierByIndex (u32 index) const
{
	if (index < identifier.getNElem())
		return identifier(index).getBuffer();
	else
	{
		DBGBREAK;
		return NULL;
	}	
}

//********************************************
bool IniFileSection::exists  (const char *identifier) const
{
	return (priv_get (identifier) != NULL);
}

//********************************************
bool IniFileSection::get (const char *identifier, char *out, u32 sizeof_out) const
{
	const char *pstr = priv_get (identifier);
	if (NULL == pstr)
		return false;

	u32 n = (u32)string::utf8::lengthInByte(pstr);
	if (n>=sizeof_out)
	{
		n = sizeof_out-1;
		DBGBREAK;
	}
	memcpy (out, pstr, n);
	out[n] = 0;

	return true;
}

//********************************************
void IniFileSection::getOrDefault (const char *identifier, const char *defaultValue, UTF8String &out) const
{
	if (get(identifier, out))
		return;
	out = defaultValue;
}

//*******************************************
void IniFileSection::getOrDefault (const char *identifier, const char *defaultValue, char *out, u32 sizeof_out) const
{
	if (get (identifier, out, sizeof_out))
		return;
	
	assert (NULL != defaultValue);
	u32 n = (u32)string::utf8::lengthInByte(defaultValue);
	if (n>=sizeof_out)
	{
		n = sizeof_out-1;
		DBGBREAK;
	}
	memcpy (out, defaultValue, n);
	out[n] = 0;
}

//********************************************
bool IniFileSection::checkString (const char *identifier, const char *valueToCmp, bool bCaseSensitive) const
{
	char out[256];
	if (!get (identifier, out, sizeof(out)))
		return false;
	return string::utf8::areEqual (out, valueToCmp, bCaseSensitive);
}

//********************************************
f32 IniFileSection::getOrDefaultAsF32 (const char *identifier, f32 defaultValue) const
{
	const char *s = priv_get(identifier);
	if (!s)
		return defaultValue;
	return string::utf8::toF32(s);
}

//********************************************
bool IniFileSection::getOrDefaultAsBool (const char *identifier, bool defaultValue) const
{
	const char *s = priv_get(identifier);
	if (!s)
		return defaultValue;
	if (string::utf8::toI32(s) == 0)
		return false;
	return true;
}

//********************************************
u64 IniFileSection::getOrDefaultAsU64 (const char *identifier, u64 defaultValue) const
{
	const char *s = priv_get(identifier);
	if (!s)
		return defaultValue;
	return string::utf8::toU64(s);
}

//********************************************
u32 IniFileSection::getOrDefaultAsU32 (const char *identifier, u32 defaultValue) const
{
	u64 ret = getOrDefaultAsU64(identifier, defaultValue);
	if (ret > u32MAX)
		ret = u32MAX;
	return static_cast<u32>(ret);
}

//********************************************
u16 IniFileSection::getOrDefaultAsU16 (const char *identifier, u16 defaultValue) const
{
	u64 ret = getOrDefaultAsU64 (identifier, defaultValue);
	if (ret > u16MAX)
		ret = u16MAX;
	return static_cast<u16>(ret);
}

//********************************************
u8 IniFileSection::getOrDefaultAsU8 (const char *identifier, u8 defaultValue) const
{
	u64 ret = getOrDefaultAsU64 (identifier, defaultValue);
	if (ret > 0xff)
		ret = 0xff;
	return static_cast<u8>(ret);
}

//********************************************
i64 IniFileSection::getOrDefaultAsI64 (const char *identifier, i64 defaultValue) const
{
	const char *s = priv_get(identifier);
	if (!s)
		return defaultValue;
	return string::utf8::toI64(s);
}

//********************************************
i32 IniFileSection::getOrDefaultAsI32 (const char *identifier, i32 defaultValue) const
{
	i64 ret = getOrDefaultAsI64 (identifier, defaultValue);
	if (ret > i32MAX)		ret = i32MAX;
	else if (ret < i32MIN)	ret = i32MIN;
	return static_cast<i32>(ret);
}

//********************************************
i16 IniFileSection::getOrDefaultAsI16 (const char *identifier, i16 defaultValue) const
{
	i64 ret = getOrDefaultAsI64 (identifier, defaultValue);
	if (ret > i16MAX)		ret = i16MAX;
	else if (ret < i16MIN)	ret = i16MIN;
	return static_cast<i16>(ret);
}

//********************************************
i8 IniFileSection::getOrDefaultAsI8 (const char *identifier, i8 defaultValue) const
{
	i64 ret = getOrDefaultAsI64 (identifier, defaultValue);
	if (ret > i8MAX)		ret = i8MAX;
	else if (ret < i8MIN)	ret = i8MIN;
	return static_cast<i8>(ret);
}

//********************************************
i32 IniFileSection::getOrDefaultHexToI32 (const char *identifier, const char *defaultValue) const
{
	u32 out;
	const char *s = priv_get(identifier);
	if (!s)
		string::ansi::hexToInt ((const char*)defaultValue, &out, (u32)string::utf8::lengthInByte(defaultValue));
	else		
		string::ansi::hexToInt ((const char*)s, &out, (u32)string::utf8::lengthInByte(s));
	return static_cast<i32>(out);
}

//********************************************
IniFileSection* IniFileSection::getSubsection (const char *identifierName) const
{
	if (NULL==identifierName || (NULL!=identifierName && identifierName[0]==0x00))
	{
		DBGBREAK;
		return NULL;
	}

	gos::string::utf8::Iter src;
	src.setup (identifierName, 0, (u32)string::utf8::lengthInByte(identifierName));
	
	const UTF8Char cPunto('.');
	char subSectionName[128];
	if (!string::utf8::advanceUntil (src, &cPunto, 1))
	{
		strcpy_s (subSectionName, sizeof(subSectionName), identifierName);
		IniFile::_resolveInplace_identifierThatMayHaveArrayIndexing (subSectionName, string::utf8::lengthInByte(subSectionName));
		return priv_simpleSubsectionExists (subSectionName);
	}


	//estraggo il nome della sezione
	src.copyStrFromXToCurrentPosition (0, subSectionName, sizeof(subSectionName), false);
	src.advanceOneChar(); //skippo il punto

	//la cerco
	IniFile::_resolveInplace_identifierThatMayHaveArrayIndexing (subSectionName, string::utf8::lengthInByte(subSectionName));
	IniFileSection *subSection = priv_simpleSubsectionExists (subSectionName);
	if (NULL == subSection)
		return NULL;
	return subSection->getSubsection (src.getPointerToCurrentPosition());
}

//********************************************
IniFileSection* IniFileSection::getOrCreateSubsection (const char *name)
{
	if (NULL == name || (NULL != name && name[0] == 0x00))
	{
		DBGBREAK;
		return NULL;
	}

	gos::string::utf8::Iter src;
	src.setup (name, 0, (u32)string::utf8::lengthInByte(name));

	const UTF8Char cPunto('.');
	if (!string::utf8::advanceUntil (src, &cPunto, 1))
	{
		//non c'e' il punto
		IniFileSection *ret = this->priv_simpleSubsectionExists (name);
		if (NULL == ret)
			ret = addSubsection(name);
		return ret;
	}

	//estraggo il nome della sezione
	char subSectionName[128];
	src.copyStrFromXToCurrentPosition (0, subSectionName, sizeof(subSectionName), false);
	src.advanceOneChar(); //skippo il punto

	//la cerco
	IniFileSection *subSection = priv_simpleSubsectionExists (subSectionName);
	if (NULL == subSection)
		subSection = addSubsection(subSectionName);
	return subSection->getOrCreateSubsection (src.getPointerToCurrentPosition());
}

//********************************************
void IniFileSection::priv_toJSon_writeIdentifierValue (u32 i, BufferLinear &buffer, u32 &ct) const
{
	UTF8String temp;
	temp.setAllocator (gos::getScrapAllocator());
	value(i).escapeTo (&temp);

	char c;
	c='"'; buffer.write (&c, ct, 1); ct++;
	buffer.write (identifier(i).getBuffer(), ct, identifier(i).lengthInByte()); ct+=identifier(i).lengthInByte();
	c='"'; buffer.write (&c, ct, 1); ct++;
		
	c=':'; buffer.write (&c, ct, 1); ct++;
		
	c='"'; buffer.write (&c, ct, 1); ct++;
	//buffer.write (value(i).getBuffer(), ct, value(i).lengthInByte()); ct+=value(i).lengthInByte();
	buffer.write (temp.getBuffer(), ct, temp.lengthInByte()); ct+=temp.lengthInByte();
	c='"'; buffer.write (&c, ct, 1); ct++;
}

//********************************************
void IniFileSection::toJSon (BufferLinear &buffer, u32 &ct) const
{
	u32 totalLen = 5+name.lengthInByte();		// "name": {},

	for (u32 i = 0; i < identifier.getNElem(); i++)
	{
		totalLen += 3 + identifier(i).lengthInByte();	// "id":
		//totalLen += 3 + value(i).lengthInByte();		// "value",
		totalLen += 3 + gos::string::utf8::calcEscapedSeqLength (value(i).getBuffer(), value(i).lengthInByte());		// "value",
	}

	buffer.growUpTo (ct + totalLen);

	char c;
	
	//section name
	c='"'; buffer.write (&c, ct, 1); ct++;
	buffer.write (name.getBuffer(), ct, name.lengthInByte());	ct+=name.lengthInByte();
	c='"'; buffer.write (&c, ct, 1); ct++;
	c=':'; buffer.write (&c, ct, 1); ct++;
	c='{'; buffer.write (&c, ct, 1); ct++;


		//coppie "id":"value"
		const u32 nIdentifier = identifier.getNElem();
		for (u32 i = 0; i < nIdentifier; i++)
		{
			priv_toJSon_writeIdentifierValue (i, buffer, ct);
		
			if (i != nIdentifier - 1)
			{
				c = ',';
				buffer.write (&c, ct, 1);
				ct++;
			}
		}

		//sottosezioni
		const u32 nSubSection = subSection.getNElem();
		if (nSubSection)
		{
			c = ','; buffer.write (&c, ct, 1); ct++;

			for (u32 i = 0; i < nSubSection; i++)
			{
				subSection(i)->toJSon (buffer, ct);

				if (i != nSubSection - 1)
				{
					c = ',';
					buffer.write (&c, ct, 1);
					ct++;
				}
			}
		}

	c='}'; buffer.write (&c, ct, 1); ct++;
}

//********************************************
bool IniFileSection::fromJSon (gos::string::utf8::Iter &iter)
{
	static const gos::UTF8Char chDoppioApice ('"');
	static const gos::UTF8Char chVirgola (',');
	static const gos::UTF8Char chDuepunti (':');
	static const gos::UTF8Char chGraffaAperta ('{');
	static const gos::UTF8Char chGraffaChiusa ('}');
	static const gos::UTF8Char chQuadraAperta ('[');
	static const gos::UTF8Char chQuadraChiusa (']');
	
	static constexpr u8 NUM_VALID_INTEGER_CLOSING_CHAR = 9;
	static const gos::UTF8Char validIntegerClosingChars[NUM_VALID_INTEGER_CLOSING_CHAR] = { ' ', '\r', '\n', '\t', '{', ',', '}', '[', ']' };

	//json inizia e termina con { }
	gos::string::utf8::toNextValidChar(iter);
	if (iter.getCurChar() != chGraffaAperta)
		return false;
	iter.advanceOneChar();
	gos::string::utf8::toNextValidChar(iter);


	while (1)
	{
		gos::string::utf8::toNextValidChar(iter);
		if (iter.getCurChar().isEOF())
		{
			//se sono "root", va bene, vuol dire che abbiamo finito
			if (name.lengthInByte() == 0)
				return true;
		}

		if (iter.getCurChar() == chGraffaChiusa)
		{
			//se sono "root", va bene, vuol dire che abbiamo finito
			if (name.lengthInByte() == 0)
				return true;
		}

		//mi aspetto un "
		if (iter.getCurChar() != chDoppioApice)
			return false;
		iter.advanceOneChar();

		//mi aspetto un identificatore
		gos::string::utf8::Iter it2;
		if (!gos::string::utf8::extractIdentifier (iter, &it2, NULL, 0))
			return false;

		//mi aspetto un "
		if (iter.getCurChar() != chDoppioApice)
			return false;
		iter.advanceOneChar();

		//mi aspetto un :
		gos::string::utf8::toNextValidChar(iter);
		if (iter.getCurChar() != chDuepunti)
			return false;
		iter.advanceOneChar();

		//mi aspetto un "{" una "[" oppure "
		gos::string::utf8::toNextValidChar(iter);
		if (iter.getCurChar() == chGraffaAperta)
		{
			//inizio di una nuova sezione
			char identifier[256];
			it2.copyStrFromCurrentPositionToEnd (identifier, sizeof(identifier));
			IniFileSection *sub = this->addSubsection (identifier);
			if (false == sub->fromJSon(iter))
				return false;

			//a questo punto mi aspetto una "," oppure una "}"
			gos::string::utf8::toNextValidChar(iter);
			if (iter.getCurChar() == chVirgola)
			{
				iter.advanceOneChar();
				continue;
			}

			if (iter.getCurChar() == chGraffaChiusa)
			{
				//fine della sezione corrente
				iter.advanceOneChar();
				return true;
			}
		}
		else if (iter.getCurChar() == chQuadraAperta)
		{
			//inizio di un array
			char arrayName[200];
			it2.copyStrFromCurrentPositionToEnd (arrayName, sizeof(arrayName));
			iter.advanceOneChar();
			
			//dopo [, puo' esserci { oppure un elenco di valori
			char s[256];
			gos::string::utf8::toNextValidChar(iter);
			if (iter.getCurChar() == chGraffaAperta)
			{
				/* Siamo in questo caso: [ {} ... ]
				 	Es:
						"autori": [ 
							{ 
								"nome" : "gianni",
								"eta'" : 10
							},
							{ 
								"nome" : "marco",
								"eta'" : 91
							}							
						]

					diventa:

						[autori]
						{
							nome: gianni
							età : 10
						}

						[autori]
						{
							nome: marco
							età : 91
						}							
				*/
				while (1)
				{
					//creo una nuova sezione
					sprintf_s (s, sizeof(s), "[%s]", arrayName);
					IniFileSection *sub = this->addSubsection (s);

					//e la parso
					if (false == sub->fromJSon(iter))
						return false;

					//a questo punto mi aspetto una "," oppure una "]"
					gos::string::utf8::toNextValidChar(iter);
					if (iter.getCurChar() == chVirgola)
					{
						iter.advanceOneChar();
						continue;
					}

					if (iter.getCurChar() == chQuadraChiusa)
					{
						//fine dell'array'
						iter.advanceOneChar();
						break;
					}
				}
			}
			else
			{
				//siamo nel caso  "esempio" : [ "pippo", "pluto", 32 ] ovvero un array di valori
				//Lo trasformo in
				//	[esempio] : pippo
				//	[esempio] : pluto
				//	[esempio] : 32
				while (1)
				{
					sprintf_s (s, sizeof(s), "[%s]", arrayName);

					gos::string::utf8::Iter itValue;
					if (iter.getCurChar() == chDoppioApice)
					{
						if (!gos::string::utf8::extractValue (iter, &itValue))
							return false;
						char value[256];
						itValue.copyStrFromCurrentPositionToEnd (value, sizeof(value));
						this->set (s, value, true);
					}
					else
					{
						i32 num = 0;
						if (gos::string::utf8::extractI32 (iter, &num, validIntegerClosingChars, NUM_VALID_INTEGER_CLOSING_CHAR))
							this->set (s, num, true);
						else
						{
							f32 fnum;
							if (gos::string::utf8::extractFloat (iter, &fnum, ".", validIntegerClosingChars, NUM_VALID_INTEGER_CLOSING_CHAR))
								this->set (s, fnum, true);
							else
								return false;						
						}
						
					}

					gos::string::utf8::toNextValidChar(iter);
					if (iter.getCurChar() == chVirgola)
					{
						iter.advanceOneChar();
						gos::string::utf8::toNextValidChar(iter);
						continue;
					}

					if (iter.getCurChar() == chQuadraChiusa)
					{
						iter.advanceOneChar();
						break;
					}

					DBGBREAK;
					return false;

				}
			}

			//a questo punto mi aspetto una "," oppure una "}"
			gos::string::utf8::toNextValidChar(iter);

			if (iter.getCurChar() == chVirgola)
			{
				iter.advanceOneChar();
				continue;
			}

			if (iter.getCurChar() == chGraffaChiusa)
			{
				//fine della sezione corrente
				iter.advanceOneChar();
				return true;
			}

			DBGBREAK;
			return false;
		}		
		else if (iter.getCurChar() == chDoppioApice)
		{
			//dichiarazione di coppia "id":"value"
			gos::string::utf8::Iter itValue;
			if (!gos::string::utf8::extractValue (iter, &itValue))
				return false;

			char identifier[256];
			char value[256];
			it2.copyStrFromCurrentPositionToEnd (identifier, sizeof(identifier));
			itValue.copyStrFromCurrentPositionToEnd (value, sizeof(value));
			string::utf8::unescapeInPlace(value);
			this->set (identifier, value, true);

			//a questo punto mi aspetto una "," oppure una "}" o anche fine del file
			gos::string::utf8::toNextValidChar(iter);

			if (iter.getCurChar().isEOF())
				continue;
			if (iter.getCurChar() == chVirgola)
			{
				iter.advanceOneChar();
				continue;
			}

			if (iter.getCurChar() == chGraffaChiusa)
			{
				//fine della sezione corrente
				iter.advanceOneChar();
				return true;
			}

			return false;
		}
		else
		{
			//a questo punto deve esserci un "numero".
			//Accetto anche evenutali tab e spazi prima del numero
			char identifier[256];
			it2.copyStrFromCurrentPositionToEnd (identifier, sizeof(identifier));

			i32 num = 0;
			if (gos::string::utf8::extractI32 (iter, &num, validIntegerClosingChars, NUM_VALID_INTEGER_CLOSING_CHAR))
				this->set (identifier, num, true);
			else
			{
				f32 fnum;
				if (gos::string::utf8::extractFloat (iter, &fnum, ".", validIntegerClosingChars, NUM_VALID_INTEGER_CLOSING_CHAR))
					this->set (identifier, fnum, true);
				else
				{
					//JSON e' malato e consente di indicare la stringa true o false senza chiuderla tra apici...
					gos::string::utf8::Iter itValue;
					if (!gos::string::utf8::extractIdentifier (iter, &itValue))
						return false;

					char value[256];
					itValue.copyStrFromCurrentPositionToEnd (value, sizeof(value));
					if (strcasecmp(value, "true") == 0 || strcasecmp(value, "false") == 0)
						this->set (identifier, value, true);
					else
						return false;
				}
			}


			//a questo punto mi aspetto una "," oppure una "}"
			gos::string::utf8::toNextValidChar(iter);
			if (iter.getCurChar() == chVirgola)
			{
				iter.advanceOneChar();
				continue;
			}

			if (iter.getCurChar() == chGraffaChiusa)
			{
				//fine della sezione corrente
				iter.advanceOneChar();
				return true;
			}

			return false;
		}
	}

}