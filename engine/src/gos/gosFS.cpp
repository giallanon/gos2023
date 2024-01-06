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
bool FS_isPathAbsolute (const u8 *path)
{
#ifdef GOS_PLATFORM__LINUX		
	return (path[0] == '/');
#endif
#ifdef GOS_PLATFORM__WINDOWS
	return (path[1] == ':');
#endif
}

//**************************************************************
bool fs::addAlias (const char *alias, const char *realPathNoSlash, eAliasPathMode mode)
{
	return fs::addAlias (alias, reinterpret_cast<const u8*>(realPathNoSlash), mode);
}
bool fs::addAlias (const char *alias, const u8 *realPathNoSlash, eAliasPathMode mode)
{
	u8 s[1024];

	switch (mode)
	{
	default:
		DBGBREAK;
		return false;

	case eAliasPathMode::absolutePath:
		if (!FS_isPathAbsolute(realPathNoSlash))
			return false;

		return pathResolver.addAlias (alias, realPathNoSlash);
		break;

		
	case eAliasPathMode::relativeToAppFolder:
		if (FS_isPathAbsolute(realPathNoSlash))
			return false;
		string::utf8::spf (s, sizeof(s), "%s/%s", gos::getAppPathNoSlash(), realPathNoSlash);
		return pathResolver.addAlias (alias, s);

	case eAliasPathMode::relativeToWritableFolder:
		if (FS_isPathAbsolute(realPathNoSlash))
			return false;
		string::utf8::spf (s, sizeof(s), "%s/%s", gos::getPhysicalPathToWritableFolder(), realPathNoSlash);
		return pathResolver.addAlias (alias, s);
	}
}

//**************************************************************
void fs::pathSanitize (const u8 *utf8_path, u8 *out_utf8sanitizedPath, u32 sizeOfOutSanitzed)
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

    strcpy_s ((char*)out_utf8sanitizedPath, sizeOfOutSanitzed, (const char*)utf8_path);
	fs::pathSanitizeInPlace(out_utf8sanitizedPath);
}

//**************************************************************
static bool fs_isValidHexSymbol (u8 c)
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
void fs::pathSanitizeInPlace (u8 *utf8_path, u32 nBytesToCheck)
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
					utf8_path[i] = (u8)num;
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
void fs::pathGoBack (const u8 *pathSenzaSlashIN, u8 *out, u32 sizeofout)
{
	assert (NULL != out && sizeofout > 1);
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
	u8 pathSenzaSlash[MAXSIZE];
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
			parser.copyStrFromXToCurrentPosition (0, out, sizeofout, true);
	}

}


//**************************************************************************
void fs::extractFileExt (const u8 *utf8_filename, u8 *out, u32 sizeofout)
{
	assert (out && sizeofout >=3);
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
					if (numBytesToCopy >= sizeofout-1)
					{
						DBGBREAK;
						numBytesToCopy = sizeofout -2;
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
void fs::extractFileNameWithExt (const u8 *utf8_filename, u8 *out, u32 sizeofout)
{
	assert (out && sizeofout >=3);
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
				if (numBytesToCopy >= sizeofout-1)
				{
					DBGBREAK;
					numBytesToCopy = sizeofout -2;
				}
				memcpy (out, &utf8_filename[i+1], numBytesToCopy);
				out[numBytesToCopy] = 0;
				return;
			}
		}
		
		u32 numBytesToCopy = len;
		if (numBytesToCopy >= sizeofout-1)
		{
			DBGBREAK;
			numBytesToCopy = sizeofout -2;
		}
		memcpy (out, utf8_filename, numBytesToCopy);
		out[numBytesToCopy] = 0;
		return;
	}
}

//**************************************************************************
void fs::extractFileNameWithoutExt (const u8 *utf8_filename, u8 *out, u32 sizeofout)
{
	fs::extractFileNameWithExt (utf8_filename, out, sizeofout);

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
void fs::extractFilePathWithSlash (const u8 *utf8_filename, u8 *out, u32 sizeofout)
{
	assert (out && sizeofout >=3);
	out[0] = 0;

	u32 len = (u32)strlen((const char*)utf8_filename);
	while (len-- > 0)
	{
		if (utf8_filename[len]=='/' || utf8_filename[len]=='\\')
		{
			u32 numBytesToCopy = len+1;
			if (numBytesToCopy >= sizeofout)
			{
				DBGBREAK;
				numBytesToCopy = sizeofout -1;
			}
			memcpy (out, utf8_filename, numBytesToCopy);
			out[numBytesToCopy] = 0;
			return;
		}
	}
}

//**************************************************************************
void fs::extractFilePathWithOutSlash (const u8 *utf8_filename, u8 *out, u32 sizeofout)
{
	assert (out && sizeofout >=3);
	out[0] = 0;

	u32 len = (u32)strlen((const char*)utf8_filename);
	while (len-- > 0)
	{
		if (utf8_filename[len]=='/' || utf8_filename[len]=='\\')
		{
			u32 numBytesToCopy = len;
			if (numBytesToCopy >= sizeofout)
			{
				DBGBREAK;
				numBytesToCopy = sizeofout -1;
			}
			memcpy (out, utf8_filename, numBytesToCopy);
			out[numBytesToCopy] = 0;
			return;
		}
	}
}

//*********************************************
static bool FS_doesFileNameMatchJolly (const u8 *utf8_filename, const u8 *utf8_strJolly)
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
bool fs::doesFileNameMatchJolly (const u8 *utf8_filename, const u8 *utf8_strJolly)
{
	//la stringa dei jolly potrebbe contenere piu' di una sequenza. Le sequenze sono separate da spazio
	//es: *.txt *.bmp
	bool ret = false;

	u8 jolly[128];
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
static void FS_deleteAllFileInFolderRecursively (const u8 *pathSenzaSlashRESOLVED, bool bDeleteSubFolder)
{
	if (!fs::folderExists(pathSenzaSlashRESOLVED))
		return;

	gos::FileFind ff;
	if (fs::findFirst(&ff, pathSenzaSlashRESOLVED, (const u8 *)"*"))
	{
		do
		{
			u8 s[512];
			fs::findComposeFullFilePathAndName(ff, pathSenzaSlashRESOLVED, s, sizeof(s));

			if (fs::findIsDirectory(ff))
			{
				const u8 *fname = fs::findGetFileName(ff);
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
bool fs::folderExists (const u8 *utf8_pathSenzaSlashRESOLVABLE)
{ 
	u8 pathSenzaSlash[1024];
	pathResolver.resolve (utf8_pathSenzaSlashRESOLVABLE, pathSenzaSlash, sizeof(pathSenzaSlash));
	return platform::FS_folderExists(pathSenzaSlash); 
}

//**************************************************************************
bool fs::folderDelete (const u8 *utf8_pathSenzaSlashRESOLVABLE)
{ 
	u8 pathSenzaSlash[1024];
	pathResolver.resolve (utf8_pathSenzaSlashRESOLVABLE, pathSenzaSlash, sizeof(pathSenzaSlash));
	return platform::FS_folderDelete(pathSenzaSlash); 
}

//**************************************************************************
bool fs::folderCreate (const u8 *utf8_pathSenzaSlashRESOLVABLE)
{ 
	u8 pathSenzaSlash[1024];
	pathResolver.resolve (utf8_pathSenzaSlashRESOLVABLE, pathSenzaSlash, sizeof(pathSenzaSlash));

	return platform::FS_folderCreate(pathSenzaSlash); 
}


//**************************************************************************
bool fs::folderDeleteAllFileRecursively(const u8 *utf8_pathSenzaSlashRESOLVABLE, eFolderDeleteMode folderDeleteMode)
{
	u8 pathSenzaSlash[1024];
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
void fs::folderDeleteAllFileWithJolly  (const u8 *utf8_pathSenzaSlashRESOLVABLE, const u8 *utf8_jolly)
{
	u8 pathSenzaSlash[1024];
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

			u8 s[512];
			fs::findComposeFullFilePathAndName(ff, pathSenzaSlash, s, sizeof(s));
			fs::fileDelete(s);
				
		} while (fs::findNext(ff));
		fs::findClose(ff);
	}	
}



/**************************************************************************
 * Questa suppone che [utf8_filePathAndName] sia gia' stato risolto da qualcuno altro tramite 
 * FS_PATH_RESOLVER()
 */
bool FS_fileOpenRESOLVED  (gos::File *out_h, const u8 *utf8_filePathAndName, eFileMode mode, bool bCreateIfNotExists, bool bAppend, bool bShareRead, bool bShareWrite)
{
	return platform::FS_fileOpen (&out_h->osFile, utf8_filePathAndName, mode, bCreateIfNotExists, bAppend, bShareRead, bShareWrite); 
}

//**************************************************************************
bool fs::fileOpen  (gos::File *out_h, const u8 *utf8_filePathAndNameRESOLVABLE, eFileMode mode, bool bCreateIfNotExists, bool bAppend, bool bShareRead, bool bShareWrite)
{
	u8 utf8_filePathAndName[1024];
	pathResolver.resolve (utf8_filePathAndNameRESOLVABLE, utf8_filePathAndName, sizeof(utf8_filePathAndName));
	return FS_fileOpenRESOLVED (out_h, utf8_filePathAndName, mode, bCreateIfNotExists, bAppend, bShareRead, bShareWrite); 
}

//**************************************************************************
bool fs::fileOpenForW (gos::File *out_h, const u8 *utf8_filePathAndNameRESOLVABLE, bool bAutoCreateFolders)
{
	u8 utf8_filePathAndName[1024];
	pathResolver.resolve (utf8_filePathAndNameRESOLVABLE, utf8_filePathAndName, sizeof(utf8_filePathAndName));

	if (bAutoCreateFolders)
	{
		u8 path[2048];
		fs::extractFilePathWithOutSlash (utf8_filePathAndName, path, sizeof(path));
		fs::folderCreate(path);
	}	
	return FS_fileOpenRESOLVED (out_h, utf8_filePathAndName, eFileMode::writeOnly, true, false, true, true); 
}

//**************************************************************************
bool fs::fileOpenForAppend (gos::File *out_h, const u8 *utf8_filePathAndNameRESOLVABLE, bool bAutoCreateFolders)
{
	u8 utf8_filePathAndName[1024];
	pathResolver.resolve (utf8_filePathAndNameRESOLVABLE, utf8_filePathAndName, sizeof(utf8_filePathAndName));
	if (bAutoCreateFolders)
	{
		u8 path[2048];
		fs::extractFilePathWithOutSlash (utf8_filePathAndName, path, sizeof(path));
		fs::folderCreate(path);
	}	
	return FS_fileOpenRESOLVED (out_h, utf8_filePathAndName, eFileMode::writeOnly, true, true, true, true); 
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
u8* fs::fileLoadInMemory (Allocator *allocator, const u8* utf8_filePathAndNameRESOLVABLE, u32 *out_fileSize)
{
	u8 utf8_filePathAndName[1024];
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
u64 fs::fileLength (const u8 *utf8_filePathAndNameRESOLVABLE)
{ 
	u8 utf8_filePathAndName[1024];
	pathResolver.resolve (utf8_filePathAndNameRESOLVABLE, utf8_filePathAndName, sizeof(utf8_filePathAndName));

	return platform::FS_fileLength(utf8_filePathAndName);
}

//**************************************************************************
u64 fs::fileLength (const char *utf8_filePathAndNameRESOLVABLE)
{ 
	u8 utf8_filePathAndName[1024];
	pathResolver.resolve (utf8_filePathAndNameRESOLVABLE, utf8_filePathAndName, sizeof(utf8_filePathAndName));

	return platform::FS_fileLength(utf8_filePathAndName);
}

//**************************************************************************
bool fs::findFirst (gos::FileFind *ff, const u8 *utf8_pathRESOLVABLE, const u8 *utf8_jolly)
{ 
	u8 utf8_path[1024];
	pathResolver.resolve (utf8_pathRESOLVABLE, utf8_path, sizeof(utf8_path));

	return platform::FS_findFirst (&ff->osFF, utf8_path, utf8_jolly); 
}

//**************************************************************************
bool fs::findFirst (gos::FileFind *ff, const u8 *utf8_pathRESOLVABLE, const char *jolly)
{ 
	u8 utf8_path[1024];
	pathResolver.resolve (utf8_pathRESOLVABLE, utf8_path, sizeof(utf8_path));

	return platform::FS_findFirst (&ff->osFF, utf8_path, reinterpret_cast<const u8*>(jolly)); 
}

//**************************************************************************
void fs::findComposeFullFilePathAndName (const gos::FileFind &ff, const u8 *pathNoSlash, u8 *out, u32 sizeofOut)
{
	gos::string::utf8::spf (out, sizeofOut, "%s/", pathNoSlash);
	const u32 n = string::utf8::lengthInByte(out);
	fs::findGetFileName (ff, &out[n], sizeofOut - n);
}


//**************************************************************************
bool fs::fileExists(const u8 *utf8_filePathAndNameRESOLVABLE)
{ 
	u8 utf8_filePathAndName[1024];
	pathResolver.resolve (utf8_filePathAndNameRESOLVABLE, utf8_filePathAndName, sizeof(utf8_filePathAndName));

	return platform::FS_fileExists (utf8_filePathAndName); 
}

//**************************************************************************
bool fs::fileDelete(const u8 *utf8_filePathAndNameRESOLVABLE)
{ 
	u8 utf8_filePathAndName[1024];
	pathResolver.resolve (utf8_filePathAndNameRESOLVABLE, utf8_filePathAndName, sizeof(utf8_filePathAndName));
	return platform::FS_fileDelete (utf8_filePathAndName); 
}

//**************************************************************************
bool fs::fileRename(const u8 *utf8_pathNoSlashRESOLVABLE, const u8 *utf8_oldFilename, const u8 *utf8_newFilename)
{ 
	u8 utf8_pathNoSlash[1024];
	pathResolver.resolve (utf8_pathNoSlashRESOLVABLE, utf8_pathNoSlash, sizeof(utf8_pathNoSlash));
	return platform::FS_fileRename (utf8_pathNoSlash, utf8_oldFilename, utf8_newFilename); 
}

//**************************************************************************
void fs::fileGetCreationTime_UTC(const u8 *utf8_filePathAndNameRESOLVABLE, gos::DateTime *out_dt)
{ 
	u8 utf8_filePathAndName[1024];
	pathResolver.resolve (utf8_filePathAndNameRESOLVABLE, utf8_filePathAndName, sizeof(utf8_filePathAndName));
	platform::FS_fileGetCreationTime_UTC(utf8_filePathAndName, out_dt); 
}
	
//**************************************************************************
void fs::fileGetLastTimeModified_UTC(const u8 *utf8_filePathAndNameRESOLVABLE, gos::DateTime *out_dt)
{ 
	u8 utf8_filePathAndName[1024];
	pathResolver.resolve (utf8_filePathAndNameRESOLVABLE, utf8_filePathAndName, sizeof(utf8_filePathAndName));
	platform::FS_fileGetLastTimeModified_UTC (utf8_filePathAndName, out_dt); 
}

//**************************************************************************
void fs::fileGetCreationTime_LocalTime (const u8 *utf8_filePathAndNameRESOLVABLE, gos::DateTime *out_dt)
{ 
	u8 utf8_filePathAndName[1024];
	pathResolver.resolve (utf8_filePathAndNameRESOLVABLE, utf8_filePathAndName, sizeof(utf8_filePathAndName));
	platform::FS_fileGetCreationTime_LocalTime (utf8_filePathAndName, out_dt); 
}

//**************************************************************************
void fs::fileGetLastTimeModified_LocalTime (const u8 *utf8_filePathAndNameRESOLVABLE, gos::DateTime *out_dt)
{ 
	u8 utf8_filePathAndName[1024];
	pathResolver.resolve (utf8_filePathAndNameRESOLVABLE, utf8_filePathAndName, sizeof(utf8_filePathAndName));
	platform::FS_fileGetLastTimeModified_LocalTime (utf8_filePathAndName, out_dt); 
}

