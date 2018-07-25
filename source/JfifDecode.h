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
// - This module decodes the JPEG JFIF marker segments
// - Decoding the scan segment (SOS) is not handled here as that is
//   done in the CimgDecode class.
//
// ==========================================================================

#ifndef JFIFDECODE_H
#define JFIFDECODE_H

#include <QMessageBox>
#include <QObject>
#include <QString>

#include "ImgDecode.h"
#include "DecodePs.h"
#include "DecodeDicom.h"
#include "WindowBuf.h"
//#include "snoop.h"

class CDocLog;
class CSnoopConfig;
class CDbSigs;

// Disable DICOM support until fully tested
//#define SUPPORT_DICOM

static const int32_t MAX_IFD_COMPS = 150;        // Maximum number of IFD entry components to display

static const uint32_t JFIF_SOF0 = 0xC0;
static const uint32_t JFIF_SOF1 = 0xC1;
static const uint32_t JFIF_SOF2 = 0xC2;
static const uint32_t JFIF_SOF3 = 0xC3;
static const uint32_t JFIF_SOF5 = 0xC5;
static const uint32_t JFIF_SOF6 = 0xC6;
static const uint32_t JFIF_SOF7 = 0xC7;
static const uint32_t JFIF_JPG = 0xC8;
static const uint32_t JFIF_SOF9 = 0xC9;
static const uint32_t JFIF_SOF10 = 0xCA;
static const uint32_t JFIF_SOF11 = 0xCB;
static const uint32_t JFIF_SOF13 = 0xCD;
static const uint32_t JFIF_SOF14 = 0xCE;
static const uint32_t JFIF_SOF15 = 0xCF;
static const uint32_t JFIF_DHT = 0xC4;
static const uint32_t JFIF_DAC = 0xCC;
//static const uint32_t JFIF_RST0 = 0xD0;
//static const uint32_t JFIF_RST1 = 0xD1;
//static const uint32_t JFIF_RST2 = 0xD2;
//static const uint32_t JFIF_RST3 = 0xD3;
//static const uint32_t JFIF_RST4 = 0xD4;
//static const uint32_t JFIF_RST5 = 0xD5;
//static const uint32_t JFIF_RST6 = 0xD6;
//static const uint32_t JFIF_RST7 = 0xD7;
static const uint32_t JFIF_SOI = 0xD8;
//static const uint32_t JFIF_EOI = 0xD9;
static const uint32_t JFIF_SOS = 0xDA;
static const uint32_t JFIF_DQT = 0xDB;
static const uint32_t JFIF_DNL = 0xDC;
static const uint32_t JFIF_DRI = 0xDD;
static const uint32_t JFIF_DHP = 0xDE;
static const uint32_t JFIF_EXP = 0xDF;
static const uint32_t JFIF_APP0 = 0xE0;
static const uint32_t JFIF_APP1 = 0xE1;
static const uint32_t JFIF_APP2 = 0xE2;
static const uint32_t JFIF_APP3 = 0xE3;
static const uint32_t JFIF_APP4 = 0xE4;
static const uint32_t JFIF_APP5 = 0xE5;
static const uint32_t JFIF_APP6 = 0xE6;
static const uint32_t JFIF_APP7 = 0xE7;
static const uint32_t JFIF_APP8 = 0xE8;
static const uint32_t JFIF_APP9 = 0xE9;
static const uint32_t JFIF_APP10 = 0xEA;
static const uint32_t JFIF_APP11 = 0xEB;
static const uint32_t JFIF_APP12 = 0xEC;
static const uint32_t JFIF_APP13 = 0xED;
static const uint32_t JFIF_APP14 = 0xEE;
static const uint32_t JFIF_APP15 = 0xEF;
static const uint32_t JFIF_JPG0 = 0xF0;
static const uint32_t JFIF_JPG1 = 0xF1;
static const uint32_t JFIF_JPG2 = 0xF2;
static const uint32_t JFIF_JPG3 = 0xF3;
static const uint32_t JFIF_JPG4 = 0xF4;
static const uint32_t JFIF_JPG5 = 0xF5;
static const uint32_t JFIF_JPG6 = 0xF6;
static const uint32_t JFIF_JPG7 = 0xF7;
static const uint32_t JFIF_JPG8 = 0xF8;
static const uint32_t JFIF_JPG9 = 0xF9;
static const uint32_t JFIF_JPG10 = 0xFA;
static const uint32_t JFIF_JPG11 = 0xFB;
static const uint32_t JFIF_JPG12 = 0xFC;
static const uint32_t JFIF_JPG13 = 0xFD;
static const uint32_t JFIF_COM = 0xFE;
static const uint32_t JFIF_TEM = 0x01;
static const uint32_t JFIF_DHT_FAKE = 0x999999C4;
static const uint32_t JFIF_DHT_FAKE_SZ = 0x1A4;

#define APP14_COLXFM_UNSET		-1
#define APP14_COLXFM_UNK_RGB	 0
#define APP14_COLXFM_YCC		   1
#define APP14_COLXFM_YCCK		   2

static const int32_t MAX_IDENTIFIER = 256;       // Max length for identifier strings (include terminator)

struct CStr2
{
  QString strTag;
  QString strVal;
  bool bUnknown;                // Tag is not known
};

struct MarkerNameTable
{
  uint32_t nCode;
  QString strName;
};

class CjfifDecode : public QObject
{
    Q_OBJECT

  // Constructor & Initialization
public:
  CjfifDecode(CDocLog * pLog, CDbSigs *pDbSigs, CwindowBuf * pWBuf, CimgDecode * pImgDec, CSnoopConfig *pAppConfig, QObject *_parent = 0);
  ~CjfifDecode();

  void Reset();

  // --------------------------------------
  // Class Methods
  // --------------------------------------

  // Public accesssor & mutator functions
  void GetAviMode(bool & bIsAvi, bool & bIsMjpeg);
  void SetAviMode(bool bIsAvi, bool bIsMjpeg);
  uint32_t GetPosEmbedStart();
  uint32_t GetPosEmbedEnd();
  void GetDecodeSummary(QString & strHash, QString & strHashRot, QString & strImgExifMake, QString & strImgExifModel,
                        QString & strImgQualExif, QString & strSoftware, teDbAdd & eDbReqSuggest);
  uint32_t GetDqtZigZagIndex(uint32_t nInd, bool bZigZag);
  uint32_t GetDqtQuantStd(uint32_t nInd);

  bool GetDecodeStatus();

  //      void                    ExportRangeSet(uint32_t nStart, uint32_t nEnd);
    bool ExportJpegPrepare(QString strFileIn, bool bForceSoi, bool bForceEoi, bool bIgnoreEoi);
  bool ExportJpegDo(QString strFileIn, QString strFileOut, uint32_t nFileLen,
                    bool bOverlayEn, bool bDhtAviInsert, bool bForceSoi, bool bForceEoi);

  void ProcessFile(QFile * inFile);

  // UI elements
  void SetStatusBar(QStatusBar * pStatBar);

  // Signature database:
  void PrepareSendSubmit(QString strQual, teSource eUserSource, QString strUserSoftware, QString strUserNotes);

signals:
  void updateStatus(QString statusMsg, int);

public slots:
  void ImgSrcChanged();

private:
  CjfifDecode &operator = (const CjfifDecode&);
  CjfifDecode(CjfifDecode&);
  
  void PrepareSignature();
  void PrepareSignatureSingle(bool bRotate);
  void PrepareSignatureThumb();
  void PrepareSignatureThumbSingle(bool bRotate);
  bool CompareSignature(bool bQuiet);
  void SendSubmit(QString strExifMake, QString strExifModel, QString strQual,
                  QString strDqt0, QString strDqt1, QString strDqt2, QString strDqt3,
                  QString strCss,
                  QString strSig, QString strSigRot, QString strSigThumb,
                  QString strSigThumbRot, double fQFact0, double fQFact1, uint32_t nImgW, uint32_t nImgH,
                  QString strExifSoftware, QString strComment, teMaker eMaker,
                  teSource eUserSource, QString strUserSoftware, QString strExtra,
                  QString strUserNotes, uint32_t nExifLandscape, uint32_t nThumbX, uint32_t nThumbY);
  void OutputSpecial();

  // Display routines
  void DbgAddLine(QString strLine);
  void AddHeader(uint32_t nCode);
  QString PrintAsHexUC(uint8_t *anBytes, uint32_t nCount);
  QString PrintAsHex8(uint32_t *anBytes, uint32_t nCount);
  QString PrintAsHex32(uint32_t *anWords, uint32_t nCount);

  // Buffer access
  quint8 Buf(uint32_t nOffset, bool bClean);
  void UnByteSwap4(uint32_t nVal, uint32_t &nByte0, uint32_t &nByte1, uint32_t &nByte2, uint32_t &nByte3);
  uint32_t ByteSwap4(uint32_t nByte0, uint32_t nByte1, uint32_t nByte2, uint32_t nByte3);
  uint32_t ByteSwap2(uint32_t nByte0, uint32_t nByte1);
  uint32_t ReadSwap2(uint32_t nPos);
  uint32_t ReadSwap4(uint32_t nPos);
  uint32_t ReadBe4(uint32_t nPos);

  bool ExportJpegDoRange(QString strFileIn, QString strFileOut, uint32_t nStart, uint32_t nEnd);

  uint32_t DecodeMarker();
  bool ExpectMarkerEnd(uint32_t nMarkerStart, uint32_t nMarkerLen);
  void DecodeEmbeddedThumb();
  bool DecodeAvi();

  bool ValidateValue(uint32_t &nVal, uint32_t nMin, uint32_t nMax, QString strName, bool bOverride, uint32_t nOverrideVal);

  // Marker specific parsing
  bool GetMarkerName(uint32_t nCode, QString & markerStr);
  uint32_t DecodeExifIfd(QString strIfd, uint32_t nPosExifStart, uint32_t nStartIfdPtr);
//      uint32_t                DecodeMakerIfd(uint32_t ifd_tag,uint32_t ptr,uint32_t len);
  bool DecodeMakerSubType();
  void DecodeDHT(bool bInject);
  uint32_t DecodeApp13Ps();
  uint32_t DecodeApp2Flashpix();
  uint32_t DecodeApp2IccProfile(uint32_t nLen);
  uint32_t DecodeIccHeader(uint32_t nPos);

  // DQT / DHT
  void ClearDQT();
  void SetDQTQuick(uint16_t anDqt0[], uint16_t anDqt1[]);
  void GenLookupHuffMask();

  // Field parsing
  bool DecodeValRational(uint32_t nPos, double &nVal);
  QString DecodeValFraction(uint32_t nPos);
  bool DecodeValGPS(uint32_t nPos, QString & strCoord);
  bool PrintValGPS(uint32_t nCount, double fCoord1, double fCoord2, double fCoord3, QString & strCoord);
  QString DecodeIccDateTime(uint32_t anVal[3]);
  QString LookupExifTag(QString strSect, uint32_t nTag, bool & bUnknown);
  CStr2 LookupMakerCanonTag(uint32_t nMainTag, uint32_t nSubTag, uint32_t nVal);

  void SetStatusText(QString strText);
  void DecodeErrCheck(bool bRet);

  // --------------------------------------
  // Class variables
  // --------------------------------------

  // Pointers
  CDocLog *m_pLog;
  CDbSigs *m_pDbSigs;
  CwindowBuf *m_pWBuf;
  CimgDecode *m_pImgDec;
  CSnoopConfig * m_pAppConfig;
  CDecodePs *m_pPsDec;
  CDecodeDicom *m_pDecDicom;

  bool m_bVerbose;
  bool m_bOutputDB;
  bool m_bBufFakeDHT;           // Flag to redirect DHT read to AVI DHT over Buffer content


  // UI elements & log
  QMessageBox msgBox;
  QStatusBar *m_pStatBar;       // Link to status bar

  // Constants
  static const uint8_t m_abMJPGDHTSeg[JFIF_DHT_FAKE_SZ]; // Motion JPEG DHT
  static const MarkerNameTable m_pMarkerNames[];

  // Status
  bool m_bImgOK;                // Img decode encounter SOF
  bool m_bAvi;                  // Is it an AVI file?
  bool m_bAviMjpeg;             // Is it a MotionJPEG AVI file?
  bool m_bPsd;                  // Is it a Photoshop file?

  bool m_pImgSrcDirty;          // Do we need to recalculate the scan decode?

  // File position records
  uint32_t m_nPos;         // Current file/buffer position
  uint32_t m_nPosEoi;      // Position of EOI (0xFFD9) marker
  uint32_t m_nPosSos;
  uint32_t m_nPosEmbedStart;       // Embedded/offset start
  uint32_t m_nPosEmbedEnd; // Embedded/offset end
  uint32_t m_nPosFileEnd;  // End of file position

  // Decoder state
  char m_acApp0Identifier[MAX_IDENTIFIER];      // APP0 type: JFIF, AVI1, etc.

  double m_afStdQuantLumCompare[64];
  double m_afStdQuantChrCompare[64];

  uint32_t m_anMaskLookup[32];

  uint32_t m_nImgVersionMajor;
  uint32_t m_nImgVersionMinor;
  uint32_t m_nImgUnits;
  uint32_t m_nImgDensityX;
  uint32_t m_nImgDensityY;
  uint32_t m_nImgThumbSizeX;
  uint32_t m_nImgThumbSizeY;

  bool m_bImgProgressive;       // Progressive scan?
  bool m_bImgSofUnsupported;    // SOF mode unsupported - skip SOI content

  QString m_strComment;         // Comment string

  uint32_t m_nSosNumCompScan_Ns;
  uint32_t m_nSosSpectralStart_Ss;
  uint32_t m_nSosSpectralEnd_Se;
  uint32_t m_nSosSuccApprox_A;

  bool m_nImgRstEn;             // DRI seen
  uint32_t m_nImgRstInterval;

  uint16_t m_anImgDqtTbl[MAX_DQT_DEST_ID][MAX_DQT_COEFF];
  double m_adImgDqtQual[MAX_DQT_DEST_ID];
  bool m_abImgDqtSet[MAX_DQT_DEST_ID];  // Has this table been configured?
  uint32_t m_anDhtNumCodesLen_Li[17];

  uint32_t m_nSofPrecision_P;
  uint32_t m_nSofNumLines_Y;
  uint32_t m_nSofSampsPerLine_X;
  uint32_t m_nSofNumComps_Nf;   // Number of components in frame (might not equal m_nSosNumCompScan_Ns)

  // Define Quantization table details for the indexed Component Identifier
  // - Component identifier (SOF:Ci) has a range of 0..255
  uint32_t m_anSofQuantCompId[MAX_SOF_COMP_NF]; // SOF:Ci, index is i-1
  uint32_t m_anSofQuantTblSel_Tqi[MAX_SOF_COMP_NF];
  uint32_t m_anSofHorzSampFact_Hi[MAX_SOF_COMP_NF];
  uint32_t m_anSofVertSampFact_Vi[MAX_SOF_COMP_NF];
  uint32_t m_nSofHorzSampFactMax_Hmax;
  uint32_t m_nSofVertSampFactMax_Vmax;

  // FIXME: Move to CDecodePs
  uint32_t m_nImgQualPhotoshopSa;
  uint32_t m_nImgQualPhotoshopSfw;

  int m_nApp14ColTransform;     // Color transform from JFIF APP14 Adobe marker (-1 if not set)

  teLandscape m_eImgLandscape;  // Landscape vs Portrait
  QString m_strImgQuantCss;     // Chroma subsampling (e.g. "2x1")

  uint32_t m_nImgExifEndian;    // 0=Intel 1=Motorola
  uint32_t m_nImgExifSubIfdPtr;
  uint32_t m_nImgExifGpsIfdPtr;
  uint32_t m_nImgExifInteropIfdPtr;
  uint32_t m_nImgExifMakerPtr;

  bool m_bImgExifMakeSupported; // Mark makes that we support decode for
  uint32_t m_nImgExifMakeSubtype;       // Used for Nikon (e.g. type3)

  QString m_strImgExtras;       // Extra strings used for DB submission

  // Embedded EXIF Thumbnail
  uint32_t m_nImgExifThumbComp;
  uint32_t m_nImgExifThumbOffset;
  uint32_t m_nImgExifThumbLen;
  uint32_t m_anImgThumbDqt[4][64];
  bool m_abImgDqtThumbSet[4];
  QString m_strHashThumb;
  QString m_strHashThumbRot;
  uint32_t m_nImgThumbNumLines;
  uint32_t m_nImgThumbSampsPerLine;

  // Signature handling
  bool m_bSigExactInDB;         // Is current entry already in local DB?

  // State of decoder -- have we seen each marker?
  bool m_bStateAbort;           // Do we abort decoding? (eg. user hits cancel after errs)

  bool m_bStateSoi;
  bool m_bStateDht;
  bool m_bStateDhtOk;
  bool m_bStateDhtFake;         // Fake DHT required for AVI
  bool m_bStateDqt;
  bool m_bStateDqtOk;
  bool m_bStateSof;
  bool m_bStateSofOk;
  bool m_bStateSos;
  bool m_bStateSosOk;
  bool m_bStateEoi;

  teDbAdd m_eDbReqSuggest;
  QString m_strHash;
  QString m_strHashRot;
  QString m_strImgExifMake;
  QString m_strImgExifModel;
  QString m_strImgQualExif;     // Quality (e.g. "fine") from makernotes
  QString m_strSoftware;        // EXIF Software field
  bool m_bImgExifMakernotes;    // Are any Makernotes present?

  teEdited m_eImgEdited;        // Image edited? 0 = unset, 1 = yes, etc.
};

#endif
