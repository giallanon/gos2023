#ifdef GOS_PLATFORM__WINDOWS
#include "winOS.h"
#include <mbstring.h>
#include <string.h>
#include <shlobj.h>
#include <strsafe.h>
#include "../../gos.h"
#include "../../gosString.h"
#include "../../dataTypes/gosDateTime.h"

using namespace gos;

//********************************************* 
bool win32_createFolderFromUTF8Path (const char *utf8_path, u32 nBytesToUseForPath)
{
	wchar_t temp[512];
	if (!win32::utf8_towchar(utf8_path, nBytesToUseForPath, temp, sizeof(temp)))
		return false;

	BOOL ret = ::CreateDirectory (temp, NULL);
	if (ret == 1)
		return true;
	if (GetLastError() == ERROR_ALREADY_EXISTS)
		return true;
	return false;
}

//*****************************************************
bool platform::FS_folderCreate (const char *utf8_path)
{
	if (NULL == utf8_path)
		return false;
	if (utf8_path[0] == 0x00)
		return false;

	if (utf8_path[1] != ':')
	{
		DBGBREAK;
		return false;
	}
	if (utf8_path[2] == 0x00)
		return true;

	u32 n = 3;
	while (utf8_path[n] != 0x00)
	{
		if (utf8_path[n]=='\\' || utf8_path[n]=='/')
		{
			if (!win32_createFolderFromUTF8Path(utf8_path,n))
				return false;
		}
		n++;
	}

	return win32_createFolderFromUTF8Path(utf8_path, n);
}

//*****************************************************
bool platform::FS_folderDelete(const char *utf8_path)
{
	wchar_t temp[512];
	if (win32::utf8_towchar(utf8_path, u32MAX, temp, sizeof(temp)))
		return (::RemoveDirectory(temp) != 0); 
	return false;
}

//*****************************************************
bool platform::FS_folderExists(const char *utf8_path)
{
	wchar_t temp[512];
	if (!win32::utf8_towchar(utf8_path, u32MAX, temp, sizeof(temp)))
		return false;

	DWORD ftyp = GetFileAttributes(temp);
	if (ftyp == INVALID_FILE_ATTRIBUTES)
		return false;
	if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
		return true;
	return false;
}

//*****************************************************
bool platform::FS_fileExists (const char *utf8_filename)
{
	wchar_t temp[512];
	if (!win32::utf8_towchar(utf8_filename, u32MAX, temp, sizeof(temp)))
		return false;

	assert (sizeof(wchar_t) == sizeof(u16));
	DWORD ftyp = GetFileAttributes(temp);
	if (ftyp == INVALID_FILE_ATTRIBUTES)
		return false;
	if ((ftyp & FILE_ATTRIBUTE_DIRECTORY) == 0)
		return true;
	return false;
}

//*****************************************************
bool platform::FS_fileDelete (const char *utf8_filename)
{
	wchar_t temp[512];
	if (!win32::utf8_towchar(utf8_filename, u32MAX, temp, sizeof(temp)))
		return false;
	if (::DeleteFile (temp))
		return true;

	const DWORD err = GetLastError();
	if (ERROR_FILE_NOT_FOUND == err)
		return true;

	return false;
}

//*****************************************************
bool platform::FS_fileRename(const char *utf8_pathNoSlash, const char *utf8_oldFilename, const char *utf8_newFilename)
{
	char utf8_temp[512];

	wchar_t temp1[512];
	gos::string::utf8::spf (utf8_temp, sizeof(utf8_temp), "%s/%s", utf8_pathNoSlash, utf8_oldFilename);
	gos::fs::pathSanitizeInPlace(utf8_temp);
	if (!win32::utf8_towchar(utf8_temp, u32MAX, temp1, sizeof(temp1)))
		return false;

	wchar_t temp2[512];
	gos::string::utf8::spf (utf8_temp, sizeof(utf8_temp), "%s/%s", utf8_pathNoSlash, utf8_newFilename);
	gos::fs::pathSanitizeInPlace(utf8_temp);
	if (!win32::utf8_towchar(utf8_temp, u32MAX, temp2, sizeof(temp2)))
		return false;

	return (::MoveFileEx (temp1, temp2, 0) != 0);
}

//*****************************************************
void platform::FS_fileGetCreationTime_UTC (const char *utf8_filePathAndName, gos::DateTime *out_dt)
{
	assert(NULL != utf8_filePathAndName);
	assert(NULL != out_dt);

	HANDLE hFile;
	if (FS_fileOpen  (&hFile, utf8_filePathAndName, eFileMode::readOnly, false, false, true, false))
	{
		FILETIME time;
		if (::GetFileTime (hFile, &time, NULL, NULL))
		{
			SYSTEMTIME  stime;
			FileTimeToSystemTime (&time, &stime);
			out_dt->date.setYMD(stime.wYear, stime.wMonth, stime.wDay);
			out_dt->time.setHMS(stime.wHour, stime.wMinute, stime.wSecond, 0);
		}

		FS_fileClose (hFile);
	}
}

//*****************************************************
void platform::FS_fileGetLastTimeModified_UTC (const char *utf8_filePathAndName, gos::DateTime *out_dt)
{
	assert(NULL != utf8_filePathAndName);
	assert(NULL != out_dt);

	HANDLE hFile;
	if (FS_fileOpen  (&hFile, utf8_filePathAndName, eFileMode::readOnly, false, false, true, false))
	{
		FILETIME time;
		if (::GetFileTime (hFile, NULL, NULL, &time))
		{
			SYSTEMTIME  stime;
			FileTimeToSystemTime (&time, &stime);
			out_dt->date.setYMD(stime.wYear, stime.wMonth, stime.wDay);
			out_dt->time.setHMS(stime.wHour, stime.wMinute, stime.wSecond, 0);
		}

		FS_fileClose (hFile);
	}
}

//*****************************************************
void platform::FS_fileGetCreationTime_LocalTime (const char *utf8_filePathAndName, gos::DateTime *out_dt)
{
	assert(NULL != utf8_filePathAndName);
	assert(NULL != out_dt);

	HANDLE hFile;
	if (FS_fileOpen  (&hFile, utf8_filePathAndName, eFileMode::readOnly, false, false, true, false))
	{
		FILETIME time;
		if (::GetFileTime (hFile, &time, NULL, NULL))
		{
			SYSTEMTIME  stime, ltime;
			FileTimeToSystemTime (&time, &stime);
			SystemTimeToTzSpecificLocalTime (NULL, &stime, &ltime);
			out_dt->date.setYMD(ltime.wYear, ltime.wMonth, ltime.wDay);
			out_dt->time.setHMS(ltime.wHour, ltime.wMinute, ltime.wSecond, 0);
		}

		FS_fileClose (hFile);
	}
}

//*****************************************************
void platform::FS_fileGetLastTimeModified_LocalTime (const char *utf8_filePathAndName, gos::DateTime *out_dt)
{
	assert(NULL != utf8_filePathAndName);
	assert(NULL != out_dt);

	HANDLE hFile;
	if (FS_fileOpen  (&hFile, utf8_filePathAndName, eFileMode::readOnly, false, false, true, false))
	{
		FILETIME time;
		if (::GetFileTime (hFile, NULL, NULL, &time))
		{
			SYSTEMTIME  stime, ltime;
			FileTimeToSystemTime (&time, &stime);
			SystemTimeToTzSpecificLocalTime (NULL, &stime, &ltime);
			out_dt->date.setYMD(ltime.wYear, ltime.wMonth, ltime.wDay);
			out_dt->time.setHMS(ltime.wHour, ltime.wMinute, ltime.wSecond, 0);
		}

		FS_fileClose (hFile);
	}
}

//*****************************************************
bool platform::FS_fileOpen  (OSFile *out_h, const char *utf8_filePathAndName, eFileMode mode, bool bCreateIfNotExists, bool bAppend, bool bShareRead, bool bShareWrite)
{
	assert (NULL != out_h);
	assert (NULL != utf8_filePathAndName);

	wchar_t filePathAndName[1024];
	win32::utf8_towchar (utf8_filePathAndName, u32MAX, filePathAndName, sizeof(filePathAndName));

	DWORD dwDesiredAccess = 0;
	switch (mode)
	{
	default:
		dwDesiredAccess = GENERIC_READ;
		DBGBREAK;
		break;

	case eFileMode::readOnly:
		dwDesiredAccess = GENERIC_READ;
		break;

	case eFileMode::readWrite:
		dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
		break;

	case eFileMode::writeOnly:
		dwDesiredAccess = GENERIC_WRITE;
		break;
	}

	if (bAppend)
		dwDesiredAccess = FILE_APPEND_DATA;
	
	DWORD dwShareMode = 0;
	if (bShareRead)
		dwShareMode |= FILE_SHARE_READ;
	if (bShareWrite)
		dwShareMode |= FILE_SHARE_WRITE;

	DWORD dwCreationDisposition = OPEN_EXISTING;
	if (bCreateIfNotExists)
	{
		if (bAppend)
			dwCreationDisposition = OPEN_ALWAYS;	//se esiste lo apre, se non esiste lo crea
		else
			dwCreationDisposition = CREATE_ALWAYS;	//se esiste lo tronca, se non esiste lo crea
	}

	DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
	
	(*out_h) = CreateFile (filePathAndName, dwDesiredAccess, dwShareMode, NULL, dwCreationDisposition, dwFlagsAndAttributes, NULL);
	if (INVALID_HANDLE_VALUE == (*out_h))
		return false;
	return true;
}

//*****************************************************
u32 platform::FS_fileRead (OSFile &h, void *buffer, u32 numMaxBytesToRead)
{
	DWORD nRead = 0;
	if (::ReadFile (h, buffer, numMaxBytesToRead, &nRead, NULL))
		return nRead;
	return 0;
}

//*****************************************************
u32 platform::FS_fileWrite (OSFile &h, const void *buffer, u32 numBytesToWrite)
{
	DWORD nWritten = 0;
	if (::WriteFile (h, buffer, numBytesToWrite, &nWritten, NULL))
		return nWritten;
	return 0;
}

//*****************************************************
void platform::FS_fileClose (OSFile &h)
{
	::CloseHandle(h);
	h = INVALID_HANDLE_VALUE;
}

//*****************************************************
void platform::FS_fileFlush (OSFile &h)
{
	FlushFileBuffers(h);
}

//*****************************************************
u64 platform::FS_fileLength (OSFile &h)
{
	LARGE_INTEGER s;
	if (::GetFileSizeEx (h, &s))
		return s.QuadPart;
	DBGBREAK;
	return 0;
}

//*****************************************************
u64 platform::FS_fileLength (const char *utf8_filePathAndName)
{
	u64 ret = 0;

	HANDLE hFile;
	if (FS_fileOpen (&hFile, utf8_filePathAndName, eFileMode::readOnly, false, false, true, false))
	{
		ret = FS_fileLength(hFile);
		FS_fileClose (hFile);
	}
	return ret;
}

//*****************************************************
bool platform::FS_fileSeek(OSFile &h, u64 position, eSeek seekMode)
{
	LARGE_INTEGER s;
	s.QuadPart = position;

	switch (seekMode)
	{
	case eSeek::current:	return (0 != ::SetFilePointerEx (h, s, NULL, FILE_CURRENT));
	case eSeek::start:		return (0 != ::SetFilePointerEx (h, s, NULL, FILE_BEGIN));
	case eSeek::end:		return (0 != ::SetFilePointerEx (h, s, NULL, FILE_END));
	default:				DBGBREAK; return false;
	}
}

//*****************************************************
u64 platform::FS_fileTell(OSFile &h)
{
	LARGE_INTEGER sIN, sOUT;
	sIN.QuadPart = 0;
	SetFilePointerEx (h, sIN, &sOUT, FILE_CURRENT);
	return sOUT.QuadPart;
}


/*****************************************************
* ritorna:
*	false se non e' un file/directory da considerare (per esempio perche' e' un file hidden o la directory "." o il filename non matcha il jolly)
*	true se e' un file valido che matcha il patterno jolly (e il filtro attuale consente i file)
*	true se e' una directory valida (e il filtro attuale consente le directory)
*/
static bool FS_findFile_doesMatch (platform::OSFileFind &ff)
{
		if ((ff.findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0) return false;
		if ((ff.findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) return false;
		if ((ff.findData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) != 0) return false;

		win32::wchar_to_utf8 (ff.findData.cFileName, u32MAX, ff.utf8_curFilename, sizeof(ff.utf8_curFilename));
		if (FS_findIsDirectory(ff))
		{
			//e' una dir

			// il filtro consente le directory?
            if (0 == (ff.findMode & platform::OSFileFind::ALLOW_FOLDER))
                return false;

			if (ff.utf8_curFilename[0] == '.')
			{
				//se e' la directory "." bisogna skipparla
				if (0x00 == ff.utf8_curFilename[1])
					return false;

				//se e' la directory ".." bisogna skipparla
				if ('.' == ff.utf8_curFilename[1] && 0x00 == ff.utf8_curFilename[2])
					return false;
			}
			
			//e' una directory buona (nientre filtro jolly sulle dire)
            return true;
		}
		else 
		{
            //E' un file
            
			// il filtro consente i file?
			if (0 == (ff.findMode & platform::OSFileFind::ALLOW_FILE))
                return false;

			//il filename matcha?
			if (fs::doesFileNameMatchJolly(ff.utf8_curFilename, ff.utf8_jolly))
				return true;
		}

		return false;
}


//*****************************************************
bool platform::FS_findFirst (OSFileFind *ff, const char *utf8_path, const char *utf8_jolly, eFileFindMode ffmode)
{
	assert(ff->h == INVALID_HANDLE_VALUE);
	
    switch (ffmode)
    {
    default: 
        ff->findMode = 0;
        DBGBREAK;
        break;
    case eFileFindMode::both_file_and_folder:   ff->findMode = OSFileFind::ALLOW_FILE | OSFileFind::ALLOW_FOLDER; break;
    case eFileFindMode::only_folder:            ff->findMode = OSFileFind::ALLOW_FOLDER; break;
    case eFileFindMode::only_file:              ff->findMode = OSFileFind::ALLOW_FILE; break;
    }  	

	wchar_t wctemp[512];
	win32::utf8_towchar (utf8_path, u32MAX, wctemp, sizeof(wctemp));
	wcscat_s (wctemp, _countof(wctemp), L"/*.*");

	ff->h = ::FindFirstFile(wctemp, &ff->findData);
	if (ff->h == INVALID_HANDLE_VALUE)
		return false;

	strcpy_s ((char*)ff->utf8_jolly, sizeof(ff->utf8_jolly), (const char*)utf8_jolly);

	if (FS_findFile_doesMatch(*ff))
		return true;
    if (FS_findNext(*ff))
        return true;
    FS_findClose(*ff);
	return false;	
}

//*****************************************************
bool platform::FS_findNext(OSFileFind &ff)
{
	assert(ff.h != INVALID_HANDLE_VALUE);
	while (FindNextFile(ff.h, &ff.findData))
	{
		if (FS_findFile_doesMatch(ff))
			return true;
	}
	return false;
}

//*****************************************************
bool platform::FS_findIsDirectory(const OSFileFind &ff)
{
	assert(ff.h != INVALID_HANDLE_VALUE);
	return ((ff.findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
}

//*****************************************************
const char *platform::FS_findGetFileName (const OSFileFind &ff)
{
	assert(ff.h != INVALID_HANDLE_VALUE);
	return ff.utf8_curFilename;
}

//*****************************************************
void platform::FS_findGetFileName(const OSFileFind &ff, char *out, u32 sizeofOut)
{
	assert(ff.h != INVALID_HANDLE_VALUE);
	strcpy_s ((char*)out, sizeofOut, (const char*)ff.utf8_curFilename);
}


//*****************************************************
void platform::FS_findClose(OSFileFind &ff)
{
	assert(ff.h != INVALID_HANDLE_VALUE);
	::FindClose(ff.h);
	ff.h = INVALID_HANDLE_VALUE;
}

//*****************************************************
bool platform::FS_fileCopy (const char *src, const char *dst)
{
	wchar_t wSRC[512];
	if (!win32::utf8_towchar(src, u32MAX, wSRC, sizeof(wSRC)))
		return false;

	wchar_t wDST[512];
	if (!win32::utf8_towchar(dst, u32MAX, wDST, sizeof(wDST)))
		return false;


	return (0 !=::CopyFile (wSRC, wDST, FALSE));
}

#endif //GOS_PLATFORM__WINDOWS
