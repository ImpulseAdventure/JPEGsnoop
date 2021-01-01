// JPEGsnoop - JPEG Image Decoder & Analysis Utility
// Copyright (C) 2018 - Calvin Hass
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

#ifndef SNOOPCONFIG_H
#define SNOOPCONFIG_H

#include "snoop.h"              // For DEBUG_LOG_OUT

#include <QSettings>
#include <QTextStream>

class CjfifDecode;

class CSnoopConfig : public QObject
{
  Q_OBJECT

public:
  CSnoopConfig(QObject *_parent = 0);
  ~CSnoopConfig(void);

  void UseDefaults();
  void printSettings();

  void RegistryLoad();
  void RegistryStore();
  void Dirty(bool mode = true);

  void CoachReset();            // Reset all coach messages

//  QString GetDefaultDbDir();    // Public use by CSettingsDlg

  // Getter functions
  QString dbPath() { return strDbDir; }

  int32_t autoUpdateDays() { return nUpdateAutoDays; }
  int32_t maxDecodeError() { return nErrMaxDecodeScan; }

  uint32_t startPos() { return nPosStart; }

  bool autoReprocess() { return bReprocessAuto; }
  bool clipStats() { return bStatClipEn; }
  bool coachIdct() { return bCoachDecodeIdct; }
  bool dbSubmitNet() { return bDbSubmitNet; }
  bool decodeAc() { return bDecodeScanImgAc; }
  bool decodeImage() { return bDecodeScanImg; }
  bool decodeMaker() { return bDecodeMaker; }
  bool displayRgbHistogram() { return bHistoEn; }
  bool displayYHistogram() { return bDumpHistoY; }
  bool expandDht() { return bOutputDHTexpand; }
  bool hideUnknownExif() { return bExifHideUnknown; }
  bool interactive() { return bInteractive; }
  bool relaxedParsing() { return bRelaxedParsing; }
  bool scanDump() { return bOutputScanDump; }
  bool searchSig() { return bSigSearch; }
  bool submitDbNet() { return bDbSubmitNet; }
  bool swUpdateEnabled() { return bUpdateAuto; }

  // Setter functions
  void setDbPath(QString path) {strDbDir = path; }
  void setAutoUpdate(bool b) { bUpdateAuto = b; }
  void setUpdatePeriod(int32_t days) {nUpdateAutoDays = days; }
  void setAutoReprocess(bool b) { bReprocessAuto = b; }
  void setOnlineDb(bool b) { bDbSubmitNet = b; }
  void setMaxErrors(int32_t errors) { nErrMaxDecodeScan = errors; }
  void setStartPos(uint32_t start) { nPosStart = start; }
  void setCoachIdct(bool b) { bCoachDecodeIdct = b; }

  // Debug Log
  // - Used if DEBUG_LOG_OUT
  bool DebugLogCreate();
  bool DebugLogAdd(QString strText);

  // Temporary status (not saved)
  QString strCurFname;          // Current filename (Debug use only)

  // TODO Move tp commandline processing
  teOffsetMode eCmdLineOffset;  // Offset operating mode
  unsigned long nCmdLineOffsetPos;      // File offset for DEC_OFFSET_POS mode

signals:
  void ImgSrcChanged();
  void scanImageAc(bool);
  void reprocess();

public slots:
  void onOptionsDhtexpand();
  void onOptionsMakernotes();
  void onScansegmentDecodeImage();
  void onOptionsHideuknownexiftags();
  void onOptionsRelaxedparsing();
  void onOptionsSignaturesearch();
  void onScansegmentNoidct();
  void onScansegmentFullidct();
  void onScansegmentHistogram();
  void onScansegmentHistogramy();
  void onScansegmentDump();

private:
  CSnoopConfig &operator = (const CSnoopConfig&);
  CSnoopConfig(CSnoopConfig&);

  QString GetExeDir();
  void CreateDir(QString Path);
  void docImageDirty();
  void HandleAutoReprocess();

  CjfifDecode *m_pJfifDec;

  // Interactive mode: shows message dialog box alerts
  // In non-interactive mode we suppress most alert dialogs but still
  // show them in certain circumstances:
  // - They involve an interactive user function (eg. another dialog box)
  // - They indicate a critical error
  bool bInteractive;          // Do we want user input (interactive mode)? (ie. show message boxes)

  bool bGuiMode;                // Do we want to show the GUI?
  bool bCmdLineOpenEn;          // input file specified
  QString strCmdLineOpenFname;  // input filename
  bool bCmdLineOutputEn;        // output file specified?
  QString strCmdLineOutputFname;        // output filename

  bool bCmdLineBatchEn;         // Do we run a batch job on a directory?
  QString strCmdLineBatchDirName;       // directory path for batch job
  bool bCmdLineBatchRec;        // recursive subdir batch operation?

  bool bCmdLineBatchSrch;       // In batch process, perform search forward first?

  bool bCmdLineExtractEn;       // Do we extract all JPEGs from file?
  bool bCmdLineExtractDhtAvi;   // Do we force MotionJPEG DHT?

  bool bCmdLineDoneMsg;         // Indicate to user when command-line operations complete?

  bool bCmdLineHelp;            // Show command list

  unsigned nPosStart;           // Starting decode file offset

  // Operating system
  bool bIsWindowsNTorLater;
  bool bIsWindowsXPorLater;

  // Registry Configuration options
  QString strDbDir;             // Directory for User DB
  QString strUpdateLastChk;     // When last checked for updates
  QString strBatchLastInput;    // Last batch process input directory
  QString strBatchLastOutput;   // Last batch process output directory
  QString strBatchExtensions;   // Extension list for batch processing (eg. ".jpg,.jpeg")

  int nUpdateAutoDays;          // How many days between checks
  int nErrMaxDecodeScan;        // Max # errs to show in scan decode

  bool bDirty;                  // Registry entry options dirtied?
  bool bEulaAccepted;           // Accepted the EULA?
  bool bUpdateAuto;             // Automatically check for updates
  bool bReprocessAuto;          // Auto reprocess file when option change
  bool bSigSearch;              // Automatically search for comp signatures
  bool bDecodeScanImg;          // Scan image decode enabled
  bool bDecodeScanImgAc;        // When scan image decode, do full AC
  bool bOutputScanDump;         // Do we dump a portion of scan data?
  bool bOutputDHTexpand;
  bool bDecodeMaker;
  bool bHistoEn;                // Histogram calcs enabled?
  bool bStatClipEn;             // Enable scan decode clip stats?
  bool bDumpHistoY;             // Dump full Y DC Histogram
  bool bDbSubmitNet;            // Submit new entries to net?
  bool bExifHideUnknown;        // Hide unknown exif tags?
  bool bRelaxedParsing;         // Proceed despite bad marker / format?
  bool bCoachReprocessAuto;     // Coach msg: Need to reprocess or change to auto
  bool bCoachDecodeIdct;        // Coach msg: Warn about slow AC decode / lowres DC

  // Coach Message
  QString strReprocess;

  // Extra config (not in registry)
  bool bDecodeColorConvert;     // Do we do color convert after scan decode?

  // Debug log
  // - Used if DEBUG_LOG_OUT
  bool bDebugLogEnable;
  QString strDebugLogFname;
  QTextStream *fpDebugLog;
};

#endif
