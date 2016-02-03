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

// ==========================================================================
// CLASS DESCRIPTION:
// - Application configuration structures
// - Registry management
// - Note that the majority of this class is defined with public members
//
// ==========================================================================

#include "snoop.h"	// For DEBUG_LOG_OUT

#pragma once

class CSnoopConfig
{
public:
	CSnoopConfig(void);
	~CSnoopConfig(void);

	void		UseDefaults();

	void		RegistryLoadStr(CString strKey,CString strDefault,CString &strSetting);
	void		RegistryLoadBool(CString strKey,unsigned nDefault,bool &bSetting);
	void		RegistryLoadUint(CString strKey,unsigned nDefault,unsigned &nSetting);
	void		RegistryStoreStr(CString strKey,CString strSetting);
	void		RegistryStoreBool(CString strKey,bool bSetting);
	void		RegistryStoreUint(CString strKey,unsigned nSetting);

	void		RegistryLoad();
	void		RegistryStore();
	void		Dirty(bool mode=true);

	void		CoachReset();		// Reset all coach messages

	CString		GetDefaultDbDir();	// Public use by CSettingsDlg

public:
	// Debug Log
	// - Used if DEBUG_LOG_OUT
	bool		DebugLogCreate();
	bool		DebugLogAdd(CString strText);

private:
	CString		GetExeDir();
	void		CreateDir(LPTSTR Path);


public:

	// Interactive mode: shows message dialog box alerts
	// In non-interactive mode we suppress most alert dialogs but still
	// show them in certain circumstances:
	// - They involve an interactive user function (eg. another dialog box)
	// - They indicate a critical error
	bool		bInteractive;			// Do we want user input (interactive mode)? (ie. show message boxes)

	bool		bGuiMode;			// Do we want to show the GUI?
	bool		bCmdLineOpenEn;			// input file specified
	CString		strCmdLineOpenFname;		// input filename
	bool		bCmdLineOutputEn;			// output file specified?
	CString		strCmdLineOutputFname;	// output filename

	bool		bCmdLineBatchEn;			// Do we run a batch job on a directory?
	CString		strCmdLineBatchDirName;	// directory path for batch job
	bool		bCmdLineBatchRec;		// recursive subdir batch operation?

	bool		bCmdLineBatchSrch;		// In batch process, perform search forward first?

	bool		bCmdLineExtractEn;		// Do we extract all JPEGs from file?
	bool		bCmdLineExtractDhtAvi;	// Do we force MotionJPEG DHT?

	bool		bCmdLineDoneMsg;		// Indicate to user when command-line operations complete?

	teOffsetMode	eCmdLineOffset;		// Offset operating mode
	unsigned long	nCmdLineOffsetPos;	// File offset for DEC_OFFSET_POS mode

	bool		bCmdLineHelp;			// Show command list

	unsigned	nPosStart;				// Starting decode file offset

	// Operating system
    bool		bIsWindowsNTorLater;
    bool		bIsWindowsXPorLater;

	// Registry Configuration options
	bool		bDirty;					// Registry entry options dirtied?
	bool		bEulaAccepted;			// Accepted the EULA?
	bool		bUpdateAuto;			// Automatically check for updates
	unsigned	nUpdateAutoDays;		// How many days between checks
	CString		strDbDir;				// Directory for User DB
	CString		strUpdateLastChk;		// When last checked for updates
	bool		bReprocessAuto;			// Auto reprocess file when option change
	bool		bSigSearch;				// Automatically search for comp signatures
	bool		bDecodeScanImg;			// Scan image decode enabled
	bool		bDecodeScanImgAc;		// When scan image decode, do full AC
	bool		bOutputScanDump;		// Do we dump a portion of scan data?
	bool		bOutputDHTexpand;
	bool		bDecodeMaker;
	bool		bHistoEn;				// Histogram calcs enabled?
	bool		bStatClipEn;			// Enable scan decode clip stats?
	bool		bDumpHistoY;			// Dump full Y DC Histogram
	bool		bDbSubmitNet;			// Submit new entries to net?
	bool		bExifHideUnknown;		// Hide unknown exif tags?
	bool		bRelaxedParsing;		// Proceed despite bad marker / format?

	unsigned	nErrMaxDecodeScan;		// Max # errs to show in scan decode

	CString		strBatchLastInput;		// Last batch process input directory
	CString		strBatchLastOutput;		// Last batch process output directory
	CString		strBatchExtensions;		// Extension list for batch processing (eg. ".jpg,.jpeg")

	bool		bCoachReprocessAuto;	// Coach msg: Need to reprocess or change to auto
	bool		bCoachDecodeIdct;		// Coach msg: Warn about slow AC decode / lowres DC

	// Extra config (not in registry)
	bool		bDecodeColorConvert;	// Do we do color convert after scan decode?

	// Temporary status (not saved)
	CString		strCurFname;			// Current filename (Debug use only)

	// Debug log
	// - Used if DEBUG_LOG_OUT
	bool		bDebugLogEnable;
	CString		strDebugLogFname;
	CStdioFile*	fpDebugLog;

};
