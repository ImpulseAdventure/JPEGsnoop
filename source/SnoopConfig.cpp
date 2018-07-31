// JPEGsnoop - JPEG Image Decoder & Analysis Utility
// Copyright (C) 2017 - Calvin Hass
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

#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>

#include "eula.h"
#include "JfifDecode.h"
#include "note.h"
#include "snoop.h"

#include "SnoopConfig.h"

//-----------------------------------------------------------------------------
// Define the defaults for the application configuration
CSnoopConfig::CSnoopConfig(QObject *_parent) : QObject(_parent)
{
  // Debug log
  strDebugLogFname = ".\\JPEGsnoop-debug.log";
  fpDebugLog = nullptr;
  bDebugLogEnable = false;

  // Default to showing message dialogs
  bInteractive = true;

  // Command-line modes
  bGuiMode = true;
  bCmdLineOpenEn = false;
  strCmdLineOpenFname = "";
  bCmdLineOutputEn = false;     // No output file specified
  strCmdLineOutputFname = "";
  bCmdLineBatchEn = false;
  strCmdLineBatchDirName = "";
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
  bDirty = false;               // Have the registry options been dirtied?
  bEulaAccepted = false;
  bUpdateAuto = true;
  nUpdateAutoDays = 14;         // Check every 14 days
  strUpdateLastChk = "";
  strDbDir = "";
  bReprocessAuto = false;
  bDecodeScanImg = true;
  bDecodeScanImgAc = false;     // Coach message will be shown just in case
  bSigSearch = true;

  bOutputScanDump = false;      // Print snippet of scan data
  bOutputDHTexpand = false;     // Print expanded huffman tables
  bDecodeMaker = false;
  bDumpHistoY = false;

  // Difference in performance: 1dsmk2 image:
  // Performance boost ~ 25%
  bHistoEn = false;             // Histogram & clipping stats enabled?
  bStatClipEn = false;          // UNUSED: Enable Scan Decode clip stats?

  bDbSubmitNet = true;          // Default to submit new DB entries to web

  bExifHideUnknown = true;      // Default to hiding unknown EXIF tags
  bRelaxedParsing = false;      // Normal parsing stops on bad marker

  nErrMaxDecodeScan = 20;

  strBatchLastInput = "C:\\";   // Default batch processing directory (input
  strBatchLastOutput = "C:\\";  // Default batch processing directory (output
  strBatchExtensions = ".jpg,.jpeg";    // Default batch extension list

  bDecodeColorConvert = true;   // Perform color convert after scan decode

  // Reset coach message flags
  CoachReset();

  strReprocess = "You have changed a processing option. To see these changes, you need to Reprocess the file or enable [Auto Reprocessing] in Configuration.";

  // Reset the current filename
  strCurFname = "";

  qDebug() << "CSnoopConfig::CSnoopConfig End";
}

//-----------------------------------------------------------------------------
CSnoopConfig::~CSnoopConfig(void)
{
}

//-----------------------------------------------------------------------------
// This is generally called after app initializes and registry has just been loaded.
void CSnoopConfig::UseDefaults()
{
  // Assume all settings are to their default values
  // But some need copying over
}

//-----------------------------------------------------------------------------
// Update the flag to mark the configuration as dirty / modified
// - This is used later to indicate that a resave is required
//
void CSnoopConfig::Dirty(bool mode)
{
  bDirty = mode;
}

//-----------------------------------------------------------------------------
// Fetch all configuration values from the registry
void CSnoopConfig::RegistryLoad()
{
  QSettings settings;
  
  QString regUserDbPath = settings.value("General\\UserDbPath", "").toString();

  // Try to load the registry entry
  if(regUserDbPath == "")
  {
    // None specified, so determine a suitable default
    strDbDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    QDir appDir(strDbDir);

    if(!appDir.exists())
    {
      appDir.mkpath(strDbDir);
    }

    bDirty = true;
  }
  else
  {
    // Specified, so store in App Config struct
    strDbDir = regUserDbPath;
  }

  QString strCurDate = QDateTime::currentDateTime().toString("yyyymmdd");
  QString regUpdateLastChk = settings.value("General\\UpdateLastChk", "").toString();

  // Try to load the registry entry
  if(regUpdateLastChk == "")
  {
    // None specified, so determine a suitable default
    strUpdateLastChk = strCurDate;
    bDirty = true;
  }
  else
  {
    // Specified, so store in App Config struct
    strUpdateLastChk = regUpdateLastChk;
  }

  bUpdateAuto = settings.value("General\\UpdateAuto", true).toBool();
  nUpdateAutoDays = settings.value("General\\UpdateAutoDays", 14).toInt();
  bEulaAccepted = settings.value("General\\EulaAccepted", false).toBool();
  bReprocessAuto = settings.value("General\\ReprocessAuto", false).toBool();
  bSigSearch = settings.value("General\\SigSearch", true).toBool();
  bDecodeScanImg = settings.value("General\\DecScanImg", false).toBool();
  bDecodeScanImgAc = settings.value("General\\DecScanImgAc", false).toBool();

  bOutputScanDump = settings.value("General\\DumpScan", false).toBool();
  bOutputDHTexpand = settings.value("General\\DumpDHTExpand", false).toBool();
  bDecodeMaker = settings.value("General\\DecMaker", false).toBool();
  bDumpHistoY = settings.value("General\\DumpHistoY", false).toBool();
//      RegistryLoadBool("General\\DecScanClip"),    999,   bStatClipEn;
  bHistoEn = settings.value("General\\DecScanHisto", false).toBool();

  bDbSubmitNet = settings.value("General\\DbSubmitNet", true).toBool();
  bExifHideUnknown = settings.value("General\\ExifHideUnk", true).toBool();
  bRelaxedParsing = settings.value("General\\RelaxedParsing", false).toBool();

  nErrMaxDecodeScan = settings.value("General\\ErrMaxDecodeScan", 20).toInt();

  strBatchLastInput = settings.value("General\\BatchLastInput", "").toString();
  strBatchLastOutput = settings.value("General\\BatchLastOutput", "").toString();
  strBatchExtensions = settings.value("General\\BatchExtensions", "").toString();

  bCoachReprocessAuto = settings.value("Coach\\ReprocessAuto", false).toBool();
  bCoachDecodeIdct = settings.value("Coach\\DecodeIdct", false).toBool();

  if(bHistoEn || bDumpHistoY || bDecodeScanImgAc)
  {
    docImageDirty();
  }

  bEulaAccepted = false;

  if(!bEulaAccepted)
  {
    Q_Eula *eulaDlg = new Q_Eula;
    eulaDlg->exec();

    if(eulaDlg->result())
    {
      bEulaAccepted = true;
      bUpdateAuto = eulaDlg->updateStatus();
      bDirty = true;
      RegistryStore();
    }
    else
    {
      exit(1);
    }
  }

  printSettings();

  // Deprecated Registry strings
  // FIXME: Later on, decide if we should delete deprecated strings if found
  // "General\\ScanImgDec"
  // "General\\ScanImgDecDef"
  // "General\\ScanPreviewDef"
  // "General\\SigSearchAuto"
}

//-----------------------------------------------------------------------------
// Write all configuration parameters to the registry
// - Skip write if no changes were made (!bDirty)
//
void CSnoopConfig::RegistryStore()
{
  QSettings settings;
  
  // If no changes made, skip
  if(bDirty)
  {
    qDebug() << "Saving Config";
    settings.setValue("General\\UserDbPath", strDbDir);
    settings.setValue("General\\UpdateAuto", bUpdateAuto);
    settings.setValue("General\\UpdateAutoDays", nUpdateAutoDays);
    settings.setValue("General\\UpdateLastChk", strUpdateLastChk);
    settings.setValue("General\\EulaAccepted", bEulaAccepted);
    settings.setValue("General\\ReprocessAuto", bReprocessAuto);
    settings.setValue("General\\SigSearch", bSigSearch);
    settings.setValue("General\\DecScanImg", bDecodeScanImg);
    settings.setValue("General\\DecScanImgAc", bDecodeScanImgAc);

    settings.setValue("General\\DumpScan", bOutputScanDump);
    settings.setValue("General\\DumpDHTExpand", bOutputDHTexpand);
    settings.setValue("General\\DecMaker", bDecodeMaker);
    settings.setValue("General\\DumpHistoY", bDumpHistoY);
    settings.setValue("General\\DecScanHisto", bHistoEn);

    settings.setValue("General\\DbSubmitNet", bDbSubmitNet);
    settings.setValue("General\\ExifHideUnk", bExifHideUnknown);
    settings.setValue("General\\RelaxedParsing", bRelaxedParsing);

    settings.setValue("Report\\ErrMaxDecodeScan", nErrMaxDecodeScan);

    settings.setValue("General\\BatchLastInput", strBatchLastInput);
    settings.setValue("General\\BatchLastOutput", strBatchLastOutput);
    settings.setValue("General\\BatchExtensions", strBatchExtensions);

    settings.setValue("Coach\\ReprocessAuto", bCoachReprocessAuto);
    settings.setValue("Coach\\DecodeIdct", bCoachDecodeIdct);

    // Registry entries are no longer dirty
    bDirty = false;
  }
}

//-----------------------------------------------------------------------------
void CSnoopConfig::printSettings()
{
  qDebug() << "strDbDir" << strDbDir;
  qDebug() << "bUpdateAuto" << bUpdateAuto;
  qDebug() << "nUpdateAutoDays" << nUpdateAutoDays;
  qDebug() << "strUpdateLastChk" << strUpdateLastChk;
  qDebug() << "bEulaAccepted" << bEulaAccepted;
  qDebug() << "bReprocessAuto" << bReprocessAuto;
  qDebug() << "bSigSearch" << bSigSearch;
  qDebug() << "bDecodeScanImg" << bDecodeScanImg;
  qDebug() << "bDecodeScanImgAc" << bDecodeScanImgAc;

  qDebug() << "bOutputScanDump" << bOutputScanDump;
  qDebug() << "bOutputDHTexpand" << bOutputDHTexpand;
  qDebug() << "bDecodeMaker" << bDecodeMaker;
  qDebug() << "bDumpHistoY" << bDumpHistoY;
  qDebug() << "bHistoEn" << bHistoEn;

  qDebug() << "bDbSubmitNet" << bDbSubmitNet;
  qDebug() << "bExifHideUnknown" << bExifHideUnknown;
  qDebug() << "bRelaxedParsing" << bRelaxedParsing;

  qDebug() << "nErrMaxDecodeScan" << nErrMaxDecodeScan;

  qDebug() << "strBatchLastInput" << strBatchLastInput;
  qDebug() << "strBatchLastOutput" << strBatchLastOutput;
  qDebug() << "strBatchExtensions" << strBatchExtensions;

  qDebug() << "bCoachReprocessAuto" << bCoachReprocessAuto;
  qDebug() << "bCoachDecodeIdct" << bCoachDecodeIdct;

}

//-----------------------------------------------------------------------------
// Reset coach messages
void CSnoopConfig::CoachReset()
{
  // Enable all coach messages
  bCoachReprocessAuto = true;
  bCoachDecodeIdct = true;

  Dirty();
}

//-----------------------------------------------------------------------------
// Select a suitable working directory, create it if it doesn't already exist
//QString CSnoopConfig::GetDefaultDbDir()
//{

/*	TCHAR szUserProfile[MAX_PATH];
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
			PathAppend( szFilePath, "\\JPEGsnoop") ;

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

		ExpandEnvironmentStrings("%userprofile%"),szUserProfile,sizeof(szUserProfile)/sizeof(*szUserProfile);

		// NOTE:
		// - In Win95/98 it is likely that the above will return the literal "%userprofile%"
		//   so in that case we don't want to create a directory -- use the exe dir instead
		if (!_tcscmp(szUserProfile,"%userprofile%")) {
			wsprintf(szFilePath, "");
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
			wsprintf(szFilePath, "%s%s"), szUserProfile,_T("\\Application Data\\JPEGsnoop");

			// Since the full path may not already exist, and CreateDirectory() only
			// works on one level of hierarchy at a time, use a recursive function
			// to do the job.
			CreateDir(szFilePath);
		}
	}
  return szFilePath; */
//}

// Get the directory path of the JPEGsnoop executable
// - This is used as a default location for the user signature database
QString CSnoopConfig::GetExeDir()
{

/*	TCHAR szPath[1024];
	GetModuleFileName(AfxGetApp()->m_hInstance, szPath, sizeof(szPath)/sizeof(*szPath));

	TCHAR *ptr = _tcsrchr(szPath, '\\');

	if (ptr)
	{
		ptr[1] = '\0';
	}

	ASSERT(_tcslen(szPath) < sizeof(szPath));

  return szPath; */
    return QString(); // FIXME temporary fix for C4716
}

// Recursively create directories as needed to create full path
// TODO: Should replace this by SHCreateDirectoryEx()
void CSnoopConfig::CreateDir(QString Path)
{

/*	TCHAR DirName[256];
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
  CreateDirectory(DirName, NULL); */
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

  if(strDebugLogFname == "")
  {
    return false;
  }

  // Detect control keys
  // - VK_CONTROL = Control key
  // - VK_SHIFT   = Shift
  // - VK_MENU    = Alt?
  if(GetAsyncKeyState(VK_CONTROL))
  {
    AfxMessageBox("Debug Log Enabled");
    bDebugLogEnable = true;
  }
  else
  {
    return true;
  }

  ASSERT(fpDebugLog == nullptr);

  try
  {
    // Open specified file
    fpDebugLog = new CStdioFile(strDebugLogFname, CFile::modeCreate | CFile::modeWrite | CFile::typeText | CFile::shareDenyNone);
  }
  catch(CFileException * e)
  {
    TCHAR msg[MAX_BUF_EX_ERR_MSG];

    QString strError;

    e->GetErrorMessage(msg, MAX_BUF_EX_ERR_MSG);
    e->Delete();
    strError.Format("ERROR: Couldn't open debug log file for write [%s]: [%s]", (LPCTSTR) strDebugLogFname, (LPCTSTR) msg);
    AfxMessageBox(strError);
    fpDebugLog = nullptr;

    return false;
  }

  QString strLine;

  strLine.Format("JPEGsnoop version [%s]\r\n"), VERSION_STR;
  fpDebugLog->WriteString(strLine);
  fpDebugLog->WriteString("-----\r\n");

  // Close the file
  if(fpDebugLog)
  {
    fpDebugLog->Close();
    delete fpDebugLog;

    fpDebugLog = nullptr;
  }

  // Extra code to record OS version
  OSVERSIONINFO osvi;

  ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  GetVersionEx(&osvi);
  strLine.Format("OS: PlatformId = %u"), osvi.dwPlatformId;
  if(DEBUG_EN)
    DebugLogAdd(strLine);
  strLine.Format("OS: MajorVersion = %u"), osvi.dwMajorVersion;
  if(DEBUG_EN)
    DebugLogAdd(strLine);
  strLine.Format("OS: MinorVersion = %u"), osvi.dwMinorVersion;
  if(DEBUG_EN)
    DebugLogAdd(strLine);
  strLine.Format("OS: BuildNumber = %u"), osvi.dwBuildNumber;
  if(DEBUG_EN)
    DebugLogAdd(strLine);
  strLine.Format("OS: OSVersionInfoSize = %u"), osvi.dwOSVersionInfoSize;
  if(DEBUG_EN)
    DebugLogAdd(strLine);
  if(DEBUG_EN)
    DebugLogAdd("\n");

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
bool CSnoopConfig::DebugLogAdd(QString strText)
{
#ifdef DEBUG_LOG_OUT

  if(!bDebugLogEnable)
  {
    return true;
  }

  if(strDebugLogFname == "")
  {
    return false;
  }

  ASSERT(fpDebugLog == nullptr);

  try
  {
    // Open specified file but append
    fpDebugLog = new CStdioFile(strDebugLogFname, CFile::modeCreate | CFile::modeWrite | CFile::typeText | CFile::shareDenyNone |
                                CFile::modeNoTruncate);
  }
  catch(CFileException * e)
  {
    TCHAR msg[MAX_BUF_EX_ERR_MSG];

    QString strError;

    e->GetErrorMessage(msg, MAX_BUF_EX_ERR_MSG);
    e->Delete();
    strError.Format("ERROR: Couldn't open debug log file for append [%s]: [%s]", (LPCTSTR) strDebugLogFname, (LPCTSTR) msg);
    AfxMessageBox(strError);
    fpDebugLog = nullptr;

    return false;
  }

  // Now seek to the end of the file
  fpDebugLog->SeekToEnd();

  QString strLine;

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
	//strLine.Format("[%4u/%2u/%2u %2u:%2u:%2u] %s\r\n"),nTmYear,nTmMon,nTmDay,nTmHour,nTmMin,nTmSec,strText;
*/
  strLine.Format("%s\n"), strText;
  fpDebugLog->WriteString(strLine);

  // Close the file
  if(fpDebugLog)
  {
    fpDebugLog->Close();
    delete fpDebugLog;

    fpDebugLog = nullptr;
  }

#endif
  return true;
}

//-----------------------------------------------------------------------------

void CSnoopConfig::docImageDirty()
{
  emit ImgSrcChanged();
}

//-----------------------------------------------------------------------------
// If Automatic Reprocessing is enabled, reprocess the file
// - If Reprocessing is not enabled and the coach message has
//   not been disabled, inform user of option to enable
void CSnoopConfig::HandleAutoReprocess()
{
  if(bReprocessAuto)
  {
    emit reprocess();
  }
  else
  {
    if(bCoachReprocessAuto)
    {
      // Show the coaching dialog
      Q_Note *note = new Q_Note(strReprocess);
      note->exec();
      bCoachReprocessAuto = !note->hideStatus();
      Dirty();
    }
  }
}

//-----------------------------------------------------------------------------

void CSnoopConfig::onOptionsDhtexpand()
{
  if(bOutputDHTexpand)
  {
    bOutputDHTexpand = false;
  }
  else
  {
    bOutputDHTexpand = true;
  }

  Dirty();
  HandleAutoReprocess();
}

//-----------------------------------------------------------------------------

void CSnoopConfig::onOptionsMakernotes()
{
  if(bOutputDHTexpand)
  {
    bDecodeMaker = false;
  }
  else
  {
    bDecodeMaker = true;
  }

  Dirty();
  HandleAutoReprocess();
}

//-----------------------------------------------------------------------------

void CSnoopConfig::onOptionsHideuknownexiftags()
{
  if(bExifHideUnknown)
  {
    bExifHideUnknown = false;
  }
  else
  {
    bExifHideUnknown = true;
  }

  Dirty();
  HandleAutoReprocess();
}

//-----------------------------------------------------------------------------

void CSnoopConfig::onOptionsSignaturesearch()
{
  if(bSigSearch)
  {
    bSigSearch = false;
  }
  else
  {
    bSigSearch = true;
  }

  Dirty();
  HandleAutoReprocess();
}

//-----------------------------------------------------------------------------

void CSnoopConfig::onOptionsRelaxedparsing()
{
  if(bRelaxedParsing)
  {
    bRelaxedParsing = false;
  }
  else
  {
    bRelaxedParsing = true;
  }

  Dirty();
  HandleAutoReprocess();
}

//-----------------------------------------------------------------------------

void CSnoopConfig::onScansegmentDecodeImage()
{
  if(bDecodeScanImg)
  {
    bDecodeScanImg = false;
  }
  else
  {
    bDecodeScanImg = true;
  }

  Dirty();
  HandleAutoReprocess();
}

//-----------------------------------------------------------------------------

void CSnoopConfig::onScansegmentNoidct()
{
  if(bDecodeScanImgAc)
  {
    bDecodeScanImgAc = false;
  }

  emit scanImageAc(bDecodeScanImgAc);
  Dirty();
  docImageDirty();
  HandleAutoReprocess();
}

//-----------------------------------------------------------------------------

void CSnoopConfig::onScansegmentFullidct()
{
  if(!bDecodeScanImgAc)
  {
    bDecodeScanImgAc = true;
  }

  emit scanImageAc(bDecodeScanImgAc);
  Dirty();
  docImageDirty();
  HandleAutoReprocess();
}

//-----------------------------------------------------------------------------

void CSnoopConfig::onScansegmentHistogram()
{
  if(bHistoEn)
  {
    bHistoEn = false;
  }
  else
  {
    bHistoEn = true;
  }

  Dirty();

  docImageDirty();
  HandleAutoReprocess();
}

//-----------------------------------------------------------------------------

void CSnoopConfig::onScansegmentHistogramy()
{
  if(bDumpHistoY)
  {
    bDumpHistoY = false;
  }
  else
  {
    bDumpHistoY = true;
  }

  Dirty();

  docImageDirty();
  HandleAutoReprocess();
}

//-----------------------------------------------------------------------------

void CSnoopConfig::onScansegmentDump()
{
  if(bOutputScanDump)
  {
    bOutputScanDump = false;
  }
  else
  {
    bOutputScanDump = true;
  }

  Dirty();
  HandleAutoReprocess();
}
