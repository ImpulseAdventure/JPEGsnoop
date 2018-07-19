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
// - This module parses the Photoshop PSD and 8BIM segments (APP13)
// - No rendering is done
//
// ==========================================================================

#pragma once

#include "WindowBuf.h"
#include "snoop.h"
#include "SnoopConfig.h"

#include "Dib.h"

//
// Structure used for each IPTC field
// - NUM     : binary number
// - STR     : alphabetic characters, graphic characters, numeric character
// - HEX     :
enum teIptcType { IPTC_T_NUM, IPTC_T_NUM1, IPTC_T_NUM2, IPTC_T_HEX, IPTC_T_STR, IPTC_T_UNK, IPTC_T_END };
struct tsIptcField {
	unsigned	nRecord;
	unsigned	nDataSet;
	teIptcType	eFldType;
	CString		strFldName;
};

// Structure used for each Image Resource Block (8BIM) record
enum teBimType { BIM_T_UNK, BIM_T_STR, BIM_T_HEX, 
	BIM_T_IPTC_NAA,
	BIM_T_JPEG_QUAL,
	BIM_T_PS_SLICES,
	BIM_T_PS_THUMB_RES,
	BIM_T_PS_DESCRIPTOR,
	BIM_T_PS_VER_INFO,
	BIM_T_PS_RESOLUTION_INFO,
	BIM_T_PS_PRINT_SCALE,
	BIM_T_PS_PIXEL_ASPECT_RATIO,
	BIM_T_PS_DOC_SPECIFIC_SEED,
	BIM_T_PS_GRID_GUIDES,
	BIM_T_PS_GLOBAL_ANGLE,
	BIM_T_PS_GLOBAL_ALTITUDE,
	BIM_T_PS_PRINT_FLAGS,
	BIM_T_PS_PRINT_FLAGS_INFO,
	BIM_T_PS_COPYRIGHT_FLAG,
	BIM_T_PS_LAYER_STATE_INFO,
	BIM_T_PS_LAYER_GROUP_INFO,
	BIM_T_PS_LAYER_GROUP_ENABLED,
	BIM_T_PS_LAYER_SELECT_ID,
	BIM_T_PS_STR_UNI,
	BIM_T_PS_STR_ASC,
	BIM_T_PS_STR_ASC_LONG,
	BIM_T_END
};
struct tsBimRecord {
	unsigned	nCode;			// Code value or start code for range
	unsigned	nCodeEnd;		// 0x0000 if not a range, else specifies last code value
	teBimType	eBimType;
	CString		strRecordName;
};


// Structure used for each enumerated field in IRB decoding
enum teBimEnumField {
	BIM_T_ENUM_UNK,
	BIM_T_ENUM_FILE_HDR_COL_MODE,
	BIM_T_ENUM_RESOLUTION_INFO_RES_UNIT,
	BIM_T_ENUM_RESOLUTION_INFO_WIDTH_UNIT,
	BIM_T_ENUM_PRINT_SCALE_STYLE,
	BIM_T_ENUM_GRID_GUIDE_DIR,
	BIM_T_ENUM_BLEND_MODE_KEY,
	BIM_T_ENUM_END
};
struct tsBimEnum {
	teBimEnumField	eBimEnum;
	unsigned		nVal;
	CString			strVal;
};

// Byte Swap enable for Photoshop decoding
#define PS_BSWAP	false

// Define the 8BIM/IRB hex output display characteristics
#define PS_HEX_MAX_INLINE	16		// Threshold for displaying hex in-line with field name
#define PS_HEX_MAX_ROW		16		// Maximum number of bytes to report per line
#define PS_HEX_TOTAL		128		// Total number of bytes to report before clipping

// Define the maximum length Unicode string to display
#define PS_MAX_UNICODE_STRLEN 256

// Information about layer and channels within it
struct tsLayerInfo {
	unsigned	nNumChans;
	unsigned*	pnChanLen;
	unsigned*	pnChanID;
	unsigned	nWidth;
	unsigned	nHeight;
};

// From Layer Info
struct tsLayerAllInfo {
	unsigned		nNumLayers;
	tsLayerInfo*	psLayers;
};

// From File Header
struct tsImageInfo {
	unsigned		nImageWidth;
	unsigned		nImageHeight;
	unsigned		nNumChans;
	unsigned		nDepthBpp;
};

class CDecodePs
{
public:
	CDecodePs(CwindowBuf* pWBuf,CDocLog* pLog);
	~CDecodePs(void);

	void			Reset();

	bool			DecodePsd(unsigned long nPos,CDIB* pDibTemp,unsigned &nWidth,unsigned &nHeight);
	bool			PhotoshopParseImageResourceBlock(unsigned long &nPos,unsigned nIndent);
	
private:
	CString			PhotoshopParseGetLStrAsc(unsigned long &nPos);
	CString			PhotoshopParseIndent(unsigned nIndent);
	void			PhotoshopParseReportNote(unsigned nIndent,CString strNote);
	CString			PhotoshopParseLookupEnum(teBimEnumField eEnumField,unsigned nVal);
	void			PhotoshopParseReportFldNum(unsigned nIndent,CString strField,unsigned nVal,CString strUnits);
	void			PhotoshopParseReportFldBool(unsigned nIndent,CString strField,unsigned nVal);
	void			PhotoshopParseReportFldEnum(unsigned nIndent,CString strField,teBimEnumField eEnumField,unsigned nVal);
	void			PhotoshopParseReportFldFixPt(unsigned nIndent,CString strField,unsigned nVal,CString strUnits);
	void			PhotoshopParseReportFldFloatPt(unsigned nIndent,CString strField,unsigned nVal,CString strUnits);
	void			PhotoshopParseReportFldDoublePt(unsigned nIndent,CString strField,unsigned nVal1,unsigned nVal2,CString strUnits);
	void			PhotoshopParseReportFldStr(unsigned nIndent,CString strField,CString strVal);
	void			PhotoshopParseReportFldOffset(unsigned nIndent,CString strField,unsigned long nOffset);
	void			PhotoshopParseReportFldHex(unsigned nIndent,CString strField,unsigned long nPosStart,unsigned nLen);
	void			PhotoshopParseThumbnailResource(unsigned long &nPos,unsigned nIndent);
	void			PhotoshopParseSliceHeader(unsigned long &nPos,unsigned nIndent,unsigned long nPosEnd);
	void			PhotoshopParseSliceResource(unsigned long &nPos,unsigned nIndent,unsigned long nPosEnd);
	void			PhotoshopParseDescriptor(unsigned long &nPos,unsigned nIndent);
	void			PhotoshopParseList(unsigned long &nPos,unsigned nIndent);
	void			PhotoshopParseInteger(unsigned long &nPos,unsigned nIndent);
	void			PhotoshopParseBool(unsigned long &nPos,unsigned nIndent);
	void			PhotoshopParseEnum(unsigned long &nPos,unsigned nIndent);
	void			PhotoshopParseStringUni(unsigned long &nPos,unsigned nIndent);
	void			PhotoshopParseHandleOsType(CString strOsType,unsigned long &nPos,unsigned nIndent);
	void			PhotoshopParseFileHeader(unsigned long &nPos,unsigned nIndent,tsImageInfo* psImageInfo);
	void			PhotoshopParseColorModeSection(unsigned long &nPos,unsigned nIndent);
	bool			PhotoshopParseImageResourcesSection(unsigned long &nPos,unsigned nIndent);
	void			PhotoshopParseVersionInfo(unsigned long &nPos,unsigned nIndent);
	void			PhotoshopParseResolutionInfo(unsigned long &nPos,unsigned nIndent);
	void			PhotoshopParsePrintScale(unsigned long &nPos,unsigned nIndent);
	void			PhotoshopParsePixelAspectRatio(unsigned long &nPos,unsigned nIndent);
	void			PhotoshopParseDocSpecificSeed(unsigned long &nPos,unsigned nIndent);
	void			PhotoshopParseGridGuides(unsigned long &nPos,unsigned nIndent);
	void			PhotoshopParseGlobalAngle(unsigned long &nPos,unsigned nIndent);
	void			PhotoshopParseGlobalAltitude(unsigned long &nPos,unsigned nIndent);
	void			PhotoshopParsePrintFlags(unsigned long &nPos,unsigned nIndent);
	void			PhotoshopParsePrintFlagsInfo(unsigned long &nPos,unsigned nIndent);
	void			PhotoshopParseCopyrightFlag(unsigned long &nPos,unsigned nIndent);
	void			PhotoshopParseLayerStateInfo(unsigned long &nPos,unsigned nIndent);
	void			PhotoshopParseLayerGroupInfo(unsigned long &nPos,unsigned nIndent,unsigned nLen);
	void			PhotoshopParseLayerGroupEnabled(unsigned long &nPos,unsigned nIndent,unsigned nLen);
	void			PhotoshopParseLayerSelectId(unsigned long &nPos,unsigned nIndent);
	void			PhotoshopParseJpegQuality(unsigned long &nPos,unsigned nIndent,unsigned long nPosEnd);

	bool			PhotoshopParseLayerMaskInfo(unsigned long &nPos,unsigned nIndent,CDIB* pDibTemp);
	bool			PhotoshopParseLayerInfo(unsigned long &nPos,unsigned nIndent,CDIB* pDibTemp);
	bool			PhotoshopParseLayerRecord(unsigned long &nPos,unsigned nIndent,tsLayerInfo* psLayerInfo);
	bool			PhotoshopParseLayerMask(unsigned long &nPos,unsigned nIndent);
	bool			PhotoshopParseLayerBlendingRanges(unsigned long &nPos,unsigned nIndent);
	bool			PhotoshopParseChannelImageData(unsigned long &nPos,unsigned nIndent,unsigned nWidth,unsigned nHeight,unsigned nChan,unsigned char* pDibBits);
	bool			PhotoshopParseGlobalLayerMaskInfo(unsigned long &nPos,unsigned nIndent);
	bool			PhotoshopParseAddtlLayerInfo(unsigned long &nPos,unsigned nIndent);
	bool			PhotoshopParseImageData(unsigned long &nPos,unsigned nIndent,tsImageInfo* psImageInfo,unsigned char* pDibBits);

	bool			PhotoshopDecodeRowUncomp(unsigned long &nPos,unsigned nWidth,unsigned nHeight,unsigned nRow,unsigned nChanID,unsigned char* pDibBits);
	bool			PhotoshopDecodeRowRle(unsigned long &nPos,unsigned nWidth,unsigned nHeight,unsigned nRow,unsigned nRowLen,unsigned nChanID,unsigned char* pDibBits);

	CString			PhotoshopDispHexWord(unsigned nVal);

	// 8BIM
	CString			PhotoshopParseGetBimLStrUni(unsigned long nPos,unsigned &nPosOffset);
	bool			FindBimRecord(unsigned nBimId,unsigned &nFldInd);

	// IPTC
	void			DecodeIptc(unsigned long &nPos,unsigned nLen,unsigned nIndent);
	bool			LookupIptcField(unsigned nRecord,unsigned nDataSet,unsigned &nFldInd);
	CString			DecodeIptcValue(teIptcType eIptcType,unsigned nFldCnt,unsigned long nPos);

	BYTE			Buf(unsigned long offset,bool bClean);

private:
	// Configuration
	CSnoopConfig*	m_pAppConfig;

	// General classes required for decoding
	CwindowBuf*		m_pWBuf;
	CDocLog*		m_pLog;

	bool			m_bAbort;		// Abort continued decode?

public:
	bool			m_bPsd;
	unsigned		m_nQualitySaveAs;
	unsigned		m_nQualitySaveForWeb;

	bool			m_bDisplayLayer;
	unsigned		m_nDisplayLayerInd;
	bool			m_bDisplayImage;
};

