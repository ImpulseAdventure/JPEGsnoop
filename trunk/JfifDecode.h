// JPEGsnoop - JPEG Image Decoder & Analysis Utility
// Copyright (C) 2015 - Calvin Hass
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

#pragma once

#include "DocLog.h"
#include "ImgDecode.h"
#include "DecodePs.h"
#include "DecodeDicom.h"
#include "WindowBuf.h"
#include "snoop.h"
#include "SnoopConfig.h"

#include "DbSigs.h"


// Disable DICOM support until fully tested
//#define SUPPORT_DICOM

#define MAX_IFD_COMPS			150	// Maximum number of IFD entry components to display

#define JFIF_SOF0	0xC0
#define JFIF_SOF1	0xC1
#define JFIF_SOF2	0xC2
#define JFIF_SOF3	0xC3
#define JFIF_SOF5	0xC5
#define JFIF_SOF6	0xC6
#define JFIF_SOF7	0xC7
#define JFIF_JPG	0xC8
#define JFIF_SOF9	0xC9
#define JFIF_SOF10	0xCA
#define JFIF_SOF11	0xCB
#define JFIF_SOF13	0xCD
#define JFIF_SOF14	0xCE
#define JFIF_SOF15	0xCF
#define JFIF_DHT	0xC4
#define JFIF_DAC	0xCC
#define JFIF_RST0	0xD0
#define JFIF_RST1	0xD1
#define JFIF_RST2	0xD2
#define JFIF_RST3	0xD3
#define JFIF_RST4	0xD4
#define JFIF_RST5	0xD5
#define JFIF_RST6	0xD6
#define JFIF_RST7	0xD7
#define JFIF_SOI	0xD8
#define JFIF_EOI	0xD9
#define JFIF_SOS	0xDA
#define JFIF_DQT	0xDB
#define JFIF_DNL	0xDC
#define JFIF_DRI	0xDD
#define JFIF_DHP	0xDE
#define JFIF_EXP	0xDF
#define JFIF_APP0	0xE0
#define JFIF_APP1	0xE1
#define JFIF_APP2	0xE2
#define JFIF_APP3	0xE3
#define JFIF_APP4	0xE4
#define JFIF_APP5	0xE5
#define JFIF_APP6	0xE6
#define JFIF_APP7	0xE7
#define JFIF_APP8	0xE8
#define JFIF_APP9	0xE9
#define JFIF_APP10	0xEA
#define JFIF_APP11	0xEB
#define JFIF_APP12	0xEC
#define JFIF_APP13	0xED
#define JFIF_APP14	0xEE
#define JFIF_APP15	0xEF
#define JFIF_JPG0	0xF0
#define JFIF_JPG1	0xF1
#define JFIF_JPG2	0xF2
#define JFIF_JPG3	0xF3
#define JFIF_JPG4	0xF4
#define JFIF_JPG5	0xF5
#define JFIF_JPG6	0xF6
#define JFIF_JPG7	0xF7
#define JFIF_JPG8	0xF8
#define JFIF_JPG9	0xF9
#define JFIF_JPG10	0xFA
#define JFIF_JPG11	0xFB
#define JFIF_JPG12	0xFC
#define JFIF_JPG13	0xFD
#define JFIF_COM	0xFE
#define JFIF_TEM	0x01

#define JFIF_DHT_FAKE		0x999999C4
#define JFIF_DHT_FAKE_SZ	0x1A4

#define APP14_COLXFM_UNSET		-1
#define APP14_COLXFM_UNK_RGB	0
#define APP14_COLXFM_YCC		1
#define APP14_COLXFM_YCCK		2

#define MAX_IDENTIFIER			256		// Max length for identifier strings (include terminator)

struct CStr2 {
	CString		strTag;
	CString		strVal;
	bool		bUnknown;	// Tag is not known
};


struct MarkerNameTable {
	unsigned	nCode;
	LPTSTR		strName;
};


class CjfifDecode
{
	// Constructor & Initialization
	public:
	CjfifDecode(CDocLog* pLog,CwindowBuf* pWBuf,CimgDecode* pImgDec);
	~CjfifDecode();

public:
	void		Reset();

	// --------------------------------------
	// Class Methods
	// --------------------------------------

	// Public accesssor & mutator functions
public:
	void			GetAviMode(bool &bIsAvi,bool &bIsMjpeg);
	void			SetAviMode(bool bIsAvi,bool bIsMjpeg);
	void			ImgSrcChanged();
	unsigned long	GetPosEmbedStart();
	unsigned long	GetPosEmbedEnd();
	void			GetDecodeSummary(CString &strHash,CString &strHashRot,CString &strImgExifMake,CString &strImgExifModel,
										CString &strImgQualExif,CString &strSoftware,teDbAdd &eDbReqSuggest);
	unsigned		GetDqtZigZagIndex(unsigned nInd,bool bZigZag);
	unsigned		GetDqtQuantStd(unsigned nInd);

	bool			GetDecodeStatus();

private:


	// Export Operations
public:
//	void			ExportRangeSet(unsigned nStart, unsigned nEnd);
	bool			ExportJpegPrepare(CString strFileIn,bool bForceSoi,bool bForceEoi,bool bIgnoreEoi);
	bool			ExportJpegDo(CString strFileIn, CString strFileOut, unsigned long nFileLen,
						bool bOverlayEn, bool bDhtAviInsert,bool bForceSoi,bool bForceEoi);
private:
	bool			ExportJpegDoRange(CString strFileIn, CString strFileOut, 
						unsigned long nStart, unsigned long nEnd);


	// General parsing
public:
	void			ProcessFile(CFile* inFile);
private:
	unsigned		DecodeMarker();
	bool			ExpectMarkerEnd(unsigned long nMarkerStart,unsigned nMarkerLen);
	void			DecodeEmbeddedThumb();
	bool			DecodeAvi();

	bool			ValidateValue(unsigned &nVal,unsigned nMin,unsigned nMax,CString strName,bool bOverride,unsigned nOverrideVal);

	// Marker specific parsing
	bool			GetMarkerName(unsigned nCode,CString &markerStr);
	unsigned		DecodeExifIfd(CString strIfd,unsigned nPosExifStart,unsigned nStartIfdPtr);
//	unsigned		DecodeMakerIfd(unsigned ifd_tag,unsigned ptr,unsigned len);
	bool			DecodeMakerSubType();
	void			DecodeDHT(bool bInject);
	unsigned		DecodeApp13Ps();
	unsigned		DecodeApp2Flashpix();
	unsigned		DecodeApp2IccProfile(unsigned nLen);
	unsigned		DecodeIccHeader(unsigned nPos);

	// DQT / DHT
	void			ClearDQT();
	void			SetDQTQuick(unsigned anDqt0[64],unsigned anDqt1[64]);
	void			GenLookupHuffMask();

	// Field parsing
	bool			DecodeValRational(unsigned nPos,float &nVal);
	CString			DecodeValFraction(unsigned nPos);
	bool			DecodeValGPS(unsigned nPos,CString &strCoord);
	bool			PrintValGPS(unsigned nCount, float fCoord1, float fCoord2, float fCoord3,CString &strCoord);
	CString			DecodeIccDateTime(unsigned anVal[3]);
	CString			LookupExifTag(CString strSect, unsigned nTag, bool &bUnknown);
	CStr2			LookupMakerCanonTag(unsigned nMainTag,unsigned nSubTag,unsigned nVal);


	// Signature database
public:
	void			PrepareSendSubmit(CString strQual,teSource eUserSource,CString strUserSoftware,CString strUserNotes);
private:
	void			PrepareSignature();
	void			PrepareSignatureSingle(bool bRotate);
	void			PrepareSignatureThumb();
	void			PrepareSignatureThumbSingle(bool bRotate);
	bool			CompareSignature(bool bQuiet);
	void			SendSubmit(CString strExifMake, CString strExifModel, CString strQual, 
							CString strDqt0, CString strDqt1, CString strDqt2, CString strDqt3,
							CString strCss,
							CString strSig, CString strSigRot, CString strSigThumb, 
							CString strSigThumbRot, float fQFact0, float fQFact1, unsigned nImgW, unsigned nImgH, 
							CString strExifSoftware, CString strComment, teMaker eMaker,
							teSource eUserSource, CString strUserSoftware, CString strExtra,
							CString strUserNotes, unsigned nExifLandscape,
							unsigned nThumbX,unsigned nThumbY);
	void			OutputSpecial();


	// Display routines
	void			DbgAddLine(LPCTSTR strLine);
	void			AddHeader(unsigned nCode);
	CString			PrintAsHexUC(unsigned char* anBytes,unsigned nCount);
	CString			PrintAsHex8(unsigned* anBytes,unsigned nCount);
	CString			PrintAsHex32(unsigned* anWords,unsigned nCount);

	// Buffer access
	BYTE			Buf(unsigned long nOffset,bool bClean);
	void			UnByteSwap4(unsigned nVal,unsigned &nByte0,unsigned &nByte1,unsigned &nByte2,unsigned &nByte3);
	unsigned		ByteSwap4(unsigned nByte0,unsigned nByte1, unsigned nByte2, unsigned nByte3);
	unsigned		ByteSwap2(unsigned nByte0,unsigned nByte1);
	unsigned		ReadSwap2(unsigned nPos);
	unsigned		ReadSwap4(unsigned nPos);
	unsigned		ReadBe4(unsigned nPos);

	// UI elements
public:
	void			SetStatusBar(CStatusBar* pStatBar);
private:
	void			SetStatusText(CString strText);
	void			DecodeErrCheck(bool bRet);


	// --------------------------------------
	// Class variables
	// --------------------------------------

private:
	// Configuration
	CSnoopConfig*	m_pAppConfig;
	bool			m_bVerbose;
	bool			m_bOutputDB;
	bool			m_bBufFakeDHT;		// Flag to redirect DHT read to AVI DHT over Buffer content

	// General classes required for decoding
	CwindowBuf*		m_pWBuf;
	CimgDecode*		m_pImgDec;
	CDecodePs*		m_pPsDec;
	CDecodeDicom*	m_pDecDicom;

	// UI elements & log
	CDocLog*		m_pLog;
	CStatusBar*		m_pStatBar;		// Link to status bar

	// Constants
	static const BYTE				m_abMJPGDHTSeg[JFIF_DHT_FAKE_SZ];	// Motion JPEG DHT
	static const MarkerNameTable	m_pMarkerNames[];



	// Status
	bool			m_bImgOK;					// Img decode encounter SOF
	bool			m_bAvi;						// Is it an AVI file?
	bool			m_bAviMjpeg;				// Is it a MotionJPEG AVI file?
	bool			m_bPsd;						// Is it a Photoshop file?

	bool			m_pImgSrcDirty;				// Do we need to recalculate the scan decode?


	// File position records
	unsigned long	m_nPos;				// Current file/buffer position
	unsigned long	m_nPosEoi;			// Position of EOI (0xFFD9) marker
	unsigned		m_nPosSos;
	unsigned long	m_nPosEmbedStart;	// Embedded/offset start
	unsigned long	m_nPosEmbedEnd;		// Embedded/offset end
	unsigned long	m_nPosFileEnd;		// End of file position


	// Decoder state
	TCHAR			m_acApp0Identifier[MAX_IDENTIFIER];	// APP0 type: JFIF, AVI1, etc.

	float			m_afStdQuantLumCompare[64];
	float			m_afStdQuantChrCompare[64];

	unsigned		m_anMaskLookup[32];

	unsigned		m_nImgVersionMajor;
	unsigned		m_nImgVersionMinor;
	unsigned		m_nImgUnits;
	unsigned		m_nImgDensityX;
	unsigned		m_nImgDensityY;
	unsigned		m_nImgThumbSizeX;
	unsigned		m_nImgThumbSizeY;

	bool			m_bImgProgressive;		// Progressive scan?
	bool			m_bImgSofUnsupported;	// SOF mode unsupported - skip SOI content

	CString			m_strComment;			// Comment string

	unsigned		m_nSosNumCompScan_Ns;	
	unsigned		m_nSosSpectralStart_Ss;
	unsigned		m_nSosSpectralEnd_Se;
	unsigned		m_nSosSuccApprox_A;

	bool			m_nImgRstEn;		// DRI seen
	unsigned		m_nImgRstInterval;

	unsigned		m_anImgDqtTbl[MAX_DQT_DEST_ID][MAX_DQT_COEFF];
	double			m_adImgDqtQual[MAX_DQT_DEST_ID];
	bool			m_abImgDqtSet[MAX_DQT_DEST_ID];		// Has this table been configured?
	unsigned		m_anDhtNumCodesLen_Li[17];

	unsigned		m_nSofPrecision_P;
	unsigned		m_nSofNumLines_Y;
	unsigned		m_nSofSampsPerLine_X;
	unsigned		m_nSofNumComps_Nf;					// Number of components in frame (might not equal m_nSosNumCompScan_Ns)

	// Define Quantization table details for the indexed Component Identifier
	// - Component identifier (SOF:Ci) has a range of 0..255
	unsigned		m_anSofQuantCompId[MAX_SOF_COMP_NF];		// SOF:Ci, index is i-1
	unsigned		m_anSofQuantTblSel_Tqi[MAX_SOF_COMP_NF];
	unsigned		m_anSofHorzSampFact_Hi[MAX_SOF_COMP_NF];
	unsigned		m_anSofVertSampFact_Vi[MAX_SOF_COMP_NF];
	unsigned		m_nSofHorzSampFactMax_Hmax;
	unsigned		m_nSofVertSampFactMax_Vmax;

	// FIXME: Move to CDecodePs
	unsigned		m_nImgQualPhotoshopSa;
	unsigned		m_nImgQualPhotoshopSfw;

	int				m_nApp14ColTransform;	// Color transform from JFIF APP14 Adobe marker (-1 if not set)

	teLandscape		m_eImgLandscape;		// Landscape vs Portrait
	CString			m_strImgQuantCss;		// Chroma subsampling (e.g. "2x1")
	
	unsigned		m_nImgExifEndian;		// 0=Intel 1=Motorola
	unsigned		m_nImgExifSubIfdPtr;
	unsigned		m_nImgExifGpsIfdPtr;
	unsigned		m_nImgExifInteropIfdPtr;
	unsigned		m_nImgExifMakerPtr;

	bool			m_bImgExifMakeSupported;	// Mark makes that we support decode for
	unsigned		m_nImgExifMakeSubtype;		// Used for Nikon (e.g. type3)

	CString			m_strImgExtras;				// Extra strings used for DB submission

	// Embedded EXIF Thumbnail
	unsigned		m_nImgExifThumbComp;
	unsigned		m_nImgExifThumbOffset;
	unsigned		m_nImgExifThumbLen;
	unsigned		m_anImgThumbDqt[4][64];
	bool			m_abImgDqtThumbSet[4];
	CString			m_strHashThumb;
	CString			m_strHashThumbRot;
	unsigned		m_nImgThumbNumLines;
	unsigned		m_nImgThumbSampsPerLine;

	// Signature handling
	bool			m_bSigExactInDB;		// Is current entry already in local DB?

	// State of decoder -- have we seen each marker?
	bool			m_bStateAbort;			// Do we abort decoding? (eg. user hits cancel after errs)

	bool			m_bStateSoi;
	bool			m_bStateDht;
	bool			m_bStateDhtOk;
	bool			m_bStateDhtFake;		// Fake DHT required for AVI
	bool			m_bStateDqt;
	bool			m_bStateDqtOk;
	bool			m_bStateSof;
	bool			m_bStateSofOk;
	bool			m_bStateSos;
	bool			m_bStateSosOk;
	bool			m_bStateEoi;


	teDbAdd			m_eDbReqSuggest;
	CString			m_strHash;
	CString			m_strHashRot;
	CString			m_strImgExifMake;
	CString			m_strImgExifModel;
	CString			m_strImgQualExif;		// Quality (e.g. "fine") from makernotes
	CString			m_strSoftware;			// EXIF Software field
	bool			m_bImgExifMakernotes;	// Are any Makernotes present?

	teEdited		m_eImgEdited;			// Image edited? 0 = unset, 1 = yes, etc.


};

