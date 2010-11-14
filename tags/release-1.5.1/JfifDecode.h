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

#define MAX_COMPS 150

#include "DocLog.h"
#include "ImgDecode.h"
#include "WindowBuf.h"
#include "snoop.h"
#include "SnoopConfig.h"

#include "DbSigs.h"

#define JFIF_SOI  0xD8
#define JFIF_APP0 0xE0
#define JFIF_APP1 0xE1
#define JFIF_APP2 0xE2
#define JFIF_APP3 0xE3
#define JFIF_APP4 0xE4
#define JFIF_APP5 0xE5
#define JFIF_APP6 0xE6
#define JFIF_APP7 0xE7
#define JFIF_APP8 0xE8
#define JFIF_APP9 0xE9
#define JFIF_APP10 0xEA
#define JFIF_APP11 0xEB
#define JFIF_APP12 0xEC
#define JFIF_APP13 0xED
#define JFIF_APP14 0xEE
#define JFIF_APP15 0xEF
#define JFIF_DQT  0xDB
#define JFIF_SOF0 0xC0
#define JFIF_SOF1 0xC1
#define JFIF_SOF2 0xC2
#define JFIF_SOF3 0xC3
#define JFIF_SOF5 0xC5
#define JFIF_SOF6 0xC6
#define JFIF_SOF7 0xC7
#define JFIF_JPG  0xC8
#define JFIF_SOF9 0xC9
#define JFIF_SOF10 0xCA
#define JFIF_SOF11 0xCB
#define JFIF_SOF12 0xCC
#define JFIF_SOF13 0xCD
#define JFIF_SOF14 0xCE
#define JFIF_SOF15 0xCF
#define JFIF_COM  0xFE
#define JFIF_DHT  0xC4
#define JFIF_SOS  0xDA
#define JFIF_DRI  0xDD
#define JFIF_RST0 0xD0
#define JFIF_RST1 0xD1
#define JFIF_RST2 0xD2
#define JFIF_RST3 0xD3
#define JFIF_RST4 0xD4
#define JFIF_RST5 0xD5
#define JFIF_RST6 0xD6
#define JFIF_RST7 0xD7
#define JFIF_EOI  0xD9

#define JFIF_DHT_FAKE		0x999999C4
#define JFIF_DHT_FAKE_SZ	0x1A4


struct CStr2 {
	CString	tag;
	CString	val;
	bool	bUnknown;	// Tag is not known
};


struct MarkerNameTable {
	unsigned	code;
	char*		str;
};

class CjfifDecode
{
public:
	CjfifDecode(CDocLog* pLog,CwindowBuf* pWBuf,CimgDecode* pImgDec);
	~CjfifDecode();

	void		SetStatusBar(CStatusBar* pStatBar);
	void		Reset();
	void		ImgSrcChanged();

	void		process(CFile* inFile);
	void		PrepareSignature();
	void		PrepareSignatureSingle(bool bRotate);
	void		PrepareSignatureThumb();
	void		PrepareSignatureThumbSingle(bool bRotate);
	bool		CompareSignature(bool bQuiet);
	void		PrepareSendSubmit(CString strQual,unsigned nUserSource,CString strUserSoftware,CString strUserNotes);
	void		SetDQTQuick(unsigned dqt0[64],unsigned dqt1[64]);

//	void		ExportRangeSet(unsigned nStart, unsigned nEnd);
	bool		ExportJpegPrepare(CString strFileIn,bool bForceSoi,bool bForceEoi,bool bIgnoreEoi);
	bool		ExportJpegDo(CString strFileIn, CString strFileOut, unsigned long nFileLen,
					bool bOverlayEn, bool bDhtAviInsert,bool bForceSoi,bool bForceEoi);
	bool		ExportJpegDoRange(CString strFileIn, CString strFileOut, 
					unsigned long nStart, unsigned long nEnd);

private:
	void		HuffMaskLookupGen();
	void		DecodeDHT(bool bInject);
	unsigned	DecodeApp13Ps();
	unsigned	DecodeApp2Flashpix();
	unsigned	DecodeApp2IccProfile(unsigned nLen);
	unsigned	DecodeIccHeader(unsigned nPos);
	bool		DecodeAvi();

	void		DecodeErrCheck(bool bRet);
	unsigned	DecodeMarker();
	unsigned	DecodeExifIfd(CString ifd_str,unsigned pos_exif_start,unsigned start_ifd_ptr);
//	unsigned	DecodeMakerIfd(unsigned ifd_tag,unsigned ptr,unsigned len);
	bool		DecodeMakerSubType();
	CString		LookupExifTag(CString sect, unsigned tag, bool &bUnknown);
	CStr2		LookupMakerCanonTag(unsigned mainTag,unsigned subTag,unsigned val);

	void		DecodeEmbeddedThumb();

	bool		GetMarkerName(unsigned code,CString &markerStr);
	void		ClearDQT();

	void		OutputSpecial();
	void		SendSubmit(CString make, CString model, CString qual, 
							CString dqt0, CString dqt1, CString dqt2, CString dqt3,
							CString css,
								CString sig, CString sig_rot, CString sig_thumb, 
								CString sig_thumb_rot, float qfact0, float qfact1, unsigned img_w, unsigned img_h, 
								CString software, CString comment, unsigned maker,
								unsigned user_source, CString user_software, CString extra,
								CString user_notes, unsigned exif_landscape,
								unsigned thumb_x,unsigned thumb_y);

	void		AddHeader(unsigned code);
	void		UnByteSwap4(unsigned nVal,unsigned &nByte0,unsigned &nByte1,unsigned &nByte2,unsigned &nByte3);
	unsigned	ByteSwap4(unsigned b0,unsigned b1, unsigned b2, unsigned b3);
	unsigned	ByteSwap2(unsigned b0,unsigned b1);
	unsigned	ReadSwap2(unsigned pos);
	unsigned	ReadSwap4(unsigned pos);
	unsigned	ReadBe4(unsigned pos);
	CString		Uint2Chars(unsigned nVal);
	CString		Uint2DotByte(unsigned nVal);
	bool		TestBit(unsigned nVal,unsigned nBit);
	CString		DecodeIccDateTime(unsigned nVal[3]);


	void		DbgAddLine(const char* strLine);

	BYTE		Buf(unsigned long offset,bool bClean);

	void		SetStatusText(CString str);

	bool		DecodeValRational(unsigned pos,float &val);
	CString		DecodeValFraction(unsigned pos);
	bool		DecodeValGPS(unsigned pos,CString &strCoord);
	bool		PrintValGPS(unsigned nCount, float fCoord1, float fCoord2, float fCoord3,CString &strCoord);
	CString		PrintAsHexUC(unsigned char* naBytes,unsigned nCount);
	CString		PrintAsHex(unsigned* naBytes,unsigned nCount);

public:

	// The following is made public b/c its reused in EXE DQT search
	static const unsigned	m_sZigZag[];
	static const unsigned	m_sUnZigZag[];
	static const unsigned   m_sStdQuantLum[];
	static const unsigned	m_sStdQuantChr[];

private:
	CDocLog*		m_pLog;
	CimgDecode*		m_pImgDec;
	CwindowBuf*		m_pWBuf;
	CSnoopConfig*	m_pAppConfig;
	CStatusBar*		m_pStatBar;		// Link to status bar

public: // FIXME public for BATCH_DO_DBSUBMIT
	CString			m_strFileName;
private:



	bool		bVerbose;
	bool		bOutputWeb;
	bool		bOutputExcel;
	bool		bOutputDB;

	unsigned long	m_nPos;				// Current file/buffer position
	unsigned long	m_nPosEoi;			// Position of EOI (0xFFD9) marker
public: // for ExportJpeg
	unsigned long	m_nPosEmbedStart;	// Embedded/offset start
	unsigned long	m_nPosEmbedEnd;		// Embedded/offset end
public:
	unsigned long	m_nPosFileEnd;		// End of file position
private:
	bool			m_bBufFakeDHT;		// Temporary Buf redirect mode to read in AVI DHT

	static const unsigned	m_sQuantRotate[];



	bool		m_bImgOK;					// Img decode encounter SOF
	bool		m_bAvi;						// Is it an AVI file?
public: // for ExportJpeg
	bool		m_bAviMjpeg;				// Is it a MotionJPEG AVI file?
private:

	bool		m_pImgSrcDirty;				// Do we need to recalculate the scan decode?

	char		m_strApp0Identifier[32];	// APP0 type: JFIF, AVI1, etc.

	float		m_afStdQuantLumCompare[64];
	float		m_afStdQuantChrCompare[64];

	unsigned	m_anMaskLookup[32];

	unsigned	m_nImgVersionMajor,m_nImgVersionMinor;
	unsigned	m_nImgUnits;
	unsigned	m_nImgDensityX,m_nImgDensityY;
	unsigned	m_nImgThumbSizeX,m_nImgThumbSizeY;

	bool		m_bImgProgressive;		// Progressive scan?

	CString		m_strComment;			// Comment string

	unsigned	m_nImgSosCompNum;
	unsigned	m_nImgSpectralStart,m_nImgSpectralEnd;
	unsigned	m_nImgSuccApprox;

	bool		m_nImgRstEn;		// DRI seen
	unsigned	m_nImgRstInterval;

	unsigned	m_anImgDqt[4][64];
	double		m_adImgDqtQual[4];
	bool		m_bImgDqtSet[4];		// Has this table been configured?
	unsigned	m_anImgDhtCodesLen[17];

	unsigned	m_nImgPrecision;
	unsigned	m_nImgNumLines,m_nImgSampsPerLine;
	unsigned	m_nImgSofCompNum;	// From SOF, probably same as m_nImgSosCompNum from SOS. No, differs for progscan

	unsigned	m_anImgQuantCompId[4];
	unsigned	m_anImgQuantTblSel[4];
	unsigned	m_nImgSampFactorXMax,m_nImgSampFactorYMax;
	unsigned	m_anImgSampFactorX[4];
	unsigned	m_anImgSampFactorY[4];

	unsigned	m_nImgQualPhotoshopSa;
	unsigned	m_nImgQualPhotoshopSfw;


	unsigned	m_nImgLandscape;		// TRUE=Landscape, FALSE=portrait
	CString		m_strImgQuantCss;  // Chroma subsampling (e.g. "2x1")
	
	unsigned	m_nImgExifEndian;	// 0=Intel 1=Motorola
	unsigned	m_nImgExifSubIfdPtr;
	unsigned	m_nImgExifGpsIfdPtr;
	unsigned	m_nImgExifInteropIfdPtr;
	unsigned	m_nImgExifMakerPtr;

	bool		m_bImgExifMakeSupported; // Mark makes that we support decode for
	unsigned	m_nImgExifMakeSubtype; // Used for Nikon (e.g. type3)

	CString		m_strImgExtras;			// Extra strings used for DB submission

	static const BYTE m_abMJPGDHTSeg[JFIF_DHT_FAKE_SZ];

private:

	static const MarkerNameTable m_pMarkerNames[];

	// Embedded EXIF Thumbnail
	unsigned	m_nImgExifThumbComp;
	unsigned	m_nImgExifThumbOffset;
	unsigned	m_nImgExifThumbLen;
	unsigned	m_anImgThumbDqt[4][64];
	bool		m_bImgDqtThumbSet[4];
	CString		m_strHashThumb;
	CString		m_strHashThumbRot;
	unsigned	m_nImgThumbNumLines;
	unsigned	m_nImgThumbSampsPerLine;

	bool		m_bSigExactInDB;		// Is current entry already in local DB?

	// State of decoder -- have we seen each marker?
	bool		m_bStateAbort;			// Do we abort decoding? (eg. user hits cancel after errs)

	bool		m_bStateSoi;
	bool		m_bStateDht;
	bool		m_bStateDhtOk;
	bool		m_bStateDhtFake;		// Fake DHT required for AVI
	bool		m_bStateDqt;
	bool		m_bStateDqtOk;
	bool		m_bStateSof;
	bool		m_bStateSofOk;
	bool		m_bStateSos;
	bool		m_bStateSosOk;
	bool		m_bStateEoi;

public:
	unsigned	m_nPosSos;

public:
	// FIXME
	// Following are public for JPEGsnoopDoc, which uses them
	// for DbSubmitDlg, etc.
	unsigned	m_nDbReqSuggest;
	CString		m_strHash;
	CString		m_strHashRot;
	CString		m_strImgExifMake;
	CString		m_strImgExifModel;
	CString		m_strImgQualExif;		// Quality (e.g. "fine") from makernotes
	CString		m_strSoftware;			// EXIF Software field
	bool		m_bImgExifMakernotes;	// Are any Makernotes present?

	unsigned	m_nImgEdited;			// Image edited? 0 = unset, 1 = yes, etc.

};

