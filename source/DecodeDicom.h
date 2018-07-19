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
// - This module parses DICOM files
// - No rendering is done
//
// ==========================================================================

#pragma once

#include "WindowBuf.h"
#include "snoop.h"
#include "SnoopConfig.h"


// Structure used for each Tag entry
// Reference: Types defined in Part 3, Section 5.2
enum teDicomType { DICOM_T_UNK,
	DICOM_T_TYPE1,
	DICOM_T_TYPE1C,
	DICOM_T_TYPE3,
	DICOM_T_END };
// Definition of Group & Element in Part 10, Section 5
struct tsDicomTag {
	unsigned	nTagGroup;
	unsigned	nTagElement;
	teDicomType	eTagType;
	CString		strTagName;
};

// Definition of Transfer Syntaxes and other UIDs
struct tsDicomTagSpec {
	unsigned	nTagGroup;
	unsigned	nTagElement;
	CString		strVal;
	CString		strDefinition;
};

struct tsTagDetail {
	unsigned		nTagGroup;
	unsigned		nTagElement;
	CString			strVR;
	unsigned		nLen;
	unsigned		nOffset;		// Offset after parsing this tag

	bool			bVrExplicit;
	bool			bLen4B;

	bool			bTagOk;
	CString			strTag;
	bool			bValOk;
	CString			strVal;

	bool			bTagIsJpeg;
	unsigned long	nPosJpeg;

	tsTagDetail() {
		Reset();		
	};
	void Reset() {
		nTagGroup		= 0;
		nTagElement		= 0;
		strVR			= _T("");
		nLen			= 0;
		nOffset			= 0;

		bVrExplicit		= false;
		bLen4B			= false;

		bTagOk			= false;
		strTag			= _T("");
		bValOk			= false;
		strVal			= _T("");

		bTagIsJpeg		= false;
		nPosJpeg		= 0;
	};
};


// Byte Swap enable for DICOM decoding
#define DC_BSWAP	true

// Define the DICOM hex output display characteristics
#define DC_HEX_MAX_INLINE	16		// Threshold for displaying hex in-line with field name
#define DC_HEX_MAX_ROW		16		// Maximum number of bytes to report per line
#define DC_HEX_TOTAL		128		// Total number of bytes to report before clipping

// Define the maximum length Unicode string to display
#define DC_MAX_UNICODE_STRLEN 256


class CDecodeDicom
{
public:
	CDecodeDicom(CwindowBuf* pWBuf,CDocLog* pLog);
	~CDecodeDicom(void);

	void			Reset();

	bool			DecodeDicom(unsigned long nPos,unsigned long nPosFileEnd,unsigned long &nPosJpeg);
	//bool			DecodeTagHeader(unsigned long nPos,CString &strTag,CString &strVR,unsigned &nLen,unsigned &nOffset,unsigned long &nPosJpeg);
	bool			GetTagHeader(unsigned long nPos,tsTagDetail &sTagDetail);
	bool			FindTag(unsigned nTagGroup,unsigned nTagElement,unsigned &nFldInd);

	BYTE			Buf(unsigned long offset,bool bClean);

	CString			ParseIndent(unsigned nIndent);
	void			ReportFldStr(unsigned nIndent,CString strField,CString strVal);
	void			ReportFldStrEnc(unsigned nIndent,CString strField,CString strVal,CString strEncVal);
	void			ReportFldHex(unsigned nIndent,CString strField,unsigned long nPosStart,unsigned nLen);
	void			ReportFldEnum(unsigned nIndent,CString strField,unsigned nTagGroup,unsigned nTagElement,CString strVal);
	bool			LookupEnum(unsigned nTagGroup,unsigned nTagElement,CString strVal,CString &strDesc);

private:
	// Configuration
	CSnoopConfig*	m_pAppConfig;

	// General classes required for decoding
	CwindowBuf*		m_pWBuf;
	CDocLog*		m_pLog;

public:
	bool			m_bDicom;
	bool			m_bJpegEncap;
	bool			m_bJpegEncapOffsetNext;

};

