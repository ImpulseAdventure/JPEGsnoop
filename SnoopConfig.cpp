// JPEGsnoop - JPEG Image Decoder & Analysis Utility
// Copyright (C) 2015 - Calvin Hass
// http://www.impulseadventure.com/photo/jpeg-snoop.html
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "StdAfx.h"
#include ".\snoopconfig.h"
#include "snoop.h"
#include "Registry.h"

//#include <shlobj.h>    // for SHGetFolderPath


// Define the defaults for the application configuration
CSnoopConfig::CSnoopConfig(void)
{

	// Debug log
	strDebugLogFname = _T(".\\JPEGsnoop-debug.log");
	fpDebugLog = NULL;
	bDebugLogEnable = false;

	// Default to showing message dialogs
	bInteractive = true;

	// Command-line modes
	bGuiMode = true;
	bCmdLineOpenEn = false;
	strCmdLineOpenFname = _T("");
	bCmdLineOutputEn = false;	// No output file specified
	strCmdLineOutputFname = _T("");
	bCmdLineBatchEn = false;
	strCmdLineBatchDirName = _T("");
	bCmdLineBatchRec = false;
	bCmdLineExtractEn = false;
	bCmdLineExtractDhtAvi = false;
	bCmdLineBatchSrch = false;
	bCmdLineDoneMsg = false;

	eCmdLineOffset = DEC_OFFSET_START;
	nCmdLineOffsetPos = 0;

	bCmdLineHelp = false;

	nPosStart = 0;

	// --------------------------------
	// Registry settings
	bDirty = false;			// Have the registry options been dirtied?
	bEulaAccepted = false;
	bUpdateAuto = true;
	nUpdateAutoDays = 14;		// Check every 14 days
	strUpdateLastChk = _T("");
	strDbDir = _T("");
	bReprocessAuto = false;
	bDecodeScanImg = true;
	bDecodeScanImgAc = false;		// Coach message will be shown just in case
	bSigSearch = true;

	bOutputScanDump = false;		// Print snippet of scan data
	bOutputDHTexpand = false;		// Print expanded huffman tables
	bDecodeMaker = false;
	bDumpHistoY = false;

	// Difference in performance: 1dsmk2 image:
	// Performance boost ~ 25%
	bHistoEn = false;				// Histogram & clipping stats enabled?

	bStatClipEn = false;			// UNUSED: Enable Scan Decode clip stats?

	bDbSubmitNet = true;			// Default to submit new DB entries to web

	bExifHideUnknown = true;		// Default to hiding unknown EXIF tags
	bRelaxedParsing = false;		// Normal parsing stops on bad marker

	nErrMaxDecodeScan = 20;

	strBatchLastInput = _T("C:\\");			// Default batch processing directory (input)
	strBatchLastOutput = _T("C:\\");		// Default batch processing directory (output)
	strBatchExtensions = _T(".jpg,.jpeg");	// Default batch extension list

	bDecodeColorConvert = true;		// Perform color convert after scan decode

	// Reset coach message flags
	CoachReset();

	// Reset the current filename
	strCurFname = _T("");

	// --------------------------------

	
	// Determine operating system
	// Particularly for: WinHTTP functions
    OSVERSIONINFO osvi;

    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    GetVersionEx(&osvi);

	bIsWindowsNTorLater = (osvi.dwPlatformId >= VER_PLATFORM_WIN32_NT);

    bIsWindowsXPorLater = 
       ( (osvi.dwMajorVersion > 5) ||
       ( (osvi.dwMajorVersion == 5) && (osvi.dwMinorVersion >= 1) ));
	
	/*
	// Debug code to output the version info
	CString strTmp;
	strTmp.Format(_T("OS Version: Platform=[%u] VerMajor=[%u] VerMinor=[%u] (>=NT:%s >=XP:%s)"),
		osvi.dwPlatformId,osvi.dwMajorVersion,osvi.dwMinorVersion,
		(bIsWindowsNTorLater)?"Y":"N",(bIsWindowsXPorLater)?"Y":"N");
	AfxMessageBox(strTmp);
	*/

}

CSnoopConfig::~CSnoopConfig(void)
{
}


// This is generally called after app initializes and registry
// has just been loaded.
void CSnoopConfig::UseDefaults()
{
	// Assume all settings are to their default values
	// But some need copying over
}

// Update the flag to mark the configuration as dirty / modified
// - This is used later to indicate that a resave is required
//
void CSnoopConfig::Dirty(bool mode)
{
	bDirty = mode;
}

// Fetch all configuration values from the registry
void CSnoopConfig::RegistryLoad()
{
	CString		strKeyPath;
	CString		strField;
	CString		strKeyFull;

	strKeyPath = REG_KEY_PATH;

	/////////////

	strField = _T("General\\UserDbPath");
	strKeyFull = strKeyPath + strField;
	CRegString regUserDbPath(strKeyFull,_T("???"),TRUE,HKEY_CURRENT_USER);

	// Try to load the registry entry
	if ((CString)regUserDbPath == _T("???")) {
		// None specified, so determine a suitable default
		strDbDir = GetDefaultDbDir();
		bDirty = true;

	} else {
		// Specified, so store in App Config struct
		strDbDir = regUserDbPath;
	}



	//////////////////

	CString strCurDate;
	CTime tmeToday = CTime::GetCurrentTime();
	strCurDate = tmeToday.Format(_T("%Y%m%d"));

	strField = _T("General\\UpdateLastChk");
	strKeyFull = strKeyPath + strField;
	CRegString regUpdateLastChk(strKeyFull,_T("???"),TRUE,HKEY_CURRENT_USER);

	// Try to load the registry entry
	if ((CString)regUpdateLastChk == _T("???")) {
		// None specified, so determine a suitable default
		strUpdateLastChk = strCurDate;
		bDirty = true;
	} else {
		// Specified, so store in App Config struct
		strUpdateLastChk = regUpdateLastChk;
	}

	//////////////////

//	RegistryLoadStr( _T("General\\UserDbPath"),     "???", strDbDir);
	RegistryLoadBool(_T("General\\UpdateAuto"),     999,   bUpdateAuto);
	RegistryLoadUint(_T("General\\UpdateAutoDays"), 999,   nUpdateAutoDays);
//	RegistryLoadStr( _T("General\\UpdateLastChk"),  "???", strUpdateLastChk);
	RegistryLoadBool(_T("General\\EulaAccepted"),   999,   bEulaAccepted);
	RegistryLoadBool(_T("General\\ReprocessAuto"),  999,   bReprocessAuto);
	RegistryLoadBool(_T("General\\SigSearch"),      999,   bSigSearch);
	RegistryLoadBool(_T("General\\DecScanImg"),     999,   bDecodeScanImg);
	RegistryLoadBool(_T("General\\DecScanImgAc"),   999,   bDecodeScanImgAc);

	RegistryLoadBool(_T("General\\DumpScan"),       999,   bOutputScanDump);
	RegistryLoadBool(_T("General\\DumpDHTExpand"),  999,   bOutputDHTexpand);
	RegistryLoadBool(_T("General\\DecMaker"),       999,   bDecodeMaker);
	RegistryLoadBool(_T("General\\DumpHistoY"),     999,   bDumpHistoY);
//	RegistryLoadBool(_T("General\\DecScanClip"),    999,   bStatClipEn);
	RegistryLoadBool(_T("General\\DecScanHisto"),   999,   bHistoEn);

	RegistryLoadBool(_T("General\\DbSubmitNet"),    999,   bDbSubmitNet);
	RegistryLoadBool(_T("General\\ExifHideUnk"),    999,   bExifHideUnknown);
	RegistryLoadBool(_T("General\\RelaxedParsing"), 999,   bRelaxedParsing);

	RegistryLoadUint(_T("Report\\ErrMaxDecodeScan"), 999,  nErrMaxDecodeScan);

	RegistryLoadStr( _T("General\\BatchLastInput"),  _T(""), strBatchLastInput);
	RegistryLoadStr( _T("General\\BatchLastOutput"), _T(""), strBatchLastOutput);
	RegistryLoadStr( _T("General\\BatchExtensions"), _T(""), strBatchExtensions);

	RegistryLoadBool(_T("Coach\\ReprocessAuto"),    999,   bCoachReprocessAuto);
	RegistryLoadBool(_T("Coach\\DecodeIdct"),       999,   bCoachDecodeIdct);

	// Deprecated Registry strings
	// FIXME: Later on, decide if we should delete deprecated strings if found
	// "General\\ScanImgDec"
	// "General\\ScanImgDecDef"
	// "General\\ScanPreviewDef"
	// "General\\SigSearchAuto"
}

// Write all configuration parameters to the registry
// - Skip write if no changes were made (!bDirty)
//
void CSnoopConfig::RegistryStore()
{
	// If no changes made, skip
	if (!bDirty) {
		return;
	}

	RegistryStoreStr(  _T("General\\UserDbPath"),     strDbDir);
	RegistryStoreBool( _T("General\\UpdateAuto"),     bUpdateAuto);
	RegistryStoreUint( _T("General\\UpdateAutoDays"), nUpdateAutoDays);
	RegistryStoreStr(  _T("General\\UpdateLastChk"),  strUpdateLastChk);
	RegistryStoreBool( _T("General\\EulaAccepted"),   bEulaAccepted);
	RegistryStoreBool( _T("General\\ReprocessAuto"),  bReprocessAuto);
	RegistryStoreBool( _T("General\\SigSearch"),      bSigSearch);
	RegistryStoreBool( _T("General\\DecScanImg"),     bDecodeScanImg);
	RegistryStoreBool( _T("General\\DecScanImgAc"),   bDecodeScanImgAc);

	RegistryStoreBool( _T("General\\DumpScan"),       bOutputScanDump);
	RegistryStoreBool( _T("General\\DumpDHTExpand"),  bOutputDHTexpand);
	RegistryStoreBool( _T("General\\DecMaker"),       bDecodeMaker);
	RegistryStoreBool( _T("General\\DumpHistoY"),     bDumpHistoY);
	RegistryStoreBool( _T("General\\DecScanHisto"),   bHistoEn);

	RegistryStoreBool( _T("General\\DbSubmitNet"),    bDbSubmitNet);
	RegistryStoreBool( _T("General\\ExifHideUnk"),    bExifHideUnknown);
	RegistryStoreBool( _T("General\\RelaxedParsing"), bRelaxedParsing);

	RegistryStoreUint( _T("Report\\ErrMaxDecodeScan"), nErrMaxDecodeScan);

	RegistryStoreStr( _T("General\\BatchLastInput"),  strBatchLastInput);
	RegistryStoreStr( _T("General\\BatchLastOutput"), strBatchLastOutput);
	RegistryStoreStr( _T("General\\BatchExtensions"), strBatchExtensions);

	RegistryStoreBool( _T("Coach\\ReprocessAuto"),    bCoachReprocessAuto);
	RegistryStoreBool( _T("Coach\\DecodeIdct"),       bCoachDecodeIdct);

	// Registry entries are no longer dirty
	bDirty = false;

}


// Load a string from the registry
//
// INPUT:
// - strKey				Registry key to read
// - strDefault			Default value (in case the key doesn't exist)
//
// OUTPUT:
// - String value from the registry key (or strDefault if not defined)
//
void CSnoopConfig::RegistryLoadStr(CString strKey,CString strDefault,CString &strSetting)
{
	CString strKeyPath = REG_KEY_PATH;
	CString strKeyFull = strKeyPath + strKey;
	CRegString reg(strKeyFull,strDefault,TRUE,HKEY_CURRENT_USER);

	// Try to load the registry entry
	if (reg == strDefault) {
		// None specified, so leave default from constructor
	} else {
		// Specified, so store in App Config struct
		strSetting = reg;
	}
}

// Load a boolean from the registry
//
// INPUT:
// - strKey				Registry key to read
// - nDefault			Default value (in case the key doesn't exist)
//
// OUTPUT:
// - Boolean value from the registry key (or nDefault if not defined)
//
void CSnoopConfig::RegistryLoadBool(CString strKey,unsigned nDefault,bool &bSetting)
{
	CString strKeyPath = REG_KEY_PATH;
	CString strKeyFull = strKeyPath + strKey;
	CRegDWORD reg(strKeyFull,nDefault,TRUE,HKEY_CURRENT_USER);

	// Try to load the registry entry
	if (reg == nDefault) {
		// None specified, so leave default from constructor
	} else {
		// Specified, so store in App Config struct
		bSetting = (reg>=1)?true:false;
	}
}

// Load an UINT32 from the registry
//
// INPUT:
// - strKey				Registry key to read
// - nDefault			Default value (in case the key doesn't exist)
//
// OUTPUT:
// - UINT32 value from the registry key (or nDefault if not defined)
//
void CSnoopConfig::RegistryLoadUint(CString strKey,unsigned nDefault,unsigned &nSetting)
{
	CString strKeyPath = REG_KEY_PATH;
	CString strKeyFull = strKeyPath + strKey;
	CRegDWORD reg(strKeyFull,nDefault,TRUE,HKEY_CURRENT_USER);

	// Try to load the registry entry
	if (reg == nDefault) {
		// None specified, so leave default from constructor
	} else {
		// Specified, so store in App Config struct
		nSetting = reg;
	}
}

// Store an UINT32 to the registry
//
// INPUT:
// - strKey				Registry key to write
// - nSetting			Value to store at registry key
//
void CSnoopConfig::RegistryStoreUint(CString strKey,unsigned nSetting)
{
	CString strKeyPath = REG_KEY_PATH;
	CString strKeyFull = strKeyPath + strKey;
	CRegDWORD regWr(strKeyFull,999,TRUE,HKEY_CURRENT_USER);
	regWr = nSetting;
	CRegDWORD regRd(strKeyFull,999,TRUE,HKEY_CURRENT_USER);
	ASSERT(regRd == nSetting);
}

// Store a boolean to the registry
//
// INPUT:
// - strKey				Registry key to write
// - bSetting			Value to store at registry key
//
void CSnoopConfig::RegistryStoreBool(CString strKey,bool bSetting)
{
	RegistryStoreUint(strKey,(bSetting)?1:0);
}

// Store a string to the registry
//
// INPUT:
// - strKey				Registry key to write
// - strSetting			Value to store at registry key
//
void CSnoopConfig::RegistryStoreStr(CString strKey,CString strSetting)
{
	CString strKeyPath = REG_KEY_PATH;
	CString strKeyFull = strKeyPath + strKey;
	CRegString regWr(strKeyFull,_T("???"),TRUE,HKEY_CURRENT_USER);
	regWr = strSetting;
	CRegString regRd(strKeyFull,_T("???"),TRUE,HKEY_CURRENT_USER);
	ASSERT(regRd == strSetting);
}



// Reset coach messages
void CSnoopConfig::CoachReset()
{
	// Enable all coach messages
	bCoachReprocessAuto = true;
	bCoachDecodeIdct = true;

	Dirty();
}

// Select a suitable working directory, create it if it doesn't already exist
CString CSnoopConfig::GetDefaultDbDir()
{
	TCHAR szUserProfile[MAX_PATH]; 
	TCHAR szFilePath[MAX_PATH];


	// Apparently, the SHGetFolderPath is not well supported in earlier
	// versions of the OS, so we are only going to try it if we are
	// running Win XP or later.
	// See: http://support.microsoft.com/kb/310294
	// Older OS versions may require linking to shfolder.dll, which I decided
	// not to implement yet.

	// Since I could not get SHGetFolderPath() to compile, even after including
	// <shlobj.h>, I decided to revert to older version of the call:
	// https://msdn2.microsoft.com/en-us/library/aa931257.aspx
	
	if (bIsWindowsNTorLater) {
		// Get path for each computer, non-user specific and non-roaming data.
		//if ( SUCCEEDED( SHGetFolderPath( NULL, CSIDL_APPDATA, 
		//								NULL, 0, szFilePath ) ) )
		if ( SHGetSpecialFolderPath( NULL, szFilePath, CSIDL_APPDATA, true ))
		{
			// Append product-specific path.
			PathAppend( szFilePath, _T("\\JPEGsnoop") );

			// Since the full path may not already exist, and CreateDirectory() only
			// works on one level of hierarchy at a time, use a recursive function
			// to do the job.
			CreateDir(szFilePath);
		} else {
			// No error handling
			AfxMessageBox(_T("Problem trying to locate suitable directory; please choose manually"));

			// As a default, start with EXE directory
			wsprintf(szFilePath,GetExeDir());
		}
	} else {

		// Apparently not a recent version of Windows, so we'll revert
		// to some older methods at getting the directory

		ExpandEnvironmentStrings(_T("%userprofile%"),szUserProfile,sizeof(szUserProfile)/sizeof(*szUserProfile));

		// NOTE:
		// - In Win95/98 it is likely that the above will return the literal "%userprofile%"
		//   so in that case we don't want to create a directory -- use the exe dir instead
		if (!_tcscmp(szUserProfile,_T("%userprofile%"))) {
			wsprintf(szFilePath, _T(""));
			// Since we can't locate the user profile directory from environment
			// strings, we don't want to create any subdirectory. If we were to
			// call CreateDir() with an empty string, we'd end up with a garbled
			// folder name created!

			// So, for now, default to executable directory
			wsprintf(szFilePath,GetExeDir());
		} else {
			// FIXME
			// Note that the following is not very good as it doesn't select
			// a suitable directory for non-English systems.
			wsprintf(szFilePath, _T("%s%s"), szUserProfile,_T("\\Application Data\\JPEGsnoop"));

			// Since the full path may not already exist, and CreateDirectory() only
			// works on one level of hierarchy at a time, use a recursive function
			// to do the job.
			CreateDir(szFilePath);
		}
	}
	return szFilePath;
}

// Get the directory path of the JPEGsnoop executable
// - This is used as a default location for the user signature database
CString CSnoopConfig::GetExeDir()
{
	TCHAR szPath[1024];
	GetModuleFileName(AfxGetApp()->m_hInstance, szPath, sizeof(szPath)/sizeof(*szPath));
	
	TCHAR *ptr = _tcsrchr(szPath, '\\');
	
	if (ptr)
	{
		ptr[1] = '\0';
	}
	
	ASSERT(_tcslen(szPath) < sizeof(szPath));
	
	return szPath;
}

// Recursively create directories as needed to create full path
// TODO: Should replace this by SHCreateDirectoryEx()
void CSnoopConfig::CreateDir(LPTSTR Path)
{
	TCHAR DirName[256];
	TCHAR* p = Path;
	TCHAR* q = DirName; 
	DirName[0] = '\0';	// Start as null-terminated

	while(*p)
	{
		if (('\\' == *p) || ('/' == *p))
		{
			if (':' != *(p-1))
			{
				CreateDirectory(DirName, NULL);
			}
		}
		*q++ = *p++;
		*q = '\0';
	}
	CreateDirectory(DirName, NULL);
}



// Create a new debug log if the control key is pressed during startup
//
// Note that we do not keep the log file open to help ensure that
// all debug entries are flushed to the file (in case of an
// an application crash).
//
bool CSnoopConfig::DebugLogCreate()
{
#ifdef DEBUG_LOG_OUT

	if (strDebugLogFname == _T("")) {
		return false;
	}

	// Detect control keys
	// - VK_CONTROL = Control key
	// - VK_SHIFT   = Shift
	// - VK_MENU    = Alt?
	if (GetAsyncKeyState(VK_CONTROL)) {
		AfxMessageBox(_T("Debug Log Enabled"));
		bDebugLogEnable = true;
	} else {
		return true;
	}

	ASSERT(fpDebugLog == NULL);

	try
	{
		// Open specified file
		fpDebugLog = new CStdioFile(strDebugLogFname, CFile::modeCreate| CFile::modeWrite | CFile::typeText | CFile::shareDenyNone);
	}
	catch (CFileException* e)
	{
		TCHAR msg[MAX_BUF_EX_ERR_MSG];
		CString strError;
		e->GetErrorMessage(msg,MAX_BUF_EX_ERR_MSG);
		e->Delete();
		strError.Format(_T("ERROR: Couldn't open debug log file for write [%s]: [%s]"),
			(LPCTSTR)strDebugLogFname, (LPCTSTR)msg);
		AfxMessageBox(strError);
		fpDebugLog = NULL;

		return false;
	}

	CString		strLine;
	strLine.Format(_T("JPEGsnoop version [%s]\r\n"),VERSION_STR);
	fpDebugLog->WriteString(strLine);
	fpDebugLog->WriteString(_T("-----\r\n"));

	// Close the file
	if (fpDebugLog) {
		fpDebugLog->Close();
		delete fpDebugLog;
		fpDebugLog = NULL;
	}

	// Extra code to record OS version
	OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);
	strLine.Format(_T("OS: PlatformId = %u"),osvi.dwPlatformId);
	if (DEBUG_EN) DebugLogAdd(strLine);
	strLine.Format(_T("OS: MajorVersion = %u"),osvi.dwMajorVersion);
	if (DEBUG_EN) DebugLogAdd(strLine);
	strLine.Format(_T("OS: MinorVersion = %u"),osvi.dwMinorVersion);
	if (DEBUG_EN) DebugLogAdd(strLine);
	strLine.Format(_T("OS: BuildNumber = %u"),osvi.dwBuildNumber);
	if (DEBUG_EN) DebugLogAdd(strLine);
	strLine.Format(_T("OS: OSVersionInfoSize = %u"),osvi.dwOSVersionInfoSize);
	if (DEBUG_EN) DebugLogAdd(strLine);
	if (DEBUG_EN) DebugLogAdd(_T("\n"));

#endif
	return true;
}

// If the debug log has been enabled, append the string parameter
// to the log file.
//
// Note that we do not keep the log file open to help ensure that
// all debug entries are flushed to the file (in case of an
// an application crash).
//
bool CSnoopConfig::DebugLogAdd(CString strText)
{
#ifdef DEBUG_LOG_OUT

	if (!bDebugLogEnable) {
		return true;
	}

	if (strDebugLogFname == _T("")) {
		return false;
	}

	ASSERT(fpDebugLog == NULL);

	try
	{
		// Open specified file but append
		fpDebugLog = new CStdioFile(strDebugLogFname, CFile::modeCreate| CFile::modeWrite | CFile::typeText | CFile::shareDenyNone |
			CFile::modeNoTruncate);
	}
	catch (CFileException* e)
	{
		TCHAR msg[MAX_BUF_EX_ERR_MSG];
		CString strError;
		e->GetErrorMessage(msg,MAX_BUF_EX_ERR_MSG);
		e->Delete();
		strError.Format(_T("ERROR: Couldn't open debug log file for append [%s]: [%s]"),
			(LPCTSTR)strDebugLogFname, (LPCTSTR)msg);
		AfxMessageBox(strError);
		fpDebugLog = NULL;

		return false;
	}

	// Now seek to the end of the file
	fpDebugLog->SeekToEnd();

	CString		strLine;

	// Get current time
/*
	time_t		sTime;
	struct tm	sNow;
	unsigned	nTmYear,nTmMon,nTmDay;
	unsigned	nTmHour,nTmMin,nTmSec;
	localtime_s(&sNow,&sTime);
	nTmYear = sNow.tm_year + 1900;
	nTmMon = sNow.tm_mon + 1;
	nTmDay = sNow.tm_mday;
	nTmHour = sNow.tm_hour;
	nTmMin = sNow.tm_min;
	nTmSec = sNow.tm_sec;


	// Generate log line
	//strLine.Format(_T("[%4u/%2u/%2u %2u:%2u:%2u] %s\r\n"),nTmYear,nTmMon,nTmDay,nTmHour,nTmMin,nTmSec,strText);
*/
	strLine.Format(_T("%s\n"),strText);
	fpDebugLog->WriteString(strLine);

	// Close the file
	if (fpDebugLog) {
		fpDebugLog->Close();
		delete fpDebugLog;
		fpDebugLog = NULL;
	}

#endif
	return true;
}