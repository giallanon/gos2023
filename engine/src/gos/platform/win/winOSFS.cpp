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
bool win32_createFolderFromUTF8Path (const u8 *utf8_path, u32 nBytesToUseForPath)
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
bool platform::FS_folderCreate (const u8 *utf8_path)
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
bool platform::FS_folderDelete(const u8 *utf8_path)
{
	wchar_t temp[512];
	if (win32::utf8_towchar(utf8_path, u32MAX, temp, sizeof(temp)))
		return (::RemoveDirectory(temp) != 0); 
	return false;
}

//*****************************************************
bool platform::FS_folderExists(const u8 *utf8_path)
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
bool platform::FS_fileExists(const u8 *utf8_filename)
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
bool platform::FS_fileDelete(const u8 *utf8_filename)
{
	wchar_t temp[512];
	if (!win32::utf8_towchar(utf8_filename, u32MAX, temp, sizeof(temp)))
		return false;
	return (::DeleteFile (temp) != 0);
}

//*****************************************************
bool platform::FS_fileRename(const u8 *utf8_pathNoSlash, const u8 *utf8_oldFilename, const u8 *utf8_newFilename)
{
	u8 utf8_temp[512];

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
void platform::FS_fileGetCreationTime_UTC (const char *filePathAndName, gos::DateTime *out_dt)
{
	platform::FS_fileGetCreationTime_UTC (reinterpret_cast<const u8*>(filePathAndName), out_dt);
}
void platform::FS_fileGetCreationTime_UTC (const u8 *utf8_filePathAndName, gos::DateTime *out_dt)
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
void platform::FS_fileGetLastTimeModified_UTC (const char *filePathAndName, gos::DateTime *out_dt)
{
	platform::FS_fileGetLastTimeModified_UTC (reinterpret_cast<const u8*>(filePathAndName), out_dt);
}
void platform::FS_fileGetLastTimeModified_UTC (const u8 *utf8_filePathAndName, gos::DateTime *out_dt)
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
void platform::FS_fileGetCreationTime_LocalTime (const char *filePathAndName, gos::DateTime *out_dt)
{
	FS_fileGetCreationTime_LocalTime (reinterpret_cast<const u8*>(filePathAndName), out_dt);
}
void platform::FS_fileGetCreationTime_LocalTime (const u8 *utf8_filePathAndName, gos::DateTime *out_dt)
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
void platform::FS_fileGetLastTimeModified_LocalTime (const char *filePathAndName, gos::DateTime *out_dt)
{
	FS_fileGetLastTimeModified_LocalTime (reinterpret_cast<const u8*>(filePathAndName), out_dt);
}
void platform::FS_fileGetLastTimeModified_LocalTime (const u8 *utf8_filePathAndName, gos::DateTime *out_dt)
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
bool platform::FS_fileOpen  (OSFile *out_h, const u8 *utf8_filePathAndName, eFileMode mode, bool bCreateIfNotExists, bool bAppend, bool bShareRead, bool bShareWrite)
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
		dwCreationDisposition = OPEN_ALWAYS;

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
u64 platform::FS_fileLength (const u8 *utf8_filePathAndName)
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
void platform::FS_fileSeek(OSFile &h, u64 position, eSeek seekMode)
{
	LARGE_INTEGER s;
	s.QuadPart = position;

	switch (seekMode)
	{
	case eSeek::current:	::SetFilePointerEx (h, s, NULL, FILE_CURRENT); break;
	case eSeek::start:		::SetFilePointerEx (h, s, NULL, FILE_BEGIN); break;
	case eSeek::end:		::SetFilePointerEx (h, s, NULL, FILE_END); break;
	default:				break;
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




//*****************************************************
bool platform::FS_findFirst(OSFileFind *ff, const u8 *utf8_path, const u8 *utf8_jolly)
{
	assert(ff->h == INVALID_HANDLE_VALUE);

	wchar_t wctemp[512];
	win32::utf8_towchar (utf8_path, u32MAX, wctemp, sizeof(wctemp));
	wcscat_s (wctemp, _countof(wctemp), L"/*.*");

	ff->h = ::FindFirstFile(wctemp, &ff->findData);
	if (ff->h == INVALID_HANDLE_VALUE)
		return false;

	strcpy_s ((char*)ff->utf8_jolly, sizeof(ff->utf8_jolly), (const char*)utf8_jolly);
	do
	{
		if ((ff->findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0) continue;
		if ((ff->findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) continue;
		if ((ff->findData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) != 0) continue;
		
		win32::wchar_to_utf8 (ff->findData.cFileName, u32MAX, ff->utf8_curFilename, sizeof(ff->utf8_curFilename));
		if (FS_findIsDirectory(*ff) || fs::doesFileNameMatchJolly(ff->utf8_curFilename, utf8_jolly))
			return true;
	} while (FS_findNext(*ff));
	
	FS_findClose(*ff);
	return false;	
}

//*****************************************************
bool platform::FS_findNext(OSFileFind &ff)
{
	assert(ff.h != INVALID_HANDLE_VALUE);
	while (FindNextFile(ff.h, &ff.findData))
	{
		if ((ff.findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0) continue;
		if ((ff.findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) continue;
		if ((ff.findData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) != 0) continue;

		win32::wchar_to_utf8 (ff.findData.cFileName, u32MAX, ff.utf8_curFilename, sizeof(ff.utf8_curFilename));
		if (FS_findIsDirectory(ff) || fs::doesFileNameMatchJolly(ff.utf8_curFilename, ff.utf8_jolly))
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
const u8 *platform::FS_findGetFileName(const OSFileFind &ff)
{
	assert(ff.h != INVALID_HANDLE_VALUE);
	return ff.utf8_curFilename;
}

//*****************************************************
void platform::FS_findGetFileName(const OSFileFind &ff, u8 *out, u32 sizeofOut)
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



#endif //GOS_PLATFORM__WINDOWS
