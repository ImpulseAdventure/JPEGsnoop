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

#include "stdafx.h"

#include "ImgDecode.h"
#include "snoop.h"
#include <math.h>

#include "JPEGsnoop.h"


// ------------------------------------------------------
// Settings

// Flag: Use fixed point arithmetic for IDCT?
//#define IDCT_FIXEDPT

// Flag: Do we stop during scan decode if 0xFF (but not pad)?
// TODO: Make this a config option
//#define SCAN_BAD_MARKER_STOP


// ------------------------------------------------------
// Main code




// Reset decoding state for start of new decode
// Note that we don't touch the DQT or DHT entries as
// those are set at different times versus reset (sometimes
// before Reset() ).
void CimgDecode::Reset()
{
	DecodeRestartScanBuf(0,false);
	DecodeRestartDcState();

	m_bRestartRead = false;	// No restarts seen yet
	m_nRestartRead = 0;

	m_nImgSizeXPartMcu = 0;
	m_nImgSizeYPartMcu = 0;
	m_nImgSizeX  = 0;
	m_nImgSizeY  = 0;
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

	// If a DIB has been generated, release it!
	if (m_bDibTempReady) {
		m_pDibTemp.Kill();
		m_bDibTempReady = false;
	}

	if (m_bDibHistRgbReady) {
		m_pDibHistRgb.Kill();
		m_bDibHistRgbReady = false;
	}

	if (m_bDibHistYReady) {
		m_pDibHistY.Kill();
		m_bDibHistYReady = false;
	}

	if (m_pMcuFileMap) {
		delete [] m_pMcuFileMap;
		m_pMcuFileMap = NULL;
	}

	if (m_pBlkDcValY) {
		delete [] m_pBlkDcValY;
		m_pBlkDcValY = NULL;
	}
	if (m_pBlkDcValCb) {
		delete [] m_pBlkDcValCb;
		m_pBlkDcValCb = NULL;
	}
	if (m_pBlkDcValCr) {
		delete [] m_pBlkDcValCr;
		m_pBlkDcValCr = NULL;
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

}

// Constructor for the Image Decoder
// - This constructor is called only once by Document class
CimgDecode::CimgDecode(CDocLog* pLog, CwindowBuf* pWBuf)
{
	// Ideally this would be passed by constructor, but simply access
	// directly for now.
	CJPEGsnoopApp*	pApp;
	pApp = (CJPEGsnoopApp*)AfxGetApp();
    m_pAppConfig = pApp->m_pAppConfig;
	ASSERT(m_pAppConfig);

	if (DEBUG_EN) m_pAppConfig->DebugLogAdd(_T("CimgDecode::CimgDecode() Begin"));

	m_bVerbose = false;

	ASSERT(pLog);
	ASSERT(pWBuf);
	m_pLog = pLog;
	m_pWBuf = pWBuf;

	m_pStatBar = NULL;
	m_bDibTempReady = false;
	m_bPreviewIsJpeg = false;
	m_bDibHistRgbReady = false;
	m_bDibHistYReady = false;

	m_bHistEn = false;
	m_bStatClipEn = false;	// UNUSED

	m_pMcuFileMap = NULL;
	m_pBlkDcValY = NULL;
	m_pBlkDcValCb = NULL;
	m_pBlkDcValCr = NULL;
	m_pPixValY = NULL;
	m_pPixValCb = NULL;
	m_pPixValCr = NULL;

	if (DEBUG_EN) m_pAppConfig->DebugLogAdd(_T("CimgDecode::CimgDecode() Checkpoint 1"));

	// Reset the image decoding state
	Reset();

	if (DEBUG_EN) m_pAppConfig->DebugLogAdd(_T("CimgDecode::CimgDecode() Checkpoint 2"));

	m_nImgSizeXPartMcu = 0;
	m_nImgSizeYPartMcu = 0;
	m_nImgSizeX = 0;
	m_nImgSizeY = 0;

	// FIXME: Temporary hack to avoid divide-by-0 when displaying PSD (instead of JPEG)
	m_nMcuWidth = 1;
	m_nMcuHeight = 1;

	// Detailed VLC Decode mode
	m_bDetailVlc = false;
	m_nDetailVlcX = 0;
	m_nDetailVlcY = 0;
	m_nDetailVlcLen = 1;

	m_bDecodeScanAc = true;


	// Set up the IDCT lookup tables
	PrecalcIdct();

	if (DEBUG_EN) m_pAppConfig->DebugLogAdd(_T("CimgDecode::CimgDecode() Checkpoint 3"));

	GenLookupHuffMask();
	if (DEBUG_EN) m_pAppConfig->DebugLogAdd(_T("CimgDecode::CimgDecode() Checkpoint 4"));



	// The following contain information that is set by
	// the JFIF Decoder. We can only reset them here during
	// the constructor and later by explicit call by JFIF Decoder.
	ResetState();
	if (DEBUG_EN) m_pAppConfig->DebugLogAdd(_T("CimgDecode::CimgDecode() Checkpoint 5"));

	// We don't call SetPreviewMode() here because it would
	// automatically try to recalculate the view (but nothing ready yet)
	m_nPreviewMode = PREVIEW_RGB;
	SetPreviewZoom(false,false,true,PRV_ZOOM_12);
	if (DEBUG_EN) m_pAppConfig->DebugLogAdd(_T("CimgDecode::CimgDecode() Checkpoint 6"));

	m_bViewOverlaysMcuGrid = false;

	// Start off with no YCC offsets for CalcChannelPreview()
	SetPreviewYccOffset(0,0,0,0,0);
	if (DEBUG_EN) m_pAppConfig->DebugLogAdd(_T("CimgDecode::CimgDecode() Checkpoint 7"));

	SetPreviewMcuInsert(0,0,0);
	if (DEBUG_EN) m_pAppConfig->DebugLogAdd(_T("CimgDecode::CimgDecode() Checkpoint 8"));

	if (DEBUG_EN) m_pAppConfig->DebugLogAdd(_T("CimgDecode::CimgDecode() End"));
}


// Destructor for Image Decode class
// - Deallocate any image-related dynamic storage
CimgDecode::~CimgDecode()
{
	if (m_pMcuFileMap) {
		delete [] m_pMcuFileMap;
		m_pMcuFileMap = NULL;
	}

	if (m_pBlkDcValY) {
		delete [] m_pBlkDcValY;
		m_pBlkDcValY = NULL;
	}
	if (m_pBlkDcValCb) {
		delete [] m_pBlkDcValCb;
		m_pBlkDcValCb = NULL;
	}
	if (m_pBlkDcValCr) {
		delete [] m_pBlkDcValCr;
		m_pBlkDcValCr = NULL;
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

}

// Reset the major parameters
// - Called by JFIF Decoder when we begin a new file
// TODO: Consider merging with Reset()
// POST:
// - m_anSofSampFactH[]
// - m_anSofSampFactV[]
// - m_bImgDetailsSet
// - m_nNumSofComps
// - m_nPrecision
// - m_bScanErrorsDisable
// - m_nMarkersBlkNum
//
void CimgDecode::ResetState()
{
	ResetDhtLookup();
	ResetDqtTables();

	for (unsigned nCompInd=0;nCompInd<MAX_SOF_COMP_NF;nCompInd++) {
		m_anSofSampFactH[nCompInd] = 0;
		m_anSofSampFactV[nCompInd] = 0;
	}

	m_bImgDetailsSet = false;
	m_nNumSofComps = 0;

	m_nPrecision = 0; // Default to "precision not set"

	m_bScanErrorsDisable = false;

	// Reset the markers
	m_nMarkersBlkNum = 0;

}

// Save a copy of the status bar control
//
// INPUT:
// - pStatBar			= Pointer to status bar
// POST:
// - m_pStatBar
//
void CimgDecode::SetStatusBar(CStatusBar* pStatBar)
{
	m_pStatBar = pStatBar;
}


// Update the status bar text
//
// INPUT:
// - str				= New text to display on status bar
// PRE:
// - m_pStatBar
//
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
// POST:
// - m_anDqtTblSel[]
// - m_anDqtCoeff[][]
// - m_anDqtCoeffZz[][]
void CimgDecode::ResetDqtTables()
{
	for (unsigned nDqtComp=0;nDqtComp<MAX_DQT_COMP;nDqtComp++) {
		// Force entries to an invalid value. This makes
		// sure that we have to get a valid SetDqtTables() call
		// from JfifDecode first.
		m_anDqtTblSel[nDqtComp] = -1;
	}

	for (unsigned nDestId=0;nDestId<MAX_DQT_DEST_ID;nDestId++) {
		for (unsigned nCoeff=0;nCoeff<MAX_DQT_COEFF;nCoeff++) {
			m_anDqtCoeff[nDestId][nCoeff] = 0;
			m_anDqtCoeffZz[nDestId][nCoeff] = 0;
		}
	}

	m_nNumSofComps = 0;
}

// Reset the DHT lookup tables
// - These tables are used to speed up VLC lookups
// - This should be called by the JFIF decoder any time we start a new file
// POST:
// - m_anDhtLookupSetMax[]
// - m_anDhtLookupSize[][]
// - m_anDhtLookup_bitlen[][][]
// - m_anDhtLookup_bits[][][]
// - m_anDhtLookup_code[][][]
// - m_anDhtLookupfast[][][]
//
void CimgDecode::ResetDhtLookup()
{
	memset(m_anDhtHisto,0,sizeof(m_anDhtHisto));

	// Use explicit loop ranges instead of memset
	for (unsigned nClass=DHT_CLASS_DC;nClass<=DHT_CLASS_AC;nClass++) {
		m_anDhtLookupSetMax [nClass] = 0;
		// DHT table destination ID is range 0..3
		for (unsigned nDestId=0;nDestId<MAX_DHT_DEST_ID;nDestId++) {
			m_anDhtLookupSize[nClass][nDestId] = 0;
			for (unsigned nCodeInd=0;nCodeInd<MAX_DHT_CODES;nCodeInd++) {
				m_anDhtLookup_bitlen[nClass][nDestId][nCodeInd] = 0;
				m_anDhtLookup_bits  [nClass][nDestId][nCodeInd] = 0;
				m_anDhtLookup_mask  [nClass][nDestId][nCodeInd] = 0;
				m_anDhtLookup_code  [nClass][nDestId][nCodeInd] = 0;
			}
			for (unsigned nElem=0;nElem<(2<<DHT_FAST_SIZE);nElem++) {
				// Mark with invalid value
				m_anDhtLookupfast[nClass][nDestId][nElem] = DHT_CODE_UNUSED;
			}
		}

		for (unsigned nCompInd=0;nCompInd<1+MAX_SOS_COMP_NS;nCompInd++) {
			// Force entries to an invalid value. This makes
			// sure that we have to get a valid SetDhtTables() call
			// from JfifDecode first.
			// Even though nCompInd is supposed to be 1-based numbering,
			// we start at index 0 to ensure it is marked as invalid.
			m_anDhtTblSel[nClass][nCompInd] = -1;
		}
	}

	m_nNumSosComps = 0;
}


// Configure an entry in a quantization table
//
// INPUT:
// - nSet			= Quant table dest ID (from DQT:Tq)
// - nInd			= Coeff index (normal order)
// - nIndzz			= Coeff index (zigzag order)
// - nCoeff			= Coeff value
// POST:
// - m_anDqtCoeff[]
// - m_anDqtCoeffZz[]
// RETURN:
// - True if params in range, false otherwise
//
// NOTE: Asynchronously called by JFIF Decoder
//
bool CimgDecode::SetDqtEntry(unsigned nTblDestId, unsigned nCoeffInd, unsigned nCoeffIndZz, unsigned short nCoeffVal)
{
	if ((nTblDestId < MAX_DQT_DEST_ID) && (nCoeffInd < MAX_DQT_COEFF)) {
		m_anDqtCoeff[nTblDestId][nCoeffInd] = nCoeffVal;

		// Save a copy that represents the original zigzag order
		// This is used by the IDCT logic
		m_anDqtCoeffZz[nTblDestId][nCoeffIndZz] = nCoeffVal;

	} else {
		// Should never get here!
		CString strTmp;
		strTmp.Format(_T("ERROR: Attempt to set DQT entry out of range (nTblDestId=%u, nCoeffInd=%u, nCoeffVal=%u)"),
			nTblDestId,nCoeffInd,nCoeffVal);

#ifdef DEBUG_LOG
		CString	strDebug;
		strDebug.Format(_T("## File=[%-100s] Block=[%-10s] Error=[%s]\n"),(LPCTSTR)m_pAppConfig->strCurFname,
			_T("ImgDecode"),(LPCTSTR)strTmp);
		OutputDebugString(strDebug);
#else
		ASSERT(false);
#endif

		if (m_pAppConfig->bInteractive)
			AfxMessageBox(strTmp);
		return false;
	}
	return true;
}


// Fetch a DQT table entry
//
// INPUT:
// - nTblDestId				= DQT Table Destination ID
// - nCoeffInd				= Coefficient index in 8x8 matrix
// PRE:
// - m_anDqtCoeff[][]
// RETURN:
// - Returns the indexed DQT matrix entry
//
unsigned CimgDecode::GetDqtEntry(unsigned nTblDestId, unsigned nCoeffInd)
{
	if ((nTblDestId < MAX_DQT_DEST_ID) && (nCoeffInd < MAX_DQT_COEFF)) {
		return m_anDqtCoeff[nTblDestId][nCoeffInd];
	} else {
		// Should never get here!
		CString strTmp;
		strTmp.Format(_T("ERROR: GetDqtEntry(nTblDestId=%u, nCoeffInd=%u) out of indexed range"),
			nTblDestId,nCoeffInd);
		m_pLog->AddLineErr(strTmp);
		if (m_pAppConfig->bInteractive)
			AfxMessageBox(strTmp);

#ifdef DEBUG_LOG
		CString	strDebug;
		strDebug.Format(_T("## File=[%-100s] Block=[%-10s] Error=[%s]\n"),(LPCTSTR)m_pAppConfig->strCurFname,
			_T("ImgDecode"),(LPCTSTR)strTmp);
		OutputDebugString(strDebug);
#else
		ASSERT(false);
#endif

		return 0;
	}
}


// Set a DQT table for a frame image component identifier
//
// INPUT:
// - nCompInd			= Component index. Based on m_nSofNumComps_Nf-1 (ie. 0..254)
// - nTbl				= DQT Table number. Based on SOF:Tqi (ie. 0..3)
// POST:
// - m_anDqtTblSel[]
// RETURN:
// - Success if index and table are in range
// NOTE:
// - Asynchronously called by JFIF Decoder
//
bool CimgDecode::SetDqtTables(unsigned nCompId, unsigned nTbl)
{
	if ((nCompId < MAX_SOF_COMP_NF) && (nTbl < MAX_DQT_DEST_ID)) {
		m_anDqtTblSel[nCompId] = (int)nTbl;
	} else {
		// Should never get here unless the JFIF SOF table has a bad entry!
		CString strTmp;
		strTmp.Format(_T("ERROR: SetDqtTables(Comp ID=%u, Table=%u) out of indexed range"),
			nCompId,nTbl);
		m_pLog->AddLineErr(strTmp);
		if (m_pAppConfig->bInteractive)
			AfxMessageBox(strTmp);
		return false;
	}
	return true;
}

// Set a DHT table for a scan image component index
// - The DHT Table select array is stored as:
//   m_anDhtTblSel[0][1,2,3] for DC
//   m_anDhtTblSel[1][1,2,3] for AC
//
// INPUT:
// - nCompInd			= Component index (1-based). Range 1..4
// - nTblDc				= DHT table index for DC elements of component
// - nTblAc				= DHT table index for AC elements of component
// POST:
// - m_anDhtTblSel[][]
// RETURN:
// - Success if indices are in range
//
bool CimgDecode::SetDhtTables(unsigned nCompInd, unsigned nTblDc, unsigned nTblAc)
{
	// Note use of (nCompInd < MAX_SOS_COMP_NS+1) as nCompInd is 1-based notation
	if ((nCompInd>=1) && (nCompInd < MAX_SOS_COMP_NS+1) && (nTblDc < MAX_DHT_DEST_ID) && (nTblAc < MAX_DHT_DEST_ID)) {
		m_anDhtTblSel[DHT_CLASS_DC][nCompInd] = (int)nTblDc;
		m_anDhtTblSel[DHT_CLASS_AC][nCompInd] = (int)nTblAc;
	} else {
		// Should never get here!
		CString strTmp;
		strTmp.Format(_T("ERROR: SetDhtTables(comp=%u, TblDC=%u TblAC=%u) out of indexed range"),
			nCompInd,nTblDc,nTblAc);
		m_pLog->AddLineErr(strTmp);
		if (m_pAppConfig->bInteractive)
			AfxMessageBox(strTmp);
		return false;
	}
	return true;
}



// Get the precision field
//
// INPUT:
// - nPrecision			= DCT sample precision (typically 8 or 12)
// POST:
// - m_nPrecision
//
void CimgDecode::SetPrecision(unsigned nPrecision)
{
	m_nPrecision = nPrecision;
}


// Set the general image details for the image decoder
//
// INPUT:
// - nDimX				= Image dimension (X)
// - nDimY				= Image dimension (Y)
// - nCompsSOF			= Number of components in Frame
// - nCompsSOS			= Number of components in Scan
// - bRstEn				= Restart markers present?
// - nRstInterval		= Restart marker interval
// POST:
// - m_bImgDetailsSet
// - m_nDimX
// - m_nDimY
// - m_nNumSofComps
// - m_nNumSosComps
// - m_bRestartEn
// - m_nRestartInterval
// NOTE:
// - Called asynchronously by the JFIF decoder
//
void CimgDecode::SetImageDetails(unsigned nDimX,unsigned nDimY,unsigned nCompsSOF,unsigned nCompsSOS, bool bRstEn,unsigned nRstInterval)
{
	m_bImgDetailsSet = true;
	m_nDimX = nDimX;
	m_nDimY = nDimY;
	m_nNumSofComps = nCompsSOF;
	m_nNumSosComps = nCompsSOS;
	m_bRestartEn = bRstEn;
	m_nRestartInterval = nRstInterval;
}

// Reset the image content to prepare it for the upcoming scans
// - TODO: Migrate pixel bitmap allocation / clearing from DecodeScanImg() to here
void CimgDecode::ResetImageContent()
{
}

// Set the sampling factor for an image component
//
// INPUT:
// - nCompInd			= Component index from Nf (ie. 1..255)
// - nSampFactH			= Sampling factor in horizontal direction
// - nSampFactV			= Sampling factor in vertical direction
// POST:
// - m_anSofSampFactH[]
// - m_anSofSampFactV[]
// NOTE:
// - Called asynchronously by the JFIF decoder in SOF
//
void CimgDecode::SetSofSampFactors(unsigned nCompInd,unsigned nSampFactH, unsigned nSampFactV)
{
	// TODO: Check range
	m_anSofSampFactH[nCompInd] = nSampFactH;
	m_anSofSampFactV[nCompInd] = nSampFactV;
}


// Update the preview mode (affects channel display)
//
// INPUT:
// - nMode				= Mode used in channel display (eg. NONE, RGB, YCC)
//                        See PREVIEW_* constants
//
void CimgDecode::SetPreviewMode(unsigned nMode)
{
	// Need to check to see if mode has changed. If so, we
	// need to recalculate the temporary preview.
	m_nPreviewMode = nMode;
	CalcChannelPreview();
}

// Update any level shifts for the preview display
//
// INPUT:
// - nMcuX				= MCU index in X direction
// - nMcuY				= MCU index in Y direction
// - nY					= DC shift in Y component
// - nCb				= DC shift in Cb component
// - nCr				= DC shift in Cr component
//
void CimgDecode::SetPreviewYccOffset(unsigned nMcuX,unsigned nMcuY,int nY,int nCb,int nCr)
{
	m_nPreviewShiftY  = nY;
	m_nPreviewShiftCb = nCb;
	m_nPreviewShiftCr = nCr;
	m_nPreviewShiftMcuX = nMcuX;
	m_nPreviewShiftMcuY = nMcuY;

	CalcChannelPreview();
}

// Fetch the current level shift setting for the preview display
//
// OUTPUT:
// - nMcuX				= MCU index in X direction
// - nMcuY				= MCU index in Y direction
// - nY					= DC shift in Y component
// - nCb				= DC shift in Cb component
// - nCr				= DC shift in Cr component
//
void CimgDecode::GetPreviewYccOffset(unsigned &nMcuX,unsigned &nMcuY,int &nY,int &nCb,int &nCr)
{
	nY = m_nPreviewShiftY;
	nCb = m_nPreviewShiftCb;
	nCr = m_nPreviewShiftCr;
	nMcuX = m_nPreviewShiftMcuX;
	nMcuY = m_nPreviewShiftMcuY;
}


// Set the Preview MCU insert
// UNUSED
void CimgDecode::SetPreviewMcuInsert(unsigned nMcuX,unsigned nMcuY,int nLen)
{

	m_nPreviewInsMcuX = nMcuX;
	m_nPreviewInsMcuY = nMcuY;
	m_nPreviewInsMcuLen = nLen;

	CalcChannelPreview();
}

// Get the Preview MCU insert
// UNUSED
void CimgDecode::GetPreviewMcuInsert(unsigned &nMcuX,unsigned &nMcuY,unsigned &nLen)
{
	nMcuX = m_nPreviewInsMcuX;
	nMcuY = m_nPreviewInsMcuY;
	nLen = m_nPreviewInsMcuLen;
}

// Fetch the coordinate of the top-left corner of the preview image
//
// OUTPUT:
// - nX				= X coordinate of top-left corner
// - nY				= Y coordinate of top-left corner
//
void CimgDecode::GetPreviewPos(unsigned &nX,unsigned &nY)
{
	nX = m_nPreviewPosX;
	nY = m_nPreviewPosY;
}

// Fetch the dimensions of the preview image
//
// OUTPUT:
// - nX				= X dimension of image
// - nY				= Y dimension of iamge
//
void CimgDecode::GetPreviewSize(unsigned &nX,unsigned &nY)
{
	nX = m_nPreviewSizeX;
	nY = m_nPreviewSizeY;
}

// Set a DHT table entry and associated lookup table
// - Configures the fast lookup table for short code bitstrings
//
// INPUT:
// - nDestId			= DHT destination table ID (0..3)
// - nClass				= Select between DC and AC tables (0=DC, 1=AC)
// - nInd				= Index into table
// - nLen				= Huffman code length
// - nBits				= Huffman code bitstring (left justified)
// - nMask				= Huffman code bit mask (left justified)
// - nCode				= Huffman code value
// POST:
// - m_anDhtLookup_bitlen[][][]
// - m_anDhtLookup_bits[][][]
// - m_anDhtLookup_mask[][][]
// - m_anDhtLookup_code[][][]
// - m_anDhtLookupSetMax[]
// - m_anDhtLookupfast[][][]
// RETURN:
// - Success if indices are in range
// NOTE:
// - Asynchronously called by JFIF Decoder
//
bool CimgDecode::SetDhtEntry(unsigned nDestId, unsigned nClass, unsigned nInd, unsigned nLen,
							 unsigned nBits, unsigned nMask, unsigned nCode)
{
	if ( (nDestId >= MAX_DHT_DEST_ID) || (nClass >= MAX_DHT_CLASS) || (nInd >= MAX_DHT_CODES) ) {
		CString strTmp = _T("ERROR: Attempt to set DHT entry out of range");
		m_pLog->AddLineErr(strTmp);
		if (m_pAppConfig->bInteractive)
			AfxMessageBox(strTmp);
#ifdef DEBUG_LOG
		CString	strDebug;
		strDebug.Format(_T("## File=[%-100s] Block=[%-10s] Error=[%s]\n"),(LPCTSTR)m_pAppConfig->strCurFname,
			_T("ImgDecode"),(LPCTSTR)strTmp);
		OutputDebugString(strDebug);
#else
		ASSERT(false);
#endif
		return false;
	}
	m_anDhtLookup_bitlen[nClass][nDestId][nInd] = nLen;
	m_anDhtLookup_bits[nClass][nDestId][nInd] = nBits;
	m_anDhtLookup_mask[nClass][nDestId][nInd] = nMask;
	m_anDhtLookup_code[nClass][nDestId][nInd] = nCode;


	// Record the highest numbered DHT set.
	// TODO: Currently assuming that there are no missing tables in the sequence
	if (nDestId > m_anDhtLookupSetMax[nClass]) {
		m_anDhtLookupSetMax[nClass] = nDestId;
	}

	unsigned	nBitsMsb;
	unsigned	nBitsExtraLen;
	unsigned	nBitsExtraVal;
	unsigned	nBitsMax;
	unsigned	nFastVal;

	// If the variable-length code is short enough, add it to the
	// fast lookup table.
	if (nLen <= DHT_FAST_SIZE) {
		// nBits is a left-justified number (assume right-most bits are zero)
		// nLen is number of leading bits to compare

		//   nLen     = 5
		//   nBits    = 32'b1011_1xxx_xxxx_xxxx_xxxx_xxxx_xxxx_xxxx  (0xB800_0000)
		//   nBitsMsb =  8'b1011_1xxx (0xB8)
		//   nBitsMsb =  8'b1011_1000 (0xB8)
		//   nBitsMax =  8'b1011_1111 (0xBF)

		//   nBitsExtraLen = 8-len = 8-5 = 3

		//   nBitsExtraVal = (1<<nBitsExtraLen) -1 = 1<<3 -1 = 8'b0000_1000 -1 = 8'b0000_0111
		//
		//   nBitsMax = nBitsMsb + nBitsExtraVal
		//   nBitsMax =  8'b1011_1111
		nBitsMsb = (nBits & nMask) >> (32-DHT_FAST_SIZE);
		nBitsExtraLen = DHT_FAST_SIZE-nLen;
		nBitsExtraVal = (1<<nBitsExtraLen) - 1;
		nBitsMax = nBitsMsb + nBitsExtraVal;

		for (unsigned ind1=nBitsMsb;ind1<=nBitsMax;ind1++) {
			// The arrangement of the dht_lookupfast[] values:
			//                         0xFFFF_FFFF = No entry, resort to slow search
			//    m_anDhtLookupfast[nClass][nDestId] [15: 8] = nLen
			//    m_anDhtLookupfast[nClass][nDestId] [ 7: 0] = nCode
			//
			//ASSERT(code <= 0xFF);
			//ASSERT(ind1 <= 0xFF);
			nFastVal = nCode + (nLen << 8);
			m_anDhtLookupfast[nClass][nDestId][ind1] = nFastVal;
		}
	}
	return true;
}


// Assign the size of the DHT table
//
// INPUT:
// - nDestId			= Destination DHT table ID (From DHT:Th, range 0..3)
// - nClass				= Select between DC and AC tables (0=DC, 1=AC) (From DHT:Tc, range 0..1)
// - nSize				= Number of entries in the DHT table
// POST:
// - m_anDhtLookupSize[][]
// RETURN:
// - Success if indices are in range
//
bool CimgDecode::SetDhtSize(unsigned nDestId,unsigned nClass,unsigned nSize)
{
	if ( (nDestId >= MAX_DHT_DEST_ID) || (nClass >= MAX_DHT_CLASS) || (nSize >= MAX_DHT_CODES) ) {
		CString strTmp = _T("ERROR: Attempt to set DHT table size out of range");
		m_pLog->AddLineErr(strTmp);
		if (m_pAppConfig->bInteractive)
			AfxMessageBox(strTmp);
		return false;
	} else {
		m_anDhtLookupSize[nClass][nDestId] = nSize;
	}

	return true;
}


// Convert huffman code (DC) to signed value
// - Convert according to ITU-T.81 Table 5
//
// INPUT:
// - nVal				= Huffman DC value (left justified)
// - nBits				= Bitstring length of value
// RETURN:
// - Signed integer representing Huffman DC value
//
signed CimgDecode::HuffmanDc2Signed(unsigned nVal,unsigned nBits)
{
	if (nVal >= (unsigned)(1<<(nBits-1))) {
		return (signed)(nVal);
	} else {
		return (signed)( nVal - ((1<<nBits)-1) );
	}
}


// Generate the Huffman code lookup table mask
//
// POST:
// - m_anHuffMaskLookup[]
//
void CimgDecode::GenLookupHuffMask()
{
	unsigned int nMask;
	for (unsigned nLen=0;nLen<32;nLen++)
	{
		nMask = (1 << (nLen))-1;
		nMask <<= 32-nLen;
		m_anHuffMaskLookup[nLen] = nMask;
	}
}


// Extract a specified number of bits from a 32-bit holding register
//
// INPUT:
// - nWord				= The 32-bit holding register
// - nBits				= Number of bits (leftmost) to extract from the holding register
// PRE:
// - m_anHuffMaskLookup[]
// RETURN:
// - The subset of bits extracted from the holding register
// NOTE:
// - This routine is inlined for speed
//
inline unsigned CimgDecode::ExtractBits(unsigned nWord,unsigned nBits)
{
	unsigned nVal;
	nVal = (nWord & m_anHuffMaskLookup[nBits]) >> (32-nBits);
	return nVal;
}


// Removes bits from the holding buffer
// - Discard the leftmost "nNumBits" of m_nScanBuff
// - And then realign file position pointers
//
// INPUT:
// - nNumBits				= Number of left-most bits to consume
// POST:
// - m_nScanBuff
// - m_nScanBuff_vacant
// - m_nScanBuffPtr_align
// - m_anScanBuffPtr_pos[]
// - m_anScanBuffPtr_err[]
// - m_nScanBuffLatchErr
// - m_nScanBuffPtr_num
//
inline void CimgDecode::ScanBuffConsume(unsigned nNumBits)
{
	m_nScanBuff <<= nNumBits;
	m_nScanBuff_vacant += nNumBits;

	// Need to latch any errors during consumption of multi-bytes
	// otherwise we'll miss the error notification if we skip over
	// it before we exit this routine! e.g if num_bytes=2 and error
	// appears on first byte, then we want to retain it in pos[0]


	// Now realign the file position pointers if necessary
	unsigned nNumBytes = (m_nScanBuffPtr_align+nNumBits) / 8;
	for (unsigned nInd=0;nInd<nNumBytes;nInd++) {
		m_anScanBuffPtr_pos[0] = m_anScanBuffPtr_pos[1];
		m_anScanBuffPtr_pos[1] = m_anScanBuffPtr_pos[2];
		m_anScanBuffPtr_pos[2] = m_anScanBuffPtr_pos[3];
		// Don't clear the last ptr position because during an overread
		// this will be the only place that the last position was preserved
		//m_anScanBuffPtr_pos[3] = 0;

		m_anScanBuffPtr_err[0] = m_anScanBuffPtr_err[1];
		m_anScanBuffPtr_err[1] = m_anScanBuffPtr_err[2];
		m_anScanBuffPtr_err[2] = m_anScanBuffPtr_err[3];
		m_anScanBuffPtr_err[3] = SCANBUF_OK;

		if (m_anScanBuffPtr_err[0] != SCANBUF_OK) {
			m_nScanBuffLatchErr = m_anScanBuffPtr_err[0];
		}

		m_nScanBuffPtr_num--;
	}
	m_nScanBuffPtr_align = (m_nScanBuffPtr_align+nNumBits)%8;

}


// Augment the current scan buffer with another byte
// - Extra bits are added to right side of existing bitstring
//
// INPUT:
// - nNewByte			= 8-bit byte to add to buffer
// - nPtr				= UNUSED
// PRE:
// - m_nScanBuff
// - m_nScanBuff_vacant
// - m_nScanBuffPtr_num
// POST:
// - m_nScanBuff
// - m_nScanBuff_vacant
// - m_anScanBuffPtr_err[]
// - m_anScanBuffPtr_pos[]
//
inline void CimgDecode::ScanBuffAdd(unsigned nNewByte,unsigned nPtr)
{
	// Add the new byte to the buffer
	// Assume that m_nScanBuff has already been shifted to be
	// aligned to bit 31 as first bit.
	m_nScanBuff += (nNewByte << (m_nScanBuff_vacant-8));
	m_nScanBuff_vacant -= 8;

	ASSERT(m_nScanBuffPtr_num<4);
	if (m_nScanBuffPtr_num>=4) { return; } // Unexpected by design
	m_anScanBuffPtr_err[m_nScanBuffPtr_num]   = SCANBUF_OK;
	m_anScanBuffPtr_pos[m_nScanBuffPtr_num++] = nPtr;

	// m_nScanBuffPtr_align stays the same as we add 8 bits
}

// Augment the current scan buffer with another byte (but mark as error)
//
// INPUT:
// - nNewByte			= 8-bit byte to add to buffer
// - nPtr				= UNUSED
// - nErr				= Error code to associate with this buffer byte
// POST:
// - m_anScanBuffPtr_err[]
//
inline void CimgDecode::ScanBuffAddErr(unsigned nNewByte,unsigned nPtr,unsigned nErr)
{
	ScanBuffAdd(nNewByte,nPtr);
	m_anScanBuffPtr_err[m_nScanBuffPtr_num-1]   = nErr;

}

// Disable any further reporting of scan errors
//
// PRE:
// - m_nScanErrMax
// POST:
// - m_nWarnBadScanNum
// - m_bScanErrorsDisable
//
void CimgDecode::ScanErrorsDisable()
{
	m_nWarnBadScanNum = m_nScanErrMax;
	m_bScanErrorsDisable = true;
}

// Enable reporting of scan errors
//
// POST:
// - m_nWarnBadScanNum
// - m_bScanErrorsDisable
//
void CimgDecode::ScanErrorsEnable()
{
	m_nWarnBadScanNum = 0;
	m_bScanErrorsDisable = false;
}


// Read in bits from the buffer and find matching huffman codes
// - Input buffer is m_nScanBuff
// - Perform shift in buffer afterwards
// - Abort if there aren't enough bits (note that the scan buffer
//   is almost empty when we reach the end of a scan segment)
//
// INPUT:
// - nClass					= DHT Table class (0..1)
// - nTbl					= DHT Destination ID (0..3)
// PRE:
// - Assume that dht_lookup_size[nTbl] != 0 (already checked)
// - m_bRestartRead
// - m_nScanBuff_vacant
// - m_nScanErrMax
// - m_nScanBuff
// - m_anDhtLookupFast[][][]
// - m_anDhtLookupSize[][]
// - m_anDhtLookup_mask[][][]
// - m_anDhtLookup_bits[][][]
// - m_anDhtLookup_bitlen[][][]
// - m_anDhtLookup_code[][][]
// - m_nPrecision
// POST:
// - m_nScanBitsUsed# is calculated
// - m_bScanEnd
// - m_bScanBad
// - m_nWarnBadScanNum
// - m_anDhtHisto[][][]
// OUTPUT:
// - rZrl				= Zero run amount (if any)
// - rVal				= Coefficient value
// RETURN:
// - Status from attempting to decode the current value
//
// PERFORMANCE:
// - Tried to unroll all function calls listed below,
//   but performance was same before & after (27sec).
// - Calls unrolled: BuffTopup(), ScanBuffConsume(),
//                   ExtractBits(), HuffmanDc2Signed()
teRsvRet CimgDecode::ReadScanVal(unsigned nClass,unsigned nTbl,unsigned &rZrl,signed &rVal)
{
	bool		bDone = false;
	unsigned	nInd = 0;
	unsigned	nCode = DHT_CODE_UNUSED; // Not a valid nCode
	unsigned	nVal;

	ASSERT(nClass < MAX_DHT_CLASS);
	ASSERT(nTbl < MAX_DHT_DEST_ID);

	m_nScanBitsUsed1 = 0;		// bits consumed for nCode
	m_nScanBitsUsed2 = 0;

	rZrl = 0;
	rVal = 0;

	// First check to see if we've entered here with a completely empty
	// scan buffer with a restart marker already observed. In that case
	// we want to exit with condition 3 (restart terminated)
	if ( (m_nScanBuff_vacant == 32) && (m_bRestartRead) ) {
		return RSV_RST_TERM;
	}


	// Has the scan buffer been depleted?
	if (m_nScanBuff_vacant >= 32) {
		// Trying to overread end of scan segment

		if (m_nWarnBadScanNum < m_nScanErrMax) {
			CString strTmp;
			strTmp.Format(_T("*** ERROR: Overread scan segment (before nCode)! @ Offset: %s"),(LPCTSTR)GetScanBufPos());
			m_pLog->AddLineErr(strTmp);

			m_nWarnBadScanNum++;
			if (m_nWarnBadScanNum >= m_nScanErrMax) {
				strTmp.Format(_T("    Only reported first %u instances of this message..."),m_nScanErrMax);
				m_pLog->AddLineErr(strTmp);
			}
		}

		m_bScanEnd = true;
		m_bScanBad = true;
		return RSV_UNDERFLOW;
	}

	// Top up the buffer just in case
	BuffTopup();

	bDone = false;
	bool bFound = false;

	// Fast search for variable-length huffman nCode
	// Do direct lookup for any codes DHT_FAST_SIZE bits or shorter

	unsigned nCodeMsb;
	unsigned nCodeFastSearch;

	// Only enable this fast search if m_nScanBuff_vacant implies
	// that we have at least DHT_FAST_SIZE bits available in the buffer!
	if ((32-m_nScanBuff_vacant) >= DHT_FAST_SIZE) {
		nCodeMsb = m_nScanBuff>>(32-DHT_FAST_SIZE);
		nCodeFastSearch = m_anDhtLookupfast[nClass][nTbl][nCodeMsb];
		if (nCodeFastSearch != DHT_CODE_UNUSED) {
			// We found the code!
			m_nScanBitsUsed1 += (nCodeFastSearch>>8);
			nCode = (nCodeFastSearch & 0xFF);
			bDone = true;
			bFound = true;
		}
	}


	// Slow search for variable-length huffman nCode
	while (!bDone) {
		unsigned nBitLen;
		if ((m_nScanBuff & m_anDhtLookup_mask[nClass][nTbl][nInd]) == m_anDhtLookup_bits[nClass][nTbl][nInd]) {

			nBitLen = m_anDhtLookup_bitlen[nClass][nTbl][nInd];
			// Just in case this VLC bit string is longer than the number of
			// bits we have left in the buffer (due to restart marker or end
			// of scan data), we need to double-check
			if (nBitLen <= 32-m_nScanBuff_vacant) {
				nCode = m_anDhtLookup_code[nClass][nTbl][nInd];
				m_nScanBitsUsed1 += nBitLen;
				bDone = true;
				bFound = true;
			}
		}
		nInd++;
		if (nInd >= m_anDhtLookupSize[nClass][nTbl]) {
			bDone = true;
		}
	}

	// Could not find huffman nCode in table!
	if (!bFound) {

		// If we didn't find the huffman nCode, it might be due to a
		// restart marker that prematurely stopped the scan buffer filling.
		// If so, return with an indication so that DecodeScanComp() can
		// handle the restart marker, refill the scan buffer and then
		// re-do ReadScanVal()
		if (m_bRestartRead) {
			return RSV_RST_TERM;
		}

		// FIXME:
		// What should we be consuming here? we need to make forward progress
		// in file. Options:
		// 1) Move forward to next byte in file
		// 2) Move forward to next bit in file
		// Currently moving 1 bit so that we have slightly more opportunities
		// to re-align earlier.
		m_nScanBitsUsed1 = 1;
		nCode = DHT_CODE_UNUSED;
	}

	// Log an entry into a histogram
	if (m_nScanBitsUsed1 < MAX_DHT_CODELEN+1) {
		m_anDhtHisto[nClass][nTbl][m_nScanBitsUsed1]++;
	} else {
		ASSERT(false);
		// Somehow we got out of range
	}


	ScanBuffConsume(m_nScanBitsUsed1);

	// Did we overread the scan buffer?
	if (m_nScanBuff_vacant > 32) {
		// The nCode consumed more bits than we had!
		CString strTmp;
		strTmp.Format(_T("*** ERROR: Overread scan segment (after nCode)! @ Offset: %s"),(LPCTSTR)GetScanBufPos());
		m_pLog->AddLineErr(strTmp);
		m_bScanEnd = true;
		m_bScanBad = true;
		return RSV_UNDERFLOW;
	}

	// Replenish buffer after nCode extraction and before variable extra bits
	// This is required because we may have a 12 bit nCode followed by a 16 bit var length bitstring
	BuffTopup();

	// Did we find the nCode?
	if (nCode != DHT_CODE_UNUSED) {
		rZrl = (nCode & 0xF0) >> 4;
		m_nScanBitsUsed2 = nCode & 0x0F;
		if ( (rZrl == 0) && (m_nScanBitsUsed2 == 0) ) {
			// EOB (was bits_extra=0)
			return RSV_EOB;
		} else if (m_nScanBitsUsed2 == 0) {
			// Zero rValue
			rVal = 0;
			return RSV_OK;

		} else {
			// Normal nCode
			nVal = ExtractBits(m_nScanBuff,m_nScanBitsUsed2);
			rVal = HuffmanDc2Signed(nVal,m_nScanBitsUsed2);

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

			// Did we overread the scan buffer?
			if (m_nScanBuff_vacant > 32) {
				// The nCode consumed more bits than we had!
				CString strTmp;
				strTmp.Format(_T("*** ERROR: Overread scan segment (after bitstring)! @ Offset: %s"),(LPCTSTR)GetScanBufPos());
				m_pLog->AddLineErr(strTmp);
				m_bScanEnd = true;
				m_bScanBad = true;
				return RSV_UNDERFLOW;
			}

			return RSV_OK;
		}
	} else {
		// ERROR: Invalid huffman nCode!

		// FIXME: We may also enter here if we are about to encounter a
		// restart marker! Need to see if ScanBuf is terminated by
		// restart marker; if so, then we simply flush the ScanBuf,
		// consume the 2-byte RST marker, clear the ScanBuf terminate
		// reason, then indicate to caller that they need to call ReadScanVal
		// again.

		if (m_nWarnBadScanNum < m_nScanErrMax) {
			CString strTmp;
			strTmp.Format(_T("*** ERROR: Can't find huffman bitstring @ %s, table %u, value [0x%08x]"),(LPCTSTR)GetScanBufPos(),nTbl,m_nScanBuff);
			m_pLog->AddLineErr(strTmp);

			m_nWarnBadScanNum++;
			if (m_nWarnBadScanNum >= m_nScanErrMax) {
				strTmp.Format(_T("    Only reported first %u instances of this message..."),m_nScanErrMax);
				m_pLog->AddLineErr(strTmp);
			}
		}

		// TODO: What rValue and ZRL should we return?
		m_bScanBad = true;
		return RSV_UNDERFLOW;
	}

	// NOTE: Can't reach here
	// return RSV_UNDERFLOW;
}



// Refill the scan buffer as needed
//
void CimgDecode::BuffTopup()
{
	unsigned nRetVal;

	// Do we have space to load another byte?
	bool bDone = (m_nScanBuff_vacant < 8);

	// Have we already reached the end of the scan segment?
	if (m_bScanEnd) {
		bDone = true;
	}

	while (!bDone) {
		nRetVal = BuffAddByte();

		// NOTE: If we have read in a restart marker, the above call will not
		// read in any more bits into the scan buffer, so we should just simply
		// say that we've done the best we can for the top up.
		if (m_bRestartRead) {
			bDone = true;
		}

		if (m_nScanBuff_vacant < 8) {
			bDone = true;
		}
		// If the buffer read returned an error or end of scan segment
		// then stop filling buffer
		if (nRetVal != 0) {
			bDone = true;
		}
	}
}

// Check for restart marker and compare the index against expected
//
// PRE:
// - m_nScanBuffPtr
// RETURN:
// - Restart marker found at the current buffer position
// NOTE:
// - This routine expects that we did not remove restart markers
//   from the bytestream (in BuffAddByte).
//
bool CimgDecode::ExpectRestart()
{
	unsigned nMarker;

	unsigned nBuf0 = m_pWBuf->Buf(m_nScanBuffPtr);
	unsigned nBuf1 = m_pWBuf->Buf(m_nScanBuffPtr+1);

	// Check for restart marker first. Assume none back-to-back.
	if (nBuf0 == 0xFF) {
		nMarker = nBuf1;

		// FIXME: Later, check that we are on the right marker!
		if ((nMarker >= JFIF_RST0) && (nMarker <= JFIF_RST7)) {

			if (m_bVerbose) {
	  		  CString strTmp;
			  strTmp.Format(_T("  RESTART marker: @ 0x%08X.0 : RST%02u"),
				m_nScanBuffPtr,nMarker-JFIF_RST0);
			  m_pLog->AddLine(strTmp);
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

// Add a byte to the scan buffer from the file
// - Handle stuff bytes
// - Handle restart markers
//
// PRE:
// - m_nScanBuffPtr
// POST:
// - m_nRestartRead
// - m_nRestartLastInd
//
unsigned CimgDecode::BuffAddByte()
{
	unsigned nMarker = 0x00;
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
		nMarker = nBuf1;

		if ((nMarker >= JFIF_RST0) && (nMarker <= JFIF_RST7)) {

			if (m_bVerbose) {
	  		  CString strTmp;
			  strTmp.Format(_T("  RESTART marker: @ 0x%08X.0 : RST%02u"),
				m_nScanBuffPtr,nMarker-JFIF_RST0);
			  m_pLog->AddLine(strTmp);
			}

			m_nRestartRead++;
			m_nRestartLastInd = nMarker - JFIF_RST0;
			if (m_nRestartLastInd != m_nRestartExpectInd) {
				if (!m_bScanErrorsDisable) {
	  				CString strTmp;
					strTmp.Format(_T("  ERROR: Expected RST marker index RST%u got RST%u @ 0x%08X.0"),
						m_nRestartExpectInd,m_nRestartLastInd,m_nScanBuffPtr);
					m_pLog->AddLineErr(strTmp);
				}
			}
			m_nRestartExpectInd = (m_nRestartLastInd+1)%8;

			// FIXME: Later on, we should be checking for RST out of sequence

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
			m_anScanBuffPtr_err[m_nScanBuffPtr_num] = SCANBUF_RST;

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


	// Check for Byte Stuff
	if ((nBuf0 == 0xFF) && (nBuf1 == 0x00)) {

		// Add byte to m_nScanBuff & record file position
		ScanBuffAdd(nBuf0,m_nScanBuffPtr);
		m_nScanBuffPtr+=2;


	} else if ((nBuf0 == 0xFF) && (nBuf1 == 0xFF)) {
		// NOTE:
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
			CString strTmp;
			strTmp.Format(_T("  Scan Data encountered sequence 0xFFFF @ 0x%08X.0 - Assume start of marker pad at end of scan segment"),
				m_nScanBuffPtr);
			m_pLog->AddLineWarn(strTmp);

			m_nWarnBadScanNum++;
			if (m_nWarnBadScanNum >= m_nScanErrMax) {
				strTmp.Format(_T("    Only reported first %u instances of this message..."),m_nScanErrMax);
				m_pLog->AddLineErr(strTmp);
			}
		}

		// Treat as single byte of byte stuff for now, since we don't
		// know if FF bytes will arrive in pairs or not.
		m_nScanBuffPtr+=1;
		*/

		// NOTE:
		// If I treat the 0xFFFF as a potential marker pad, we may not stop correctly
		// upon error if we see this inside the image somewhere (not at end).
		// Therefore, let's simply add these bytes to the buffer and let the DecodeScanImg()
		// routine figure out when we're at the end, etc.

		ScanBuffAdd(nBuf0,m_nScanBuffPtr);
		m_nScanBuffPtr+=1;

	} else if ((nBuf0 == 0xFF) && (nMarker != 0x00)) {

		// We have read a marker... don't assume that this is bad as it will
		// always happen at the end of the scan segment. Therefore, we will
		// assume this marker is valid (ie. not bit error in scan stream)
		// and mark the end of the scan segment.

		if (m_nWarnBadScanNum < m_nScanErrMax) {
			CString strTmp;
			strTmp.Format(_T("  Scan Data encountered marker   0xFF%02X @ 0x%08X.0"),
				nMarker,m_nScanBuffPtr);
			m_pLog->AddLine(strTmp);

			if (nMarker != JFIF_EOI) {
				m_pLog->AddLineErr(_T("  NOTE: Marker wasn't EOI (0xFFD9)"));
			}

			m_nWarnBadScanNum++;
			if (m_nWarnBadScanNum >= m_nScanErrMax) {
				strTmp.Format(_T("    Only reported first %u instances of this message..."),m_nScanErrMax);
				m_pLog->AddLineErr(strTmp);
			}
		}


		// Optionally stop immediately upon a bad marker
#ifdef SCAN_BAD_MARKER_STOP
		m_bScanEnd = true;
		return 1;
#else
		// Add byte to m_nScanBuff & record file position
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


// Define minimum value before we include DCT entry in
// the IDCT calcs.
// NOTE: Currently disabled
#define IDCT_COEF_THRESH 4


// Decode a single component for one block of an MCU
// - Pull bits from the main buffer
// - Find matching huffman codes
// - Fill in the IDCT matrix (m_anDctBlock[])
// - Perform the IDCT to create spatial domain
//
// INPUT:
// - nTblDhtDc					= DHT table ID for DC component
// - nTblDhtAc					= DHT talbe ID for AC component
// - nMcuX						= UNUSED
// - nMcuY						= UNUSED
// RETURN:
// - Indicate if the decode was successful
// POST:
// - m_nScanCurDc = DC component value
// Performance:
// - This routine is called on every block so it must be
//   reasonably efficient. Only call string routines when we
//   are sure that we have an error.
//
// FIXME: Consider adding checks for DHT table like in ReadScanVal()
//
bool CimgDecode::DecodeScanComp(unsigned nTblDhtDc,unsigned nTblDhtAc,unsigned nTblDqt,unsigned nMcuX,unsigned nMcuY)
{
	nMcuX;	// Unreferenced param
	nMcuY;	// Unreferenced param
	unsigned		nZrl;
	signed			nVal;
	bool			bDone = false;
	bool			bDC = true;	// Start with DC coeff

	teRsvRet	eRsvRet;	// Return value from ReadScanVal()

	unsigned nNumCoeffs = 0;
	//unsigned nDctMax = 0;			// Maximum DCT coefficient to use for IDCT
	unsigned nSavedBufPos = 0;
	unsigned nSavedBufErr = SCANBUF_OK;
	unsigned nSavedBufAlign = 0;

	// Profiling: No difference noted
	DecodeIdctClear();

	while (!bDone) {
		BuffTopup();

		// Note that once we perform ReadScanVal(), then GetScanBufPos() will be
		// after the decoded VLC
		// Save old file position info in case we want accurate error positioning
		nSavedBufPos   = m_anScanBuffPtr_pos[0];
		nSavedBufErr   = m_nScanBuffLatchErr;
		nSavedBufAlign = m_nScanBuffPtr_align;

		// ReadScanVal return values:
		// - RSV_OK			OK
		// - RSV_EOB		End of block
		// - RSV_UNDERFLOW	Ran out of data in buffer
		// - RSV_RST_TERM	No huffman code found, but restart marker seen
		// Assume nTblDht just points to DC tables, adjust for AC
		// e.g. nTblDht = 0,2,4
		eRsvRet = ReadScanVal(bDC?0:1,bDC?nTblDhtDc:nTblDhtAc,nZrl,nVal);

		// Handle Restart marker first.
		if (eRsvRet == RSV_RST_TERM) {
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
			DecodeRestartScanBuf(m_nScanBuffPtr,true);

			// Step 4
			m_bRestartRead = false;

			// Step 5
			BuffTopup();

			// Step 6
			// ASSERT is because we assume that we don't get 2 restart
			// markers in a row!
			eRsvRet = ReadScanVal(bDC?0:1,bDC?nTblDhtDc:nTblDhtAc,nZrl,nVal);
			ASSERT(eRsvRet != RSV_RST_TERM);

		}

		// In case we encountered a restart marker or bad scan marker
		if (nSavedBufErr == SCANBUF_BADMARK) {

			// Mark as scan error
			m_nScanCurErr = true;

			m_bScanBad = true;

			if (m_nWarnBadScanNum < m_nScanErrMax) {
				CString strPos = GetScanBufPos(nSavedBufPos,nSavedBufAlign);
				CString strTmp;
				strTmp.Format(_T("*** ERROR: Bad marker @ %s"),(LPCTSTR)strPos);
				m_pLog->AddLineErr(strTmp);

				m_nWarnBadScanNum++;
				if (m_nWarnBadScanNum >= m_nScanErrMax) {
					strTmp.Format(_T("    Only reported first %u instances of this message..."),m_nScanErrMax);
					m_pLog->AddLineErr(strTmp);
				}
			}

			// Reset the latched error now that we've dealt with it
			m_nScanBuffLatchErr = SCANBUF_OK;

		}


		short int	nVal2;
		nVal2 = static_cast<short int>(nVal & 0xFFFF);

		if (eRsvRet == RSV_OK) {
			// DC entry is always one value only
			if (bDC) {
				DecodeIdctSet(nTblDqt,nNumCoeffs,nZrl,nVal2); //CALZ
				bDC = false;			// Now we will be on AC comps
			} else {
				// We're on AC entry, so keep looping until
				// we have finished up to 63 entries
				// Set entry in table
				// PERFORMANCE:
				//   No noticeable difference if following is skipped
				if (m_bDecodeScanAc) {
					DecodeIdctSet(nTblDqt,nNumCoeffs,nZrl,nVal2);
				}
			}
		} else if (eRsvRet == RSV_EOB) {
			if (bDC) {
				DecodeIdctSet(nTblDqt,nNumCoeffs,nZrl,nVal2); //CALZ
				// Now that we have finished the DC coefficient, start on AC coefficients
				bDC = false;
			} else {
				// Now that we have finished the AC coefficients, we are done
				bDone = true;
			}
			//
		} else if (eRsvRet == RSV_UNDERFLOW) {
			// ERROR

			if (m_nWarnBadScanNum < m_nScanErrMax) {
				CString strPos = GetScanBufPos(nSavedBufPos,nSavedBufAlign);
				CString strTmp;
				strTmp.Format(_T("*** ERROR: Bad huffman code @ %s"),(LPCTSTR)strPos);
				m_pLog->AddLineErr(strTmp);

				m_nWarnBadScanNum++;
				if (m_nWarnBadScanNum >= m_nScanErrMax) {
					strTmp.Format(_T("    Only reported first %u instances of this message..."),m_nScanErrMax);
					m_pLog->AddLineErr(strTmp);
				}
			}

			m_nScanCurErr = true;
			bDone = true;

			return false;
		}

		// Increment the number of coefficients
		nNumCoeffs += 1+nZrl;

		// If we have filled out an entire 64 entries, then we move to
		// the next block without an EOB
		// NOTE: This is only 63 entries because we assume that we
		//       are doing the AC (DC was already bDone in a different pass)

		// FIXME: Would like to combine DC & AC in one pass so that
		// we don't end up having to use 2 tables. The check below will
		// also need to be changed to == 64.
		//
		// Currently, we will have to correct AC nNumCoeffs entries (in IDCT) to
		// be +1 to get real index, as we are ignoring DC position 0.

		if (nNumCoeffs == 64) {
			bDone = true;
		} else if (nNumCoeffs > 64) {
			// ERROR

			if (m_nWarnBadScanNum < m_nScanErrMax) {
				CString strTmp;
				CString strPos = GetScanBufPos(nSavedBufPos,nSavedBufAlign);
				strTmp.Format(_T("*** ERROR: @ %s, nNumCoeffs>64 [%u]"),(LPCTSTR)strPos,nNumCoeffs);
				m_pLog->AddLineErr(strTmp);

				m_nWarnBadScanNum++;
				if (m_nWarnBadScanNum >= m_nScanErrMax) {
					strTmp.Format(_T("    Only reported first %u instances of this message..."),m_nScanErrMax);
					m_pLog->AddLineErr(strTmp);
				}
			}

			m_nScanCurErr = true;
			m_bScanBad = true;
			bDone = true;

			nNumCoeffs = 64;	// Just to ensure we don't use an overrun value anywhere
		}


	}

	// We finished the MCU
	// Now calc the IDCT matrix

	// The following code needs to be very efficient.
	// A number of experiments have been carried out to determine
	// the magnitude of speed improvements through various settings
	// and IDCT methods:
	//
	// PERFORMANCE:
	//   Example file: canon_1dsmk2_
	//
	//   0:06	Turn off m_bDecodeScanAc (so no array memset, etc.)
	//   0:10   m_bDecodeScanAc=true, but DecodeIdctCalc() skipped
	//   0:26	m_bDecodeScanAc=true and DecodeIdctCalcFixedpt()
	//   0:27	m_bDecodeScanAc=true and DecodeIdctCalcFloat()

	if (m_bDecodeScanAc) {
#ifdef IDCT_FIXEDPT
		DecodeIdctCalcFixedpt();
#else

		// TODO: Select appropriate conversion routine based on performance
//		DecodeIdctCalcFloat(nDctMax);
//		DecodeIdctCalcFloat(nNumCoeffs);
//		DecodeIdctCalcFloat(m_nDctCoefMax);
		DecodeIdctCalcFloat(64);
//		DecodeIdctCalcFloat(32);

#endif
	}


	return true;
}


// Decode a single component for one block of an MCU with printing
// used for the Detailed Decode functionality
// - Same as DecodeScanComp() but adds reporting of variable length codes (VLC)
//
// INPUT:
// - nTblDhtDc					= DHT table ID for DC component
// - nTblDhtAc					= DHT talbe ID for AC component
// - nMcuX						= Current MCU X coordinate (for reporting only)
// - nMcuY						= Current MCU Y coordinate (for reporting only)
// RETURN:
// - Indicate if the decode was successful
// POST:
// - m_nScanCurDc = DC component value
// PERFORMANCE:
// - As this routine is called for every MCU, it is important
//   for it to be efficient. However, we are in print mode, so
//   we can afford to be slower.
//
// FIXME: need to fix like DecodeScanComp() (ordering of exit conditions, etc.)
// FIXME: Consider adding checks for DHT table like in ReadScanVal()
//
bool CimgDecode::DecodeScanCompPrint(unsigned nTblDhtDc,unsigned nTblDhtAc,unsigned nTblDqt,unsigned nMcuX,unsigned nMcuY)
{
	bool		bPrint = true;
	teRsvRet	eRsvRet;
	CString		strTmp;
	CString		strTbl;
	CString		strSpecial;
	CString		strPos;
	unsigned	nZrl;
	signed		nVal;
	bool		bDone = false;

	bool		bDC = true;	// Start with DC component

	if (bPrint) {
		switch(nTblDqt) {
			case 0:
				strTbl = _T("Lum");
				break;
			case 1:
				strTbl = _T("Chr(0)"); // Usually Cb
				break;
			case 2:
				strTbl = _T("Chr(1)"); // Usually Cr
				break;
			default:
				strTbl = _T("???");
				break;
		}
		strTmp.Format(_T("    %s (Tbl #%u), MCU=[%u,%u]"),(LPCTSTR)strTbl,nTblDqt,nMcuX,nMcuY);
		m_pLog->AddLine(strTmp);
	}

	unsigned nNumCoeffs = 0;
	unsigned nSavedBufPos = 0;
	unsigned nSavedBufErr = SCANBUF_OK;
	unsigned nSavedBufAlign = 0;

	DecodeIdctClear();

	while (!bDone) {
		BuffTopup();

		// Note that once we perform ReadScanVal(), then GetScanBufPos() will be
		// after the decoded VLC

		// Save old file position info in case we want accurate error positioning
		nSavedBufPos   = m_anScanBuffPtr_pos[0];
		nSavedBufErr   = m_nScanBuffLatchErr;
		nSavedBufAlign = m_nScanBuffPtr_align;

		// Return values:
		//	0 - OK
		//  1 - EOB
		//  2 - Overread error
		//  3 - No huffman code found, but restart marker seen
		eRsvRet = ReadScanVal(bDC?0:1,bDC?nTblDhtDc:nTblDhtAc,nZrl,nVal);

		// Handle Restart marker first.
		if (eRsvRet == RSV_RST_TERM) {
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
			DecodeRestartScanBuf(m_nScanBuffPtr,true);

			// Step 4
			m_bRestartRead = false;

			// Step 5
			BuffTopup();

			// Step 6
			// ASSERT is because we assume that we don't get 2 restart
			// markers in a row!
			eRsvRet = ReadScanVal(bDC?0:1,bDC?nTblDhtDc:nTblDhtAc,nZrl,nVal);
			ASSERT(eRsvRet != RSV_RST_TERM);

		}

		// In case we encountered a restart marker or bad scan marker
		if (nSavedBufErr == SCANBUF_BADMARK) {

			// Mark as scan error
			m_nScanCurErr = true;

			m_bScanBad = true;

			if (m_nWarnBadScanNum < m_nScanErrMax) {
				strPos = GetScanBufPos(nSavedBufPos,nSavedBufAlign);
				strTmp.Format(_T("*** ERROR: Bad marker @ %s"),(LPCTSTR)strPos);
				m_pLog->AddLineErr(strTmp);

				m_nWarnBadScanNum++;
				if (m_nWarnBadScanNum >= m_nScanErrMax) {
					strTmp.Format(_T("    Only reported first %u instances of this message..."),m_nScanErrMax);
					m_pLog->AddLineErr(strTmp);
				}
			}

			// Reset the latched error now that we've dealt with it
			m_nScanBuffLatchErr = SCANBUF_OK;

		}


		// Should this be before or after restart checks?
		unsigned nCoeffStart = nNumCoeffs;
		unsigned nCoeffEnd   = nNumCoeffs+nZrl;

		short int	nVal2;
		nVal2 = static_cast<short int>(nVal & 0xFFFF);

		if (eRsvRet == RSV_OK) {
			strSpecial = _T("");
			// DC entry is always one value only
			// FIXME: Do I need nTblDqt == 4 as well?
			if (bDC) {
				DecodeIdctSet(nTblDqt,nNumCoeffs,nZrl,nVal2);
				bDC = false;			// Now we will be on AC comps
			} else {
				// We're on AC entry, so keep looping until
				// we have finished up to 63 entries
				// Set entry in table
				DecodeIdctSet(nTblDqt,nNumCoeffs,nZrl,nVal2);
			}
		} else if (eRsvRet == RSV_EOB) {
			if (bDC) {
				DecodeIdctSet(nTblDqt,nNumCoeffs,nZrl,nVal2);
				bDC = false;			// Now we will be on AC comps
			} else {
				bDone = true;
			}
			strSpecial = _T("EOB");
		} else if (eRsvRet == RSV_UNDERFLOW) {

			if (m_nWarnBadScanNum < m_nScanErrMax) {
				strSpecial = _T("ERROR");
				strPos = GetScanBufPos(nSavedBufPos,nSavedBufAlign);

				strTmp.Format(_T("*** ERROR: Bad huffman code @ %s"),(LPCTSTR)strPos);
				m_pLog->AddLineErr(strTmp);

				m_nWarnBadScanNum++;
				if (m_nWarnBadScanNum >= m_nScanErrMax) {
					strTmp.Format(_T("    Only reported first %u instances of this message..."),m_nScanErrMax);
					m_pLog->AddLineErr(strTmp);
				}
			}

			m_nScanCurErr = true;
			bDone = true;

			// Print out before we leave
			if (bPrint) {
				ReportVlc(nSavedBufPos,nSavedBufAlign,nZrl,nVal2,
					nCoeffStart,nCoeffEnd,strSpecial);
			}


			return false;
		}

		// Increment the number of coefficients
		nNumCoeffs += 1+nZrl;
		// If we have filled out an entire 64 entries, then we move to
		// the next block without an EOB
		// NOTE: This is only 63 entries because we assume that we
		//       are doing the AC (DC was already done in a different pass)
		if (nNumCoeffs == 64) {
			strSpecial = _T("EOB64");
			bDone = true;
		} else if (nNumCoeffs > 64) {
			// ERROR

			if (m_nWarnBadScanNum < m_nScanErrMax) {
				strPos = GetScanBufPos(nSavedBufPos,nSavedBufAlign);
				strTmp.Format(_T("*** ERROR: @ %s, nNumCoeffs>64 [%u]"),(LPCTSTR)strPos,nNumCoeffs);
				m_pLog->AddLineErr(strTmp);

				m_nWarnBadScanNum++;
				if (m_nWarnBadScanNum >= m_nScanErrMax) {
					strTmp.Format(_T("    Only reported first %u instances of this message..."),m_nScanErrMax);
					m_pLog->AddLineErr(strTmp);
				}
			}

			m_nScanCurErr = true;
			m_bScanBad = true;
			bDone = true;

			nNumCoeffs = 64;	// Just to ensure we don't use an overrun value anywhere
		}

		if (bPrint) {
			ReportVlc(nSavedBufPos,nSavedBufAlign,nZrl,nVal2,
				nCoeffStart,nCoeffEnd,strSpecial);
		}

	}

	// We finished the MCU component


	// Now calc the IDCT matrix
#ifdef IDCT_FIXEDPT
	DecodeIdctCalcFixedpt();
#else
//	DecodeIdctCalcFloat(nNumCoeffs);
	DecodeIdctCalcFloat(64);
#endif

	// Now report the coefficient matrix (after zigzag reordering)
	if (bPrint) {
		ReportDctMatrix();
	}

	return true;
}




// Print out the DCT matrix for a given block
//
// PRE:
// - m_anDctBlock[]
//
void CimgDecode::ReportDctMatrix()
{
	CString	strTmp;
	CString	strLine;
	int		nCoefVal;

	for (unsigned nY=0;nY<8;nY++) {
		if (nY == 0) {
			strLine = _T("                      DCT Matrix=[");
		} else {
			strLine = _T("                                 [");
		}
		for (unsigned nX=0;nX<8;nX++) {
			strTmp = _T("");
			nCoefVal = m_anDctBlock[nY*8+nX];
			strTmp.Format(_T("%5d"),nCoefVal);
			strLine.Append(strTmp);

			if (nX != 7) {
				strLine.Append(_T(" "));
			}
		}
		strLine.Append(_T("]"));
		m_pLog->AddLine(strLine);
	}
	m_pLog->AddLine(_T(""));

}


// Report out the variable length codes (VLC)
//
// Need to consider impact of padding bytes... When pads get extracted, they
// will not appear in the binary display shown below. Might want the get buffer
// to do post-pad removal first.
//
// eg. file Dsc0019.jpg
// Overlay 0x4215 = 7FFF0000 len=4
//
// INPUT:
// - nVlcPos				=
// - nVlcAlign				=
// - nZrl					=
// - nVal					=
// - nCoeffStart			=
// - nCoeffEnd				=
// - specialStr				=
//
void CimgDecode::ReportVlc(unsigned nVlcPos, unsigned nVlcAlign,
						   unsigned nZrl, int nVal,
						   unsigned nCoeffStart,unsigned nCoeffEnd,
						   CString specialStr)
{
	CString		strPos;
	CString		strTmp;

	unsigned	nBufByte[4];
	unsigned	nBufPosInd = nVlcPos;
	CString		strData = _T("");
	CString		strByte1 = _T("");
	CString		strByte2 = _T("");
	CString		strByte3 = _T("");
	CString		strByte4 = _T("");
	CString		strBytes = _T("");
	CString		strBytesOrig = _T("");
	CString		strBinMarked = _T("");

	strPos = GetScanBufPos(nVlcPos,nVlcAlign);

	// Read in the buffer bytes, but skip pad bytes (0xFF00 -> 0xFF)

	// We need to look at previous byte as it might have been
	// start of stuff byte! If so, we need to ignore the byte
	// and advance the pointers.
	BYTE nBufBytePre = m_pWBuf->Buf(nBufPosInd-1);
	nBufByte[0] = m_pWBuf->Buf(nBufPosInd++);
	if ( (nBufBytePre == 0xFF) && (nBufByte[0] == 0x00) ) {
		nBufByte[0] = m_pWBuf->Buf(nBufPosInd++);
	}

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

	strByte1 = Dec2Bin(nBufByte[0],8,true);
	strByte2 = Dec2Bin(nBufByte[1],8,true);
	strByte3 = Dec2Bin(nBufByte[2],8,true);
	strByte4 = Dec2Bin(nBufByte[3],8,true);
	strBytesOrig = strByte1 + _T(" ") + strByte2 + _T(" ") + strByte3 + _T(" ") + strByte4;
	strBytes = strByte1 + strByte2 + strByte3 + strByte4;

	for (unsigned ind=0;ind<nVlcAlign;ind++) {
		strBinMarked += _T("-");
	}

	strBinMarked += strBytes.Mid(nVlcAlign,m_nScanBitsUsed1+m_nScanBitsUsed2);

	for (unsigned ind=nVlcAlign+m_nScanBitsUsed1+m_nScanBitsUsed2;ind<32;ind++) {
		strBinMarked += _T("-");
	}

	strBinMarked.Insert(24,_T(" "));
	strBinMarked.Insert(16,_T(" "));
	strBinMarked.Insert(8,_T(" "));


	strData.Format(_T("0x %02X %02X %02X %02X = 0b (%s)"),
		nBufByte[0],nBufByte[1],nBufByte[2],nBufByte[3],
		(LPCTSTR)strBinMarked);

	if ((nCoeffStart == 0) && (nCoeffEnd == 0)) {
		strTmp.Format(_T("      [%s]: ZRL=[%2u] Val=[%5d] Coef=[%02u= DC] Data=[%s] %s"),
			(LPCTSTR)strPos,nZrl,nVal,nCoeffStart,(LPCTSTR)strData,(LPCTSTR)specialStr);
	} else {
		strTmp.Format(_T("      [%s]: ZRL=[%2u] Val=[%5d] Coef=[%02u..%02u] Data=[%s] %s"),
			(LPCTSTR)strPos,nZrl,nVal,nCoeffStart,nCoeffEnd,(LPCTSTR)strData,(LPCTSTR)specialStr);
	}
	m_pLog->AddLine(strTmp);

}


// Clear input and output matrix
//
// POST:
// - m_anDctBlock[]
// - m_afIdctBlock[]
// - m_anIdctBlock[]
// - m_nDctCoefMax
//
void CimgDecode::DecodeIdctClear()
{
	memset(m_anDctBlock,  0, sizeof m_anDctBlock);
	memset(m_afIdctBlock, 0, sizeof m_afIdctBlock);
	memset(m_anIdctBlock, 0, sizeof m_anIdctBlock);

	m_nDctCoefMax = 0;
}

// Set the DCT matrix entry
// - Fills in m_anDctBlock[] with the unquantized coefficients
// - Reversing the quantization is done using m_anDqtCoeffZz[][]
//
// INPUT:
// - nDqtTbl				=
// - num_coeffs				=
// - zrl					=
// - val					=
// PRE:
// - glb_anZigZag[]
// - m_anDqtCoeffZz[][]
// - m_nDctCoefMax
// POST:
// - m_anDctBlock[]
// NOTE:
// - We need to convert between the zigzag order and the normal order
//
void CimgDecode::DecodeIdctSet(unsigned nDqtTbl,unsigned num_coeffs,unsigned zrl,short int val)
{
	unsigned ind = num_coeffs+zrl;
	if (ind >= 64) {
		// We have an error! Don't set the block. Skip this comp for now
		// After this call, we will likely trap the error.
	} else {
		unsigned nDctInd = glb_anZigZag[ind];
		short int nValUnquant = val * m_anDqtCoeffZz[nDqtTbl][ind];

		/*
		// NOTE:
		//  To test steganography analysis, we can experiment with dropping
		//  specific components of the image.
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
//
// POST:
// - m_afIdctLookup[]
// - m_anIdctLookup[]
// NOTE:
// - This is 4k entries @ 4B each = 16KB
//
void CimgDecode::PrecalcIdct()
{
	unsigned	nX,nY,nU,nV;
	unsigned	nYX,nVU;
	float		fCu,fCv;
	float		fCosProd;
	float		fInsideProd;

	float		fPi			= (float)3.141592654;
	float		fSqrtHalf	= (float)0.707106781;

	for (nY=0;nY<DCT_SZ_Y;nY++) {
		for (nX=0;nX<DCT_SZ_X;nX++) {

			nYX = nY*DCT_SZ_X + nX;

			for (nV=0;nV<DCT_SZ_Y;nV++) {
				for (nU=0;nU<DCT_SZ_X;nU++) {

					nVU = nV*DCT_SZ_X + nU;

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
// INPUT:
// - nCoefMax				= Maximum number of coefficients to calculate
// PRE:
// - m_afIdctLookup[][]
// - m_anDctBlock[]
// POST:
// - m_afIdctBlock[]
//
void CimgDecode::DecodeIdctCalcFloat(unsigned nCoefMax)
{
	unsigned	nYX,nVU;
	float		fSum;

	for (nYX=0;nYX<DCT_SZ_ALL;nYX++) {
		fSum = 0;

		// Skip DC coefficient!
		for (nVU=1;nVU<nCoefMax;nVU++) {
			fSum += m_afIdctLookup[nYX][nVU]*m_anDctBlock[nVU];
		}
		fSum *= 0.25;

		// Store the result
		// FIXME: Note that float->int is very slow!
		//   Should consider using fixed point instead!
		m_afIdctBlock[nYX] = fSum;
	}

}

// Fixed point version of DecodeIdctCalcFloat()
//
// PRE:
// - m_anIdctLookup[][]
// - m_anDctBlock[]
// POST:
// - m_anIdctBlock[]
//
void CimgDecode::DecodeIdctCalcFixedpt()
{
	unsigned	nYX,nVU;
	int			nSum;

	for (nYX=0;nYX<DCT_SZ_ALL;nYX++) {
		nSum = 0;
		// Skip DC coefficient!
		for (nVU=1;nVU<DCT_SZ_ALL;nVU++) {
			nSum += m_anIdctLookup[nYX][nVU] * m_anDctBlock[nVU];
		}

		nSum /= 4;

		// Store the result
		// FIXME: Note that float->int is very slow!
		//   Should consider using fixed point instead!
		m_anIdctBlock[nYX] = nSum >> 10;

	}

}

// Clear the entire pixel image arrays for all three components (YCC)
//
// INPUT:
// - nWidth					= Current allocated image width
// - nHeight				= Current allocated image height
// POST:
// - m_pPixValY
// - m_pPixValCb
// - m_pPixValCr
//
void CimgDecode::ClrFullRes(unsigned nWidth,unsigned nHeight)
{
	ASSERT(m_pPixValY);
	if (m_nNumSosComps == NUM_CHAN_YCC) {
		ASSERT(m_pPixValCb);
		ASSERT(m_pPixValCr);
	}
	// FIXME: Add in range checking here
	memset(m_pPixValY,  0, (nWidth * nHeight * sizeof(short)) );
	if (m_nNumSosComps == NUM_CHAN_YCC) {
		memset(m_pPixValCb, 0, (nWidth * nHeight * sizeof(short)) );
		memset(m_pPixValCr, 0, (nWidth * nHeight * sizeof(short)) );
	}
}

// Generate a single component's pixel content for one MCU
// - Fetch content from the 8x8 IDCT block (m_afIdctBlock[])
//   for the specified component (nComp)
// - Transfer the pixel content to the specified component's
//   pixel map (m_pPixValY[],m_pPixValCb[],m_pPixValCr[])
// - DC level shifting is performed (nDcOffset)
// - Replication of pixels according to Chroma Subsampling (sampling factors)
//
// INPUT:
// - nMcuX					=
// - nMcuY					=
// - nComp					= Component index (1,2,3)
// - nCssXInd				=
// - nCssYInd				=
// - nDcOffset				=
// PRE:
// - DecodeIdctCalc() already called on Lum AC, and Lum DC already done
//
void CimgDecode::SetFullRes(unsigned nMcuX,unsigned nMcuY,unsigned nComp,unsigned nCssXInd,unsigned nCssYInd,short int nDcOffset)
{
	unsigned	nYX;
	float		fVal;
	short int	nVal;
	unsigned	nChan;

	// Convert from Component index (1-based) to Channel index (0-based)
	// Component index is direct from SOF/SOS
	// Channel index is used for internal display representation
	if (nComp <= 0) {
#ifdef DEBUG_LOG
		CString	strTmp;
		CString	strDebug;
		strTmp.Format(_T("SetFullRes() with nComp <= 0 [%d]"),nComp);
		strDebug.Format(_T("## File=[%-100s] Block=[%-10s] Error=[%s]\n"),(LPCTSTR)m_pAppConfig->strCurFname,
			_T("ImgDecode"),(LPCTSTR)strTmp);
		OutputDebugString(strDebug);
#else
		ASSERT(false);
#endif
		return;
	}
	nChan = nComp - 1;

	unsigned	nPixMapW = m_nBlkXMax*BLK_SZ_X;	// Width of pixel map
	unsigned	nOffsetBlkCorner;	// Linear offset to top-left corner of block
	unsigned	nOffsetPixCorner;	// Linear offset to top-left corner of pixel (start point for expansion)

	// Calculate the linear pixel offset for the top-left corner of the block in the MCU
	nOffsetBlkCorner = ((nMcuY*m_nMcuHeight) + nCssYInd*BLK_SZ_X) * nPixMapW +
						((nMcuX*m_nMcuWidth)  + nCssXInd*BLK_SZ_Y);

	// Use the expansion factor to determine how many bits to replicate
	// Typically for luminance (Y) this will be 1 & 1
	// The replication factor is available in m_anExpandBitsMcuH[] and m_anExpandBitsMcuV[]

	// Step through all pixels in the block
	for (unsigned nY=0;nY<BLK_SZ_Y;nY++) {
		for (unsigned nX=0;nX<BLK_SZ_X;nX++) {
			nYX = nY*BLK_SZ_X+nX;

			// Fetch the pixel value from the IDCT 8x8 block
			// and perform DC level shift
#ifdef IDCT_FIXEDPT
			nVal = m_anIdctBlock[nYX];
			// TODO: Why do I need AC value x8 multiplier?
			nVal = (nVal*8) + nDcOffset;
#else
			fVal = m_afIdctBlock[nYX];
			// TODO: Why do I need AC value x8 multiplier?
			nVal = ((short int)(fVal*8) + nDcOffset);
#endif

			// NOTE: These range checks were already done in DecodeScanImg()
			ASSERT(nCssXInd<MAX_SAMP_FACT_H);
			ASSERT(nCssYInd<MAX_SAMP_FACT_V);
			ASSERT(nY<BLK_SZ_Y);
			ASSERT(nX<BLK_SZ_X);

			// Set the pixel value for the component

			// We start with the linear offset into the pixel map for the top-left
			// corner of the block. Then we adjust to determine the top-left corner
			// of the pixel that we may optionally expand in subsampling scenarios.

			// Calculate the top-left corner pixel linear offset after taking
			// into account any expansion in the X direction
			nOffsetPixCorner = nOffsetBlkCorner + nX*m_anExpandBitsMcuH[nComp];

			// Replication the pixels as specified in the sampling factor
			// This is typically done for the chrominance channels when
			// chroma subsamping is used.
			for (unsigned nIndV=0;nIndV<m_anExpandBitsMcuV[nComp];nIndV++) {
				for (unsigned nIndH=0;nIndH<m_anExpandBitsMcuH[nComp];nIndH++) {
					if (nChan == CHAN_Y) {
						m_pPixValY[nOffsetPixCorner+(nIndV*nPixMapW)+nIndH] = nVal;
					} else if (nChan == CHAN_CB) {
						m_pPixValCb[nOffsetPixCorner+(nIndV*nPixMapW)+nIndH] = nVal;
					} else if (nChan == CHAN_CR) {
						m_pPixValCr[nOffsetPixCorner+(nIndV*nPixMapW)+nIndH] = nVal;
					} else {
						ASSERT(false);
					}
				} // nIndH
			} // nIndV

		} // nX

		nOffsetBlkCorner += (nPixMapW * m_anExpandBitsMcuV[nComp]);

	} // nY

}




// Calculates the actual byte offset (from start of file) for
// the current position in the m_nScanBuff.
//
// PRE:
// - m_anScanBuffPtr_pos[]
// - m_nScanBuffPtr_align
// RETURN:
// - File position
//
CString CimgDecode::GetScanBufPos()
{
	return GetScanBufPos(m_anScanBuffPtr_pos[0],m_nScanBuffPtr_align);
}

// Generate a file position string that also indicates bit alignment
//
// INPUT:
// - pos			= File position (byte)
// - align			= File position (bit)
// RETURN:
// - Formatted string
//
CString CimgDecode::GetScanBufPos(unsigned pos, unsigned align)
{
	CString strTmp;
	strTmp.Format(_T("0x%08X.%u"),pos,align);
	return strTmp;
}


// Test the scan error flag and, if set, report out the position
//
// INPUT:
// - nMcuX				= MCU x coordinate
// - nMcuY				= MCU y coordinate
// - nCssIndH			= Chroma subsampling (horizontal)
// - nCssIndV			= Chroma subsampling (vertical)
// - nComp				= Image component
//
void CimgDecode::CheckScanErrors(unsigned nMcuX,unsigned nMcuY,unsigned nCssIndH,unsigned nCssIndV,unsigned nComp)
{
	//unsigned mcu_x_max = (m_nDimX/m_nMcuWidth);
	//unsigned mcu_y_max = (m_nDimY/m_nMcuHeight);

	// Determine pixel position, taking into account sampling quadrant as well
	unsigned err_pos_x = m_nMcuWidth*nMcuX  + nCssIndH*BLK_SZ_X;
	unsigned err_pos_y = m_nMcuHeight*nMcuY + nCssIndV*BLK_SZ_Y;

	if (m_nScanCurErr) {
		CString strTmp,errStr;

		// Report component and subsampling quadrant
		switch (nComp) {
			case SCAN_COMP_Y:
				strTmp.Format(_T("Lum CSS(%u,%u)"),nCssIndH,nCssIndV);
				break;
			case SCAN_COMP_CB:
				strTmp.Format(_T("Chr(Cb) CSS(%u,%u)"),nCssIndH,nCssIndV);
				break;
			case SCAN_COMP_CR:
				strTmp.Format(_T("Chr(Cr) CSS(%u,%u)"),nCssIndH,nCssIndV);
				break;
			default:
				// Unknown component
				strTmp.Format(_T("??? CSS(%u,%u)"),nCssIndH,nCssIndV);
				break;
		}


		if (m_nWarnBadScanNum < m_nScanErrMax) {

			errStr.Format(_T("*** ERROR: Bad scan data in MCU(%u,%u): %s @ Offset %s"),nMcuX,nMcuY,(LPCTSTR)strTmp,(LPCTSTR)GetScanBufPos());
			m_pLog->AddLineErr(errStr);
			errStr.Format(_T("           MCU located at pixel=(%u,%u)"),err_pos_x,err_pos_y);
			m_pLog->AddLineErr(errStr);

			//errStr.Format(_T("*** Resetting Error state to continue ***"));
			//m_pLog->AddLineErr(errStr);

			m_nWarnBadScanNum++;
			if (m_nWarnBadScanNum >= m_nScanErrMax) {
				strTmp.Format(_T("    Only reported first %u instances of this message..."),m_nScanErrMax);
				m_pLog->AddLineErr(strTmp);
			}
		}

		// TODO: Should we reset m_nScanCurErr?
		m_nScanCurErr = false;

		//errStr.Format(_T("*** Resetting Error state to continue ***"));
		//m_pLog->AddLineErr(errStr);

	} // Error?

}



// Report the cumulative DC value
//
// INPUT:
// - nMcuX				= MCU x coordinate
// - nMcuY				= MCU y coordinate
// - nVal				= DC value
//
void CimgDecode::PrintDcCumVal(unsigned nMcuX,unsigned nMcuY,int nVal)
{
	nMcuX;	// Unreferenced param
	nMcuY;	// Unreferenced param
	CString strTmp;
//	strTmp.Format(_T("  MCU [%4u,%4u] DC Cumulative Val = [%5d]"),nMcuX,nMcuY,nVal);
	strTmp.Format(_T("                 Cumulative DC Val=[%5d]"),nVal);
	m_pLog->AddLine(strTmp);
}


// Reset the DC values in the decoder (e.g. at start and
// after restart markers)
//
// POST:
// - m_nDcLum
// - m_nDcChrCb
// - m_nDcChrCr
// - m_anDcLumCss[]
// - m_anDcChrCbCss[]
// - m_anDcChrCrCss[]
//
void CimgDecode::DecodeRestartDcState()
{
	m_nDcLum = 0;
	m_nDcChrCb = 0;
	m_nDcChrCr = 0;
	for (unsigned nInd=0;nInd<MAX_SAMP_FACT_V*MAX_SAMP_FACT_H;nInd++) {
		m_anDcLumCss[nInd] = 0;
		m_anDcChrCbCss[nInd] = 0;
		m_anDcChrCrCss[nInd] = 0;
	}
}

//TODO
void CimgDecode::SetImageDimensions(unsigned nWidth,unsigned nHeight)
{
	m_rectImgBase = CRect(CPoint(0,0),CSize(nWidth,nHeight));
}


// Process the entire scan segment and optionally render the image
// - Reset and clear the output structures
// - Loop through each MCU and read each component
// - Maintain running DC level accumulator
// - Call SetFullRes() to transfer IDCT output to YCC Pixel Map
//
// INPUT:
// - nStart					= File position at start of scan
// - bDisplay				= Generate a preview image?
// - bQuiet					= Disable output of certain messages during decode?
//
void CimgDecode::DecodeScanImg(unsigned nStart,bool bDisplay,bool bQuiet)
{
	CString		strTmp;
	bool		bDieOnFirstErr = false; // FIXME: do we want this? It makes it less useful for corrupt jpegs


	// Fetch configuration values locally
	bool		bDumpHistoY		= m_pAppConfig->bDumpHistoY;
	bool		bDecodeScanAc;
	unsigned	nScanErrMax		= m_pAppConfig->nErrMaxDecodeScan;

	// Add some extra speed-up in hidden mode (we don't need AC)
	if (bDisplay) {
		bDecodeScanAc = m_pAppConfig->bDecodeScanImgAc;
	} else {
		bDecodeScanAc = false;
	}
	m_bHistEn		= m_pAppConfig->bHistoEn;
	m_bStatClipEn	= m_pAppConfig->bStatClipEn;


	unsigned	nPixMapW = 0;
	unsigned	nPixMapH = 0;

	// Reset the decoder state variables
	Reset();

	m_nScanErrMax = nScanErrMax;
	m_bDecodeScanAc = bDecodeScanAc;

	// Detect the scenario where the image component details haven't been set yet
	// The image details are set via SetImageDetails()
	if (!m_bImgDetailsSet) {
		m_pLog->AddLineErr(_T("*** ERROR: Decoding image before Image components defined ***"));
		return;
	}



	// Even though we support decoding of MAX_SOS_COMP_NS we limit
	// the component flexibility further
	if ( (m_nNumSosComps != NUM_CHAN_GRAYSCALE) && (m_nNumSosComps != NUM_CHAN_YCC) ) {
		strTmp.Format(_T("  NOTE: Number of SOS components not supported [%u]"),m_nNumSosComps);
		m_pLog->AddLineWarn(strTmp);
#ifndef DEBUG_YCCK
		return;
#endif
	}

	// Determine the maximum sampling factor and min sampling factor for this scan
	m_nSosSampFactHMax = 0;
	m_nSosSampFactVMax = 0;
	m_nSosSampFactHMin = 0xFF;
	m_nSosSampFactVMin = 0xFF;

	for (unsigned nComp=1;nComp<=m_nNumSosComps;nComp++) {
		m_nSosSampFactHMax = max(m_nSosSampFactHMax,m_anSofSampFactH[nComp]);
		m_nSosSampFactVMax = max(m_nSosSampFactVMax,m_anSofSampFactV[nComp]);
		m_nSosSampFactHMin = min(m_nSosSampFactHMin,m_anSofSampFactH[nComp]);
		m_nSosSampFactVMin = min(m_nSosSampFactVMin,m_anSofSampFactV[nComp]);
		ASSERT(m_nSosSampFactHMin != 0);
		ASSERT(m_nSosSampFactVMin != 0);
	}


	// ITU-T.81 clause A.2.2 "Non-interleaved order (Ns=1)"
	// - In some cases an image may have a single component in a scan but with sampling factors other than 1:
	//     Number of Img components = 1
	//       Component[1]: ID=0x01, Samp Fac=0x22 (Subsamp 1 x 1), Quant Tbl Sel=0x00 (Lum: Y)
	// - This could either be in a 3-component SOF with multiple 1-component SOS or a 1-component SOF (monochrome image)
	// - In general, grayscale images exhibit a sampling factor of 0x11
	// - Per ITU-T.81 A.2.2:
	//     When Ns = 1 (where Ns is the number of components in a scan), the order of data units
	//     within a scan shall be left-to-right and top-to-bottom, as shown in Figure A.2. This
	//     ordering applies whenever Ns = 1, regardless of the values of H1 and V1.
	// - Thus, instead of the usual decoding sequence for 0x22:
	//   [ 0 1 ] [ 4 5 ]
	//   [ 2 3 ] [ 6 7 ]
	// - The sequence for decode should be:
	//   [ 0 ] [ 1 ] [ 2 ] [ 3 ] [ 4 ] ...
	// - Which is equivalent to the non-subsampled ordering (ie. 0x11)
	// - Apply a correction for such images to remove the sampling factor
	if ( m_nNumSosComps == 1) {
		// TODO: Need to confirm if component index needs to be looked up
		// in the case of multiple SOS or if [1] is the correct index
		if ( (m_anSofSampFactH[1] != 1) || (m_anSofSampFactV[1] != 1) ) {
			m_pLog->AddLineWarn(_T("    Altering sampling factor for single component scan to 0x11"));
		}
		m_anSofSampFactH[1] = 1;
		m_anSofSampFactV[1] = 1;
		m_nSosSampFactHMax = 1;
		m_nSosSampFactVMax = 1;
		m_nSosSampFactHMin = 1;
		m_nSosSampFactVMin = 1;
	}


	// Perform additional range checks
	if ( (m_nSosSampFactHMax==0) || (m_nSosSampFactVMax==0) || (m_nSosSampFactHMax>MAX_SAMP_FACT_H) || (m_nSosSampFactVMax>MAX_SAMP_FACT_V)) {
		strTmp.Format(_T("  NOTE: Degree of subsampling factor not supported [HMax=%u, VMax=%u]"),m_nSosSampFactHMax,m_nSosSampFactVMax);
		m_pLog->AddLineWarn(strTmp);
		return;
	}

	// Calculate the MCU size for this scan. We do it here rather
	// than at the time of SOF (ie. SetImageDetails) for the reason
	// that under some circumstances we need to override the sampling
	// factor in single-component scans. This is done earlier.
	m_nMcuWidth = m_nSosSampFactHMax * BLK_SZ_X;
	m_nMcuHeight = m_nSosSampFactVMax * BLK_SZ_Y;


	// Calculate the number of bits to replicate when we generate the pixel map
	for (unsigned nComp=1;nComp<=m_nNumSosComps;nComp++) {
		m_anExpandBitsMcuH[nComp] = m_nSosSampFactHMax / m_anSofSampFactH[nComp];
		m_anExpandBitsMcuV[nComp] = m_nSosSampFactVMax / m_anSofSampFactV[nComp];
	}

	// Calculate the number of component samples per MCU
	for (unsigned nComp=1;nComp<=m_nNumSosComps;nComp++) {
		m_anSampPerMcuH[nComp] = m_anSofSampFactH[nComp];
		m_anSampPerMcuV[nComp] = m_anSofSampFactV[nComp];
	}


	// Determine the MCU ranges
	m_nMcuXMax   = (m_nDimX/m_nMcuWidth);
	m_nMcuYMax   = (m_nDimY/m_nMcuHeight);

	m_nImgSizeXPartMcu = m_nMcuXMax * m_nMcuWidth;
	m_nImgSizeYPartMcu = m_nMcuYMax * m_nMcuHeight;

	// Detect incomplete (partial) MCUs and round-up the MCU
	// ranges if necessary.
	if ((m_nDimX%m_nMcuWidth) != 0) m_nMcuXMax++;
	if ((m_nDimY%m_nMcuHeight) != 0) m_nMcuYMax++;

	// Save the maximum 8x8 block dimensions
	m_nBlkXMax = m_nMcuXMax * m_nSosSampFactHMax;
	m_nBlkYMax = m_nMcuYMax * m_nSosSampFactVMax;


	// Ensure the image has a size
	if ( (m_nBlkXMax == 0) || (m_nBlkYMax == 0) ) {
		return;
	}

	// Set the decoded size and before scaling
	m_nImgSizeX = m_nMcuXMax * m_nMcuWidth;
	m_nImgSizeY = m_nMcuYMax * m_nMcuHeight;

	m_rectImgBase = CRect(CPoint(0,0),CSize(m_nImgSizeX,m_nImgSizeY));


	// Determine decoding range
	unsigned	nDecMcuRowStart;
	unsigned	nDecMcuRowEnd;		// End to AC scan decoding
	unsigned	nDecMcuRowEndFinal; // End to general decoding
	nDecMcuRowStart = 0;
	nDecMcuRowEnd = m_nMcuYMax;
	nDecMcuRowEndFinal = m_nMcuYMax;

	// Limit the decoding range to valid range
	nDecMcuRowEnd = min(nDecMcuRowEnd,m_nMcuYMax);
	nDecMcuRowEndFinal = min(nDecMcuRowEndFinal,m_nMcuYMax);


	// Allocate the MCU File Map
	ASSERT(m_pMcuFileMap == NULL);
	m_pMcuFileMap = new unsigned[m_nMcuYMax*m_nMcuXMax];
	if (!m_pMcuFileMap) {
		strTmp = _T("ERROR: Not enough memory for Image Decoder MCU File Pos Map");
		m_pLog->AddLineErr(strTmp);
		if (m_pAppConfig->bInteractive)
			AfxMessageBox(strTmp);
		return;
	}
	memset(m_pMcuFileMap, 0, (m_nMcuYMax*m_nMcuXMax*sizeof(unsigned)) );


	// Allocate the 8x8 Block DC Map
	m_pBlkDcValY  = new short[m_nBlkYMax*m_nBlkXMax];
	if ( (!m_pBlkDcValY) ) {
		strTmp = _T("ERROR: Not enough memory for Image Decoder Blk DC Value Map");
		m_pLog->AddLineErr(strTmp);
		if (m_pAppConfig->bInteractive)
			AfxMessageBox(strTmp);
		return;
	}
	if (m_nNumSosComps == NUM_CHAN_YCC) {
		m_pBlkDcValCb = new short[m_nBlkYMax*m_nBlkXMax];
		m_pBlkDcValCr = new short[m_nBlkYMax*m_nBlkXMax];
		if ( (!m_pBlkDcValCb) || (!m_pBlkDcValCr) ) {
			strTmp = _T("ERROR: Not enough memory for Image Decoder Blk DC Value Map");
			m_pLog->AddLineErr(strTmp);
			if (m_pAppConfig->bInteractive)
				AfxMessageBox(strTmp);
			return;
		}
	}

	memset(m_pBlkDcValY,  0, (m_nBlkYMax*m_nBlkXMax*sizeof(short)) );
	if (m_nNumSosComps == NUM_CHAN_YCC) {
		memset(m_pBlkDcValCb, 0, (m_nBlkYMax*m_nBlkXMax*sizeof(short)) );
		memset(m_pBlkDcValCr, 0, (m_nBlkYMax*m_nBlkXMax*sizeof(short)) );
	}

	// Allocate the real YCC pixel Map
	nPixMapH = m_nBlkYMax*BLK_SZ_Y;
	nPixMapW = m_nBlkXMax*BLK_SZ_X;

	// Ensure no image allocated yet
	ASSERT(m_pPixValY==NULL);
	if (m_nNumSosComps == NUM_CHAN_YCC) {
		ASSERT(m_pPixValCb==NULL);
		ASSERT(m_pPixValCr==NULL);
	}


	// Allocate image (YCC)
	m_pPixValY  = new short[nPixMapW * nPixMapH];
	if ( (!m_pPixValY) ) {
		strTmp = _T("ERROR: Not enough memory for Image Decoder Pixel YCC Value Map");
		m_pLog->AddLineErr(strTmp);
		if (m_pAppConfig->bInteractive)
			AfxMessageBox(strTmp);
		return;
	}
	if (m_nNumSosComps == NUM_CHAN_YCC) {
		m_pPixValCb = new short[nPixMapW * nPixMapH];
		m_pPixValCr = new short[nPixMapW * nPixMapH];
		if ( (!m_pPixValCb) || (!m_pPixValCr) ) {
			strTmp = _T("ERROR: Not enough memory for Image Decoder Pixel YCC Value Map");
			m_pLog->AddLineErr(strTmp);
			if (m_pAppConfig->bInteractive)
				AfxMessageBox(strTmp);
			return;
		}
	}

	// Reset pixel map
	if (bDisplay) {
		ClrFullRes(nPixMapW,nPixMapH);
	}


	// -------------------------------------
	// Allocate the device-independent bitmap (DIB)

	unsigned char *		pDibImgTmpBits = NULL;
	unsigned			nDibImgRowBytes;

	// If a previous bitmap was created, deallocate it and start fresh
	m_pDibTemp.Kill();
	m_bDibTempReady = false;
	m_bPreviewIsJpeg = false;

	// Create the DIB
	// Although we are creating a 32-bit DIB, it should also
	// work to run in 16- and 8-bit modes

	bool bDoImage = false;	// Are we safe to set bits?

	if (bDisplay) {
		m_pDibTemp.CreateDIB(m_nImgSizeX,m_nImgSizeY,32);
		nDibImgRowBytes = m_nImgSizeX * sizeof(RGBQUAD);
		pDibImgTmpBits = (unsigned char*) ( m_pDibTemp.GetDIBBitArray() );

		if (pDibImgTmpBits) {
			bDoImage = true;
		} else {
			// TODO: Should we be exiting here instead?
		}

	}
	// -------------------------------------


	CString picStr;

	// Reset the DC cumulative state
	DecodeRestartDcState();

	// Reset the scan buffer
	DecodeRestartScanBuf(nStart,false);

	// Load the buffer
	m_pWBuf->BufLoadWindow(nStart);

	// Expect that we start with RST0
	m_nRestartExpectInd = 0;
	m_nRestartLastInd = 0;

	// Load the first data into the scan buffer
	// This is done so that the MCU map will already
	// have access to the data.
	BuffTopup();

	if (!bQuiet) {
		m_pLog->AddLineHdr(_T("*** Decoding SCAN Data ***"));
		strTmp.Format(_T("  OFFSET: 0x%08X"),nStart);
		m_pLog->AddLine(strTmp);
	}


	// TODO: Might be more appropriate to check against m_nNumSosComps instead?
	if ( (m_nNumSofComps != NUM_CHAN_GRAYSCALE) && (m_nNumSofComps != NUM_CHAN_YCC) ) {
		strTmp.Format(_T("  NOTE: Number of Image Components not supported [%u]"),m_nNumSofComps);
		m_pLog->AddLineWarn(strTmp);
#ifndef DEBUG_YCCK
		return;
#endif
	}

	// Check DQT tables
	// We need to ensure that the DQT Table selection has already
	// been done (via a call from JfifDec to SetDqtTables() ).
	unsigned	nDqtTblY  =0;
	unsigned	nDqtTblCr =0;
	unsigned	nDqtTblCb =0;
#ifdef DEBUG_YCCK
	unsigned	nDqtTblK =0;
#endif
	bool		bDqtReady = true;
	for (unsigned ind=1;ind<=m_nNumSosComps;ind++) {
		if (m_anDqtTblSel[ind]<0) bDqtReady = false;
	}
	if (!bDqtReady)
	{
		m_pLog->AddLineErr(_T("*** ERROR: Decoding image before DQT Table Selection via JFIF_SOF ***"));
		// TODO: Is more error handling required?
		return;
	} else {
		// FIXME: Not sure that we can always depend on the indices to appear
		// in this order. May need another layer of indirection to get at the
		// frame image component index.
		nDqtTblY  = m_anDqtTblSel[DQT_DEST_Y];
		nDqtTblCb = m_anDqtTblSel[DQT_DEST_CB];
		nDqtTblCr = m_anDqtTblSel[DQT_DEST_CR];
#ifdef DEBUG_YCCK
		if (m_nNumSosComps==4) {
			nDqtTblK = m_anDqtTblSel[DQT_DEST_K];
		}
#endif
	}

	// Now check DHT tables
	bool bDhtReady = true;
	unsigned nDhtTblDcY,nDhtTblDcCb,nDhtTblDcCr;
	unsigned nDhtTblAcY,nDhtTblAcCb,nDhtTblAcCr;
#ifdef DEBUG_YCCK
	unsigned nDhtTblDcK,nDhtTblAcK;
#endif
	for (unsigned nClass=DHT_CLASS_DC;nClass<=DHT_CLASS_AC;nClass++) {
		for (unsigned nCompInd=1;nCompInd<=m_nNumSosComps;nCompInd++) {
			if (m_anDhtTblSel[nClass][nCompInd]<0) bDhtReady = false;
		}
	}

	// Ensure that the table has been defined already!
	unsigned nSel;
	for (unsigned nCompInd=1;nCompInd<=m_nNumSosComps;nCompInd++) {
		// Check for DC DHT table
		nSel = m_anDhtTblSel[DHT_CLASS_DC][nCompInd];
		if (m_anDhtLookupSize[DHT_CLASS_DC][nSel] == 0) {
			bDhtReady = false;
		}
		// Check for AC DHT table
		nSel = m_anDhtTblSel[DHT_CLASS_AC][nCompInd];
		if (m_anDhtLookupSize[DHT_CLASS_AC][nSel] == 0) {
			bDhtReady = false;
		}
	}


	if (!bDhtReady)
	{
		m_pLog->AddLineErr(_T("*** ERROR: Decoding image before DHT Table Selection via JFIF_SOS ***"));
		// TODO: Is more error handling required here?
		return;
	} else {
		// Define the huffman table indices that are selected for each
		// image component index and class (AC,DC).
		//
		// No need to check if a table is valid here since we have
		// previously checked to ensure that all required tables
		// exist.
		// NOTE: If the table has not been defined, then the index
		// will be 0xFFFFFFFF. ReadScanVal() will trap this with ASSERT
		// should it ever be used.
		nDhtTblDcY  = m_anDhtTblSel[DHT_CLASS_DC][COMP_IND_YCC_Y];
		nDhtTblAcY  = m_anDhtTblSel[DHT_CLASS_AC][COMP_IND_YCC_Y];
		nDhtTblDcCb = m_anDhtTblSel[DHT_CLASS_DC][COMP_IND_YCC_CB];
		nDhtTblAcCb = m_anDhtTblSel[DHT_CLASS_AC][COMP_IND_YCC_CB];
		nDhtTblDcCr = m_anDhtTblSel[DHT_CLASS_DC][COMP_IND_YCC_CR];
		nDhtTblAcCr = m_anDhtTblSel[DHT_CLASS_AC][COMP_IND_YCC_CR];
#ifdef DEBUG_YCCK
		nDhtTblDcK	= m_anDhtTblSel[DHT_CLASS_DC][COMP_IND_YCC_K];
		nDhtTblAcK	= m_anDhtTblSel[DHT_CLASS_AC][COMP_IND_YCC_K];
#endif
	}

	// Done checks


	// Inform if they are in AC+DC/DC mode
	if (!bQuiet) {
		if (m_bDecodeScanAc) {
			m_pLog->AddLine(_T("  Scan Decode Mode: Full IDCT (AC + DC)"));
		} else {
			m_pLog->AddLine(_T("  Scan Decode Mode: No IDCT (DC only)"));
			m_pLog->AddLineWarn(_T("    NOTE: Low-resolution DC component shown. Can decode full-res with [Options->Scan Segment->Full IDCT]"));
		}
		m_pLog->AddLine(_T(""));
	}

	// Report any Buffer overlays
	m_pWBuf->ReportOverlays(m_pLog);

	m_nNumPixels = 0;

	// Clear the histogram and color correction clipping stats
	if (bDisplay) {
		memset(&m_sStatClip,0,sizeof(m_sStatClip));
		memset(&m_sHisto,0,sizeof(m_sHisto));

		// FIXME: Histo should now be done after color convert
		memset(&m_anCcHisto_r,0,sizeof(m_anCcHisto_r));
		memset(&m_anCcHisto_g,0,sizeof(m_anCcHisto_g));
		memset(&m_anCcHisto_b,0,sizeof(m_anCcHisto_b));

		memset(&m_anHistoYFull,0,sizeof(m_anHistoYFull));
		memset(&m_anHistoYSubset,0,sizeof(m_anHistoYSubset));
	}



	// -----------------------------------------------------------------------
	// Process all scan MCUs
	// -----------------------------------------------------------------------

	for (unsigned nMcuY=nDecMcuRowStart;nMcuY<nDecMcuRowEndFinal;nMcuY++) {

		// Set the statusbar text to Processing...
		strTmp.Format(_T("Decoding Scan Data... Row %04u of %04u (%3.0f%%)"),nMcuY,m_nMcuYMax,nMcuY*100.0/m_nMcuYMax);
		SetStatusText(strTmp);

		// TODO: Trap escape keypress here (or run as thread)

		bool	bDscRet;	// Return value for DecodeScanComp()
		bool	bScanStop = false;
		for (unsigned nMcuX=0;(nMcuX<m_nMcuXMax)&&(!bScanStop);nMcuX++) {

			// Check to see if we should expect a restart marker!
			// FIXME: Should actually check to ensure that we do in
			// fact get a restart marker, and that it was the right
			// one!
			if ((m_bRestartEn) && (m_nRestartMcusLeft == 0)) {
				/*
				if (m_bVerbose) {
					strTmp.Format(_T("  Expect Restart interval elapsed @ %s"),GetScanBufPos());
					m_pLog->AddLine(strTmp);
				}
				*/
				if (m_bRestartRead) {
					/*
					// FIXME: Check for restart counter value match
					if (m_bVerbose) {
						strTmp.Format(_T("  Restart marker matched"));
						m_pLog->AddLine(strTmp);
					}
					*/
				} else {
					strTmp.Format(_T("  Expect Restart interval elapsed @ %s"),(LPCTSTR)GetScanBufPos());
					m_pLog->AddLine(strTmp);
						strTmp.Format(_T("    ERROR: Restart marker not detected"));
						m_pLog->AddLineErr(strTmp);
				}
				/*
				if (ExpectRestart()) {
					if (m_bVerbose) {
						strTmp.Format(_T("  Restart marker detected"));
						m_pLog->AddLine(strTmp);
					}
				} else {
					strTmp.Format(_T("  ERROR: Restart marker expected but not found @ %s"),GetScanBufPos());
					m_pLog->AddLineErr(strTmp);
				}
				*/


			}

			// To support a fast decode mode, allow for a subset of the
			// image to have DC+AC decoding, while the remainder is only DC decoding
			if ((nMcuY<nDecMcuRowStart) || (nMcuY>nDecMcuRowEnd)) {
				m_bDecodeScanAc = false;
			} else {
				m_bDecodeScanAc = bDecodeScanAc;
			}


			// Precalculate MCU matrix index
			unsigned nMcuXY = nMcuY*m_nMcuXMax+nMcuX;

			// Mark the start of the MCU in the file map
			m_pMcuFileMap[nMcuXY] = PackFileOffset(m_anScanBuffPtr_pos[0],m_nScanBuffPtr_align);

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
			// We store them all in an array m_anDcLumCss[]

			// Give separator line between MCUs
			if (bVlcDump) {
				m_pLog->AddLine(_T(""));
			}

			// CSS array indices
			unsigned	nCssIndH;
			unsigned	nCssIndV;
			unsigned	nComp;

			// No need to reset the IDCT output matrix for this MCU
			// since we are going to be generating it here. This help
			// maintain performance.

			// --------------------------------------------------------------
			nComp = SCAN_COMP_Y;

			// Step through the sampling factors per image component
			// TODO: Could rewrite this to use single loop across each image component
			for (nCssIndV=0;nCssIndV<m_anSampPerMcuV[nComp];nCssIndV++) {
				for (nCssIndH=0;nCssIndH<m_anSampPerMcuH[nComp];nCssIndH++) {

					if (!bVlcDump) {
						bDscRet = DecodeScanComp(nDhtTblDcY,nDhtTblAcY,nDqtTblY,nMcuX,nMcuY);// Lum DC+AC
					} else {
						bDscRet = DecodeScanCompPrint(nDhtTblDcY,nDhtTblAcY,nDqtTblY,nMcuX,nMcuY);// Lum DC+AC
					}
					if (m_nScanCurErr) CheckScanErrors(nMcuX,nMcuY,nCssIndH,nCssIndV,nComp);
					if (!bDscRet && bDieOnFirstErr) return;

					// The DCT Block matrix has already been dezigzagged
					// and multiplied against quantization table entry
					m_nDcLum += m_anDctBlock[DCT_COEFF_DC];

					if (bVlcDump) {
//						PrintDcCumVal(nMcuX,nMcuY,m_nDcLum);
					}

					// Now take a snapshot of the current cumulative DC value
					m_anDcLumCss[nCssIndV*MAX_SAMP_FACT_H+nCssIndH] = m_nDcLum;

					// At this point we have one of the luminance comps
					// fully decoded (with IDCT if enabled). The result is
					// currently in the array: m_afIdctBlock[]
					// The next step would be to move these elements into
					// the 3-channel MCU image map

					// Store the pixels associated with this channel into
					// the full-res pixel map. IDCT has already been computed
					// on the 8x8 (or larger) MCU block region.

#if 1
					if (bDisplay)
						SetFullRes(nMcuX,nMcuY,nComp,nCssIndH,nCssIndV,m_nDcLum);
#else
					// FIXME
					// Temporarily experiment with trying to handle multiple scans
					// by converting sampling factor of luminance scan back to 1x1
					unsigned nNewMcuX,nNewMcuY,nNewCssX,nNewCssY;
					if (nCssIndV == 0) {
						if (nMcuX < m_nMcuXMax/2) {
							nNewMcuX = nMcuX;
							nNewMcuY = nMcuY;
							nNewCssY = 0;
						} else {
							nNewMcuX = nMcuX - (m_nMcuXMax/2);
							nNewMcuY = nMcuY;
							nNewCssY = 1;
						}
						nNewCssX = nCssIndH;
						SetFullRes(nNewMcuX,nNewMcuY,SCAN_COMP_Y,nNewCssX,nNewCssY,m_nDcLum);
					} else {
						nNewMcuX = (nMcuX / 2) + 1;
						nNewMcuY = (nMcuY / 2);
						nNewCssX = nCssIndH;
						nNewCssY = nMcuY % 2;
					}
#endif

					// ---------------

					// TODO: Counting pixels makes assumption that luminance is
					// not subsampled, so we increment by 64.
					m_nNumPixels += BLK_SZ_X*BLK_SZ_Y;

				}
			}

			// In a grayscale image, we don't do this part!
			//if (m_nNumSofComps == NUM_CHAN_YCC) {
			if (m_nNumSosComps == NUM_CHAN_YCC) {

				// --------------------------------------------------------------
				nComp = SCAN_COMP_CB;

				// Chrominance Cb
				for (nCssIndV=0;nCssIndV<m_anSampPerMcuV[nComp];nCssIndV++) {
					for (nCssIndH=0;nCssIndH<m_anSampPerMcuH[nComp];nCssIndH++) {

						if (!bVlcDump) {
							bDscRet = DecodeScanComp(nDhtTblDcCb,nDhtTblAcCb,nDqtTblCb,nMcuX,nMcuY);// Chr Cb DC+AC
						} else {
							bDscRet = DecodeScanCompPrint(nDhtTblDcCb,nDhtTblAcCb,nDqtTblCb,nMcuX,nMcuY);// Chr Cb DC+AC
						}
						if (m_nScanCurErr) CheckScanErrors(nMcuX,nMcuY,nCssIndH,nCssIndV,nComp);
						if (!bDscRet && bDieOnFirstErr) return;

						m_nDcChrCb += m_anDctBlock[DCT_COEFF_DC];


						if (bVlcDump) {
							//PrintDcCumVal(nMcuX,nMcuY,m_nDcChrCb);
						}

						// Now take a snapshot of the current cumulative DC value
						m_anDcChrCbCss[nCssIndV*MAX_SAMP_FACT_H+nCssIndH] = m_nDcChrCb;

						// Store fullres value
						if (bDisplay)
							SetFullRes(nMcuX,nMcuY,nComp,nCssIndH,nCssIndV,m_nDcChrCb);

					}
				}

				// --------------------------------------------------------------
				nComp = SCAN_COMP_CR;

				// Chrominance Cr
				for (nCssIndV=0;nCssIndV<m_anSampPerMcuV[nComp];nCssIndV++) {
					for (nCssIndH=0;nCssIndH<m_anSampPerMcuH[nComp];nCssIndH++) {
						if (!bVlcDump) {
							bDscRet = DecodeScanComp(nDhtTblDcCr,nDhtTblAcCr,nDqtTblCr,nMcuX,nMcuY);// Chr Cr DC+AC
						} else {
							bDscRet = DecodeScanCompPrint(nDhtTblDcCr,nDhtTblAcCr,nDqtTblCr,nMcuX,nMcuY);// Chr Cr DC+AC
						}
						if (m_nScanCurErr) CheckScanErrors(nMcuX,nMcuY,nCssIndH,nCssIndV,nComp);
						if (!bDscRet && bDieOnFirstErr) return;

						m_nDcChrCr += m_anDctBlock[DCT_COEFF_DC];



						if (bVlcDump) {
							//PrintDcCumVal(nMcuX,nMcuY,m_nDcChrCr);
						}

						// Now take a snapshot of the current cumulative DC value
						m_anDcChrCrCss[nCssIndV*MAX_SAMP_FACT_H+nCssIndH] = m_nDcChrCr;

						// Store fullres value
						if (bDisplay)
							SetFullRes(nMcuX,nMcuY,nComp,nCssIndH,nCssIndV,m_nDcChrCr);

					}
				}


			}
#ifdef DEBUG_YCCK
			else if (m_nNumSosComps == NUM_CHAN_YCCK) {

				// --------------------------------------------------------------
				nComp = SCAN_COMP_CB;

				// Chrominance Cb
				for (nCssIndV=0;nCssIndV<m_anSampPerMcuV[nComp];nCssIndV++) {
					for (nCssIndH=0;nCssIndH<m_anSampPerMcuH[nComp];nCssIndH++) {

						if (!bVlcDump) {
							bDscRet = DecodeScanComp(nDhtTblDcCb,nDhtTblAcCb,nDqtTblCb,nMcuX,nMcuY);// Chr Cb DC+AC
						} else {
							bDscRet = DecodeScanCompPrint(nDhtTblDcCb,nDhtTblAcCb,nDqtTblCb,nMcuX,nMcuY);// Chr Cb DC+AC
						}
						if (m_nScanCurErr) CheckScanErrors(nMcuX,nMcuY,nCssIndH,nCssIndV,nComp);
						if (!bDscRet && bDieOnFirstErr) return;

						m_nDcChrCb += m_anDctBlock[DCT_COEFF_DC];


						if (bVlcDump) {
							//PrintDcCumVal(nMcuX,nMcuY,m_nDcChrCb);
						}

						// Now take a snapshot of the current cumulative DC value
						m_anDcChrCbCss[nCssIndV*MAX_SAMP_FACT_H+nCssIndH] = m_nDcChrCb;

						// Store fullres value
						if (bDisplay)
							SetFullRes(nMcuX,nMcuY,nComp,0,0,m_nDcChrCb);

					}
				}

				// --------------------------------------------------------------
				nComp = SCAN_COMP_CR;

				// Chrominance Cr
				for (nCssIndV=0;nCssIndV<m_anSampPerMcuV[nComp];nCssIndV++) {
					for (nCssIndH=0;nCssIndH<m_anSampPerMcuH[nComp];nCssIndH++) {
						if (!bVlcDump) {
							bDscRet = DecodeScanComp(nDhtTblDcCr,nDhtTblAcCr,nDqtTblCr,nMcuX,nMcuY);// Chr Cr DC+AC
						} else {
							bDscRet = DecodeScanCompPrint(nDhtTblDcCr,nDhtTblAcCr,nDqtTblCr,nMcuX,nMcuY);// Chr Cr DC+AC
						}
						if (m_nScanCurErr) CheckScanErrors(nMcuX,nMcuY,nCssIndH,nCssIndV,nComp);
						if (!bDscRet && bDieOnFirstErr) return;

						m_nDcChrCr += m_anDctBlock[DCT_COEFF_DC];



						if (bVlcDump) {
							//PrintDcCumVal(nMcuX,nMcuY,m_nDcChrCr);
						}

						// Now take a snapshot of the current cumulative DC value
						m_anDcChrCrCss[nCssIndV*MAX_SAMP_FACT_H+nCssIndH] = m_nDcChrCr;

						// Store fullres value
						if (bDisplay)
							SetFullRes(nMcuX,nMcuY,nComp,0,0,m_nDcChrCr);

					}
				}

				// --------------------------------------------------------------
				// IGNORED
				nComp = SCAN_COMP_K;

				// Black K
				for (nCssIndV=0;nCssIndV<m_anSampPerMcuV[nComp];nCssIndV++) {
					for (nCssIndH=0;nCssIndH<m_anSampPerMcuH[nComp];nCssIndH++) {

						if (!bVlcDump) {
							bDscRet = DecodeScanComp(nDhtTblDcK,nDhtTblAcK,nDqtTblK,nMcuX,nMcuY);// K DC+AC
						} else {
							bDscRet = DecodeScanCompPrint(nDhtTblDcK,nDhtTblAcK,nDqtTblK,nMcuX,nMcuY);// K DC+AC
						}
						if (m_nScanCurErr) CheckScanErrors(nMcuX,nMcuY,nCssIndH,nCssIndV,nComp);
						if (!bDscRet && bDieOnFirstErr) return;

/*
						m_nDcChrK += m_anDctBlock[DCT_COEFF_DC];


						if (bVlcDump) {
							//PrintDcCumVal(nMcuX,nMcuY,m_nDcChrCb);
						}

						// Now take a snapshot of the current cumulative DC value
						m_anDcChrKCss[nCssIndV*MAX_SAMP_FACT_H+nCssIndH] = m_nDcChrK;

						// Store fullres value
						if (bDisplay)
							SetFullRes(nMcuX,nMcuY,nComp,0,0,m_nDcChrK);
*/

					}
				}


			}
#endif

			// --------------------------------------------------------------------

			unsigned	nBlkXY;

			// Now save the DC YCC values (expanded per 8x8 block)
			// without ranging or translation into RGB.
			//
			// We enter this code once per MCU so we need to expand
			// out to cover all blocks in this MCU.


			// --------------------------------------------------------------
			nComp = SCAN_COMP_Y;

			// Calculate top-left corner of MCU in block map
			// and then linear offset into block map
			unsigned   nBlkCornerMcuX,nBlkCornerMcuY,nBlkCornerMcuLinear;
			nBlkCornerMcuX = nMcuX * m_anExpandBitsMcuH[nComp];
			nBlkCornerMcuY = nMcuY * m_anExpandBitsMcuV[nComp];
			nBlkCornerMcuLinear = (nBlkCornerMcuY * m_nBlkXMax) + nBlkCornerMcuX;

			// Now step through each block in the MCU per subsampling
			for (nCssIndV=0;nCssIndV<m_anSampPerMcuV[nComp];nCssIndV++) {
				for (nCssIndH=0;nCssIndH<m_anSampPerMcuH[nComp];nCssIndH++) {
					// Calculate upper-left Blk index
					// FIXME: According to code analysis the following write assignment
					// to m_pBlkDcValY[] can apparently exceed the buffer bounds (C6386).
					// I have not yet determined what scenario can lead to
					// this. So for now, add in specific clause to trap and avoid.
					nBlkXY = nBlkCornerMcuLinear + (nCssIndV * m_nBlkXMax) + nCssIndH;
					// FIXME: Temporarily catch any range issue
					if (nBlkXY >= m_nBlkXMax*m_nBlkYMax) {
#ifdef DEBUG_LOG
					CString	strDebug;
					strTmp.Format(_T("DecodeScanImg() with nBlkXY out of range. nBlkXY=[%u] m_nBlkXMax=[%u] m_nBlkYMax=[%u]"),nBlkXY,m_nBlkXMax,m_nBlkYMax);
					strDebug.Format(_T("## File=[%-100s] Block=[%-10s] Error=[%s]\n"),(LPCTSTR)m_pAppConfig->strCurFname,
						_T("ImgDecode"),(LPCTSTR)strTmp);
					OutputDebugString(strDebug);
#else
					ASSERT(false);
#endif
					} else {
						m_pBlkDcValY [nBlkXY] = m_anDcLumCss[nCssIndV*MAX_SAMP_FACT_H+nCssIndH];
					}
				}
			}
			// Only process the chrominance if it is YCC
			if (m_nNumSosComps == NUM_CHAN_YCC) {

				// --------------------------------------------------------------
				nComp = SCAN_COMP_CB;

				for (nCssIndV=0;nCssIndV<m_anSampPerMcuV[nComp];nCssIndV++) {
					for (nCssIndH=0;nCssIndH<m_anSampPerMcuH[nComp];nCssIndH++) {
						// Calculate upper-left Blk index
						nBlkXY = (nMcuY*m_anExpandBitsMcuV[nComp] + nCssIndV)*m_nBlkXMax + (nMcuX*m_anExpandBitsMcuH[nComp] + nCssIndH);
						// FIXME: Temporarily catch any range issue
						if (nBlkXY >= m_nBlkXMax*m_nBlkYMax) {
#ifdef DEBUG_LOG
							CString	strDebug;
							strTmp.Format(_T("DecodeScanImg() with nBlkXY out of range. nBlkXY=[%u] m_nBlkXMax=[%u] m_nBlkYMax=[%u]"),nBlkXY,m_nBlkXMax,m_nBlkYMax);
							strDebug.Format(_T("## File=[%-100s] Block=[%-10s] Error=[%s]\n"),(LPCTSTR)m_pAppConfig->strCurFname,
								_T("ImgDecode"),(LPCTSTR)strTmp);
							OutputDebugString(strDebug);
#else
							ASSERT(false);
#endif
						} else {
							m_pBlkDcValCb [nBlkXY] = m_anDcChrCbCss[nCssIndV*MAX_SAMP_FACT_H+nCssIndH];
						}
					}
				}

				// --------------------------------------------------------------
				nComp = SCAN_COMP_CR;

				for (nCssIndV=0;nCssIndV<m_anSampPerMcuV[nComp];nCssIndV++) {
					for (nCssIndH=0;nCssIndH<m_anSampPerMcuH[nComp];nCssIndH++) {
						// Calculate upper-left Blk index
						nBlkXY = (nMcuY*m_anExpandBitsMcuV[nComp] + nCssIndV)*m_nBlkXMax + (nMcuX*m_anExpandBitsMcuH[nComp] + nCssIndH);
						// FIXME: Temporarily catch any range issue
						if (nBlkXY >= m_nBlkXMax*m_nBlkYMax) {
#ifdef DEBUG_LOG
							CString	strDebug;
							strTmp.Format(_T("DecodeScanImg() with nBlkXY out of range. nBlkXY=[%u] m_nBlkXMax=[%u] m_nBlkYMax=[%u]"),nBlkXY,m_nBlkXMax,m_nBlkYMax);
							strDebug.Format(_T("## File=[%-100s] Block=[%-10s] Error=[%s]\n"),(LPCTSTR)m_pAppConfig->strCurFname,
								_T("ImgDecode"),(LPCTSTR)strTmp);
							OutputDebugString(strDebug);
#else
							ASSERT(false);
#endif
						} else {
							m_pBlkDcValCr [nBlkXY] = m_anDcChrCrCss[nCssIndV*MAX_SAMP_FACT_H+nCssIndH];
						}
					}
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

		} // nMcuX


	} // nMcuY
	if (!bQuiet) {
		m_pLog->AddLine(_T(""));
	}

	// ---------------------------------------------------------

	// Now we can create the final preview. Since we have just finished
	// decoding a new image, we need to ensure that we invalidate
	// the temporary preview (multi-channel option). Done earlier
	// with PREVIEW_NONE
	if (bDisplay) {
		CalcChannelPreview();
	}

	// DIB is ready for display now
	if (bDisplay) {
		m_bDibTempReady = true;
		m_bPreviewIsJpeg = true;
	}


	// ------------------------------------
	// Report statistics

	if (!bQuiet) {

		// Report Compression stats
		// TODO: Should we use m_nNumSofComps?
		strTmp.Format(_T("  Compression stats:"));
		m_pLog->AddLine(strTmp);
		float nCompressionRatio = (float)(m_nDimX*m_nDimY*m_nNumSosComps*8) / (float)((m_anScanBuffPtr_pos[0]-m_nScanBuffPtr_first)*8);
		strTmp.Format(_T("    Compression Ratio: %5.2f:1"),nCompressionRatio);
		m_pLog->AddLine(strTmp);
		float nBitsPerPixel = (float)((m_anScanBuffPtr_pos[0]-m_nScanBuffPtr_first)*8) / (float)(m_nDimX*m_nDimY);
		strTmp.Format(_T("    Bits per pixel:    %5.2f:1"),nBitsPerPixel);
		m_pLog->AddLine(strTmp);
		m_pLog->AddLine(_T(""));


		// Report Huffman stats
		strTmp.Format(_T("  Huffman code histogram stats:"));
		m_pLog->AddLine(strTmp);

		unsigned nDhtHistoTotal;
		for (unsigned nClass=DHT_CLASS_DC;nClass<=DHT_CLASS_AC;nClass++) {
			for (unsigned nDhtDestId=0;nDhtDestId<=m_anDhtLookupSetMax[nClass];nDhtDestId++) {
				nDhtHistoTotal = 0;
				for (unsigned nBitLen=1;nBitLen<=MAX_DHT_CODELEN;nBitLen++) {
					nDhtHistoTotal += m_anDhtHisto[nClass][nDhtDestId][nBitLen];
				}

				strTmp.Format(_T("    Huffman Table: (Dest ID: %u, Class: %s)"),nDhtDestId,(nClass?_T("AC"):_T("DC")));
				m_pLog->AddLine(strTmp);
				for (unsigned nBitLen=1;nBitLen<=MAX_DHT_CODELEN;nBitLen++) {
					strTmp.Format(_T("      # codes of length %02u bits: %8u (%3.0f%%)"),
						nBitLen,m_anDhtHisto[nClass][nDhtDestId][nBitLen],(m_anDhtHisto[nClass][nDhtDestId][nBitLen]*100.0)/nDhtHistoTotal);
					m_pLog->AddLine(strTmp);
				}
				m_pLog->AddLine(_T(""));
			}
		}

		// Report YCC stats
		ReportColorStats();

	}	// !bQuiet

	// ------------------------------------

	// Display the image histogram if enabled
	if (bDisplay && m_bHistEn) {
		DrawHistogram(bQuiet,bDumpHistoY);
	}

	if (bDisplay && m_bAvgYValid) {
		m_pLog->AddLine(_T("  Average Pixel Luminance (Y):"));
		strTmp.Format(_T("    Y=[%3u] (range: 0..255)"),
			m_nAvgY);
		m_pLog->AddLine(strTmp);
		m_pLog->AddLine(_T(""));
	}

	if (bDisplay && m_bBrightValid) {
		m_pLog->AddLine(_T("  Brightest Pixel Search:"));
		strTmp.Format(_T("    YCC=[%5d,%5d,%5d] RGB=[%3u,%3u,%3u] @ MCU[%3u,%3u]"),
			m_nBrightY,m_nBrightCb,m_nBrightCr,m_nBrightR,m_nBrightG,m_nBrightB,
			m_ptBrightMcu.x,m_ptBrightMcu.y);
		m_pLog->AddLine(strTmp);
		m_pLog->AddLine(_T(""));
	}


	// --------------------------------------

	if (!bQuiet) {
		m_pLog->AddLine(_T("  Finished Decoding SCAN Data"));
		strTmp.Format(_T("    Number of RESTART markers decoded: %u"),m_nRestartRead);
		m_pLog->AddLine(strTmp);
		strTmp.Format(_T("    Next position in scan buffer: Offset %s"),(LPCTSTR)GetScanBufPos());
		m_pLog->AddLine(strTmp);

		m_pLog->AddLine(_T(""));
	}

	// --------------------------------------
	// Write out the full Y histogram if requested!

	CString strFull;

	if (bDisplay && m_bHistEn && bDumpHistoY) {
		ReportHistogramY();
	}


}

//
// Report if image preview is ready to display
//
// RETURN:
// - True if image preview is ready
//
bool CimgDecode::IsPreviewReady()
{
	return m_bPreviewIsJpeg;
}

// Report out the color conversion statistics
//
// PRE:
// - m_sStatClip
// - m_sHisto
//
void CimgDecode::ReportColorStats()
{
	CString		strTmp;

	// Report YCC stats
	if (CC_CLIP_YCC_EN) {
		strTmp.Format(_T("  YCC clipping in DC:"));
		m_pLog->AddLine(strTmp);
		strTmp.Format(_T("    Y  component: [<0=%5u] [>255=%5u]"),m_sStatClip.nClipYUnder,m_sStatClip.nClipYOver);
		m_pLog->AddLine(strTmp);
		strTmp.Format(_T("    Cb component: [<0=%5u] [>255=%5u]"),m_sStatClip.nClipCbUnder,m_sStatClip.nClipCbOver);
		m_pLog->AddLine(strTmp);
		strTmp.Format(_T("    Cr component: [<0=%5u] [>255=%5u]"),m_sStatClip.nClipCrUnder,m_sStatClip.nClipCrOver);
		m_pLog->AddLine(strTmp);
		m_pLog->AddLine(_T(""));
	}

	if (m_bHistEn) {

		strTmp.Format(_T("  YCC histogram in DC (DCT sums : pre-ranged:"));
		m_pLog->AddLine(strTmp);
		strTmp.Format(_T("    Y  component histo: [min=%5d max=%5d avg=%7.1f]"),
			m_sHisto.nPreclipYMin,m_sHisto.nPreclipYMax,(float)m_sHisto.nPreclipYSum / (float)m_sHisto.nCount);
		m_pLog->AddLine(strTmp);
		strTmp.Format(_T("    Cb component histo: [min=%5d max=%5d avg=%7.1f]"),
			m_sHisto.nPreclipCbMin,m_sHisto.nPreclipCbMax,(float)m_sHisto.nPreclipCbSum / (float)m_sHisto.nCount);
		m_pLog->AddLine(strTmp);
		strTmp.Format(_T("    Cr component histo: [min=%5d max=%5d avg=%7.1f]"),
			m_sHisto.nPreclipCrMin,m_sHisto.nPreclipCrMax,(float)m_sHisto.nPreclipCrSum / (float)m_sHisto.nCount);
		m_pLog->AddLine(strTmp);
		m_pLog->AddLine(_T(""));

		strTmp.Format(_T("  YCC histogram in DC:"));
		m_pLog->AddLine(strTmp);
		strTmp.Format(_T("    Y  component histo: [min=%5d max=%5d avg=%7.1f]"),
			m_sHisto.nClipYMin,m_sHisto.nClipYMax,(float)m_sHisto.nClipYSum / (float)m_sHisto.nCount);
		m_pLog->AddLine(strTmp);
		strTmp.Format(_T("    Cb component histo: [min=%5d max=%5d avg=%7.1f]"),
			m_sHisto.nClipCbMin,m_sHisto.nClipCbMax,(float)m_sHisto.nClipCbSum / (float)m_sHisto.nCount);
		m_pLog->AddLine(strTmp);
		strTmp.Format(_T("    Cr component histo: [min=%5d max=%5d avg=%7.1f]"),
			m_sHisto.nClipCrMin,m_sHisto.nClipCrMax,(float)m_sHisto.nClipCrSum / (float)m_sHisto.nCount);
		m_pLog->AddLine(strTmp);
		m_pLog->AddLine(_T(""));


		strTmp.Format(_T("  RGB histogram in DC (before clip):"));
		m_pLog->AddLine(strTmp);
		strTmp.Format(_T("    R  component histo: [min=%5d max=%5d avg=%7.1f]"),
			m_sHisto.nPreclipRMin,m_sHisto.nPreclipRMax,(float)m_sHisto.nPreclipRSum / (float)m_sHisto.nCount);
		m_pLog->AddLine(strTmp);
		strTmp.Format(_T("    G  component histo: [min=%5d max=%5d avg=%7.1f]"),
			m_sHisto.nPreclipGMin,m_sHisto.nPreclipGMax,(float)m_sHisto.nPreclipGSum / (float)m_sHisto.nCount);
		m_pLog->AddLine(strTmp);
		strTmp.Format(_T("    B  component histo: [min=%5d max=%5d avg=%7.1f]"),
			m_sHisto.nPreclipBMin,m_sHisto.nPreclipBMax,(float)m_sHisto.nPreclipBSum / (float)m_sHisto.nCount);
		m_pLog->AddLine(strTmp);
		m_pLog->AddLine(_T(""));
	}

	strTmp.Format(_T("  RGB clipping in DC:"));
	m_pLog->AddLine(strTmp);
	strTmp.Format(_T("    R  component: [<0=%5u] [>255=%5u]"),m_sStatClip.nClipRUnder,m_sStatClip.nClipROver);
	m_pLog->AddLine(strTmp);
	strTmp.Format(_T("    G  component: [<0=%5u] [>255=%5u]"),m_sStatClip.nClipGUnder,m_sStatClip.nClipGOver);
	m_pLog->AddLine(strTmp);
	strTmp.Format(_T("    B  component: [<0=%5u] [>255=%5u]"),m_sStatClip.nClipBUnder,m_sStatClip.nClipBOver);
	m_pLog->AddLine(strTmp);
	/*
	strTmp.Format(_T("    White Highlight:         [>255=%5u]"),m_sStatClip.nClipWhiteOver);
	m_pLog->AddLine(strTmp);
	*/
	m_pLog->AddLine(_T(""));
}


// Report the histogram stats from the Y component
//
// PRE:
// - m_anHistoYFull
//
void CimgDecode::ReportHistogramY()
{
	CString		strFull;
	CString		strTmp;

	m_pLog->AddLine(_T("  Y Histogram in DC: (DCT sums) Full"));
	for (unsigned row=0;row<2048/8;row++) {
		strFull.Format(_T("    Y=%5d..%5d: "),-1024+(row*8),-1024+(row*8)+7);
		for (unsigned col=0;col<8;col++) {
			strTmp.Format(_T("0x%06x, "),m_anHistoYFull[col+row*8]);
			strFull += strTmp;
		}
		m_pLog->AddLine(strFull);
	}
}


// Draw the histograms (RGB and/or Y)
//
// INPUT:
// - bQuiet					= Calculate stats without reporting to log?
// - bDumpHistoY			= Generate the Y histogram?
// PRE:
// - m_sHisto
//
void CimgDecode::DrawHistogram(bool bQuiet,bool bDumpHistoY)
{
	CString		strTmp;

	if (!bQuiet) {
		strTmp.Format(_T("  RGB histogram in DC (after clip):"));
		m_pLog->AddLine(strTmp);
		strTmp.Format(_T("    R  component histo: [min=%5d max=%5d avg=%7.1f]"),
			m_sHisto.nClipRMin,m_sHisto.nClipRMax,(float)m_sHisto.nClipRSum / (float)m_sHisto.nCount);
		m_pLog->AddLine(strTmp);
		strTmp.Format(_T("    G  component histo: [min=%5d max=%5d avg=%7.1f]"),
			m_sHisto.nClipGMin,m_sHisto.nClipGMax,(float)m_sHisto.nClipGSum / (float)m_sHisto.nCount);
		m_pLog->AddLine(strTmp);
		strTmp.Format(_T("    B  component histo: [min=%5d max=%5d avg=%7.1f]"),
			m_sHisto.nClipBMin,m_sHisto.nClipBMax,(float)m_sHisto.nClipBSum / (float)m_sHisto.nCount);
		m_pLog->AddLine(strTmp);
		m_pLog->AddLine(_T(""));
	}

	// --------------------------------------
	// Now draw the RGB histogram!

	unsigned		nCoordY;
	unsigned		nHistoBinHeight;
	unsigned		nHistoPeakVal;
	unsigned		nHistoX;
	unsigned		nHistoCurVal;
	unsigned		nDibHistoRowBytes;
	unsigned char*	pDibHistoBits;

	m_pDibHistRgb.Kill();
	m_bDibHistRgbReady = false;

	m_pDibHistRgb.CreateDIB(HISTO_BINS*HISTO_BIN_WIDTH,3*HISTO_BIN_HEIGHT_MAX,32);
	nDibHistoRowBytes = (HISTO_BINS*HISTO_BIN_WIDTH) * 4;
	pDibHistoBits = (unsigned char*) ( m_pDibHistRgb.GetDIBBitArray() );

	m_rectHistBase = CRect(CPoint(0,0),CSize(HISTO_BINS*HISTO_BIN_WIDTH,3*HISTO_BIN_HEIGHT_MAX));

	if (pDibHistoBits != NULL) {
		memset(pDibHistoBits,0,3*HISTO_BIN_HEIGHT_MAX*nDibHistoRowBytes);

		// Do peak detect first
		// Don't want to reset peak value to 0 as otherwise we might get
		// division by zero later when we calculate nHistoBinHeight
		nHistoPeakVal = 1;

		// Peak value is across all three channels!
		for (unsigned nHistChan=0;nHistChan<3;nHistChan++) {

			for (unsigned i=0;i<HISTO_BINS;i++) {
				if      (nHistChan == 0) {nHistoCurVal = m_anCcHisto_r[i];}
				else if (nHistChan == 1) {nHistoCurVal = m_anCcHisto_g[i];}
				else                     {nHistoCurVal = m_anCcHisto_b[i];}
				nHistoPeakVal = (nHistoCurVal > nHistoPeakVal)?nHistoCurVal:nHistoPeakVal;
			}
		}

		for (unsigned nHistChan=0;nHistChan<3;nHistChan++) {
			for (unsigned i=0;i<HISTO_BINS;i++) {

				// Calculate bin's height (max HISTO_BIN_HEIGHT_MAX)
				if      (nHistChan == 0) {nHistoCurVal = m_anCcHisto_r[i];}
				else if (nHistChan == 1) {nHistoCurVal = m_anCcHisto_g[i];}
				else                     {nHistoCurVal = m_anCcHisto_b[i];}
				nHistoBinHeight = HISTO_BIN_HEIGHT_MAX*nHistoCurVal/nHistoPeakVal;

				for (unsigned y=0;y<nHistoBinHeight;y++) {
					// Store the RGB triplet
					for (unsigned bin_width=0;bin_width<HISTO_BIN_WIDTH;bin_width++) {
						nHistoX = (i*HISTO_BIN_WIDTH)+bin_width;
						nCoordY = ((2-nHistChan) * HISTO_BIN_HEIGHT_MAX) + y;
						pDibHistoBits[(nHistoX*4)+3+(nCoordY*nDibHistoRowBytes)] = 0;
						pDibHistoBits[(nHistoX*4)+2+(nCoordY*nDibHistoRowBytes)] = (nHistChan==0)?255:0;
						pDibHistoBits[(nHistoX*4)+1+(nCoordY*nDibHistoRowBytes)] = (nHistChan==1)?255:0;
						pDibHistoBits[(nHistoX*4)+0+(nCoordY*nDibHistoRowBytes)] = (nHistChan==2)?255:0;
					}
				}
			} // i: 0..HISTO_BINS-1

		} // nHistChan

		m_bDibHistRgbReady = true;
	}

	// Only create the Y DC Histogram if requested
	m_bDibHistYReady = false;
	if (bDumpHistoY) {

		m_pDibHistY.Kill();

		m_pDibHistY.CreateDIB(SUBSET_HISTO_BINS*HISTO_BIN_WIDTH,HISTO_BIN_HEIGHT_MAX,32);
		nDibHistoRowBytes = (SUBSET_HISTO_BINS*HISTO_BIN_WIDTH) * 4;
		pDibHistoBits = (unsigned char*) ( m_pDibHistY.GetDIBBitArray() );

		m_rectHistYBase = CRect(CPoint(0,0),CSize(SUBSET_HISTO_BINS*HISTO_BIN_WIDTH,HISTO_BIN_HEIGHT_MAX));

		if (pDibHistoBits != NULL) {
			memset(pDibHistoBits,0,HISTO_BIN_HEIGHT_MAX*nDibHistoRowBytes);

			// Do peak detect first
			// Don't want to reset peak value to 0 as otherwise we might get
			// division by zero later when we calculate nHistoBinHeight
			nHistoPeakVal = 1;

			// TODO: Temporarily made quarter width - need to resample instead

			// Peak value
			for (unsigned i=0;i<SUBSET_HISTO_BINS;i++) {
				nHistoCurVal  = m_anHistoYFull[i*4+0];
				nHistoCurVal += m_anHistoYFull[i*4+1];
				nHistoCurVal += m_anHistoYFull[i*4+2];
				nHistoCurVal += m_anHistoYFull[i*4+3];
				nHistoPeakVal = (nHistoCurVal > nHistoPeakVal)?nHistoCurVal:nHistoPeakVal;
			}

			for (unsigned i=0;i<SUBSET_HISTO_BINS;i++) {

				// Calculate bin's height (max HISTO_BIN_HEIGHT_MAX)
				nHistoCurVal  = m_anHistoYFull[i*4+0];
				nHistoCurVal += m_anHistoYFull[i*4+1];
				nHistoCurVal += m_anHistoYFull[i*4+2];
				nHistoCurVal += m_anHistoYFull[i*4+3];
				nHistoBinHeight = HISTO_BIN_HEIGHT_MAX*nHistoCurVal/nHistoPeakVal;

				for (unsigned y=0;y<nHistoBinHeight;y++) {
					// Store the RGB triplet
					for (unsigned bin_width=0;bin_width<HISTO_BIN_WIDTH;bin_width++) {
						nHistoX = (i*HISTO_BIN_WIDTH)+bin_width;
						nCoordY = y;
						pDibHistoBits[(nHistoX*4)+3+(nCoordY*nDibHistoRowBytes)] = 0;
						pDibHistoBits[(nHistoX*4)+2+(nCoordY*nDibHistoRowBytes)] = 255;
						pDibHistoBits[(nHistoX*4)+1+(nCoordY*nDibHistoRowBytes)] = 255;
						pDibHistoBits[(nHistoX*4)+0+(nCoordY*nDibHistoRowBytes)] = 0;
					}
				}
			} // i: 0..HISTO_BINS-1
			m_bDibHistYReady = true;

		}
	} // bDumpHistoY

}

// Reset the decoder Scan Buff (at start of scan and
// after any restart markers)
//
// INPUT:
// - nFilePos					= File position at start of scan
// - bRestart					= Is this a reset due to RSTn marker?
// PRE:
// - m_nRestartInterval
// POST:
// - m_bScanEnd
// - m_bScanBad
// - m_nScanBuff
// - m_nScanBuffPtr_first
// - m_nScanBuffPtr_start
// - m_nScanBuffPtr_align
// - m_anScanBuffPtr_pos[]
// - m_anScanBuffPtr_err[]
// - m_nScanBuffLatchErr
// - m_nScanBuffPtr_num
// - m_nScanBuff_vacant
// - m_nScanCurErr
// - m_bRestartRead
// - m_nRestartMcusLeft
//
void CimgDecode::DecodeRestartScanBuf(unsigned nFilePos,bool bRestart)
{
	// Reset the state
	m_bScanEnd = false;
	m_bScanBad = false;
	m_nScanBuff = 0x00000000;
	m_nScanBuffPtr = nFilePos;
	if (!bRestart) {
		// Only reset the scan buffer pointer at the start of the file,
		// not after any RSTn markers. This is only used for the compression
		// ratio calculations.
		m_nScanBuffPtr_first = nFilePos;
	}
	m_nScanBuffPtr_start = nFilePos;
	m_nScanBuffPtr_align = 0;			// Start with byte alignment (0)
	m_anScanBuffPtr_pos[0] = 0;
	m_anScanBuffPtr_pos[1] = 0;
	m_anScanBuffPtr_pos[2] = 0;
	m_anScanBuffPtr_pos[3] = 0;
	m_anScanBuffPtr_err[0] = SCANBUF_OK;
	m_anScanBuffPtr_err[1] = SCANBUF_OK;
	m_anScanBuffPtr_err[2] = SCANBUF_OK;
	m_anScanBuffPtr_err[3] = SCANBUF_OK;
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



// Color conversion from YCC to RGB
//
// INPUT:
// - sPix				= Structure for color conversion
// OUTPUT:
// - sPix				= Structure for color conversion
//
void CimgDecode::ConvertYCCtoRGBFastFloat(PixelCc &sPix)
{
	int nValY,nValCb,nValCr;
	float nValR,nValG,nValB;


	// Perform ranging to adjust from Huffman sums to reasonable range
	// -1024..+1024 -> -128..127
	sPix.nPreclipY  = sPix.nPrerangeY >> 3;
	sPix.nPreclipCb = sPix.nPrerangeCb >> 3;
	sPix.nPreclipCr = sPix.nPrerangeCr >> 3;

	// Limit on YCC input
	// The y/cb/nPreclipCr values should already be 0..255 unless we have a
	// decode error where DC value gets out of range!
	//CapYccRange(nMcuX,nMcuY,sPix);
	nValY = (sPix.nPreclipY<-128)?-128:(sPix.nPreclipY>127)?127:sPix.nPreclipY;
	nValCb = (sPix.nPreclipCb<-128)?-128:(sPix.nPreclipCb>127)?127:sPix.nPreclipCb;
	nValCr = (sPix.nPreclipCr<-128)?-128:(sPix.nPreclipCr>127)?127:sPix.nPreclipCr;

	// Save the YCC values (0..255)
	sPix.nFinalY  = static_cast<BYTE>(nValY  + 128);
	sPix.nFinalCb = static_cast<BYTE>(nValCb + 128);
	sPix.nFinalCr = static_cast<BYTE>(nValCr + 128);

	// Convert
	// Since the following seems to preserve the multiplies and subtractions
	// we could expand this out manually
	float fConstRed   = 0.299f;
	float fConstGreen = 0.587f;
	float fConstBlue  = 0.114f;
	// r = cr * 1.402 + y;
	// b = cb * 1.772 + y;
	// g = (y - 0.03409 * r) / 0.587;
	nValR = nValCr*(2-2*fConstRed)+nValY;
	nValB = nValCb*(2-2*fConstBlue)+nValY;
	nValG = (nValY-fConstBlue*nValB-fConstRed*nValR)/fConstGreen;

	// Level shift
	nValR += 128;
	nValB += 128;
	nValG += 128;

	// --------------- Finshed the color conversion


	// Limit
	//   r/g/nPreclipB -> r/g/b
	//CapRgbRange(nMcuX,nMcuY,sPix);
	sPix.nFinalR = (nValR<0)?0:(nValR>255)?255:(BYTE)nValR;
	sPix.nFinalG = (nValG<0)?0:(nValG>255)?255:(BYTE)nValG;
	sPix.nFinalB = (nValB<0)?0:(nValB>255)?255:(BYTE)nValB;

}

// Color conversion from YCC to RGB
//
// INPUT:
// - sPix				= Structure for color conversion
// OUTPUT:
// - sPix				= Structure for color conversion
//
void CimgDecode::ConvertYCCtoRGBFastFixed(PixelCc &sPix)
{
	int	nPreclipY,nPreclipCb,nPreclipCr;
	int	nValY,nValCb,nValCr;
	int nValR,nValG,nValB;

	// Perform ranging to adjust from Huffman sums to reasonable range
	// -1024..+1024 -> -128..+127
	nPreclipY  = sPix.nPrerangeY >> 3;
	nPreclipCb = sPix.nPrerangeCb >> 3;
	nPreclipCr = sPix.nPrerangeCr >> 3;


	// Limit on YCC input
	// The nPreclip* values should already be 0..255 unless we have a
	// decode error where DC value gets out of range!

	//CapYccRange(nMcuX,nMcuY,sPix);
	nValY  = (nPreclipY<-128)?-128:(nPreclipY>127)?127:nPreclipY;
	nValCb = (nPreclipCb<-128)?-128:(nPreclipCb>127)?127:nPreclipCb;
	nValCr = (nPreclipCr<-128)?-128:(nPreclipCr>127)?127:nPreclipCr;

	// Save the YCC values (0..255)
	sPix.nFinalY  = static_cast<BYTE>(nValY  + 128);
	sPix.nFinalCb = static_cast<BYTE>(nValCb + 128);
	sPix.nFinalCr = static_cast<BYTE>(nValCr + 128);

	// --------------

	// Convert
	// Fixed values is x 1024 (10 bits). Leaves 22 bits for integer
	//r2 = 1024*cr1*(2-2*fConstRed)+1024*y1;
	//b2 = 1024*cb1*(2-2*fConstBlue)+1024*y1;
	//g2 = 1024*(y1-fConstBlue*b2/1024-fConstRed*r2/1024)/fConstGreen;
	const long int	CFIX_R = 306;
	const long int	CFIX_G = 601;
	const long int	CFIX_B = 116;
	const long int	CFIX2_R = 1436; // 2*(1024-cfix_red)
	const long int	CFIX2_B = 1816; // 2*(1024-cfix_blue)
	const long int	CFIX2_G = 1048576; // 1024*1024

	nValR = CFIX2_R*nValCr + 1024*nValY;
	nValB = CFIX2_B*nValCb + 1024*nValY;
	nValG = (CFIX2_G*nValY - CFIX_B*nValB - CFIX_R*nValR) / CFIX_G;

	nValR >>= 10;
	nValG >>= 10;
	nValB >>= 10;

	// Level shift
	nValR += 128;
	nValB += 128;
	nValG += 128;


	// Limit
	//   r/g/nPreclipB -> r/g/b
	sPix.nFinalR = (nValR<0)?0:(nValR>255)?255:static_cast<BYTE>(nValR);
	sPix.nFinalG = (nValG<0)?0:(nValG>255)?255:static_cast<BYTE>(nValG);
	sPix.nFinalB = (nValB<0)?0:(nValB>255)?255:static_cast<BYTE>(nValB);

}



// Color conversion from YCC to RGB
// - CC: y/cb/cr -> r/g/b
//
// INPUT:
// - nMcuX				= MCU x coordinate
// - nMcuY				= MCU y coordinate
// - sPix				= Structure for color conversion
// OUTPUT:
// - sPix				= Structure for color conversion
// POST:
// - m_sHisto
// - m_anHistoYFull[]
// - m_anCcHisto_r[]
// - m_anCcHisto_g[]
// - m_anCcHisto_b[]
//
void CimgDecode::ConvertYCCtoRGB(unsigned nMcuX,unsigned nMcuY,PixelCc &sPix)
{
	float	fConstRed   = (float)0.299;
	float	fConstGreen = (float)0.587;
	float	fConstBlue  = (float)0.114;
	int		nByteY,nByteCb,nByteCr;
	int		nValY,nValCb,nValCr;
	float	nValR,nValG,nValB;

	if (m_bHistEn) {
		// Calc stats on preranged YCC (direct from huffman DC sums)
		m_sHisto.nPreclipYMin = (sPix.nPrerangeY<m_sHisto.nPreclipYMin)?sPix.nPrerangeY:m_sHisto.nPreclipYMin;
		m_sHisto.nPreclipYMax = (sPix.nPrerangeY>m_sHisto.nPreclipYMax)?sPix.nPrerangeY:m_sHisto.nPreclipYMax;
		m_sHisto.nPreclipYSum += sPix.nPrerangeY;
		m_sHisto.nPreclipCbMin = (sPix.nPrerangeCb<m_sHisto.nPreclipCbMin)?sPix.nPrerangeCb:m_sHisto.nPreclipCbMin;
		m_sHisto.nPreclipCbMax = (sPix.nPrerangeCb>m_sHisto.nPreclipCbMax)?sPix.nPrerangeCb:m_sHisto.nPreclipCbMax;
		m_sHisto.nPreclipCbSum += sPix.nPrerangeCb;
		m_sHisto.nPreclipCrMin = (sPix.nPrerangeCr<m_sHisto.nPreclipCrMin)?sPix.nPrerangeCr:m_sHisto.nPreclipCrMin;
		m_sHisto.nPreclipCrMax = (sPix.nPrerangeCr>m_sHisto.nPreclipCrMax)?sPix.nPrerangeCr:m_sHisto.nPreclipCrMax;
		m_sHisto.nPreclipCrSum += sPix.nPrerangeCr;
	}

	if (m_bHistEn) {
		// Now generate the Y histogram, if requested
		// Add the Y value to the full histogram (for image similarity calcs)
		//if (bDumpHistoY) {
			int histo_index = sPix.nPrerangeY;
			if (histo_index < -1024) histo_index = -1024;
			if (histo_index > 1023) histo_index = 1023;
			histo_index += 1024;
			m_anHistoYFull[histo_index]++;
		//}
	}

	// Perform ranging to adjust from Huffman sums to reasonable range
	// -1024..+1024 -> 0..255
	// Add 1024 then / 8
	sPix.nPreclipY  = (sPix.nPrerangeY+1024)/8;
	sPix.nPreclipCb = (sPix.nPrerangeCb+1024)/8;
	sPix.nPreclipCr = (sPix.nPrerangeCr+1024)/8;


	// Limit on YCC input
	// The y/cb/nPreclipCr values should already be 0..255 unless we have a
	// decode error where DC value gets out of range!
	CapYccRange(nMcuX,nMcuY,sPix);

	// --------------- Perform the color conversion
	nByteY  = sPix.nFinalY;
	nByteCb = sPix.nFinalCb;
	nByteCr = sPix.nFinalCr;

	// Level shift
	nValY  = nByteY - 128;
	nValCb = nByteCb - 128;
	nValCr = nByteCr - 128;

	// Convert
	nValR = nValCr*(2-2*fConstRed)+nValY;
	nValB = nValCb*(2-2*fConstBlue)+nValY;
	nValG = (nValY-fConstBlue*nValB-fConstRed*nValR)/fConstGreen;

	// Level shift
	nValR += 128;
	nValB += 128;
	nValG += 128;

	sPix.nPreclipR = nValR;
	sPix.nPreclipG = nValG;
	sPix.nPreclipB = nValB;
	// --------------- Finshed the color conversion


	// Limit
	// - Preclip RGB to Final RGB
	CapRgbRange(nMcuX,nMcuY,sPix);

	// Display
	/*
	strTmp.Format(_T("* (YCC->RGB) @ (%03u,%03u): YCC=(%4d,%4d,%4d) RGB=(%03u,%03u,%u)"),
		nMcuX,nMcuY,y,cb,cr,r_limb,g_limb,b_limb);
	m_pLog->AddLine(strTmp);
	*/

	if (m_bHistEn) {
		// Bin the result into a histogram!
		//   value = 0..255
		//   bin   = 0..7, 8..15, ..., 248..255
		// Channel: Red
		unsigned bin_divider = 256/HISTO_BINS;
		m_anCcHisto_r[sPix.nFinalR/bin_divider]++;
		m_anCcHisto_g[sPix.nFinalG/bin_divider]++;
		m_anCcHisto_b[sPix.nFinalB/bin_divider]++;


	}

}

// Color conversion clipping
// - Process the pre-clipped YCC values and ensure they
//   have been clipped into the valid region
//
// INPUT:
// - nMcuX				= MCU x coordinate
// - nMcuY				= MCU y coordinate
// - sPix				= Structure for color conversion
// OUTPUT:
// - sPix				= Structure for color conversion
// POST:
// - m_sHisto
//
void CimgDecode::CapYccRange(unsigned nMcuX,unsigned nMcuY,PixelCc &sPix)
{
	// Check the bounds on the YCC value
	// It should probably be 0..255 unless our DC
	// values got really messed up in a corrupt file
	// Perhaps it might be best to reset it to 0? Otherwise
	// it will continuously report an out-of-range value.
	int	nCurY,nCurCb,nCurCr;

	nCurY  = sPix.nPreclipY;
	nCurCb = sPix.nPreclipCb;
	nCurCr = sPix.nPreclipCr;

	if (m_bHistEn) {
		m_sHisto.nClipYMin = (nCurY<m_sHisto.nClipYMin)?nCurY:m_sHisto.nClipYMin;
		m_sHisto.nClipYMax = (nCurY>m_sHisto.nClipYMax)?nCurY:m_sHisto.nClipYMax;
		m_sHisto.nClipYSum += nCurY;
		m_sHisto.nClipCbMin = (nCurCb<m_sHisto.nClipCbMin)?nCurCb:m_sHisto.nClipCbMin;
		m_sHisto.nClipCbMax = (nCurCb>m_sHisto.nClipCbMax)?nCurCb:m_sHisto.nClipCbMax;
		m_sHisto.nClipCbSum += nCurCb;
		m_sHisto.nClipCrMin = (nCurCr<m_sHisto.nClipCrMin)?nCurCr:m_sHisto.nClipCrMin;
		m_sHisto.nClipCrMax = (nCurCr>m_sHisto.nClipCrMax)?nCurCr:m_sHisto.nClipCrMax;
		m_sHisto.nClipCrSum += nCurCr;
		m_sHisto.nCount++;
	}


	if (CC_CLIP_YCC_EN)
	{

		if (nCurY > CC_CLIP_YCC_MAX) {
			if (YCC_CLIP_REPORT_ERR && (m_nWarnYccClipNum < YCC_CLIP_REPORT_MAX)) {
				CString strTmp;
				strTmp.Format(_T("*** NOTE: YCC Clipped. MCU=(%4u,%4u) YCC=(%5d,%5d,%5d) Y Overflow @ Offset %s"),
					nMcuX,nMcuY,nCurY,nCurCb,nCurCr,(LPCTSTR)GetScanBufPos());
				m_pLog->AddLineWarn(strTmp);
				m_nWarnYccClipNum++;
				m_sStatClip.nClipYOver++;
				if (m_nWarnYccClipNum == YCC_CLIP_REPORT_MAX) {
					strTmp.Format(_T("    Only reported first %u instances of this message..."),YCC_CLIP_REPORT_MAX);
					m_pLog->AddLineWarn(strTmp);
				}
			}
			sPix.nClip |= CC_CLIP_Y_OVER;
			nCurY = CC_CLIP_YCC_MAX;
		}
		if (nCurY < CC_CLIP_YCC_MIN) {
			if (YCC_CLIP_REPORT_ERR && (m_nWarnYccClipNum < YCC_CLIP_REPORT_MAX)) {
				CString strTmp;
				strTmp.Format(_T("*** NOTE: YCC Clipped. MCU=(%4u,%4u) YCC=(%5d,%5d,%5d) Y Underflow @ Offset %s"),
					nMcuX,nMcuY,nCurY,nCurCb,nCurCr,(LPCTSTR)GetScanBufPos());
				m_pLog->AddLineWarn(strTmp);
				m_nWarnYccClipNum++;
				m_sStatClip.nClipYUnder++;
				if (m_nWarnYccClipNum == YCC_CLIP_REPORT_MAX) {
					strTmp.Format(_T("    Only reported first %u instances of this message..."),YCC_CLIP_REPORT_MAX);
					m_pLog->AddLineWarn(strTmp);
				}
			}
			sPix.nClip |= CC_CLIP_Y_UNDER;
			nCurY = CC_CLIP_YCC_MIN;
		}
		if (nCurCb > CC_CLIP_YCC_MAX) {
			if (YCC_CLIP_REPORT_ERR && (m_nWarnYccClipNum < YCC_CLIP_REPORT_MAX)) {
				CString strTmp;
				strTmp.Format(_T("*** NOTE: YCC Clipped. MCU=(%4u,%4u) YCC=(%5d,%5d,%5d) Cb Overflow @ Offset %s"),
					nMcuX,nMcuY,nCurY,nCurCb,nCurCr,(LPCTSTR)GetScanBufPos());
				m_pLog->AddLineWarn(strTmp);
				m_nWarnYccClipNum++;
				m_sStatClip.nClipCbOver++;
				if (m_nWarnYccClipNum == YCC_CLIP_REPORT_MAX) {
					strTmp.Format(_T("    Only reported first %u instances of this message..."),YCC_CLIP_REPORT_MAX);
					m_pLog->AddLineWarn(strTmp);
				}
			}
			sPix.nClip |= CC_CLIP_CB_OVER;
			nCurCb = CC_CLIP_YCC_MAX;
		}
		if (nCurCb < CC_CLIP_YCC_MIN) {
			if (YCC_CLIP_REPORT_ERR && (m_nWarnYccClipNum < YCC_CLIP_REPORT_MAX)) {
				CString strTmp;
				strTmp.Format(_T("*** NOTE: YCC Clipped. MCU=(%4u,%4u) YCC=(%5d,%5d,%5d) Cb Underflow @ Offset %s"),
					nMcuX,nMcuY,nCurY,nCurCb,nCurCr,(LPCTSTR)GetScanBufPos());
				m_pLog->AddLineWarn(strTmp);
				m_nWarnYccClipNum++;
				m_sStatClip.nClipCbUnder++;
				if (m_nWarnYccClipNum == YCC_CLIP_REPORT_MAX) {
					strTmp.Format(_T("    Only reported first %u instances of this message..."),YCC_CLIP_REPORT_MAX);
					m_pLog->AddLineWarn(strTmp);
				}
			}
			sPix.nClip |= CC_CLIP_CB_UNDER;
			nCurCb = CC_CLIP_YCC_MIN;
		}
		if (nCurCr > CC_CLIP_YCC_MAX) {
			if (YCC_CLIP_REPORT_ERR && (m_nWarnYccClipNum < YCC_CLIP_REPORT_MAX)) {
				CString strTmp;
				strTmp.Format(_T("*** NOTE: YCC Clipped. MCU=(%4u,%4u) YCC=(%5d,%5d,%5d) Cr Overflow @ Offset %s"),
					nMcuX,nMcuY,nCurY,nCurCb,nCurCr,(LPCTSTR)GetScanBufPos());
				m_pLog->AddLineWarn(strTmp);
				m_nWarnYccClipNum++;
				m_sStatClip.nClipCrOver++;
				if (m_nWarnYccClipNum == YCC_CLIP_REPORT_MAX) {
					strTmp.Format(_T("    Only reported first %u instances of this message..."),YCC_CLIP_REPORT_MAX);
					m_pLog->AddLineWarn(strTmp);
				}
			}
			sPix.nClip |= CC_CLIP_CR_OVER;
			nCurCr = CC_CLIP_YCC_MAX;
		}
		if (nCurCr < CC_CLIP_YCC_MIN) {
			if (YCC_CLIP_REPORT_ERR && (m_nWarnYccClipNum < YCC_CLIP_REPORT_MAX)) {
				CString strTmp;
				strTmp.Format(_T("*** NOTE: YCC Clipped. MCU=(%4u,%4u) YCC=(%5d,%5d,%5d) Cr Underflow @ Offset %s"),
					nMcuX,nMcuY,nCurY,nCurCb,nCurCr,(LPCTSTR)GetScanBufPos());
				m_pLog->AddLineWarn(strTmp);
				m_nWarnYccClipNum++;
				m_sStatClip.nClipCrUnder++;
				if (m_nWarnYccClipNum == YCC_CLIP_REPORT_MAX) {
					strTmp.Format(_T("    Only reported first %u instances of this message..."),YCC_CLIP_REPORT_MAX);
					m_pLog->AddLineWarn(strTmp);
				}
			}
			sPix.nClip |= CC_CLIP_CR_UNDER;
			nCurCr = CC_CLIP_YCC_MIN;
		}
	} // YCC clip enabled?

	// Perform color conversion: YCC->RGB
	// The nCurY/cb/cr values should already be clipped to BYTE size
	sPix.nFinalY  = static_cast<BYTE>(nCurY);
	sPix.nFinalCb = static_cast<BYTE>(nCurCb);
	sPix.nFinalCr = static_cast<BYTE>(nCurCr);

}



// Color conversion clipping (RGB)
// - Input RGB triplet in floats
// - Expect range to be 0..255
// - Return RGB triplet in bytes
// - Report if it is out of range
// - Converts from Preclip RGB to Final RGB
//
// INPUT:
// - nMcuX				= MCU x coordinate
// - nMcuY				= MCU y coordinate
// - sPix				= Structure for color conversion
// OUTPUT:
// - sPix				= Structure for color conversion
// POST:
// - m_sHisto
//
void CimgDecode::CapRgbRange(unsigned nMcuX,unsigned nMcuY,PixelCc &sPix)
{
	int nLimitR,nLimitG,nLimitB;

	// Truncate
	nLimitR = (int)(sPix.nPreclipR);
	nLimitG = (int)(sPix.nPreclipG);
	nLimitB = (int)(sPix.nPreclipB);

	if (m_bHistEn) {
		m_sHisto.nPreclipRMin = (nLimitR<m_sHisto.nPreclipRMin)?nLimitR:m_sHisto.nPreclipRMin;
		m_sHisto.nPreclipRMax = (nLimitR>m_sHisto.nPreclipRMax)?nLimitR:m_sHisto.nPreclipRMax;
		m_sHisto.nPreclipRSum += nLimitR;
		m_sHisto.nPreclipGMin = (nLimitG<m_sHisto.nPreclipGMin)?nLimitG:m_sHisto.nPreclipGMin;
		m_sHisto.nPreclipGMax = (nLimitG>m_sHisto.nPreclipGMax)?nLimitG:m_sHisto.nPreclipGMax;
		m_sHisto.nPreclipGSum += nLimitG;
		m_sHisto.nPreclipBMin = (nLimitB<m_sHisto.nPreclipBMin)?nLimitB:m_sHisto.nPreclipBMin;
		m_sHisto.nPreclipBMax = (nLimitB>m_sHisto.nPreclipBMax)?nLimitB:m_sHisto.nPreclipBMax;
		m_sHisto.nPreclipBSum += nLimitB;
	}

	if (nLimitR < 0) {
		if (m_bVerbose) {
			CString strTmp;
			strTmp.Format(_T("  YCC->RGB Clipped. MCU=(%4u,%4u) RGB=(%5d,%5d,%5d) Red Underflow"),
				nMcuX,nMcuY,nLimitR,nLimitG,nLimitB);
			m_pLog->AddLineWarn(strTmp);
		}
		sPix.nClip |= CC_CLIP_R_UNDER;
		m_sStatClip.nClipRUnder++;
		nLimitR = 0;
	}
	if (nLimitG < 0) {
		if (m_bVerbose) {
			CString strTmp;
			strTmp.Format(_T("  YCC->RGB Clipped. MCU=(%4u,%4u) RGB=(%5d,%5d,%5d) Green Underflow"),
				nMcuX,nMcuY,nLimitR,nLimitG,nLimitB);
			m_pLog->AddLineWarn(strTmp);
		}
		sPix.nClip |= CC_CLIP_G_UNDER;
		m_sStatClip.nClipGUnder++;
		nLimitG = 0;
	}
	if (nLimitB < 0) {
		if (m_bVerbose) {
			CString strTmp;
			strTmp.Format(_T("  YCC->RGB Clipped. MCU=(%4u,%4u) RGB=(%5d,%5d,%5d) Blue Underflow"),
				nMcuX,nMcuY,nLimitR,nLimitG,nLimitB);
			m_pLog->AddLineWarn(strTmp);
		}
		sPix.nClip |= CC_CLIP_B_UNDER;
		m_sStatClip.nClipBUnder++;
		nLimitB = 0;
	}
	if (nLimitR > 255) {
		if (m_bVerbose) {
			CString strTmp;
			strTmp.Format(_T("  YCC->RGB Clipped. MCU=(%4u,%4u) RGB=(%5d,%5d,%5d) Red Overflow"),
				nMcuX,nMcuY,nLimitR,nLimitG,nLimitB);
			m_pLog->AddLineWarn(strTmp);
		}
		sPix.nClip |= CC_CLIP_R_OVER;
		m_sStatClip.nClipROver++;
		nLimitR = 255;
	}
	if (nLimitG > 255) {
		if (m_bVerbose) {
			CString strTmp;
			strTmp.Format(_T("  YCC->RGB Clipped. MCU=(%4u,%4u) RGB=(%5d,%5d,%5d) Green Overflow"),
				nMcuX,nMcuY,nLimitR,nLimitG,nLimitB);
			m_pLog->AddLineWarn(strTmp);
		}
		sPix.nClip |= CC_CLIP_G_OVER;
		m_sStatClip.nClipGOver++;
		nLimitG = 255;
	}
	if (nLimitB > 255) {
		if (m_bVerbose) {
			CString strTmp;
			strTmp.Format(_T("  YCC->RGB Clipped. MCU=(%4u,%4u) RGB=(%5d,%5d,%5d) Blue Overflow"),
				nMcuX,nMcuY,nLimitR,nLimitG,nLimitB);
			m_pLog->AddLineWarn(strTmp);
		}
		sPix.nClip |= CC_CLIP_B_OVER;
		m_sStatClip.nClipBOver++;
		nLimitB = 255;
	}

	if (m_bHistEn) {
		m_sHisto.nClipRMin = (nLimitR<m_sHisto.nClipRMin)?nLimitR:m_sHisto.nClipRMin;
		m_sHisto.nClipRMax = (nLimitR>m_sHisto.nClipRMax)?nLimitR:m_sHisto.nClipRMax;
		m_sHisto.nClipRSum += nLimitR;
		m_sHisto.nClipGMin = (nLimitG<m_sHisto.nClipGMin)?nLimitG:m_sHisto.nClipGMin;
		m_sHisto.nClipGMax = (nLimitG>m_sHisto.nClipGMax)?nLimitG:m_sHisto.nClipGMax;
		m_sHisto.nClipGSum += nLimitG;
		m_sHisto.nClipBMin = (nLimitB<m_sHisto.nClipBMin)?nLimitB:m_sHisto.nClipBMin;
		m_sHisto.nClipBMax = (nLimitB>m_sHisto.nClipBMax)?nLimitB:m_sHisto.nClipBMax;
		m_sHisto.nClipBSum += nLimitB;
	}


	// Now convert to BYTE
	sPix.nFinalR = (byte)nLimitR;
	sPix.nFinalG = (byte)nLimitG;
	sPix.nFinalB = (byte)nLimitB;

}


// Recalcs the full image based on the original YCC pixmap
// - Also locate the brightest pixel.
// - Note that we cannot do the brightest pixel search when we called SetFullRes()
//   because we need to have access to all of the channel components at once to do this.
//
// INPUT:
// - pRectView				= UNUSED. Intended to limit updates to visible region
//                            (Range of real image that is visibile / cropped)
// PRE:
// - m_pPixValY[]
// - m_pPixValCb[]
// - m_pPixValCr[]
// OUTPUT:
// - pTmp					= RGB pixel map (32-bit per pixel, [0x00,R,G,B])
//
void CimgDecode::CalcChannelPreviewFull(CRect* pRectView,unsigned char* pTmp)
{
	pRectView;	// Unreferenced param
	PixelCc		sPixSrc,sPixDst;
	CString		strTmp;

	unsigned	nRowBytes;
	nRowBytes = m_nImgSizeX * sizeof(RGBQUAD);


	// Color conversion process

	unsigned	nPixMapW = m_nBlkXMax*BLK_SZ_X;
	unsigned	nPixmapInd;

	unsigned		nRngX1,nRngX2,nRngY1,nRngY2;
	unsigned		nSumY;
	unsigned long	nNumPixels = 0;

	// TODO: Update ranges to take into account the visible view region
	// The approach might include:
	//   nRngX1 = pRectView->left;
	//   nRngX2 = pRectView->right;
	//   nRngY1 = pRectView->top;
	//   nRngY2 = pRectView->bottom;
	//
	// These co-ords will define range in nPixX,nPixY that get drawn
	// This will help YCC Adjust display react much faster. Only recalc
	// visible portion of image, but then recalc entire one once Adjust
	// dialog is closed.
	//
	// NOTE: if we were to make these ranges a subset of the
	// full image dimensions, then we'd have to determine the best
	// way to handle the brightest pixel search & average luminance logic
	// since those appear in the nRngX/Y loops.

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
	CString		strLine;


	unsigned	nMcuShiftInd = m_nPreviewShiftMcuY * (m_nImgSizeX/m_nMcuWidth) + m_nPreviewShiftMcuX;

	SetStatusText(_T("Color conversion..."));

	if (m_bDetailVlc) {
		m_pLog->AddLine(_T("  Detailed IDCT Dump (RGB):"));
		strTmp.Format(_T("    MCU [%3u,%3u]:"),m_nDetailVlcX,m_nDetailVlcY);
		m_pLog->AddLine(strTmp);
	}

	// Determine pixel count
	nNumPixels = (nRngY2-nRngY1+1) * (nRngX2-nRngX1+1);

	// Step through the image
	for (unsigned nPixY=nRngY1;nPixY<nRngY2;nPixY++) {

		unsigned nMcuY = nPixY/m_nMcuHeight;
		// DIBs appear to be stored up-side down, so correct Y
		unsigned nCoordYInv = (m_nImgSizeY-1) - nPixY;

		for (unsigned nPixX=nRngX1;nPixX<nRngX2;nPixX++) {

			nPixmapInd = nPixY*nPixMapW + nPixX;
			unsigned	nPixByte = nPixX*4+0+nCoordYInv*nRowBytes;

			unsigned	nMcuX = nPixX/m_nMcuWidth;
			unsigned	nMcuInd = nMcuY * (m_nImgSizeX/m_nMcuWidth) + nMcuX;
			int			nTmpY,nTmpCb,nTmpCr;
			nTmpY = m_pPixValY[nPixmapInd];

			if (m_nNumSosComps == NUM_CHAN_YCC) {
				nTmpCb = m_pPixValCb[nPixmapInd];
				nTmpCr = m_pPixValCr[nPixmapInd];
			} else {
				nTmpCb = 0;
				nTmpCr = 0;
			}

			// Load the YCC value
			sPixSrc.nPrerangeY  = nTmpY;
			sPixSrc.nPrerangeCb = nTmpCb;
			sPixSrc.nPrerangeCr = nTmpCr;

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
				sPixSrc.nPrerangeY  += m_nPreviewShiftY;
				sPixSrc.nPrerangeCb += m_nPreviewShiftCb;
				sPixSrc.nPrerangeCr += m_nPreviewShiftCr;
			}

			// Invoke the appropriate color conversion routine
			if (m_bHistEn || m_bStatClipEn) {
				ConvertYCCtoRGB(nMcuX,nMcuY,sPixSrc);
			} else {
				ConvertYCCtoRGBFastFloat(sPixSrc);
				//ConvertYCCtoRGBFastFixed(sPixSrc);
			}

			// Accumulate the luminance value for this pixel
			// after we have converted it to the range 0..255
			nSumY += sPixSrc.nFinalY;


			// If we want a detailed decode of RGB, print it out
			// now if we are on the correct MCU.
			// NOTE: The level shift (adjust) will affect this report!
			if (m_bDetailVlc) {
				if (nMcuY == m_nDetailVlcY) {

					if (nMcuX == m_nDetailVlcX) {
					//if ((nMcuX >= m_nDetailVlcX) && (nMcuX < m_nDetailVlcX+m_nDetailVlcLen)) {
						if (!bRowStart) {
							bRowStart = true;
							strLine.Format(_T("      [ "));
						}
						strTmp.Format(_T("x%02X%02X%02X "),sPixSrc.nFinalR,sPixSrc.nFinalG,sPixSrc.nFinalB);
						strLine.Append(strTmp);
					} else {
						if (bRowStart) {
							// We had started a row, but we are now out of range, so we
							// need to close up!
							strTmp.Format(_T(" ]"));
							strLine.Append(strTmp);
							m_pLog->AddLine(strLine);
							bRowStart = false;
						}
					}
				}

			}

			// Perform any channel filtering if enabled
			ChannelExtract(m_nPreviewMode,sPixSrc,sPixDst);

			// Assign the RGB pixel map
			pTmp[nPixByte+3] = 0;
			pTmp[nPixByte+2] = sPixDst.nFinalR;
			pTmp[nPixByte+1] = sPixDst.nFinalG;
			pTmp[nPixByte+0] = sPixDst.nFinalB;

		} // x
	} // y

	SetStatusText(_T(""));
	// ---------------------------------------------------------

	if (m_bDetailVlc) {
		m_pLog->AddLine(_T(""));
	}


	// Assume that brightest pixel search was successful
	// Now compute the RGB value for this pixel!
	m_bBrightValid = true;
	sPixSrc.nPrerangeY = m_nBrightY;
	sPixSrc.nPrerangeCb = m_nBrightCb;
	sPixSrc.nPrerangeCr = m_nBrightCr;
	ConvertYCCtoRGBFastFloat(sPixSrc);
	m_nBrightR = sPixSrc.nFinalR;
	m_nBrightG = sPixSrc.nFinalG;
	m_nBrightB = sPixSrc.nFinalB;

	// Now perform average luminance calculation
	// NOTE: This will result in a value in the range 0..255
	ASSERT(nNumPixels > 0);
	// Avoid divide by zero
	if (nNumPixels == 0) { nNumPixels = 1; }
	m_nAvgY = nSumY / nNumPixels;
	m_bAvgYValid = true;

}


// Extract the specified channel
//
// INPUT:
// - nMode					= Channel(s) to extract from
// - sSrc					= Color representations (YCC & RGB) for pixel
// OUTPUT:
// - sDst					= Resulting RGB output after filtering
//
void CimgDecode::ChannelExtract(unsigned nMode,PixelCc &sSrc,PixelCc &sDst)
{
	if (nMode == PREVIEW_RGB) {
		sDst.nFinalR = sSrc.nFinalR;
		sDst.nFinalG = sSrc.nFinalG;
		sDst.nFinalB = sSrc.nFinalB;
	} else if (nMode == PREVIEW_YCC) {
		sDst.nFinalR = sSrc.nFinalCr;
		sDst.nFinalG = sSrc.nFinalY;
		sDst.nFinalB = sSrc.nFinalCb;
	} else if (nMode == PREVIEW_R) {
		sDst.nFinalR = sSrc.nFinalR;
		sDst.nFinalG = sSrc.nFinalR;
		sDst.nFinalB = sSrc.nFinalR;
	} else if (nMode == PREVIEW_G) {
		sDst.nFinalR = sSrc.nFinalG;
		sDst.nFinalG = sSrc.nFinalG;
		sDst.nFinalB = sSrc.nFinalG;
	} else if (nMode == PREVIEW_B) {
		sDst.nFinalR = sSrc.nFinalB;
		sDst.nFinalG = sSrc.nFinalB;
		sDst.nFinalB = sSrc.nFinalB;
	} else if (nMode == PREVIEW_Y) {
		sDst.nFinalR = sSrc.nFinalY;
		sDst.nFinalG = sSrc.nFinalY;
		sDst.nFinalB = sSrc.nFinalY;
	} else if (nMode == PREVIEW_CB) {
		sDst.nFinalR = sSrc.nFinalCb;
		sDst.nFinalG = sSrc.nFinalCb;
		sDst.nFinalB = sSrc.nFinalCb;
	} else if (nMode == PREVIEW_CR) {
		sDst.nFinalR = sSrc.nFinalCr;
		sDst.nFinalG = sSrc.nFinalCr;
		sDst.nFinalB = sSrc.nFinalCr;
	} else {
		sDst.nFinalR = sSrc.nFinalR;
		sDst.nFinalG = sSrc.nFinalG;
		sDst.nFinalB = sSrc.nFinalB;
	}

}

// Fetch the detailed decode settings (VLC)
//
// OUTPUT:
// - bDetail			= Enable for detailed scan VLC reporting
// - nX					= Start of detailed scan decode MCU X coordinate
// - nY					= Start of detailed scan decode MCU Y coordinate
// - nLen				= Number of MCUs to parse in detailed scan decode
//
void CimgDecode::GetDetailVlc(bool &bDetail,unsigned &nX,unsigned &nY,unsigned &nLen)
{
	bDetail = m_bDetailVlc;
	nX = m_nDetailVlcX;
	nY = m_nDetailVlcY;
	nLen = m_nDetailVlcLen;
}

// Set the detailed scan decode settings (VLC)
//
// INPUT:
// - bDetail			= Enable for detailed scan VLC reporting
// - nX					= Start of detailed scan decode MCU X coordinate
// - nY					= Start of detailed scan decode MCU Y coordinate
// - nLen				= Number of MCUs to parse in detailed scan decode
//
void CimgDecode::SetDetailVlc(bool bDetail,unsigned nX,unsigned nY,unsigned nLen)
{
	m_bDetailVlc = bDetail;
	m_nDetailVlcX = nX;
	m_nDetailVlcY = nY;
	m_nDetailVlcLen = nLen;
}

// Fetch the pointers for the pixel map
//
// OUTPUT:
// - pMayY				= Pointer to pixel map for Y component
// - pMapCb				= Pointer to pixel map for Cb component
// - pMapCr				= Pointer to pixel map for Cr component
//
void CimgDecode::GetPixMapPtrs(short* &pMapY,short* &pMapCb,short* &pMapCr)
{
	ASSERT(m_pPixValY);
	ASSERT(m_pPixValCb);
	ASSERT(m_pPixValCr);
	pMapY = m_pPixValY;
	pMapCb = m_pPixValCb;
	pMapCr = m_pPixValCr;
}

// Get image pixel dimensions rounded up to nearest MCU
//
// OUTPUT:
// - nX					= X dimension of preview image
// - nY					= Y dimension of preview image
//
void CimgDecode::GetImageSize(unsigned &nX,unsigned &nY)
{
	nX = m_nImgSizeX;
	nY = m_nImgSizeY;
}

// Get the bitmap pointer
//
// OUTPUT:
// - pBitmap			= Bitmap (DIB) of preview
//
void CimgDecode::GetBitmapPtr(unsigned char* &pBitmap)
{
	unsigned char *		pDibImgTmpBits = NULL;

	pDibImgTmpBits = (unsigned char*) ( m_pDibTemp.GetDIBBitArray() );

	// Ensure that the pointers are available!
	if ( !pDibImgTmpBits ) {
		pBitmap = NULL;
	} else {
		pBitmap = pDibImgTmpBits;
	}
}

// Calculate RGB pixel map from selected channels of YCC pixel map
//
// PRE:
// - m_pPixValY
// - m_pPixValCb
// - m_pPixValCr
// POST:
// - m_pDibTemp
// NOTE:
// - Channels are selected in CalcChannelPreviewFull()
//
void CimgDecode::CalcChannelPreview()
{
	unsigned char *		pDibImgTmpBits = NULL;

	pDibImgTmpBits = (unsigned char*) ( m_pDibTemp.GetDIBBitArray() );

	// Ensure that the pointers are available!
	if ( !pDibImgTmpBits ) {
		return;
	}


	// If we need to do a YCC shift, then do full recalc into tmp array
	CalcChannelPreviewFull(NULL,pDibImgTmpBits);

	// Since this was a complex mod, we don't mark this channel as
	// being "done", so we will need to recalculate any time we change
	// the channel display.

	// Force an update of the view to be sure
	//m_pDoc->UpdateAllViews(NULL);

	return;

}


// Determine the file position from a pixel coordinate
//
// INPUT:
// - nPixX					= Pixel coordinate (x)
// - nPixY					= Pixel coordinate (y)
// OUTPUT:
// - nByte					= File offset (byte)
// - nBit					= File offset (bit)
//
void CimgDecode::LookupFilePosPix(unsigned nPixX,unsigned nPixY, unsigned &nByte, unsigned &nBit)
{
	unsigned nMcuX,nMcuY;
	unsigned nPacked;
	nMcuX = nPixX / m_nMcuWidth;
	nMcuY = nPixY / m_nMcuHeight;
	nPacked = m_pMcuFileMap[nMcuX + nMcuY*m_nMcuXMax];
	UnpackFileOffset(nPacked,nByte,nBit);
}

// Determine the file position from a MCU coordinate
//
// INPUT:
// - nMcuX					= MCU coordinate (x)
// - nMcuY					= MCU coordinate (y)
// OUTPUT:
// - nByte					= File offset (byte)
// - nBit					= File offset (bit)
//
void CimgDecode::LookupFilePosMcu(unsigned nMcuX,unsigned nMcuY, unsigned &nByte, unsigned &nBit)
{
	unsigned nPacked;
	nPacked = m_pMcuFileMap[nMcuX + nMcuY*m_nMcuXMax];
	UnpackFileOffset(nPacked,nByte,nBit);
}

// Determine the YCC DC value of a specified block
//
// INPUT:
// - nBlkX					= 8x8 block coordinate (x)
// - nBlkY					= 8x8 block coordinate (y)
// OUTPUT:
// - nY						= Y channel value
// - nCb					= Cb channel value
// - nCr					= Cr channel value
//
void CimgDecode::LookupBlkYCC(unsigned nBlkX,unsigned nBlkY,int &nY,int &nCb,int &nCr)
{
	nY  = m_pBlkDcValY [nBlkX + nBlkY*m_nBlkXMax];
	if (m_nNumSosComps == NUM_CHAN_YCC) {
		nCb = m_pBlkDcValCb[nBlkX + nBlkY*m_nBlkXMax];
		nCr = m_pBlkDcValCr[nBlkX + nBlkY*m_nBlkXMax];
	} else {
		nCb = 0;	// FIXME
		nCr = 0;	// FIXME
	}
}

// Convert pixel coordinate to MCU coordinate
//
// INPUT:
// - ptPix					= Pixel coordinate
// RETURN:
// - MCU coordinate
//
CPoint CimgDecode::PixelToMcu(CPoint ptPix)
{
	CPoint ptMcu;
	ptMcu.x = ptPix.x / m_nMcuWidth;
	ptMcu.y = ptPix.y / m_nMcuHeight;
	return ptMcu;
}

// Convert pixel coordinate to block coordinate
//
// INPUT:
// - ptPix					= Pixel coordinate
// RETURN:
// - 8x8 block coordinate
//
CPoint CimgDecode::PixelToBlk(CPoint ptPix)
{
	CPoint ptBlk;
	ptBlk.x = ptPix.x / BLK_SZ_X;
	ptBlk.y = ptPix.y / BLK_SZ_Y;
	return ptBlk;
}

// Return the linear MCU offset from an MCU X,Y coord
//
// INPUT:
// - ptMcu					= MCU coordinate
// PRE:
// - m_nMcuXMax
// RETURN:
// - Index of MCU from start of MCUs
//
unsigned CimgDecode::McuXyToLinear(CPoint ptMcu)
{
	unsigned nLinear;
	nLinear = ( (ptMcu.y * m_nMcuXMax) + ptMcu.x);
	return nLinear;
}

// Create a file offset notation that represents bytes and bits
// - Essentially a fixed-point notation
//
// INPUT:
// - nByte				= File byte position
// - nBit				= File bit position
// RETURN:
// - Fixed-point file offset (29b for bytes, 3b for bits)
//
unsigned CimgDecode::PackFileOffset(unsigned nByte,unsigned nBit)
{
	unsigned nTmp;
	// Note that we only really need 3 bits, but I'll keep 4
	// so that the file offset is human readable. We will only
	// handle files up to 2^28 bytes (256MB), so this is probably
	// fine!
	nTmp = (nByte << 4) + nBit;
	return nTmp;
}

// Convert from file offset notation to bytes and bits
//
// INPUT:
// - nPacked			= Fixed-point file offset (29b for bytes, 3b for bits)
// OUTPUT:
// - nByte				= File byte position
// - nBit				= File bit position
//
void CimgDecode::UnpackFileOffset(unsigned nPacked, unsigned &nByte, unsigned &nBit)
{
	nBit  = nPacked & 0x7;
	nByte = nPacked >> 4;
}


// Fetch the number of block markers assigned
//
// RETURN:
// - Number of marker blocks
//
unsigned CimgDecode::GetMarkerCount()
{
	return m_nMarkersBlkNum;
}

// Fetch an indexed block marker
//
// INPUT:
// - nInd					= Index into marker block array
// RETURN:
// - Point (8x8 block) from marker array
//
CPoint CimgDecode::GetMarkerBlk(unsigned nInd)
{
	CPoint	myPt(0,0);
	if (nInd < m_nMarkersBlkNum) {
		myPt = m_aptMarkersBlk[nInd];
	} else {
		ASSERT(false);
	}
	return myPt;
}



// Add a block to the block marker list
// - Also report out the YCC DC value for the block
//
// INPUT:
// - nBlkX					= 8x8 block X coordinate
// - nBlkY					= 8x8 block Y coordinate
// POST:
// - m_nMarkersBlkNum
// - m_aptMarkersBlk[]
//
void CimgDecode::SetMarkerBlk(unsigned nBlkX,unsigned nBlkY)
{
	if (m_nMarkersBlkNum == MAX_BLOCK_MARKERS) {
		// Shift them down by 1. Last entry will be deleted next
		for (unsigned ind=1;ind<MAX_BLOCK_MARKERS;ind++) {
			m_aptMarkersBlk[ind-1] = m_aptMarkersBlk[ind];
		}
		// Reflect new reduced count
		m_nMarkersBlkNum--;
	}

	CString		strTmp;
	int			nY,nCb,nCr;
	unsigned	nMcuX,nMcuY;
	unsigned	nCssX,nCssY;

	// Determine the YCC of the block
	// Then calculate the MCU coordinate and the block coordinate within the MCU
	LookupBlkYCC(nBlkX,nBlkY,nY,nCb,nCr);
	nMcuX = nBlkX / (m_nMcuWidth  / BLK_SZ_X);
	nMcuY = nBlkY / (m_nMcuHeight / BLK_SZ_Y);
	nCssX = nBlkX % (m_nMcuWidth  / BLK_SZ_X);
	nCssY = nBlkY % (m_nMcuHeight / BLK_SZ_X);

	// Force text out to log
	bool	bQuickModeSaved = m_pLog->GetQuickMode();
	m_pLog->SetQuickMode(false);
	strTmp.Format(_T("Position Marked @ MCU=[%4u,%4u](%u,%u) Block=[%4u,%4u] YCC=[%5d,%5d,%5d]"),
		nMcuX,nMcuY,nCssX,nCssY,nBlkX,nBlkY,nY,nCb,nCr);
	m_pLog->AddLine(strTmp);
	m_pLog->AddLine(_T(""));
	m_pLog->SetQuickMode(bQuickModeSaved);


	m_aptMarkersBlk[m_nMarkersBlkNum].x = nBlkX;
	m_aptMarkersBlk[m_nMarkersBlkNum].y = nBlkY;
	m_nMarkersBlkNum++;
}



// Fetch the preview zoom mode
//
// RETURN:
// - The zoom mode enumeration
//
unsigned CimgDecode::GetPreviewZoomMode()
{
	return m_nZoomMode;
}

// Fetch the preview mode
//
// RETURN:
// - The image preview mode enumeration
//
unsigned CimgDecode::GetPreviewMode()
{
	return m_nPreviewMode;
}

// Fetch the preview zoom level
//
// RETURN:
// - The preview zoom level enumeration
//
float CimgDecode::GetPreviewZoom()
{
	return m_nZoom;
}

// Change the current zoom level
// - Supports either direct setting of the level
//   or an increment/decrement operation
//
// INPUT:
// - bInc				= Flag to increment the zoom level
// - bDec				= Flag to decrement the zoom level
// - bSet				= Flag to set the zoom level
// - nVal				= Zoom level for "set" operation
// POST:
// - m_nZoomMode
//
void CimgDecode::SetPreviewZoom(bool bInc,bool bDec,bool bSet,unsigned nVal)
{
	if (bInc) {
		if (m_nZoomMode+1 < PRV_ZOOMEND) {
			m_nZoomMode++;

		}
	} else if (bDec) {
		if (m_nZoomMode-1 > PRV_ZOOMBEGIN) {
			m_nZoomMode--;
		}
	} else if (bSet) {
		m_nZoomMode = nVal;
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

// Main draw routine for the image
// - Draws the preview image with frame
// - Draws any histogram
// - Draws a title
// - Draws any MCU overlays / grid
//
// INPUT:
// - pDC					= The device context pointer
// - rectClient				= From GetClientRect()
// - ptScrolledPos			= From GetScrollPosition()
// - pFont					= Pointer to the font used for title/lables
// OUTPUT:
// - szNewScrollSize		= New dimension used for SetScrollSizes()
//
void CimgDecode::ViewOnDraw(CDC* pDC,CRect rectClient,CPoint ptScrolledPos,
							CFont* pFont, CSize &szNewScrollSize)
{

	unsigned	nBorderLeft		= 10;
	unsigned	nBorderBottom	= 10;
	unsigned	nTitleHeight	= 20;
	unsigned	nTitleIndent    = 5;
	unsigned	nTitleLowGap	= 3;

	m_nPageWidth = 600;
	m_nPageHeight = 10;	// Start with some margin


	CString strTmp;
	CString strRender;
	int		nHeight;


	unsigned nYPosImgTitle		= 0;
	unsigned nYPosImg			= 0;
	unsigned nYPosHistTitle		= 0;
	unsigned nYPosHist			= 0;
	unsigned nYPosHistYTitle	= 0;
	unsigned nYPosHistY			= 0;	// Y position of Img & Histogram

	CRect rectTmp;

	// If we have displayed an image, make sure to allow for
	// the additional space!

	bool bImgDrawn = false;

	CBrush	brGray(    RGB(128, 128, 128));
	CBrush	brGrayLt1( RGB(210, 210, 210));
	CBrush	brGrayLt2( RGB(240, 240, 240));
	CBrush	brBlueLt(  RGB(240, 240, 255));
	CPen	penRed(PS_DOT,1,RGB(255,0,0));


	if (m_bDibTempReady) {

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

		if (m_bDibHistRgbReady) {

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

		if (m_bDibHistYReady) {

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
	rectClientScrolled.OffsetRect(ptScrolledPos);

	// Change the font
	CFont*	pOldFont;
	pOldFont = pDC->SelectObject(pFont);

	CString strTitle;

	// Draw the bitmap if ready
	if (m_bDibTempReady) {

		// Print label

		if (!m_bPreviewIsJpeg) {
			// For all non-JPEG images, report with simple title
			strTitle = _T("Image");
		} else {
			strTitle = _T("Image (");
			switch (m_nPreviewMode) {
				case PREVIEW_RGB:	strTitle += _T("RGB"); break;
				case PREVIEW_YCC:	strTitle += _T("YCC"); break;
				case PREVIEW_R:		strTitle += _T("R"); break;
				case PREVIEW_G:		strTitle += _T("G"); break;
				case PREVIEW_B:		strTitle += _T("B"); break;
				case PREVIEW_Y:		strTitle += _T("Y"); break;
				case PREVIEW_CB:	strTitle += _T("Cb"); break;
				case PREVIEW_CR:	strTitle += _T("Cr"); break;
				default:			strTitle += _T("???"); break;
			}
			if (m_bDecodeScanAc) {
				strTitle += _T(", DC+AC)");
			} else {
				strTitle += _T(", DC)");
			}
		}


		switch (m_nZoomMode) {
			case PRV_ZOOM_12: strTitle += " @ 12.5% (1/8)"; break;
			case PRV_ZOOM_25: strTitle += " @ 25% (1/4)"; break;
			case PRV_ZOOM_50: strTitle += " @ 50% (1/2)"; break;
			case PRV_ZOOM_100: strTitle += _T(" @ 100% (1:1)"); break;
			case PRV_ZOOM_150: strTitle += _T(" @ 150% (3:2)"); break;
			case PRV_ZOOM_200: strTitle += _T(" @ 200% (2:1)"); break;
			case PRV_ZOOM_300: strTitle += _T(" @ 300% (3:1)"); break;
			case PRV_ZOOM_400: strTitle += _T(" @ 400% (4:1)"); break;
			case PRV_ZOOM_800: strTitle += _T(" @ 800% (8:1)"); break;
			default: strTitle += _T(""); break;
		}


		// Calculate the title width
		CRect rectCalc = CRect(0,0,0,0);
		nHeight = pDC->DrawText(strTitle,-1,&rectCalc,
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
		nHeight = pDC->DrawText(strTitle,-1,&rectTmp,
			DT_TOP | DT_NOPREFIX | DT_SINGLELINE);
		pDC->SetBkMode(nBkMode);

		// Draw image

		// Assume that the temp image has already been generated!
		// For speed purposes, we use m_pDibTemp only when we are
		// in a mode other than RGB or YCC. In the RGB/YCC modes,
		// we skip the CalcChannelPreview() step.

		// TODO: Improve redraw time by only redrawing the currently visible
		// region. Requires a different CopyDIB routine with a subset region.
		// Needs to take into account zoom setting (eg. intersection between
		// rectClient and m_rectImgReal).

		// Use a common DIB instead of creating/swapping tmp / ycc and rgb.
		// This way we can also have more flexibility in modifying RGB & YCC displays.
		// Time calling CalcChannelPreview() seems small, so no real impact.

		// Image member usage:
		// m_pDibTemp:

		m_pDibTemp.CopyDIB(pDC,m_rectImgReal.left,m_rectImgReal.top,m_nZoom);

		// Now create overlays

		// Only draw overlay (eg. actual image boundary overlay) if the values
		// have been set properly. Note that PSD decode currently sets these
		// values to zero.
		if ((m_nDimX != 0) && (m_nDimY != 0)) {
			CPen* pPen = pDC->SelectObject(&penRed);

			// Draw boundary for end of valid data (inside partial MCU)
			int nXZoomed = (int)(m_nDimX*m_nZoom);
			int nYZoomed = (int)(m_nDimY*m_nZoom);

			pDC->MoveTo(m_rectImgReal.left+nXZoomed,m_rectImgReal.top);
			pDC->LineTo(m_rectImgReal.left+nXZoomed,m_rectImgReal.top+nYZoomed);

			pDC->MoveTo(m_rectImgReal.left,m_rectImgReal.top+nYZoomed);
			pDC->LineTo(m_rectImgReal.left+nXZoomed,m_rectImgReal.top+nYZoomed);

			pDC->SelectObject(pPen);
		}

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

		// m_nPageWidth is already hardcoded above
		ASSERT(m_nPageWidth>=0);
		m_nPageWidth = max((unsigned)m_nPageWidth, m_nPreviewPosX + m_nPreviewSizeX);


	}

	if (m_bHistEn) {

		if (m_bDibHistRgbReady) {

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
			m_pDibHistRgb.CopyDIB(pDC,m_rectHistReal.left,m_rectHistReal.top);
		}

		if (m_bDibHistYReady) {

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
			m_pDibHistY.CopyDIB(pDC,m_rectHistYReal.left,m_rectHistYReal.top);
			//m_pDibHistY.CopyDIB(pDC,nBorderLeft,nYPosHistY);
		}
	}

	// If no image has been drawn, indicate to user why!
	if (!bImgDrawn) {
		//ScrollRect.top = m_nPageHeight;	// FIXME:?

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

// Draw an overlay that shows the MCU grid
//
// INPUT:
// - pDC			= The device context pointer
//
void CimgDecode::ViewMcuOverlay(CDC* pDC)
{
	// Now create overlays
	unsigned nXZoomed,nYZoomed;

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

}

// Draw an overlay that highlights the marked MCUs
//
// INPUT:
// - pDC			= The device context pointer
//
void CimgDecode::ViewMcuMarkedOverlay(CDC* pDC)
{
	pDC;	// Unreferenced param

	// Now draw a simple MCU Marker overlay
	CRect	my_rect;
	CBrush	my_brush(RGB(255, 0, 255));
	for (unsigned nMcuY=0;nMcuY<m_nMcuYMax;nMcuY++) {
		for (unsigned nMcuX=0;nMcuX<m_nMcuXMax;nMcuX++) {
			//unsigned nXY = nMcuY*m_nMcuXMax + nMcuX;
			//unsigned nXZoomed = (unsigned)(nMcuX*m_nMcuWidth*m_nZoom);
			//unsigned nYZoomed = (unsigned)(nMcuY*m_nMcuHeight*m_nZoom);

			/*
			// TODO: Implement an overlay function
			if (m_bMarkedMcuMapEn && m_abMarkedMcuMap && m_abMarkedMcuMap[nXY]) {
				// Note that drawing is an overlay, so we are dealing with real
				// pixel coordinates, not preview image coords
				my_rect = CRect(nXZoomed,nYZoomed,nXZoomed+(unsigned)(m_nMcuWidth*m_nZoom),
					nYZoomed+(unsigned)(m_nMcuHeight*m_nZoom));
				my_rect.OffsetRect(m_rectImgReal.left,m_rectImgReal.top);
				pDC->FrameRect(my_rect,&my_brush);
			}
			*/

		}
	}

}


// Draw an overlay for the indexed block
//
// INPUT:
// - pDC			= The device context pointer
// - nBlkX			= 8x8 block X coordinate
// - nBlkY			= 8x8 block Y coordinate
//
void CimgDecode::ViewMarkerOverlay(CDC* pDC,unsigned nBlkX,unsigned nBlkY)
{
	CRect	my_rect;
	CBrush	my_brush(RGB(255, 0, 255));

	// Note that drawing is an overlay, so we are dealing with real
	// pixel coordinates, not preview image coords
	my_rect = CRect(	(unsigned)(m_nZoom*(nBlkX+0)),
						(unsigned)(m_nZoom*(nBlkY+0)),
						(unsigned)(m_nZoom*(nBlkX+1)),
						(unsigned)(m_nZoom*(nBlkY+1)));
	my_rect.OffsetRect(m_nPreviewPosX,m_nPreviewPosY);
	pDC->FillRect(my_rect,&my_brush);
}


// Get the preview MCU grid setting
//
// OUTPUT:
// - The MCU grid enabled flag
//
bool CimgDecode::GetPreviewOverlayMcuGrid()
{
	return m_bViewOverlaysMcuGrid;
}

// Toggle the preview MCU grid setting
//
// POST:
// - m_bViewOverlaysMcuGrid
//
void CimgDecode::SetPreviewOverlayMcuGridToggle()
{
	if (m_bViewOverlaysMcuGrid) {
		m_bViewOverlaysMcuGrid = false;
	} else {
		m_bViewOverlaysMcuGrid = true;
	}
}

// Report the DC levels
// - UNUSED
//
// INPUT:
// - nMcuX				= MCU x coordinate
// - nMcuY				= MCU y coordinate
// - nMcuLen			= Number of MCUs to report
//
void CimgDecode::ReportDcRun(unsigned nMcuX, unsigned nMcuY, unsigned nMcuLen)
{
	// FIXME: Should I be working on MCU level or block level?
	CString	strTmp;
	m_pLog->AddLine(_T(""));
	m_pLog->AddLineHdr(_T("*** Reporting DC Levels ***"));
	strTmp.Format(_T("  Starting MCU   = [%u,%u]"),nMcuX,nMcuY);
	strTmp.Format(_T("  Number of MCUs = %u"),nMcuLen);
	m_pLog->AddLine(strTmp);
	for (unsigned ind=0;ind<nMcuLen;ind++)
	{
		// TODO: Need some way of getting these values
		// For now, just rely on bDetailVlcEn & PrintDcCumVal() to report this...
	}
}

// Update the YCC status text
//
// INPUT:
// - strText			= Status text to display
//
void CimgDecode::SetStatusYccText(CString strText)
{
	m_strStatusYcc = strText;
}

// Fetch the YCC status text
//
// RETURN:
// - Status text currently displayed
//
CString CimgDecode::GetStatusYccText()
{
	return m_strStatusYcc;
}

// Update the MCU status text
//
// INPUT:
// - strText				= MCU indicator text
//
void CimgDecode::SetStatusMcuText(CString strText)
{
	m_strStatusMcu = strText;
}

// Fetch the MCU status text
//
// RETURN:
// - MCU indicator text
//
CString CimgDecode::GetStatusMcuText()
{
	return m_strStatusMcu;
}

// Update the file position text
//
// INPUT:
// - strText				= File position text
//
void CimgDecode::SetStatusFilePosText(CString strText)
{
	m_strStatusFilePos = strText;
}

// Fetch the file position text
//
// RETURN:
// - File position text
//
CString CimgDecode::GetStatusFilePosText()
{
	return m_strStatusFilePos;
}



