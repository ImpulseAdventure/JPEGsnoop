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

#ifndef IMGDECODE_H
#define IMGDECODE_H

#include <QMessageBox>
#include <QPainter>
#include <QPoint>
#include <QString>
#include <QStatusBar>


#include "snoop.h"

#include "DocLog.h"
#include "WindowBuf.h"

#include <map>

#include "General.h"

class CSnoopConfig;

// Color conversion clipping (YCC) reporting
#define YCC_CLIP_REPORT_ERR true        // Are YCC clips an error?
#define YCC_CLIP_REPORT_MAX 10  // Number of same error to report

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
#define	MAX_DHT_DEST_ID		4       // Maximum range for DHT Destination ID (ie. DHT:Th). Range 0..3
#define DHT_CLASS_DC		0
#define	DHT_CLASS_AC		1

#define MAX_DHT_CODES		260     // Was 180, but with 12-bit DHT, we may have nearly all 0xFF codes
#define	MAX_DHT_CODELEN		16      // Max number of bits in a code
#define	DHT_CODE_UNUSED		0xFFFFFFFF      // Mark table entry as unused

#define	MAX_DQT_DEST_ID		4       // Maximum range for DQT Destination ID (ie. DQT:Tq). Range 0..3
#define MAX_DQT_COEFF		64      // Number of coefficients in DQT matrix
#define MAX_DQT_COMP		256     // Max range of frame component identifier (SOF:Ci, 0..255)

#define DCT_COEFF_DC		0       // DCT matrix coefficient index for DC component

#define MAX_SAMP_FACT_H		4       // Maximum sampling factor supported
#define MAX_SAMP_FACT_V		4       // Maximum sampling factor supported

#define	DQT_DEST_Y			1
#define	DQT_DEST_CB			2
#define	DQT_DEST_CR			3
#define	DQT_DEST_K			4

#define	BLK_SZ_X			8       // JPEG block size (x)
#define	BLK_SZ_Y			8       // JPEG block size (y)

#define DCT_SZ_X			8       // IDCT matrix size (x)
#define DCT_SZ_Y			8       // IDCT matrix size (y)
#define	DCT_SZ_ALL			(DCT_SZ_X*DCT_SZ_Y)     // IDCT matrix all coeffs

#define IMG_BLK_SZ				1       // Size of each MCU in image display
#define MAX_SCAN_DECODED_DIM	512     // X & Y dimension for top-left image display
#define DHT_FAST_SIZE			9       // Number of bits for DHT direct lookup

// FIXME: MAX_SOF_COMP_NF per spec might actually be 255
#define MAX_SOF_COMP_NF			256     // Maximum number of Image Components in Frame (Nf) [from SOF] (Nf range 1..255)
#define MAX_SOS_COMP_NS			4       // Maximum number of Image Components in Scan (Ns) [from SOS] (Ns range 1..4)

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

#define CHAN_R				0
#define	CHAN_G				1
#define	CHAN_B				2

// Maximum number of blocks that can be marked
// - This feature is generally used to mark ranges for the detailed scan decode feature
#define MAX_BLOCK_MARKERS		10

// JFIF Markers relevant for Scan Decoder
// - Restart markers
// - EOI
static const uint32_t JFIF_RST0 = 0xD0;
static const uint32_t JFIF_RST1 = 0xD1;
static const uint32_t JFIF_RST2 = 0xD2;
static const uint32_t JFIF_RST3 = 0xD3;
static const uint32_t JFIF_RST4 = 0xD4;
static const uint32_t JFIF_RST5 = 0xD5;
static const uint32_t JFIF_RST6 = 0xD6;
static const uint32_t JFIF_RST7 = 0xD7;
static const uint32_t JFIF_EOI = 0xD9;

// Color correction clipping indicator
#define CC_CLIP_NONE		  0x00000000
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
static const int32_t HISTO_BINS = 255; // 128;
static const int32_t HISTO_BIN_WIDTH	= 1;
static const int32_t HISTO_BIN_HEIGHT_MAX = 60;

// Histogram of Y component (-1024..+1023) = 2048 bins
static const int32_t FULL_HISTO_BINS	= 2048;
static const int32_t SUBSET_HISTO_BINS	= 512;

// Image locations
static const int32_t nBorderLeft = 10;
static const int32_t nBorderBottom = 10;
static const int32_t nTitleHeight = 20;
static const int32_t nTitleIndent = 5;
static const int32_t nTitleLowGap = 3;

// Return values for ReadScanVal()
enum teRsvRet
{
  RSV_OK,                       // OK
  RSV_EOB,                      // End of Block
  RSV_UNDERFLOW,                // Ran out of data in buffer
  RSV_RST_TERM                  // No huffman code found, but restart marker seen
};

// Scan decode errors (in m_anScanBuffPtr_err[] buffer array)
enum teScanBufStatus
{
  SCANBUF_OK,
  SCANBUF_BADMARK,
  SCANBUF_RST
};

// Per-pixel color conversion structure
// - Records each stage of the process and associated clipping/ranging
typedef struct
{
  // Pre-ranged YCC
  // - Raw YCC value from pixel map before any adjustment
  // - Example range: -1024...1023
  int32_t nPrerangeY;
  int32_t nPrerangeCb;
  int32_t nPrerangeCr;

  // Pre-clip YCC
  // - After scaling pre-ranged values
  // - Typical range should be 0..255
  int32_t nPreclipY;
  int32_t nPreclipCb;
  int32_t nPreclipCr;

  // Final YCC
  // - After clipping to ensure we are in range 0..255
  uint8_t nFinalY;
  uint8_t nFinalCr;
  uint8_t nFinalCb;

  // Pre-clip RGB
  // - After color conversion and level shifting
  double nPreclipR;
  double nPreclipG;
  double nPreclipB;

  // Final RGB
  // - After clipping to 0..255
  uint8_t nFinalR;
  uint8_t nFinalG;
  uint8_t nFinalB;

  // Any YCC or RGB clipping performed?
  uint32_t nClip;               // Multi-bit indicator of clipping (CC_CLIP_*)
} PixelCc;

// Per-pixel stats: clipping (overflow and underflow) in YCC and RGB
typedef struct
{
  uint32_t nClipYUnder;
  uint32_t nClipYOver;
  uint32_t nClipCbUnder;
  uint32_t nClipCbOver;
  uint32_t nClipCrUnder;
  uint32_t nClipCrOver;
  uint32_t nClipRUnder;
  uint32_t nClipROver;
  uint32_t nClipGUnder;
  uint32_t nClipGOver;
  uint32_t nClipBUnder;
  uint32_t nClipBOver;
  uint32_t nClipWhiteOver;
} PixelCcClip;

// Min-max and average pixel stats for histogram
typedef struct
{
  int32_t nPreclipYMin;
  int32_t nPreclipYMax;
  int32_t nPreclipYSum;
  int32_t nPreclipCbMin;
  int32_t nPreclipCbMax;
  int32_t nPreclipCbSum;
  int32_t nPreclipCrMin;
  int32_t nPreclipCrMax;
  int32_t nPreclipCrSum;

  int32_t nClipYMin;
  int32_t nClipYMax;
  int32_t nClipYSum;
  int32_t nClipCbMin;
  int32_t nClipCbMax;
  int32_t nClipCbSum;
  int32_t nClipCrMin;
  int32_t nClipCrMax;
  int32_t nClipCrSum;

  int32_t nClipRMin;
  int32_t nClipRMax;
  int32_t nClipRSum;
  int32_t nClipGMin;
  int32_t nClipGMax;
  int32_t nClipGSum;
  int32_t nClipBMin;
  int32_t nClipBMax;
  int32_t nClipBSum;

  int32_t nPreclipRMin;
  int32_t nPreclipRMax;
  int32_t nPreclipRSum;
  int32_t nPreclipGMin;
  int32_t nPreclipGMax;
  int32_t nPreclipGSum;
  int32_t nPreclipBMin;
  int32_t nPreclipBMax;
  int32_t nPreclipBSum;

  uint32_t nCount;
} PixelCcHisto;

class CimgDecode : public QObject
{
  Q_OBJECT

public:
  CimgDecode(CDocLog * pLog, CwindowBuf * pWBuf, CSnoopConfig *pAppConfig, QObject *_parent);
  ~CimgDecode();

  void Reset();                 // Called during start of SOS decode
  void ResetState();            // Called at start of new JFIF Decode

  void SetStatusBar(QStatusBar *pStatBar);
  void DecodeScanImg(uint32_t bStart, bool bDisplay, bool bQuiet);

  void DrawHistogram(bool bQuiet, bool bDumpHistoY);
  void ReportHistogramY();
  void ReportColorStats();

  bool isYHistogramReady() {return m_bDibHistYReady; }
  bool isRgbHistogramReady() {return m_bDibHistRgbReady; }
  bool IsPreviewReady();

  // Config
  void SetImageDimensions(uint32_t nWidth, uint32_t nHeight);
  void SetImageDetails(uint32_t nDimX, uint32_t nDimY, uint32_t nCompsSOF, uint32_t nCompsSOS, bool bRstEn,
                       uint32_t nRstInterval);
  void SetSofSampFactors(uint32_t nCompInd, uint32_t nSampFactH, uint32_t nSampFactV);
  void ResetImageContent();

  bool SetDqtEntry(uint32_t nTblDestId, uint32_t nCoeffInd, uint32_t nCoeffIndZz, uint16_t nCoeffVal);
  bool SetDqtTables(uint32_t nCompInd, uint32_t nTbl);
  uint32_t GetDqtEntry(uint32_t nTblDestId, uint32_t nCoeffInd);
  bool SetDhtTables(uint32_t nCompInd, uint32_t nTblDc, uint32_t nTblAc);
  bool SetDhtEntry(uint32_t nDestId, uint32_t nClass, uint32_t nInd, uint32_t nLen,
                   uint32_t nBits, uint32_t nMask, uint32_t nCode);
  bool SetDhtSize(uint32_t nDestId, uint32_t nClass, uint32_t nSize);
  void SetPrecision(uint32_t nPrecision);

  // Modes -- accessed from Doc
//  void SetPreviewMode(uint32_t nMode);
  uint32_t GetPreviewMode();
  void SetPreviewYccOffset(uint32_t nMcuX, uint32_t nMcuY, int32_t nY, int32_t nCb, int32_t nCr);
  void GetPreviewYccOffset(uint32_t & nMcuX, uint32_t & nMcuY, int32_t &nY, int32_t &nCb, int32_t &nCr);
  void SetPreviewMcuInsert(uint32_t nMcuX, uint32_t nMcuY, int32_t nLen);
  void GetPreviewMcuInsert(uint32_t & nMcuX, uint32_t & nMcuY, uint32_t & nLen);
  void SetPreviewZoom(bool bInc, bool bDec, bool bSet, uint32_t nVal);
  uint32_t GetPreviewZoomMode();
  double GetPreviewZoom();
  bool GetPreviewOverlayMcuGrid();
  void SetPreviewOverlayMcuGridToggle();

  // Utilities
  void LookupFilePosPix(QPoint p, uint32_t &nByte, uint32_t &nBit);
  void LookupFilePosMcu(QPoint p, uint32_t &nByte, uint32_t &nBit);
  void LookupBlkYCC(QPoint p, int32_t &nY, int32_t &nCb, int32_t &nCr);

  void SetMarkerBlk(QPoint p);
  uint32_t GetMarkerCount();
  QPoint GetMarkerBlk(uint32_t nInd);

  QPoint PixelToMcu(QPoint ptPix);
  QPoint PixelToBlk(QPoint ptPix);
  uint32_t McuXyToLinear(QPoint ptMcu);
  void GetImageSize(uint32_t & nX, uint32_t & nY);

  // View helper routines
  void ViewOnDraw(QPainter * pDC, QRect rectClient, QPoint ptScrolledPos, QFont * pFont, QSize & szNewScrollSize);
  void ViewMcuOverlay(QPainter * pDC);
  void ViewMcuMarkedOverlay(QPainter * pDC);
  void ViewMarkerOverlay(QPainter * pDC, uint32_t nBlkX, uint32_t nBlkY);       // UNUSED?

  void GetPixMapPtrs(int16_t *&pMapY, int16_t *&pMapCb, int16_t *&pMapCr);
  void GetDetailVlc(bool & bDetail, uint32_t & nX, uint32_t & nY, uint32_t & nLen);
  void SetDetailVlc(bool bDetail, uint32_t nX, uint32_t nY, uint32_t nLen);

  void GetPreviewPos(uint32_t & nX, uint32_t & nY);
  void GetPreviewSize(uint32_t & nX, uint32_t & nY);

  void ScanErrorsDisable();
  void ScanErrorsEnable();

  uint32_t PackFileOffset(uint32_t nByte, uint32_t nBit);
  void UnpackFileOffset(uint32_t nPacked, uint32_t & nByte, uint32_t & nBit);

  void GetBitmapPtr(uint8_t * &pBitmap);

  // Miscellaneous
  void SetStatusText(QString strText);
  void SetStatusYccText(QString strText);
  QString GetStatusYccText();
  void SetStatusMcuText(QString strText);
  QString GetStatusMcuText();
  void SetStatusFilePosText(QString strText);
  QString GetStatusFilePosText();

  QImage *yHistogram() { return m_pDibHistY; }
  QImage *rgbHistogram() { return m_pDibHistRgb; }

  void ReportDctMatrix();
  void ReportDctYccMatrix();
  void ReportVlc(uint32_t nVlcPos, uint32_t nVlcAlign,
                 uint32_t nZrl, int32_t nVal, uint32_t nCoeffStart, uint32_t nCoeffEnd, QString specialStr);
  void PrintDcCumVal(uint32_t nMcuX, uint32_t nMcuY, int32_t nVal);
  void ReportDcRun(uint32_t nMcuX, uint32_t nMcuY, uint32_t nMcuLen);

  bool m_bDibTempReady;

  QImage *m_pDibTemp;              // Temporary version for display
  bool m_bPreviewIsJpeg;        // Is the preview image from decoded JPEG?

  // DQT Table
  uint16_t m_anDqtCoeff[MAX_DQT_DEST_ID][MAX_DQT_COEFF];  // Normal ordering
  uint16_t m_anDqtCoeffZz[MAX_DQT_DEST_ID][MAX_DQT_COEFF];        // Original zigzag ordering
  int32_t m_anDqtTblSel[MAX_DQT_COMP];      // DQT table selector for image component in frame

public slots:
  void setPreviewMode(QAction *action);

signals:
  void updateStatus(QString statusMsg, int);
  void updateImage();

private:
  CimgDecode &operator = (const CimgDecode&);
  CimgDecode(CimgDecode&);
  
  void ResetDqtTables();
  void ResetDhtLookup();

  QString GetScanBufPos();
  QString GetScanBufPos(uint32_t pos, uint32_t align);

  void ConvertYCCtoRGB(uint32_t nMcuX, uint32_t nMcuY, PixelCc & sPix);
  void ConvertYCCtoRGBFastFloat(PixelCc & sPix);
  void ConvertYCCtoRGBFastFixed(PixelCc & sPix);

  void ScanYccToRgb();
  void CapYccRange(uint32_t nMcuX, uint32_t nMcuY, PixelCc & sPix);
  void CapRgbRange(uint32_t nMcuX, uint32_t nMcuY, PixelCc & sPix);

  void GenLookupHuffMask();
  uint32_t ExtractBits(uint32_t nWord, uint32_t nBits);
  teRsvRet ReadScanVal(uint32_t nClass, uint32_t nTbl, uint32_t & rZrl, int32_t &rVal);
  bool DecodeScanComp(uint32_t nTblDhtDc, uint32_t nTblDhtAc, uint32_t nTblDqt, uint32_t nMcuX, uint32_t nMcuY);
  bool DecodeScanCompPrint(uint32_t nTblDhtDc, uint32_t nTblDhtAc, uint32_t nTblDqt, uint32_t nMcuX, uint32_t nMcuY);
  int32_t HuffmanDc2Signed(uint32_t nVal, uint32_t nBits);
  void CheckScanErrors(uint32_t nMcuX, uint32_t nMcuY, uint32_t nCssX, uint32_t nCssY, uint32_t nComp);

  bool ExpectRestart();
  void DecodeRestartDcState();
  void DecodeRestartScanBuf(uint32_t nFilePos, bool bRestart);
  uint32_t BuffAddByte();
  void BuffTopup();
  void ScanBuffConsume(uint32_t nNumBits);
  void ScanBuffAdd(uint32_t nNewByte, uint32_t nPtr);
  void ScanBuffAddErr(uint32_t nNewByte, uint32_t nPtr, uint32_t nErr);

  // IDCT calcs
  void PrecalcIdct();
  void DecodeIdctClear();
  void DecodeIdctSet(uint32_t nTbl, uint32_t num_coeffs, uint32_t zrl, int16_t val);
  void DecodeIdctCalcFloat(uint32_t nCoefMax);
  void DecodeIdctCalcFixedpt();
  void ClrFullRes(int32_t nWidth, int32_t nHeight);
  void SetFullRes(int32_t nMcuX, int32_t nMcuY, int32_t nComp, uint32_t nCssXInd, uint32_t nCssYInd, int16_t nDcOffset);

  void ChannelExtract(uint32_t nMode, PixelCc & sSrc, PixelCc & sDst);
  void CalcChannelPreviewFull(QRect * pRectView, uint8_t * pTmp);
  void CalcChannelPreview();

  // -------------------------------------------------------------
  // Member variables
  // -------------------------------------------------------------

  CDocLog *m_pLog;
  CwindowBuf *m_pWBuf;
  CSnoopConfig *m_pAppConfig;        // Pointer to application config

  QMessageBox msgBox;

  uint32_t *m_pMcuFileMap;
  int32_t m_nMcuWidth;         // Width (pix) of MCU (e.g. 8,16)
  int32_t m_nMcuHeight;        // Height (pix) of MCU (e.g. 8,16)
  int32_t m_nMcuXMax;          // Number of MCUs across
  int32_t m_nMcuYMax;          // Number of MCUs vertically
  int32_t m_nBlkXMax;          // Number of 8x8 blocks across
  int32_t m_nBlkYMax;          // Number of 8x8 blocks vertically

  // Fill these with the cumulative values so that we can do
  // a YCC to RGB conversion (for level shift previews, etc.)
  int16_t *m_pPixValY;            // Pixel value
  int16_t *m_pPixValCb;           // Pixel value
  int16_t *m_pPixValCr;           // Pixel value

  // Array of block DC values. Only used for under-cursor reporting.
  int16_t *m_pBlkDcValY;          // Block DC value
  int16_t *m_pBlkDcValCb;         // Block DC value
  int16_t *m_pBlkDcValCr;         // Block DC value

  // TODO: Later use these to support frequency spectrum
  // display of image data
  int16_t *m_pBlkFreqY;           // 8x8 Block frequency value
  int16_t *m_pBlkFreqCb;          // 8x8 Block frequency value
  int16_t *m_pBlkFreqCr;          // 8x8 Block frequency value

  // Array that indicates whether or not a block has been marked
  // This is generally used to mark ranges for the detailed scan decode feature
  uint32_t m_nMarkersBlkNum;    // Number of 8x8 Block markers
  QPoint m_aptMarkersBlk[MAX_BLOCK_MARKERS];    // MCU Markers

  QStatusBar *m_pStatBar;       // Link to status bar
  QString m_strStatusYcc;       // Status bar text: YCC
  QString m_strStatusMcu;       // Status bar text: MCU
  QString m_strStatusFilePos;   // Status bar text: File Position
  QString m_strTitle;           // Image title


  int32_t m_nImgSizeXPartMcu;  // Image size with possible partial MCU
  int32_t m_nImgSizeYPartMcu;
  int32_t m_nImgSizeX;         // Image size rounding up to full MCU
  int32_t m_nImgSizeY;

  // Image rects
  QRect m_rectImgBase;          // Image with no offset, no scaling
  QRect m_rectImgReal;          // Image with scaling & offset
  QRect m_rectHistBase;         // Hist with no offset
  QRect m_rectHistReal;         // Hist with offset
  QRect m_rectHistYBase;        // HistY with no offset
  QRect m_rectHistYReal;        // HistY with offset

  // Decoder DC state
  // Tables use signed to help flag undefined table indices
  int16_t m_nDcLum;
  int16_t m_nDcChrCb;
  int16_t m_nDcChrCr;
  int16_t m_anDcLumCss[MAX_SAMP_FACT_V * MAX_SAMP_FACT_H]; // Need 4x2 at least. Support up to 4x4
  int16_t m_anDcChrCbCss[MAX_SAMP_FACT_V * MAX_SAMP_FACT_H];
  int16_t m_anDcChrCrCss[MAX_SAMP_FACT_V * MAX_SAMP_FACT_H];

  bool m_bScanBad;              // Any errors found?
  uint32_t m_nScanErrMax;       // Max # scan decode errors shown

  bool m_bDibHistRgbReady;
  QImage *m_pDibHistRgb;

  bool m_bDibHistYReady;
  QImage *m_pDibHistY;

  bool m_bDetailVlc;
  uint32_t m_nDetailVlcX;
  uint32_t m_nDetailVlcY;
  uint32_t m_nDetailVlcLen;

  uint32_t m_nZoomMode;
  double m_nZoom;

  // Image details (from JFIF decode) set by SetImageDetails()
  bool m_bImgDetailsSet;        // Have image details been set yet (from SOS)
  int32_t m_nDimX;             // Image Dimension in X
  int32_t m_nDimY;             // Image Dimension in Y
  int32_t m_nNumSosComps;      // Number of Image Components (DHT?)
  int32_t m_nNumSofComps;      // Number of Image Components (DQT?)
  int32_t m_nPrecision;        // 8-bit or 12-bit (defined in JFIF_SOF1)
  int32_t m_anSofSampFactH[MAX_SOF_COMP_NF];   // Sampling factor per component ID in frame from SOF
  int32_t m_anSofSampFactV[MAX_SOF_COMP_NF];   // Sampling factor per component ID in frame from SOF
  int32_t m_nSosSampFactHMax;  // Maximum sampling factor for scan
  int32_t m_nSosSampFactVMax;  // Maximum sampling factor for scan
  int32_t m_nSosSampFactHMin;  // Minimum sampling factor for scan
  int32_t m_nSosSampFactVMin;  // Minimum sampling factor for scan
  int32_t m_anSampPerMcuH[MAX_SOF_COMP_NF];    // Number of samples of component ID per MCU
  int32_t m_anSampPerMcuV[MAX_SOF_COMP_NF];    // Number of samples of component ID per MCU
  int32_t m_anExpandBitsMcuH[MAX_SOF_COMP_NF]; // Number of bits to replicate in SetFullRes() due to sampling factor
  int32_t m_anExpandBitsMcuV[MAX_SOF_COMP_NF]; // Number of bits to replicate in SetFullRes() due to sampling factor

  bool m_bRestartEn;            // Did decoder see DRI?
  int32_t m_nRestartInterval;  // ... if so, what is the MCU interval
  int32_t m_nRestartRead;      // Number RST read during m_nScanBuff
  int32_t m_nPreviewMode;       // Preview mode

  // Test shifting of YCC for ChannelPreview()
  int32_t m_nPreviewShiftY;
  int32_t m_nPreviewShiftCb;
  int32_t m_nPreviewShiftCr;
  int32_t m_nPreviewShiftMcuX; // Start co-ords of shift
  int32_t m_nPreviewShiftMcuY;

  uint32_t m_nPreviewInsMcuX;   // Test blank MCU insert
  uint32_t m_nPreviewInsMcuY;
  uint32_t m_nPreviewInsMcuLen;

  bool m_bDecodeScanAc;       // User request decode of AC components?

  // Brightest pixel detection
  int32_t m_nBrightY;
  int32_t m_nBrightCb;
  int32_t m_nBrightCr;
  uint32_t m_nBrightR;
  uint32_t m_nBrightG;
  uint32_t m_nBrightB;
  QPoint m_ptBrightMcu;
  bool m_bBrightValid;

  // Average pixel intensity
  int32_t m_nAvgY;
  bool m_bAvgYValid;

  bool m_bScanErrorsDisable;    // Disable scan errors reporting

  // Temporary processing of IDCT per block
  double m_afIdctLookup[DCT_SZ_ALL][DCT_SZ_ALL]; // IDCT lookup table (doubleing point)
  int32_t m_anIdctLookup[DCT_SZ_ALL][DCT_SZ_ALL];   // IDCT lookup table (fixed point)
  uint32_t m_nDctCoefMax;       // Largest DCT coeff to process
  int16_t m_anDctBlock[DCT_SZ_ALL];        // Input block for IDCT process
  double m_afIdctBlock[DCT_SZ_ALL];      // Output block after IDCT (via doubleing point)
  int32_t m_anIdctBlock[DCT_SZ_ALL];        // Output block after IDCT (via fixed point)

  // DHT Lookup table for real decode
  // Note: Component destination index is 1-based; first entry [0] is unused
  int32_t m_anDhtTblSel[MAX_DHT_CLASS][1 + MAX_SOS_COMP_NS];        // DHT table selected for image component index (1..4)
  uint32_t m_anHuffMaskLookup[32];
  // Huffman lookup table for current scan
  uint32_t m_anDhtLookupSetMax[MAX_DHT_CLASS];  // Highest DHT table index (ie. 0..3) per class
  uint32_t m_anDhtLookupSize[MAX_DHT_CLASS][MAX_DHT_DEST_ID];   // Number of entries in each lookup table
  uint32_t m_anDhtLookup_bitlen[MAX_DHT_CLASS][MAX_DHT_DEST_ID][MAX_DHT_CODES]; // Number of compare bits in MSB
  uint32_t m_anDhtLookup_bits[MAX_DHT_CLASS][MAX_DHT_DEST_ID][MAX_DHT_CODES];   // Compare bits in MSB
  uint32_t m_anDhtLookup_mask[MAX_DHT_CLASS][MAX_DHT_DEST_ID][MAX_DHT_CODES];   // Mask of compare bits
  uint32_t m_anDhtLookup_code[MAX_DHT_CLASS][MAX_DHT_DEST_ID][MAX_DHT_CODES];   // Resulting code
  uint32_t m_anDhtLookupfast[MAX_DHT_CLASS][MAX_DHT_DEST_ID][2 << DHT_FAST_SIZE];       // Fast direct lookup for codes <= 10 bits
  uint32_t m_anDhtHisto[MAX_DHT_CLASS][MAX_DHT_DEST_ID][MAX_DHT_CODELEN + 1];
  // Note: MAX_DHT_CODELEN is +1 because this array index is 1-based since there are no codes of length 0 bits

  uint32_t m_nScanBuff;         // 32 bits of scan data after removing stuffs

  uint32_t m_nScanBuff_vacant;  // Bits unused in LSB after shifting (add if >= 8)
  uint32_t m_nScanBuffPtr; // Next uint8_t position to load
  uint32_t m_nScanBuffPtr_start;   // Saved first position of scan data (reset by RSTn markers)
  uint32_t m_nScanBuffPtr_first;   // Saved first position of scan data in file (not reset by RSTn markers). For comp ratio.

  bool m_nScanCurErr;           // Mark as soon as error occurs
  uint32_t m_anScanBuffPtr_pos[4];      // File posn for each uint8_t in buffer
  uint32_t m_anScanBuffPtr_err[4];      // Does this uint8_t have an error?
  uint32_t m_nScanBuffLatchErr;
  uint32_t m_nScanBuffPtr_num;  // Number of uint8_ts in buffer
  uint32_t m_nScanBuffPtr_align;        // Bit alignment in file for 1st uint8_t in buffer
  bool m_bScanEnd;              // Reached end of scan segment?

  bool m_bRestartRead;          // Have we seen a restart marker?
  uint32_t m_nRestartLastInd;   // Last Restart marker read (0..7)
  uint32_t m_nRestartExpectInd; // Next Restart marker expected (0..7)
  uint32_t m_nRestartMcusLeft;  // How many MCUs until next RST?

  bool m_bSkipDone;
  uint32_t m_nSkipCount;
  uint32_t m_nSkipData;

  bool m_bVerbose;
  uint32_t m_nWarnYccClipNum;
  uint32_t m_nWarnBadScanNum;

  uint32_t m_nScanBitsUsed1;
  uint32_t m_nScanBitsUsed2;

  // Image Analysis, histograms
  bool m_bHistEn;               // Histograms enabled? (by AppConfig)
  bool m_bStatClipEn;           // UNUSED: Enable scan clipping stats?
  uint32_t m_nNumPixels;
  PixelCcHisto m_sHisto;        // YCC/RGB histogram (min/max/avg)
  PixelCcClip m_sStatClip;      // YCC/RGB clipping stats

  uint32_t m_anCcHisto_r[HISTO_BINS];
  uint32_t m_anCcHisto_g[HISTO_BINS];
  uint32_t m_anCcHisto_b[HISTO_BINS];

  // For image similarity analysis
  uint32_t m_anHistoYFull[FULL_HISTO_BINS];
  uint32_t m_anHistoYSubset[FULL_HISTO_BINS];

  // For View management
  int32_t m_nPageHeight;
  int32_t m_nPageWidth;

  uint32_t m_nPreviewPosX;      // Top-left coord of preview image
  uint32_t m_nPreviewPosY;      // Top-left coord of preview image
  uint32_t m_nPreviewSizeX;     // X dimension of preview image
  uint32_t m_nPreviewSizeY;     // Y dimension of preview image

  bool m_bViewOverlaysMcuGrid;  // Do we enable MCU Grid Overlay
};

#endif
