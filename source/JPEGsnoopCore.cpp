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

#include <QDir>

//#include "stdafx.h"
#include "JPEGsnoopCore.h"

// For glb_pDocLog
#include "JPEGsnoop.h"

CJPEGsnoopCore::CJPEGsnoopCore(void)
{
  if(DEBUG_EN)
    m_pAppConfig->DebugLogAdd("CJPEGsnoopCore::CJPEGsnoopCore() Begin");
  // Initialize processing classes

  // Save a local copy of the config struct pointer
//@@  m_pAppConfig = theApp.m_pAppConfig;

  // Ensure the local log isn't linked to a CDocument
//  glb_pDocLog->SetDoc(NULL);

  if(DEBUG_EN)
    m_pAppConfig->DebugLogAdd("CJPEGsnoopCore::CJPEGsnoopCore() Checkpoint 1");

  // Allocate the file window buffer
  m_pWBuf = new CwindowBuf();

  if(!m_pWBuf)
  {
    msgBox.setText("ERROR: Not enough memory for File Buffer");
    exit(1);
  }

  if(DEBUG_EN)
    m_pAppConfig->DebugLogAdd("CJPEGsnoopCore::CJPEGsnoopCore() Checkpoint 2");

  // Allocate the JPEG decoder
//  m_pImgDec = new CimgDecode(glb_pDocLog, m_pWBuf);

  if(!m_pWBuf)
  {
    msgBox.setText("ERROR: Not enough memory for Image Decoder");
    exit(1);
  }

  if(DEBUG_EN)
    m_pAppConfig->DebugLogAdd("CJPEGsnoopCore::CJPEGsnoopCore() Checkpoint 3");

  m_pJfifDec = new CjfifDecode(glb_pDocLog, m_pWBuf, m_pImgDec);

  if(!m_pWBuf)
  {
    msgBox.setText("ERROR: Not enough memory for JFIF Decoder");
    exit(1);
  }

  if(DEBUG_EN)
    m_pAppConfig->DebugLogAdd("CJPEGsnoopCore::CJPEGsnoopCore() Checkpoint 4");

  // Reset all members
  Reset();

  if(DEBUG_EN)
    m_pAppConfig->DebugLogAdd("CJPEGsnoopCore::CJPEGsnoopCore() Checkpoint 5");

  if(DEBUG_EN)
    m_pAppConfig->DebugLogAdd("CJPEGsnoopCore::CJPEGsnoopCore() End");
}

CJPEGsnoopCore::~CJPEGsnoopCore(void)
{
  if(m_pJfifDec != NULL)
  {
    delete m_pJfifDec;
    m_pJfifDec = NULL;
  }

  if(m_pWBuf != NULL)
  {
    delete m_pWBuf;
    m_pWBuf = NULL;
  }

  if(m_pImgDec != NULL)
  {
    delete m_pImgDec;
    m_pImgDec = NULL;
  }
}

// Reset all state
void CJPEGsnoopCore::Reset()
{
  // Reset all members
  m_pFile = NULL;
  m_lFileSize = 0L;
  m_strPathName = "";

  // No log data available until we open & process a file
  m_bFileOpened = false;
  m_bFileAnalyzed = false;

  // FIXME: Should we be calling Reset on the JfifDecoder, ImgDecoder and WBuf?

  // Indicate to JFIF ProcessFile() that document has changed
  // and that the scan decode needs to be redone if it
  // is to be displayed.
  m_pJfifDec->ImgSrcChanged();

  // Clean up the quick log
  glb_pDocLog->Clear();
}

void CJPEGsnoopCore::SetStatusBar(QStatusBar * pStatBar)
{
  // Save a copy of the status bar
  //xxx m_pStatBar = pStatBar;

  // Now update the JFIF decoder and Image Decoder with the
  // revised status bar pointer (or NULL)
  m_pJfifDec->SetStatusBar(pStatBar);
  m_pImgDec->SetStatusBar(pStatBar);
}

//
// Has a file been analyzed previously?
// - This indicator is used extensively in the CJPEGsnoopDoc
//   to determine whether menu items are enabled or not
//
bool CJPEGsnoopCore::IsAnalyzed()
{
  return m_bFileAnalyzed;
}

// Open the file named in m_strPathName
// - Close / cleanup if another file was already open
//
// INPUT:
// - strFname           = File to open
//
// POST:
// - m_pFile
// - m_lFileSize
// - m_pWBuf loaded
//
// RETURN:
// - Indicates if file open was successful
//
bool CJPEGsnoopCore::AnalyzeOpen()
{
  if(DEBUG_EN)
    m_pAppConfig->DebugLogAdd("CJPEGsnoopCore::AnalyzeOpen() Begin");

  Q_ASSERT(m_strPathName != "");

  if(m_strPathName == "")
  {
    // Force dialog box to show as this is a major error
    // TODO: Handle non-GUI mode
    msgBox.setText("ERROR: AnalyzeOpen() but m_strPathName empty");
    return false;
  }

  // Clean up if a file is already open
  if(m_pFile != NULL)
  {
    // Mark previous buffer as closed
    m_pWBuf->BufFileUnset();
    m_pFile->close();
    delete m_pFile;

    m_pFile = NULL;
    m_lFileSize = 0L;
  }

  // Open specified file
  // Added in shareDenyNone as this apparently helps resolve some people's troubles
  // with an error showing: Couldn't open file "Sharing Violation"

  m_pFile = new QFile(m_strPathName);

  if(m_pFile->open(QIODevice::ReadOnly) == false)
  {
    QString strError;

    // Note: msg includes m_strPathName
    strError = QString("ERROR: Couldn't open file: [%1]").arg(m_strPathName);
    glb_pDocLog->AddLineErr(strError);

    if(m_pAppConfig->bInteractive)
    {
      msgBox.setText(strError);
      msgBox.exec();
    }

    m_pFile = NULL;

    return false;
  }

  // Set the file size variable
  m_lFileSize = m_pFile->size();

  // Don't attempt to load buffer with zero length file!
  if(m_lFileSize == 0)
  {
    return true;
  }

  // Open up the buffer
  m_pWBuf->BufFileSet(m_pFile);
  m_pWBuf->BufLoadWindow(0);

  // Mark file as opened
  m_bFileOpened = true;

  if(DEBUG_EN)
    m_pAppConfig->DebugLogAdd("CJPEGsnoopCore::AnalyzeOpen() End");

  return true;
}

// Close the current file
// - Invalidate the buffer
//
// POST:
// - m_bFileOpened
//
void CJPEGsnoopCore::AnalyzeClose()
{
  // Indicate no file is currently opened
  // Note that we don't clear m_bFileAnalyzed as the last
  // decoder state is still available
  m_bFileOpened = false;

  // Close the buffer window
  m_pWBuf->BufFileUnset();

  // Now that we've finished parsing the file, close it!
  if(m_pFile != NULL)
  {
    m_pFile->close();
    delete m_pFile;

    m_pFile = NULL;
  }
}

// Perform the file analysis
// - Output the log header text
// - Process the current file via ProcessFile()
// - Dump the output to the log (if in QuickLog mode)
//
// PRE:
// - m_strPathname
//
// PRE:
// - m_bFileOpened
// - m_pFile
// - m_pJfifDec
// - m_pImgDec
// - m_lFileSize
// - m_pAppConfig->nPosStart    = Starting file offset for decode
//
// POST:
// - m_bFileAnalyzed
//
void CJPEGsnoopCore::AnalyzeFileDo()
{
  if(DEBUG_EN)
    m_pAppConfig->DebugLogAdd("CJPEGsnoopCore::AnalyzeFileDo() Checkpoint 1");

  // Ensure file was already opened
  Q_ASSERT(m_pFile);
  Q_ASSERT(m_bFileOpened);

  // Reset the analyzed state in case this file is invalid (eg. zero length)
  m_bFileAnalyzed = false;

  // Start in Quick mode (probably don't care for command-line mode)
//  glb_pDocLog->SetQuickMode(true);

  // Clear the document log
  glb_pDocLog->Clear();

  QString strTmp;

  glb_pDocLog->AddLine("");
  strTmp = QString("JPEGsnoop %1 by Calvin Hass").arg(QString(VERSION_STR));
  glb_pDocLog->AddLine(strTmp);
  glb_pDocLog->AddLine("  http://www.impulseadventure.com/photo/");
  glb_pDocLog->AddLine("  -------------------------------------");
  glb_pDocLog->AddLine("");
  strTmp = QString("  Filename: [%1]").arg(m_strPathName);
  glb_pDocLog->AddLine(strTmp);
  strTmp = QString("  Filesize: [%1] bytes").arg(m_lFileSize);
  glb_pDocLog->AddLine(strTmp);
  glb_pDocLog->AddLine("");

  // Perform the actual decoding
  if(m_lFileSize > 0xFFFFFFFFULL)
  {
    strTmp = QString("ERROR: File too large for this version of JPEGsnoop. [Size=0x%1]").arg(m_lFileSize);
    glb_pDocLog->AddLineErr(strTmp);
  }
  else if(m_lFileSize == 0)
  {
    glb_pDocLog->AddLineErr("ERROR: File length is zero, no decoding done.");
  }
  else
  {
    if(DEBUG_EN)
      m_pAppConfig->DebugLogAdd("CJPEGsnoopCore::AnalyzeFileDo() Checkpoint 2");

    m_pJfifDec->ProcessFile(m_pFile);
    // Now indicate that the file has been processed
    m_bFileAnalyzed = true;
  }
}

// Analyze the current file
// - Open file
// - Analyze file
// - Close file
//
// INPUT:
// - strFname           = File to analyze
//
// PRE:
// - m_pAppConfig->nPosStart    = Starting file offset for decode
//
// RETURN:
// - Status from opening file
//
bool CJPEGsnoopCore::AnalyzeFile(QString strFname)
{
  if(DEBUG_EN)
    m_pAppConfig->DebugLogAdd("CJPEGsnoopCore::AnalyzeFile() Begin");

  bool bRetVal;

  // Save the input filename
  // This filename is used later to confirm that output filename doesn't
  // match input filename when we save the log
  m_strPathName = strFname;

  // Assumes that we have set up the member vars already
  // Perform the actual processing. We have this in a routine
  // so that we can quickly recalculate the log file again
  // if an option changes.

  bRetVal = AnalyzeOpen();

  if(bRetVal)
  {
    // Only now that we have successfully opened the document
    // should be mark the flag as such. This flag is used by
    // other menu items to know whether or not the file is ready.
    AnalyzeFileDo();
  }

  AnalyzeClose();

  // In the last part of AnalyzeClose(), we mark the file
  // as not modified, so that we don't get prompted to save.

  if(DEBUG_EN)
    m_pAppConfig->DebugLogAdd("CJPEGsnoopDoc::AnalyzeFile() End");

  return bRetVal;
}

// Save the current log to text file with a simple implementation
//
// - This routine is implemented with a simple output mechanism rather
//   than leveraging CRichEditCtrl::DoSave() so that we can perform
//   this operation in command-line mode or batch mode operations.
//
// TODO: Should this be moved into CDocLog?
//
void CJPEGsnoopCore::DoLogSave(QString strLogName)
{
  QFile *pLog;

  // Open the file for output
  Q_ASSERT(strLogName != "");

  // OLD COMMENTS FOLLOW
  // This save method will only work if we were in Quick Log mode
  // where we have recorded the log to a string buffer and not
  // directly to the RichEdit
  // Note that m_bLogQuickMode is only toggled on during processing
  // so we can't check the status here (except seeing that there are no
  // lines in the array)

  // TODO: Ensure file doesn't exist and only overwrite if specified in command-line?

  // TODO: Confirm that we are not writing to the same file we opened (m_strPathName)
/*  Q_ASSERT(strLogName != m_strPathName);

  pLog = new QFile(strLogName);
  // Open specified file

  if(pLog->open(QIODevice::WriteOnly | QIODevice::Text) == false)
  {
    QString strError;

    strError = QString("ERROR: Couldn't open file for write [%s]: [%s]").arg(strLogName);
    // FIXME: Find an alternate method of signaling error in command-line mode
    msgBox.setText(strError);
    pLog = NULL;

    return;
  }

  // Step through the current log buffer
  QString strLine;

  QColor sCol;

  unsigned nQuickLines = glb_pDocLog->GetNumLinesLocal();

  for(unsigned nLine = 0; nLine < nQuickLines; nLine++)
  {
    glb_pDocLog->GetLineLogLocal(nLine, strLine, sCol);

    // NOTE: At this time, batch file processing uses an 8-bit text file output format
    // rather than the Unicode-supporting CRichEdit that is used when we manually
    // save the log file.
    //
    // If we pass extended characters to WriteString() when the CStdioFile has been
    // opened as CFile::typeText, the extended characters will terminate the lines
    // early.
    //
    // Therefore, we will convert all extended characters back to ASCII here.
    //
    // TODO: Revise this logic so that we can have more foreign-character support
    // in the batch output files.
//              CW2A    strAscii(strLine);
//    QString   strConv(strAscii);
    pLog->write(strLine.toLatin1().data());
  } */

  // Close the file
  if(pLog)
  {
    pLog->close();
    delete pLog;
  }
}

// --------------------------------------------------------------------
// --- START OF BATCH PROCESSING
// --------------------------------------------------------------------

//
// Generate a list of files for a batch processing operation
//
// POST:
// - m_asBatchFiles             = Contains list of full file paths
//
void CJPEGsnoopCore::GenBatchFileList(QString strDirSrc, QString strDirDst, bool bRecSubdir, bool bExtractAll)
{
  // TODO:
  // - What is the best way to provide a "Cancel Dialog" for a
  //   recursive operation? I can easily create the cancel / progress
  //   dialog with operations that have can be single-stepped, but
  //   not ones that require accumulation on the stack.
  // - For now, just leave as-is.

  // Example code that I can use for single-stepping the operation:
  //
  // // === START
  // COperationDlg LengthyOp(this);
  // LengthyOp.SetFunctions( PrepareOperation, NextIteration, GetProgress );
  //
  // // Until-done based
  // bool bOk = LengthyOp.RunUntilDone( true );
  // // === END

  m_asBatchFiles.clear();
  m_asBatchOutputs.clear();

  // Start the batch operation
  GenBatchFileListRecurse(strDirSrc, strDirDst, "", bRecSubdir, bExtractAll);
}

// Recursive routine that searches for files and folders
// Used in batch file processing mode
//
// INPUT:
// - strSrcRootName             Starting path for input files
// - strDstRootName             Starting path for output files
// - strPathName                        Path (below strSrcRootName) and filename for current input file
// - bSubdirs                   Flag to descend into all sub-directories
// - bExtractAll                Flag to extract all JPEG from file
//
void CJPEGsnoopCore::GenBatchFileListRecurse(QString strSrcRootName, QString strDstRootName, QString strPathName, bool bSubdirs,
                                             bool bExtractAll)
{
  // The following code snippet is based on MSDN code:
  // http://msdn.microsoft.com/en-us/library/scx99850%28VS.80%29.aspx

//  CFileFind finder;

  QString strWildcard;
  QString strDirName;
  QString strSubPath;
  QString strPath;

  // build a string with wildcards
  if(strPathName.isEmpty())
  {
    strWildcard = QString("%1\\*.*").arg(strSrcRootName);
  }
  else
  {
    strWildcard = QString("%1\\%2\\*.*").arg(strSrcRootName).arg(strPathName);
  }

  // start working for files
//  bool bWorking = finder.FindFile(strWildcard);
/*
  while(bWorking)
  {
    bWorking = finder.FindNextFile();

    // skip . and .. files; otherwise, we'd
    // recur infinitely!

    if(finder.IsDots())
      continue;

    strPath = finder.GetFilePath();

    // Now need to remove "root" from "path" to recalculate
    // new relative subpath
    strDirName = finder.GetFileName();

    if(strPathName.isEmpty())
    {
      strSubPath = strDirName;
    }
    else
    {
      strSubPath = QString("%s\\%s").arg(strPathName).arg(strDirName);
    }

    // if it's a directory, recursively     search it
    if(finder.IsDirectory())
    {
      if(bSubdirs)
      {
        GenBatchFileListRecurse(strSrcRootName, strDstRootName, strSubPath, bSubdirs, bExtractAll);
      }
    }
    else
    {
      // GetFilePath() includes both the path & filename
      // when called on a     file entry,     so there is     no need
      // to specifically call GetFileName()
      //                 QString strFname =     finder.GetFileName();

      // Now perform final filtering check on file
      // and add to batch file list
      GenBatchFileListSingle(strSrcRootName, strDstRootName, strSubPath, bExtractAll);
    }
  }

  finder.Close(); */
}

// Perform processing on file selected by the batch recursion
// process GenBatchFileListRecurse().
//
// INPUT:
// - strSrcRootName             Starting path for input files
// - strDstRootName             Starting path for output files
// - strPathName                Path (below szSrcRootName) and filename for current input file
// - bExtractAll                Flag to extract all JPEG from file
//
void CJPEGsnoopCore::GenBatchFileListSingle(QString strSrcRootName, QString strDstRootName, QString strPathName, bool bExtractAll)
{
  //bool          bDoSubmit = false;
  uint32_t nInd;

  QString strFnameSrc;
  QString strFnameDst;
  QString strFnameOnly;
  QString strFnameExt;
  QString strFnameLog;
  QString strFnameEmbed;

  strFnameSrc = QString("%1\\%2").arg(strSrcRootName).arg(strPathName);
  strFnameDst = QString("%1\\%2").arg(strDstRootName).arg(strPathName);

  // Extract the filename (without extension) and extension from
  // the full pathname.
  //
  // FIXME: Rewrite to use Windows APIs to do this or else
  // use the boost library as they will be more robust than
  // the simple searches for '\' and '.' that are done here.
  //
#if 1
  // -----------
  strFnameOnly = strFnameSrc.mid(strFnameSrc.lastIndexOf('\\') + 1);
  nInd = strFnameOnly.lastIndexOf('.');
  strFnameOnly = strFnameOnly.mid(0, nInd);

  // Extract the file extension
  nInd = strFnameSrc.lastIndexOf('.');
  strFnameExt = strFnameSrc.mid(nInd);
  strFnameExt = strFnameExt.toLower();
  // -----------
#else
  uint32_t nLen = strFnameSrc.GetLength();

  LPTSTR lpstrSrcFullpath = new TCHAR[nLen + 1];

  _tcscpy_s(lpstrSrcFullpath, nLen + 1, strFnameSrc);

  LPTSTR lpstrSrcFnameAndExt = NULL;

  lpstrSrcFnameAndExt = PathFindFileName(lpstrSrcFullpath);
  ...
#endif
    // Only process files that have an extension that implies JPEG
    // Batch processing is generally limited to files that appear to
    // represent a JPEG image (ie. ".jpg" an ".jpeg").
    //
    // TODO: Enable user to present list of extensions or disable
    // the check altogether.
    // TODO: Add Batch processing support for .PSD?
    bool bProcessFile = false;

  // TODO: Support m_pAppConfig->strBatchExtensions
  if((strFnameExt == ".jpg") || (strFnameExt == ".jpeg"))
  {
    bProcessFile = true;
  }

  if(bExtractAll)
  {
    // In the "Extract all" mode during batch processing, we disable
    // the extension check altogether as "Extract all" mode generally
    // implies that the files may not be standalone JPEG image files.
    // They could also be corrupt files that contain JPEG images.
    //
    // TODO: Provide additional configuration options ahead of time.
    bProcessFile = true;
  }

  if(bProcessFile)
  {
    // Generate the output log file name
    //
    // IMPORTANT NOTE:
    //   It is *essential* that this Append function work properly
    //   as it is used to generate the log filename from the
    //   image filename. Since we may be automatically overwriting
    //   the logfile, it is imperative that we ensure that there is
    //   no chance that the log filename happens to be the original
    //   JPEG filename.
    //
    //   In the interests of paranoia, we perform some extra checks
    //   here to ensure that this doesn't happen.

    strFnameLog = strFnameDst;
    strFnameLog.append(".txt");

    // Now double check the filenames to ensure they don't match the image

    // - Is the last 4 characters of the strFname ".txt"?
    if(strFnameLog.right(4) != ".txt")
    {
      // Report error message and skip logfile save
      // TODO: Handle non-GUI mode
      msgBox.setText("ERROR: Internal error #10100");
      return;
    }

    // - Is the strFnameLog different from the input filename (strFname)?
    if(strFnameLog == strPathName)
    {
      // Report error message and skip logfile save
      // TODO: Handle non-GUI mode
      msgBox.setText("ERROR: Internal error #10101");
      return;
    }

    // Add filename pair to the list
    // - Input filename to analyze
    // - Output filename for associated log/report
    m_asBatchFiles.append(strFnameSrc);
    m_asBatchDest.append(strFnameDst);
    m_asBatchOutputs.append(strFnameLog);
  }
}

// Return the number of files in the batch list
uint32_t CJPEGsnoopCore::GetBatchFileCount()
{
  return m_asBatchFiles.count();
}

// Return the filename at the batch index specified
QString CJPEGsnoopCore::GetBatchFileInfo(uint32_t nFileInd)
{
  if(nFileInd < GetBatchFileCount())
  {
    return m_asBatchFiles[nFileInd];
  }
  else
  {
    return "???";
  }
}

// Perform AnalyzeFile() but handle any search modes first
//
// RETURN:
// - true if file opened OK, false if issue during open
//
bool CJPEGsnoopCore::DoAnalyzeOffset(QString strFname)
{
  // Handle the different file offset / search modes
  bool bStatus = false;
  bool bSearchResult = false;

  uint32_t nStartPos = 0;
  uint32_t nSearchPos = 0;

  if(m_pAppConfig->eCmdLineOffset == DEC_OFFSET_START)
  {
    // Decode at start of file
    m_pAppConfig->nPosStart = 0;
    bStatus = AnalyzeFile(strFname);
  }
  else if(m_pAppConfig->eCmdLineOffset == DEC_OFFSET_SRCH1)
  {
    // Decode at 1st SOI found in file
    m_pAppConfig->nPosStart = 0;
    bStatus = AnalyzeFile(strFname);

    if(bStatus)
    {
      if(!m_pJfifDec->GetDecodeStatus())
      {
        // SOI not found at start, so begin search
        bStatus = AnalyzeOpen();
        nStartPos = m_pAppConfig->nPosStart;
        bSearchResult = B_BufSearch(nStartPos, 0xFFD8FF, 3, true, nSearchPos);
        AnalyzeClose();

        if(bSearchResult)
        {
          // If found, update offset & re-analyze
          m_pAppConfig->nPosStart = nSearchPos;
          bStatus = AnalyzeFile(strFname);
        }
      }
    }
  }
  else if(m_pAppConfig->eCmdLineOffset == DEC_OFFSET_SRCH2)
  {
    // Decode at 1st SOI found after start of file
    // Force a search
    m_pAppConfig->nPosStart = 0;
    bStatus = AnalyzeFile(strFname);

    if(bStatus)
    {
      bStatus = AnalyzeOpen();
      nStartPos = m_pAppConfig->nPosStart;
      bSearchResult = B_BufSearch(nStartPos, 0xFFD8FF, 3, true, nSearchPos);
      AnalyzeClose();

      if(bSearchResult)
      {
        // If found, update offset & re-analyze
        m_pAppConfig->nPosStart = nSearchPos;
        bStatus = AnalyzeFile(strFname);
      }
    }
  }
  else if(m_pAppConfig->eCmdLineOffset == DEC_OFFSET_POS)
  {
    // Decode from quint8 ### in file
    m_pAppConfig->nPosStart = m_pAppConfig->nCmdLineOffsetPos;
    bStatus = AnalyzeFile(strFname);
  }
  else
  {
    Q_ASSERT(false);
  }

  // bStatus reports the success in opening the file

  return bStatus;
}

// Process a file in the batch file list
//
// INPUT:
// - nFileInd                   = Index into batch file list
// - bWriteLog                  = Write out log file after processing file?
// - bExtractAll                = Perform extract-all on this file?
//
void CJPEGsnoopCore::DoBatchFileProcess(uint32_t nFileInd, bool bWriteLog, bool bExtractAll)
{
  bool bStatus = false;

  uint32_t nBatchFileCount;

  QString strFnameFile;
  QString strFnameDst;
  QString strFnameLog;
  QString strFnameEmbed;

  nBatchFileCount = GetBatchFileCount();

  if(nFileInd >= nBatchFileCount)
  {
    Q_ASSERT(false);
    return;
  }

  //TODO: strFnameDst is not really used as it is base destination filename
  strFnameFile = m_asBatchFiles[nFileInd];
  strFnameDst = m_asBatchDest[nFileInd];
  strFnameLog = m_asBatchOutputs[nFileInd];

  // Indicate that the file source has changed so that any image decoding is enabled
  J_ImgSrcChanged();

  // Save the current filename into the temporary config space
  // This is only done to assist debugging reporting
  m_pAppConfig->strCurFname = strFnameFile;

  // Handle the different file offset / search modes
  bStatus = DoAnalyzeOffset(strFnameFile);

  if(!bStatus)
  {
    // If we had an issue opening the file, abort here
    // TODO: Consider whether we should alert the user (perhaps in direct window log write)
    return;
  }

  if(bWriteLog || bExtractAll)
  {
    // Now we need to ensure that the destination directory has been created
    // Build up the subdirectories as required
    BuildDirPath(strFnameDst);
  }

  // Save the output log
  if(bWriteLog)
  {
    DoLogSave(strFnameLog);
  }

  if(bExtractAll)
  {
    //
    // TODO: In the "Extract all" mode we should ask the
    // user for the full configuration ahead of the batch
    // process.

    // Create the filepath for any batch embedded JPEG extraction
    strFnameEmbed = strFnameDst;
    strFnameEmbed.append(".export.jpg");

    QString strInputFname = strFnameFile;
    QString strExportFname = strFnameEmbed;

    bool bOverlayEn = false;
    bool bForceSoi = false;
    bool bForceEoi = false;
    bool bIgnoreEoi = false;
    bool bExtractAllEn = true;
    bool bDhtAviInsert = false;

    QString strOutPath = "";    // unused

    DoExtractEmbeddedJPEG(strInputFname, strExportFname, bOverlayEn, bForceSoi, bForceEoi, bIgnoreEoi, bExtractAllEn,
                          bDhtAviInsert, strOutPath);
  }

  // Now submit entry to database!
#if 0                           // Not supported currently
#ifdef BATCH_DO_DBSUBMIT
  bDoSubmit = m_pJfifDec->CompareSignature(true);
  if(bDoSubmit)
  {
    // FIXME: Check function param values. Might be outdated
    m_pJfifDec->PrepareSendSubmit(m_pJfifDec->m_strImgQualExif, m_pJfifDec->m_eDbReqSuggest, ""), "BATCH";
  }
#endif
#endif
}

// --------------------------------------------------------------------
// --- END OF BATCH PROCESSING
// --------------------------------------------------------------------

// Ensure that the file system path exists,
// create the nested subdirectories as required
void CJPEGsnoopCore::BuildDirPath(QString strPath)
{
  // Now we need to ensure that the destination directory has been created
  // Build up the subdirectories as required
  //
  // Use PathRemoveFileSpec() to find the directory path instead of
  // simply looking for the last '\'. This should be safer.
//  unsigned nLen = strPath.length();

//  char *lpstrDstPathOnly = new char[nLen + 1];

//  strcpy(lpstrDstPathOnly, strPath.toLatin1().data());
//  PathRemoveFileSpec(lpstrDstPathOnly);

//  SHCreateDirectoryEx(NULL, lpstrDstPathOnly, NULL);
  QDir dir;
  strPath = dir.filePath(strPath);
  dir.mkdir(strPath);
//  if(lpstrDstPathOnly)
//  {
//    delete[]lpstrDstPathOnly;
//    lpstrDstPathOnly = NULL;
//  }
}

#if 0                           //xxx
// Placeholder code for command-line call to extract JPEGs
void CJPEGsnoopCore::DoExtractEmbeddedJPEGCmdLine()
{
  // Determine if file is AVI
  // If so, default to DHT AVI insert mode
  // NOTE: Calling GetAviMode() requires that the CDocument JFIF decoder
  //       instance has processed the file.
  bool bIsAvi, bIsMjpeg;

  m_pJfifDec->GetAviMode(bIsAvi, bIsMjpeg);
  dlgExport.m_bDhtAviInsert = bIsMjpeg;

  bExtractAllEn = true;
  bForceSoi = false;
  bForceEoi = false;
  bIgnoreEoi = false;
  bOverlayEn = false;
  bDhtAviInsert = bInsDhtAvi;

  DoExtractEmbeddedJPEG1(..);
}
#endif

// Extracts an embedded JPEG from the current file
// Also support extraction of all embedded JPEG from the file
//
// NOTE: In "Extract All" mode, all file types are processed.
//
// TODO:
// - strOutPath unused
void CJPEGsnoopCore::DoExtractEmbeddedJPEG(QString strInputFname, QString strOutputFname,
                                           bool bOverlayEn, bool bForceSoi, bool bForceEoi, bool bIgnoreEoi, bool bExtractAllEn,
                                           bool bDhtAviInsert, QString strOutPath)
{
  unsigned int nFileSize = 0;

  bool bRet;

  if(!bExtractAllEn)
  {
    // ========================================================
    // Extract Single mode
    // ========================================================

    // For export, we need to analyze first to get some info
    // including suitability for export
    // AnalyzeOpen() calls require m_strPathName to be set
    m_strPathName = strInputFname;
    bRet = AnalyzeOpen();

    if(bRet)
    {
      AnalyzeFileDo();
    }

    // FIXME: Is this being used anywhere?
    // strTmp = QString("0x%08X",m_pJfifDec->GetPosEmbedStart());

    // If we are not in "extract all" mode, then check now to see if
    // the current file position looks OK to extract a valid JPEG.

    // Is the currently-decoded file in a suitable
    // state for JPEG extraction (ie. have we seen all
    // the necessary markers?)
    bRet = m_pJfifDec->ExportJpegPrepare(strInputFname, bForceSoi, bForceEoi, bIgnoreEoi);

    if(!bRet)
    {
      return;
    }

    // --------------------------------------------------------
    // Perform extraction of single embedded JPEG
    // --------------------------------------------------------

    if(m_lFileSize > 0xFFFFFFFFUL)
    {
      QString strTmp = "Extract file too large. Skipping.";

      glb_pDocLog->AddLineErr(strTmp);

      if(m_pAppConfig->bInteractive)
        msgBox.setText(strTmp);
    }
    else
    {
      // Extract the JPEG file
      nFileSize = static_cast < uint32_t >(m_lFileSize);

      bRet = m_pJfifDec->ExportJpegDo(strInputFname, strOutputFname, nFileSize, bOverlayEn, bDhtAviInsert, bForceSoi, bForceEoi);
    }

    AnalyzeClose();

  }
  else
  {
    // ========================================================
    // Extract All mode
    // ========================================================

    Q_ASSERT(strOutputFname != "");

    // In extract-all, start at file offset 0
    m_pAppConfig->nPosStart = 0;

    // For added safety, reset the JFIF decoder state
    // Note that this is already covered by the AnalyzeFileDo() call to JFIF Process() later
    m_pJfifDec->Reset();

    // --------------------------------------------------------
    // Perform batch extraction of all embedded JPEGs
    // --------------------------------------------------------

    // Ensure we have no file state left over
    // FIXME: Do I really need this here?
    AnalyzeClose();

    bool bDoneBatch = false;

    uint32_t nExportCnt = 1;

    uint32_t nStartPos = 0;

    bool bSearchResult = false;

    uint32_t nSearchPos = 0;

    bool bSkipFrame = false;

    // Determine the root filename
    // Filename selected by user: strEmbedFileName
    QString strRootFileName;

    int nExtInd;

    // FIXME: Do I really want to use strOutputFname? That is typically the log file
    nExtInd = strOutputFname.lastIndexOf('.');
    Q_ASSERT(nExtInd != -1);

    if(nExtInd == -1)
    {
      QString strTmp;

      strTmp = QString("ERROR: Invalid filename [%1]").arg(strOutputFname);
      glb_pDocLog->AddLineErr(strTmp);
      if(m_pAppConfig->bInteractive)
        msgBox.setText(strTmp);
      return;
    }

    strRootFileName = strOutputFname.left(nExtInd);
    QString strOutputFnameTemp = "";

    // Loop through file until done
    while(!bDoneBatch)
    {

      bSkipFrame = false;

      // Create filename
      // 6 digits of numbering will support videos at 60 fps for over 4.5hrs.
      strOutputFnameTemp = QString("%1.%06u.jpg").arg(strRootFileName).arg(nExportCnt);

      // Opens file (m_strPathName), resets window buffer, file size, etc.
      AnalyzeOpen();

      // AnalyzeFileDo() also resets all JFIF decoder state for us
      // Perform the JFIF parsing & decoding
      // Decoding starts at file offset m_pAppConfig->nPosStart
      AnalyzeFileDo();

      // Is the currently-decoded file in a suitable state for JPEG extraction?
      // (ie. have we seen all the necessary markers?)
      bRet = m_pJfifDec->ExportJpegPrepare(strInputFname, bForceSoi, bForceEoi, bIgnoreEoi);

      if(!bRet)
      {
        // Skip this particular frame
        bSkipFrame = true;
      }

      if(!bSkipFrame)
      {
        if(m_lFileSize > 0xFFFFFFFFULL)
        {
          QString strTmp;

          strTmp = QString("ERROR: Extract file too large. Skipping. [Size=0x%1]").arg(m_lFileSize);
          glb_pDocLog->AddLineErr(strTmp);
          if(m_pAppConfig->bInteractive)
            msgBox.setText(strTmp);
        }
        else
        {
          // Extract the JPEG file
          nFileSize = static_cast < uint32_t >(m_lFileSize);

          bRet = m_pJfifDec->ExportJpegDo(strInputFname, strOutputFnameTemp, nFileSize,
                                          bOverlayEn, bDhtAviInsert, bForceSoi, bForceEoi);
        }

        // Still increment frame # even if we skipped export (due to file size)
        nExportCnt++;
      }

      // See if we can find the next embedded JPEG (file must still be open for this)
      Q_ASSERT(m_bFileOpened);
      nStartPos = m_pAppConfig->nPosStart;
      m_pJfifDec->ImgSrcChanged();

      bSearchResult = m_pWBuf->BufSearch(nStartPos, 0xFFD8FF, 3, true, nSearchPos);

      if(bSearchResult)
      {
        // Set starting file offset
        m_pAppConfig->nPosStart = nSearchPos;

        // AnalyzeFileDo() also resets all JFIF decoder state for us
        AnalyzeFileDo();

      }
      else
      {
        // No SOI Marker found in Forward search -> Stop batch
        bDoneBatch = true;
      }

      // For export, we need to call AnalyzeClose()
      AnalyzeClose();
    }                           // bDoneBatch

    // Close the file in case we aborted above
    AnalyzeClose();
  }                             // bExtractAllEn
}

// ==============================================

// ----------------------------------------------
// Accessors for CjfifDecode
// ----------------------------------------------

void CJPEGsnoopCore::J_GetAviMode(bool & bIsAvi, bool & bIsMjpeg)
{
  m_pJfifDec->GetAviMode(bIsAvi, bIsMjpeg);
}

void CJPEGsnoopCore::J_SetAviMode(bool bIsAvi, bool bIsMjpeg)
{
  m_pJfifDec->SetAviMode(bIsAvi, bIsMjpeg);
}

void CJPEGsnoopCore::J_ImgSrcChanged()
{
  m_pJfifDec->ImgSrcChanged();
}

uint32_t CJPEGsnoopCore::J_GetPosEmbedStart()
{
  return m_pJfifDec->GetPosEmbedStart();
}

uint32_t CJPEGsnoopCore::J_GetPosEmbedEnd()
{
  return m_pJfifDec->GetPosEmbedEnd();
}

void CJPEGsnoopCore::J_GetDecodeSummary(QString & strHash, QString & strHashRot, QString & strImgExifMake,
                                        QString & strImgExifModel, QString & strImgQualExif, QString & strSoftware,
                                        teDbAdd & eDbReqSuggest)
{
  m_pJfifDec->GetDecodeSummary(strHash, strHashRot, strImgExifMake, strImgExifModel, strImgQualExif, strSoftware, eDbReqSuggest);
}

uint32_t CJPEGsnoopCore::J_GetDqtZigZagIndex(uint32_t nInd, bool bZigZag)
{
  return m_pJfifDec->GetDqtZigZagIndex(nInd, bZigZag);
}

uint32_t CJPEGsnoopCore::J_GetDqtQuantStd(uint32_t nInd)
{
  return m_pJfifDec->GetDqtQuantStd(nInd);
}

void CJPEGsnoopCore::J_SetStatusBar(QStatusBar * pStatBar)
{
  m_pJfifDec->SetStatusBar(pStatBar);
}

void CJPEGsnoopCore::J_ProcessFile(QFile * inFile)
{
  m_pJfifDec->ProcessFile(inFile);
}

void CJPEGsnoopCore::J_PrepareSendSubmit(QString strQual, teSource eUserSource, QString strUserSoftware, QString strUserNotes)
{
  m_pJfifDec->PrepareSendSubmit(strQual, eUserSource, strUserSoftware, strUserNotes);
}

// ----------------------------------------------
// Accessors for CwindowBuf
// ----------------------------------------------

void CJPEGsnoopCore::B_SetStatusBar(QStatusBar * pStatBar)
{
  m_pWBuf->SetStatusBar(pStatBar);
}

void CJPEGsnoopCore::B_BufLoadWindow(uint32_t nPosition)
{
  m_pWBuf->BufLoadWindow(nPosition);
}

void CJPEGsnoopCore::B_BufFileSet(QFile * inFile)
{
  m_pWBuf->BufFileSet(inFile);
}

void CJPEGsnoopCore::B_BufFileUnset()
{
  m_pWBuf->BufFileUnset();
}

quint8 CJPEGsnoopCore::B_Buf(uint32_t nOffset, bool bClean)
{
  return m_pWBuf->Buf(nOffset, bClean);
}

bool CJPEGsnoopCore::B_BufSearch(uint32_t nStartPos, uint32_t nSearchVal, uint32_t nSearchLen,
                                 bool bDirFwd, uint32_t &nFoundPos)
{
  return m_pWBuf->BufSearch(nStartPos, nSearchVal, nSearchLen, bDirFwd, nFoundPos);
}

bool CJPEGsnoopCore::B_OverlayInstall(uint32_t nOvrInd, quint8 * pOverlay, uint32_t nLen, uint32_t nBegin,
                                      uint32_t nMcuX, uint32_t nMcuY, uint32_t nMcuLen, uint32_t nMcuLenIns,
                                      int nAdjY, int nAdjCb, int nAdjCr)
{
  return m_pWBuf->OverlayInstall(nOvrInd, pOverlay, nLen, nBegin, nMcuX, nMcuY, nMcuLen, nMcuLenIns, nAdjY, nAdjCb, nAdjCr);
}

void CJPEGsnoopCore::B_OverlayRemoveAll()
{
  m_pWBuf->OverlayRemoveAll();
}

bool CJPEGsnoopCore::B_OverlayGet(uint32_t nOvrInd, quint8 * &pOverlay, uint32_t &nLen, uint32_t &nBegin)
{
  return m_pWBuf->OverlayGet(nOvrInd, pOverlay, nLen, nBegin);
}

// ----------------------------------------------
// Accessors for CImgDec
// ----------------------------------------------
void CJPEGsnoopCore::I_SetStatusBar(QStatusBar * pStatBar)
{
  m_pImgDec->SetStatusBar(pStatBar);
}

uint32_t CJPEGsnoopCore::I_GetDqtEntry(uint32_t nTblDestId, uint32_t nCoeffInd)
{
  return m_pImgDec->GetDqtEntry(nTblDestId, nCoeffInd);
}

void CJPEGsnoopCore::I_SetPreviewMode(uint32_t nMode)
{
  m_pImgDec->SetPreviewMode(nMode);
}

uint32_t CJPEGsnoopCore::I_GetPreviewMode()
{
  return m_pImgDec->GetPreviewMode();
}

void CJPEGsnoopCore::I_SetPreviewYccOffset(uint32_t nMcuX, uint32_t nMcuY, int nY, int nCb, int nCr)
{
  m_pImgDec->SetPreviewYccOffset(nMcuX, nMcuY, nY, nCb, nCr);
}

void CJPEGsnoopCore::I_GetPreviewYccOffset(uint32_t &nMcuX, uint32_t &nMcuY, int &nY, int &nCb, int &nCr)
{
  m_pImgDec->GetPreviewYccOffset(nMcuX, nMcuY, nY, nCb, nCr);
}

void CJPEGsnoopCore::I_SetPreviewMcuInsert(uint32_t nMcuX, uint32_t nMcuY, int nLen)
{
  m_pImgDec->SetPreviewMcuInsert(nMcuX, nMcuY, nLen);
}

void CJPEGsnoopCore::I_GetPreviewMcuInsert(uint32_t &nMcuX, uint32_t &nMcuY, uint32_t &nLen)
{
  m_pImgDec->GetPreviewMcuInsert(nMcuX, nMcuY, nLen);
}

void CJPEGsnoopCore::I_SetPreviewZoom(bool bInc, bool bDec, bool bSet, uint32_t nVal)
{
  m_pImgDec->SetPreviewZoom(bInc, bDec, bSet, nVal);
}

uint32_t CJPEGsnoopCore::I_GetPreviewZoomMode()
{
  return m_pImgDec->GetPreviewZoomMode();
}

float CJPEGsnoopCore::I_GetPreviewZoom()
{
  return m_pImgDec->GetPreviewZoom();
}

bool CJPEGsnoopCore::I_GetPreviewOverlayMcuGrid()
{
  return m_pImgDec->GetPreviewOverlayMcuGrid();
}

void CJPEGsnoopCore::I_SetPreviewOverlayMcuGridToggle()
{
  m_pImgDec->SetPreviewOverlayMcuGridToggle();
}

QPoint CJPEGsnoopCore::I_PixelToMcu(QPoint ptPix)
{
  return m_pImgDec->PixelToMcu(ptPix);
}

QPoint CJPEGsnoopCore::I_PixelToBlk(QPoint ptPix)
{
  return m_pImgDec->PixelToBlk(ptPix);
}

uint32_t CJPEGsnoopCore::I_McuXyToLinear(QPoint ptMcu)
{
  return m_pImgDec->McuXyToLinear(ptMcu);
}

void CJPEGsnoopCore::I_GetImageSize(uint32_t &nX, uint32_t &nY)
{
  m_pImgDec->GetImageSize(nX, nY);
}

void CJPEGsnoopCore::I_GetPixMapPtrs(short *&pMapY, short *&pMapCb, short *&pMapCr)
{
  m_pImgDec->GetPixMapPtrs(pMapY, pMapCb, pMapCr);
}

void CJPEGsnoopCore::I_GetDetailVlc(bool & bDetail, uint32_t &nX, uint32_t &nY, uint32_t &nLen)
{
  m_pImgDec->GetDetailVlc(bDetail, nX, nY, nLen);
}

void CJPEGsnoopCore::I_SetDetailVlc(bool bDetail, uint32_t nX, uint32_t nY, uint32_t nLen)
{
  m_pImgDec->SetDetailVlc(bDetail, nX, nY, nLen);
}

uint32_t CJPEGsnoopCore::I_GetMarkerCount()
{
  return m_pImgDec->GetMarkerCount();
}

void CJPEGsnoopCore::I_SetMarkerBlk(uint32_t nBlkX, uint32_t nBlkY)
{
  m_pImgDec->SetMarkerBlk(nBlkX, nBlkY);
}

QPoint CJPEGsnoopCore::I_GetMarkerBlk(uint32_t nInd)
{
  return m_pImgDec->GetMarkerBlk(nInd);
}

void CJPEGsnoopCore::I_SetStatusText(QString strText)
{
  m_pImgDec->SetStatusText(strText);
}

QString CJPEGsnoopCore::I_GetStatusYccText()
{
  return m_pImgDec->GetStatusYccText();
}

void CJPEGsnoopCore::I_SetStatusYccText(QString strText)
{
  m_pImgDec->SetStatusYccText(strText);
}

QString CJPEGsnoopCore::I_GetStatusMcuText()
{
  return m_pImgDec->GetStatusMcuText();
}

void CJPEGsnoopCore::I_SetStatusMcuText(QString strText)
{
  m_pImgDec->SetStatusMcuText(strText);
}

QString CJPEGsnoopCore::I_GetStatusFilePosText()
{
  return m_pImgDec->GetStatusFilePosText();
}

void CJPEGsnoopCore::I_SetStatusFilePosText(QString strText)
{
  m_pImgDec->SetStatusFilePosText(strText);
}

void CJPEGsnoopCore::I_GetBitmapPtr(unsigned char *&pBitmap)
{
  m_pImgDec->GetBitmapPtr(pBitmap);
}

void CJPEGsnoopCore::I_LookupFilePosMcu(uint32_t nMcuX, uint32_t nMcuY, uint32_t &nquint8, uint32_t &nBit)
{
  m_pImgDec->LookupFilePosMcu(nMcuX, nMcuY, nquint8, nBit);
}

void CJPEGsnoopCore::I_LookupFilePosPix(uint32_t nPixX, uint32_t nPixY, uint32_t &nquint8, uint32_t &nBit)
{
  m_pImgDec->LookupFilePosPix(nPixX, nPixY, nquint8, nBit);
}

void CJPEGsnoopCore::I_LookupBlkYCC(uint32_t nBlkX, uint32_t nBlkY, int &nY, int &nCb, int &nCr)
{
  m_pImgDec->LookupBlkYCC(nBlkX, nBlkY, nY, nCb, nCr);
}

void CJPEGsnoopCore::I_ViewOnDraw(QPainter *pDC, QRect rectClient, QPoint ptScrolledPos, QFont *pFont, QSize & szNewScrollSize)
{
  m_pImgDec->ViewOnDraw(pDC, rectClient, ptScrolledPos, pFont, szNewScrollSize);
}

void CJPEGsnoopCore::I_GetPreviewPos(uint32_t &nX, uint32_t &nY)
{
  m_pImgDec->GetPreviewPos(nX, nY);
}

void CJPEGsnoopCore::I_GetPreviewSize(uint32_t &nX, uint32_t &nY)
{
  m_pImgDec->GetPreviewSize(nX, nY);
}

bool CJPEGsnoopCore::I_IsPreviewReady()
{
  return m_pImgDec->IsPreviewReady();
}
