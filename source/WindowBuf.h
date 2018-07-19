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


#pragma once

#include "DocLog.h"

// For now, we only ever use MAX_BUF_WINDOW bytes, even though we
// have allocated MAX_BUF bytes up front. I might change this
// later. We don't want the window size to be too large as it
// could have an impact on performance.
#define MAX_BUF				262144L
#define MAX_BUF_WINDOW		131072L
#define MAX_BUF_WINDOW_REV	16384L //1024L

#define NUM_OVERLAYS		500
#define MAX_OVERLAY			500		// 500 bytes

#define NUM_HOLES			10

#define	MAX_BUF_READ_STR	255	// Max number of bytes to fetch in BufReadStr()

typedef struct {
	bool			bEn;					// Enabled? -- not used currently
	unsigned		nStart;					// File position
	unsigned		nLen;					// MCU Length
	BYTE			anData[MAX_OVERLAY];	// Byte data

	// For reporting purposes:
	unsigned		nMcuX;			// Starting MCU X
	unsigned		nMcuY;			// Starting MCU Y
	unsigned		nMcuLen;		// Number of MCUs deleted
	unsigned		nMcuLenIns;		// Number of MCUs inserted
	int				nDcAdjustY;
	int				nDcAdjustCb;
	int				nDcAdjustCr;

} sOverlay;

class CwindowBuf
{
public:
	CwindowBuf();
	~CwindowBuf();

public:
	void			SetStatusBar(CStatusBar* pStatBar);

	void			BufLoadWindow(unsigned long nPosition);
	void			BufFileSet(CFile* inFile);
	void			BufFileUnset();
	BYTE			Buf(unsigned long nOffset,bool bClean=false);
	unsigned		BufX(unsigned long nOffset,unsigned nSz,bool bByteSwap=false);

	unsigned char	BufRdAdv1(unsigned long &nOffset,bool bByteSwap);
	unsigned short	BufRdAdv2(unsigned long &nOffset,bool bByteSwap);
	unsigned        BufRdAdv4(unsigned long &nOffset,bool bByteSwap);


	CString			BufReadStr(unsigned long nPosition);
	CString			BufReadUniStr(unsigned long nPosition);
	CString			BufReadUniStr2(unsigned long nPos, unsigned nBufLen);
	CString			BufReadStrn(unsigned long nPosition,unsigned nLen);

	bool			BufSearch(unsigned long nStartPos, unsigned nSearchVal, unsigned nSearchLen,
						   bool bDirFwd, unsigned long &nFoundPos);
	bool			BufSearchX(unsigned long nStartPos, BYTE* anSearchVal, unsigned nSearchLen,
						   bool bDirFwd, unsigned long &nFoundPos);

	bool			OverlayAlloc(unsigned nInd);
	bool			OverlayInstall(unsigned nOvrInd, BYTE* pOverlay,unsigned nLen,unsigned nBegin,
							unsigned nMcuX,unsigned nMcuY,unsigned nMcuLen,unsigned nMcuLenIns,
							int nAdjY,int nAdjCb,int nAdjCr);
	void			OverlayRemove();
	void			OverlayRemoveAll();
	bool			OverlayGet(unsigned nOvrInd, BYTE* &pOverlay,unsigned &nLen,unsigned &nBegin);
	unsigned		OverlayGetNum();
	void			ReportOverlays(CDocLog* pLog);
	
	bool			GetBufOk();
	unsigned long	GetPosEof();

private:
	void			Reset();


private:
	BYTE*			m_pBuffer;
	CFile*			m_pBufFile;
	unsigned long	m_nBufWinSize;
	unsigned long	m_nBufWinStart;

	unsigned		m_nOverlayMax;	// Number of overlays allocated (limited by mem)
	unsigned		m_nOverlayNum;
	sOverlay*		m_psOverlay[NUM_OVERLAYS];

	CStatusBar*		m_pStatBar;

	bool			m_bBufOK;
	unsigned long	m_nPosEof;	// Byte count at EOF

};
