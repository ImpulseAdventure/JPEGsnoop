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

#include "stdafx.h"

#include "WindowBuf.h"

// Clear out internal members
void CwindowBuf::Reset()
{
	// File handling
	BufOK = false;			// Initialize the buffer to not loaded yet
	m_pBufFile = NULL;		// No file open yet

}

CwindowBuf::CwindowBuf()
{
	pszBuffer = new BYTE[MAX_BUF];
	if (!pszBuffer) {
		AfxMessageBox("ERROR: Not enough memory for File Buffer");
		exit(1);
	}


	m_pStatBar = NULL;
	dummyOverrun = 0x6969;

	Reset();

	m_nOverlayNum = 0;
	m_nOverlayMax = 0;
	// Initialize all overlays as not defined.
	// Only create space for them as required
	for (unsigned ind=0;ind<NUM_OVERLAYS;ind++) {
		m_psOverlay[ind] = NULL;
	}

#ifdef DEV_MODE
	HoleRemoveAll();
#endif

	// Clear any inserts
	m_nInsertEn = false;
	m_nInsertStartByte = 0;
	m_nInsertVal = 0x00;

}

CwindowBuf::~CwindowBuf()
{
	if (pszBuffer != NULL) {
		delete pszBuffer;
		pszBuffer = NULL;
		BufOK = false;
	}

	// Clear up overlays

	for (unsigned ind=0;ind<NUM_OVERLAYS;ind++) {
		if (m_psOverlay[ind]) {
			delete m_psOverlay[ind];
			m_psOverlay[ind] = NULL;
		}
	}
}

void CwindowBuf::BufFileSet(CFile* inFile)
{
	ASSERT(inFile);
	if (!inFile) { AfxMessageBox("ERROR: BufFileSet() with NULL inFile"); }

	m_pBufFile = inFile;
	pos_eof = (unsigned long) m_pBufFile->GetLength();
	if (pos_eof == 0) {
		m_pBufFile = NULL;
		AfxMessageBox("ERROR: BufFileSet() File length zero");
	}
}

// Called to mark the buffer as closed!
// Typically done when we've finished processing a file
// This avoids problems where we try to read a buffer
// that has already been terminated.
void CwindowBuf::BufFileUnset()
{
	if (m_pBufFile) {
		m_pBufFile = NULL;	
	}
}

bool CwindowBuf::BufSearch(unsigned long start_pos, unsigned search_val, unsigned search_len,
						   bool dir_fwd, unsigned long &found_pos)
{
	// Save the current position
	unsigned long   cur_pos;
	unsigned        cur_val;

	cur_pos = start_pos;
	bool done = false;
	bool found = false;
	while (!done) {

		if (dir_fwd) {
			cur_pos++;
			if (cur_pos+(search_len-1) >= pos_eof) {
				done = true;
			}
		} else {
			if (cur_pos > 0) {
				cur_pos--;
			} else {
				done = true;
			}
		}

		if (search_len == 4) {
			cur_val = (Buf(cur_pos+0)<<24) + (Buf(cur_pos+1)<<16) + (Buf(cur_pos+2)<<8) + Buf(cur_pos+3);
		} else if (search_len == 3) {
			cur_val = (Buf(cur_pos+0)<<16) + (Buf(cur_pos+1)<<8) + Buf(cur_pos+2);
		} else if (search_len == 2) {
			cur_val = (Buf(cur_pos+0)<<8) + Buf(cur_pos+1);
		} else if (search_len == 1) {
			cur_val = Buf(cur_pos+0);
		} else {
			AfxMessageBox("ERROR: Unexpected search_len");
			cur_val = 0x0000;
		}
		if (cur_val == search_val) {
			found = true;
			done = true;
		}

	}


	found_pos = cur_pos;
	return found;
}

void CwindowBuf::SetStatusBar(CStatusBar* pStatBar)
{
	m_pStatBar = pStatBar;
}

// Try to find a variable-length byte string
bool CwindowBuf::BufSearchX(unsigned long start_pos, BYTE* search_val, unsigned search_len,
						   bool dir_fwd, unsigned long &found_pos)
{
	// Save the current position
	unsigned long   cur_pos;
	unsigned        byte_cur;
	unsigned		byte_search;

	unsigned long	cur_pos_offset;

	unsigned long	match_start_pos = 0;
	bool			match_start = false;
	bool			match_on = false;


	cur_pos_offset = 0;
	cur_pos = start_pos;
	bool done = false;
	bool found = false;		// Matched entire search string
	while (!done) {

		if (dir_fwd) {
			cur_pos++;
			if (cur_pos+(search_len-1) >= pos_eof) {
				done = true;
			}
		} else {
			if (cur_pos > 0) {
				cur_pos--;
			} else {
				done = true;
			}
		}

		if ((cur_pos % 1024)==0) {
			CString tmpStr;
			if (m_pStatBar) {
				tmpStr.Format("Searching %3u%% (%lu of %lu)...",(cur_pos*100/pos_eof),cur_pos,pos_eof);
				m_pStatBar->SetPaneText(0,tmpStr);
			}
		}

		byte_search = search_val[cur_pos_offset];
		byte_cur = Buf(cur_pos);
		if (byte_search == byte_cur) {
			if (!match_on) {
				// Since we aren't in match mode, we are beginning a new
				// sequence, so save the starting position in case we
				// have to rewind
				match_start_pos = cur_pos;
				match_on = true;
			}
			cur_pos_offset++;
		} else {
			if (match_on) {
				// Since we were in a sequence of matches, but ended early,
				// we now need to reset our position to just after the start
				// of the previous 1st match.
				cur_pos = match_start_pos;
				match_on = false;
			}
			cur_pos_offset = 0;
		}
		if (cur_pos_offset >= search_len) {
			// We matched the entire length of our search string!
			found = true;
			done = true;
		}

	}

	if (m_pStatBar) {
		m_pStatBar->SetPaneText(0,"Done");
	}


	if (found) {
		found_pos = match_start_pos;
	}
	return found;

}


void CwindowBuf::BufLoadWindow(unsigned long position)
{

	// We must not try to perform a seek command on a CFile that
	// has already been closed, so we must check first.

	if (m_pBufFile) {
		/*
		CString tmpStr;
		tmpStr.Format("** BufLoadWindow @ 0x%08X",position);
		log->AddLine(tmpStr);
		*/

		// Initialize to bad values
		BufOK = false;
		buf_win_size = 0;
		buf_win_start = 0;

		// For now, just read 128KB starting at current position
		// Later on, we will need to do range checking and even start
		// at a position some distance before the "position" to allow
		// for some reverse seeking without refetching data.

		long position_adj;
		if (position >= MAX_BUF_WINDOW_REV) {
			position_adj = position - MAX_BUF_WINDOW_REV;
		} else {
			position_adj = 0;
		}

		// *** Need to ensure that we don't try to read past the end
		// of the file!!! Note that I have encountered a JPEG that has
		// Canon makernote fields with OffsetValue > 0xFFFF0000! This
		// will cause a BufLoadWindow() well past the end of file. This
		// may be for DRM purposes? Need to be robust.
		if (position_adj >= (long)pos_eof) {

			// ERROR! For now, just do this silently
			// We're not going to throw up any errors unless we can
			// limit the number that we'll display to the user!
			// The current code will not reset the position of the
			// buffer, so every byte will try to reset it.
			// FIXME
			return;
		}


		unsigned long val;
		val = (unsigned long)m_pBufFile->Seek(position_adj,CFile::begin);
		val = (unsigned long)m_pBufFile->Read(pszBuffer,MAX_BUF_WINDOW);
		if (val <= 0) {
			// Failed to read anything!
			//CAL!
			// ERROR!
			return;
		} else {
			// Read OK
			// Recalculate bounds
			BufOK = true;
			buf_win_start = position_adj;
			buf_win_size = val;
		}

	} else {
		AfxMessageBox("ERROR: BufLoadWindow() and no file open");
	}
}


bool CwindowBuf::OverlayAlloc(unsigned ind)
{
	//unsigned ind = m_nOverlayMax;

	if (m_psOverlay[ind]) {
		// Already allocated, move on
		return true;

	} else if (ind >= NUM_OVERLAYS) {
		AfxMessageBox("ERROR: Maximum number of overlays reached");
		return false;

	} else {
		m_psOverlay[ind] = new sOverlay();
		if (!m_psOverlay[ind]) {
			AfxMessageBox("NOTE: Out of memory for extra file overlays");
			return false;
		} else {
			memset(m_psOverlay[ind],0,sizeof(sOverlay));
			// FIXME may not be necessary
			m_psOverlay[ind]->bEn = false;
			m_psOverlay[ind]->nStart = 0;
			m_psOverlay[ind]->nLen = 0;

			m_psOverlay[ind]->nMcuX = 0;
			m_psOverlay[ind]->nMcuY = 0;
			m_psOverlay[ind]->nMcuLen = 0;
			m_psOverlay[ind]->nMcuLenIns = 0;
			m_psOverlay[ind]->nDcAdjustY = 0;
			m_psOverlay[ind]->nDcAdjustCb = 0;
			m_psOverlay[ind]->nDcAdjustCr = 0;

			// FIXME: Need to ensure that this is right
			if (ind+1 >= m_nOverlayMax) {
				m_nOverlayMax = ind+1;
			}
			//m_nOverlayMax++;

			return true;
		}
	}
}



void CwindowBuf::ReportOverlays(CDocLog* pLog)
{
	CString	tmpStr;

	if (m_nOverlayNum>0) {
		tmpStr.Format("  Buffer Overlays active: %u",m_nOverlayNum);
		pLog->AddLine(tmpStr);
		for (unsigned ind=0;ind<m_nOverlayNum;ind++) {
			if (m_psOverlay[ind]) {
				tmpStr.Format("    %03u: MCU[%4u,%4u] MCU DelLen=[%2u] InsLen=[%2u] DC Offset YCC=[%5d,%5d,%5d] Overlay Byte Len=[%4u]",
					ind,m_psOverlay[ind]->nMcuX,m_psOverlay[ind]->nMcuY,m_psOverlay[ind]->nMcuLen,m_psOverlay[ind]->nMcuLenIns,
					m_psOverlay[ind]->nDcAdjustY,m_psOverlay[ind]->nDcAdjustCb,m_psOverlay[ind]->nDcAdjustCr,
					m_psOverlay[ind]->nLen);
				pLog->AddLine(tmpStr);
			}
		}
		pLog->AddLine("");
	}
}

#ifdef DEV_MODE
void CwindowBuf::HoleInstall(unsigned nStart,unsigned nLen)
{
	if (m_nHoleCnt < NUM_HOLES) {
		m_nHoleStart[m_nHoleCnt] = nStart;
		m_nHoleLen[m_nHoleCnt] = nLen;
		m_bHoleEn[m_nHoleCnt] = true;
		m_nHoleCnt++;
	} else {
		AfxMessageBox("ERROR: Too many File Holes installed");
	}
}

void CwindowBuf::HoleRemoveAll()
{
	m_nHoleCnt = 0;
	memset(m_nHoleStart,0,sizeof(m_nHoleStart));
	memset(m_nHoleLen,0,sizeof(m_nHoleLen));
	memset(m_bHoleEn,0,sizeof(m_bHoleEn));
}

void CwindowBuf::HoleRemoveLast()
{
	if (m_nHoleCnt > 0) {
		m_nHoleCnt--;
		m_nHoleStart[m_nHoleCnt] = 0;
		m_nHoleLen[m_nHoleCnt] = 0;
		m_bHoleEn[m_nHoleCnt] = false;
	} else {
		AfxMessageBox("ERROR: No more File Holes to remove");
	}
}
#endif


bool CwindowBuf::OverlayInstall(unsigned nOvrInd, BYTE* pOverlay,unsigned nLen,unsigned nBegin,
								unsigned nMcuX,unsigned nMcuY,unsigned nMcuLen,unsigned nMcuLenIns,
								int nAdjY,int nAdjCb,int nAdjCr)
{
	// Ensure that the overlay is allocated, and allocate it
	// if required. Fail out if we can't add (or run out of space)
	if (!OverlayAlloc(m_nOverlayNum)) {
		return false;
	}

	if (nLen < MAX_OVERLAY) {
		m_psOverlay[m_nOverlayNum]->bEn = true;
		m_psOverlay[m_nOverlayNum]->nLen = nLen;
		for (unsigned i=0;i<MAX_OVERLAY;i++) {
			m_psOverlay[m_nOverlayNum]->anData[i] = (i<nLen)?pOverlay[i]:0x00;
		}
		m_psOverlay[m_nOverlayNum]->nStart = nBegin;

		// For reporting, save the extra data
		m_psOverlay[m_nOverlayNum]->nMcuX = nMcuX;
		m_psOverlay[m_nOverlayNum]->nMcuY = nMcuY;
		m_psOverlay[m_nOverlayNum]->nMcuLen = nMcuLen;
		m_psOverlay[m_nOverlayNum]->nMcuLenIns = nMcuLenIns;
		m_psOverlay[m_nOverlayNum]->nDcAdjustY = nAdjY;
		m_psOverlay[m_nOverlayNum]->nDcAdjustCb = nAdjCb;
		m_psOverlay[m_nOverlayNum]->nDcAdjustCr = nAdjCr;

		m_nOverlayNum++;
	} else {
		AfxMessageBox("ERROR: CwindowBuf:OverlayInstall() overlay too large");
		return false;
	}

	return true;
}

// Remove latest overlay entry
void CwindowBuf::OverlayRemove()
{
	if (m_nOverlayNum<=0) {
		return;
	}
	m_nOverlayNum--;

	if (m_psOverlay[m_nOverlayNum]) {
		m_psOverlay[m_nOverlayNum]->bEn = false;
		// Don't need to delete it, might as well reuse buffer
		//delete m_psOverlay[m_nOverlayNum];
		//m_psOverlay[m_nOverlayNum] = NULL;
	}
}

void CwindowBuf::OverlayRemoveAll()
{
	m_nOverlayNum = 0;
	for (unsigned ind=0;ind<m_nOverlayMax;ind++) {
		if (m_psOverlay[ind]) {
			m_psOverlay[ind]->bEn = false;
		}
	}
}

// FIXME: I think the following should be using nOvrInd instead of relying on
// hardcoded value of m_nOverlayNum in all array subscripts!!!
bool CwindowBuf::OverlayGet(unsigned nOvrInd, BYTE* &pOverlay,unsigned &nLen,unsigned &nBegin)
{
	if (m_psOverlay[nOvrInd]) {
		pOverlay = m_psOverlay[nOvrInd]->anData;
		nLen = m_psOverlay[nOvrInd]->nLen;
		nBegin = m_psOverlay[nOvrInd]->nStart;
		return m_psOverlay[nOvrInd]->bEn;
	} else {
		return false;
	}
}

unsigned CwindowBuf::OverlayGetNum()
{
	return m_nOverlayNum;
}

// Replaces the direct buffer access with a managed
// refillable window.
// "clean" means that it doesn't allow any overlay buffer to
// modify the data (i.e. it will return the actual data).
inline BYTE CwindowBuf::Buf(unsigned long offset,bool clean)
{

	// We are requesting address "offset"
	// Our current window runs from "buf_win_start...buf_win_end" (buf_win_size)
	// Therefore, our relative addr is offset-buf_win_start

	long win_rel;

	BYTE		nCurVal = 0;
	unsigned	bInOvrWindow = false;

	if (!m_pBufFile) {
		// FIXME Open it!!! or give error

	}
	ASSERT(m_pBufFile);

	// Allow for overlay buffer capability

	if (!clean) {

		// First, handle any "holes"
		// If our position is before the hole -- no change
		// If our position is in the hole or after -- shift by hole length

#ifdef DEV_MODE
		// REQUIREMENT: Holes must be in ascending file position order!!
		unsigned offset_orig = offset;
		for (unsigned ind=0;ind<m_nHoleCnt;ind++) {
			if (m_bHoleEn[ind]) {
				if (offset_orig < m_nHoleStart[ind]) {
					//
				} else {
					offset += m_nHoleLen[ind];
				}
			}
		}
#endif

		// Handle any byte insertions...
		if (m_nInsertEn) {
			if (offset == m_nInsertStartByte) {
				return m_nInsertVal;
			} else if (offset > m_nInsertStartByte) {
				offset--;
			}
		}


		// Now handle any overlays
		for (unsigned ind=0;ind<m_nOverlayNum;ind++) {
			if (m_psOverlay[ind]) {
				if (m_psOverlay[ind]->bEn) {
					if ((offset >= m_psOverlay[ind]->nStart) && 
						(offset < m_psOverlay[ind]->nStart + m_psOverlay[ind]->nLen))
					{
						nCurVal = m_psOverlay[ind]->anData[offset-m_psOverlay[ind]->nStart];
						bInOvrWindow = true;
					}
				}
			}
		}
		if (bInOvrWindow) {

			// Before we return, make sure that the real buffer handles this region!
			win_rel = offset-buf_win_start;
			if ((win_rel >= 0) && (win_rel < (long)buf_win_size)) {
			} else {
				// Address is outside of current window
				BufLoadWindow(offset);
			}

			return nCurVal;
		}
	}

	win_rel = offset-buf_win_start;
	if ((win_rel >= 0) && (win_rel < (long)buf_win_size)) {
		// Address is within current window
		return pszBuffer[win_rel];
	} else {
		// Address is outside of current window
		BufLoadWindow(offset);

		// Now we assume that the address is in range
		// buf_win_start has now been updated
		//CAL! We should check this!!!
		win_rel = offset-buf_win_start;

		// Now recheck the window
		//CAL! This should be rewritten in a better manner
		if ((win_rel >= 0) && (win_rel < (long)buf_win_size)) {
			return pszBuffer[win_rel];
		} else {
			// Still bad after refreshing window, so it must be bad addr
			BufOK = false;
			//CAL! log->AddLine(_T("ERROR: Overread buffer - file may be truncated"),9);
			//CAL! *** NEED TO REPORT ERROR SOMEHOW!
			return 0;
		}
	}
}

unsigned CwindowBuf::BufX(unsigned long offset,unsigned sz,bool byteswap)
{
	long win_rel;
	unsigned val = 0;

	ASSERT(m_pBufFile);

	win_rel = offset-buf_win_start;
	if ((win_rel >= 0) && (win_rel+sz < buf_win_size)) {
		// Address is within current window
		if (!byteswap) {
			if (sz==4) {
				return ( (pszBuffer[win_rel+0]<<24) + (pszBuffer[win_rel+1]<<16) + (pszBuffer[win_rel+2]<<8) + (pszBuffer[win_rel+3]) );
			} else if (sz==2) {
				return ( (pszBuffer[win_rel+0]<<8) + (pszBuffer[win_rel+1]) );
			} else if (sz==1) {
				return (pszBuffer[win_rel+0]);
			} else {
				AfxMessageBox("ERROR: BufX() with bad sz");
				return 0;
			}
		} else {
			if (sz==4) {
				return ( (pszBuffer[win_rel+3]<<24) + (pszBuffer[win_rel+2]<<16) + (pszBuffer[win_rel+1]<<8) + (pszBuffer[win_rel+0]) );
			} else if (sz==2) {
				return ( (pszBuffer[win_rel+1]<<8) + (pszBuffer[win_rel+0]) );
			} else if (sz==1) {
				return (pszBuffer[win_rel+0]);
			} else {
				AfxMessageBox("ERROR: BufX() with bad sz");
				return 0;
			}
		}

	} else {
		// Address is outside of current window
		BufLoadWindow(offset);

		// Now we assume that the address is in range
		// buf_win_start has now been updated
		//CAL! We should check this!!!
		win_rel = offset-buf_win_start;

		// Now recheck the window
		//CAL! This should be rewritten in a better manner
		if ((win_rel >= 0) && (win_rel+sz < buf_win_size)) {
			if (!byteswap) {
				if (sz==4) {
					return ( (pszBuffer[win_rel+0]<<24) + (pszBuffer[win_rel+1]<<16) + (pszBuffer[win_rel+2]<<8) + (pszBuffer[win_rel+3]) );
				} else if (sz==2) {
					return ( (pszBuffer[win_rel+0]<<8) + (pszBuffer[win_rel+1]) );
				} else if (sz==1) {
					return (pszBuffer[win_rel+0]);
				} else {
					AfxMessageBox("ERROR: BufX() with bad sz");
					return 0;
				}
			} else {
				if (sz==4) {
					return ( (pszBuffer[win_rel+3]<<24) + (pszBuffer[win_rel+2]<<16) + (pszBuffer[win_rel+1]<<8) + (pszBuffer[win_rel+0]) );
				} else if (sz==2) {
					return ( (pszBuffer[win_rel+1]<<8) + (pszBuffer[win_rel+0]) );
				} else if (sz==1) {
					return (pszBuffer[win_rel+0]);
				} else {
					AfxMessageBox("ERROR: BufX() with bad sz");
					return 0;
				}
			}
		} else {
			// Still bad after refreshing window, so it must be bad addr
			BufOK = false;
			//CAL! log->AddLine(_T("ERROR: Overread buffer - file may be truncated"),9);
			//CAL! *** NEED TO REPORT ERROR SOMEHOW!
			return 0;
		}
	}
}


// Note: Doesn't change file pointer
CString CwindowBuf::BufReadStr(unsigned long position)
{
	// Try to read a NULL-terminated string from file offset "position"
	// up to a maximum of 255 bytes. Result is max length 255 and NULL-terminated.
	CString rd_str;
	char	rd_ch;
	bool done = false;
	unsigned index = 0;

	while (!done)
	{
		rd_ch = (char)Buf(position+index);

		rd_str += rd_ch;
		index++;
		if (rd_ch == 0) {
			done = true;
		} else if (index >= 255) {
			done = true;
			rd_str += char(0);
		}
	}
	return rd_str;
}

// Unicode version (16-bit chars), conver to normal chars
// Note: Doesn't change file pointer
//CAL! BUG #1112
CString CwindowBuf::BufReadUniStr(unsigned long position)
{
	// Try to read a NULL-terminated string from file offset "position"
	// up to a maximum of 255 bytes. Result is max length 255 and NULL-terminated.
	CString rd_str;
	char	rd_ch;
	bool done = false;
	unsigned index = 0;

	while (!done)
	{
		rd_ch = (char)Buf(position+index);

		// Make sure it is a printable char!
		// NO! Can't do this because it will cause the strlen()
		// call in the calling function to get the wrong length
		// since it's no longer null-terminated!
		// SKIP this for now.
//		if (isprint(rd_ch)) {
//			rd_str += rd_ch;
//		} else {
//			rd_str += ".";
//		}
		rd_str += rd_ch;

		index+=2;
		if (rd_ch == 0) {
			done = true;
		} else if (index >= (255*2)) {
			done = true;
			rd_str += char(0);
		}
	}
	return rd_str;
}



CString CwindowBuf::BufReadStrn(unsigned long position,unsigned len)
{
	// Try to read a fixed-length string from file offset "position"
	// up to a maximum of "len" bytes. Result is length "len" and NULL-terminated.
	CString rd_str;
	char	rd_ch;

	if (len > 0) {
		for (unsigned ind=0;ind<len;ind++)
		{
			rd_ch = (char)Buf(position+ind);
			rd_str += rd_ch;
		}
		rd_str += char(0);
		return rd_str;
	} else {
		return char(0);
	}
}
