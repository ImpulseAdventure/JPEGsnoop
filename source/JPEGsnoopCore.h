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
// - This module performs all of the main decoding and processing functions
// - It contains the major context (m_pWBuf, m_pImgDec, m_pJfifDec)
//
// ==========================================================================

#pragma once

#include <QMessageBox>
#include <QPainter>

#include "JfifDecode.h"
#include "ImgDecode.h"
#include "WindowBuf.h"
#include "SnoopConfig.h"

class CJPEGsnoopCore
{
public:
  CJPEGsnoopCore(void);
  ~CJPEGsnoopCore(void);

  void Reset();

  void SetStatusBar(QStatusBar * pStatBar);

  bool AnalyzeFile(QString strFname);
  void AnalyzeFileDo();
  bool AnalyzeOpen();
  void AnalyzeClose();
  bool IsAnalyzed();
  bool DoAnalyzeOffset(QString strFname);

  void DoLogSave(QString strLogName);

  void GenBatchFileList(QString strDirSrc, QString strDirDst, bool bRecSubdir, bool bExtractAll);
  uint32_t GetBatchFileCount();
  void DoBatchFileProcess(uint32_t nFileInd, bool bWriteLog, bool bExtractAll);
  QString GetBatchFileInfo(uint32_t nFileInd);

  void BuildDirPath(QString strPath);

  void DoExtractEmbeddedJPEG(QString strInputFname, QString strOutputFname,
                             bool bOverlayEn, bool bForceSoi, bool bForceEoi, bool bIgnoreEoi, bool bExtractAllEn,
                             bool bDhtAviInsert, QString strOutPath);

  // Accessor wrappers for CjfifDecode
  void J_GetAviMode(bool & bIsAvi, bool & bIsMjpeg);
  void J_SetAviMode(bool bIsAvi, bool bIsMjpeg);
  void J_ImgSrcChanged();
  uint32_t J_GetPosEmbedStart();
  uint32_t J_GetPosEmbedEnd();
  void J_GetDecodeSummary(QString & strHash, QString & strHashRot, QString & strImgExifMake, QString & strImgExifModel,
                          QString & strImgQualExif, QString & strSoftware, teDbAdd & eDbReqSuggest);
  uint32_t J_GetDqtZigZagIndex(uint32_t nInd, bool bZigZag);
  uint32_t J_GetDqtQuantStd(uint32_t nInd);
  void J_SetStatusBar(QStatusBar * pStatBar);
  void J_ProcessFile(QFile * inFile);
  void J_PrepareSendSubmit(QString strQual, teSource eUserSource, QString strUserSoftware, QString strUserNotes);

  // Accessor wrappers for CImgDec
  void I_SetStatusBar(QStatusBar * pStatBar);
  uint32_t I_GetDqtEntry(uint32_t nTblDestId, uint32_t nCoeffInd);
  void I_SetPreviewMode(uint32_t nMode);
  uint32_t I_GetPreviewMode();
  void I_SetPreviewYccOffset(uint32_t nMcuX, uint32_t nMcuY, int nY, int nCb, int nCr);
  void I_GetPreviewYccOffset(uint32_t &nMcuX, uint32_t &nMcuY, int &nY, int &nCb, int &nCr);
  void I_SetPreviewMcuInsert(uint32_t nMcuX, uint32_t nMcuY, int nLen);
  void I_GetPreviewMcuInsert(uint32_t &nMcuX, uint32_t &nMcuY, uint32_t &nLen);
  void I_SetPreviewZoom(bool bInc, bool bDec, bool bSet, uint32_t nVal);
  uint32_t I_GetPreviewZoomMode();
  float I_GetPreviewZoom();
  bool I_GetPreviewOverlayMcuGrid();
  void I_SetPreviewOverlayMcuGridToggle();
  QPoint I_PixelToMcu(QPoint ptPix);
  QPoint I_PixelToBlk(QPoint ptPix);
  uint32_t I_McuXyToLinear(QPoint ptMcu);
  void I_GetImageSize(uint32_t &nX, uint32_t &nY);
  void I_GetPixMapPtrs(short *&pMapY, short *&pMapCb, short *&pMapCr);
  void I_GetDetailVlc(bool & bDetail, uint32_t &nX, uint32_t &nY, uint32_t &nLen);
  void I_SetDetailVlc(bool bDetail, uint32_t nX, uint32_t nY, uint32_t nLen);
  uint32_t I_GetMarkerCount();
  void I_SetMarkerBlk(uint32_t nBlkX, uint32_t nBlkY);
  QPoint I_GetMarkerBlk(uint32_t nInd);
  void I_SetStatusText(QString strText);
  QString I_GetStatusYccText();
  void I_SetStatusYccText(QString strText);
  QString I_GetStatusMcuText();
  void I_SetStatusMcuText(QString strText);
  QString I_GetStatusFilePosText();
  void I_SetStatusFilePosText(QString strText);
  void I_GetBitmapPtr(unsigned char *&pBitmap);
  void I_LookupFilePosMcu(uint32_t nMcuX, uint32_t nMcuY, uint32_t &nByte, uint32_t &nBit);
  void I_LookupFilePosPix(uint32_t nPixX, uint32_t nPixY, uint32_t &nByte, uint32_t &nBit);
  void I_LookupBlkYCC(uint32_t nBlkX, uint32_t nBlkY, int &nY, int &nCb, int &nCr);
  void I_ViewOnDraw(QPainter* pDC,QRect rectClient,QPoint ptScrolledPos,QFont* pFont, QSize &szNewScrollSize);
  void I_GetPreviewPos(uint32_t &nX, uint32_t &nY);
  void I_GetPreviewSize(uint32_t &nX, uint32_t &nY);
  bool I_IsPreviewReady();

  // Accessor wrappers for CwindowBuf
  void B_SetStatusBar(QStatusBar * pStatBar);
  void B_BufLoadWindow(uint32_t nPosition);
  void B_BufFileSet(QFile * inFile);
  void B_BufFileUnset();
  quint8 B_Buf(uint32_t nOffset, bool bClean = false);
  bool B_BufSearch(uint32_t nStartPos, uint32_t nSearchVal, uint32_t nSearchLen, bool bDirFwd, uint32_t &nFoundPos);
  bool B_OverlayInstall(uint32_t nOvrInd, quint8 * pOverlay, uint32_t nLen, uint32_t nBegin,
                        uint32_t nMcuX, uint32_t nMcuY, uint32_t nMcuLen, uint32_t nMcuLenIns, int nAdjY, int nAdjCb, int nAdjCr);
  void B_OverlayRemoveAll();
  bool B_OverlayGet(uint32_t nOvrInd, quint8 * &pOverlay, uint32_t &nLen, uint32_t &nBegin);

  CimgDecode *imgDecoder() { return m_pImgDec; }
private:

  // Batch processing
  void GenBatchFileListRecurse(QString strSrcRootName, QString strDstRootName, QString strPathName, bool bSubdirs,
                               bool bExtractAll);
  void GenBatchFileListSingle(QString strSrcRootName, QString strDstRootName, QString strPathName, bool bExtractAll);

private:
    QMessageBox msgBox;

  // Config
  CSnoopConfig *m_pAppConfig;   // Pointer to application config

  // Input JPEG file
  QFile *m_pFile;
  quint64 m_lFileSize;

  QString m_strPathName;
  bool m_bFileAnalyzed;         // Have we opened and analyzed a file?
  bool m_bFileOpened;           // Is a file currently opened?

  // Decoders and Buffers
  CjfifDecode *m_pJfifDec;
  CimgDecode *m_pImgDec;
  CwindowBuf *m_pWBuf;

  // Batch processing
  QStringList m_asBatchFiles;
  QStringList m_asBatchDest;
  QStringList m_asBatchOutputs;

};
