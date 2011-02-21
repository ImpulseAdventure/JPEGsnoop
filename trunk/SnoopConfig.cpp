// JPEGsnoop - JPEG Image Decoder & Analysis Utility
// Copyright (C) 2010 - Calvin Hass
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


CSnoopConfig::CSnoopConfig(void)
{

	cmdline_gui = true;
	cmdline_open = false;
	cmdline_output = false;	// No output file specified

	nPosStart = 0;

	// --------------------------------
	// Registry settings
	bDirty = false;			// Have the registry options been dirtied?
	bEulaAccepted = false;
	bUpdateAuto = false;
	nUpdateAutoDays = 14;		// Check every 14 days
	strUpdateLastChk = "";
	strDbDir = "";
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

	nErrMaxDecodeScan = 20;

	bDecodeColorConvert = true;		// Perform color convert after scan decode

	// Reset coach message flags
	CoachReset();


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
	CString tmpStr;//CAL! ***
	tmpStr.Format("OS Version: Platform=[%u] VerMajor=[%u] VerMinor=[%u] (>=NT:%s >=XP:%s)",
		osvi.dwPlatformId,osvi.dwMajorVersion,osvi.dwMinorVersion,
		(bIsWindowsNTorLater)?"Y":"N",(bIsWindowsXPorLater)?"Y":"N");
	AfxMessageBox(tmpStr);
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

void CSnoopConfig::Dirty(bool mode)
{
	bDirty = mode;
}

void CSnoopConfig::RegistryLoad()
{
	CString		strKeyPath;
	CString		strField;
	CString		strKeyFull;

	strKeyPath = REG_KEY_PATH;

	/////////////

	strField = "General\\UserDbPath";
	strKeyFull = strKeyPath + strField;
	CRegString regUserDbPath(strKeyFull,"???",TRUE,HKEY_CURRENT_USER);

	// Try to load the registry entry
	if ((CString)regUserDbPath == "???") {
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
	strCurDate = tmeToday.Format("%Y%m%d");

	strField = "General\\UpdateLastChk";
	strKeyFull = strKeyPath + strField;
	CRegString regUpdateLastChk(strKeyFull,"???",TRUE,HKEY_CURRENT_USER);

	// Try to load the registry entry
	if ((CString)regUpdateLastChk == "???") {
		// None specified, so determine a suitable default
		strUpdateLastChk = strCurDate;
		bDirty = true;
	} else {
		// Specified, so store in App Config struct
		strUpdateLastChk = regUpdateLastChk;
	}

	//////////////////

//	RegistryLoadStr( "General\\UserDbPath",     "???", strDbDir);
	RegistryLoadBool("General\\UpdateAuto",     999,   bUpdateAuto);
	RegistryLoadUint("General\\UpdateAutoDays", 999,   nUpdateAutoDays);
//	RegistryLoadStr( "General\\UpdateLastChk",  "???", strUpdateLastChk);
	RegistryLoadBool("General\\EulaAccepted",   999,   bEulaAccepted);
	RegistryLoadBool("General\\ReprocessAuto",  999,   bReprocessAuto);
	RegistryLoadBool("General\\SigSearch",      999,   bSigSearch);
	RegistryLoadBool("General\\DecScanImg",     999,   bDecodeScanImg);
	RegistryLoadBool("General\\DecScanImgAc",   999,   bDecodeScanImgAc);

	RegistryLoadBool("General\\DumpScan",       999,   bOutputScanDump);
	RegistryLoadBool("General\\DumpDHTExpand",  999,   bOutputDHTexpand);
	RegistryLoadBool("General\\DecMaker",       999,   bDecodeMaker);
	RegistryLoadBool("General\\DumpHistoY",     999,   bDumpHistoY);
//	RegistryLoadBool("General\\DecScanClip",    999,   bStatClipEn);
	RegistryLoadBool("General\\DecScanHisto",   999,   bHistoEn);

	RegistryLoadBool("General\\DbSubmitNet",    999,   bDbSubmitNet);
	RegistryLoadBool("General\\ExifHideUnk",    999,   bExifHideUnknown);

	RegistryLoadUint("Report\\ErrMaxDecodeScan", 999,  nErrMaxDecodeScan);

	RegistryLoadBool("Coach\\ReprocessAuto",    999,   bCoachReprocessAuto);
	RegistryLoadBool("Coach\\DecodeIdct",       999,   bCoachDecodeIdct);

	// Deprecated Registry strings
	// FIXME - Later on, decide to delete these if found
	// "General\\ScanImgDec"
	// "General\\ScanImgDecDef"
	// "General\\ScanPreviewDef"
	// "General\\SigSearchAuto"
	// "General\\DecScanClip"		(only for me)
}

void CSnoopConfig::RegistryStore()
{
	// If no changes made, skip
	if (!bDirty) {
		return;
	}

	RegistryStoreStr(  "General\\UserDbPath",     strDbDir);
	RegistryStoreBool( "General\\UpdateAuto",     bUpdateAuto);
	RegistryStoreUint( "General\\UpdateAutoDays", nUpdateAutoDays);
	RegistryStoreStr(  "General\\UpdateLastChk",  strUpdateLastChk);
	RegistryStoreBool( "General\\EulaAccepted",   bEulaAccepted);
	RegistryStoreBool( "General\\ReprocessAuto",  bReprocessAuto);
	RegistryStoreBool( "General\\SigSearch",      bSigSearch);
	RegistryStoreBool( "General\\DecScanImg",     bDecodeScanImg);
	RegistryStoreBool( "General\\DecScanImgAc",   bDecodeScanImgAc);

	RegistryStoreBool( "General\\DumpScan",       bOutputScanDump);
	RegistryStoreBool( "General\\DumpDHTExpand",  bOutputDHTexpand);
	RegistryStoreBool( "General\\DecMaker",       bDecodeMaker);
	RegistryStoreBool( "General\\DumpHistoY",     bDumpHistoY);
//	RegistryStoreBool( "General\\DecScanClip",    bStatClipEn);
	RegistryStoreBool( "General\\DecScanHisto",   bHistoEn);

	RegistryStoreBool( "General\\DbSubmitNet",    bDbSubmitNet);
	RegistryStoreBool( "General\\ExifHideUnk",    bExifHideUnknown);

	RegistryStoreUint( "Report\\ErrMaxDecodeScan", nErrMaxDecodeScan);

	RegistryStoreBool( "Coach\\ReprocessAuto",    bCoachReprocessAuto);
	RegistryStoreBool( "Coach\\DecodeIdct",       bCoachDecodeIdct);

	// Registry entries are no longer dirty
	bDirty = false;

}


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

void CSnoopConfig::RegistryStoreUint(CString strKey,unsigned nSetting)
{
	CString strKeyPath = REG_KEY_PATH;
	CString strKeyFull = strKeyPath + strKey;
	CRegDWORD regWr(strKeyFull,999,TRUE,HKEY_CURRENT_USER);
	regWr = nSetting;
	CRegDWORD regRd(strKeyFull,999,TRUE,HKEY_CURRENT_USER);
	ASSERT(regRd == nSetting);
}

void CSnoopConfig::RegistryStoreBool(CString strKey,bool bSetting)
{
	RegistryStoreUint(strKey,(bSetting)?1:0);
}

void CSnoopConfig::RegistryStoreStr(CString strKey,CString strSetting)
{
	CString strKeyPath = REG_KEY_PATH;
	CString strKeyFull = strKeyPath + strKey;
	CRegString regWr(strKeyFull,"???",TRUE,HKEY_CURRENT_USER);
	regWr = strSetting;
	CRegString regRd(strKeyFull,"???",TRUE,HKEY_CURRENT_USER);
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
		if ( SUCCEEDED( SHGetSpecialFolderPath( NULL, szFilePath, CSIDL_APPDATA, true )))
		{
			// Append product-specific path.
			PathAppend( szFilePath, "\\JPEGsnoop" );

			// Since the full path may not already exist, and CreateDirectory() only
			// works on one level of hierarchy at a time, use a recursive function
			// to do the job.
			CreateDir(szFilePath);
		} else {
			// No error handling
			AfxMessageBox("Problem trying to locate suitable directory; please choose manually");

			// As a default, start with EXE directory
			wsprintf(szFilePath,GetExeDir());
		}
	} else {

		// Apparently not a recent version of Windows, so we'll revert
		// to some older methods at getting the directory

		ExpandEnvironmentStrings("%userprofile%",szUserProfile,sizeof(szUserProfile));

		// *** In Win95/98 it is likely that the above will return the literal "%userprofile%"
		//     so in that case we don't want to create a directory -- use the exe dir instead
		if (!strcmp(szUserProfile,"%userprofile%")) {
			wsprintf(szFilePath, "");
			// Since we can't locate the user profile directory from environment
			// strings, we don't want to create any subdirectory. If we were to
			// call CreateDir() with an empty string, we'd end up with a garbled
			// folder name created!

			// So, for now, default to executable directory!
			wsprintf(szFilePath,GetExeDir());
		} else {
			// FIXME
			// Note that the following is not very good as it doesn't select
			// a suitable directory for non-English systems.
			wsprintf(szFilePath, "%s%s", szUserProfile,"\\Application Data\\JPEGsnoop");

			// Since the full path may not already exist, and CreateDirectory() only
			// works on one level of hierarchy at a time, use a recursive function
			// to do the job.
			CreateDir(szFilePath);
		}
	}
	return szFilePath;
}

CString CSnoopConfig::GetExeDir()
{
	char szPath[1024];
	GetModuleFileName(AfxGetApp()->m_hInstance, szPath, sizeof(szPath));
	
	char *ptr = strrchr(szPath, '\\');
	
	if (ptr)
	{
		ptr[1] = '\0';
	}
	
	ASSERT(strlen(szPath) < sizeof(szPath));
	
	return szPath;
}

// Recursively create directories as needed to create full path
void CSnoopConfig::CreateDir(char* Path)
{
	char DirName[256];
	char* p = Path;
	char* q = DirName; 
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

