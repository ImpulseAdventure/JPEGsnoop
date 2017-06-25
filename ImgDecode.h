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
// - This module decodes the scan segment (SOS)
// - Depends on quantization tables and huffman tables
// - At this time, only a single scan per frame/file is decoded
// - Chroma subsampling
// - Color conversion and bitmap display output
// - Channel selection for output
// - Statistics: histogram, brightest pixel, etc.
//
// ==========================================================================


#pragma once

#include "SnoopConfig.h"

#include "snoop.h"

#include "DocLog.h"
#include "WindowBuf.h"
#include "afxwin.h"
#include "Dib.h"

#include <map>

#include "General.h"


// Color conversion clipping (YCC) reporting
#define YCC_CLIP_REPORT_ERR true	// Are YCC clips an error?
#define YCC_CLIP_REPORT_MAX 10		// Number of same error to report

// Scan image component indices for known arrangements
#define	COMP_IND_YCC_Y			1
#define	COMP_IND_YCC_CB	 		2
#define	COMP_IND_YCC_CR			3
#define	COMP_IND_YCC_K			4
#define	COMP_IND_CMYK_C			1
#define	COMP_IND_CMYK_M	 		2
#define	COMP_IND_CMYK_Y			3
#define	COMP_IND_CMYK_K			3

// Definitions for DHT array indices m_anDhtTblSel[][]
#define	MAX_DHT_CLASS		2
#define	MAX_DHT_DEST_ID		4	// Maximum range for DHT Destination ID (ie. DHT:Th). Range 0..3
#define DHT_CLASS_DC		0
#define	DHT_CLASS_AC		1

#define MAX_DHT_CODES		260			// Was 180, but with 12-bit DHT, we may have nearly all 0xFF codes
#define	MAX_DHT_CODELEN		16			// Max number of bits in a code
#define	DHT_CODE_UNUSED		0xFFFFFFFF	// Mark table entry as unused


#define	MAX_DQT_DEST_ID		4			// Maximum range for DQT Destination ID (ie. DQT:Tq). Range 0..3
#define MAX_DQT_COEFF		64			// Number of coefficients in DQT matrix
#define MAX_DQT_COMP		256			// Max range of frame component identifier (SOF:Ci, 0..255)

#define DCT_COEFF_DC		0			// DCT matrix coefficient index for DC component

#define MAX_SAMP_FACT_H		4		// Maximum sampling factor supported
#define MAX_SAMP_FACT_V		4		// Maximum sampling factor supported

#define	DQT_DEST_Y			1
#define	DQT_DEST_CB			2
#define	DQT_DEST_CR			3
#define	DQT_DEST_K			4

#define	BLK_SZ_X			8			// JPEG block size (x)
#define	BLK_SZ_Y			8			// JPEG block size (y)

#define DCT_SZ_X			8						// IDCT matrix size (x)
#define DCT_SZ_Y			8						// IDCT matrix size (y)
#define	DCT_SZ_ALL			(DCT_SZ_X*DCT_SZ_Y)		// IDCT matrix all coeffs

#define IMG_BLK_SZ				1	// Size of each MCU in image display
#define MAX_SCAN_DECODED_DIM	512	// X & Y dimension for top-left image display
#define DHT_FAST_SIZE			9	// Number of bits for DHT direct lookup

// FIXME: MAX_SOF_COMP_NF per spec might actually be 255
#define MAX_SOF_COMP_NF			256		// Maximum number of Image Components in Frame (Nf) [from SOF] (Nf range 1..255)
#define MAX_SOS_COMP_NS			4		// Maximum number of Image Components in Scan (Ns) [from SOS] (Ns range 1..4)

// TODO: Merge with COMP_IND_YCC_*?
#define SCAN_COMP_Y				1
#define SCAN_COMP_CB			2
#define SCAN_COMP_CR			3
#define SCAN_COMP_K				4



// Number of image channels supported for image output
#define NUM_CHAN_GRAYSCALE	1
#define	NUM_CHAN_YCC		3
#define	NUM_CHAN_YCCK		4

// Image channel indices (after converting component index to channel index)
#define CHAN_Y				0
#define	CHAN_CB				1
#define	CHAN_CR				2

// Maximum number of blocks that can be marked
// - This feature is generally used to mark ranges for the detailed scan decode feature
#define MAX_BLOCK_MARKERS		10

// JFIF Markers relevant for Scan Decoder
// - Restart markers
// - EOI
#define JFIF_RST0			0xD0
#define JFIF_RST1			0xD1
#define JFIF_RST2			0xD2
#define JFIF_RST3			0xD3
#define JFIF_RST4			0xD4
#define JFIF_RST5			0xD5
#define JFIF_RST6			0xD6
#define JFIF_RST7			0xD7
#define JFIF_EOI			0xD9

// Color correction clipping indicator
#define CC_CLIP_NONE		0x00000000
#define CC_CLIP_Y_UNDER		0x80000000
#define CC_CLIP_Y_OVER		0x00800000
#define CC_CLIP_CB_UNDER	0x40000000
#define CC_CLIP_CB_OVER		0x00400000
#define CC_CLIP_CR_UNDER	0x20000000
#define CC_CLIP_CR_OVER		0x00200000
#define CC_CLIP_R_UNDER		0x00008000
#define CC_CLIP_R_OVER		0x00000080
#define CC_CLIP_G_UNDER		0x00004000
#define CC_CLIP_G_OVER		0x00000040
#define CC_CLIP_B_UNDER		0x00002000
#define CC_CLIP_B_OVER		0x00000020

#define CC_CLIP_YCC_EN		true
#define CC_CLIP_YCC_MIN		0
#define CC_CLIP_YCC_MAX		255

// Image histogram definitions
#define HISTO_BINS				128
#define	HISTO_BIN_WIDTH			1
#define	HISTO_BIN_HEIGHT_MAX	30

// Histogram of Y component (-1024..+1023) = 2048 bins
#define FULL_HISTO_BINS		2048
#define SUBSET_HISTO_BINS	512

// Return values for ReadScanVal()
enum teRsvRet {
	RSV_OK,				// OK
	RSV_EOB,			// End of Block
	RSV_UNDERFLOW,		// Ran out of data in buffer
	RSV_RST_TERM		// No huffman code found, but restart marker seen
};

// Scan decode errors (in m_anScanBuffPtr_err[] buffer array)
enum teScanBufStatus {
	SCANBUF_OK,
	SCANBUF_BADMARK,
	SCANBUF_RST
};


// Per-pixel color conversion structure
// - Records each stage of the process and associated clipping/ranging
typedef struct {

	// Pre-ranged YCC
	// - Raw YCC value from pixel map before any adjustment
	// - Example range: -1024...1023
	int			nPrerangeY;
	int			nPrerangeCb;
	int			nPrerangeCr;
	// Pre-clip YCC
	// - After scaling pre-ranged values
	// - Typical range should be 0..255
	int			nPreclipY;
	int			nPreclipCb;
	int			nPreclipCr;
	// Final YCC
	// - After clipping to ensure we are in range 0..255
	BYTE		nFinalY;
	BYTE		nFinalCr;
	BYTE		nFinalCb;

	// Pre-clip RGB
	// - After color conversion and level shifting
	float		nPreclipR;
	float		nPreclipG;
	float		nPreclipB;
	// Final RGB
	// - After clipping to 0..255
	BYTE		nFinalR;
	BYTE		nFinalG;
	BYTE		nFinalB;

	// Any YCC or RGB clipping performed?
	unsigned	nClip;				// Multi-bit indicator of clipping (CC_CLIP_*)
} PixelCc;


// Per-pixel stats: clipping (overflow and underflow) in YCC and RGB
typedef struct {
	unsigned	nClipYUnder;
	unsigned	nClipYOver;
	unsigned	nClipCbUnder;
	unsigned	nClipCbOver;
	unsigned	nClipCrUnder;
	unsigned	nClipCrOver;
	unsigned	nClipRUnder;
	unsigned	nClipROver;
	unsigned	nClipGUnder;
	unsigned	nClipGOver;
	unsigned	nClipBUnder;
	unsigned	nClipBOver;
	unsigned	nClipWhiteOver;
} PixelCcClip;


// Min-max and average pixel stats for histogram
typedef struct {
	int			nPreclipYMin;
	int			nPreclipYMax;
	int			nPreclipYSum;
	int			nPreclipCbMin;
	int			nPreclipCbMax;
	int			nPreclipCbSum;
	int			nPreclipCrMin;
	int			nPreclipCrMax;
	int			nPreclipCrSum;

	int			nClipYMin;
	int			nClipYMax;
	int			nClipYSum;
	int			nClipCbMin;
	int			nClipCbMax;
	int			nClipCbSum;
	int			nClipCrMin;
	int			nClipCrMax;
	int			nClipCrSum;

	int			nClipRMin;
	int			nClipRMax;
	int			nClipRSum;
	int			nClipGMin;
	int			nClipGMax;
	int			nClipGSum;
	int			nClipBMin;
	int			nClipBMax;
	int			nClipBSum;

	int			nPreclipRMin;
	int			nPreclipRMax;
	int			nPreclipRSum;
	int			nPreclipGMin;
	int			nPreclipGMax;
	int			nPreclipGSum;
	int			nPreclipBMin;
	int			nPreclipBMax;
	int			nPreclipBSum;

	unsigned	nCount;
} PixelCcHisto;



class CimgDecode
{
public:
	CimgDecode(CDocLog* pLog, CwindowBuf* pWBuf);
	~CimgDecode();

	void		Reset();		// Called during start of SOS decode
	void		ResetState();	// Called at start of new JFIF Decode

	void		SetStatusBar(CStatusBar* pStatBar);
	void		DecodeScanImg(unsigned bStart,bool bDisplay,bool bQuiet);

	void		DrawHistogram(bool bQuiet,bool bDumpHistoY);
	void		ReportHistogramY();
	void		ReportColorStats();

	bool		IsPreviewReady();

	// Config
	void		SetImageDimensions(unsigned nWidth,unsigned nHeight);
	void		SetImageDetails(unsigned nDimX,unsigned nDimY,unsigned nCompsSOF,unsigned nCompsSOS,bool bRstEn,unsigned nRstInterval);
	void		SetSofSampFactors(unsigned nCompInd, unsigned nSampFactH, unsigned nSampFactV);
	void		ResetImageContent();
	
	bool		SetDqtEntry(unsigned nTblDestId, unsigned nCoeffInd, unsigned nCoeffIndZz, unsigned short nCoeffVal);
	bool		SetDqtTables(unsigned nCompInd, unsigned nTbl);
	unsigned	GetDqtEntry(unsigned nTblDestId, unsigned nCoeffInd);
	bool		SetDhtTables(unsigned nCompInd, unsigned nTblDc, unsigned nTblAc);
	bool		SetDhtEntry(unsigned nDestId, unsigned nClass, unsigned nInd, unsigned nLen,
							unsigned nBits, unsigned nMask, unsigned nCode);
	bool		SetDhtSize(unsigned nDestId,unsigned nClass,unsigned nSize);
	void		SetPrecision(unsigned nPrecision);


	// Modes -- accessed from Doc
	void		SetPreviewMode(unsigned nMode);
	unsigned	GetPreviewMode();
	void		SetPreviewYccOffset(unsigned nMcuX,unsigned nMcuY,int nY,int nCb,int nCr);
	void		GetPreviewYccOffset(unsigned &nMcuX,unsigned &nMcuY,int &nY,int &nCb,int &nCr);
	void		SetPreviewMcuInsert(unsigned nMcuX,unsigned nMcuY,int nLen);
	void		GetPreviewMcuInsert(unsigned &nMcuX,unsigned &nMcuY,unsigned &nLen);
	void		SetPreviewZoom(bool bInc,bool bDec,bool bSet,unsigned nVal);
	unsigned	GetPreviewZoomMode();
	float		GetPreviewZoom();
	bool		GetPreviewOverlayMcuGrid();
	void		SetPreviewOverlayMcuGridToggle();

	// Utilities
	void		LookupFilePosPix(unsigned nPixX,unsigned nPixY, unsigned &nByte, unsigned &nBit);
	void		LookupFilePosMcu(unsigned nMcuX,unsigned nMcuY, unsigned &nByte, unsigned &nBit);
	void		LookupBlkYCC(unsigned nBlkX,unsigned nBlkY,int &nY,int &nCb,int &nCr);

	void		SetMarkerBlk(unsigned nBlkX,unsigned nBlkY);
	unsigned	GetMarkerCount();
	CPoint		GetMarkerBlk(unsigned nInd);

	CPoint		PixelToMcu(CPoint ptPix);
	CPoint		PixelToBlk(CPoint ptPix);
	unsigned	McuXyToLinear(CPoint ptMcu);
	void		GetImageSize(unsigned &nX,unsigned &nY);

	// View helper routines
	void		ViewOnDraw(CDC* pDC,CRect rectClient,CPoint ptScrolledPos,CFont* pFont, CSize &szNewScrollSize);
	void		ViewMcuOverlay(CDC* pDC);
	void		ViewMcuMarkedOverlay(CDC* pDC);
	void		ViewMarkerOverlay(CDC* pDC,unsigned nBlkX,unsigned nBlkY);	// UNUSED?

	void		GetPixMapPtrs(short* &pMapY,short* &pMapCb,short* &pMapCr);
	void		GetDetailVlc(bool &bDetail,unsigned &nX,unsigned &nY,unsigned &nLen);
	void		SetDetailVlc(bool bDetail,unsigned nX,unsigned nY,unsigned nLen);

	void		GetPreviewPos(unsigned &nX,unsigned &nY);
	void		GetPreviewSize(unsigned &nX,unsigned &nY);

private:

	void		ResetDqtTables();
	void		ResetDhtLookup();

	CString		GetScanBufPos();
	CString		GetScanBufPos(unsigned pos, unsigned align);

	void		ConvertYCCtoRGB(unsigned nMcuX,unsigned nMcuY,PixelCc &sPix);
	void		ConvertYCCtoRGBFastFloat(PixelCc &sPix);
	void		ConvertYCCtoRGBFastFixed(PixelCc &sPix);

	void		ScanYccToRgb();
	void		CapYccRange(unsigned nMcuX,unsigned nMcuY,PixelCc &sPix);
	void		CapRgbRange(unsigned nMcuX,unsigned nMcuY,PixelCc &sPix);

	void		GenLookupHuffMask();
	unsigned	ExtractBits(unsigned nWord,unsigned nBits);
	teRsvRet	ReadScanVal(unsigned nClass,unsigned nTbl,unsigned &rZrl,signed &rVal);
	bool		DecodeScanComp(unsigned nTblDhtDc,unsigned nTblDhtAc,unsigned nTblDqt,unsigned nMcuX,unsigned nMcuY);
	bool		DecodeScanCompPrint(unsigned nTblDhtDc,unsigned nTblDhtAc,unsigned nTblDqt,unsigned nMcuX,unsigned nMcuY);
	signed		HuffmanDc2Signed(unsigned nVal,unsigned nBits);
	void		CheckScanErrors(unsigned nMcuX,unsigned nMcuY,unsigned nCssX,unsigned nCssY,unsigned nComp);


public:
	void		ScanErrorsDisable();
	void		ScanErrorsEnable();
private:

	bool		ExpectRestart();
	void		DecodeRestartDcState();
	void		DecodeRestartScanBuf(unsigned nFilePos,bool bRestart);
	unsigned	BuffAddByte();
	void		BuffTopup();
	void		ScanBuffConsume(unsigned nNumBits);
	void		ScanBuffAdd(unsigned nNewByte,unsigned nPtr);
	void		ScanBuffAddErr(unsigned nNewByte,unsigned nPtr,unsigned nErr);

	// IDCT calcs
	void		PrecalcIdct();
	void		DecodeIdctClear();
	void		DecodeIdctSet(unsigned nTbl,unsigned num_coeffs,unsigned zrl,short int val);
	void		DecodeIdctCalcFloat(unsigned nCoefMax);
	void		DecodeIdctCalcFixedpt();
	void		ClrFullRes(unsigned nWidth,unsigned nHeight);
	void		SetFullRes(unsigned nMcuX,unsigned nMcuY,unsigned nComp,unsigned nCssXInd,unsigned nCssYInd,short int nDcOffset);

public: // For ImgMod
	unsigned	PackFileOffset(unsigned nByte,unsigned nBit);
	void		UnpackFileOffset(unsigned nPacked, unsigned &nByte, unsigned &nBit);
private:

	void		ChannelExtract(unsigned nMode,PixelCc &sSrc,PixelCc &sDst);
	void		CalcChannelPreviewFull(CRect* pRectView,unsigned char* pTmp);
	void		CalcChannelPreview();

public: // For Export
	void		GetBitmapPtr(unsigned char* &pBitmap);

	// Miscellaneous
	void		SetStatusText(CString strText);
	void		SetStatusYccText(CString strText);
	CString		GetStatusYccText();
	void		SetStatusMcuText(CString strText);
	CString		GetStatusMcuText();
	void		SetStatusFilePosText(CString strText);
	CString		GetStatusFilePosText();

	void		ReportDctMatrix();
	void		ReportDctYccMatrix();
	void		ReportVlc(unsigned nVlcPos, unsigned nVlcAlign,
						   unsigned nZrl, int nVal,
						   unsigned nCoeffStart,unsigned nCoeffEnd,
						   CString specialStr);
	void		PrintDcCumVal(unsigned nMcuX,unsigned nMcuY,int nVal);
	void		ReportDcRun(unsigned nMcuX, unsigned nMcuY, unsigned nMcuLen);


	// -------------------------------------------------------------
	// Member variables
	// -------------------------------------------------------------

private:
	CSnoopConfig*		m_pAppConfig;	// Pointer to application config

	unsigned *			m_pMcuFileMap;
	unsigned			m_nMcuWidth;	// Width (pix) of MCU (e.g. 8,16)
	unsigned			m_nMcuHeight;	// Height (pix) of MCU (e.g. 8,16)
	unsigned			m_nMcuXMax;		// Number of MCUs across
	unsigned			m_nMcuYMax;		// Number of MCUs vertically
	unsigned			m_nBlkXMax;		// Number of 8x8 blocks across
	unsigned			m_nBlkYMax;		// Number of 8x8 blocks vertically

	// Fill these with the cumulative values so that we can do
	// a YCC to RGB conversion (for level shift previews, etc.)
	short *				m_pPixValY;		// Pixel value
	short *				m_pPixValCb;	// Pixel value
	short *				m_pPixValCr;	// Pixel value

	// Array of block DC values. Only used for under-cursor reporting.
	short *				m_pBlkDcValY;	// Block DC value
	short *				m_pBlkDcValCb;	// Block DC value
	short *				m_pBlkDcValCr;	// Block DC value

	// TODO: Later use these to support frequency spectrum
	// display of image data
	short *				m_pBlkFreqY;	// 8x8 Block frequency value
	short *				m_pBlkFreqCb;	// 8x8 Block frequency value
	short *				m_pBlkFreqCr;	// 8x8 Block frequency value


	// Array that indicates whether or not a block has been marked
	// This is generally used to mark ranges for the detailed scan decode feature
	unsigned			m_nMarkersBlkNum;					// Number of 8x8 Block markers
	CPoint				m_aptMarkersBlk[MAX_BLOCK_MARKERS];	// MCU Markers


	CStatusBar*			m_pStatBar;			// Link to status bar
	CString				m_strStatusYcc;		// Status bar text: YCC
	CString				m_strStatusMcu;		// Status bar text: MCU
	CString				m_strStatusFilePos;	// Status bar text: File Position

	unsigned			m_nImgSizeXPartMcu;	// Image size with possible partial MCU
	unsigned			m_nImgSizeYPartMcu;
	unsigned int		m_nImgSizeX;		// Image size rounding up to full MCU
	unsigned int		m_nImgSizeY;

	// Image rects
	CRect				m_rectImgBase;		// Image with no offset, no scaling
	CRect				m_rectImgReal;		// Image with scaling & offset
	CRect				m_rectHistBase;		// Hist with no offset
	CRect				m_rectHistReal;		// Hist with offset
	CRect				m_rectHistYBase;	// HistY with no offset
	CRect				m_rectHistYReal;	// HistY with offset

	// Decoder DC state
	// Tables use signed to help flag undefined table indices
	signed short		m_nDcLum;
	signed short		m_nDcChrCb;
	signed short		m_nDcChrCr;
	signed short		m_anDcLumCss[MAX_SAMP_FACT_V*MAX_SAMP_FACT_H];	// Need 4x2 at least. Support up to 4x4
	signed short		m_anDcChrCbCss[MAX_SAMP_FACT_V*MAX_SAMP_FACT_H];
	signed short		m_anDcChrCrCss[MAX_SAMP_FACT_V*MAX_SAMP_FACT_H];


	bool				m_bScanBad;			// Any errors found?
	unsigned			m_nScanErrMax;		// Max # scan decode errors shown

public: //xxx hack
	bool				m_bDibTempReady;
	CDIB				m_pDibTemp;				// Temporary version for display
	bool				m_bPreviewIsJpeg;		// Is the preview image from decoded JPEG?
private:

	bool				m_bDibHistRgbReady;
	CDIB				m_pDibHistRgb;

	bool				m_bDibHistYReady;
	CDIB				m_pDibHistY;

	bool				m_bDetailVlc;
	unsigned			m_nDetailVlcX;
	unsigned			m_nDetailVlcY;
	unsigned			m_nDetailVlcLen;

	unsigned			m_nZoomMode;
	float				m_nZoom;

	// Logger connection
	CDocLog*			m_pLog;
	CwindowBuf*			m_pWBuf;

	// Image details (from JFIF decode) set by SetImageDetails()
	bool				m_bImgDetailsSet;						// Have image details been set yet (from SOS)
	unsigned			m_nDimX;								// Image Dimension in X
	unsigned			m_nDimY;								// Image Dimension in Y
	unsigned			m_nNumSosComps;							// Number of Image Components (DHT?)
	unsigned			m_nNumSofComps;							// Number of Image Components (DQT?)
	unsigned			m_nPrecision;							// 8-bit or 12-bit (defined in JFIF_SOF1)
	unsigned			m_anSofSampFactH[MAX_SOF_COMP_NF];		// Sampling factor per component ID in frame from SOF
	unsigned			m_anSofSampFactV[MAX_SOF_COMP_NF];	  	// Sampling factor per component ID in frame from SOF
	unsigned			m_nSosSampFactHMax;						// Maximum sampling factor for scan
	unsigned			m_nSosSampFactVMax;						// Maximum sampling factor for scan
	unsigned			m_nSosSampFactHMin;						// Minimum sampling factor for scan
	unsigned			m_nSosSampFactVMin;						// Minimum sampling factor for scan
	unsigned			m_anSampPerMcuH[MAX_SOF_COMP_NF];		// Number of samples of component ID per MCU
	unsigned			m_anSampPerMcuV[MAX_SOF_COMP_NF];		// Number of samples of component ID per MCU
	unsigned			m_anExpandBitsMcuH[MAX_SOF_COMP_NF];	// Number of bits to replicate in SetFullRes() due to sampling factor
	unsigned			m_anExpandBitsMcuV[MAX_SOF_COMP_NF];	// Number of bits to replicate in SetFullRes() due to sampling factor

	bool				m_bRestartEn;		// Did decoder see DRI?
	unsigned			m_nRestartInterval;	// ... if so, what is the MCU interval
	unsigned			m_nRestartRead;		// Number RST read during m_nScanBuff
	unsigned			m_nPreviewMode;		// Preview mode

	// Test shifting of YCC for ChannelPreview()
	int					m_nPreviewShiftY;
	int					m_nPreviewShiftCb;
	int					m_nPreviewShiftCr;
	unsigned			m_nPreviewShiftMcuX;	// Start co-ords of shift
	unsigned			m_nPreviewShiftMcuY;

	unsigned			m_nPreviewInsMcuX;	// Test blank MCU insert
	unsigned			m_nPreviewInsMcuY;
	unsigned			m_nPreviewInsMcuLen;



	// DQT Table
public: 
	unsigned short		m_anDqtCoeff[MAX_DQT_DEST_ID][MAX_DQT_COEFF];	// Normal ordering
	unsigned short		m_anDqtCoeffZz[MAX_DQT_DEST_ID][MAX_DQT_COEFF];	// Original zigzag ordering
	int					m_anDqtTblSel[MAX_DQT_COMP];					// DQT table selector for image component in frame

private:


	bool				m_bDecodeScanAc;				// User request decode of AC components?

	// Brightest pixel detection
	int					m_nBrightY;
	int					m_nBrightCb;
	int					m_nBrightCr;
	unsigned			m_nBrightR;
	unsigned			m_nBrightG;
	unsigned			m_nBrightB;
	CPoint				m_ptBrightMcu;
	bool				m_bBrightValid;

	// Average pixel intensity
	long				m_nAvgY;
	bool				m_bAvgYValid;


	bool				m_bScanErrorsDisable;			// Disable scan errors reporting

	// Temporary processing of IDCT per block
	float				m_afIdctLookup[DCT_SZ_ALL][DCT_SZ_ALL];	// IDCT lookup table (floating point)
	int					m_anIdctLookup[DCT_SZ_ALL][DCT_SZ_ALL];	// IDCT lookup table (fixed point)
	unsigned			m_nDctCoefMax;							// Largest DCT coeff to process
	signed short		m_anDctBlock[DCT_SZ_ALL];				// Input block for IDCT process
	float				m_afIdctBlock[DCT_SZ_ALL];				// Output block after IDCT (via floating point)
	int					m_anIdctBlock[DCT_SZ_ALL];				// Output block after IDCT (via fixed point)

	// DHT Lookup table for real decode
	// Note: Component destination index is 1-based; first entry [0] is unused
	int					m_anDhtTblSel       [MAX_DHT_CLASS][1+MAX_SOS_COMP_NS];					// DHT table selected for image component index (1..4)
	unsigned			m_anHuffMaskLookup[32];
	// Huffman lookup table for current scan
	unsigned			m_anDhtLookupSetMax [MAX_DHT_CLASS];									// Highest DHT table index (ie. 0..3) per class
	unsigned			m_anDhtLookupSize   [MAX_DHT_CLASS][MAX_DHT_DEST_ID];						// Number of entries in each lookup table
	unsigned			m_anDhtLookup_bitlen[MAX_DHT_CLASS][MAX_DHT_DEST_ID][MAX_DHT_CODES];		// Number of compare bits in MSB
	unsigned			m_anDhtLookup_bits  [MAX_DHT_CLASS][MAX_DHT_DEST_ID][MAX_DHT_CODES];		// Compare bits in MSB
	unsigned			m_anDhtLookup_mask  [MAX_DHT_CLASS][MAX_DHT_DEST_ID][MAX_DHT_CODES];		// Mask of compare bits
	unsigned			m_anDhtLookup_code  [MAX_DHT_CLASS][MAX_DHT_DEST_ID][MAX_DHT_CODES];		// Resulting code
	unsigned			m_anDhtLookupfast   [MAX_DHT_CLASS][MAX_DHT_DEST_ID][2<<DHT_FAST_SIZE];	// Fast direct lookup for codes <= 10 bits
	unsigned			m_anDhtHisto        [MAX_DHT_CLASS][MAX_DHT_DEST_ID][MAX_DHT_CODELEN+1];
	// Note: MAX_DHT_CODELEN is +1 because this array index is 1-based since there are no codes of length 0 bits

	unsigned			m_nScanBuff;			// 32 bits of scan data after removing stuffs

	unsigned			m_nScanBuff_vacant;		// Bits unused in LSB after shifting (add if >= 8)
	unsigned long		m_nScanBuffPtr;			// Next byte position to load
	unsigned long		m_nScanBuffPtr_start;	// Saved first position of scan data (reset by RSTn markers)
	unsigned long		m_nScanBuffPtr_first;	// Saved first position of scan data in file (not reset by RSTn markers). For comp ratio.

	bool				m_nScanCurErr;			// Mark as soon as error occurs
	unsigned			m_anScanBuffPtr_pos[4];	// File posn for each byte in buffer
	unsigned			m_anScanBuffPtr_err[4];	// Does this byte have an error?
	unsigned			m_nScanBuffLatchErr;
	unsigned			m_nScanBuffPtr_num;		// Number of bytes in buffer
	unsigned			m_nScanBuffPtr_align;	// Bit alignment in file for 1st byte in buffer
	bool				m_bScanEnd;				// Reached end of scan segment?

	bool				m_bRestartRead;			// Have we seen a restart marker?
	unsigned			m_nRestartLastInd;		// Last Restart marker read (0..7)
	unsigned			m_nRestartExpectInd;	// Next Restart marker expected (0..7)
	unsigned			m_nRestartMcusLeft;		// How many MCUs until next RST?

	bool				m_bSkipDone;
	unsigned			m_nSkipCount;
	unsigned			m_nSkipData;

	bool				m_bVerbose;
	unsigned			m_nWarnYccClipNum;
	unsigned			m_nWarnBadScanNum;

	unsigned			m_nScanBitsUsed1;
	unsigned			m_nScanBitsUsed2;

	// Image Analysis, histograms
	bool				m_bHistEn;		// Histograms enabled? (by AppConfig)
	bool				m_bStatClipEn;	// UNUSED: Enable scan clipping stats?
	unsigned			m_nNumPixels;
	PixelCcHisto		m_sHisto;		// YCC/RGB histogram (min/max/avg)
	PixelCcClip			m_sStatClip;	// YCC/RGB clipping stats

	unsigned			m_anCcHisto_r[HISTO_BINS];
	unsigned			m_anCcHisto_g[HISTO_BINS];
	unsigned			m_anCcHisto_b[HISTO_BINS];

	// For image similarity analysis
	unsigned			m_anHistoYFull[FULL_HISTO_BINS];
	unsigned			m_anHistoYSubset[FULL_HISTO_BINS];


	// For View management
	int					m_nPageHeight;
	int					m_nPageWidth;

	unsigned			m_nPreviewPosX;		// Top-left coord of preview image
	unsigned			m_nPreviewPosY;		// Top-left coord of preview image
	unsigned			m_nPreviewSizeX;	// X dimension of preview image
	unsigned			m_nPreviewSizeY;	// Y dimension of preview image

	bool				m_bViewOverlaysMcuGrid;	// Do we enable MCU Grid Overlay

};




