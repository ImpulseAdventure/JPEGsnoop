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

#pragma once

class CSnoopConfig
{
public:
	CSnoopConfig(void);
	~CSnoopConfig(void);

	void	UseDefaults();

	void	RegistryLoadStr(CString strKey,CString strDefault,CString &strSetting);
	void	RegistryLoadBool(CString strKey,unsigned nDefault,bool &bSetting);
	void	RegistryLoadUint(CString strKey,unsigned nDefault,unsigned &nSetting);
	void	RegistryStoreStr(CString strKey,CString strSetting);
	void	RegistryStoreBool(CString strKey,bool bSetting);
	void	RegistryStoreUint(CString strKey,unsigned nSetting);

	void	RegistryLoad();
	void	RegistryStore();
	void	Dirty(bool mode=true);

	void	CoachReset();		// Reset all coach messages

	CString	GetDefaultDbDir();	// Public use by CSettingsDlg

private:
	CString	GetExeDir();
	void	CreateDir(char* Path);


public:

	bool		cmdline_gui;			// Do we open GUI?
	bool		cmdline_open;			// input file specified
	CString		cmdline_open_fname;		// input filename
	bool		cmdline_output;			// output file specified?
	CString		cmdline_output_fname;	// output filename

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

	unsigned	nErrMaxDecodeScan;		// Max # errs to show in scan decode

	bool		bCoachReprocessAuto;	// Coach msg: Need to reprocess or change to auto
	bool		bCoachDecodeIdct;		// Coach msg: Warn about slow AC decode / lowres DC

	// Extra config (not in registry)
	bool		bDecodeColorConvert;	// Do we do color convert after scan decode?
};
