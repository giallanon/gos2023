#include <limits.h>
#include "gos.h"
#include "gosFS_FSSpecialPathResolver.h"

using namespace gos;


static gos::fs::SpecialPathResolver pathResolver;


//**************************************************************
bool fs::priv_init()
{
	pathResolver.setup (gos::getSysHeapAllocator());
	return true;
}

//**************************************************************
void fs::priv_deinit()
{
	pathResolver.unsetup();
}

//**************************************************************
bool fs::isPathAbsolute (const char *path)
{
	assert (NULL != path);
#ifdef GOS_PLATFORM__LINUX		
	return (path[0] == '/');
#endif
#ifdef GOS_PLATFORM__WINDOWS
	if (path[0] == 0x00)
		return false;
	return (path[1] == ':');
#endif
}

//**************************************************************
void fs::removeAlias (const char *alias)
{
	pathResolver.removeAlias (alias);
}

//**************************************************************
bool fs::addAlias (const char *alias, const char *realPathNoSlash, eAliasPathMode mode, bool bReplaceAliasIfAlreadyExists)
{
	char s[1024];

	if (bReplaceAliasIfAlreadyExists)
		fs::removeAlias (alias);

	switch (mode)
	{
	default:
		DBGBREAK;
		return false;

	case eAliasPathMode::absolutePath:
		if (!fs::isPathAbsolute(realPathNoSlash))
			return false;

		return pathResolver.addAlias (alias, realPathNoSlash);
		break;

		
	case eAliasPathMode::relativeToAppFolder:
		if (fs::isPathAbsolute(realPathNoSlash))
			return false;
		string::utf8::spf (s, sizeof(s), "%s/%s", gos::getAppPathNoSlash(), realPathNoSlash);
		return pathResolver.addAlias (alias, s);

	case eAliasPathMode::relativeToWritableFolder:
		if (fs::isPathAbsolute(realPathNoSlash))
			return false;
		string::utf8::spf (s, sizeof(s), "%s/%s", gos::getPhysicalPathToWritableFolder(), realPathNoSlash);
		return pathResolver.addAlias (alias, s);
	}
}

//**************************************************************
void fs::pathSanitize (const char *utf8_path, char *out_utf8sanitizedPath, u32 sizeOfOutSanitzed)
{
	if (NULL == utf8_path)
	{
		out_utf8sanitizedPath[0] = 0;
		return;
	}
	if (utf8_path[0] == 0x00)
	{
		out_utf8sanitizedPath[0] = 0;
		return;
	}

    strcpy_s (out_utf8sanitizedPath, sizeOfOutSanitzed, utf8_path);
	fs::pathSanitizeInPlace(out_utf8sanitizedPath);
}

//**************************************************************
static bool fs_isValidHexSymbol (char c)
{
	if (c >= '0' && c <= '9') return true;
	if (c == 'A' || c == 'a') return true;
	if (c == 'B' || c == 'b') return true;
	if (c == 'C' || c == 'c') return true;
	if (c == 'D' || c == 'd') return true;
	if (c == 'E' || c == 'e') return true;
	if (c == 'F' || c == 'f') return true;
	return false;
}

//**************************************************************
void fs::pathSanitizeInPlace (char *utf8_path, u32 nBytesToCheck)
{
	if (u32MAX == nBytesToCheck)
		nBytesToCheck = (u32)strlen((const char*)utf8_path);

	for (u32 i = 0; i < nBytesToCheck; i++)
	{
		if (utf8_path[i] == '\\')
			utf8_path[i] = '/';
	}

	//eventuali %20 li trasforma in blank
	if (nBytesToCheck > 2)
	{
		for (u32 i = 0; i < nBytesToCheck - 2; i++)
		{
			if (utf8_path[i] == '%')
			{
				/*if (utf8_path[i + 1] == '2' && utf8_path[i + 2] == '0')
				{
					utf8_path[i] = ' ';
					memcpy(&utf8_path[i + 1], &utf8_path[i + 3], nBytesToCheck - i - 3);
					nBytesToCheck -= 2;
					utf8_path[nBytesToCheck] = 0;
				}*/
				
				if (fs_isValidHexSymbol(utf8_path[i + 1]) && fs_isValidHexSymbol(utf8_path[i + 2]))
				{
					char hex[4] = { (char)utf8_path[i + 1], (char)utf8_path[i + 2], 0, 0 };
					u32 num = 32;
					gos::string::ansi::hexToInt (hex, &num);
					utf8_path[i] = (char)num;
					memcpy(&utf8_path[i + 1], &utf8_path[i + 3], nBytesToCheck - i - 3);
					nBytesToCheck -= 2;
					utf8_path[nBytesToCheck] = 0;
				}
			}
		}
	}

	u32 i = 0;
	u32 t = 0;
	while (i < nBytesToCheck)
	{
		if (utf8_path[i] == '/')
		{
			utf8_path[t++] = utf8_path[i++];
			while (utf8_path[i] == '/')
				++i;
		}
		else if (utf8_path[i] == '.')
		{
			//se xxx/./yyy
			if (i > 0 && utf8_path[i - 1] == '/' && utf8_path[i + 1] == '/')
				i += 2;
			//se xxx/../yyy
			else if (i > 0 && utf8_path[i - 1] == '/' && utf8_path[i + 1] == '.')
			{
				i += 3;
				if (t >= 2)
					t -= 2;
				while (t && utf8_path[t] != '/')
					--t;
				if (utf8_path[t] == '/')
					++t;
			}
			else
				utf8_path[t++] = utf8_path[i++];
		}
		else
			utf8_path[t++] = utf8_path[i++];
	}
	utf8_path[t] = 0;
	if (t > 1 && utf8_path[t - 1] == '/')
		utf8_path[t - 1] = 0;
}

//******************************************** 
void fs::pathGoBack (const char *pathSenzaSlashIN, char *out, u32 sizeof_out)
{
	assert (NULL != out && sizeof_out > 1);
	out[0] ='/'; 
	out[1] = 0;
	if (NULL == pathSenzaSlashIN || (NULL != pathSenzaSlashIN && pathSenzaSlashIN[0] == 0))
		return;
	if (pathSenzaSlashIN[1] == 0)
	{
		assert (pathSenzaSlashIN[0]=='/');
		return;
	}

	const u32 MAXSIZE = 1024;
	char pathSenzaSlash[MAXSIZE];
	fs::pathSanitize(pathSenzaSlashIN, pathSenzaSlash, sizeof(pathSenzaSlash));

	string::utf8::Iter parser;
	parser.setup (pathSenzaSlash);
	parser.toLast();

	assert(parser.getCurChar() != '/');

	while (parser.getCurChar() != '/')
	{
		if (!parser.backOneChar())
			break;
	}

	if (parser.getCurChar() == '/')
	{
		if (parser.backOneChar())
			parser.copyStrFromXToCurrentPosition (0, out, sizeof_out, true);
	}

}


//**************************************************************************
void fs::extractFileExt (const char *utf8_filename, char *out, u32 sizeof_out)
{
	assert (out && sizeof_out >=3);
	out[0] = 0;

	u32 len = (u32)strlen((const char*)utf8_filename);
	if (len > 0)
	{
		u32 i = len;
		while (i-- > 0)
		{
			if (utf8_filename[i] == '.')
			{
				if (i < len - 1)
				{
					u32 numBytesToCopy = len - i - 1;
					if (numBytesToCopy >= sizeof_out-1)
					{
						DBGBREAK;
						numBytesToCopy = sizeof_out -2;
					}
					memcpy (out, &utf8_filename[i+1], numBytesToCopy);
					out[numBytesToCopy] = 0;
				}
				return;
			}
		}
	}
}

//**************************************************************************
void fs::extractFileNameWithExt (const char *utf8_filename, char *out, u32 sizeof_out)
{
	assert (out && sizeof_out >=3);
	out[0] = 0;

	u32 len = (u32)strlen((const char*)utf8_filename);
	if (len > 0)
	{
		u32 i = len;
		while (i-- > 0)
		{
			if (utf8_filename[i]=='/' || utf8_filename[i]=='\\')
			{
				u32 numBytesToCopy = len - i - 1;
				if (numBytesToCopy >= sizeof_out-1)
				{
					DBGBREAK;
					numBytesToCopy = sizeof_out -2;
				}
				memcpy (out, &utf8_filename[i+1], numBytesToCopy);
				out[numBytesToCopy] = 0;
				return;
			}
		}
		
		u32 numBytesToCopy = len;
		if (numBytesToCopy >= sizeof_out-1)
		{
			DBGBREAK;
			numBytesToCopy = sizeof_out -2;
		}
		memcpy (out, utf8_filename, numBytesToCopy);
		out[numBytesToCopy] = 0;
		return;
	}
}

//**************************************************************************
void fs::extractFileNameWithoutExt (const char *utf8_filename, char *out, u32 sizeof_out)
{
	fs::extractFileNameWithExt (utf8_filename, out, sizeof_out);

	u32 len = (u32)strlen((const char*)out);
	while (len--)
	{
		if (out[len] == '.')
		{
			out[len] = 0;
			return;
		}
	}
}

//**************************************************************************
void fs::extractFilePathWithSlash (const char *utf8_filename, char *out, u32 sizeof_out)
{
	assert (out && sizeof_out >=3);
	out[0] = 0;

	u32 len = (u32)strlen((const char*)utf8_filename);
	while (len-- > 0)
	{
		if (utf8_filename[len]=='/' || utf8_filename[len]=='\\')
		{
			u32 numBytesToCopy = len+1;
			if (numBytesToCopy >= sizeof_out)
			{
				DBGBREAK;
				numBytesToCopy = sizeof_out -1;
			}
			memcpy (out, utf8_filename, numBytesToCopy);
			out[numBytesToCopy] = 0;
			return;
		}
	}
}

//**************************************************************************
void fs::extractFilePathWithOutSlash (const char *utf8_filename, char *out, u32 sizeof_out)
{
	assert (out && sizeof_out >=3);
	out[0] = 0;

	u32 len = (u32)strlen((const char*)utf8_filename);
	while (len-- > 0)
	{
		if (utf8_filename[len]=='/' || utf8_filename[len]=='\\')
		{
			u32 numBytesToCopy = len;
			if (numBytesToCopy >= sizeof_out)
			{
				DBGBREAK;
				numBytesToCopy = sizeof_out -1;
			}
			memcpy (out, utf8_filename, numBytesToCopy);
			out[numBytesToCopy] = 0;
			return;
		}
	}
}

//*********************************************
static bool FS_doesFileNameMatchJolly (const char *utf8_filename, const char *utf8_strJolly)
{
    assert (NULL != utf8_filename && NULL != utf8_strJolly);

	string::utf8::Iter parserFilename;
	parserFilename.setup (utf8_filename);

    string::utf8::Iter parserJolly;
    parserJolly.setup (utf8_strJolly);

    while (1)
    {
        if (parserJolly.getCurChar().isEOF() || parserFilename.getCurChar().isEOF())
        {
            if (parserJolly.getCurChar().isEOF() && parserFilename.getCurChar().isEOF())
                return true;
            return false;
        }

        if (parserJolly.getCurChar() == '?')
        {
            //il char jolly � ?, quindi va bene un char qualunque
            parserFilename.advanceOneChar();
            parserJolly.advanceOneChar();
        }
        else if (parserJolly.getCurChar() == '*')
        {
            //il char jolly � un *, quindi prendo il prox char jolly e lo cerco nel filename
            parserJolly.advanceOneChar();
            if (parserJolly.getCurChar().isEOF())
                return true;

            //cerco il char jolly
            while (1)
            {
                parserFilename.advanceOneChar();
                if (parserFilename.getCurChar().isEOF())
                    return false;
                if (parserFilename.getCurChar() == parserJolly.getCurChar())
                {
                    if (fs::doesFileNameMatchJolly (parserFilename.getPointerToCurrentPosition(), parserJolly.getPointerToCurrentPosition()))
                        return true;
                }
            }
        }
        else
        {
            //il carattere jolly e' un char normale, quindi deve essere uguale al char del filename
			const UTF8Char chJolly = parserJolly.getCurChar();
			const UTF8Char chFile = parserFilename.getCurChar();
			if (gos::string::utf8::isALetter (chJolly))
			{
				//se parliamo di lettere 'az AZ', allora voglio il case insensitive
				char letterJolly = chJolly.data[0];
				char letterFile = chFile.data[0];
				if (letterJolly >='a')
					letterJolly -= 'a';
				if (letterFile >='a')
					letterFile -= 'a';
				if (letterFile != letterJolly)
					return false;
			}
			else
			{
				if (chFile != chJolly)
					return false;
			}
            parserFilename.advanceOneChar();
            parserJolly.advanceOneChar();
        }

    }
    return true;
}

//*********************************************
bool fs::doesFileNameMatchJolly (const char *utf8_filename, const char *utf8_strJolly)
{
	//la stringa dei jolly potrebbe contenere piu' di una sequenza. Le sequenze sono separate da spazio
	//es: *.txt *.bmp
	bool ret = false;

	char jolly[128];
	string::utf8::Iter parserJolly;
	parserJolly.setup (utf8_strJolly);

	u8 ct = 0;
	while (1)
	{
		const UTF8Char ch = parserJolly.getCurChar();
		if (ch.isEOF())
		{
			jolly[ct] = 0x00;
			if (FS_doesFileNameMatchJolly(utf8_filename, jolly))
				ret = true;
			break;
		}
		else if (ch.isEqual(' '))
		{
			jolly[ct] = 0x00;
			if (FS_doesFileNameMatchJolly(utf8_filename, jolly))
				ret = true;
			
			parserJolly.advanceOneChar();
			ct = 0;
		}
		else
		{
			memcpy (&jolly[ct], ch.data, ch.length());
			ct += ch.length();
			parserJolly.advanceOneChar();
		}
	}

	return ret;

}

/**************************************************************************
 * Si aspetta un path gia' risolto tramite FS_PATH_RESOLVER()
 */
static void FS_deleteAllFileInFolderRecursively (const char *pathSenzaSlashRESOLVED, bool bDeleteSubFolder)
{
	if (!fs::folderExists(pathSenzaSlashRESOLVED))
		return;

	gos::FileFind ff;
	if (fs::findFirst(&ff, pathSenzaSlashRESOLVED, "*"))
	{
		do
		{
			char s[512];
			fs::findComposeFullFilePathAndName(ff, pathSenzaSlashRESOLVED, s, sizeof(s));

			if (fs::findIsDirectory(ff))
			{
				const char *fname = fs::findGetFileName(ff);
				if (fname[0] != '.')
				{
					FS_deleteAllFileInFolderRecursively(s, bDeleteSubFolder);
					if (bDeleteSubFolder)
						fs::folderDelete(s);
				}
			}
			else
				fs::fileDelete(s);
		} while (fs::findNext(ff));
		fs::findClose(ff);
	}	

}

//**************************************************************************
bool fs::folderExists (const char *utf8_pathSenzaSlashRESOLVABLE)
{ 
	char pathSenzaSlash[1024];
	pathResolver.resolve (utf8_pathSenzaSlashRESOLVABLE, pathSenzaSlash, sizeof(pathSenzaSlash));
	return platform::FS_folderExists(pathSenzaSlash); 
}

//**************************************************************************
bool fs::folderDelete (const char *utf8_pathSenzaSlashRESOLVABLE)
{ 
	char pathSenzaSlash[1024];
	pathResolver.resolve (utf8_pathSenzaSlashRESOLVABLE, pathSenzaSlash, sizeof(pathSenzaSlash));
	return platform::FS_folderDelete(pathSenzaSlash); 
}

//**************************************************************************
bool fs::folderCreate (const char *utf8_pathSenzaSlashRESOLVABLE)
{ 
	char pathSenzaSlash[1024];
	pathResolver.resolve (utf8_pathSenzaSlashRESOLVABLE, pathSenzaSlash, sizeof(pathSenzaSlash));

	return platform::FS_folderCreate(pathSenzaSlash); 
}


//**************************************************************************
bool fs::folderDeleteAllFileRecursively(const char *utf8_pathSenzaSlashRESOLVABLE, eFolderDeleteMode folderDeleteMode)
{
	char pathSenzaSlash[1024];
	pathResolver.resolve (utf8_pathSenzaSlashRESOLVABLE, pathSenzaSlash, sizeof(pathSenzaSlash));

	if (!fs::folderExists(pathSenzaSlash))
		return false;

	if (folderDeleteMode == eFolderDeleteMode::doNotDeleteAnyFolder)
		FS_deleteAllFileInFolderRecursively (pathSenzaSlash, false);
	else
	{
		//se arriviamo qui, vuol dire che di sicuro voglio cancellare tutte le subfolder
		FS_deleteAllFileInFolderRecursively (pathSenzaSlash, true);

		//eventualmente cancello anche la main folder
		if (folderDeleteMode == eFolderDeleteMode::deleteAlsoTheSubfolderAndTheMainFolder)
			folderDelete(pathSenzaSlash);
	}

	return true;
}

//**************************************************************************
void fs::folderDeleteAllFileWithJolly  (const char *utf8_pathSenzaSlashRESOLVABLE, const char *utf8_jolly)
{
	char pathSenzaSlash[1024];
	pathResolver.resolve (utf8_pathSenzaSlashRESOLVABLE, pathSenzaSlash, sizeof(pathSenzaSlash));

	if (!fs::folderExists(pathSenzaSlash))
		return;

	gos::FileFind ff;
	if (fs::findFirst(&ff, pathSenzaSlash, utf8_jolly))
	{
		do
		{
			if (fs::findIsDirectory(ff))
				continue;

			char s[512];
			fs::findComposeFullFilePathAndName(ff, pathSenzaSlash, s, sizeof(s));
			fs::fileDelete(s);
				
		} while (fs::findNext(ff));
		fs::findClose(ff);
	}	
}

//******************************************
void fs::makeABSPathFromABSFilename (const char *origin_absFilename, const char *rel_or_abs_path, char *out, u32 sizeof_out)
{
    assert (fs::isPathAbsolute(origin_absFilename));

    //<rel_or_abs_path> puo' essere assoluto o relativo
    char s[1024];
    if (fs::isPathAbsolute(rel_or_abs_path))
        sprintf_s (s, sizeof(s), "%s", rel_or_abs_path);
    else
    {
		if (rel_or_abs_path[0] == '@')
			fs::resolvePath (rel_or_abs_path, s, sizeof(s));
		else
		{
			fs::extractFilePathWithSlash (origin_absFilename, s, sizeof(s));
			strcat_s (s, sizeof(s), rel_or_abs_path);
		}
    }    

    fs::pathSanitizeInPlace(s);
    const u32 len = string::ansi::lengthInBytes(s);

    assert (len < sizeof_out);
    memset (out, 0, sizeof_out);
    memcpy (out, s, len);
}

//**************************************************************************
void fs::resolvePath (const char *pathIN, char *out, u32 sizeof_out)
{
	pathResolver.resolve (pathIN, out, sizeof_out);
}	

/**************************************************************************
 * Questa suppone che [utf8_filePathAndName] sia gia' stato risolto da qualcuno altro tramite 
 * FS_PATH_RESOLVER()
 */
bool FS_fileOpenRESOLVED  (gos::File *out_h, const char *utf8_filePathAndName, eFileMode mode, bool bCreateIfNotExists, bool bAppend, bool bShareRead, bool bShareWrite)
{
	return platform::FS_fileOpen (&out_h->osFile, utf8_filePathAndName, mode, bCreateIfNotExists, bAppend, bShareRead, bShareWrite); 
}

//**************************************************************************
bool fs::fileOpen  (gos::File *out_h, const char *utf8_filePathAndNameRESOLVABLE, eFileMode mode, bool bCreateIfNotExists, bool bAppend, bool bShareRead, bool bShareWrite)
{
	char utf8_filePathAndName[1024];
	pathResolver.resolve (utf8_filePathAndNameRESOLVABLE, utf8_filePathAndName, sizeof(utf8_filePathAndName));
	return FS_fileOpenRESOLVED (out_h, utf8_filePathAndName, mode, bCreateIfNotExists, bAppend, bShareRead, bShareWrite); 
}

//**************************************************************************
bool fs::fileOpenForW (gos::File *out_h, const char *utf8_filePathAndNameRESOLVABLE, bool bAutoCreateFolders)
{
	char utf8_filePathAndName[1024];
	pathResolver.resolve (utf8_filePathAndNameRESOLVABLE, utf8_filePathAndName, sizeof(utf8_filePathAndName));

	if (bAutoCreateFolders)
	{
		char path[2048];
		fs::extractFilePathWithOutSlash (utf8_filePathAndName, path, sizeof(path));
		fs::folderCreate(path);
	}	
	return FS_fileOpenRESOLVED (out_h, utf8_filePathAndName, eFileMode::writeOnly, true, false, true, true); 
}

//**************************************************************************
bool fs::fileOpenForAppend (gos::File *out_h, const char *utf8_filePathAndNameRESOLVABLE, bool bAutoCreateFolders)
{
	char utf8_filePathAndName[1024];
	pathResolver.resolve (utf8_filePathAndNameRESOLVABLE, utf8_filePathAndName, sizeof(utf8_filePathAndName));
	if (bAutoCreateFolders)
	{
		char path[2048];
		fs::extractFilePathWithOutSlash (utf8_filePathAndName, path, sizeof(path));
		fs::folderCreate(path);
	}	
	return FS_fileOpenRESOLVED (out_h, utf8_filePathAndName, eFileMode::writeOnly, true, true, true, true); 
}

//**************************************************************************
u32 fs::fileWriteU16 (gos::File &h, u16 val)
{
	u8 buffer[2];
	buffer[0] = (u8)((val & 0xFF00) >> 8);
	buffer[1] = (u8)(val & 0x00FF);
	return fileWrite (h, buffer, 2);
}

//**************************************************************************
u32 fs::fileWriteU32 (gos::File &h, u32 val)
{
	u8 buffer[4];
	buffer[0] = (u8)((val & 0xFF000000) >> 24);
	buffer[1] = (u8)((val & 0x00FF0000) >> 16);
	buffer[2] = (u8)((val & 0x0000FF00) >> 8);
	buffer[3] = (u8)(val & 0x000000FF);
	return fileWrite (h, buffer, 4);
}

//**************************************************************************
u32 fs::fileWriteU64 (gos::File &h, u64 val)
{
	u8 buffer[8];
	buffer[0] = (u8)((val & 0xFF00000000000000) >> 56); 
	buffer[1] = (u8)((val & 0x00FF000000000000) >> 48); 
	buffer[2] = (u8)((val & 0x0000FF0000000000) >> 40); 
	buffer[3] = (u8)((val & 0x000000FF00000000) >> 32);  
	buffer[4] = (u8)((val & 0x00000000FF000000) >> 24); 
	buffer[5] = (u8)((val & 0x0000000000FF0000) >> 16); 
	buffer[6] = (u8)((val & 0x000000000000FF00) >> 8); 
	buffer[7] = (u8) (val & 0x00000000000000FF); 
	return fileWrite (h, buffer, 8);
}

//**************************************************************************
u32 fs::fileReadU16 (gos::File &h, u16 *out)
{
	u8 buffer[2];
	const u32 ret = fileRead (h, buffer, 2);
	*out = (((u16)buffer[0]) << 8) | ((u16)buffer[1]); 
	return ret;
}

//**************************************************************************
u32 fs::fileReadU32 (gos::File &h, u32 *out)
{
	u8 buffer[4];
	const u32 ret = fileRead (h, buffer, 4);
	*out = (((u32)buffer[0]) << 24) | (((u32)buffer[1]) << 16) | (((u32)buffer[2]) << 8) | ((u32)buffer[3]);
	return ret;
}

//**************************************************************************
u32 fs::fileReadU64 (gos::File &h, u64 *out)
{
	u8 buffer[8];
	const u32 ret = fileRead (h, buffer, 8);
	*out = (((u64)buffer[0]) << 56) | 
           (((u64)buffer[1]) << 48) | 
           (((u64)buffer[2]) << 40) | 
           (((u64)buffer[3]) << 32) | 
           (((u64)buffer[4]) << 24) | 
           (((u64)buffer[5]) << 16) | 
           (((u64)buffer[6]) << 8) | 
           ((u64)buffer[7]); 
	return ret;
}

//**************************************************************************
void fs::fpf_valist (gos::File &h, const char *format, va_list argptr)
{
	static char buffer[2048];
	const int n = vsnprintf (buffer, sizeof(buffer), format, argptr);
	fs::fileWrite (h, buffer, n);
}

void fs::fpf (gos::File &h, const char *format, ...)
{
	va_list argptr;
	va_start (argptr, format );
	fs::fpf_valist (h, format, argptr);
	va_end(argptr);
}

//**************************************************************************
u8* fs::fileLoadInMemory (Allocator *allocator, const char* utf8_filePathAndNameRESOLVABLE, u32 *out_fileSize)
{
	char utf8_filePathAndName[1024];
	pathResolver.resolve (utf8_filePathAndNameRESOLVABLE, utf8_filePathAndName, sizeof(utf8_filePathAndName));


	gos::File hFile;
	if (!fs::fileOpenForR (&hFile, utf8_filePathAndName))
		return NULL;
	u32 fLen = (u32)fs::fileLength(hFile);

	u8 *buffer = (u8*)GOSALLOC(allocator, fLen);
	fs::fileRead (hFile, buffer, fLen);
	fs::fileClose (hFile);

	if (NULL != out_fileSize)
		*out_fileSize = fLen;
	return buffer;
}

//**************************************************************************
bool fs::fileSaveBuffer (const char* utf8_filePathAndNameRESOLVABLE, const void *buffer, u32 sizeof_buffer)
{
	gos::File f;
	if (!fs::fileOpenForW (&f, utf8_filePathAndNameRESOLVABLE))
		return false;

	fs::fileWrite (f, buffer, sizeof_buffer);
	fs::fileClose (f);
	return true;
}

//**************************************************************************
u64 fs::fileLength (const char *utf8_filePathAndNameRESOLVABLE)
{ 
	char utf8_filePathAndName[1024];
	pathResolver.resolve (utf8_filePathAndNameRESOLVABLE, utf8_filePathAndName, sizeof(utf8_filePathAndName));

	return platform::FS_fileLength(utf8_filePathAndName);
}
//**************************************************************************
bool fs::findFirst (gos::FileFind *ff, const char *utf8_pathRESOLVABLE, const char *jolly, eFileFindMode ffmode)
{ 
	char utf8_path[1024];
	pathResolver.resolve (utf8_pathRESOLVABLE, utf8_path, sizeof(utf8_path));
	return platform::FS_findFirst (&ff->osFF, utf8_path, jolly, ffmode); 
}

//**************************************************************************
void fs::findComposeFullFilePathAndName (const gos::FileFind &ff, const char *pathNoSlash, char *out, u32 sizeofOut)
{
	gos::string::utf8::spf (out, sizeofOut, "%s/", pathNoSlash);
	const u32 n = string::utf8::lengthInByte(out);
	fs::findGetFileName (ff, &out[n], sizeofOut - n);
}

//**************************************************************************
void fs::findGetLastTimeModified_UTC (const gos::FileFind &ff, const char *pathNoSlash, gos::DateTime *out)
{
	char s[1024];
	fs::findComposeFullFilePathAndName (ff, pathNoSlash, s, sizeof(s));

	gos::DateTime dt;
	fs::fileGetLastTimeModified_UTC(s, &dt);
}

//**************************************************************************
u64 fs::findGetLastTimeModified_UTC_niceu64 (const gos::FileFind &ff, const char *pathNoSlash)
{
	char s[1024];
	fs::findComposeFullFilePathAndName (ff, pathNoSlash, s, sizeof(s));

	gos::DateTime dt;
	fs::fileGetLastTimeModified_UTC(s, &dt);
	return dt.getAsNiceU64();
}

//**************************************************************************
bool fs::fileExists(const char *utf8_filePathAndNameRESOLVABLE)
{ 
	char utf8_filePathAndName[1024];
	pathResolver.resolve (utf8_filePathAndNameRESOLVABLE, utf8_filePathAndName, sizeof(utf8_filePathAndName));

	return platform::FS_fileExists (utf8_filePathAndName); 
}

//**************************************************************************
bool fs::fileDelete(const char *utf8_filePathAndNameRESOLVABLE)
{ 
	char utf8_filePathAndName[1024];
	pathResolver.resolve (utf8_filePathAndNameRESOLVABLE, utf8_filePathAndName, sizeof(utf8_filePathAndName));
	return platform::FS_fileDelete (utf8_filePathAndName); 
}

//**************************************************************************
bool fs::fileRename(const char *utf8_pathNoSlashRESOLVABLE, const char *utf8_oldFilename, const char *utf8_newFilename)
{ 
	char utf8_pathNoSlash[1024];
	pathResolver.resolve (utf8_pathNoSlashRESOLVABLE, utf8_pathNoSlash, sizeof(utf8_pathNoSlash));
	return platform::FS_fileRename (utf8_pathNoSlash, utf8_oldFilename, utf8_newFilename); 
}

//**************************************************************************
void fs::fileGetCreationTime_UTC(const char *utf8_filePathAndNameRESOLVABLE, gos::DateTime *out_dt)
{ 
	char utf8_filePathAndName[1024];
	pathResolver.resolve (utf8_filePathAndNameRESOLVABLE, utf8_filePathAndName, sizeof(utf8_filePathAndName));
	platform::FS_fileGetCreationTime_UTC(utf8_filePathAndName, out_dt); 
}
	
//**************************************************************************
void fs::fileGetLastTimeModified_UTC(const char *utf8_filePathAndNameRESOLVABLE, gos::DateTime *out_dt)
{ 
	char utf8_filePathAndName[1024];
	pathResolver.resolve (utf8_filePathAndNameRESOLVABLE, utf8_filePathAndName, sizeof(utf8_filePathAndName));
	platform::FS_fileGetLastTimeModified_UTC (utf8_filePathAndName, out_dt); 
}

//**************************************************************************
u64 fs::fileGetLastTimeModified_UTC_niceu64 (const char *utf8_filePathAndNameRESOLVABLE)
{ 
	gos::DateTime dt;
	fileGetLastTimeModified_UTC (utf8_filePathAndNameRESOLVABLE, &dt);
	return dt.getAsNiceU64();
}

//**************************************************************************
void fs::fileGetCreationTime_LocalTime (const char *utf8_filePathAndNameRESOLVABLE, gos::DateTime *out_dt)
{ 
	char utf8_filePathAndName[1024];
	pathResolver.resolve (utf8_filePathAndNameRESOLVABLE, utf8_filePathAndName, sizeof(utf8_filePathAndName));
	platform::FS_fileGetCreationTime_LocalTime (utf8_filePathAndName, out_dt); 
}

//**************************************************************************
void fs::fileGetLastTimeModified_LocalTime (const char *utf8_filePathAndNameRESOLVABLE, gos::DateTime *out_dt)
{ 
	char utf8_filePathAndName[1024];
	pathResolver.resolve (utf8_filePathAndNameRESOLVABLE, utf8_filePathAndName, sizeof(utf8_filePathAndName));
	platform::FS_fileGetLastTimeModified_LocalTime (utf8_filePathAndName, out_dt); 
}

//**************************************************************************
bool fs::fileCopy (const char *src, const char *dst)
{
	char resolved_src[1024];
	char resolved_dst[1024];
	pathResolver.resolve (src, resolved_src, sizeof(resolved_src));
	pathResolver.resolve (dst, resolved_dst, sizeof(resolved_dst));
	return platform::FS_fileCopy (resolved_src, resolved_dst);
}


