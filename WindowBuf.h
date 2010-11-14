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

#include "DocLog.h"

// For now, we only ever use MAX_BUF_WINDOW bytes, even though we
// have allocated MAX_BUF bytes up front. I might change this
// later. We don't want the window size to be too large as it
// could have an impact on performance.
#define MAX_BUF				262144L
#define MAX_BUF_WINDOW		131072L
#define MAX_BUF_WINDOW_REV	16384L //1024L

#define NUM_OVERLAYS 500
#define MAX_OVERLAY 500		// 500 bytes

#define NUM_HOLES 10

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

	void			BufLoadWindow(unsigned long position);
	void			BufFileSet(CFile* inFile);
	void			BufFileUnset();
	BYTE			Buf(unsigned long offset,bool clean=false);
	unsigned		BufX(unsigned long offset,unsigned sz,bool byteswap=false);

	CString			BufReadStr(unsigned long position);
	CString			BufReadUniStr(unsigned long position);
	CString			BufReadStrn(unsigned long position,unsigned len);

	bool			BufSearch(unsigned long start_pos, unsigned search_val, unsigned search_len,
						   bool dir_fwd, unsigned long &found_pos);
	bool			BufSearchX(unsigned long start_pos, BYTE* search_val, unsigned search_len,
						   bool dir_fwd, unsigned long &found_pos);

	bool			OverlayAlloc(unsigned ind);
	bool			OverlayInstall(unsigned nOvrInd, BYTE* pOverlay,unsigned nLen,unsigned nBegin,
							unsigned nMcuX,unsigned nMcuY,unsigned nMcuLen,unsigned nMcuLenIns,
							int nAdjY,int nAdjCb,int nAdjCr);
	void			OverlayRemove();
	void			OverlayRemoveAll();
	bool			OverlayGet(unsigned nOvrInd, BYTE* &pOverlay,unsigned &nLen,unsigned &nBegin);
	unsigned		OverlayGetNum();
	void			ReportOverlays(CDocLog* pLog);

#ifdef DEV_MODE
	void			HoleInstall(unsigned nStart,unsigned nLen);
	void			HoleRemoveLast();
	void			HoleRemoveAll();
#endif


private:
	void			Reset();


private:
	BYTE*			pszBuffer;
	unsigned		dummyOverrun;
	CFile*			m_pBufFile;
	unsigned long	buf_win_size;
	unsigned long	buf_win_start;

	unsigned		m_nOverlayMax;	// Number of overlays allocated (limited by mem)
	unsigned		m_nOverlayNum;
	sOverlay*		m_psOverlay[NUM_OVERLAYS];

public: // FIXME for now
#ifdef DEV_MODE
	unsigned		m_nHoleCnt;
	bool			m_bHoleEn[NUM_HOLES];
	unsigned		m_nHoleStart[NUM_HOLES];
	unsigned		m_nHoleLen[NUM_HOLES];
#endif

	bool			m_nInsertEn;
	unsigned		m_nInsertStartByte;
	BYTE			m_nInsertVal;

private:

	CStatusBar*		m_pStatBar;

public:
	bool			BufOK;
	unsigned long	pos_eof;	// Byte count at EOF

};
