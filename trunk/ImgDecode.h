// JPEGsnoop - JPEG Image Decoder & Analysis Utility
// Copyright (C) 2010 - Calvin Hass
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

#include "SnoopConfig.h"

#define YCC_CLIP_REPORT_ERR true	// Are YCC clips an error?
#define YCC_CLIP_REPORT_MAX 10		// Number of same error to report

#define MAX_COMPS 150

#include "snoop.h"

#include "DocLog.h"
#include "WindowBuf.h"
#include "afxwin.h"
#include "Dib.h"

//#include "JPEGsnoopDoc.h"


// I thought that there was a max of 4 tables, but in my
// progressive example I see more!
#define MAX_DHT 4
#define MAX_DHT_CODES 260			// Was 180, but with 12-bit DHT, we may have nearly all 0xFF codes

#define IMG_BLK_SZ				1	// Size of each MCU in image display
#define MAX_SCAN_DECODED_DIM	512	// X & Y dimension for top-left image display
#define DHT_FAST_SIZE			9	// Number of bits for DHT direct lookup

#define JFIF_RST0 0xD0
#define JFIF_RST1 0xD1
#define JFIF_RST2 0xD2
#define JFIF_RST3 0xD3
#define JFIF_RST4 0xD4
#define JFIF_RST5 0xD5
#define JFIF_RST6 0xD6
#define JFIF_RST7 0xD7
#define JFIF_EOI  0xD9

#define DAMAGED_MCU_MAX 200

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

#define HISTO_BINS			128
#define	HISTO_BIN_WIDTH		1
#define	HISTO_BIN_HEIGHT	30

// Histogram of Y component (-1024..+1023) = 2048 bins
#define FULL_HISTO_BINS		2048
#define SUBSET_HISTO_BINS	512

// Number of resume points to define in map
#define RESUME_MAX			600 //200
#define RESUME_STEP			1 //5

// Marked MCU Map
// Allocate enough space for 40 megapixel image
// Note that we pre-allocate space because we don't want to 
// lose the markers between re-process events!
#define MARKMCUMAP_SIZE		(40000000/64)


typedef struct {
	// Pre-ranged YCC
	int			y_prerange;
	int			cb_prerange;
	int			cr_prerange;

	// Pre-clip YCC
	int			y_preclip;
	int			cb_preclip;
	int			cr_preclip;
	// Final YCC
	int			y;
	int			cr;
	int			cb;
	// Pre-clip RGB
	float		r_preclip;
	float		g_preclip;
	float		b_preclip;
	// Final RGB
	BYTE		r;
	BYTE		g;
	BYTE		b;
	// Any YCC or RGB clipping?
	unsigned	clip;
} PixelCc;

typedef struct {
	unsigned	y_under;
	unsigned	y_over;
	unsigned	cb_under;
	unsigned	cb_over;
	unsigned	cr_under;
	unsigned	cr_over;
	unsigned	r_under;
	unsigned	r_over;
	unsigned	g_under;
	unsigned	g_over;
	unsigned	b_under;
	unsigned	b_over;
	unsigned	white_over;
} PixelCcClip;

typedef struct {
	int			y_pre_min;
	int			y_pre_max;
	int			y_pre_sum;
	int			cb_pre_min;
	int			cb_pre_max;
	int			cb_pre_sum;
	int			cr_pre_min;
	int			cr_pre_max;
	int			cr_pre_sum;

	int			y_min;
	int			y_max;
	int			y_sum;
	int			cb_min;
	int			cb_max;
	int			cb_sum;
	int			cr_min;
	int			cr_max;
	int			cr_sum;

	int			r_min;
	int			r_max;
	int			r_sum;
	int			g_min;
	int			g_max;
	int			g_sum;
	int			b_min;
	int			b_max;
	int			b_sum;

	int			r_pre_min;
	int			r_pre_max;
	int			r_pre_sum;
	int			g_pre_min;
	int			g_pre_max;
	int			g_pre_sum;
	int			b_pre_min;
	int			b_pre_max;
	int			b_pre_sum;

	unsigned	count;
} PixelCcHisto;



class CimgDecode
{
public:
	CimgDecode(CDocLog* pLog, CwindowBuf* pWBuf);
	~CimgDecode();

	void		Reset();		// Called during start of SOS decode
	void		FreshStart();	// Called at start of new Jfif Decode
	void		ResetNewFile();	// Called when we open up a new file

	void		SetStatusBar(CStatusBar* pStatBar);
	void		DecodeScanImg(unsigned bStart,bool bDisplay,bool bQuiet,CSnoopConfig* pAppConfig);
	void		DecodeScanImgQuick(unsigned bStart,bool bDisplay,bool bQuiet,CSnoopConfig* pAppConfig);



	// Config
	void		SetImageDetails(unsigned nCssX,unsigned nCssY,unsigned nDimX,unsigned nDimY,
								 unsigned nCompsSOF, unsigned nCompsSOS,bool bRstEn,unsigned nRstInterval);

	bool		SetDqtEntry(unsigned nSet, unsigned nInd, unsigned nIndzz, unsigned nCoeff);
	bool		SetDqtTables(unsigned nComp, unsigned nTbl);
	unsigned	GetDqtEntry(unsigned nSet, unsigned nInd);
	bool		SetDhtTables(unsigned nComp, unsigned nTblDc, unsigned nTblAc);
	bool		SetDhtEntry(unsigned nDestId, unsigned nClass, unsigned nInd, unsigned nLen,
							unsigned nBits, unsigned nMask, unsigned nCode);
	void		GetDhtEntry(unsigned nDcAc,unsigned nComp,unsigned nInd,
							 unsigned &nBits, unsigned &nBitlen, unsigned &nCode);
	bool		SetDhtSize(unsigned nDestId,unsigned nClass,unsigned nSize);
	unsigned	GetDhtSize(unsigned nDcAc,unsigned nComp);
	void		SetPrecision(unsigned nPrecision);


	// Modes -- accessed from Doc
	void		SetPreviewMode(unsigned nMode);
	unsigned	GetPreviewMode();
	void		SetPreviewYccOffset(unsigned nMcuX,unsigned nMcuY,int nY,int nCb,int nCr);
	void		GetPreviewYccOffset(unsigned &nMcuX,unsigned &nMcuY,int &nY,int &nCb,int &nCr);
	void		SetPreviewMcuInsert(unsigned nMcuX,unsigned nMcuY,int nLen);
	void		GetPreviewMcuInsert(unsigned &nMcuX,unsigned &nMcuY,unsigned &nLen);
	void		SetPreviewZoom(bool inc,bool dec,bool set,unsigned val);
	unsigned	GetPreviewZoom();
	bool		GetPreviewOverlayMcuGrid();
	void		SetPreviewOverlayMcuGridToggle();

	// Utilities
	void		LookupFilePosPix(unsigned pix_x,unsigned pix_y, unsigned &nByte, unsigned &nBit);
	void		LookupFilePosMcu(unsigned mcu_x,unsigned mcu_y, unsigned &nByte, unsigned &nBit);
	void		LookupFilePosBlk(unsigned blk_x,unsigned blk_y, unsigned &nByte, unsigned &nBit);
	void		LookupBlkYCC(unsigned blk_x,unsigned blk_y,int &nY,int &nCb,int &nCr);

	void		SetMarker(unsigned blk_x,unsigned blk_y);
	void		SetMarkerBlk(unsigned nBlkX,unsigned nBlkY);
	void		SetMarkerMcu(unsigned nMcuX,unsigned nMcuY,unsigned nState);
	void		SetMarkerMcuTo(unsigned nMcuX,unsigned nMcuY,unsigned nState);
	CPoint		PixelToMcu(CPoint ptPix);
	CPoint		PixelToBlk(CPoint ptPix);


	// View helper routines
	void		ViewOnDraw(CDC* pDC,CRect rectClient,CPoint ScrolledPos,CFont* pFont, CSize &szNewScrollSize);
	void		ViewMcuOverlay(CDC* pDC);
	void		ViewMcuMarkedOverlay(CDC* pDC);
	void		ViewMarkerOverlay(CDC* pDC,unsigned blk_x,unsigned blk_y);	// UNUSED?

private:

	void		ResetDqtTables();
	void		ResetDhtLookup();

	CString		GetScanBufPos();
	CString		GetScanBufPos(unsigned pos, unsigned align);

	void		ConvertYCCtoRGB(unsigned nMcuX,unsigned nMcuY,PixelCc &rPix);
	void		ConvertYCCtoRGBFastFloat(PixelCc &rPix);
	void		ConvertYCCtoRGBFastFixed(PixelCc &rPix);

	void		ScanYccToRgb();
	void		CapYccRange(unsigned nMcuX,unsigned nMcuY,PixelCc &rPix);
	void		CapRgbRange(unsigned nMcuX,unsigned nMcuY,PixelCc &rPix);

	void		HuffMaskLookupGen();
	unsigned	ExtractBits(unsigned nWord,unsigned nBits);
	unsigned	ReadScanVal(unsigned nClass,unsigned nTbl,unsigned &rZrl,signed &rVal);
	unsigned	DecodeScanComp(unsigned nTbl,unsigned nMcuX,unsigned nMcuY);
	unsigned	DecodeScanCompPrint(unsigned nTbl,unsigned nMcuX,unsigned nMcuY);
	signed		HuffmanDc2Signed(unsigned nVal,unsigned nBits);
	void		CheckScanErrors(unsigned nMcuX,unsigned nMcuY,unsigned nTbl,unsigned nInd);


public:
	unsigned	DecodeScanDword(unsigned nFilePos,unsigned nWordAlign,unsigned nClass,unsigned nTbl,unsigned &rZrl,signed &rVal);
	void		ScanErrorsDisable();
	void		ScanErrorsEnable();
private:

	bool		ExpectRestart();
	void		DecodeRestartDcState();
	void		DecodeRestartScanBuf(unsigned nFilePos);
	unsigned	BuffAddByte();
	void		BuffTopup();
	void		ScanBuffConsume(unsigned nNumBits);
	void		ScanBuffAdd(unsigned nNewByte,unsigned nPtr);
	void		ScanBuffAddErr(unsigned nNewByte,unsigned nPtr,unsigned nErr);

	// Decoder DC state
	signed		nDcLum;
	signed		nDcLumCss[4];
	signed		nDcChrCb;
	signed		nDcChrCr;

public: // For ImgMod
	unsigned	PackFileOffset(unsigned nByte,unsigned nBit);
	void		UnpackFileOffset(unsigned nPacked, unsigned &nByte, unsigned &nBit);
private:

	void		ChannelExtract(unsigned nMode,PixelCc &sSrc,PixelCc &sDst);
	void		CalcChannelPreviewFull(CRect* pRectView,unsigned char* pTmp);
	void		CalcChannelPreview();

public: // For Export
	unsigned char*	GetBitmapPtr();

	// Miscellaneous
	void		SetStatusText(CString str);
	void		SetStatusYccText(CString str);
	CString		GetStatusYccText();
	void		SetStatusMcuText(CString str);
	CString		GetStatusMcuText();
	void		SetStatusFilePosText(CString str);
	CString		GetStatusFilePosText();

	CString		Dec2Bin(unsigned nVal,unsigned nLen);
	void		ReportDctMatrix();
	void		ReportDctRGBMatrix();
	void		ReportVlc(unsigned nVlcPos, unsigned nVlcAlign,
						   unsigned nZrl, int nVal,
						   unsigned nCoeffStart,unsigned nCoeffEnd,
						   CString specialStr);
	void		PrintDcCumVal(unsigned nMcuX,unsigned nMcuY,int nVal);
	void		ReportDcRun(unsigned nMcuX, unsigned nMcuY, unsigned nMcuLen);


	// Member variables
public:

	// FIXME
	// For now make public until I can fix connection to the Lookup Dialog
	// Mapping between file position/bit offset for each MCU
	unsigned *	m_pMcuFileMap;
	unsigned	m_nMcuWidth;	// Width (pix) of MCU (e.g. 8,16)
	unsigned	m_nMcuHeight;	// Height (pix) of MCU (e.g. 8,16)
	unsigned	m_nMcuXMax;		// Number of MCUs across
	unsigned	m_nMcuYMax;		// Number of MCUs vertically
	unsigned	m_nBlkXMax;		// Number of 8x8 blocks across
	unsigned	m_nBlkYMax;		// Number of 8x8 blocks vertically

	// Fill these with the cumulative
	// values so that we can do a YCC to RGB conversion (for level
	// shift previews, etc.)
	short *		m_pPixValY;		// Pixel value
	short *		m_pPixValCb;	// Pixel value
	short *		m_pPixValCr;	// Pixel value

	// The following is used for under-cursor examination of cumulative DC values
	short *		m_pBlkValY;		// Blk DC value
	short *		m_pBlkValCb;	// Blk DC value
	short *		m_pBlkValCr;	// Blk DC value

	// FIXME Later, may use these to allow display of frequency
	// spectrum view of image data.
	short *		m_pBlkFreqY;	// 8x8 Block frequency value
	short *		m_pBlkFreqCb;	// 8x8 Block frequency value
	short *		m_pBlkFreqCr;	// 8x8 Block frequency value

	

	unsigned	m_nMarkersBlkNum;	// Number of 8x8 Block markers
	CPoint		m_ptMarkersBlk[10];	// MCU Markers


	// Array that indicates whether or not an MCU has been marked
	bool *		m_abMarkedMcuMap;
	unsigned	m_nMarkedMcuLastXY;

private:
	CStatusBar*		m_pStatBar;		// Link to status bar
	CString			m_strStatusYcc;		// Status bar text: YCC
	CString			m_strStatusMcu;		// Status bar text: MCU
	CString			m_strStatusFilePos;	// Status bar text: File Position

	unsigned		m_nImgSizeXPartMcu;	// Image size with possible partial MCU
	unsigned		m_nImgSizeYPartMcu;
	unsigned int	m_nImgSizeX;		// Image size rounding up to full MCU
	unsigned int	m_nImgSizeY;

	// Image rects
	CRect			m_rectImgBase;		// Image with no offset, no scaling
	CRect			m_rectImgReal;		// Image with scaling & offset
	CRect			m_rectHistBase;		// Hist with no offset
	CRect			m_rectHistReal;		// Hist with offset
	CRect			m_rectHistYBase;	// HistY with no offset
	CRect			m_rectHistYReal;	// HistY with offset


public:	//FIXME: Probably doesn't need to be public
	bool			m_bScanBad;				// Any errors found?
private:
	unsigned		m_nScanErrMax;		// Max # scan decode errors shown

	bool			my_DIB_ready;
	CDIB			my_DIBtmp;		// Temporary version for display

	bool			my_hist_rgb_DIB_ready;
	CDIB			my_hist_rgb_DIB;

	bool			my_hist_y_DIB_ready;
	CDIB			my_hist_y_DIB;

	bool			m_bMarkBlock;
	unsigned		m_nMarkBlockX;
	unsigned		m_nMarkBlockY;

public:
	bool			m_bDetailVlc;
	unsigned		m_nDetailVlcX;
	unsigned		m_nDetailVlcY;
	unsigned		m_nDetailVlcLen;

private:
	unsigned		m_nZoomMode;
public: // FIXME public for ImgView
	float			m_nZoom;
private:

	// Logger connection
	CDocLog*	m_pLog;
	CwindowBuf* m_pWBuf;
//	CJPEGsnoopDoc* m_pDoc;	// For calling UpdateAllViews()

	// Image details (from JFIF decode) set by SetImageDetails()
	bool		m_bImgDetailsSet;	//
public:
	unsigned	m_nCssX;			// Chroma subsampling in X
	unsigned	m_nCssY;			// Chroma subsampling in Y
private:
	unsigned	m_nDimX;			// Image Dimension in X
	unsigned	m_nDimY;			// Image Dimension in Y
	unsigned	m_nNumSosComps;		// Number of Image Components (DHT?)
	unsigned	m_nNumSofComps;		// Number of Image Components (DQT?)
	unsigned	m_nPrecision;		// 8-bit or 12-bit (defined in JFIF_SOF1)
	bool		m_bRestartEn;		// Did decoder see DRI?
	unsigned	m_nRestartInterval;	// ... if so, what is the MCU interval
	unsigned	m_nRestartRead;		// Number RST read during m_nScanBuff
	unsigned    m_nPreviewMode;		// Preview mode
	unsigned	m_nPreviewModeDone;	// Last preview mode calculated

	// Test shifting of YCC for ChannelPreview()
	int			m_nPreviewShiftY;
	int			m_nPreviewShiftCb;
	int			m_nPreviewShiftCr;
	unsigned	m_nPreviewShiftMcuX;	// Start co-ords of shift
	unsigned	m_nPreviewShiftMcuY;

	unsigned	m_nPreviewInsMcuX;	// Test blank MCU insert
	unsigned	m_nPreviewInsMcuY;
	unsigned	m_nPreviewInsMcuLen;



	// DQT Table (only DC component for now)
public: //
	unsigned	m_anDqtCoeff[MAX_DHT][64];		// Normal ordering
	unsigned	m_anDqtCoeffZz[MAX_DHT][64];	// Original zigzag ordering
	int			m_anDqtTblSel[8];
private:

	// ----------------
	// IDCT calcs
	void		PrecalcIdct();
	void		DecodeIdctClear();
	void		DecodeIdctSet(unsigned nTbl,unsigned num_coeffs,unsigned zrl,int val);
	void		DecodeIdctCalcFloat(unsigned nCoefMax);
	void		DecodeIdctCalcFixedpt();
	void		ClrFullRes();
	void		SetFullRes(unsigned nMcuX,unsigned nMcuY,unsigned nChan,unsigned nCssXInd,unsigned nCssYInd,int nDcOffset);

	bool		m_bDecodeScanAc;				// Mirror of App Config 

	// Brightest pixel detection
	int			m_nBrightY;
	int			m_nBrightCb;
	int			m_nBrightCr;
	unsigned	m_nBrightR;
	unsigned	m_nBrightG;
	unsigned	m_nBrightB;
	CPoint		m_ptBrightMcu;
	bool		m_bBrightValid;

	// Average pixel intensity
	long		m_nAvgY;
	bool		m_bAvgYValid;


	bool		m_bScanErrorsDisable;			// Disable scan errors reporting

	float		m_afIdctLookup[64][64];
	int			m_anIdctLookup[64][64];	// fixed pt version
	unsigned	m_nDctCoefMax;			// Largest DCT coeff to process
	int			m_anDctBlock[64];
	float		m_afIdctBlock[64];
	int			m_anIdctBlock[64];		// fixed pt version
	int			m_anImgFullres[4][2*8][2*8];	// Lum/Chr for 16x16 pixel region (2x2 MCUs)

	static const unsigned	m_sZigZag[];
	static const unsigned	m_sUnZigZag[];
	// ----------------

	// DHT Lookup table for real decode
	unsigned		m_anHuffMaskLookup[32];
	int				m_anDhtTblSel[2][MAX_DHT];
	unsigned		m_anDhtLookupSetMax[2];			// # of tables defined
	unsigned		m_anDhtLookupSize[2][MAX_DHT];		// Number of entries in each lookup table
	unsigned		m_anDhtLookup_bitlen[2][MAX_DHT][MAX_DHT_CODES]; // Number of compare bits in MSB
	unsigned		m_anDhtLookup_bits[2][MAX_DHT][MAX_DHT_CODES]; // Compare bits in MSB
	unsigned		m_anDhtLookup_mask[2][MAX_DHT][MAX_DHT_CODES]; // Mask of compare bits
	unsigned		m_anDhtLookup_code[2][MAX_DHT][MAX_DHT_CODES]; // Resulting code
	unsigned int	m_anDhtLookupfast[2][MAX_DHT][2<<DHT_FAST_SIZE];	// Fast direct lookup for codes <= 10 bits
	unsigned int	m_anDhtHisto[2][MAX_DHT][17];

	unsigned			m_nScanBuff;				// 32 bits of scan data after removing stuffs

	unsigned			m_nScanBuff_vacant;		// Bits unused in LSB after shifting (add if >= 8)
	unsigned long		m_nScanBuffPtr;			// Next byte position to load
	unsigned long		m_nScanBuffPtr_start;	// Saved first position of scan data

	bool				m_nScanCurErr;			// Mark as soon as error occurs
	unsigned			m_nScanBuffPtr_pos[4];	// File posn for each byte in buffer
	unsigned			m_nScanBuffPtr_err[4];	// Does this byte have an error?
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
	bool				m_bStatClipEn;	// UNUSED *** Enable scan clipping stats?
	unsigned			m_nNumPixels;
	PixelCcHisto		m_sHisto;
	unsigned			m_pCC_histo_r[HISTO_BINS];
	unsigned			m_pCC_histo_g[HISTO_BINS];
	unsigned			m_pCC_histo_b[HISTO_BINS];

	// For image similarity analysis
	unsigned			m_histo_y_full[FULL_HISTO_BINS];
	unsigned			m_histo_y_subset[FULL_HISTO_BINS];

public: // FIXME for ImgMod
	static const BYTE		m_anMaskByte[];
private:

	// For View management
	int			m_nPageHeight;
	int			m_nPageWidth;
public: // FIXME public for now for ImgView
	unsigned	m_nPreviewPosX;		// Top-left coord
	unsigned	m_nPreviewPosY;
	unsigned	m_nPreviewSizeX;
	unsigned	m_nPreviewSizeY;
private:

	bool		m_bViewOverlaysMcuGrid;	// Do we enable MCU Grid Overlay


};




