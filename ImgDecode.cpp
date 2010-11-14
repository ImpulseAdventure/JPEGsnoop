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

#include "ImgDecode.h"
#include "snoop.h"
#include <math.h>

#include "JPEGsnoop.h"


// Scan decode errors (in m_nScanBuffPtr_err[] buffer array)
#define SCANBUF_OK 0
#define SCANBUF_BADMARK 1
#define SCANBUF_RST 2

//#define IDCT_FIXEDPT

// Do we stop (fatal) during scan decode if 0xFF (and not pad)?
// FIXME: Make this a config option
//#define SCAN_BAD_MARKER_STOP

// Reset decoding state for start of new decode
// Note that we don't touch the DQT or DHT entries as
// those are set at different times versus reset (sometimes
// before Reset() ).
void CimgDecode::Reset()
{
	DecodeRestartScanBuf(0);
	DecodeRestartDcState();

	m_bRestartRead = false;	// No restarts seen yet
	m_nRestartRead = 0;

	m_nImgSizeXPartMcu = 0;
	m_nImgSizeYPartMcu = 0;
	m_nImgSizeX  = 0;
	m_nImgSizeY  = 0;
	m_nMcuWidth  = 0;
	m_nMcuHeight = 0;
	m_nMcuXMax   = 0;
	m_nMcuYMax   = 0;
	m_nBlkXMax   = 0;
	m_nBlkYMax   = 0;

	m_bBrightValid = false;
	m_nBrightY  = -32768;
	m_nBrightCb = -32768;
	m_nBrightCr = -32768;
	m_nBrightR = 0;
	m_nBrightG = 0;
	m_nBrightB = 0;
	m_ptBrightMcu.x = 0;
	m_ptBrightMcu.y = 0;

	m_bAvgYValid = false;
	m_nAvgY = 0;

	// No marker
	m_bMarkBlock = false;
	m_nMarkBlockX = 0;
	m_nMarkBlockY = 0;

	m_nPreviewModeDone = PREVIEW_NONE;	// Not calculated yet!

	// If a DIB has been generated, release it!
	if (my_DIB_ready) {
		my_DIBtmp.Kill();
		my_DIB_ready = false;
	}

	if (my_hist_rgb_DIB_ready) {
		my_hist_rgb_DIB.Kill();
		my_hist_rgb_DIB_ready = false;
	}

	if (my_hist_y_DIB_ready) {
		my_hist_y_DIB.Kill();
		my_hist_y_DIB_ready = false;
	}

	if (m_pMcuFileMap) {
		delete [] m_pMcuFileMap;
		m_pMcuFileMap = NULL;
	}

	if (m_pBlkValY) {
		delete [] m_pBlkValY;
		m_pBlkValY = NULL;
	}
	if (m_pBlkValCb) {
		delete [] m_pBlkValCb;
		m_pBlkValCb = NULL;
	}
	if (m_pBlkValCr) {
		delete [] m_pBlkValCr;
		m_pBlkValCr = NULL;
	}

	if (m_pPixValY) {
		delete [] m_pPixValY;
		m_pPixValY = NULL;
	}
	if (m_pPixValCb) {
		delete [] m_pPixValCb;
		m_pPixValCb = NULL;
	}
	if (m_pPixValCr) {
		delete [] m_pPixValCr;
		m_pPixValCr = NULL;
	}

	// Haven't warned about anything yet
	if (!m_bScanErrorsDisable) {
		m_nWarnBadScanNum = 0;
	}
	m_nWarnYccClipNum = 0;

	// Reset the view
	m_nPreviewPosX = 0;
	m_nPreviewPosY = 0;
	m_nPreviewSizeX = 0;
	m_nPreviewSizeY = 0;

/*
	if (m_abMarkedMcuMap) {
		delete m_abMarkedMcuMap;
		m_abMarkedMcuMap = NULL;
	}
	m_nMarkedMcuLastXY = 0;
*/

}

// This constructor is called only once by Document class
CimgDecode::CimgDecode(CDocLog* pLog, CwindowBuf* pWBuf)
{
	m_bVerbose = false;

	ASSERT(pLog);
	ASSERT(pWBuf);
	m_pLog = pLog;
	m_pWBuf = pWBuf;

	m_pStatBar = NULL;
	my_DIB_ready = false;
	my_hist_rgb_DIB_ready = false;
	my_hist_y_DIB_ready = false;

	m_bHistEn = false;
	m_bStatClipEn = false;	// UNUSED

	m_pMcuFileMap = NULL;
	m_pBlkValY = NULL;
	m_pBlkValCb = NULL;
	m_pBlkValCr = NULL;
	m_pPixValY = NULL;
	m_pPixValCb = NULL;
	m_pPixValCr = NULL;


	// Reset the image decoding state
	Reset();


	m_nImgSizeXPartMcu = 0;
	m_nImgSizeYPartMcu = 0;
	m_nImgSizeX = 0;
	m_nImgSizeY = 0;

	// Detailed VLC Decode mode
	m_bDetailVlc = false;
	m_nDetailVlcX = 0;
	m_nDetailVlcY = 0;
	m_nDetailVlcLen = 1;
	
	m_bDecodeScanAc = true;


	// Set up the IDCT lookup tables
	PrecalcIdct();

	HuffMaskLookupGen();

	// Allocate Marker MCU map
	m_abMarkedMcuMap = new bool[MARKMCUMAP_SIZE];
	if (!m_abMarkedMcuMap) {
		AfxMessageBox("ERROR: Not enough memory for Image Decoder MCU Marker Map");
		//exit(1);
	}


	// The following contain information that is set by
	// the JFIF Decoder. We can only reset them here during
	// the constructor and later by explicit call by JFIF Decoder.
	FreshStart();

	ResetNewFile();

	// We don't call SetPreviewMode() here because it would
	// automatically try to recalculate the view (but nothing ready yet)
	m_nPreviewMode = PREVIEW_RGB;
	SetPreviewZoom(false,false,true,PRV_ZOOM_12);

	m_bViewOverlaysMcuGrid = false;

	// Start off with no YCC offsets for CalcChannelPreview()
	SetPreviewYccOffset(0,0,0,0,0);

	SetPreviewMcuInsert(0,0,0);

}

CimgDecode::~CimgDecode()
{
	if (m_pMcuFileMap) {
		delete [] m_pMcuFileMap;
		m_pMcuFileMap = NULL;
	}


	if (m_pBlkValY) {
		delete [] m_pBlkValY;
		m_pBlkValY = NULL;
	}
	if (m_pBlkValCb) {
		delete [] m_pBlkValCb;
		m_pBlkValCb = NULL;
	}
	if (m_pBlkValCr) {
		delete [] m_pBlkValCr;
		m_pBlkValCr = NULL;
	}

	if (m_pPixValY) {
		delete [] m_pPixValY;
		m_pPixValY = NULL;
	}
	if (m_pPixValCb) {
		delete [] m_pPixValCb;
		m_pPixValCb = NULL;
	}
	if (m_pPixValCr) {
		delete [] m_pPixValCr;
		m_pPixValCr = NULL;
	}

	if (m_abMarkedMcuMap) {
		delete m_abMarkedMcuMap;
		m_abMarkedMcuMap = NULL;
	}

}

// Called by Jfif Decoder when we begin a new file
void CimgDecode::FreshStart()
{
	ResetDhtLookup();
	ResetDqtTables();

	m_bImgDetailsSet = false;
	m_nNumSofComps = 0;

	m_nPrecision = 0; // Default to "precision not set"

	m_bScanErrorsDisable = false;

}

// Reset that is called when we open up a new file
// Everything gets reset properly.
void CimgDecode::ResetNewFile()
{

	// Reset the markers
	if (m_abMarkedMcuMap) {
		memset(m_abMarkedMcuMap, 0, (MARKMCUMAP_SIZE*sizeof(bool)) );
	}
	m_nMarkedMcuLastXY = 0;

	m_nMarkersBlkNum = 0;


}

void CimgDecode::SetStatusBar(CStatusBar* pStatBar)
{
	m_pStatBar = pStatBar;
}


void CimgDecode::SetStatusText(CString str)
{
	// Make sure that we have been connected to the status
	// bar of the main window first! Note that it is jpegsnoopDoc
	// that sets this variable.
	if (m_pStatBar) {
		m_pStatBar->SetPaneText(0,str);
	}
}

// Clears the DQT entries
void CimgDecode::ResetDqtTables()
{
	memset(m_anDqtCoeff,0,MAX_DHT*64*sizeof(unsigned));
	memset(m_anDqtCoeffZz,0,MAX_DHT*64*sizeof(unsigned));

	// Force entries to an invalid value. This makes
	// sure that we have to get a valid SetDqtTables() call
	// from JfifDecode first.
	for (unsigned i=0;i<8;i++) {
		m_anDqtTblSel[i] = -1;
	}
	m_nNumSofComps = 0;
}

// This should be called by the JFIF decoder any time we start a new file
void CimgDecode::ResetDhtLookup()
{

	memset(m_anDhtLookupSetMax,0,2*sizeof(unsigned));
	memset(m_anDhtLookupSize,0,2*4*sizeof(unsigned));
	memset(m_anDhtLookup_bitlen,0,2*4*180*sizeof(unsigned));
	memset(m_anDhtLookup_bits,0,2*4*180*sizeof(unsigned));
	memset(m_anDhtLookup_mask,0,2*4*180*sizeof(unsigned));
	memset(m_anDhtLookup_code,0,2*4*180*sizeof(unsigned));
	memset(m_anDhtLookupfast,0xFFFFFFFF,2*4*(2<<DHT_FAST_SIZE)*sizeof(unsigned));
	memset(m_anDhtHisto,0,2*4*16*sizeof(unsigned));

	// Force entries to an invalid value. This makes
	// sure that we have to get a valid SetDhtTables() call
	// from JfifDecode first.
	for (unsigned int nClass=0;nClass<2;nClass++)
	{
		for (unsigned int nDestId=0;nDestId<4;nDestId++)
		{
			m_anDhtTblSel[nClass][nDestId] = -1;
		}
	}

	m_nNumSosComps = 0;
}


// NOTE: Asynchronously called by JFIF Decoder
// The nInd index has already been remapped from zigzag order to normal
// The nIndzz index is the original zigzag order index
bool CimgDecode::SetDqtEntry(unsigned nSet, unsigned nInd, unsigned nIndzz, unsigned nCoeff)
{
	//CAL! Should probably define a different constant instead of MAX_DHT!
	if ((nSet < MAX_DHT) && (nInd < 64)) {
		m_anDqtCoeff[nSet][nInd] = nCoeff;

		// Save a copy that represents the original zigzag order
		// This is used by the IDCT logic
		m_anDqtCoeffZz[nSet][nIndzz] = nCoeff;

		//CAL! Debug
		/*
		CString tmpStr;
		tmpStr.Format(_T("*** SetDqtEntry( set=%2u ind=%2u indzz=%2u coef=%4u )"),
			nSet,nInd,nIndzz,nCoeff);
		m_pLog->AddLineWarn(tmpStr);
		*/


	} else {
		// Should never get here!
		ASSERT(false);
		CString tmpStr;
		tmpStr.Format(_T("ERROR: Attempt to set DQT entry out of range (set=%u, ind=%u, coeff=%u)"),
			nSet,nInd,nCoeff);
		AfxMessageBox(tmpStr);
		return false;
	}
	return true;
}

unsigned CimgDecode::GetDqtEntry(unsigned nSet, unsigned nInd)
{
	if ((nSet < MAX_DHT) && (nInd < 64)) {
		return m_anDqtCoeff[nSet][nInd];
	} else {
		ASSERT(false);
		// Should never get here!
		CString tmpStr;
		tmpStr.Format(_T("ERROR: GetDqtEntry(set=%u, ind=%u) out of indexed range"),
			nSet,nInd);
		AfxMessageBox(tmpStr);
		return 0;
	}
}


// NOTE: Asynchronously called by JFIF Decoder
bool CimgDecode::SetDqtTables(unsigned nComp, unsigned nTbl)
{
	if ((nComp < 4) && (nTbl < 4)) {
		m_anDqtTblSel[nComp] = (int)nTbl;
	} else {
		// Should never get here!
		CString tmpStr;
		tmpStr.Format(_T("ERROR: SetDqtTables(comp=%u, tbl=%u) out of indexed range"),
			nComp,nTbl);
		AfxMessageBox(tmpStr);
		return false;
	}
	return true;
}

// The DHT Table select array is stored as:
//   [0,1] = Comp 0: DC,AC
//   [2,3] = Comp 1: DC,AC
//   [4,5] = Comp 2: DC,AC
bool CimgDecode::SetDhtTables(unsigned nComp, unsigned nTblDc, unsigned nTblAc)
{
	if ((nComp < 4) && (nTblDc < 4) && (nTblAc < 4)) {
		m_anDhtTblSel[0][nComp] = (int)nTblDc;
		m_anDhtTblSel[1][nComp] = (int)nTblAc;
	} else {
		// Should never get here!
		CString tmpStr;
		tmpStr.Format(_T("ERROR: SetDhtTables(comp=%u, TblDC=%u TblAC=%u) out of indexed range"),
			nComp,nTblDc,nTblAc);
		AfxMessageBox(tmpStr);
		return false;
	}
	return true;
}

void CimgDecode::GetDhtEntry(unsigned nDcAc,unsigned nComp,unsigned nInd,
							 unsigned &nBits, unsigned &nBitlen, unsigned &nCode)
{
	nBits = m_anDhtLookup_bits[nDcAc][nComp][nInd];
	nBitlen = m_anDhtLookup_bitlen[nDcAc][nComp][nInd];
	nCode = m_anDhtLookup_code[nDcAc][nComp][nInd];
}

unsigned CimgDecode::GetDhtSize(unsigned nDcAc,unsigned nComp)
{
	return m_anDhtLookupSize[nDcAc][nComp];
}

void CimgDecode::SetPrecision(unsigned nPrecision)
{
	m_nPrecision = nPrecision;
}

// NOTE: Asynchronously called by JFIF Decoder
void CimgDecode::SetImageDetails(unsigned nCssX,unsigned nCssY,unsigned nDimX,unsigned nDimY,
								 unsigned nCompsSOF,unsigned nCompsSOS, bool bRstEn,unsigned nRstInterval)
{
	m_bImgDetailsSet = true;
	m_nCssX = nCssX;
	m_nCssY = nCssY;
	m_nDimX = nDimX;
	m_nDimY = nDimY;
	m_nNumSofComps = nCompsSOF;
	m_nNumSosComps = nCompsSOS;
	m_bRestartEn = bRstEn;
	m_nRestartInterval = nRstInterval;
}

void CimgDecode::SetPreviewMode(unsigned nMode)
{
	// Need to check to see if mode has changed. If so, we
	// need to recalculate the temporary preview.
	m_nPreviewMode = nMode;
	CalcChannelPreview();
}

void CimgDecode::SetPreviewYccOffset(unsigned nMcuX,unsigned nMcuY,int nY,int nCb,int nCr)
{
	m_nPreviewShiftY  = nY;
	m_nPreviewShiftCb = nCb;
	m_nPreviewShiftCr = nCr;
	m_nPreviewShiftMcuX = nMcuX;
	m_nPreviewShiftMcuY = nMcuY;

	m_nPreviewModeDone = PREVIEW_NONE;	// Force a recalculate
	CalcChannelPreview();
}

void CimgDecode::GetPreviewYccOffset(unsigned &nMcuX,unsigned &nMcuY,int &nY,int &nCb,int &nCr)
{
	nY = m_nPreviewShiftY;
	nCb = m_nPreviewShiftCb;
	nCr = m_nPreviewShiftCr;
	nMcuX = m_nPreviewShiftMcuX;
	nMcuY = m_nPreviewShiftMcuY;
}

void CimgDecode::SetPreviewMcuInsert(unsigned nMcuX,unsigned nMcuY,int nLen)
{

	m_nPreviewInsMcuX = nMcuX;
	m_nPreviewInsMcuY = nMcuY;
	m_nPreviewInsMcuLen = nLen;

	m_nPreviewModeDone = PREVIEW_NONE;	// Force a recalculate
	CalcChannelPreview();
}

void CimgDecode::GetPreviewMcuInsert(unsigned &nMcuX,unsigned &nMcuY,unsigned &nLen)
{
	nMcuX = m_nPreviewInsMcuX;
	nMcuY = m_nPreviewInsMcuY;
	nLen = m_nPreviewInsMcuLen;
}


// NOTE: Asynchronously called by JFIF Decoder
bool CimgDecode::SetDhtEntry(unsigned nDestId, unsigned nClass, unsigned nInd, unsigned nLen, 
							 unsigned nBits, unsigned nMask, unsigned nCode)
{
	if ( (nDestId >= 4) || (nClass >= 2) || (nInd >= MAX_DHT_CODES) ) {
		AfxMessageBox("ERROR: Attempt to set DHT entry out of range");
		return false;
	}
	m_anDhtLookup_bitlen[nClass][nDestId][nInd] = nLen;
	m_anDhtLookup_bits[nClass][nDestId][nInd] = nBits;
	m_anDhtLookup_mask[nClass][nDestId][nInd] = nMask;
	m_anDhtLookup_code[nClass][nDestId][nInd] = nCode;


	// Record the highest numbered DHT set.
	//CAL! *** ASSUME that there are no missing tables in the sequence!!
	if (nDestId > m_anDhtLookupSetMax[nClass]) {
		m_anDhtLookupSetMax[nClass] = nDestId;
	}

	unsigned bits_msb,bits_extra_len,bits_extra_val,bits_max,fastval;

	// If the variable-length code is short enough, add it to the
	// fast lookup table.
	if (nLen <= DHT_FAST_SIZE) {
		// bits is a left-justified number (assume right-most bits are zero)
		// len is number of leading bits to compare

		//   len      = 5
		//   bits     = 32'b1011_1xxx_xxxx_xxxx_xxxx_xxxx_xxxx_xxxx  (0xB800_0000)
		//   bits_msb =  8'b1011_1xxx (0xB8)
		//   bits_msb =  8'b1011_1000 (0xB8)
		//   bits_max =  8'b1011_1111 (0xBF)

		//   bits_extra_len = 8-len = 8-5 = 3

		//   bits_extra_val = (1<<bits_extra_len) -1 = 1<<3 -1 = 8'b0000_1000 -1 = 8'b0000_0111
		//   
		//   bits_max = bits_msb + bits_extra_val
		//   bits_max =  8'b1011_1111
		bits_msb = (nBits & nMask) >> (32-DHT_FAST_SIZE);
		bits_extra_len = DHT_FAST_SIZE-nLen;
		bits_extra_val = (1<<bits_extra_len) - 1;
		bits_max = bits_msb + bits_extra_val;

		for (unsigned ind1=bits_msb;ind1<=bits_max;ind1++) {
			// The arrangement of the dht_lookupfast[] values:
			//                         0xFFFF_FFFF = No entry, resort to slow search
			//    dht_lookupfast[set][ind] [15: 8] = len
			//    dht_lookupfast[set][ind] [ 7: 0] = code
			//
			//ASSERT(code <= 0xFF);
			//ASSERT(ind1 <= 0xFF);
			fastval = nCode + (nLen << 8);
			m_anDhtLookupfast[nClass][nDestId][ind1] = fastval;
		}
	}
	return true;
}

bool CimgDecode::SetDhtSize(unsigned nDestId,unsigned nClass,unsigned nSize)
{
	if ( (nDestId >= 4) || (nClass >= 2) || (nSize >= MAX_DHT_CODES) ) {
		AfxMessageBox("ERROR: Attempt to set DHT table size out of range");
		return false;
	} else {
		m_anDhtLookupSize[nClass][nDestId] = nSize;
	}

	return true;
}


// Convert according to Table 5
signed CimgDecode::HuffmanDc2Signed(unsigned nVal,unsigned nBits)
{
	if (nVal >= (unsigned)(1<<(nBits-1))) {
		return (signed)(nVal);
	} else {
		return (signed)( nVal - ((1<<nBits)-1) );
	}
}


void CimgDecode::HuffMaskLookupGen()
{
	unsigned int mask;
	for (unsigned len=0;len<32;len++)
	{
		mask = (1 << (len))-1;
		mask <<= 32-len;
		m_anHuffMaskLookup[len] = mask;
	}
}


inline unsigned CimgDecode::ExtractBits(unsigned nWord,unsigned nBits)
{
	unsigned val;
	val = (nWord & m_anHuffMaskLookup[nBits]) >> (32-nBits);
	return val;
}

// Discard the first "num_bits" of m_nScanBuff
// And then realign file position pointers
inline void CimgDecode::ScanBuffConsume(unsigned nNumBits)
{
	m_nScanBuff <<= nNumBits;
	m_nScanBuff_vacant += nNumBits;

	// Need to latch any errors during consumption of multi-bytes
	// otherwise we'll miss the error notification if we skip over
	// it before we exit this routine! e.g if num_bytes=2 and error
	// appears on first byte, then we want to retain it in pos[0]


	// Now realign the file position pointers if necessary
	unsigned num_bytes = (m_nScanBuffPtr_align+nNumBits) / 8;
	for (unsigned i=0;i<num_bytes;i++) {
		m_nScanBuffPtr_pos[0] = m_nScanBuffPtr_pos[1];
		m_nScanBuffPtr_pos[1] = m_nScanBuffPtr_pos[2];
		m_nScanBuffPtr_pos[2] = m_nScanBuffPtr_pos[3];
		// Don't clear the last ptr position because during an overread
		// this will be the only place that the last position was preserved
		//m_nScanBuffPtr_pos[3] = 0;

		m_nScanBuffPtr_err[0] = m_nScanBuffPtr_err[1];
		m_nScanBuffPtr_err[1] = m_nScanBuffPtr_err[2];
		m_nScanBuffPtr_err[2] = m_nScanBuffPtr_err[3];
		m_nScanBuffPtr_err[3] = SCANBUF_OK;

		if (m_nScanBuffPtr_err[0] != SCANBUF_OK) {
			m_nScanBuffLatchErr = m_nScanBuffPtr_err[0];
		}

		m_nScanBuffPtr_num--;
	}
	m_nScanBuffPtr_align = (m_nScanBuffPtr_align+nNumBits)%8;

}


inline void CimgDecode::ScanBuffAdd(unsigned nNewByte,unsigned nPtr)
{
	// Add the new byte to the buffer
	// Assume that m_nScanBuff has already been shifted to be
	// aligned to bit 31 as first bit.
	m_nScanBuff += (nNewByte << (m_nScanBuff_vacant-8));
	m_nScanBuff_vacant -= 8;

	ASSERT(m_nScanBuffPtr_num<4);
	m_nScanBuffPtr_err[m_nScanBuffPtr_num]   = SCANBUF_OK;
	m_nScanBuffPtr_pos[m_nScanBuffPtr_num++] = nPtr;

	// m_nScanBuffPtr_align stays the same as we add 8 bits
}

// Same as ScanBuffAdd(), but we mark this byte as having an error!
inline void CimgDecode::ScanBuffAddErr(unsigned nNewByte,unsigned nPtr,unsigned nErr)
{
	ScanBuffAdd(nNewByte,nPtr);
	m_nScanBuffPtr_err[m_nScanBuffPtr_num-1]   = nErr;

}

void CimgDecode::ScanErrorsDisable()
{
	m_nWarnBadScanNum = m_nScanErrMax;
	m_bScanErrorsDisable = true;
}

void CimgDecode::ScanErrorsEnable()
{
	m_nWarnBadScanNum = 0;
	m_bScanErrorsDisable = false;
}

// work on m_nScanBuff and automatically perform shift afterwards
// Need to make sure that we abort if there aren't enough bits
// left to work with! (e.g. scan buffer is almost empty as we
// reach end of scan segment)
// PRE: Assume that dht_lookup_size[nTbl] != 0 (already checked)
// POST: m_nScanBitsUsed# is calculated

// PERFORMANCE: Tried to unroll all function calls listed below,
// but performance was same before & after (27sec).
//   Calls unrolled: BuffTopup(), ScanBuffConsume(),
//                   ExtractBits(), HuffmanDc2Signed()

unsigned CimgDecode::ReadScanVal(unsigned nClass,unsigned nTbl,unsigned &rZrl,signed &rVal)
{
	bool done = false;
	unsigned ind = 0;
	unsigned code = 0xFFFFFF; // Not a rValid code
	unsigned val_u;
	m_nScanBitsUsed1 = 0;		// bits consumed for code
	m_nScanBitsUsed2 = 0;

	rZrl = 0;
	rVal = 0;
	
	// First check to see if we've entered here with a completely empty
	// scan buffer with a restart marker already observed. In that case
	// we want to exit with condition 3 (restart terminated)
	if ( (m_nScanBuff_vacant == 32) && (m_bRestartRead) ) {
		return 3;
	}


	// Has the scan buffer been depleted?
	if (m_nScanBuff_vacant >= 32) {
		// Trying to overread end of scan segment

		if (m_nWarnBadScanNum < m_nScanErrMax) {
			CString tmpStr;
			tmpStr.Format(_T("*** ERROR: Overread scan segment (before code)! @ Offset: %s"),GetScanBufPos());
			m_pLog->AddLineErr(tmpStr);

			m_nWarnBadScanNum++;
			if (m_nWarnBadScanNum >= m_nScanErrMax) {
				tmpStr.Format(_T("    Only reported first %u instances of this message..."),m_nScanErrMax);
				m_pLog->AddLineErr(tmpStr);
			}
		}

		m_bScanEnd = true;
		m_bScanBad = true;
		return 2;
	}

	unsigned nDbgScanBufVacant1,nDbgScanBufVacant2,nDbgScanBufVacant3,nDbgScanBufVacant4,nDbgScanBufVacant5;
	nDbgScanBufVacant1 = m_nScanBuff_vacant; //CAL! Debug

	// Top up the buffer just in case
	BuffTopup();
	nDbgScanBufVacant2 = m_nScanBuff_vacant; //CAL! Debug

	done = false;
	bool found = false;

	// Fast search for variable-length huffman code
	// Do direct lookup for any codes DHT_FAST_SIZE bits or shorter

	unsigned code_msb;
	unsigned code_fastsearch;

	// Only enable this fast search if m_nScanBuff_vacant implies
	// that we have at least DHT_FAST_SIZE bits available in the buffer!
	if ((32-m_nScanBuff_vacant) >= DHT_FAST_SIZE) {
		code_msb = m_nScanBuff>>(32-DHT_FAST_SIZE);
		code_fastsearch = m_anDhtLookupfast[nClass][nTbl][code_msb];
		if (code_fastsearch != 0xFFFFFFFF) {
			// We found the code!
			m_nScanBitsUsed1 += (code_fastsearch>>8);
			code = (code_fastsearch & 0xFF);
			done = true;
			found = true;
		}
	} else {
		unsigned tmp = 0;
	}
	

	// Slow search for variable-length huffman code
	while (!done) {
		unsigned nBitLen;
		if ((m_nScanBuff & m_anDhtLookup_mask[nClass][nTbl][ind]) == m_anDhtLookup_bits[nClass][nTbl][ind]) {

			nBitLen = m_anDhtLookup_bitlen[nClass][nTbl][ind];
			// Just in case this VLC bit string is longer than the number of
			// bits we have left in the buffer (due to restart marker or end
			// of scan data), we need to double-check
			if (nBitLen <= 32-m_nScanBuff_vacant) {
				code = m_anDhtLookup_code[nClass][nTbl][ind];
				m_nScanBitsUsed1 += nBitLen;
				done = true;
				found = true;
			}
		}
		ind++;
		if (ind >= m_anDhtLookupSize[nClass][nTbl]) {
			done = true;
		}
	}

	// Could not find huffman code in table!
	if (!found) {

		// If we didn't find the huffman code, it might be due to a
		// restart marker that prematurely stopped the scan buffer filling.
		// If so, return with an indication so that DecodeScanComp() can
		// handle the restart marker, refill the scan buffer and then
		// re-do ReadScanVal()
		if (m_bRestartRead) {
			return 3;
		}

		//CAL! FIXME
		// What should we be consuming here? we need to make forward progress
		// in file. Maybe just move to next byte in file? For now, I'm only
		// moving 1 bit forward, so that we may have slightly more opportunities
		// to re-align earlier.
		m_nScanBitsUsed1 = 1;
		code = 0xFFFFFF;
	}

	// m_pLog an entry into a histogram
	m_anDhtHisto[nClass][nTbl][m_nScanBitsUsed1]++;
	// Alternative code to see if we can reuse indices
	//unsigned nArrInd = nClass*4*17*4 + nTbl*17*4 + m_nScanBitsUsed1;
	//unsigned*	pm_anDhtHisto = (unsigned*)m_anDhtHisto;
	//pm_anDhtHisto[nArrInd]++;


	ScanBuffConsume(m_nScanBitsUsed1);
	nDbgScanBufVacant3 = m_nScanBuff_vacant; //CAL! Debug

	// Did we overread the scan buffer?
	if (m_nScanBuff_vacant > 32) {
		// The code consumed more bits than we had!
		CString tmpStr;
		tmpStr.Format(_T("*** ERROR: Overread scan segment (after code)! @ Offset: %s"),GetScanBufPos());
		m_pLog->AddLineErr(tmpStr);
		m_bScanEnd = true;
		m_bScanBad = true;
		return 2;
	}

	// Replenish buffer after code extraction and before variable extra bits
	// This is required because we may have a 12 bit code followed by a 16 bit var length bitstring
	BuffTopup();

	nDbgScanBufVacant4 = m_nScanBuff_vacant; //CAL! Debug
	// Did we find the code?
	if (code != 0xFFFFFF) {
		rZrl = (code & 0xF0) >> 4;
		m_nScanBitsUsed2 = code & 0x0F;
		if ( (rZrl == 0) && (m_nScanBitsUsed2 == 0) ) {
			// EOB (was bits_extra=0)
			return 1;
		} else if (m_nScanBitsUsed2 == 0) {
			// Zero rValue
			rVal = 0;
			return 0;

		} else {
			// Normal code
			val_u = ExtractBits(m_nScanBuff,m_nScanBitsUsed2);
			rVal = HuffmanDc2Signed(val_u,m_nScanBitsUsed2);

			// Now handle the different precision values
			// Treat 12-bit like 8-bit but scale values first (ie. drop precision down to 8-bit)
			signed nPrecisionDivider = 1;
			if (m_nPrecision >= 8) {
				nPrecisionDivider = 1<<(m_nPrecision-8);
				rVal /= nPrecisionDivider;
			} else {
				// Precision value seems out of range!
			}

			ScanBuffConsume(m_nScanBitsUsed2);
			nDbgScanBufVacant5 = m_nScanBuff_vacant; //CAL! Debug

			// Did we overread the scan buffer?
			if (m_nScanBuff_vacant > 32) {
				// The code consumed more bits than we had!
				CString tmpStr;
				tmpStr.Format(_T("*** ERROR: Overread scan segment (after bitstring)! @ Offset: %s"),GetScanBufPos());
				m_pLog->AddLineErr(tmpStr);
				m_bScanEnd = true;
				m_bScanBad = true;
				return 2;
			}

			return 0;
		}
	} else {
		// ERROR: Invalid huffman code!

		// FIXME: We may enter here if we are about to encounter a
		// restart marker! Need to see if ScanBuf is terminated by
		// restart marker; if so, then we simply flush the ScanBuf,
		// consume the 2-byte RST marker, clear the ScanBuf terminate
		// reason, then indicate to caller that they need to call ReadScanVal
		// again.

		if (m_nWarnBadScanNum < m_nScanErrMax) {
			CString tmpStr;
			tmpStr.Format(_T("*** ERROR: Can't find huffman bitstring @ %s, table %u, value [0x%08x]"),GetScanBufPos(),nTbl,m_nScanBuff);
			m_pLog->AddLineErr(tmpStr);

			m_nWarnBadScanNum++;
			if (m_nWarnBadScanNum >= m_nScanErrMax) {
				tmpStr.Format(_T("    Only reported first %u instances of this message..."),m_nScanErrMax);
				m_pLog->AddLineErr(tmpStr);
			}
		}

		// What rValue and ZRL should we return?
		//CAL! For now, return code x00
		m_bScanBad = true;
		return 2;
	}

	return 2;
}


// Do a quick decode on the current dword (with alignment),
// determining the associated code value. This can be used
// to get the current VLC total bit length.
unsigned CimgDecode::DecodeScanDword(unsigned nFilePos,unsigned nWordAlign,unsigned nClass,unsigned nTbl,unsigned &rZrl,signed &rVal)
{
	unsigned nRet;

	//CAL! In the following call, we want to remove align param, as it doesn't work
	DecodeRestartScanBuf(nFilePos);
	BuffTopup();
	ScanBuffConsume(nWordAlign);
	//CAL! In the following call, we want to get the actual total len, not just ZRL, Val
	nRet = ReadScanVal(nClass,nTbl,rZrl,rVal);
	return 0;
}



void CimgDecode::BuffTopup()
{
	unsigned retval;

	// Do we have space to load another byte?
	bool done = (m_nScanBuff_vacant < 8);

	// Have we already reached the end of the scan segment?
	if (m_bScanEnd) {
		done = true;
	}

	while (!done) {
		retval = BuffAddByte();

		// NOTE: If we have read in a restart marker, the above call will not
		// read in any more bits into the scan buffer, so we should just simply
		// say that we've done the best we can for the top up.
		if (m_bRestartRead) {
			done = true;
		}

		if (m_nScanBuff_vacant < 8) {
			done = true;
		}
		// If the buffer read returned an error or end of scan segment
		// then stop filling buffer
		if (retval != 0) {
			done = true;
		}
	}
}

// NOTE:
// This routine expects that we did not remove restart markers
// from the bytestream (in BuffAddByte).
bool CimgDecode::ExpectRestart()
{
	unsigned marker;

	unsigned nBuf0 = m_pWBuf->Buf(m_nScanBuffPtr);
	unsigned nBuf1 = m_pWBuf->Buf(m_nScanBuffPtr+1);

	// Check for restart marker first. Assume none back-to-back.
	if (nBuf0 == 0xFF) {
		marker = nBuf1;

		// FIXME: Later, check that we are on the right marker!
		if ((marker >= JFIF_RST0) && (marker <= JFIF_RST7)) {

			if (m_bVerbose) {
	  		  CString tmpStr;
			  tmpStr.Format(_T("  RESTART marker: @ 0x%08X.0 : RST%02u"),
				m_nScanBuffPtr,marker-JFIF_RST0);
			  m_pLog->AddLine(tmpStr);
			}

			m_nRestartRead++;

			// FIXME:
			//     Later on, we should be checking for RST out of
			//     sequence!

			// Now we need to skip to the next bytes
			m_nScanBuffPtr+=2;

			// Now refill in the buffer if we need to
			BuffAddByte();

			return true;
		}
	}

	return false;

}

unsigned CimgDecode::BuffAddByte()
{
	unsigned marker;
	unsigned nBuf0,nBuf1;

	// If we have already read in a restart marker but not
	// handled it yet, then simply return without reading any
	// more bytes
	if (m_bRestartRead) {
		return 0;
	}

	nBuf0 = m_pWBuf->Buf(m_nScanBuffPtr);
	nBuf1 = m_pWBuf->Buf(m_nScanBuffPtr+1);

	// Check for restart marker first. Assume none back-to-back.
	if (nBuf0 == 0xFF) {
		marker = nBuf1;

		if ((marker >= JFIF_RST0) && (marker <= JFIF_RST7)) {

			if (m_bVerbose) {
	  		  CString tmpStr;
			  tmpStr.Format(_T("  RESTART marker: @ 0x%08X.0 : RST%02u"),
				m_nScanBuffPtr,marker-JFIF_RST0);
			  m_pLog->AddLine(tmpStr);
			}

			m_nRestartRead++;
			m_nRestartLastInd = marker - JFIF_RST0;
			if (m_nRestartLastInd != m_nRestartExpectInd) {
				if (!m_bScanErrorsDisable) {
	  				CString tmpStr;
					tmpStr.Format(_T("  ERROR: Expected RST marker index RST%u got RST%u @ 0x%08X.0"),
						m_nRestartExpectInd,m_nRestartLastInd,m_nScanBuffPtr);
					m_pLog->AddLineErr(tmpStr);
				}
			}
			m_nRestartExpectInd = (m_nRestartLastInd+1)%8;

			// FIXME:
			//     Later on, we should be checking for RST out of
			//     sequence!

			// END BUFFER READ HERE!
			// Indicate that a Restart marker has been seen, which
			// will prevent further reading of scan buffer until it
			// has been handled.
			m_bRestartRead = true;

			return 0;

/*
			// Now we need to skip to the next bytes
			m_nScanBuffPtr+=2;

			// Update local saved buffer vars
			nBuf0 = m_pWBuf->Buf(m_nScanBuffPtr);
			nBuf1 = m_pWBuf->Buf(m_nScanBuffPtr+1);


			// Use ScanBuffAddErr() to mark this byte as being after Restart marker!
			m_nScanBuffPtr_err[m_nScanBuffPtr_num] = SCANBUF_RST;

			// FIXME: We should probably discontinue filling the scan
			// buffer if we encounter a restart marker. This will stop us
			// from reading past the restart marker and missing the necessary
			// step in resetting the decoder accumulation state.

			// When we stop adding bytes to the buffer, we should also
			// mark this scan buffer with a flag to indicate that it was
			// ended by RST not by EOI or an Invalid Marker.

			// If the main decoder (in ReadScanVal) cannot find a VLC code 
			// match with the few bits left in the scan buffer 
			// (presumably padded with 1's), then it should check to see
			// if the buffer is terminated by RST. If so, then it 
			// purges the scan buffer, advances to the next byte (after the
			// RST marker) and does a fill, then re-invokes the ReadScanVal
			// routine. At the level above, the Decoder that is calling
			// ReadScanVal should be counting MCU rows and expect this error
			// from ReadScanVal (no code match and buf terminated by RST).
			// If it happened out of place, we have corruption somehow!

			// See IJG Code:
			//   jdmarker.c:
			//     read_restart_marker()
			//     jpeg_resync_to_restart()
*/
		}

	}


	if ((nBuf0 == 0xFF) && (nBuf1 == 0x00)) {
		// Byte stuff!

		// Add byte to m_nScanBuff & record file position
		//CAL! BUG FIX #1002
//		ScanBuffAdd(nBuf0,m_nScanBuffPtr+1);
		ScanBuffAdd(nBuf0,m_nScanBuffPtr);
		m_nScanBuffPtr+=2;


	} else if ((nBuf0 == 0xFF) && (nBuf1 == 0xFF)) {
		// CAL! *** NOTE:
		// We should be checking for a run of 0xFF before EOI marker.
		// It is possible that we could get marker padding on the end
		// of the scan segment, so we'd want to handle it here, otherwise
		// we'll report an error that we got a non-EOI Marker in scan
		// segment.

		// The downside of this is that we don't detect errors if we have
		// a run of 0xFF in the stream, until we leave the string of FF's.
		// If it were followed by an 0x00, then we may not notice it at all.
		// Probably OK.

		/*
		if (m_nWarnBadScanNum < m_nScanErrMax) {
			CString tmpStr;
			tmpStr.Format(_T("  Scan Data encountered sequence 0xFFFF @ 0x%08X.0 - Assume start of marker pad at end of scan segment"),
				m_nScanBuffPtr);
			m_pLog->AddLineWarn(tmpStr);

			m_nWarnBadScanNum++;
			if (m_nWarnBadScanNum >= m_nScanErrMax) {
				tmpStr.Format(_T("    Only reported first %u instances of this message..."),m_nScanErrMax);
				m_pLog->AddLineErr(tmpStr);
			}
		}

		// Treat as single byte of byte stuff for now, since we don't
		// know if FF bytes will arrive in pairs or not.
		m_nScanBuffPtr+=1;
		*/

		// CAL! ***
		// If I treat the 0xFFFF as a potential marker pad, we may not stop correctly
		// upon error if we see this inside the image somewhere (not at end).
		// Therefore, let's simply add these bytes to the buffer and let the DecodeScanImg()
		// routine figure out when we're at the end, etc.

		ScanBuffAdd(nBuf0,m_nScanBuffPtr);
		m_nScanBuffPtr+=1;

	} else if ((nBuf0 == 0xFF) && (marker != 0x00)) {



		// We have read a marker... don't assume that this is bad as it will
		// always happen at the end of the scan segment. Therefore, we will
		// assume this marker is valid (ie. not bit error in scan stream)
		// and mark the end of the scan segment.

		if (m_nWarnBadScanNum < m_nScanErrMax) {
			CString tmpStr;
			tmpStr.Format(_T("  Scan Data encountered marker   0xFF%02X @ 0x%08X.0"),
				marker,m_nScanBuffPtr);
			m_pLog->AddLine(tmpStr);

			if (marker != JFIF_EOI) {
				m_pLog->AddLineErr(_T("  NOTE: Marker wasn't EOI (0xFFD9)"));
			}

			m_nWarnBadScanNum++;
			if (m_nWarnBadScanNum >= m_nScanErrMax) {
				tmpStr.Format(_T("    Only reported first %u instances of this message..."),m_nScanErrMax);
				m_pLog->AddLineErr(tmpStr);
			}
		}



#ifdef SCAN_BAD_MARKER_STOP
		m_bScanEnd = true;
		return 1;
#else
		// Add byte to m_nScanBuff & record file position
		//ScanBuffAdd(nBuf0,m_nScanBuffPtr);
		ScanBuffAddErr(nBuf0,m_nScanBuffPtr,SCANBUF_BADMARK);

		m_nScanBuffPtr+=1;
#endif


	} else {
		// Normal byte
		// Add byte to m_nScanBuff & record file position
		ScanBuffAdd(nBuf0,m_nScanBuffPtr);

		m_nScanBuffPtr+=1;
	}

	return 0;
}


// As this routine is called for every MCU, we want it to be
// reasonably efficient. Only call string routines when we are
// sure that we have an error.
// RETURN: 0 - Error Fail
//         1 - OK
//         2 - Error, but continue
// POST:   m_nScanCurDc = DC component value

// Define minimum value before we include DCT entry in
// the IDCT calcs. *** Currently disabled
#define IDCT_COEF_THRESH 4

// FIXME Consider adding checks for DHT table like in ReadScanVal()
unsigned CimgDecode::DecodeScanComp(unsigned nTbl,unsigned nMcuX,unsigned nMcuY)
{
	unsigned	ok;
	unsigned	zrl;
	signed		val;
	bool		done = false;
	bool		bDC = true;	// Start with DC coeff

	unsigned num_coeffs = 0;
	unsigned nDctMax = 0;			// Maximum DCT coefficient to use for IDCT
	unsigned saved_buf_pos = 0;
	unsigned saved_buf_err = SCANBUF_OK;
	unsigned saved_buf_align = 0;

	// Profiling: No difference noted
	DecodeIdctClear();

	while (!done) {
		BuffTopup();

		// Note that once we perform ReadScanVal(), then GetScanBufPos() will be
		// after the decoded VLC
		// Save old file position info in case we want accurate error positioning
		saved_buf_pos   = m_nScanBuffPtr_pos[0];
		saved_buf_err   = m_nScanBuffLatchErr;
		saved_buf_align = m_nScanBuffPtr_align;
		//strPos = GetScanBufPos();

		// Return values:
		//	0 - OK
		//  1 - EOB
		//  2 - Overread error
		//  3 - No huffman code found, but restart marker seen
		// Assume nTbl just points to DC tables, adjust for AC
		// e.g. nTbl = 0,2,4
		ok = ReadScanVal(bDC?0:1,nTbl,zrl,val);

		// Handle Restart marker first.
		if (ok == 3) {
			// Assume that m_bRestartRead is TRUE
			// No huffman code found because either we ran out of bits
			// in the scan buffer or the bits padded with 1's didn't result
			// in a valid VLC code.

			// Steps:
			//   1) Reset the decoder state (DC values)
			//   2) Advance the buffer pointer (might need to handle the
			//      case of perfect alignment to byte boundary separately)
			//   3) Flush the Scan Buffer
			//   4) Clear m_bRestartRead
			//   5) Refill Scan Buffer with BuffTopUp()
			//   6) Re-invoke ReadScanVal()

			// Step 1:
			DecodeRestartDcState();

			// Step 2
			m_nScanBuffPtr += 2;

			// Step 3
			DecodeRestartScanBuf(m_nScanBuffPtr);

			// Step 4
			m_bRestartRead = false;

			// Step 5
			BuffTopup();

			// Step 6
			// ASSERT is because we assume that we don't get 2 restart
			// markers in a row!
			ok = ReadScanVal(bDC?0:1,nTbl,zrl,val);
			ASSERT(ok != 3);

		}

		// In case we encountered a restart marker or bad scan marker
//		if (saved_buf_err != SCANBUF_OK) {
		if (saved_buf_err == SCANBUF_BADMARK) {

			// Mark as scan error
			m_nScanCurErr = true;

			m_bScanBad = true;

			if (m_nWarnBadScanNum < m_nScanErrMax) {
				CString strPos = GetScanBufPos(saved_buf_pos,saved_buf_align);
				CString tmpStr;
				tmpStr.Format(_T("*** ERROR: Bad marker @ %s"),strPos);
				m_pLog->AddLineErr(tmpStr);

				m_nWarnBadScanNum++;
				if (m_nWarnBadScanNum >= m_nScanErrMax) {
					tmpStr.Format(_T("    Only reported first %u instances of this message..."),m_nScanErrMax);
					m_pLog->AddLineErr(tmpStr);
				}
			}

			// Reset the latched error now that we've dealt with it
			m_nScanBuffLatchErr = SCANBUF_OK;

		}

		if (ok == 0) {
			// DC entry is always one value only
			if (bDC) {
				DecodeIdctSet(nTbl,num_coeffs,zrl,val); //CALZ
				bDC = false;			// Now we will be on AC comps
			} else {
				// We're on AC entry, so keep looping until
				// we have finished up to 63 entries
				// Set entry in table
				// PERFORMANCE:
				//   No noticeable difference if following is skipped
				if (m_bDecodeScanAc) {
					DecodeIdctSet(nTbl,num_coeffs,zrl,val);
				}
			}
		} else if (ok == 1) {
			if (bDC) {
				DecodeIdctSet(nTbl,num_coeffs,zrl,val); //CALZ
				bDC = false;			// Now we will be on AC comps
			} else {
				done = true;
			}
			//
		} else if (ok == 2) {
			// ERROR

			if (m_nWarnBadScanNum < m_nScanErrMax) {
				CString strPos = GetScanBufPos(saved_buf_pos,saved_buf_align);
				CString tmpStr;
				tmpStr.Format(_T("*** ERROR: Bad huffman code @ %s"),strPos);
				m_pLog->AddLineErr(tmpStr);

				m_nWarnBadScanNum++;
				if (m_nWarnBadScanNum >= m_nScanErrMax) {
					tmpStr.Format(_T("    Only reported first %u instances of this message..."),m_nScanErrMax);
					m_pLog->AddLineErr(tmpStr);
				}
			}

			m_nScanCurErr = true;
			done = true;

			return 0;
		}

		// Increment the number of coefficients
		num_coeffs += 1+zrl;
		// If we have filled out an entire 64 entries, then we move to
		// the next block without an EOB
		// NOTE: This is only 63 entries because we assume that we
		//       are doing the AC (DC was already done in a different pass)

		// FIXME - Would like to combine DC & AC in one pass so that
		// we don't end up having to use 2 tables. The check below will
		// also need to be changed to == 64.
		//
		// Currently, we will have to correct AC num_coeffs entries (in IDCT) to
		// be +1 to get real index, as we are ignoring DC position 0.

		if (num_coeffs == 64) {
			done = true;
		} else if (num_coeffs > 64) {
			// ERROR

			if (m_nWarnBadScanNum < m_nScanErrMax) {
				CString tmpStr;
				CString strPos = GetScanBufPos(saved_buf_pos,saved_buf_align);
				tmpStr.Format(_T("*** ERROR: @ %s, num_coeffs>64 [%u]"),strPos,num_coeffs);
				m_pLog->AddLineErr(tmpStr);

				m_nWarnBadScanNum++;
				if (m_nWarnBadScanNum >= m_nScanErrMax) {
					tmpStr.Format(_T("    Only reported first %u instances of this message..."),m_nScanErrMax);
					m_pLog->AddLineErr(tmpStr);
				}
			}

			m_nScanCurErr = true;
			m_bScanBad = true;
			done = true;

			num_coeffs = 64;	// Just to ensure we don't use an overrun value anywhere
		}


	}

	// We finished the MCU
	// Now calc the IDCT matrix

	// PERFORMANCE:
	//   On file: canon_1dsmk2_
	//
	//   0:06	Turn off m_bDecodeScanAc (so no array memset, etc.)
	//   0:10   m_bDecodeScanAc=true, but DecodeIdctCalc() skipped
	//   0:26	m_bDecodeScanAc=true and DecodeIdctCalcFixedpt()
	//   0:27	m_bDecodeScanAc=true and DecodeIdctCalcFloat()

	if (m_bDecodeScanAc) {
#ifdef IDCT_FIXEDPT
		DecodeIdctCalcFixedpt();
#else
// FIXME!!!
//		DecodeIdctCalcFloat(nDctMax);
//		DecodeIdctCalcFloat(num_coeffs);
//		DecodeIdctCalcFloat(m_nDctCoefMax);
		DecodeIdctCalcFloat(64);
//		DecodeIdctCalcFloat(32);
#endif
	}


	return 1;
}


// As this routine is called for every MCU, it is important
// for it to be efficient. However, we are in print mode, so
// we can afford to be slower.
// FIXME need to fix like DecodeScanComp() (ordering of exit conditions, etc.)
// FIXME Consider adding checks for DHT table like in ReadScanVal()
unsigned CimgDecode::DecodeScanCompPrint(unsigned nTbl,unsigned nMcuX,unsigned nMcuY)
{
	bool bPrint = true;
	unsigned ok;
	CString tmpStr;
	CString tblStr;
	CString specialStr;
	CString strPos;
	unsigned zrl;
	signed val;
	bool done = false;

	bool bDC = true;	// Start with DC component

	if (bPrint) {
		switch(nTbl) {
			case 0:
				tblStr = "Lum";
				break;
			case 1:
				tblStr = "Chr(0)"; // Usually Cb
				break;
			case 2:
				tblStr = "Chr(1)"; // Usually Cr
				break;
			default:
				tblStr = "???";
				break;
		}
		tmpStr.Format(_T("    %s (Tbl #%u), MCU=[%u,%u]"),tblStr,nTbl,nMcuX,nMcuY);
		m_pLog->AddLine(tmpStr);
	}

	unsigned num_coeffs = 0;
	unsigned saved_buf_pos = 0;
	unsigned saved_buf_err = SCANBUF_OK;
	unsigned saved_buf_align = 0;

	DecodeIdctClear();

	while (!done) {
		BuffTopup();

		// Note that once we perform ReadScanVal(), then GetScanBufPos() will be
		// after the decoded VLC

		// Save old file position info in case we want accurate error positioning
		saved_buf_pos   = m_nScanBuffPtr_pos[0];
		saved_buf_err   = m_nScanBuffLatchErr;
		saved_buf_align = m_nScanBuffPtr_align;
		//strPos = GetScanBufPos();

		// Return values:
		//	0 - OK
		//  1 - EOB
		//  2 - Overread error
		//  3 - No huffman code found, but restart marker seen
		// Assume nTbl just points to DC tables, adjust for AC
		// e.g. nTbl = 0,2,4
		ok = ReadScanVal(bDC?0:1,nTbl,zrl,val);

		// Handle Restart marker first.
		if (ok == 3) {
			// Assume that m_bRestartRead is TRUE
			// No huffman code found because either we ran out of bits
			// in the scan buffer or the bits padded with 1's didn't result
			// in a valid VLC code.

			// Steps:
			//   1) Reset the decoder state (DC values)
			//   2) Advance the buffer pointer (might need to handle the
			//      case of perfect alignment to byte boundary separately)
			//   3) Flush the Scan Buffer
			//   4) Clear m_bRestartRead
			//   5) Refill Scan Buffer with BuffTopUp()
			//   6) Re-invoke ReadScanVal()

			// Step 1:
			DecodeRestartDcState();

			// Step 2
			m_nScanBuffPtr += 2;

			// Step 3
			DecodeRestartScanBuf(m_nScanBuffPtr);

			// Step 4
			m_bRestartRead = false;

			// Step 5
			BuffTopup();

			// Step 6
			// ASSERT is because we assume that we don't get 2 restart
			// markers in a row!
			ok = ReadScanVal(bDC?0:1,nTbl,zrl,val);
			ASSERT(ok != 3);

		}

		// In case we encountered a restart marker or bad scan marker
//		if (saved_buf_err != SCANBUF_OK) {
		if (saved_buf_err == SCANBUF_BADMARK) {

			// Mark as scan error
			m_nScanCurErr = true;

			m_bScanBad = true;

			if (m_nWarnBadScanNum < m_nScanErrMax) {
				CString strPos = GetScanBufPos(saved_buf_pos,saved_buf_align);
				CString tmpStr;
				tmpStr.Format(_T("*** ERROR: Bad marker @ %s"),strPos);
				m_pLog->AddLineErr(tmpStr);

				m_nWarnBadScanNum++;
				if (m_nWarnBadScanNum >= m_nScanErrMax) {
					tmpStr.Format(_T("    Only reported first %u instances of this message..."),m_nScanErrMax);
					m_pLog->AddLineErr(tmpStr);
				}
			}

			// Reset the latched error now that we've dealt with it
			m_nScanBuffLatchErr = SCANBUF_OK;

		}


		// Should this be before or after restart checks?
		unsigned nCoeffStart = num_coeffs;
		unsigned nCoeffEnd   = num_coeffs+zrl;

		if (ok == 0) {
			specialStr = "";
			// DC entry is always one value only
			// FIXME: Do I need nTbl == 4 as well?
			if (bDC) {
				DecodeIdctSet(nTbl,num_coeffs,zrl,val);
				bDC = false;			// Now we will be on AC comps
			} else {
				// We're on AC entry, so keep looping until
				// we have finished up to 63 entries
				// Set entry in table
				DecodeIdctSet(nTbl,num_coeffs,zrl,val);
			}
		} else if (ok == 1) {
			if (bDC) {
				DecodeIdctSet(nTbl,num_coeffs,zrl,val);
				bDC = false;			// Now we will be on AC comps
			} else {
				done = true;
			}
			specialStr = "EOB";
		} else if (ok == 2) {

			if (m_nWarnBadScanNum < m_nScanErrMax) {
				specialStr = "ERROR";
				strPos = GetScanBufPos(saved_buf_pos,saved_buf_align);

				tmpStr.Format(_T("*** ERROR: Bad huffman code @ %s"),strPos);
				m_pLog->AddLineErr(tmpStr);

				m_nWarnBadScanNum++;
				if (m_nWarnBadScanNum >= m_nScanErrMax) {
					tmpStr.Format(_T("    Only reported first %u instances of this message..."),m_nScanErrMax);
					m_pLog->AddLineErr(tmpStr);
				}
			}

			m_nScanCurErr = true;
			done = true;

			// Print out before we leave
			if (bPrint) {
				ReportVlc(saved_buf_pos,saved_buf_align,zrl,val,
					nCoeffStart,nCoeffEnd,specialStr);
			}


			return 0;
		}

		// Increment the number of coefficients
		num_coeffs += 1+zrl;
		// If we have filled out an entire 64 entries, then we move to
		// the next block without an EOB
		// NOTE: This is only 63 entries because we assume that we
		//       are doing the AC (DC was already done in a different pass)
		if (num_coeffs == 64) {
			specialStr = "EOB64";
			done = true;
		} else if (num_coeffs > 64) {
			// ERROR

			if (m_nWarnBadScanNum < m_nScanErrMax) {
				CString tmpStr;
				CString strPos = GetScanBufPos(saved_buf_pos,saved_buf_align);
				tmpStr.Format(_T("*** ERROR: @ %s, num_coeffs>64 [%u]"),strPos,num_coeffs);
				m_pLog->AddLineErr(tmpStr);

				m_nWarnBadScanNum++;
				if (m_nWarnBadScanNum >= m_nScanErrMax) {
					tmpStr.Format(_T("    Only reported first %u instances of this message..."),m_nScanErrMax);
					m_pLog->AddLineErr(tmpStr);
				}
			}

			m_nScanCurErr = true;
			m_bScanBad = true;
			done = true;
		}

		if (bPrint) {
			ReportVlc(saved_buf_pos,saved_buf_align,zrl,val,
				nCoeffStart,nCoeffEnd,specialStr);
		}

	}

	// We finished the MCU component


	// Now calc the IDCT matrix
#ifdef IDCT_FIXEDPT
	DecodeIdctCalcFixedpt();
#else
	DecodeIdctCalcFloat(num_coeffs);
#endif

	// Now report the coefficient matrix (after zigzag reordering)
	if (bPrint) {
		ReportDctMatrix();
	}

	return 1;
}


#if 0

// **** The following is the original DecodeScanCompPrint() code
//      It has a bug in that it doesn't handle restart markers properly
//
// As this routine is called for every MCU, it is important
// for it to be efficient. However, we are in print mode, so
// we can afford to be slower.
// FIXME need to fix like DecodeScanComp() (ordering of exit conditions, etc.)
// FIXME Consider adding checks for DHT table like in ReadScanVal()
unsigned CimgDecode::DecodeScanCompPrint(unsigned nTbl,unsigned nMcuX,unsigned nMcuY)
{
	bool bPrint = true;
	unsigned ok;
	CString tmpStr;
	CString tblStr;
	CString specialStr;
	CString strPos;
	unsigned zrl;
	signed val;
	bool done = false;

	bool bDC = true;	// Start with DC component

	if (bPrint) {
		switch(nTbl) {
			case 0:
				tblStr = "Lum";
				break;
			case 1:
				tblStr = "Chr(0)"; // Usually Cb
				break;
			case 2:
				tblStr = "Chr(1)"; // Usually Cr
				break;
			default:
				tblStr = "???";
				break;
		}
		tmpStr.Format(_T("    %s (Tbl #%u), MCU=[%u,%u]"),tblStr,nTbl,nMcuX,nMcuY);
		m_pLog->AddLine(tmpStr);
	}

	unsigned num_coeffs = 0;
	unsigned saved_buf_pos = 0;
	unsigned saved_buf_align = 0;

	DecodeIdctClear();

	while (!done) {
		BuffTopup();

		// Note that once we perform ReadScanVal(), then GetScanBufPos() will be
		// after the decoded VLC

		// Save old file position info in case we want accurate error positioning
		saved_buf_pos   = m_nScanBuffPtr_pos[0];
		saved_buf_align = m_nScanBuffPtr_align;
		//strPos = GetScanBufPos();

		// Assume nTbl just points to DC tables, adjust for AC
		// e.g. nTbl = 0,2,4
		ok = ReadScanVal(bDC?0:1,nTbl,zrl,val);

		unsigned nCoeffStart = num_coeffs;
		unsigned nCoeffEnd   = num_coeffs+zrl;

		if (ok == 0) {
			specialStr = "";
			// DC entry is always one value only
			// FIXME: Do I need nTbl == 4 as well?
			if (bDC) {
				DecodeIdctSet(nTbl,num_coeffs,zrl,val);
				bDC = false;			// Now we will be on AC comps
			} else {
				// We're on AC entry, so keep looping until
				// we have finished up to 63 entries
				// Set entry in table
				DecodeIdctSet(nTbl,num_coeffs,zrl,val);
			}
		} else if (ok == 1) {
			if (bDC) {
				DecodeIdctSet(nTbl,num_coeffs,zrl,val);
				bDC = false;			// Now we will be on AC comps
			} else {
				done = true;
			}
			specialStr = "EOB";
		} else if (ok == 2) {

			if (m_nWarnBadScanNum < m_nScanErrMax) {
				specialStr = "ERROR";
				strPos = GetScanBufPos(saved_buf_pos,saved_buf_align);

				tmpStr.Format(_T("*** ERROR: Bad huffman code @ %s"),strPos);
				m_pLog->AddLineErr(tmpStr);

				m_nWarnBadScanNum++;
				if (m_nWarnBadScanNum >= m_nScanErrMax) {
					tmpStr.Format(_T("    Only reported first %u instances of this message..."),m_nScanErrMax);
					m_pLog->AddLineErr(tmpStr);
				}
			}

			m_nScanCurErr = true;
			done = true;

			// Print out before we leave
			if (bPrint) {
				ReportVlc(saved_buf_pos,saved_buf_align,zrl,val,
					nCoeffStart,nCoeffEnd,specialStr);
			}


			return 0;
		}

		// Increment the number of coefficients
		num_coeffs += 1+zrl;
		// If we have filled out an entire 64 entries, then we move to
		// the next block without an EOB
		// NOTE: This is only 63 entries because we assume that we
		//       are doing the AC (DC was already done in a different pass)
		if (num_coeffs == 64) {
			specialStr = "EOB64";
			done = true;
		} else if (num_coeffs > 64) {
			// ERROR

			if (m_nWarnBadScanNum < m_nScanErrMax) {
				CString tmpStr;
				CString strPos = GetScanBufPos(saved_buf_pos,saved_buf_align);
				tmpStr.Format(_T("*** ERROR: @ %s, num_coeffs>64 [%u]"),strPos,num_coeffs);
				m_pLog->AddLineErr(tmpStr);

				m_nWarnBadScanNum++;
				if (m_nWarnBadScanNum >= m_nScanErrMax) {
					tmpStr.Format(_T("    Only reported first %u instances of this message..."),m_nScanErrMax);
					m_pLog->AddLineErr(tmpStr);
				}
			}

			m_nScanCurErr = true;
			m_bScanBad = true;
			done = true;
		}

		if (bPrint) {
			ReportVlc(saved_buf_pos,saved_buf_align,zrl,val,
				nCoeffStart,nCoeffEnd,specialStr);
		}

	}

	// We finished the MCU component


	// Now calc the IDCT matrix
#ifdef IDCT_FIXEDPT
	DecodeIdctCalcFixedpt();
#else
	DecodeIdctCalcFloat(num_coeffs);
#endif

	// Now report the coefficient matrix (after zigzag reordering)
	if (bPrint) {
		ReportDctMatrix();
	}

	return 1;
}

#endif



void CimgDecode::ReportDctMatrix()
{
	CString	tmpStr;
	CString	lineStr;
	int		nCoefVal;

	for (unsigned nY=0;nY<8;nY++) {
		if (nY == 0) {
			lineStr = "                      DCT Matrix=[";
		} else {
			lineStr = "                                 [";
		}
		for (unsigned nX=0;nX<8;nX++) {
			tmpStr = "";
			nCoefVal = m_anDctBlock[nY*8+nX];
			tmpStr.Format("%5d",nCoefVal);
			lineStr.Append(tmpStr);

			if (nX != 7) {
				lineStr.Append(" ");
			}
		}
		lineStr.Append("]");
		m_pLog->AddLine(lineStr);
	}
	m_pLog->AddLine("");

}

void CimgDecode::ReportDctRGBMatrix()
{
	CString	tmpStr;
	CString	lineStr;
	unsigned	nR,nG,nB;

	for (unsigned nY=0;nY<8;nY++) {
		if (nY == 0) {
			lineStr = "                      RGB Matrix=[";
		} else {
			lineStr = "                                 [";
		}
		for (unsigned nX=0;nX<8;nX++) {
			tmpStr = "";
			nR = m_anImgFullres[0][nY][nX];
			nG = m_anImgFullres[1][nY][nX];
			nB = m_anImgFullres[2][nY][nX];
			//nCoefVal = m_anDctBlock[nY*8+nX];
			//tmpStr.Format("%5d",nCoefVal);
			tmpStr.Format("[%3u,%3u,%3u]",nR,nG,nB);
			lineStr.Append(tmpStr);

			if (nX != 7) {
				lineStr.Append(" ");
			}
		}
		lineStr.Append("]");
		m_pLog->AddLine(lineStr);
	}
	m_pLog->AddLine("");

}


/*
Need to consider impact of padding bytes... When pads get extracted, they 
will not appear in the binary display shown below. Might want the get buffer
to do post-pad removal first.

eg. file Dsc0019.jpg
Overlay 0x4215 = 7FFF0000 len=4
*/

void CimgDecode::ReportVlc(unsigned nVlcPos, unsigned nVlcAlign,
						   unsigned nZrl, int nVal,
						   unsigned nCoeffStart,unsigned nCoeffEnd,
						   CString specialStr)
{
	CString strPos;
	CString	tmpStr;
	strPos = GetScanBufPos(nVlcPos,nVlcAlign);

	unsigned	nBufByte[4];
	unsigned	nBufPosInd = nVlcPos;
	CString strData = "";
	CString strByte1 = "";
	CString strByte2 = "";
	CString strByte3 = "";
	CString strByte4 = "";
	CString strBytes = "";
	CString strBytesOrig = "";
	CString strBinMarked = "";

	// Read in the buffer bytes, but skip pad bytes (0xFF00 -> 0xFF)

//	nBufByte[0] = m_pWBuf->Buf(nBufPosInd++);

	//CAL! BUG Fix #1002
	// We need to look at previous byte as it might have been
	// start of stuff byte! If so, we need to ignore the byte
	// and advance the pointers.
	if (nBufPosInd == 0x36F) {
		int dummy=1;
	}
	BYTE nBufBytePre = m_pWBuf->Buf(nBufPosInd-1);
	nBufByte[0] = m_pWBuf->Buf(nBufPosInd++);
	if ( (nBufBytePre == 0xFF) && (nBufByte[0] == 0x00) ) {
		nBufByte[0] = m_pWBuf->Buf(nBufPosInd++);
	}
	//CAL! BUF Fix End

	nBufByte[1] = m_pWBuf->Buf(nBufPosInd++);
	if ( (nBufByte[0] == 0xFF) && (nBufByte[1] == 0x00) ) {
		nBufByte[1] = m_pWBuf->Buf(nBufPosInd++);
	}
	nBufByte[2] = m_pWBuf->Buf(nBufPosInd++);
	if ( (nBufByte[1] == 0xFF) && (nBufByte[2] == 0x00) ) {
		nBufByte[2] = m_pWBuf->Buf(nBufPosInd++);
	}
	nBufByte[3] = m_pWBuf->Buf(nBufPosInd++);
	if ( (nBufByte[2] == 0xFF) && (nBufByte[3] == 0x00) ) {
		nBufByte[3] = m_pWBuf->Buf(nBufPosInd++);
	}

	strByte1 = Dec2Bin(nBufByte[0],8);
	strByte2 = Dec2Bin(nBufByte[1],8);
	strByte3 = Dec2Bin(nBufByte[2],8);
	strByte4 = Dec2Bin(nBufByte[3],8);
	strBytesOrig = strByte1 + " " + strByte2 + " " + strByte3 + " " + strByte4;
	strBytes = strByte1 + strByte2 + strByte3 + strByte4;

	//strBinMarked += strBytes.Mid(0,nVlcAlign);
	//strBinMarked += "<";
	for (unsigned ind=0;ind<nVlcAlign;ind++) {
		strBinMarked += "-";
	}

	strBinMarked += strBytes.Mid(nVlcAlign,m_nScanBitsUsed1+m_nScanBitsUsed2);

	//strBinMarked += ">";
	//strBinMarked += strBytes.Mid(nVlcAlign+m_nScanBitsUsed1+m_nScanBitsUsed2);
	for (unsigned ind=nVlcAlign+m_nScanBitsUsed1+m_nScanBitsUsed2;ind<32;ind++) {
		strBinMarked += "-";
	}
	
	strBinMarked.Insert(24," ");
	strBinMarked.Insert(16," ");
	strBinMarked.Insert(8," ");


	strData.Format("0x %02X %02X %02X %02X = 0b (%s)",
//CAL! BUG Fix #1002
//		m_pWBuf->Buf(nVlcPos),m_pWBuf->Buf(nVlcPos+1),
//		m_pWBuf->Buf(nVlcPos+2),m_pWBuf->Buf(nVlcPos+3),
		nBufByte[0],nBufByte[1],nBufByte[2],nBufByte[3],
		strBinMarked);

	if ((nCoeffStart == 0) && (nCoeffEnd == 0)) {
		tmpStr.Format(_T("      [%s]: ZRL=[%2u] Val=[%5d] Coef=[%02u= DC] Data=[%s] %s"),
			strPos,nZrl,nVal,nCoeffStart,strData,specialStr);
	} else {
		tmpStr.Format(_T("      [%s]: ZRL=[%2u] Val=[%5d] Coef=[%02u..%02u] Data=[%s] %s"),
			strPos,nZrl,nVal,nCoeffStart,nCoeffEnd,strData,specialStr);
	}
	m_pLog->AddLine(tmpStr);

}

CString CimgDecode::Dec2Bin(unsigned nVal,unsigned nLen)
{
	unsigned	nBit;
	CString		strBin = "";
	for (int nInd=nLen-1;nInd>=0;nInd--)
	{
		nBit = ( nVal & (1 << nInd) ) >> nInd;
		strBin += (nBit==1)?"1":"0";
		if ( ((nInd % 8) == 0) && (nInd != 0) ) {
			strBin += " ";
		}
	}
	return strBin;
}


// Clear input and output matrix
void CimgDecode::DecodeIdctClear()
{
	memset(m_anDctBlock,  0, (64*sizeof(int)) );
	memset(m_afIdctBlock, 0, (64*sizeof(float)) );
	memset(m_anIdctBlock, 0, (64*sizeof(int)) );

	m_nDctCoefMax = 0;
}

// Set the DCT matrix entry
// Note that we need to convert between the zigzag order and the normal order
void CimgDecode::DecodeIdctSet(unsigned nDqtTbl,unsigned num_coeffs,unsigned zrl,int val)
{
	unsigned ind = num_coeffs+zrl;
	if (ind >= 64) {
		// We have an error! Don't set the block. Skip this comp for now
		// After this call, we will likely trap the error.
	} else {
		unsigned nDctInd = m_sZigZag[ind];
		int nValUnquant = val * m_anDqtCoeffZz[nDqtTbl][ind];

		/*
		// CAL!
		//  For a test, experiment with dropping specific components of
		//  the image. This will help with steganography analysis.
		unsigned nRow = nDctInd/8;
		unsigned nCol = nDctInd - (nRow*8);
		if ((nRow == 0) && (nCol>=0 && nCol<=7)) {
			nValUnquant = 0;
		}
		*/

		m_anDctBlock[nDctInd] = nValUnquant;

		// Update max DCT coef # (after unzigzag) so that we can save
		// some work when performing IDCT.
		// FIXME: The following doesn't seem to work when we later
		// restrict DecodeIdctCalc() to only m_nDctCoefMax coefs!

//		if ( (nDctInd > m_nDctCoefMax) && (abs(nValUnquant) >= IDCT_COEF_THRESH) ) {
		if (nDctInd > m_nDctCoefMax) {
			m_nDctCoefMax = nDctInd;
		}
	}
}

// Precalculate the IDCT lookup tables
// Note that this is 4k entries @ 4B each = 16KB
void CimgDecode::PrecalcIdct()
{
	unsigned	nX,nY,nU,nV;
	unsigned	nYX,nVU;
	float		fCu,fCv;
	float		fCosProd;
	float		fInsideProd;

	float		fPi			= (float)3.141592654;
	float		fSqrtHalf	= (float)0.707106781;

	for (nY=0;nY<8;nY++) {
		for (nX=0;nX<8;nX++) {

			nYX = nY*8+nX;

			for (nV=0;nV<8;nV++) {
				for (nU=0;nU<8;nU++) {

					nVU = nV*8+nU;

					fCu = (nU==0)?fSqrtHalf:1;
					fCv = (nV==0)?fSqrtHalf:1;
					fCosProd = cos((2*nX+1)*nU*fPi/16) * cos((2*nY+1)*nV*fPi/16);
					// Note that the only part we are missing from
					// the "Inside Product" is the "m_afDctBlock[nV*8+nU]" term
					fInsideProd = fCu*fCv*fCosProd;

					// Store the Lookup result
					m_afIdctLookup[nYX][nVU] = fInsideProd;

					// Store a fixed point Lookup as well
					m_anIdctLookup[nYX][nVU] = (int)(fInsideProd * (1<<10));
				}
			}

		}
	}
}

// Perform IDCT
//
// Formula:
//  See itu-t81.pdf, section A.3.3
//
// s(yx) = 1/4*Sum(u=0..7)[ Sum(v=0..7)[ C(u) * C(v) * S(vu) *
//                     cos( (2x+1)*u*Pi/16 ) * cos( (2y+1)*v*Pi/16 ) ] ]
// Cu, Cv = 1/sqrt(2) for u,v=0
// Cu, Cv = 1 else
//

void CimgDecode::DecodeIdctCalcFloat(unsigned nCoefMax)
{
	unsigned	nYX,nVU;
	float		fSum;

/* The following was an attempt to optimize this code by
   eliminating the use of repeated indirect access to class members
   as well as array index calcs. It didn't seem to save any time!
	float*		pfIdctLookup;
	int*		pnDctBlock;
	pfIdctLookup = &m_afIdctLookup[0][0];
	pnDctBlock = &m_anDctBlock[0];
*/



	for (nYX=0;nYX<64;nYX++) {
		fSum = 0;

//		unsigned	pInd = nYX*64;

		// Skip DC coefficient!
		for (nVU=1;nVU<nCoefMax;nVU++) {
			fSum += m_afIdctLookup[nYX][nVU]*m_anDctBlock[nVU];
//			fSum += pfIdctLookup[pInd+nVU]*pnDctBlock[nVU];
		}
		fSum *= 0.25;

		// Store the result
		// FIXME Note that float->int is very slow!
		//   Should consider using fixed point instead!
		m_afIdctBlock[nYX] = fSum;
	}

}

// Fixed point version of DecodeIdctCalcFloat()
void CimgDecode::DecodeIdctCalcFixedpt()
{
	unsigned	nYX,nVU;
	int			nSum;

	for (nYX=0;nYX<64;nYX++) {
		nSum = 0;
		// Skip DC coefficient!
		for (nVU=1;nVU<64;nVU++) {
			nSum += m_anIdctLookup[nYX][nVU] * m_anDctBlock[nVU];
		}

		nSum /= 4;

		// Store the result
		// FIXME Note that float->int is very slow!
		//   Should consider using fixed point instead!
		m_anIdctBlock[nYX] = nSum >> 10;
		
	}

}

void CimgDecode::ClrFullRes()
{
	memset(m_anImgFullres,  0, (4*(2*8)*(2*8)*sizeof(unsigned)) );
}


// Store a small fraction of the final high-res image
// From the IDCT matrix (and any DC level shifting required)
// FIXME later need to allocate DIB space
// PRE: DecodeIdctCalc() already called on Lum AC, and Lum DC already done
void CimgDecode::SetFullRes(unsigned nMcuX,unsigned nMcuY,unsigned nChan,unsigned nCssXInd,unsigned nCssYInd,int nDcOffset)
{
	unsigned	nYX;
	float		fVal;
	int			nVal;

	unsigned	nPixMapW = m_nBlkXMax*8;
	unsigned	nPixmapOffsetBase,nPixmapOffsetLumRow,nPixmapOffsetLumFinal;
	unsigned	nPixmapOffsetChrRow,nPixmapOffsetChrFinal;
	nPixmapOffsetBase = ((nMcuY*m_nMcuHeight) + nCssYInd*8) * nPixMapW + 
		                ((nMcuX*m_nMcuWidth)  + nCssXInd*8);
	nPixmapOffsetLumRow = nPixmapOffsetBase;
	nPixmapOffsetChrRow = nPixmapOffsetBase;

	for (unsigned nY=0;nY<8;nY++) {


		for (unsigned nX=0;nX<8;nX++) {
			nYX = nY*8+nX;
#ifdef IDCT_FIXEDPT
			nVal = m_anIdctBlock[nYX];
			// FIXME Why do I need AC value x8 multiplier?
			nVal = (nVal*8) + nDcOffset;
#else
			fVal = m_afIdctBlock[nYX];
			// FIXME Why do I need AC value x8 multiplier?
			nVal = ((int)(fVal*8) + nDcOffset);
#endif

			// Store in big array
			ASSERT(nChan<3);
			ASSERT(nCssXInd<2);
			ASSERT(nCssYInd<2);
			ASSERT(nY<8);
			ASSERT(nX<8);

			m_anImgFullres[nChan][(nCssYInd*8)+nY][(nCssXInd*8)+nX] = nVal;

			// We are given a PixMap starting offset (index offset to top-left corner
			// of the MCU). We then need to calculate the row offset and then finally
			// add the column offset.

			// Offset is used as an index into the Pix Map, but when we use it,
			// it will be automatically multiplied by 2 to create the address
			// as the array is of short values.
			nPixmapOffsetLumFinal = nPixmapOffsetLumRow + nX;
			nPixmapOffsetChrFinal = nPixmapOffsetChrRow + nX*m_nCssX;

			if (nChan == 0) {
				m_pPixValY[nPixmapOffsetLumFinal] = nVal;
			} else if (nChan == 1) {
				for (unsigned nCssY=0;nCssY<m_nCssY;nCssY++) {
					for (unsigned nCssX=0;nCssX<m_nCssX;nCssX++) {
						m_pPixValCb[nPixmapOffsetChrFinal+(nCssY*nPixMapW)+nCssX] = nVal;
					}
				}
			} else { // Assume Cr
				for (unsigned nCssY=0;nCssY<m_nCssY;nCssY++) {
					for (unsigned nCssX=0;nCssX<m_nCssX;nCssX++) {
						m_pPixValCr[nPixmapOffsetChrFinal+(nCssY*nPixMapW)+nCssX] = nVal;
					}
				}
			}

		}

		nPixmapOffsetLumRow += nPixMapW;
		nPixmapOffsetChrRow += (nPixMapW * m_nCssY);

	}

}




// This routine calculates the actual byte offset (from start of file) for
// the current position in the m_nScanBuff.
CString CimgDecode::GetScanBufPos()
{
	return GetScanBufPos(m_nScanBuffPtr_pos[0],m_nScanBuffPtr_align);
}

// The following routine is used to generate the message string 
CString CimgDecode::GetScanBufPos(unsigned pos, unsigned align)
{
	CString tmpStr;
	tmpStr.Format(_T("0x%08X.%u"),pos,align);
	return tmpStr;
}


void CimgDecode::CheckScanErrors(unsigned nMcuX,unsigned nMcuY,unsigned nTbl,unsigned nInd)
{
	unsigned mcu_width = m_nCssX*8;
	unsigned mcu_height = m_nCssY*8;
	unsigned mcu_x_max = (m_nDimX/mcu_width);
	unsigned mcu_y_max = (m_nDimY/mcu_height);


	unsigned err_pos_x = mcu_width*nMcuX;
	unsigned err_pos_y = mcu_height*nMcuY;

	if (m_nScanCurErr) {
		CString tmpStr,errStr;
		switch (nTbl) {
			// Luminance: In CSS, nInd determine which quadrant
			case 0: // Lum DC
				tmpStr.Format(_T("Lum DC CSS(%u,%u)"),nInd%2,nInd/2);
				err_pos_x += (nInd%2)*8;
				err_pos_y += (nInd/2)*8;
				break;
			case 1: // Lum AC
				tmpStr.Format(_T("Lum AC CSS(%u,%u)"),nInd%2,nInd/2);
				err_pos_x += (nInd%2)*8;
				err_pos_y += (nInd/2)*8;
				break;
			// Chrominance: nInd specifies which chrominance channel Cb (0) or Cr (1)
			case 2: // Chr DC
				if (nInd==0) {tmpStr = "Chr(Cb) DC";}
				else        {tmpStr = "Chr(Cr) DC";}
				break;
			case 3: // Chr AC
				if (nInd==0) {tmpStr = "Chr(Cb) AC";}
				else        {tmpStr = "Chr(Cr) AC";}
				break;
			default:
				tmpStr = "??? Unknown nInd ???";
				break;
		}



		if (m_nWarnBadScanNum < m_nScanErrMax) {

			errStr.Format(_T("*** ERROR: Bad scan data in MCU(%u,%u): %s @ Offset %s"),nMcuX,nMcuY,tmpStr,GetScanBufPos());
			m_pLog->AddLineErr(errStr);
			errStr.Format(_T("           MCU located at pixel=(%u,%u)"),err_pos_x,err_pos_y);
			m_pLog->AddLineErr(errStr);

			//errStr.Format(_T("*** Resetting Error state to continue ***"));
			//m_pLog->AddLineErr(errStr);

			m_nWarnBadScanNum++;
			if (m_nWarnBadScanNum >= m_nScanErrMax) {
				tmpStr.Format(_T("    Only reported first %u instances of this message..."),m_nScanErrMax);
				m_pLog->AddLineErr(tmpStr);
			}
		}


		//CAL!  Should we reset m_nScanCurErr?
		m_nScanCurErr = false;

		//errStr.Format(_T("*** Resetting Error state to continue ***"));
		//m_pLog->AddLineErr(errStr);

	} // is there an error?

}

void CimgDecode::PrintDcCumVal(unsigned nMcuX,unsigned nMcuY,int nVal)
{
	CString tmpStr;
//	tmpStr.Format("  MCU [%4u,%4u] DC Cumulative Val = [%5d]",nMcuX,nMcuY,nVal);
	tmpStr.Format("                 Cumulative DC Val=[%5d]",nVal);
	m_pLog->AddLine(tmpStr);
}


// Reset the DC values in the decoder (e.g. at start and
// after restart markers)
void CimgDecode::DecodeRestartDcState()
{
	nDcLum = 0;
	nDcLumCss[0] = 0;
	nDcLumCss[1] = 0;
	nDcLumCss[2] = 0;
	nDcLumCss[3] = 0;
	nDcChrCb = 0;
	nDcChrCr = 0;
}

// Extract the DC image!
// Supports chroma subsampling
void CimgDecode::DecodeScanImg(unsigned nStart,bool bDisplay,bool bQuiet,CSnoopConfig* pAppConfig)
{

	unsigned	nRet;	// General purpose return value
	bool		bDieOnFirstErr = false; // FIXME - do we want this? It makes it less useful for corrupt jpegs

	// Mirror config values to save repeated dereferencing
	bool		bDumpHistoY		= pAppConfig->bDumpHistoY;
	bool		bDecodeScanAc;
	unsigned	nScanErrMax		= pAppConfig->nErrMaxDecodeScan;

	// Add some extra speed-up in hidden mode (we don't need AC)
	if (bDisplay) {
		bDecodeScanAc = pAppConfig->bDecodeScanImgAc;
	} else {
		bDecodeScanAc = false;
	}

	unsigned	nPixMapW = 0;
	unsigned	nPixMapH = 0;

	m_bHistEn		= pAppConfig->bHistoEn;
	m_bStatClipEn	= pAppConfig->bStatClipEn;
//	bool		bDoCC = pAppConfig->bDecodeColorConvert;


	// Reset the decoder state variables
	Reset();

	ClrFullRes();

	m_nScanErrMax = nScanErrMax;

	m_bDecodeScanAc = bDecodeScanAc;

	if (!m_bImgDetailsSet) {
		m_pLog->AddLineErr(_T("*** ERROR: Decoding image before Image components defined ***"));
		return;
	}

	m_nMcuWidth  = m_nCssX*8;
	m_nMcuHeight = m_nCssY*8;
	m_nMcuXMax   = (m_nDimX/m_nMcuWidth);
	m_nMcuYMax   = (m_nDimY/m_nMcuHeight);

	m_nImgSizeXPartMcu = m_nMcuXMax * m_nMcuWidth;
	m_nImgSizeYPartMcu = m_nMcuYMax * m_nMcuHeight;

	// Check for incomplete MCUs
	if ((m_nDimX%m_nMcuWidth) != 0) m_nMcuXMax++;
	if ((m_nDimY%m_nMcuHeight) != 0) m_nMcuYMax++;

	// Save the maximum 8x8 block dimensions
	m_nBlkXMax = m_nMcuXMax * m_nCssX;
	m_nBlkYMax = m_nMcuYMax * m_nCssY;


	// Set the decoded size and before scaling
	m_nImgSizeX = m_nMcuXMax * m_nMcuWidth;
	m_nImgSizeY = m_nMcuYMax * m_nMcuHeight;

	m_rectImgBase = CRect(CPoint(0,0),CSize(m_nImgSizeX,m_nImgSizeY));


	// Determine decoding range!
	unsigned	nDecMcuRowStart;
	unsigned	nDecMcuRowEnd;		// End to AC scan decoding
	unsigned	nDecMcuRowEndFinal; // End to general decoding
	nDecMcuRowStart = 0;
	nDecMcuRowEnd = m_nMcuYMax;
	nDecMcuRowEndFinal = m_nMcuYMax;

	// Limit the decoding range to valid range
	nDecMcuRowEnd = min(nDecMcuRowEnd,m_nMcuYMax);
	nDecMcuRowEndFinal = min(nDecMcuRowEndFinal,m_nMcuYMax);


	// Allocate the new MCU File Map
	m_pMcuFileMap = new unsigned[m_nMcuYMax*m_nMcuXMax];
	if (!m_pMcuFileMap) {
		AfxMessageBox("ERROR: Not enough memory for Image Decoder MCU File Pos Map");
		exit(1);
	}
	memset(m_pMcuFileMap, 0, (m_nMcuYMax*m_nMcuXMax*sizeof(unsigned)) );


	// Allocate the Image MCU markers
	// Assume that Marked MCU map is already allocated (in Constructor)
	// Double-check that it is large enough for our use
	if (!m_abMarkedMcuMap) {
		AfxMessageBox("ERROR: Not enough memory for Image Decoder MCU Marker Map");
		exit(1);
	}
	if (m_nMcuYMax*m_nMcuXMax > MARKMCUMAP_SIZE) {
		AfxMessageBox("ERROR: Not enough Marker MCU Map memory for current image size.");
		exit(1);
	}
/*
	m_abMarkedMcuMap = new bool[m_nMcuYMax*m_nMcuXMax];
	if (!m_abMarkedMcuMap) {
		AfxMessageBox("ERROR: Not enough memory for Image Decoder MCU Marker Map");
		exit(1);
	}
	memset(m_abMarkedMcuMap, 0, (m_nMcuYMax*m_nMcuXMax*sizeof(bool)) );
*/

	// Allocate the 8x8 Block DC Map
	m_pBlkValY  = new short[m_nBlkYMax*m_nBlkXMax];
	m_pBlkValCb = new short[m_nBlkYMax*m_nBlkXMax];
	m_pBlkValCr = new short[m_nBlkYMax*m_nBlkXMax];
	if ( (!m_pBlkValY) || (!m_pBlkValCb) || (!m_pBlkValCr) ) {
		AfxMessageBox("ERROR: Not enough memory for Image Decoder Blk DC Value Map");
		exit(1);
	}
	memset(m_pBlkValY,  0, (m_nBlkYMax*m_nBlkXMax*sizeof(short)) );
	memset(m_pBlkValCb, 0, (m_nBlkYMax*m_nBlkXMax*sizeof(short)) );
	memset(m_pBlkValCr, 0, (m_nBlkYMax*m_nBlkXMax*sizeof(short)) );

	// Allocate the real YCC pixel Map
	nPixMapW = m_nBlkYMax*8;
	nPixMapH = m_nBlkXMax*8;
	m_pPixValY  = new short[nPixMapW * nPixMapH];
	m_pPixValCb = new short[nPixMapW * nPixMapH];
	m_pPixValCr = new short[nPixMapW * nPixMapH];
	if ( (!m_pPixValY) || (!m_pPixValCb) || (!m_pPixValCr) ) {
		AfxMessageBox("ERROR: Not enough memory for Image Decoder Pixel YCC Value Map");
		exit(1);
	}
	if (bDisplay) {
		memset(m_pPixValY,  0, (nPixMapW * nPixMapH * sizeof(short)) );
		memset(m_pPixValCb, 0, (nPixMapW * nPixMapH * sizeof(short)) );
		memset(m_pPixValCr, 0, (nPixMapW * nPixMapH * sizeof(short)) );
	}

	unsigned char *		p_dib_imgtmp_bits = NULL;
	unsigned			dib_img_row_bytes;
	// -------------------------------------
	// If a previous bitmap was created, deallocate it and nStart fresh
	my_DIBtmp.Kill();
	my_DIB_ready = false;

	// Create the DIB
	// Even though we are creating a 32-bit DIB, I have tested
	// it successfully when running JPEGsnoop in both 16- and 8-bit
	// modes!

	bool bDoImage = false;	// Are we safe to set bits?
	
	if (bDisplay) {
		my_DIBtmp.CreateDIB(m_nImgSizeX,m_nImgSizeY,32);
		dib_img_row_bytes = m_nImgSizeX * sizeof(RGBQUAD);
		p_dib_imgtmp_bits = (unsigned char*) ( my_DIBtmp.GetDIBBitArray() );

		if (p_dib_imgtmp_bits) {
			bDoImage = true;
		} else {
			// FIXME Should we be exiting here?
		}

	}
	// -------------------------------------


	CString tmpStr;
	CString picStr;

	DecodeRestartDcState();
	DecodeRestartScanBuf(nStart);
	m_pWBuf->BufLoadWindow(nStart);

	// Expect that we start with RST0
	m_nRestartExpectInd = 0;
	m_nRestartLastInd = 0;

	unsigned	coord_y;

	// Load the first data into the scan buffer
	// This is done so that the MCU map will already
	// have access to the data.
	BuffTopup();

	if (!bQuiet) {
		m_pLog->AddLineHdr(_T("*** Decoding SCAN Data ***"));
		tmpStr.Format("  OFFSET: 0x%08X",nStart);
		m_pLog->AddLine(tmpStr);
	}

	// We need to ensure that the DQT Table selection has already
	// been done (via a call from JfifDec to SetDqtTables() ).
	unsigned nDqtTblY  =0;
	unsigned nDqtTblCr =0;
	unsigned nDqtTblCb =0;

	if ((m_nNumSofComps!=1) && (m_nNumSofComps !=3)) {
		tmpStr.Format("  NOTE: Number of Image Components not supported [%u]",m_nNumSofComps);
		m_pLog->AddLineWarn(tmpStr);
		return;
	}

	// Check DQT tables

	bool bDqtReady = true;
	for (unsigned ind=0;ind<m_nNumSofComps;ind++) {
		if (m_anDqtTblSel[ind]<0) bDqtReady = false;
	}
	if (!bDqtReady)
	{
		m_pLog->AddLineErr(_T("*** ERROR: Decoding image before DQT Table Selection via JFIF_SOF ***"));
		//CAL! Is this sufficient error handling?
		return;
	} else {
		nDqtTblY  = m_anDqtTblSel[0];
		nDqtTblCb = m_anDqtTblSel[1];
		nDqtTblCr = m_anDqtTblSel[2];
	}

	// Now check DHT tables

	bool bDhtReady = true;
	unsigned nDhtTbl0,nDhtTbl1,nDhtTbl2,nDhtTbl3;
	for (unsigned nClass=0;nClass<2;nClass++) {
		for (unsigned ind=0;ind<m_nNumSosComps;ind++) {
			if (m_anDhtTblSel[nClass][ind]<0) bDhtReady = false;
		}
	}

	// Ensure that the table has been defined already!
	unsigned nSel;
	for (unsigned ind=0;ind<m_nNumSosComps;ind++) {
		// Check for DC DHT table
		nSel = m_anDhtTblSel[0][ind];
		if (m_anDhtLookupSize[0][nSel] == 0) {
			bDhtReady = false;
		}
		// Check for AC DHT table
		nSel = m_anDhtTblSel[1][ind];
		if (m_anDhtLookupSize[1][nSel] == 0) {
			bDhtReady = false;
		}
	}


	if (!bDhtReady)
	{
		m_pLog->AddLineErr(_T("*** ERROR: Decoding image before DHT Table Selection via JFIF_SOS ***"));
		//CAL! Is this sufficient error handling?
		return;
	} else {
		// FIXME Don't like the way I'm having separate table for DC & AC!
		// Note: Doesn't matter if I am assinging some that don't exist
		// because previous check ensure that all ones I plan to use do exist.
		nDhtTbl0 = m_anDhtTblSel[0][0];
		nDhtTbl1 = m_anDhtTblSel[0][1];
		nDhtTbl2 = m_anDhtTblSel[0][2];
		nDhtTbl3 = m_anDhtTblSel[0][3];
	}

	// Done checks


	// Inform if they are in AC+DC/DC mode
	if (!bQuiet) {
		if (m_bDecodeScanAc) {
			m_pLog->AddLine("  Scan Decode Mode: Full IDCT (AC + DC)");
		} else {
			m_pLog->AddLine("  Scan Decode Mode: No IDCT (DC only)");
			m_pLog->AddLineWarn("    NOTE: Low-resolution DC component shown. Can decode full-res with [Options->Scan Segment->Full IDCT]");
		}
		m_pLog->AddLine("");
	}

	// Report any Buffer overlays
	m_pWBuf->ReportOverlays(m_pLog);

	//CAL! Should add a check to make sure that we only process
	//     files that have m_nImgSofCompNum == 1 or 3

	// Temporary pixel value for color conversion
	PixelCcClip	my_pix_clip;

	m_nNumPixels = 0;

	if (bDisplay) {
		// Clear CC clipping stats
		memset(&my_pix_clip,0,sizeof(my_pix_clip));
		memset(&m_sHisto,0,sizeof(m_sHisto));


		// FIXME: Histo should now be done after color convert!!
		memset(&m_pCC_histo_r,0,sizeof(m_pCC_histo_r));
		memset(&m_pCC_histo_g,0,sizeof(m_pCC_histo_g));
		memset(&m_pCC_histo_b,0,sizeof(m_pCC_histo_b));

		memset(&m_histo_y_full,0,sizeof(m_histo_y_full));
		memset(&m_histo_y_subset,0,sizeof(m_histo_y_subset));
	}

	// Invalidate last preview because we're going to
	// be changing it now
	m_nPreviewModeDone = PREVIEW_NONE;


    LONG lIdle = 0;




	for (unsigned mcu_y=nDecMcuRowStart;mcu_y<nDecMcuRowEndFinal;mcu_y++) {

		// Profiling:
		//   No difference observed when SetStatusText() turned off

		// Set the statusbar text to Processing...
		tmpStr.Format("Decoding Scan Data... Row %04u of %04u (%3.0f%%)",mcu_y,m_nMcuYMax,mcu_y*100.0/m_nMcuYMax);
		SetStatusText(tmpStr);

		/*
		// Do some idle loop processing
		if ( ((mcu_y*100/m_nMcuYMax) % 10) == 0) {

			MSG msg;
			while ( ::PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) ) 
			{ 
				if ( !::PumpMessage( ) ) 
				{ 
					//bDoingBackgroundProcessing = FALSE; 
					::PostQuitMessage( ); 
					break; 
				} 
			} 


			AfxGetApp()->OnIdle(lIdle++ );
		}
		*/


		// Ideally, I'd like to trap escape keypress here!

		bool bScanStop = false;
		for (unsigned mcu_x=0;(mcu_x<m_nMcuXMax)&&(!bScanStop);mcu_x++) {

			// Check to see if we should expect a restart marker!
			// FIXME: Should actually check to ensure that we do in
			// fact get a restart marker, and that it was the right
			// one!
			if ((m_bRestartEn) && (m_nRestartMcusLeft == 0)) {

/*
//				if (m_bVerbose) {
//					tmpStr.Format(_T("  Expect Restart interval elapsed @ %s"),GetScanBufPos());
//					m_pLog->AddLine(tmpStr);
//				}
*/

				if (m_bRestartRead) {
/*
					// FIXME: Check for restart counter value match
					if (m_bVerbose) {
						tmpStr.Format(_T("  Restart marker matched"));
						m_pLog->AddLine(tmpStr);
					}
*/
				} else {
					// 
//					if (m_bVerbose) {
					tmpStr.Format(_T("  Expect Restart interval elapsed @ %s"),GetScanBufPos());
					m_pLog->AddLine(tmpStr);
						tmpStr.Format(_T("    ERROR: Restart marker not detected"));
						m_pLog->AddLineErr(tmpStr);
//					}
				}
/*
				if (ExpectRestart()) {
//					if (m_bVerbose) {
						tmpStr.Format(_T("  Restart marker detected"));
						m_pLog->AddLine(tmpStr);
//					}
				} else {
					tmpStr.Format(_T("  ERROR: Restart marker expected but not found @ %s"),GetScanBufPos());
					m_pLog->AddLineErr(tmpStr);
				}
*/


			}

			// To speed things up, allow a subrange to have AC
			// decoding, revert to DC for the rest
			if ((mcu_y<nDecMcuRowStart) || (mcu_y>nDecMcuRowEnd)) {
				m_bDecodeScanAc = false;
			} else {
				m_bDecodeScanAc = bDecodeScanAc;
			}



			// Precalculate MCU matrix index
			unsigned nMcuXY = mcu_y*m_nMcuXMax+mcu_x;

			// Mark the start of the MCU in the file map
			m_pMcuFileMap[nMcuXY] = PackFileOffset(m_nScanBuffPtr_pos[0],m_nScanBuffPtr_align);

			// Is this an MCU that we want full printing of decode process?
			bool		bVlcDump = false;
			unsigned	nRangeBase;
			unsigned	nRangeCur;
			if (m_bDetailVlc) {
				nRangeBase = (m_nDetailVlcY * m_nMcuXMax) + m_nDetailVlcX;
				nRangeCur  = nMcuXY;
				if ( (nRangeCur >= nRangeBase) && (nRangeCur < nRangeBase+m_nDetailVlcLen) ) {
                    bVlcDump = true;
				}
			}

			// Luminance
			// If there is chroma subsampling, then this block will have
			//    (css_x * css_y) luminance blocks to process
			// We store them all in an array nDcLumCss[3:0]

			// Give separator line between MCUs
			if (bVlcDump) {
				m_pLog->AddLine("");
			}

			// CSS array indices
			unsigned nCssIndH,nCssIndV;

			// Reset the output IDCT matrix for this MCU
			if (bDisplay)
				ClrFullRes();

			for (nCssIndV=0;nCssIndV<m_nCssY;nCssIndV++) {
				for (nCssIndH=0;nCssIndH<m_nCssX;nCssIndH++) {
					if (!bVlcDump) {
						nRet = DecodeScanComp(nDhtTbl0,mcu_x,mcu_y);// Lum DC+AC
					} else {
						nRet = DecodeScanCompPrint(nDhtTbl0,mcu_x,mcu_y);// Lum DC+AC
					}
					if (m_nScanCurErr) CheckScanErrors(mcu_x,mcu_y,nDhtTbl0,nCssIndV*2+nCssIndH);
					if (!nRet && bDieOnFirstErr) return;

					// The DCT Block matrix has already been dezigzagged
					// and multiplied against quantization table entry
					nDcLum += m_anDctBlock[0];



					if (bVlcDump) {
//						PrintDcCumVal(mcu_x,mcu_y,nDcLum);
					}

					// Now take a snapshot of the current cumulative DC value
					nDcLumCss[nCssIndV*2+nCssIndH] = nDcLum;

					// At this point we have one of the luminance comps
					// fully decoded (with IDCT if enabled). The result is
					// currently in the array: m_afIdctBlock[]
					// The next step would be to move these elements into
					// m_anImgFullres[] (3 channels x MCU pixel sized)


					// Profiling: (canon_1dsmk2_fine)
					//   Including: ClrFullRes(), SetFullRes(), m_pMcuFileMap[] set
					//   Enabled  - 34 sec
					//   Disabled - 31 sec
					//   10% of total time

					// Store fullres value
					// IDCT has already been computed on the 8x8 (or larger) MCU
					// block region. Now we're transferring the pixel values to
					// the actual bitmap!
					if (bDisplay)
						SetFullRes(mcu_x,mcu_y,0,nCssIndH,nCssIndV,nDcLum);

					// By counting the pixels here, we can increment by 64 each
					// time through. We are assuming that luminance is never
					// subsampled!!
					m_nNumPixels += 64;

				}
			}
			
			// In a grayscale image, we don't do this part!
			if (m_nNumSofComps == 3) {

				// Chrominance Cb
				if (!bVlcDump) {
					nRet = DecodeScanComp(nDhtTbl1,mcu_x,mcu_y);// Chr Cb DC+AC
				} else {
					nRet = DecodeScanCompPrint(nDhtTbl1,mcu_x,mcu_y);// Chr Cb DC+AC
				}
				if (m_nScanCurErr) CheckScanErrors(mcu_x,mcu_y,nDhtTbl1,0);
				if (!nRet && bDieOnFirstErr) return;

				nDcChrCb += m_anDctBlock[0];


				if (bVlcDump) {
//					PrintDcCumVal(mcu_x,mcu_y,nDcChrCb);
				}

				// Store fullres value
				if (bDisplay)
					SetFullRes(mcu_x,mcu_y,1,0,0,nDcChrCb);


				// Chrominance Cr
				if (!bVlcDump) {
					nRet = DecodeScanComp(nDhtTbl2,mcu_x,mcu_y);// Chr Cr DC+AC
				} else {
					nRet = DecodeScanCompPrint(nDhtTbl2,mcu_x,mcu_y);// Chr Cr DC+AC
				}
				if (m_nScanCurErr) CheckScanErrors(mcu_x,mcu_y,nDhtTbl2,1);
				if (!nRet && bDieOnFirstErr) return;

				nDcChrCr += m_anDctBlock[0];



				if (bVlcDump) {
//					PrintDcCumVal(mcu_x,mcu_y,nDcChrCr);
				}

				// Store fullres value
				if (bDisplay)
					SetFullRes(mcu_x,mcu_y,2,0,0,nDcChrCr);

			}



			// Now save the DC YCC values (expanded per 8x8 block)
			// without ranging or translation into RGB.
			for (nCssIndV=0;nCssIndV<m_nCssY;nCssIndV++) {
				for (nCssIndH=0;nCssIndH<m_nCssX;nCssIndH++) {
					// Calculate upper-left Blk index
					unsigned nBlkXY = (mcu_y*m_nCssY+nCssIndV)*m_nBlkXMax + (mcu_x*m_nCssX+nCssIndH);
					m_pBlkValY [nBlkXY] = nDcLumCss[nCssIndV*2+nCssIndH];
					m_pBlkValCb[nBlkXY] = nDcChrCb;
					m_pBlkValCr[nBlkXY] = nDcChrCr;
				}
			}


			// Now that we finished an MCU, decrement the restart interval counter
			if (m_bRestartEn) {
				m_nRestartMcusLeft--;
			}

			// Check to see if we need to abort for some reason.
			// Note that only check m_bScanEnd if we have a failure.
			// m_bScanEnd is asserted during normal out-of-data when scan
			// segment ends with marker. We don't want to abort early
			// or else we'll not decode the last MCU or two!
			if (m_bScanEnd && m_bScanBad) {
				bScanStop = true;
			}

		} // mcu_x


	} // mcu_y
	if (!bQuiet) {
		m_pLog->AddLine("");
	}

	// ---------------------------------------------------------

	// Now let's create the final preview. Since we have just finished
	// decoding a new image, we need to ensure that we invalidate
	// the temporary preview (multi-channel option). Done earlier
	// with PREVIEW_NONE
	if (bDisplay) {
		CalcChannelPreview();
	}

	// DIB is ready for bDisplay now
	if (bDisplay) {
		my_DIB_ready = true;
	}

	if (!bQuiet) {

		// Report Compression stats
		tmpStr.Format(_T("  Compression stats:"));
		m_pLog->AddLine(tmpStr);
		float compression_ratio = (float)(m_nDimX*m_nDimY*m_nNumSofComps*8) / (float)((m_nScanBuffPtr_pos[0]-m_nScanBuffPtr_start)*8);
		tmpStr.Format(_T("    Compression Ratio: %5.2f:1"),compression_ratio);
		m_pLog->AddLine(tmpStr);
		float bits_per_pixel = (float)((m_nScanBuffPtr_pos[0]-m_nScanBuffPtr_start)*8) / (float)(m_nDimX*m_nDimY);
		tmpStr.Format(_T("    Bits per pixel:    %5.2f:1"),bits_per_pixel);
		m_pLog->AddLine(tmpStr);
		m_pLog->AddLine("");


		// Report Huffman stats
		tmpStr.Format(_T("  Huffman code histogram stats:"));
		m_pLog->AddLine(tmpStr);

		unsigned dht_histo_total;
		for (unsigned nClass=0;nClass<2;nClass++) {
		for (unsigned ind1=0;ind1<=m_anDhtLookupSetMax[nClass];ind1++) {
			dht_histo_total = 0;
			for (unsigned ind2=1;ind2<=16;ind2++) {
				dht_histo_total += m_anDhtHisto[nClass][ind1][ind2];
			}

			tmpStr.Format(_T("    Huffman Table: (Dest ID: %u, Class: %s)"),ind1,(nClass?"AC":"DC"));
			m_pLog->AddLine(tmpStr);
			for (unsigned ind2=1;ind2<=16;ind2++) {
				tmpStr.Format(_T("      # codes of length %02u bits: %8u (%3.0f%%)"),
					ind2,m_anDhtHisto[nClass][ind1][ind2],(m_anDhtHisto[nClass][ind1][ind2]*100.0)/dht_histo_total);
				m_pLog->AddLine(tmpStr);
			}
			m_pLog->AddLine("");
		}
		}

		// Report YCC stats
		if (CC_CLIP_YCC_EN) {
			tmpStr.Format(_T("  YCC clipping in DC:"));
			m_pLog->AddLine(tmpStr);
			tmpStr.Format(_T("    Y  component: [<0=%5u] [>255=%5u]"),my_pix_clip.y_under,my_pix_clip.y_over);
			m_pLog->AddLine(tmpStr);
			tmpStr.Format(_T("    Cb component: [<0=%5u] [>255=%5u]"),my_pix_clip.cb_under,my_pix_clip.cb_over);
			m_pLog->AddLine(tmpStr);
			tmpStr.Format(_T("    Cr component: [<0=%5u] [>255=%5u]"),my_pix_clip.cr_under,my_pix_clip.cr_over);
			m_pLog->AddLine(tmpStr);
			m_pLog->AddLine("");
		}

		if (m_bHistEn) {

			tmpStr.Format(_T("  YCC histogram in DC (DCT sums : pre-ranged:"));
			m_pLog->AddLine(tmpStr);
			tmpStr.Format(_T("    Y  component histo: [min=%5d max=%5d avg=%7.1f]"),
				m_sHisto.y_pre_min,m_sHisto.y_pre_max,(float)m_sHisto.y_pre_sum / (float)m_sHisto.count);
			m_pLog->AddLine(tmpStr);
			tmpStr.Format(_T("    Cb component histo: [min=%5d max=%5d avg=%7.1f]"),
				m_sHisto.cb_pre_min,m_sHisto.cb_pre_max,(float)m_sHisto.cb_pre_sum / (float)m_sHisto.count);
			m_pLog->AddLine(tmpStr);
			tmpStr.Format(_T("    Cr component histo: [min=%5d max=%5d avg=%7.1f]"),
				m_sHisto.cr_pre_min,m_sHisto.cr_pre_max,(float)m_sHisto.cr_pre_sum / (float)m_sHisto.count);
			m_pLog->AddLine(tmpStr);
			m_pLog->AddLine("");

			tmpStr.Format(_T("  YCC histogram in DC:"));
			m_pLog->AddLine(tmpStr);
			tmpStr.Format(_T("    Y  component histo: [min=%5d max=%5d avg=%7.1f]"),
				m_sHisto.y_min,m_sHisto.y_max,(float)m_sHisto.y_sum / (float)m_sHisto.count);
			m_pLog->AddLine(tmpStr);
			tmpStr.Format(_T("    Cb component histo: [min=%5d max=%5d avg=%7.1f]"),
				m_sHisto.cb_min,m_sHisto.cb_max,(float)m_sHisto.cb_sum / (float)m_sHisto.count);
			m_pLog->AddLine(tmpStr);
			tmpStr.Format(_T("    Cr component histo: [min=%5d max=%5d avg=%7.1f]"),
				m_sHisto.cr_min,m_sHisto.cr_max,(float)m_sHisto.cr_sum / (float)m_sHisto.count);
			m_pLog->AddLine(tmpStr);
			m_pLog->AddLine("");


			tmpStr.Format(_T("  RGB histogram in DC (before clip):"));
			m_pLog->AddLine(tmpStr);
			tmpStr.Format(_T("    R  component histo: [min=%5d max=%5d avg=%7.1f]"),
				m_sHisto.r_pre_min,m_sHisto.r_pre_max,(float)m_sHisto.r_pre_sum / (float)m_sHisto.count);
			m_pLog->AddLine(tmpStr);
			tmpStr.Format(_T("    G  component histo: [min=%5d max=%5d avg=%7.1f]"),
				m_sHisto.g_pre_min,m_sHisto.g_pre_max,(float)m_sHisto.g_pre_sum / (float)m_sHisto.count);
			m_pLog->AddLine(tmpStr);
			tmpStr.Format(_T("    B  component histo: [min=%5d max=%5d avg=%7.1f]"),
				m_sHisto.b_pre_min,m_sHisto.b_pre_max,(float)m_sHisto.b_pre_sum / (float)m_sHisto.count);
			m_pLog->AddLine(tmpStr);
			m_pLog->AddLine("");
		}

		tmpStr.Format(_T("  RGB clipping in DC:"));
		m_pLog->AddLine(tmpStr);
		tmpStr.Format(_T("    R  component: [<0=%5u] [>255=%5u]"),my_pix_clip.r_under,my_pix_clip.r_over);
		m_pLog->AddLine(tmpStr);
		tmpStr.Format(_T("    G  component: [<0=%5u] [>255=%5u]"),my_pix_clip.g_under,my_pix_clip.g_over);
		m_pLog->AddLine(tmpStr);
		tmpStr.Format(_T("    B  component: [<0=%5u] [>255=%5u]"),my_pix_clip.b_under,my_pix_clip.b_over);
		m_pLog->AddLine(tmpStr);
		/*
		tmpStr.Format(_T("    White Highlight:         [>255=%5u]"),my_pix_clip.white_over);
		m_pLog->AddLine(tmpStr);
		*/
		m_pLog->AddLine("");

	}	// !bQuiet

	if (bDisplay && m_bHistEn) {

		if (!bQuiet) {
			tmpStr.Format(_T("  RGB histogram in DC (after clip):"));
			m_pLog->AddLine(tmpStr);
			tmpStr.Format(_T("    R  component histo: [min=%5d max=%5d avg=%7.1f]"),
				m_sHisto.r_min,m_sHisto.r_max,(float)m_sHisto.r_sum / (float)m_sHisto.count);
			m_pLog->AddLine(tmpStr);
			tmpStr.Format(_T("    G  component histo: [min=%5d max=%5d avg=%7.1f]"),
				m_sHisto.g_min,m_sHisto.g_max,(float)m_sHisto.g_sum / (float)m_sHisto.count);
			m_pLog->AddLine(tmpStr);
			tmpStr.Format(_T("    B  component histo: [min=%5d max=%5d avg=%7.1f]"),
				m_sHisto.b_min,m_sHisto.b_max,(float)m_sHisto.b_sum / (float)m_sHisto.count);
			m_pLog->AddLine(tmpStr);
			m_pLog->AddLine("");
		}		

		// --------------------------------------
		// Now draw the RGB histogram!

		unsigned		histo_bin_height;
		unsigned		histo_peak_val;
		unsigned		histo_x;
		unsigned		histo_cur_val;
		unsigned		dib_histo_row_bytes;
		unsigned char*	p_dib_histo_bits;

		my_hist_rgb_DIB.Kill();
		my_hist_rgb_DIB_ready = false;

		my_hist_rgb_DIB.CreateDIB(HISTO_BINS*HISTO_BIN_WIDTH,3*HISTO_BIN_HEIGHT,32);
		dib_histo_row_bytes = (HISTO_BINS*HISTO_BIN_WIDTH) * 4;
		p_dib_histo_bits = (unsigned char*) ( my_hist_rgb_DIB.GetDIBBitArray() );

		m_rectHistBase = CRect(CPoint(0,0),CSize(HISTO_BINS*HISTO_BIN_WIDTH,3*HISTO_BIN_HEIGHT));

		if (p_dib_histo_bits != NULL) {
			memset(p_dib_histo_bits,0,3*HISTO_BIN_HEIGHT*dib_histo_row_bytes);

			// Do peak detect first
			// Don't want to reset peak value to 0 as otherwise we might get
			// division by zero later when we calculate histo_bin_height
			histo_peak_val = 1;

			// Peak value is across all three channels!
			for (unsigned hist_chan=0;hist_chan<3;hist_chan++) {

				for (unsigned i=0;i<HISTO_BINS;i++) {
					if      (hist_chan == 0) {histo_cur_val = m_pCC_histo_r[i];}
					else if (hist_chan == 1) {histo_cur_val = m_pCC_histo_g[i];}
					else                     {histo_cur_val = m_pCC_histo_b[i];}
					histo_peak_val = (histo_cur_val > histo_peak_val)?histo_cur_val:histo_peak_val;
				}
			}

			for (unsigned hist_chan=0;hist_chan<3;hist_chan++) {
				for (unsigned i=0;i<HISTO_BINS;i++) {

					// Calculate bin's height (max HISTO_BIN_HEIGHT)
					if      (hist_chan == 0) {histo_cur_val = m_pCC_histo_r[i];}
					else if (hist_chan == 1) {histo_cur_val = m_pCC_histo_g[i];}
					else                     {histo_cur_val = m_pCC_histo_b[i];}
					histo_bin_height = HISTO_BIN_HEIGHT*histo_cur_val/histo_peak_val;

					for (unsigned y=0;y<histo_bin_height;y++) {
						// Store the RGB triplet
						for (unsigned bin_width=0;bin_width<HISTO_BIN_WIDTH;bin_width++) {
							histo_x = (i*HISTO_BIN_WIDTH)+bin_width;
							coord_y = ((2-hist_chan) * HISTO_BIN_HEIGHT) + y;
							p_dib_histo_bits[(histo_x*4)+3+(coord_y*dib_histo_row_bytes)] = 0;
							p_dib_histo_bits[(histo_x*4)+2+(coord_y*dib_histo_row_bytes)] = (hist_chan==0)?255:0;
							p_dib_histo_bits[(histo_x*4)+1+(coord_y*dib_histo_row_bytes)] = (hist_chan==1)?255:0;
							p_dib_histo_bits[(histo_x*4)+0+(coord_y*dib_histo_row_bytes)] = (hist_chan==2)?255:0;
						}
					}
				} // i: 0..HISTO_BINS-1

			} // hist_chan

			my_hist_rgb_DIB_ready = true;
		}

		// Only create the Y DC Histogram if requested
		my_hist_y_DIB_ready = false;
		if (bDumpHistoY) {

			my_hist_y_DIB.Kill();

			my_hist_y_DIB.CreateDIB(SUBSET_HISTO_BINS*HISTO_BIN_WIDTH,HISTO_BIN_HEIGHT,32);
			dib_histo_row_bytes = (SUBSET_HISTO_BINS*HISTO_BIN_WIDTH) * 4;
			p_dib_histo_bits = (unsigned char*) ( my_hist_y_DIB.GetDIBBitArray() );

			m_rectHistYBase = CRect(CPoint(0,0),CSize(SUBSET_HISTO_BINS*HISTO_BIN_WIDTH,HISTO_BIN_HEIGHT));

			if (p_dib_histo_bits != NULL) {
				memset(p_dib_histo_bits,0,HISTO_BIN_HEIGHT*dib_histo_row_bytes);

				// Do peak detect first
				// Don't want to reset peak value to 0 as otherwise we might get
				// division by zero later when we calculate histo_bin_height
				histo_peak_val = 1;

				//****** CAL! Temporarily made quarter width! need to resample instead

				// Peak value
				for (unsigned i=0;i<SUBSET_HISTO_BINS;i++) {
					histo_cur_val  = m_histo_y_full[i*4+0];
					histo_cur_val += m_histo_y_full[i*4+1];
					histo_cur_val += m_histo_y_full[i*4+2];
					histo_cur_val += m_histo_y_full[i*4+3];
					histo_peak_val = (histo_cur_val > histo_peak_val)?histo_cur_val:histo_peak_val;
				}

				for (unsigned i=0;i<SUBSET_HISTO_BINS;i++) {

					// Calculate bin's height (max HISTO_BIN_HEIGHT)
					histo_cur_val  = m_histo_y_full[i*4+0];
					histo_cur_val += m_histo_y_full[i*4+1];
					histo_cur_val += m_histo_y_full[i*4+2];
					histo_cur_val += m_histo_y_full[i*4+3];
					histo_bin_height = HISTO_BIN_HEIGHT*histo_cur_val/histo_peak_val;

					for (unsigned y=0;y<histo_bin_height;y++) {
						// Store the RGB triplet
						for (unsigned bin_width=0;bin_width<HISTO_BIN_WIDTH;bin_width++) {
							histo_x = (i*HISTO_BIN_WIDTH)+bin_width;
							coord_y = y;
							p_dib_histo_bits[(histo_x*4)+3+(coord_y*dib_histo_row_bytes)] = 0;
							p_dib_histo_bits[(histo_x*4)+2+(coord_y*dib_histo_row_bytes)] = 255;
							p_dib_histo_bits[(histo_x*4)+1+(coord_y*dib_histo_row_bytes)] = 255;
							p_dib_histo_bits[(histo_x*4)+0+(coord_y*dib_histo_row_bytes)] = 0;
						}
					}
				} // i: 0..HISTO_BINS-1
				my_hist_y_DIB_ready = true;

			}
		} // bDumpHistoY

	} // m_bHistEn

	// --------------------------------------

	if (bDisplay && m_bAvgYValid) {
		m_pLog->AddLine("  Average Pixel Luminance (Y):");
		tmpStr.Format("    Y=[%3u] (range: 0..255)",
			m_nAvgY);
		m_pLog->AddLine(tmpStr);
		m_pLog->AddLine("");
	}

	if (bDisplay && m_bBrightValid) {
		m_pLog->AddLine("  Brightest Pixel Search:");
		tmpStr.Format("    YCC=[%5d,%5d,%5d] RGB=[%3u,%3u,%3u] @ MCU[%3u,%3u]",
			m_nBrightY,m_nBrightCb,m_nBrightCr,m_nBrightR,m_nBrightG,m_nBrightB,
			m_ptBrightMcu.x,m_ptBrightMcu.y);
		m_pLog->AddLine(tmpStr);
		m_pLog->AddLine("");
	}


	// --------------------------------------
	
	if (!bQuiet) {
		m_pLog->AddLine(_T("  Finished Decoding SCAN Data"));
		tmpStr.Format(_T("    Number of RESTART markers decoded: %u"),m_nRestartRead);
		m_pLog->AddLine(tmpStr);
		tmpStr.Format(_T("    Next position in scan buffer: Offset %s"),GetScanBufPos());
		m_pLog->AddLine(tmpStr);

		m_pLog->AddLine("");
	}

	// --------------------------------------
	// Write out the full Y histogram if requested!

	CString fullStr;

	if (bDisplay && m_bHistEn && bDumpHistoY) {
		m_pLog->AddLine(_T("  Y Histogram in DC: (DCT sums) Full"));
		for (unsigned row=0;row<2048/8;row++) {
			fullStr.Format("    Y=%5d..%5d: ",-1024+(row*8),-1024+(row*8)+7);
			for (unsigned col=0;col<8;col++) {
				tmpStr.Format("0x%06x, ",m_histo_y_full[col+row*8]);
				fullStr += tmpStr;
			}
			m_pLog->AddLine(fullStr);
		}
	}


}

// Reset the decoder Scan Buff (at start of scan and
// after any restart markers)
void CimgDecode::DecodeRestartScanBuf(unsigned nFilePos)
{
	// Reset the state
	m_bScanEnd = false;
	m_bScanBad = false;
	m_nScanBuff = 0x00000000;
	m_nScanBuffPtr = nFilePos;
	m_nScanBuffPtr_start = nFilePos;	// Only used for compression ratio
	m_nScanBuffPtr_align = 0;			// Start with byte alignment (0)
	m_nScanBuffPtr_pos[0] = 0;
	m_nScanBuffPtr_pos[1] = 0;
	m_nScanBuffPtr_pos[2] = 0;
	m_nScanBuffPtr_pos[3] = 0;
	m_nScanBuffPtr_err[0] = SCANBUF_OK;
	m_nScanBuffPtr_err[1] = SCANBUF_OK;
	m_nScanBuffPtr_err[2] = SCANBUF_OK;
	m_nScanBuffPtr_err[3] = SCANBUF_OK;
	m_nScanBuffLatchErr = SCANBUF_OK;

	m_nScanBuffPtr_num = 0;		// Empty m_nScanBuff
	m_nScanBuff_vacant = 32;

	m_nScanCurErr = false;

	//
	m_nScanBuffPtr = nFilePos;

	// Reset RST Interval checking
	m_bRestartRead = false;
	m_nRestartMcusLeft = m_nRestartInterval;

}

void CimgDecode::DecodeScanImgQuick(unsigned nStart,bool bDisplay,bool bQuiet,CSnoopConfig* pAppConfig)
{

	unsigned	nRet;	// General purpose return value
	bool		bDieOnFirstErr = false; // FIXME - do we want this? It makes it less useful for corrupt jpegs

	// Mirror config values to save repeated dereferencing
	bool		bDumpHistoY		= pAppConfig->bDumpHistoY;
	bool		bDecodeScanAc	= pAppConfig->bDecodeScanImgAc;
	unsigned	nScanErrMax		= pAppConfig->nErrMaxDecodeScan;

	unsigned	nPixMapW = 0;
	unsigned	nPixMapH = 0;


	// Reset the decoder state variables
	Reset();

	ClrFullRes();

	m_nScanErrMax = nScanErrMax;

	m_bDecodeScanAc = false;


	m_nMcuWidth  = m_nCssX*8;
	m_nMcuHeight = m_nCssY*8;
	m_nMcuXMax   = (m_nDimX/m_nMcuWidth);
	m_nMcuYMax   = (m_nDimY/m_nMcuHeight);

	m_nImgSizeXPartMcu = m_nMcuXMax * m_nMcuWidth;
	m_nImgSizeYPartMcu = m_nMcuYMax * m_nMcuHeight;

	// Check for incomplete MCUs
	if ((m_nDimX%m_nMcuWidth) != 0) m_nMcuXMax++;
	if ((m_nDimY%m_nMcuHeight) != 0) m_nMcuYMax++;

	// Save the maximum 8x8 block dimensions
	m_nBlkXMax = m_nMcuXMax * m_nCssX;
	m_nBlkYMax = m_nMcuYMax * m_nCssY;


	// Set the decoded size and before scaling
	m_nImgSizeX = m_nMcuXMax * m_nMcuWidth;
	m_nImgSizeY = m_nMcuYMax * m_nMcuHeight;

	m_rectImgBase = CRect(CPoint(0,0),CSize(m_nImgSizeX,m_nImgSizeY));


	// Determine decoding range!
	unsigned	nDecMcuRowStart;
	unsigned	nDecMcuRowEnd;		// End to AC scan decoding
	unsigned	nDecMcuRowEndFinal; // End to general decoding
	nDecMcuRowStart = 0;
	nDecMcuRowEnd = m_nMcuYMax;
	nDecMcuRowEndFinal = m_nMcuYMax;

	// Limit the decoding range to valid range
	nDecMcuRowEnd = min(nDecMcuRowEnd,m_nMcuYMax);
	nDecMcuRowEndFinal = min(nDecMcuRowEndFinal,m_nMcuYMax);


	// Allocate the 8x8 Block DC Map
	m_pBlkValY  = new short[m_nBlkYMax*m_nBlkXMax];
	m_pBlkValCb = new short[m_nBlkYMax*m_nBlkXMax];
	m_pBlkValCr = new short[m_nBlkYMax*m_nBlkXMax];
	if ( (!m_pBlkValY) || (!m_pBlkValCb) || (!m_pBlkValCr) ) {
		AfxMessageBox("ERROR: Not enough memory for Image Decoder Blk DC Value Map");
		exit(1);
	}
	memset(m_pBlkValY,  0, (m_nBlkYMax*m_nBlkXMax*sizeof(short)) );
	memset(m_pBlkValCb, 0, (m_nBlkYMax*m_nBlkXMax*sizeof(short)) );
	memset(m_pBlkValCr, 0, (m_nBlkYMax*m_nBlkXMax*sizeof(short)) );

	// -------------------------------------


	CString tmpStr;
	CString picStr;

	DecodeRestartDcState();
	DecodeRestartScanBuf(nStart);
	m_pWBuf->BufLoadWindow(nStart);

	// Expect that we start with RST0
	m_nRestartExpectInd = 0;
	m_nRestartLastInd = 0;



	// Load the first data into the scan buffer
	// This is done so that the MCU map will already
	// have access to the data.
	BuffTopup();

	// We need to ensure that the DQT Table selection has already
	// been done (via a call from JfifDec to SetDqtTables() ).
	unsigned nDqtTblY  =0;
	unsigned nDqtTblCr =0;
	unsigned nDqtTblCb =0;
	unsigned nDhtTbl0  =0;
	unsigned nDhtTbl1  =0;
	unsigned nDhtTbl2  =0;
	unsigned nDhtTbl3  =0;

	// Check DQT tables

		nDqtTblY  = m_anDqtTblSel[0];
		nDqtTblCb = m_anDqtTblSel[1];
		nDqtTblCr = m_anDqtTblSel[2];


		// FIXME Don't like the way I'm having separate table for DC & AC!
		// Note: Doesn't matter if I am assinging some that don't exist
		// because previous check ensure that all ones I plan to use do exist.
		nDhtTbl0 = m_anDhtTblSel[0][0];
		nDhtTbl1 = m_anDhtTblSel[0][1];
		nDhtTbl2 = m_anDhtTblSel[0][2];
		nDhtTbl3 = m_anDhtTblSel[0][3];

	// Done checks


	m_nNumPixels = 0;


	// Invalidate last preview because we're going to
	// be changing it now
	m_nPreviewModeDone = PREVIEW_NONE;


    LONG lIdle = 0;


	for (unsigned mcu_y=nDecMcuRowStart;mcu_y<nDecMcuRowEndFinal;mcu_y++) {

		bool bScanStop = false;
		for (unsigned mcu_x=0;(mcu_x<m_nMcuXMax)&&(!bScanStop);mcu_x++) {

			// Check to see if we should expect a restart marker!
			// FIXME: Should actually check to ensure that we do in
			// fact get a restart marker, and that it was the right
			// one!
/*
			if ((m_bRestartEn) && (m_nRestartMcusLeft == 0)) {

			}
*/

			m_bDecodeScanAc = false;

			// Precalculate MCU matrix index
			unsigned nMcuXY = mcu_y*m_nMcuXMax+mcu_x;

			// CSS array indices
			unsigned nCssIndH,nCssIndV;

			// Reset the output IDCT matrix for this MCU
			ClrFullRes();

			for (nCssIndV=0;nCssIndV<m_nCssY;nCssIndV++) {
				for (nCssIndH=0;nCssIndH<m_nCssX;nCssIndH++) {
					nRet = DecodeScanComp(nDhtTbl0,mcu_x,mcu_y);// Lum DC+AC

					// The DCT Block matrix has already been dezigzagged
					// and multiplied against quantization table entry
					nDcLum += m_anDctBlock[0];

					// Now take a snapshot of the current cumulative DC value
					nDcLumCss[nCssIndV*2+nCssIndH] = nDcLum;

					// Store fullres value
					//SetFullRes(mcu_x,mcu_y,0,nCssIndH,nCssIndV,nDcLum);

					// By counting the pixels here, we can increment by 64 each
					// time through. We are assuming that luminance is never
					// subsampled!!
					m_nNumPixels += 64;

				}
			}
			
			// In a grayscale image, we don't do this part!
			if (m_nNumSofComps == 3) {

				// Chrominance Cb
				nRet = DecodeScanComp(nDhtTbl1,mcu_x,mcu_y);// Chr Cb DC+AC

				nDcChrCb += m_anDctBlock[0];

				// Store fullres value
				//SetFullRes(mcu_x,mcu_y,1,0,0,nDcChrCb);

				// Chrominance Cr
				nRet = DecodeScanComp(nDhtTbl2,mcu_x,mcu_y);// Chr Cr DC+AC

				nDcChrCr += m_anDctBlock[0];

				// Store fullres value
				//SetFullRes(mcu_x,mcu_y,2,0,0,nDcChrCr);

			}



			// Now save the DC YCC values (expanded per 8x8 block)
			// without ranging or translation into RGB.
			for (nCssIndV=0;nCssIndV<m_nCssY;nCssIndV++) {
				for (nCssIndH=0;nCssIndH<m_nCssX;nCssIndH++) {
					// Calculate upper-left Blk index
					unsigned nBlkXY = (mcu_y*m_nCssY+nCssIndV)*m_nBlkXMax + (mcu_x*m_nCssX+nCssIndH);
					m_pBlkValY [nBlkXY] = nDcLumCss[nCssIndV*2+nCssIndH];
					m_pBlkValCb[nBlkXY] = nDcChrCb;
					m_pBlkValCr[nBlkXY] = nDcChrCr;
				}
			}


			// Now that we finished an MCU, decrement the restart interval counter
			if (m_bRestartEn) {
				m_nRestartMcusLeft--;
			}

			// Check to see if we need to abort for some reason.
			// Note that only check m_bScanEnd if we have a failure.
			// m_bScanEnd is asserted during normal out-of-data when scan
			// segment ends with marker. We don't want to abort early
			// or else we'll not decode the last MCU or two!
			if (m_bScanEnd && m_bScanBad) {
				bScanStop = true;
			}

		} // mcu_x


	} // mcu_y

	// ---------------------------------------------------------

}


// This coded reduced time from: 34sec -> 27sec!
void CimgDecode::ConvertYCCtoRGBFastFloat(PixelCc &rPix)
{
	int y,cb,cr;
	float r,g,b;


	// Perform ranging to adjust from Huffman sums to reasonable range
	// -1024..+1024 -> -128..127
	rPix.y_preclip  = rPix.y_prerange >> 3;
	rPix.cb_preclip = rPix.cb_prerange >> 3;
	rPix.cr_preclip = rPix.cr_prerange >> 3;

	// Limit on YCC input
	// The y/cb/cr_preclip values should already be 0..255 unless we have a
	// decode error where DC value gets out of range!
	//CapYccRange(nMcuX,nMcuY,rPix);
	y = (rPix.y_preclip<-128)?-128:(rPix.y_preclip>127)?127:rPix.y_preclip;
	cb = (rPix.cb_preclip<-128)?-128:(rPix.cb_preclip>127)?127:rPix.cb_preclip;
	cr = (rPix.cr_preclip<-128)?-128:(rPix.cr_preclip>127)?127:rPix.cr_preclip;

	// Save the YCC values (0..255)
	rPix.y  = y  + 128;
	rPix.cb = cb + 128;
	rPix.cr = cr + 128;

	// Convert
	// Since the following seems to preserve the multiplies and subtractions
	// we could expand this out manually
	float c_red   = 0.299f;
	float c_green = 0.587f;
	float c_blue  = 0.114f;
	r = cr*(2-2*c_red)+y;
	b = cb*(2-2*c_blue)+y;
	g = (y-c_blue*b-c_red*r)/c_green;
/*
	r = cr * 1.402 + y;
	b = cb * 1.772 + y;
	g = (y - 0.03409 * r) / 0.587;
*/

	// Level shift
	r += 128;
	b += 128;
	g += 128;

	// --------------- Finshed the color conversion


	// Limit
	//   r/g/b_preclip -> r/g/b
	//CapRgbRange(nMcuX,nMcuY,rPix);
	rPix.r = (r<0)?0:(r>255)?255:(BYTE)r;
	rPix.g = (g<0)?0:(g>255)?255:(BYTE)g;
	rPix.b = (b<0)?0:(b>255)?255:(BYTE)b;

}

// This coded reduced time from: 34sec -> 26sec!
void CimgDecode::ConvertYCCtoRGBFastFixed(PixelCc &rPix)
{
	int	y_preclip,cb_preclip,cr_preclip;
	int	y,cb,cr;
	int r,g,b;

	// Perform ranging to adjust from Huffman sums to reasonable range
	// -1024..+1024 -> -128..+127
	y_preclip  = rPix.y_prerange >> 3;
	cb_preclip = rPix.cb_prerange >> 3;
	cr_preclip = rPix.cr_prerange >> 3;


	// Limit on YCC input
	// The y/cb/cr_preclip values should already be 0..255 unless we have a
	// decode error where DC value gets out of range!

	//CapYccRange(nMcuX,nMcuY,rPix);
	y = (y_preclip<-128)?-128:(y_preclip>127)?127:y_preclip;
	cb = (cb_preclip<-128)?-128:(cb_preclip>127)?127:cb_preclip;
	cr = (cr_preclip<-128)?-128:(cr_preclip>127)?127:cr_preclip;

	// Save the YCC values (0..255)
	rPix.y  = y  + 128;
	rPix.cb = cb + 128;
	rPix.cr = cr + 128;

	// --------------

	// Convert
	// Fixed values is x 1024 (10 bits). Leaves 22 bits for integer
	//r2 = 1024*cr1*(2-2*c_red)+1024*y1;
	//b2 = 1024*cb1*(2-2*c_blue)+1024*y1;
	//g2 = 1024*(y1-c_blue*b2/1024-c_red*r2/1024)/c_green;
	const long int	CFIX_R = 306;
	const long int	CFIX_G = 601;
	const long int	CFIX_B = 116;
	const long int	CFIX2_R = 1436; // 2*(1024-cfix_red)
	const long int	CFIX2_B = 1816; // 2*(1024-cfix_blue)
	const long int	CFIX2_G = 1048576; // 1024*1024

	r = CFIX2_R*cr + 1024*y;
	b = CFIX2_B*cb + 1024*y;
	g = (CFIX2_G*y - CFIX_B*b - CFIX_R*r) / CFIX_G;

	r >>= 10;
	g >>= 10;
	b >>= 10;

	// Level shift
	r += 128;
	b += 128;
	g += 128;


	// Limit
	//   r/g/b_preclip -> r/g/b
	rPix.r = (r<0)?0:(r>255)?255:r;
	rPix.g = (g<0)?0:(g>255)?255:g;
	rPix.b = (b<0)?0:(b>255)?255:b;

}



// CC: y/cb/cr -> r/g/b
void CimgDecode::ConvertYCCtoRGB(unsigned nMcuX,unsigned nMcuY,PixelCc &rPix)
{
	float c_red   = (float)0.299;
	float c_green = (float)0.587;
	float c_blue  = (float)0.114;
	int y_byte,cb_byte,cr_byte;
	int y,cb,cr;
	float r,g,b;

	unsigned rgb_r_under = 0;
	unsigned rgb_g_under = 0;
	unsigned rgb_b_under = 0;
	unsigned rgb_r_over = 0;
	unsigned rgb_g_over = 0;
	unsigned rgb_b_over = 0;

	if (m_bHistEn) {
		// Calc stats on preranged YCC (direct from huffman DC sums)
		m_sHisto.y_pre_min = (rPix.y_prerange<m_sHisto.y_pre_min)?rPix.y_prerange:m_sHisto.y_pre_min;
		m_sHisto.y_pre_max = (rPix.y_prerange>m_sHisto.y_pre_max)?rPix.y_prerange:m_sHisto.y_pre_max;
		m_sHisto.y_pre_sum += rPix.y_prerange;
		m_sHisto.cb_pre_min = (rPix.cb_prerange<m_sHisto.cb_pre_min)?rPix.cb_prerange:m_sHisto.cb_pre_min;
		m_sHisto.cb_pre_max = (rPix.cb_prerange>m_sHisto.cb_pre_max)?rPix.cb_prerange:m_sHisto.cb_pre_max;
		m_sHisto.cb_pre_sum += rPix.cb_prerange;
		m_sHisto.cr_pre_min = (rPix.cr_prerange<m_sHisto.cr_pre_min)?rPix.cr_prerange:m_sHisto.cr_pre_min;
		m_sHisto.cr_pre_max = (rPix.cr_prerange>m_sHisto.cr_pre_max)?rPix.cr_prerange:m_sHisto.cr_pre_max;
		m_sHisto.cr_pre_sum += rPix.cr_prerange;
	}

	if (m_bHistEn) {
		// Now generate the Y histogram, if requested
		// Add the Y value to the full histogram (for image similarity calcs)
		//if (bDumpHistoY) {
			int histo_index = rPix.y_prerange;
			if (histo_index < -1024) histo_index = -1024;
			if (histo_index > 1023) histo_index = 1023;
			histo_index += 1024;
			m_histo_y_full[histo_index]++;
		//}
	}

	// Perform ranging to adjust from Huffman sums to reasonable range
	// -1024..+1024 -> 0..255
	// Add 1024 then / 8
	rPix.y_preclip  = (rPix.y_prerange+1024)/8;
	rPix.cb_preclip = (rPix.cb_prerange+1024)/8;
	rPix.cr_preclip = (rPix.cr_prerange+1024)/8;


	// Limit on YCC input
	// The y/cb/cr_preclip values should already be 0..255 unless we have a
	// decode error where DC value gets out of range!
	CapYccRange(nMcuX,nMcuY,rPix);

	// --------------- Perform the color conversion
	y_byte = rPix.y;
	cb_byte = rPix.cb;
	cr_byte = rPix.cr;

	// Level shift
	y  = y_byte - 128;
	cb = cb_byte - 128;
	cr = cr_byte - 128;

	// Convert
	r = cr*(2-2*c_red)+y;
	b = cb*(2-2*c_blue)+y;
	g = (y-c_blue*b-c_red*r)/c_green;

	// Level shift
	r += 128;
	b += 128;
	g += 128;

	rPix.r_preclip = r;
	rPix.g_preclip = g;
	rPix.b_preclip = b;
	// --------------- Finshed the color conversion


	// Limit
	//   r/g/b_preclip -> r/g/b
	CapRgbRange(nMcuX,nMcuY,rPix);

	// Display
	/*
	tmpStr.Format(_T("* (YCC->RGB) @ (%03u,%03u): YCC=(%4d,%4d,%4d) RGB=(%03u,%03u,%u)"),
		nMcuX,nMcuY,y,cb,cr,r_limb,g_limb,b_limb);
	m_pLog->AddLine(tmpStr);
	*/

	if (m_bHistEn) {
		// Bin the result into a histogram!
		//   value = 0..255
		//   bin   = 0..7, 8..15, ..., 248..255
		// Channel: Red
		unsigned bin_divider = 256/HISTO_BINS;
		m_pCC_histo_r[rPix.r/bin_divider]++;
		m_pCC_histo_g[rPix.g/bin_divider]++;
		m_pCC_histo_b[rPix.b/bin_divider]++;


	}

}

// CC: y/cb/cr_preclip -> y/cb/cr
void CimgDecode::CapYccRange(unsigned nMcuX,unsigned nMcuY,PixelCc &rPix)
{
	// Check the bounds on the YCC value
	// It should probably be 0..255 unless our DC
	// values got really messed up in a corrupt file
	// Perhaps it might be best to reset it to 0? Otherwise
	// it will continuously report an out-of-range value.
	int	cur_y,cur_cb,cur_cr;

	cur_y  = rPix.y_preclip;
	cur_cb = rPix.cb_preclip;
	cur_cr = rPix.cr_preclip;

	if (m_bHistEn) {
		m_sHisto.y_min = (cur_y<m_sHisto.y_min)?cur_y:m_sHisto.y_min;
		m_sHisto.y_max = (cur_y>m_sHisto.y_max)?cur_y:m_sHisto.y_max;
		m_sHisto.y_sum += cur_y;
		m_sHisto.cb_min = (cur_cb<m_sHisto.cb_min)?cur_cb:m_sHisto.cb_min;
		m_sHisto.cb_max = (cur_cb>m_sHisto.cb_max)?cur_cb:m_sHisto.cb_max;
		m_sHisto.cb_sum += cur_cb;
		m_sHisto.cr_min = (cur_cr<m_sHisto.cr_min)?cur_cr:m_sHisto.cr_min;
		m_sHisto.cr_max = (cur_cr>m_sHisto.cr_max)?cur_cr:m_sHisto.cr_max;
		m_sHisto.cr_sum += cur_cr;
		m_sHisto.count++;
	}


	if (CC_CLIP_YCC_EN)
	{

		if (cur_y > CC_CLIP_YCC_MAX) {
			if (YCC_CLIP_REPORT_ERR && (m_nWarnYccClipNum < YCC_CLIP_REPORT_MAX)) {
				CString tmpStr;
				tmpStr.Format(_T("*** NOTE: YCC Clipped. MCU=(%4u,%4u) YCC=(%5d,%5d,%5d) Y Overflow @ Offset %s"),
					nMcuX,nMcuY,cur_y,cur_cb,cur_cr,GetScanBufPos());
				m_pLog->AddLineWarn(tmpStr);
				m_nWarnYccClipNum++;
				if (m_nWarnYccClipNum == YCC_CLIP_REPORT_MAX) {
					tmpStr.Format(_T("    Only reported first %u instances of this message..."),YCC_CLIP_REPORT_MAX);
					m_pLog->AddLineWarn(tmpStr);
				}
			}
			rPix.clip |= CC_CLIP_Y_OVER;
			cur_y = CC_CLIP_YCC_MAX;
		}
		if (cur_y < CC_CLIP_YCC_MIN) {
			if (YCC_CLIP_REPORT_ERR && (m_nWarnYccClipNum < YCC_CLIP_REPORT_MAX)) {
				CString tmpStr;
				tmpStr.Format(_T("*** NOTE: YCC Clipped. MCU=(%4u,%4u) YCC=(%5d,%5d,%5d) Y Underflow @ Offset %s"),
					nMcuX,nMcuY,cur_y,cur_cb,cur_cr,GetScanBufPos());
				m_pLog->AddLineWarn(tmpStr);
				m_nWarnYccClipNum++;
				if (m_nWarnYccClipNum == YCC_CLIP_REPORT_MAX) {
					tmpStr.Format(_T("    Only reported first %u instances of this message..."),YCC_CLIP_REPORT_MAX);
					m_pLog->AddLineWarn(tmpStr);
				}
			}
			rPix.clip |= CC_CLIP_Y_UNDER;
			cur_y = CC_CLIP_YCC_MIN;
		}
		if (cur_cb > CC_CLIP_YCC_MAX) {
			if (YCC_CLIP_REPORT_ERR && (m_nWarnYccClipNum < YCC_CLIP_REPORT_MAX)) {
				CString tmpStr;
				tmpStr.Format(_T("*** NOTE: YCC Clipped. MCU=(%4u,%4u) YCC=(%5d,%5d,%5d) Cb Overflow @ Offset %s"),
					nMcuX,nMcuY,cur_y,cur_cb,cur_cr,GetScanBufPos());
				m_pLog->AddLineWarn(tmpStr);
				m_nWarnYccClipNum++;
				if (m_nWarnYccClipNum == YCC_CLIP_REPORT_MAX) {
					tmpStr.Format(_T("    Only reported first %u instances of this message..."),YCC_CLIP_REPORT_MAX);
					m_pLog->AddLineWarn(tmpStr);
				}
			}
			rPix.clip |= CC_CLIP_CB_OVER;
			cur_cb = CC_CLIP_YCC_MAX;
		}
		if (cur_cb < CC_CLIP_YCC_MIN) {
			if (YCC_CLIP_REPORT_ERR && (m_nWarnYccClipNum < YCC_CLIP_REPORT_MAX)) {
				CString tmpStr;
				tmpStr.Format(_T("*** NOTE: YCC Clipped. MCU=(%4u,%4u) YCC=(%5d,%5d,%5d) Cb Underflow @ Offset %s"),
					nMcuX,nMcuY,cur_y,cur_cb,cur_cr,GetScanBufPos());
				m_pLog->AddLineWarn(tmpStr);
				m_nWarnYccClipNum++;
				if (m_nWarnYccClipNum == YCC_CLIP_REPORT_MAX) {
					tmpStr.Format(_T("    Only reported first %u instances of this message..."),YCC_CLIP_REPORT_MAX);
					m_pLog->AddLineWarn(tmpStr);
				}
			}
			rPix.clip |= CC_CLIP_CB_UNDER;
			cur_cb = CC_CLIP_YCC_MIN;
		}
		if (cur_cr > CC_CLIP_YCC_MAX) {
			if (YCC_CLIP_REPORT_ERR && (m_nWarnYccClipNum < YCC_CLIP_REPORT_MAX)) {
				CString tmpStr;
				tmpStr.Format(_T("*** NOTE: YCC Clipped. MCU=(%4u,%4u) YCC=(%5d,%5d,%5d) Cr Overflow @ Offset %s"),
					nMcuX,nMcuY,cur_y,cur_cb,cur_cr,GetScanBufPos());
				m_pLog->AddLineWarn(tmpStr);
				m_nWarnYccClipNum++;
				if (m_nWarnYccClipNum == YCC_CLIP_REPORT_MAX) {
					tmpStr.Format(_T("    Only reported first %u instances of this message..."),YCC_CLIP_REPORT_MAX);
					m_pLog->AddLineWarn(tmpStr);
				}
			}
			rPix.clip |= CC_CLIP_CR_OVER;
			cur_cr = CC_CLIP_YCC_MAX;
		}
		if (cur_cr < CC_CLIP_YCC_MIN) {
			if (YCC_CLIP_REPORT_ERR && (m_nWarnYccClipNum < YCC_CLIP_REPORT_MAX)) {
				CString tmpStr;
				tmpStr.Format(_T("*** NOTE: YCC Clipped. MCU=(%4u,%4u) YCC=(%5d,%5d,%5d) Cr Underflow @ Offset %s"),
					nMcuX,nMcuY,cur_y,cur_cb,cur_cr,GetScanBufPos());
				m_pLog->AddLineWarn(tmpStr);
				m_nWarnYccClipNum++;
				if (m_nWarnYccClipNum == YCC_CLIP_REPORT_MAX) {
					tmpStr.Format(_T("    Only reported first %u instances of this message..."),YCC_CLIP_REPORT_MAX);
					m_pLog->AddLineWarn(tmpStr);
				}
			}
			rPix.clip |= CC_CLIP_CR_UNDER;
			cur_cr = CC_CLIP_YCC_MIN;
		}
	} // YCC clip enabled?

	// Perform color conversion: YCC->RGB
	// The cur_y/cb/cr values should already be clipped to BYTE size
	rPix.y  = cur_y;
	rPix.cb = cur_cb;
	rPix.cr = cur_cr;

}



// Input RGB triplet in floats
// Expect range to be 0..255
// Return RGB triplet in bytes
// Report if it is out of range
// CC: r/g/b_preclip -> r/g/b
void CimgDecode::CapRgbRange(unsigned nMcuX,unsigned nMcuY,PixelCc &rPix)
{
	int r_lim,g_lim,b_lim;

	// Truncate
	r_lim = (int)(rPix.r_preclip);
	g_lim = (int)(rPix.g_preclip);
	b_lim = (int)(rPix.b_preclip);

	if (m_bHistEn) {
		m_sHisto.r_pre_min = (r_lim<m_sHisto.r_pre_min)?r_lim:m_sHisto.r_pre_min;
		m_sHisto.r_pre_max = (r_lim>m_sHisto.r_pre_max)?r_lim:m_sHisto.r_pre_max;
		m_sHisto.r_pre_sum += r_lim;
		m_sHisto.g_pre_min = (g_lim<m_sHisto.g_pre_min)?g_lim:m_sHisto.g_pre_min;
		m_sHisto.g_pre_max = (g_lim>m_sHisto.g_pre_max)?g_lim:m_sHisto.g_pre_max;
		m_sHisto.g_pre_sum += g_lim;
		m_sHisto.b_pre_min = (b_lim<m_sHisto.b_pre_min)?b_lim:m_sHisto.b_pre_min;
		m_sHisto.b_pre_max = (b_lim>m_sHisto.b_pre_max)?b_lim:m_sHisto.b_pre_max;
		m_sHisto.b_pre_sum += b_lim;
	}

	if (r_lim < 0) {
		if (m_bVerbose) {
			CString tmpStr;
			tmpStr.Format(_T("  YCC->RGB Clipped. MCU=(%4u,%4u) RGB=(%5d,%5d,%5d) Red Underflow"),
				nMcuX,nMcuY,r_lim,g_lim,b_lim);
			m_pLog->AddLineWarn(tmpStr);
		}
		rPix.clip |= CC_CLIP_R_UNDER;
		r_lim = 0;
	}
	if (g_lim < 0) {
		if (m_bVerbose) {
			CString tmpStr;
			tmpStr.Format(_T("  YCC->RGB Clipped. MCU=(%4u,%4u) RGB=(%5d,%5d,%5d) Green Underflow"),
				nMcuX,nMcuY,r_lim,g_lim,b_lim);
			m_pLog->AddLineWarn(tmpStr);
		}
		rPix.clip |= CC_CLIP_G_UNDER;
		g_lim = 0;
	}
	if (b_lim < 0) {
		if (m_bVerbose) {
			CString tmpStr;
			tmpStr.Format(_T("  YCC->RGB Clipped. MCU=(%4u,%4u) RGB=(%5d,%5d,%5d) Blue Underflow"),
				nMcuX,nMcuY,r_lim,g_lim,b_lim);
			m_pLog->AddLineWarn(tmpStr);
		}
		rPix.clip |= CC_CLIP_B_UNDER;
		b_lim = 0;
	}
	if (r_lim > 255) {
		if (m_bVerbose) {
			CString tmpStr;
			tmpStr.Format(_T("  YCC->RGB Clipped. MCU=(%4u,%4u) RGB=(%5d,%5d,%5d) Red Overflow"),
				nMcuX,nMcuY,r_lim,g_lim,b_lim);
			m_pLog->AddLineWarn(tmpStr);
		}
		rPix.clip |= CC_CLIP_R_OVER;
		r_lim = 255;
	}
	if (g_lim > 255) {
		if (m_bVerbose) {
			CString tmpStr;
			tmpStr.Format(_T("  YCC->RGB Clipped. MCU=(%4u,%4u) RGB=(%5d,%5d,%5d) Green Overflow"),
				nMcuX,nMcuY,r_lim,g_lim,b_lim);
			m_pLog->AddLineWarn(tmpStr);
		}
		rPix.clip |= CC_CLIP_G_OVER;
		g_lim = 255;
	}
	if (b_lim > 255) {
		if (m_bVerbose) {
			CString tmpStr;
			tmpStr.Format(_T("  YCC->RGB Clipped. MCU=(%4u,%4u) RGB=(%5d,%5d,%5d) Blue Overflow"),
				nMcuX,nMcuY,r_lim,g_lim,b_lim);
			m_pLog->AddLineWarn(tmpStr);
		}
		rPix.clip |= CC_CLIP_B_OVER;
		b_lim = 255;
	}

	if (m_bHistEn) {
		m_sHisto.r_min = (r_lim<m_sHisto.r_min)?r_lim:m_sHisto.r_min;
		m_sHisto.r_max = (r_lim>m_sHisto.r_max)?r_lim:m_sHisto.r_max;
		m_sHisto.r_sum += r_lim;
		m_sHisto.g_min = (g_lim<m_sHisto.g_min)?g_lim:m_sHisto.g_min;
		m_sHisto.g_max = (g_lim>m_sHisto.g_max)?g_lim:m_sHisto.g_max;
		m_sHisto.g_sum += g_lim;
		m_sHisto.b_min = (b_lim<m_sHisto.b_min)?b_lim:m_sHisto.b_min;
		m_sHisto.b_max = (b_lim>m_sHisto.b_max)?b_lim:m_sHisto.b_max;
		m_sHisto.b_sum += b_lim;
	}


	// Now convert to BYTE
	rPix.r = (byte)r_lim;
	rPix.g = (byte)g_lim;
	rPix.b = (byte)b_lim;

}


// This method recalcs the full image based on the original YCC pixmap.
// PRE: Assumes that pointer checks have been made
//      pRectView is range of real image that is visible (cropped to image size)

// Also locate the brightest pixel. Note that we cannot do the brightest pixel
// search when we called SetFullRes() because we need to have access to all
// of the channel components at once to do this.

void CimgDecode::CalcChannelPreviewFull(CRect* pRectView,unsigned char* pTmp)
{
	PixelCc		src_pix,dst_pix;
	CString		tmpStr;

	unsigned	row_bytes;
	row_bytes = m_nImgSizeX * sizeof(RGBQUAD);


	// ---------------------------------------------------------
	// Now do color conversion process

	unsigned	nPixMapW = m_nBlkXMax*8;
	unsigned	nPixmapInd;

	unsigned		nRngX1,nRngX2,nRngY1,nRngY2;
	unsigned		nSumY;
	unsigned long	nNumPixels = 0;

/*
	// FIXME: Later on, need to be provided with proper rect!
	unsigned	nRngX1 = pRectView->left;
	unsigned	nRngX2 = pRectView->right;
	unsigned	nRngY1 = pRectView->top;
	unsigned	nRngY2 = pRectView->bottom;
	// These co-ords will define range in nPixX,nPixY that get drawn
	// This will help YCC Adjust display react much faster. Only recalc
	// visible portion of image, but then recalc entire one once Adjust
	// dialog is closed.

	// FIXME:
	// Note, if we were to make these ranges a subset of the
	// full image dimensions, then we'd have to determine the best
	// way to handle the brightest pixel search & average luminance logic
	// since those appear in the nRngX/Y loops. 
*/	
	nRngX1 = 0;
	nRngX2 = m_nImgSizeX;
	nRngY1 = 0;
	nRngY2 = m_nImgSizeY;



	// Brightest pixel values were already reset during Reset() call, but for
	// safety, do it again here.
	m_bBrightValid = false;
	m_nBrightY  = -32768;
	m_nBrightCb = -32768;
	m_nBrightCr = -32768;

	// Average luminance calculation
	m_bAvgYValid = false;
	m_nAvgY = 0;
	nSumY = 0;

	// For IDCT RGB Printout:
	bool		bRowStart = false;
	CString		lineStr;


	unsigned	nMcuShiftInd = m_nPreviewShiftMcuY * (m_nImgSizeX/m_nMcuWidth) + m_nPreviewShiftMcuX;

	SetStatusText("Color conversion...");

	if (m_bDetailVlc) {
		m_pLog->AddLine("  Detailed IDCT Dump (RGB):");
		tmpStr.Format("    MCU [%3u,%3u]:",m_nDetailVlcX,m_nDetailVlcY);
		m_pLog->AddLine(tmpStr);
	}

	// Determine pixel count
	nNumPixels = (nRngY2-nRngY1+1) * (nRngX2-nRngX1+1);

	for (unsigned nPixY=nRngY1;nPixY<nRngY2;nPixY++) {

		unsigned nMcuY = nPixY/m_nMcuHeight;
		// DIBs appear to be stored up-side down, so correct Y
		unsigned coord_y_inv = (m_nImgSizeY-1) - nPixY;

		for (unsigned nPixX=nRngX1;nPixX<nRngX2;nPixX++) {

			nPixmapInd = nPixY*nPixMapW + nPixX;
			unsigned pix_byte = nPixX*4+0+coord_y_inv*row_bytes;

			unsigned	nMcuX = nPixX/m_nMcuWidth;
			unsigned	nMcuInd = nMcuY * (m_nImgSizeX/m_nMcuWidth) + nMcuX;

			int nTmpY  = m_pPixValY[nPixmapInd];
			int nTmpCb = m_pPixValCb[nPixmapInd];
			int nTmpCr = m_pPixValCr[nPixmapInd];

			// Load the value
			src_pix.y_prerange  = nTmpY;
			src_pix.cb_prerange = nTmpCb;
			src_pix.cr_prerange = nTmpCr;

			// Update brightest pixel search here
			int nBrightness = nTmpY;
			if (nBrightness > m_nBrightY) {
				m_nBrightY  = nTmpY;
				m_nBrightCb = nTmpCb;
				m_nBrightCr = nTmpCr;
				m_ptBrightMcu.x = nMcuX;
				m_ptBrightMcu.y = nMcuY;
			}

			// FIXME
			// Could speed this up by adding boolean check to see if we are
			// truly needing to do any shifting!
			if (nMcuInd >= nMcuShiftInd) {
				src_pix.y_prerange  += m_nPreviewShiftY;
				src_pix.cb_prerange += m_nPreviewShiftCb;
				src_pix.cr_prerange += m_nPreviewShiftCr;
			}

			// Color convert
			if (m_bHistEn || m_bStatClipEn) {
				ConvertYCCtoRGB(nMcuX,nMcuY,src_pix);
			} else {
				ConvertYCCtoRGBFastFloat(src_pix);
				//ConvertYCCtoRGBFastFixed(src_pix);
			}

			// Accumulate the luminance value for this pixel
			// after we have converted it to the range 0..255
			nSumY += src_pix.y;


			// If we want a detailed decode of RGB, print it out
			// now if we are on the correct MCU.
			// NOTE: The level shift (adjust) will affect this report!
			if (m_bDetailVlc) {
				if (nMcuY == m_nDetailVlcY) {

					if (nMcuX == m_nDetailVlcX) {
					//if ((nMcuX >= m_nDetailVlcX) && (nMcuX < m_nDetailVlcX+m_nDetailVlcLen)) {
						if (!bRowStart) {
							bRowStart = true;
							lineStr.Format("      [ ");
						}
						tmpStr.Format("x%02X%02X%02X ",src_pix.r,src_pix.g,src_pix.b);
						lineStr.Append(tmpStr);
					} else {
						if (bRowStart) {
							// We had started a row, but we are now out of range, so we
							// need to close up!
							tmpStr.Format(" ]");
							lineStr.Append(tmpStr);
							m_pLog->AddLine(lineStr);
							bRowStart = false;
						}
					}
				}
				
			}

			ChannelExtract(m_nPreviewMode,src_pix,dst_pix);

			pTmp[pix_byte+3] = 0;
			pTmp[pix_byte+2] = dst_pix.r;
			pTmp[pix_byte+1] = dst_pix.g;
			pTmp[pix_byte+0] = dst_pix.b;

		} // x
	} // y
	SetStatusText("");
	// ---------------------------------------------------------

	if (m_bDetailVlc) {
		m_pLog->AddLine("");
	}


	// Assume that brightest pixel search was successful
	// Now compute the RGB value for this pixel!
	m_bBrightValid = true;
	src_pix.y_prerange = m_nBrightY;
	src_pix.cb_prerange = m_nBrightCb;
	src_pix.cr_prerange = m_nBrightCr;
	ConvertYCCtoRGBFastFloat(src_pix);
	m_nBrightR = src_pix.r;
	m_nBrightG = src_pix.g;
	m_nBrightB = src_pix.b;

	// Now perform average luminance calculation
	// NOTE: This will result in a value in the range 0..255
	ASSERT(nNumPixels > 0);
	m_nAvgY = nSumY / nNumPixels;
	m_bAvgYValid = true;

	// Now that we've recalculated the tmp image,
	// set the flag to mark the new result
//	m_nPreviewModeDone = m_nPreviewMode;

}





void CimgDecode::ChannelExtract(unsigned nMode,PixelCc &sSrc,PixelCc &sDst)
{
	if (nMode == PREVIEW_RGB) {
		sDst.r = sSrc.r;
		sDst.g = sSrc.g;
		sDst.b = sSrc.b;
	} else if (nMode == PREVIEW_YCC) {
		sDst.r = sSrc.cr;
		sDst.g = sSrc.y;
		sDst.b = sSrc.cb;
	} else if (nMode == PREVIEW_R) {
		sDst.r = sSrc.r;
		sDst.g = sSrc.r;
		sDst.b = sSrc.r;
	} else if (nMode == PREVIEW_G) {
		sDst.r = sSrc.g;
		sDst.g = sSrc.g;
		sDst.b = sSrc.g;
	} else if (nMode == PREVIEW_B) {
		sDst.r = sSrc.b;
		sDst.g = sSrc.b;
		sDst.b = sSrc.b;
	} else if (nMode == PREVIEW_Y) {
		sDst.r = sSrc.y;
		sDst.g = sSrc.y;
		sDst.b = sSrc.y;
	} else if (nMode == PREVIEW_CB) {
		sDst.r = sSrc.cb;
		sDst.g = sSrc.cb;
		sDst.b = sSrc.cb;
	} else if (nMode == PREVIEW_CR) {
		sDst.r = sSrc.cr;
		sDst.g = sSrc.cr;
		sDst.b = sSrc.cr;
	} else {
		sDst.r = sSrc.r;
		sDst.g = sSrc.g;
		sDst.b = sSrc.b;
	}

}

unsigned char * CimgDecode::GetBitmapPtr()
{
	unsigned char *		p_dib_imgtmp_bits = NULL;

	p_dib_imgtmp_bits = (unsigned char*) ( my_DIBtmp.GetDIBBitArray() );

	// Ensure that the pointers are available!
	if ( !p_dib_imgtmp_bits ) {
		return NULL;
	} else {
		return p_dib_imgtmp_bits;
	}
}

// Calculate my_DIBtmp bits for unusual channel modes
void CimgDecode::CalcChannelPreview()
{
	unsigned char *		p_dib_imgtmp_bits = NULL;

	p_dib_imgtmp_bits = (unsigned char*) ( my_DIBtmp.GetDIBBitArray() );

	// Ensure that the pointers are available!
	if ( !p_dib_imgtmp_bits ) {
		return;
	}



	// If we need to do a YCC shift, then do full recalc into tmp array
	CalcChannelPreviewFull(NULL,p_dib_imgtmp_bits);

	// Since this was a complex mod, we don't mark this channel as
	// being "done", so we will need to recalculate any time we change
	// the channel display.

	// Force an update of the view to be sure
	//m_pDoc->UpdateAllViews(NULL);

	return;

}


void CimgDecode::LookupFilePosPix(unsigned pix_x,unsigned pix_y, unsigned &nByte, unsigned &nBit)
{
	unsigned mcu_x,mcu_y;
	unsigned nPacked;
	mcu_x = pix_x / m_nMcuWidth;
	mcu_y = pix_y / m_nMcuHeight;
	nPacked = m_pMcuFileMap[mcu_x + mcu_y*m_nMcuXMax];
	UnpackFileOffset(nPacked,nByte,nBit);
}

void CimgDecode::LookupFilePosMcu(unsigned mcu_x,unsigned mcu_y, unsigned &nByte, unsigned &nBit)
{
	unsigned nPacked;
	nPacked = m_pMcuFileMap[mcu_x + mcu_y*m_nMcuXMax];
	UnpackFileOffset(nPacked,nByte,nBit);
}

// Lookup 8x8 pixel block
void CimgDecode::LookupFilePosBlk(unsigned blk_x,unsigned blk_y, unsigned &nByte, unsigned &nBit)
{
	unsigned nPacked;
	unsigned mcu_x,mcu_y;
	mcu_x = blk_x / m_nCssX;
	mcu_y = blk_y / m_nCssY;
	nPacked = m_pMcuFileMap[mcu_x + mcu_y*m_nMcuXMax];
	UnpackFileOffset(nPacked,nByte,nBit);
}

void CimgDecode::LookupBlkYCC(unsigned blk_x,unsigned blk_y,int &nY,int &nCb,int &nCr)
{
	nY  = m_pBlkValY [blk_x + blk_y*m_nBlkXMax];
	nCb = m_pBlkValCb[blk_x + blk_y*m_nBlkXMax];
	nCr = m_pBlkValCr[blk_x + blk_y*m_nBlkXMax];
}

CPoint CimgDecode::PixelToMcu(CPoint ptPix)
{
	CPoint ptMcu;
	ptMcu.x = ptPix.x / m_nMcuWidth;
	ptMcu.y = ptPix.y / m_nMcuHeight;
	return ptMcu;
}

CPoint CimgDecode::PixelToBlk(CPoint ptPix)
{
	CPoint ptBlk;
	ptBlk.x = ptPix.x / 8;
	ptBlk.y = ptPix.y / 8;
	return ptBlk;
}

unsigned CimgDecode::PackFileOffset(unsigned nByte,unsigned nBit)
{
	unsigned tmp;
	// Note that we only really need 3 bits, but I'll keep 4
	// so that the file offset is human readable. We will only
	// handle files up to 2^28 bytes (256MB), so this is probably
	// fine!
	tmp = (nByte << 4) + nBit;
	return tmp;
}

void CimgDecode::UnpackFileOffset(unsigned nPacked, unsigned &nByte, unsigned &nBit)
{
	nBit  = nPacked & 0x7;
	nByte = nPacked >> 4;
}

void CimgDecode::SetMarker(unsigned blk_x,unsigned blk_y)
{
	m_bMarkBlock = true;
	m_nMarkBlockX = blk_x;
	m_nMarkBlockY = blk_y;
}


void CimgDecode::SetMarkerMcu(unsigned nMcuX,unsigned nMcuY,unsigned nState)
{
	if (m_abMarkedMcuMap) {
		unsigned nXYCur = nMcuY*m_nMcuXMax + nMcuX;
		bool	bCur = m_abMarkedMcuMap[nXYCur];
		if (nState == 0) {
			bCur = false;
		} else if (nState == 1) {
			bCur = true;
		} else if (nState == 2) {
			bCur = !bCur;
		}
		m_abMarkedMcuMap[nXYCur] = bCur;

		m_nMarkedMcuLastXY = nXYCur;
	}

}

// Shift key was held down so set a run
void CimgDecode::SetMarkerMcuTo(unsigned nMcuX,unsigned nMcuY,unsigned nState)
{
	if (m_abMarkedMcuMap) {
		unsigned nXYCur = nMcuY*m_nMcuXMax + nMcuX;
		bool	bCur = m_abMarkedMcuMap[nXYCur];
		if (nState == 0) {
			bCur = false;
		} else if (nState == 1) {
			bCur = true;
		} else if (nState == 2) {
			bCur = !bCur;
		}

		// Now set the run
		for (unsigned nInd = m_nMarkedMcuLastXY;nInd<=nXYCur;nInd++) {
			m_abMarkedMcuMap[nInd] = bCur;
		}

		// Don't update "last" position, because we may still want to change
		// run from previous start position.
	}

}


void CimgDecode::SetMarkerBlk(unsigned nBlkX,unsigned nBlkY)
{
	if (m_nMarkersBlkNum == 10) {
		// Shift them down by 1. Last entry will be deleted next
		for (unsigned ind=1;ind<10;ind++) {
			m_ptMarkersBlk[ind-1] = m_ptMarkersBlk[ind];
		}
		// Reflect new reduced count
		m_nMarkersBlkNum--;
	}

	CString	tmpStr;
	int	nY,nCb,nCr;
	unsigned nMcuX,nMcuY;
	nMcuX = nBlkX / m_nCssX;
	nMcuY = nBlkY / m_nCssY;
	LookupBlkYCC(nBlkX,nBlkY,nY,nCb,nCr);
	tmpStr.Format("Position Marked @ MCU=[%4u,%4u](%u,%u) YCC=[%5d,%5d,%5d]",
		nMcuX,nMcuY,nBlkX-(nMcuX*m_nCssX),nBlkY-(nMcuY*m_nCssY),nY,nCb,nCr);
	m_pLog->AddLine(tmpStr);
	m_pLog->AddLine("");

	m_ptMarkersBlk[m_nMarkersBlkNum].x = nBlkX;
	m_ptMarkersBlk[m_nMarkersBlkNum].y = nBlkY;
	m_nMarkersBlkNum++;
}



unsigned CimgDecode::GetPreviewZoom()
{
	return m_nZoomMode;
}

unsigned CimgDecode::GetPreviewMode()
{
	return m_nPreviewMode;
}

void CimgDecode::SetPreviewZoom(bool inc,bool dec,bool set,unsigned val)
{
	float m_nZoomOld = m_nZoom;
	if (inc) {
		if (m_nZoomMode+1 < PRV_ZOOMEND) {
			m_nZoomMode++;

		}
	} else if (dec) {
		if (m_nZoomMode-1 > PRV_ZOOMBEGIN) {
			m_nZoomMode--;
		}
	} else if (set) {
		m_nZoomMode = val;
	}
	switch(m_nZoomMode) {
		case PRV_ZOOM_12:  m_nZoom = 0.125; break;
		case PRV_ZOOM_25:  m_nZoom = 0.25; break;
		case PRV_ZOOM_50:  m_nZoom = 0.5; break;
		case PRV_ZOOM_100: m_nZoom = 1.0; break;
		case PRV_ZOOM_150: m_nZoom = 1.5; break;
		case PRV_ZOOM_200: m_nZoom = 2.0; break;
		case PRV_ZOOM_300: m_nZoom = 3.0; break;
		case PRV_ZOOM_400: m_nZoom = 4.0; break;
		case PRV_ZOOM_800: m_nZoom = 8.0; break;
		default:           m_nZoom = 1.0; break;
	}

}

void CimgDecode::ViewOnDraw(CDC* pDC,CRect rectClient,CPoint ScrolledPos, 
							CFont* pFont, CSize &szNewScrollSize)
{

	unsigned	nBorderLeft		= 10;
	unsigned	nBorderBottom	= 10;
	unsigned	nTitleHeight	= 20;
	unsigned	nTitleIndent    = 5;
	unsigned	nTitleLowGap	= 3;

	m_nPageWidth = 600;
	m_nPageHeight = 10;	// Start with some margin


	CString tmpStr;
	CString strRender;
	int		nHeight;


	//CSize	ScrolledSize;
	//CRect	ScrollRect;

	unsigned nYPosImgTitle,nYPosImg;
	unsigned nYPosHistTitle,nYPosHist;
	unsigned nYPosHistYTitle,nYPosHistY;	// Y position of Img & Histogram

	CRect rectTmp;

	// If we have displayed an image, make sure to allow for
	// the additional space!

	bool bImgDrawn = false;

	CBrush	brGray(    RGB(128, 128, 128));
	CBrush	brGrayLt1( RGB(210, 210, 210));
	CBrush	brGrayLt2( RGB(240, 240, 240));
	CBrush	brBlueLt(  RGB(240, 240, 255));
	CPen	penRed(PS_DOT,1,RGB(255,0,0));


	if (my_DIB_ready) {

		nYPosImgTitle = m_nPageHeight;
		m_nPageHeight += nTitleHeight;	// Margin at top for title

		m_rectImgReal.SetRect(0,0,
			(int)(m_rectImgBase.right*m_nZoom),
			(int)(m_rectImgBase.bottom*m_nZoom) );

		nYPosImg = m_nPageHeight;
		m_nPageHeight += m_rectImgReal.Height();
		m_nPageHeight += nBorderBottom;	// Margin at bottom
		bImgDrawn = true;

		m_rectImgReal.OffsetRect(nBorderLeft,nYPosImg);

		// Now create the shadow of the main image
		rectTmp = m_rectImgReal;
		rectTmp.OffsetRect(4,4);
		pDC->FillRect(rectTmp,&brGrayLt1);
		rectTmp.InflateRect(1,1,1,1);
		pDC->FrameRect(rectTmp,&brGrayLt2);

	}

	if (m_bHistEn) {

		if (my_hist_rgb_DIB_ready) {

			nYPosHistTitle = m_nPageHeight;
			m_nPageHeight += nTitleHeight;	// Margin at top for title

			m_rectHistReal = m_rectHistBase;
	        
			nYPosHist = m_nPageHeight;
			m_nPageHeight += m_rectHistReal.Height();
			m_nPageHeight += nBorderBottom;	// Margin at bottom
			bImgDrawn = true;

			m_rectHistReal.OffsetRect(nBorderLeft,nYPosHist);

			// Create the border
			rectTmp = m_rectHistReal;
			rectTmp.InflateRect(1,1,1,1);
			pDC->FrameRect(rectTmp,&brGray);
		}

		if (my_hist_y_DIB_ready) {

			nYPosHistYTitle = m_nPageHeight;
			m_nPageHeight += nTitleHeight;	// Margin at top for title

			m_rectHistYReal = m_rectHistYBase;

			nYPosHistY = m_nPageHeight;
			m_nPageHeight += m_rectHistYReal.Height();
			m_nPageHeight += nBorderBottom;	// Margin at bottom
			bImgDrawn = true;

			m_rectHistYReal.OffsetRect(nBorderLeft,nYPosHistY);

			// Create the border
			rectTmp = m_rectHistYReal;
			rectTmp.InflateRect(1,1,1,1);
			pDC->FrameRect(rectTmp,&brGray);


		}

	}


	// Find a starting line based on scrolling
	// and current client size


	CRect rectClientScrolled = rectClient;
	rectClientScrolled.OffsetRect(ScrolledPos);

	// Change the font
	CFont*	pOldFont;
	pOldFont = pDC->SelectObject(pFont);

	CString titleStr;

	// Draw the bitmap if ready
	if (my_DIB_ready) {

		// Print label
		titleStr = "Image (";
		switch (m_nPreviewMode) {
			case PREVIEW_RGB:	titleStr += "RGB"; break;
			case PREVIEW_YCC:	titleStr += "YCC"; break;
			case PREVIEW_R:		titleStr += "R"; break;
			case PREVIEW_G:		titleStr += "G"; break;
			case PREVIEW_B:		titleStr += "B"; break;
			case PREVIEW_Y:		titleStr += "Y"; break;
			case PREVIEW_CB:	titleStr += "Cb"; break;
			case PREVIEW_CR:	titleStr += "Cr"; break;
			default:			titleStr += "???"; break;
		}
		if (m_bDecodeScanAc) {
			titleStr += ", DC+AC)";
		} else {
			titleStr += ", DC)";
		}

		
		switch (m_nZoomMode) {
			//case PRV_ZOOM_12P5: titleStr += " @ 12.5%"; break;
			case PRV_ZOOM_12: titleStr += " @ 12.5% (1/8)"; break;
			case PRV_ZOOM_25: titleStr += " @ 25% (1/4)"; break;
			case PRV_ZOOM_50: titleStr += " @ 50% (1/2)"; break;
			case PRV_ZOOM_100: titleStr += " @ 100% (1:1)"; break;
			case PRV_ZOOM_150: titleStr += " @ 150% (3:2)"; break;
			case PRV_ZOOM_200: titleStr += " @ 200% (2:1)"; break;
			case PRV_ZOOM_300: titleStr += " @ 300% (3:1)"; break;
			case PRV_ZOOM_400: titleStr += " @ 400% (4:1)"; break;
			case PRV_ZOOM_800: titleStr += " @ 800% (8:1)"; break;
			default: titleStr += ""; break;
		}
		

		// Calculate the title width
		CRect rectCalc = CRect(0,0,0,0);
		nHeight = pDC->DrawText(titleStr,-1,&rectCalc,
			DT_CALCRECT | DT_TOP | DT_NOPREFIX | DT_SINGLELINE);
		int nWidth = rectCalc.Width() + 2*nTitleIndent;

		// Determine the title area (could be larger than the image
		// if the image zoom is < 100% or sized small)

		// Draw title background
		rectTmp = CRect(m_rectImgReal.left,nYPosImgTitle,
			max(m_rectImgReal.right,m_rectImgReal.left+nWidth),m_rectImgReal.top-nTitleLowGap);
		pDC->FillRect(rectTmp,&brBlueLt);

		// Draw the title
		int	nBkMode = pDC->GetBkMode();
		pDC->SetBkMode(TRANSPARENT);
		rectTmp.OffsetRect(nTitleIndent,0);
		nHeight = pDC->DrawText(titleStr,-1,&rectTmp,
			DT_TOP | DT_NOPREFIX | DT_SINGLELINE);
		pDC->SetBkMode(nBkMode);

		// Draw image

		// Assume that the temp image has already been generated!
		// For speed purposes, we use my_DIBtmp only when we are
		// in a mode other than RGB or YCC. In the RGB/YCC modes,
		// we skip the CalcChannelPreview() step.

		// FIXME: Later on, to prevent slow redraw, I should only be redrawing the
		// current visible window! A different CopyDIB routine should be made
		// that only copies a subset of the image. Unfortunately, the zoom should
		// be accounted for as well.

		// Should probably find intersection between rectClient and
		// m_rectImgReal. Only ask to draw this subset!

		/*
		if (m_nPreviewMode == PREVIEW_RGB) {
			my_DIBrgb.CopyDIB(pDC,m_rectImgReal.left,m_rectImgReal.top,m_nZoom);

			//my_DIBrgb.CopyDIBsmall(pDC,m_rectImgReal.left,m_rectImgReal.top,m_nZoom);
			//my_DIBrgb.CopyDibPart(pDC,m_rectImgReal,&rectClientScrolled,m_nZoom);

			//my_DIBrgb.CopyDibDblBuf(pDC,m_rectImgReal.left,m_rectImgReal.top,&rectClientScrolled,m_nZoom);
		} else if (m_nPreviewMode == PREVIEW_YCC) {
			my_DIBycc.CopyDIB(pDC,m_rectImgReal.left,m_rectImgReal.top,m_nZoom);
		} else {
			my_DIBtmp.CopyDIB(pDC,m_rectImgReal.left,m_rectImgReal.top,m_nZoom);
		}
		*/

		// Just use my_DIBtmp instead of swapping between DIBtmp, DIBycc and DIBrgb.
		// This way we can also have more flexibility in modifying RGB & YCC displays.
		// Time calling CalcChannelPreview() seems small, so no real impact.
		my_DIBtmp.CopyDIB(pDC,m_rectImgReal.left,m_rectImgReal.top,m_nZoom);


		// Now create overlays
		CPen* pPen = pDC->SelectObject(&penRed);

		/*
		// Draw boundary of partial MCU edge
		pDC->MoveTo(m_rectImgReal.left+m_nImgSizeXPartMcu*m_nZoom,
			m_rectImgReal.top);
		pDC->LineTo(m_rectImgReal.left+m_nImgSizeXPartMcu*m_nZoom,
			m_rectImgReal.top+m_nImgSizeYPartMcu*m_nZoom);

		pDC->MoveTo(m_rectImgReal.left,
			m_rectImgReal.top+m_nImgSizeYPartMcu*m_nZoom);
		pDC->LineTo(m_rectImgReal.left+m_nImgSizeXPartMcu*m_nZoom,
			m_rectImgReal.top+m_nImgSizeYPartMcu*m_nZoom);
		*/


		// Draw boundary for end of valid data (inside partial MCU)
		int nXZoomed = (int)(m_nDimX*m_nZoom);
		int nYZoomed = (int)(m_nDimY*m_nZoom);

		pDC->MoveTo(m_rectImgReal.left+nXZoomed,m_rectImgReal.top);
		pDC->LineTo(m_rectImgReal.left+nXZoomed,m_rectImgReal.top+nYZoomed);

		pDC->MoveTo(m_rectImgReal.left,m_rectImgReal.top+nYZoomed);
		pDC->LineTo(m_rectImgReal.left+nXZoomed,m_rectImgReal.top+nYZoomed);


		pDC->SelectObject(pPen);

		// Before we frame the region, let's draw any remaining overlays

		// Only draw the MCU overlay if zoom is > 100%, otherwise we will have
		// replaced entire image with the boundary lines!
		if ( m_bViewOverlaysMcuGrid && (m_nZoomMode >= PRV_ZOOM_25)) {
			ViewMcuOverlay(pDC);
		}

		// Always draw the markers
		ViewMcuMarkedOverlay(pDC);


		// Final frame border
		rectTmp = m_rectImgReal;
		rectTmp.InflateRect(1,1,1,1);
		pDC->FrameRect(rectTmp,&brGray);


		// Preserve the image placement (for other functions, such as click detect)
		m_nPreviewPosX = m_rectImgReal.left;
		m_nPreviewPosY = m_rectImgReal.top;
		m_nPreviewSizeX = m_rectImgReal.Width();
		m_nPreviewSizeY = m_rectImgReal.Height();

		ASSERT(m_nPageWidth>=0);
		m_nPageWidth = max((unsigned)m_nPageWidth, m_nPreviewPosX + m_nPreviewSizeX);


	}

	if (m_bHistEn) {

		if (my_hist_rgb_DIB_ready) {

			// Draw title background
			rectTmp = CRect(m_rectHistReal.left,nYPosHistTitle,
				m_rectHistReal.right,m_rectHistReal.top-nTitleLowGap);
			pDC->FillRect(rectTmp,&brBlueLt);

			// Draw the title
			int	nBkMode = pDC->GetBkMode();
			pDC->SetBkMode(TRANSPARENT);
			rectTmp.OffsetRect(nTitleIndent,0);
			nHeight = pDC->DrawText(_T("Histogram (RGB)"),-1,&rectTmp,
				DT_TOP | DT_NOPREFIX | DT_SINGLELINE);
			pDC->SetBkMode(nBkMode);

			// Draw image
			my_hist_rgb_DIB.CopyDIB(pDC,m_rectHistReal.left,m_rectHistReal.top);
		}

		if (my_hist_y_DIB_ready) {

			// Draw title background
			rectTmp = CRect(m_rectHistYReal.left,nYPosHistYTitle,
				m_rectHistYReal.right,m_rectHistYReal.top-nTitleLowGap);
			pDC->FillRect(rectTmp,&brBlueLt);

			// Draw the title
			int	nBkMode = pDC->GetBkMode();
			pDC->SetBkMode(TRANSPARENT);
			rectTmp.OffsetRect(nTitleIndent,0);
			nHeight = pDC->DrawText(_T("Histogram (Y)"),-1,&rectTmp,
				DT_TOP | DT_NOPREFIX | DT_SINGLELINE);
			pDC->SetBkMode(nBkMode);

			// Draw image
			my_hist_y_DIB.CopyDIB(pDC,m_rectHistYReal.left,m_rectHistYReal.top);
			//my_hist_y_DIB.CopyDIB(pDC,nBorderLeft,nYPosHistY);
		}
	}

	// If no image has been drawn, indicate to user why!
	if (!bImgDrawn) {
		//ScrollRect.top = m_nPageHeight;	//CAL!??

		// Print label
		//nHeight = pDC->DrawText(_T("Image Decode disabled. Enable with [Options->Decode Scan Image]"), -1,&ScrollRect,
		//	DT_TOP | DT_NOPREFIX | DT_SINGLELINE);

	}

	// Restore the original font
	pDC->SelectObject(pOldFont);

	// Set scroll bars accordingly. We use the page dimensions here.
	CSize sizeTotal(m_nPageWidth+nBorderLeft,m_nPageHeight);

	szNewScrollSize = sizeTotal;
}

void CimgDecode::ViewMcuOverlay(CDC* pDC)
{
	// Now create overlays
	unsigned nXZoomed,nYZoomed;

//	CPen	penDot(PS_DOT,1,RGB(0,200,0));
	CPen	penDot(PS_DOT,1,RGB(32,32,32));

	int			nBkModeOld = pDC->GetBkMode();
	CPen*		pPenOld = pDC->SelectObject(&penDot);
	pDC->SetBkMode(TRANSPARENT);

	// Draw vertical lines
	for (unsigned nMcuX=0;nMcuX<m_nMcuXMax;nMcuX++) {
		nXZoomed = (int)(nMcuX*m_nMcuWidth*m_nZoom);
		pDC->MoveTo(m_rectImgReal.left+nXZoomed,m_rectImgReal.top);
		pDC->LineTo(m_rectImgReal.left+nXZoomed,m_rectImgReal.bottom);
	}

	for (unsigned nMcuY=0;nMcuY<m_nMcuYMax;nMcuY++) {
		nYZoomed = (int)(nMcuY*m_nMcuHeight*m_nZoom);
		pDC->MoveTo(m_rectImgReal.left,m_rectImgReal.top+nYZoomed);
		pDC->LineTo(m_rectImgReal.right,m_rectImgReal.top+nYZoomed);
	}

	pDC->SelectObject(pPenOld);
	pDC->SetBkMode(nBkModeOld);
	
/*
	// Now draw a simple MCU Marker overlay
	CRect	my_rect;
	CBrush	my_brush(RGB(255, 0, 255));
	for (unsigned nMcuY=0;nMcuY<m_nMcuYMax;nMcuY++) {
		for (unsigned nMcuX=0;nMcuX<m_nMcuXMax;nMcuX++) {
			unsigned nXY = nMcuY*m_nMcuXMax + nMcuX;
			if (m_abMarkedMcuMap && m_abMarkedMcuMap[nXY]) {
				unsigned nXZoomed = (nMcuX*m_nMcuWidth*m_nZoom);
				unsigned nYZoomed = (nMcuY*m_nMcuHeight*m_nZoom);

				// Note that drawing is an overlay, so we are dealing with real
				// pixel coordinates, not preview image coords
				my_rect = CRect(nXZoomed,nYZoomed,nXZoomed+m_nMcuWidth*m_nZoom,nYZoomed+m_nMcuHeight*m_nZoom);
				my_rect.OffsetRect(m_rectImgReal.left,m_rectImgReal.top);
				pDC->FrameRect(my_rect,&my_brush);
			}
		}
	}
*/

}

void CimgDecode::ViewMcuMarkedOverlay(CDC* pDC)
{

	// Now draw a simple MCU Marker overlay
	CRect	my_rect;
	CBrush	my_brush(RGB(255, 0, 255));
	for (unsigned nMcuY=0;nMcuY<m_nMcuYMax;nMcuY++) {
		for (unsigned nMcuX=0;nMcuX<m_nMcuXMax;nMcuX++) {
			unsigned nXY = nMcuY*m_nMcuXMax + nMcuX;
			if (m_abMarkedMcuMap && m_abMarkedMcuMap[nXY]) {
				unsigned nXZoomed = (unsigned)(nMcuX*m_nMcuWidth*m_nZoom);
				unsigned nYZoomed = (unsigned)(nMcuY*m_nMcuHeight*m_nZoom);

				// Note that drawing is an overlay, so we are dealing with real
				// pixel coordinates, not preview image coords
				my_rect = CRect(nXZoomed,nYZoomed,nXZoomed+(unsigned)(m_nMcuWidth*m_nZoom),
					nYZoomed+(unsigned)(m_nMcuHeight*m_nZoom));
				my_rect.OffsetRect(m_rectImgReal.left,m_rectImgReal.top);
				pDC->FrameRect(my_rect,&my_brush);
			}
		}
	}

}


void CimgDecode::ViewMarkerOverlay(CDC* pDC,unsigned blk_x,unsigned blk_y)
{
	CRect	my_rect;
	CBrush	my_brush(RGB(255, 0, 255));

	// Note that drawing is an overlay, so we are dealing with real
	// pixel coordinates, not preview image coords
	my_rect = CRect(	(unsigned)(m_nZoom*(blk_x+0)),
						(unsigned)(m_nZoom*(blk_y+0)),
						(unsigned)(m_nZoom*(blk_x+1)),
						(unsigned)(m_nZoom*(blk_y+1)));
	my_rect.OffsetRect(m_nPreviewPosX,m_nPreviewPosY);
	pDC->FillRect(my_rect,&my_brush);
}

bool CimgDecode::GetPreviewOverlayMcuGrid()
{
	return m_bViewOverlaysMcuGrid;
}

void CimgDecode::SetPreviewOverlayMcuGridToggle()
{
	if (m_bViewOverlaysMcuGrid) {
		m_bViewOverlaysMcuGrid = false;
	} else {
		m_bViewOverlaysMcuGrid = true;
	}
}

void CimgDecode::ReportDcRun(unsigned nMcuX, unsigned nMcuY, unsigned nMcuLen)
{
	// FIXME Should I be working on MCU level or block level?
	CString	tmpStr;
	m_pLog->AddLine("");
	m_pLog->AddLineHdr("*** Reporting DC Levels ***");
	tmpStr.Format("  Starting MCU   = [%u,%u]",nMcuX,nMcuY);
	tmpStr.Format("  Number of MCUs = %u",nMcuLen);
	m_pLog->AddLine(tmpStr);
	for (unsigned ind=0;ind<nMcuLen;ind++)
	{
		// Need some way of getting these values!!
		// For now, just rely on bDetailVlcEn & PrintDcCumVal() to report this...
	}
}

void CimgDecode::SetStatusYccText(CString str)
{
	m_strStatusYcc = str;
}

CString CimgDecode::GetStatusYccText()
{
	return m_strStatusYcc;
}

void CimgDecode::SetStatusMcuText(CString str)
{
	m_strStatusMcu = str;
}

CString CimgDecode::GetStatusMcuText()
{
	return m_strStatusMcu;
}

void CimgDecode::SetStatusFilePosText(CString str)
{
	m_strStatusFilePos = str;
}

CString CimgDecode::GetStatusFilePosText()
{
	return m_strStatusFilePos;
}





const BYTE CimgDecode::m_anMaskByte[] = { 0x80,0xC0,0xE0,0xF0,0xF8,0xFC,0xFE,0xFF };


// FIXME would like to share this with CjfifDecode!
const unsigned CimgDecode::m_sZigZag[64] =
{
	 0, 1, 8,16, 9, 2, 3,10,
	17,24,32,25,18,11, 4, 5,
	12,19,26,33,40,48,41,34,
	27,20,13, 6, 7,14,21,28,
	35,42,49,56,57,50,43,36,
	29,22,15,23,30,37,44,51,
	58,59,52,45,38,31,39,46,
	53,60,61,54,47,55,62,63
};

const unsigned CimgDecode::m_sUnZigZag[64] =
{
	 0, 1, 5, 6,14,15,27,28,
	 2, 4, 7,13,16,26,29,42,
	 3, 8,12,17,25,30,41,43,
	 9,11,18,24,31,40,44,53,
	10,19,23,32,39,45,52,54,
	20,22,33,38,46,51,55,60,
	21,34,37,47,50,56,59,61,
	35,36,48,49,57,58,62,63
};
