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

// ==========================================================================
// CLASS DESCRIPTION:
// - Provides a cache for file access
// - Allows random access to a file but only issues new file I/O if
//   the requested address is outside of the current cache window
// - Provides an overlay for temporary (local) buffer overwrites
// - Buffer search methods
//
// ==========================================================================

#ifndef WINDOWBUF_H
#define WINDOWBUF_H

#include <QFile>
#include <QMessageBox>
#include <QString>
#include <QStatusBar>

#include "DocLog.h"

class QPlainTextEdit;
class CDocLog;

// For now, we only ever use MAX_BUF_WINDOW bytes, even though we
// have allocated MAX_BUF bytes up front. I might change this
// later. We don't want the window size to be too large as it
// could have an impact on performance.
#define MAX_BUF				     262144
#define MAX_BUF_WINDOW		 131072
#define MAX_BUF_WINDOW_REV 16384  //1024L

#define NUM_OVERLAYS		500
#define MAX_OVERLAY			500     // 500 bytes

#define NUM_HOLES			10

#define	MAX_BUF_READ_STR	255     // Max number of bytes to fetch in BufReadStr()

typedef struct
{
  bool bEn;                     // Enabled? -- not used currently
  uint32_t nStart;              // File position
  uint32_t nLen;                // MCU Length
  uint8_t anData[MAX_OVERLAY];  // Byte data

  // For reporting purposes:
  uint32_t nMcuX;               // Starting MCU X
  uint32_t nMcuY;               // Starting MCU Y
  uint32_t nMcuLen;             // Number of MCUs deleted
  uint32_t nMcuLenIns;          // Number of MCUs inserted
  int nDcAdjustY;
  int nDcAdjustCb;
  int nDcAdjustCr;
} sOverlay;

class CwindowBuf
{
public:
  CwindowBuf(class CDocLog *pDocLog);
  ~CwindowBuf();

public:
  void SetStatusBar(QStatusBar *pStatBar);

  void BufLoadWindow(uint32_t nPosition);
  void BufFileSet(QFile * inFile);
  void BufFileUnset();
  uint8_t Buf(uint32_t nOffset, bool bClean = false);
  uint32_t BufX(uint32_t nOffset, uint32_t nSz, bool bByteSwap = false);

  unsigned char BufRdAdv1(uint32_t &nOffset, bool bByteSwap);
  uint16_t BufRdAdv2(uint32_t &nOffset, bool bByteSwap);
  uint32_t BufRdAdv4(uint32_t &nOffset, bool bByteSwap);

  QString BufReadStr(uint32_t nPosition);
  QString BufReadUniStr(uint32_t nPosition);
  QString BufReadUniStr2(uint32_t nPos, uint32_t nBufLen);
  QString BufReadStrn(uint32_t nPosition, uint32_t nLen);

  bool BufSearch(uint32_t nStartPos, uint32_t nSearchVal, uint32_t nSearchLen, bool bDirFwd, uint32_t &nFoundPos);
  bool BufSearchX(uint32_t nStartPos, uint8_t * anSearchVal, uint32_t nSearchLen, bool bDirFwd, uint32_t &nFoundPos);

  bool OverlayAlloc(uint32_t nInd);
  bool OverlayInstall(uint32_t nOvrInd, uint8_t *pOverlay, uint32_t nLen, uint32_t nBegin,
                      uint32_t nMcuX, uint32_t nMcuY, uint32_t nMcuLen, uint32_t nMcuLenIns, int nAdjY, int nAdjCb, int nAdjCr);
  void OverlayRemove();
  void OverlayRemoveAll();
  bool OverlayGet(uint32_t nOvrInd, uint8_t *&pOverlay, uint32_t &nLen, uint32_t &nBegin);
  uint32_t OverlayGetNum();
  void ReportOverlays(CDocLog *pLog);

  bool GetBufOk();
  uint32_t GetPosEof();

private:
  void Reset();

  CwindowBuf &operator = (const CwindowBuf&);
  CwindowBuf(CwindowBuf&);

  CDocLog *m_pDocLog;

  unsigned char *m_pBuffer;
  QFile *m_pBufFile;
  uint32_t m_nBufWinSize;
  uint32_t m_nBufWinStart;

  uint32_t m_nOverlayMax;       // Number of overlays allocated (limited by mem)
  uint32_t m_nOverlayNum;
  sOverlay *m_psOverlay[NUM_OVERLAYS];

  QStatusBar *m_pStatBar;

  QMessageBox msgBox;

  bool m_bBufOK;
  quint64 m_nPosEof;            // Byte count at EOF
};

#endif
