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



//#include "stdafx.h"

#include <QtGlobal>

#include "DecodeDicom.h"
#include "snoop.h"
#include "JPEGsnoop.h" // for m_pAppConfig get

#include "WindowBuf.h"


// Do we want to display extended DICOM tag info?
//#ifdef DICOM_TAG_EXTENDED


// Forward declarations
extern struct tsDicomTag	asDicomTags[];
extern struct tsDicomTagSpec	asTagSpecial[];

CDecodeDicom::CDecodeDicom(CwindowBuf* pWBuf,CDocLog* pLog)
{
	// Save copies of class pointers
	m_pWBuf = pWBuf;
	m_pLog = pLog;

	// Ideally this would be passed by constructor, but simply access
	// directly for now.
	CJPEGsnoopApp*	pApp;
//	pApp = (CJPEGsnoopApp*)AfxGetApp();
    m_pAppConfig = pApp->m_pAppConfig;

	Reset();
}

void CDecodeDicom::Reset()
{
	m_bDicom = false;
	m_bJpegEncap = false;
	m_bJpegEncapOffsetNext = false;
}


CDecodeDicom::~CDecodeDicom(void)
{
}


// Provide a short-hand alias for the m_pWBuf buffer
// INPUT:
// - offset					File offset to read from
// - bClean					Forcibly disables any redirection to Fake DHT table
//
// POST:
// - m_pLog
//
// RETURN:
// - Byte from file
//
quint8 CDecodeDicom::Buf(unsigned long offset,bool bClean=false)
{
	return m_pWBuf->Buf(offset,bClean);
}




bool CDecodeDicom::GetTagHeader(unsigned long nPos,tsTagDetail &sTagDetail)
{
	unsigned	nVR = 0;
	QString		strError;
    quint32	nTagGroup;
    quint32	nTagElement;
	QString		strTagName = "";

	unsigned	nLen = 0;
	unsigned	nOffset = 0;
	QString		strVR = "";

	bool		bTagOk = false;
	QString		strTag = "";

	bool		bVrExplicit;
	bool		bLen4B;

	bool			bTagIsJpeg;
	unsigned long	nPosJpeg;
	bool			bTagIsOffsetHdr;

	// Find the tag
	nTagGroup = m_pWBuf->BufX(nPos+0,2,true);
	nTagElement = m_pWBuf->BufX(nPos+2,2,true);

	bool		bFoundTag;
	unsigned	nFldInd = 0;
	bFoundTag = FindTag(nTagGroup,nTagElement,nFldInd);

	if (!bFoundTag) {
		// Check for group length entry
		// FIXME:
		// Reference: Part 5, Section 7.2
		if (nTagElement == 0x0000) {
            strTagName.arg(nTagGroup, 4, 16, QChar('0')); // Format("Group Length (Group=%04X)"),nTagGroup;
			bTagOk = true;
		} else {
			/*
			strError.Format("ERROR: Unknown Tag ID (%04X,%04X) @ 0x%08X"),nTagGroup,nTagElement,nPos;
			m_pLog->AddLineErr(strError);
			*/
            strTagName.arg(nTagGroup, 4, 16, QChar('0')).arg(nTagElement, 4, 16, QChar('0')); // .Format("??? (%04X,%04X)"),nTagGroup,nTagElement;
			bTagOk = false;
		}
	} else {
		strTagName = asDicomTags[nFldInd].strTagName;
		bTagOk = true;
	}


	// Check for zeros in place of VR. This might hint at implicit VR
	// FIXME
	if (m_pWBuf->BufX(nPos+4,2,true) == 0) {
		//unsigned nBad=1;
	}
	strVR = m_pWBuf->BufReadStrn(nPos+4,2);
	nVR = m_pWBuf->BufX(nPos+4,2,false);


	// Value Representation (VR)
	// Reference: Part 5, Section 6.2
	bVrExplicit = false;
	bLen4B = false;

	switch (nVR) {

	// Ref: Part 5, Section 7.1.2 "Data Element Structure with Explicit VR"
	case 'AE':
	case 'AS':
	case 'AT':
	case 'CS':
	case 'DA':
	case 'DS':
	case 'DT':
	case 'FL':
	case 'FD':
	case 'IS':
	case 'LO':
	case 'LT':
	//case 'OB':
	//case 'OF':
	//case 'OW':
	case 'PN':
	case 'SH':
	case 'SL':
	//case 'SQ':
	case 'SS':
	case 'ST':
	case 'TM':
	case 'UI':
	case 'UL':
	//case 'UN':
	case 'US':
	//case 'UT':
		bVrExplicit = true;
		bLen4B = false;
		break;

	// Definition of VRs that have 4B length
	// Reference (informal): http://cran.r-project.org/web/packages/oro.dicom/vignettes/dicom.pdf
	// Ref: Part 5, Section 7.1.2 "Data Element Structure with Explicit VR"
	case 'OB':
	case 'OF':
	case 'OW':
	case 'SQ':
	case 'UN':
		bVrExplicit = true;
		bLen4B = true;
		break;

	// FIXME:
	// Ref: Part 5, Section 7.1.2 "Data Element Structure with Explicit VR"
	case 'UT':
		bVrExplicit = true;
		bLen4B = true;
		break;

	default:
		// FIXME:
		// I have seen some documentation from others that say if VR is not
		// known then it is implicit VR. However, this doesn't seem
		// accurate to me since some implicit VRs could have length values
		// that just happen to alias with a known VR code above!
		bVrExplicit = false;
		bLen4B = true; //FIXME
		break;
	}

	// Force some known implicit VRs to be handled this way
	if ((nTagGroup == 0xFFFE) && (nTagElement == 0xE000)) { bVrExplicit = false; bLen4B = true; }
	if ((nTagGroup == 0xFFFE) && (nTagElement == 0xE00D)) { bVrExplicit = false; bLen4B = true; }
	if ((nTagGroup == 0xFFFE) && (nTagElement == 0xE0DD)) { bVrExplicit = false; bLen4B = true; }


	if (bVrExplicit) {
		if (!bLen4B) {
			// 2B length
			nLen = m_pWBuf->BufX(nPos+6,2,true);
			nOffset = 8;
		} else {
			// 4B length
			// - Skip over 2B field (should be 0)
			nLen = m_pWBuf->BufX(nPos+6,2,true);
            Q_ASSERT(nLen == 0);
			nLen = m_pWBuf->BufX(nPos+8,4,true);
			nOffset = 12;
		}
	} else {
		// Implicit VR
		strVR = "??";
		// 4B length
		nLen = m_pWBuf->BufX(nPos+4,4,true);
		nOffset = 8;
	}


	// Handle Items & Sequences (Image Fragment?)
	bTagIsJpeg = false;
	bTagIsOffsetHdr = false;
	if (!m_bJpegEncap) {
		// Not in a delimited sequence
		// FIXME: Handle special case of sequence of fragments with offset table (7FE0,0010)
		// Ref: Part 7, Section A.4, Table A.4-1
		if ((nTagGroup == 0x7FE0) && (nTagElement == 0x0010)) {
			// Start of Pixel Data
			if (nLen == 0xFFFFFFFF) {
				// Undefined length. So expect encapsulated data with delimiter
				// Expect we aren't already in encapsulated mode
                Q_ASSERT(m_bJpegEncap == false);
				// Indicate we are now starting an encapsulation
				m_bJpegEncap = true;
				// Next tag should be offset header
				m_bJpegEncapOffsetNext = true;
			} else {
				// FIXME: handle native type
                Q_ASSERT(false);
			}
		}
	} else {
		// We are in a sequence
		if ((nTagGroup == 0xFFFE) && (nTagElement == 0xE000)) {
			// Is this an offset header?
			if (m_bJpegEncapOffsetNext) {
                Q_ASSERT(nTagGroup == 0xFFFE);
                Q_ASSERT(nTagElement == 0xE000);
				// Clear next flag since we've handled it
				m_bJpegEncapOffsetNext = false;
				// This is an offset header
				bTagIsOffsetHdr = true;
			} else {
				// Not an offset header, so process it as embedded JPEG
				bTagIsJpeg = true;
			}
		} else if ((nTagGroup == 0xFFFE) && (nTagElement == 0xE0DD)) {
			// Sequence delimiter
			m_bJpegEncap = false;
		}
	}

	if (bTagIsJpeg) {
		nPosJpeg = nPos+8;
	} else {
		// Indicate not a JPEG tag
		nPosJpeg = 0;
	}



#ifdef DICOM_TAG_EXTENDED
	// Extended tag info
	if (bTagIsOffsetHdr) {
		strTag.Format("@ 0x%08X (%04X,%04X) VR=[%2s] Len=0x%08X [OffsetHdr] %s"),nPos,nTagGroup,nTagElement,strVR,nLen,strTagName;
	} else if (bTagIsJpeg) {
		strTag.Format("@ 0x%08X (%04X,%04X) VR=[%2s] Len=0x%08X [JPEG] %s"),nPos,nTagGroup,nTagElement,strVR,nLen,strTagName;
	} else {
		strTag.Format("@ 0x%08X (%04X,%04X) VR=[%2s] Len=0x%08X %s"),nPos,nTagGroup,nTagElement,strVR,nLen,strTagName;
	}
#else
	// Simple tag info
    strTag = strTagName;
#endif

	// Assign struct
	sTagDetail.nTagGroup = nTagGroup;
	sTagDetail.nTagElement = nTagElement;
	sTagDetail.strVR = strVR;
	sTagDetail.nLen = nLen;
	sTagDetail.nOffset = nOffset;

	sTagDetail.bVrExplicit = bVrExplicit;
	sTagDetail.bLen4B = bLen4B;

	sTagDetail.bTagOk = bTagOk;
	sTagDetail.strTag = strTag;
	//sTagDetail.bValOk = bValOk;
	//sTagDetail.strVal = strDesc;

	sTagDetail.bTagIsJpeg = bTagIsJpeg;
	sTagDetail.nPosJpeg = nPosJpeg;
	//sTagDetail.bTagIsOffsetHdr = bTagIsOffsetHdr;

	return true;
}

#if 0
bool CDecodeDicom::DecodeTagHeader(unsigned long nPos,QString &strTag,QString &strVR,unsigned &nLen,unsigned &nOffset,unsigned long &nPosJpeg)
{
	unsigned	nVR = 0;
	QString		strError;
	unsigned	nTagGroup;
	unsigned	nTagElement;
	QString		strTagName = "";

	// Assign defaults
	strTag = "";
	strVR = "";
	nLen = 0;
	nOffset = 0;

	// Find the tag
	nTagGroup = m_pWBuf->BufX(nPos+0,2,true);
	nTagElement = m_pWBuf->BufX(nPos+2,2,true);

	bool		bFoundTag;
	unsigned	nFldInd = 0;
	bFoundTag = FindTag(nTagGroup,nTagElement,nFldInd);

	if (!bFoundTag) {
		// Check for group length entry
		// FIXME:
		// Reference: Part 5, Section 7.2
		if (nTagElement == 0x0000) {
			strTagName.Format("Group Length (Group=%04X)"),nTagGroup;
		} else {
			strError.Format("ERROR: Unknown Tag ID (%04X,%04X) @ 0x%08X"),nTagGroup,nTagElement,nPos;
			m_pLog->AddLineErr(strError);
			strTagName.Format("??? (%04X,%04X)"),nTagGroup,nTagElement;
		}
	} else {
		strTagName = asDicomTags[nFldInd].strTagName;
	}


	// Check for zeros in place of VR. This might hint at implicit VR
	// FIXME
	if (m_pWBuf->BufX(nPos+4,2,true) == 0) {
		unsigned nBad=1;
	}
	strVR = m_pWBuf->BufReadStrn(nPos+4,2);
	nVR = m_pWBuf->BufX(nPos+4,2,false);


	// Value Representation (VR)
	// Reference: Part 5, Section 6.2
	bool	bVrExplicit = false;
	bool	bVrLen4B = false;

	switch (nVR) {

	// Ref: Part 5, Section 7.1.2 "Data Element Structure with Explicit VR"
	case 'AE':
	case 'AS':
	case 'AT':
	case 'CS':
	case 'DA':
	case 'DS':
	case 'DT':
	case 'FL':
	case 'FD':
	case 'IS':
	case 'LO':
	case 'LT':
	//case 'OB':
	//case 'OF':
	//case 'OW':
	case 'PN':
	case 'SH':
	case 'SL':
	//case 'SQ':
	case 'SS':
	case 'ST':
	case 'TM':
	case 'UI':
	case 'UL':
	//case 'UN':
	case 'US':
	//case 'UT':
		bVrExplicit = true;
		bVrLen4B = false;
		break;

	// Definition of VRs that have 4B length
	// Reference (informal): http://cran.r-project.org/web/packages/oro.dicom/vignettes/dicom.pdf
	// Ref: Part 5, Section 7.1.2 "Data Element Structure with Explicit VR"
	case 'OB':
	case 'OF':
	case 'OW':
	case 'SQ':
	case 'UN':
		bVrExplicit = true;
		bVrLen4B = true;
		break;

	// FIXME:
	// Ref: Part 5, Section 7.1.2 "Data Element Structure with Explicit VR"
	case 'UT':
		bVrExplicit = true;
		bVrLen4B = true;
		break;

	default:
		// FIXME:
		// I have seen some documentation from others that say if VR is not
		// known then it is implicit VR. However, this doesn't seem
		// accurate to me since some implicit VRs could have length values
		// that just happen to alias with a known VR code above!
		bVrExplicit = false;
		bVrLen4B = true; //FIXME
		break;
	}

	// Force some known implicit VRs to be handled this way
	if ((nTagGroup == 0xFFFE) && (nTagElement == 0xE000)) { bVrExplicit = false; bVrLen4B = true; }
	if ((nTagGroup == 0xFFFE) && (nTagElement == 0xE00D)) { bVrExplicit = false; bVrLen4B = true; }
	if ((nTagGroup == 0xFFFE) && (nTagElement == 0xE0DD)) { bVrExplicit = false; bVrLen4B = true; }


	if (bVrExplicit) {
		if (!bVrLen4B) {
			// 2B length
			nLen = m_pWBuf->BufX(nPos+6,2,true);
			nOffset = 8;
		} else {
			// 4B length
			// - Skip over 2B field (should be 0)
			nLen = m_pWBuf->BufX(nPos+6,2,true);
            Q_ASSERT(nLen == 0);
			nLen = m_pWBuf->BufX(nPos+8,4,true);
			nOffset = 12;
		}
	} else {
		// Implicit VR
		strVR = "??";
		// 4B length
		nLen = m_pWBuf->BufX(nPos+4,4,true);
		nOffset = 8;
	}


	// Handle Items & Sequences (Image Fragment?)
	bool	bTagIsJpeg = false;
	bool	bTagIsOffsetHdr = false;
	if (!m_bJpegEncap) {
		// Not in a delimited sequence
		// FIXME: Handle special case of sequence of fragments with offset table (7FE0,0010)
		// Ref: Part 7, Section A.4, Table A.4-1
		if ((nTagGroup == 0x7FE0) && (nTagElement == 0x0010)) {
			// Start of Pixel Data
			if (nLen == 0xFFFFFFFF) {
				// Undefined length. So expect encapsulated data with delimiter
				// Expect we aren't already in encapsulated mode
                Q_ASSERT(m_bJpegEncap == false);
				// Indicate we are now starting an encapsulation
				m_bJpegEncap = true;
				// Next tag should be offset header
				m_bJpegEncapOffsetNext = true;
			} else {
				// FIXME: handle native type
                Q_ASSERT(false);
			}
		}
	} else {
		// We are in a sequence
		if ((nTagGroup == 0xFFFE) && (nTagElement == 0xE000)) {
			// Is this an offset header?
			if (m_bJpegEncapOffsetNext) {
                Q_ASSERT(nTagGroup == 0xFFFE);
                Q_ASSERT(nTagElement == 0xE000);
				// Clear next flag since we've handled it
				m_bJpegEncapOffsetNext = false;
				// This is an offset header
				bTagIsOffsetHdr = true;
			} else {
				// Not an offset header, so process it as embedded JPEG
				bTagIsJpeg = true;
			}
		} else if ((nTagGroup == 0xFFFE) && (nTagElement == 0xE0DD)) {
			// Sequence delimiter
			m_bJpegEncap = false;
		}
	}



#ifdef DICOM_TAG_EXTENDED
	// Extended tag info
	if (bTagIsOffsetHdr) {
		strTag.Format("@ 0x%08X (%04X,%04X) VR=[%2s] Len=0x%08X [OffsetHdr] %s"),nPos,nTagGroup,nTagElement,strVR,nLen,strTagName;
	} else if (bTagIsJpeg) {
		strTag.Format("@ 0x%08X (%04X,%04X) VR=[%2s] Len=0x%08X [JPEG] %s"),nPos,nTagGroup,nTagElement,strVR,nLen,strTagName;
	} else {
		strTag.Format("@ 0x%08X (%04X,%04X) VR=[%2s] Len=0x%08X %s"),nPos,nTagGroup,nTagElement,strVR,nLen,strTagName;
	}
#else
	// Simple tag info
	strTag.Format("%s"),strTagName;
#endif

	if (bTagIsJpeg) {
		nPosJpeg = nPos+8;
	} else {
		// Indicate not a JPEG tag
		nPosJpeg = 0;
	}

	return true;
}
#endif

// Determine if the file is a DICOM
// If so, parse the headers. Generally want to start at start of file (nPos=0).
bool CDecodeDicom::DecodeDicom(unsigned long nPos,unsigned long nPosFileEnd,unsigned long &nPosJpeg)
{
	unsigned		nLen;
	unsigned		nOffset;
	QString			strTag;
	QString			strSig;
	QString	 		strVR;


	m_bDicom = false;


	// Skip File Preamble
	nPos += 128;

	// DICOM Prefix
	strSig = m_pWBuf->BufReadStrn(nPos,4);
	if (strSig == "DICM") {
		m_bDicom = true;
		m_pLog->AddLine("");
		m_pLog->AddLineHdr("*** DICOM File Decoding ***");
		m_pLog->AddLine("Decoding DICOM format...");
		m_pLog->AddLine("");
	} else {
		return false;
	}
	nPos+=4;

	bool bDone = false;

	// If we get here, then we have found the header signature
	// FIXME: Handle proper termination!
	if (nPos >= nPosFileEnd) {
		bDone = true;
	}

	//bool			bFoundJpeg = false;
	unsigned long	nPosJpegFound = 0;
	tsTagDetail		sTagDetail;
	while (!bDone) {

		sTagDetail.Reset();
		GetTagHeader(nPos,sTagDetail);

		// Fetch results
		nLen = sTagDetail.nLen;
		nPosJpegFound = sTagDetail.nPosJpeg;
		strTag = sTagDetail.strTag;
		strVR = sTagDetail.strVR;
		nOffset = sTagDetail.nOffset;

		// Advance the file position
		nPos += sTagDetail.nOffset;

		// Process the field
		if (nLen == 0xFFFFFFFF) {
			// Detect "invalid length"
			// In these cases we are probably dependent upon delimiters
			// so we don't have any real content in this tag.
			//
			// Example: E:\JPEG\JPEGsnoop Testcases\DICOM_samples\24-bit JPEG Lossy Color.dcm
			// Offset:  0x508
			nLen = 0;
		}

		if (nPosJpegFound != 0) {
			// Only capture first image offset
			if (nPosJpeg == 0) {
				nPosJpeg = nPosJpegFound;
			}
		}


		// See if this is an enumerated value
		// If not, just report as hex for now

		// Now determine enum value
		QString		strValTrunc = "";
		QString		strDesc = "";
		bool		bIsEnum = false;
		strValTrunc = m_pWBuf->BufReadStrn(nPos,200);
		bIsEnum = LookupEnum(sTagDetail.nTagGroup,sTagDetail.nTagElement,strValTrunc,strDesc);
		if (bIsEnum) {
			ReportFldStrEnc(1,sTagDetail.strTag,strDesc,strValTrunc);
		} else {
			ReportFldHex(1,strTag,nPos,nLen);
		}

		nPos += nLen;

		if (nPos >= nPosFileEnd) {
			bDone = true;
		}

	} // bDone


	return true;

}

// Create indent string
// - Returns a sequence of spaces according to the indent level
QString CDecodeDicom::ParseIndent(unsigned nIndent)
{
	QString	strIndent = "";
	for (unsigned nInd=0;nInd<nIndent;nInd++) {
		strIndent += "  ";
	}
	return strIndent;
}

// Display a formatted string field
// - Report the value with the field name (strField) and current indent level (nIndent)
void CDecodeDicom::ReportFldStr(unsigned nIndent,QString strField,QString strVal)
{
	QString		strIndent;
	QString		strLine;

	strIndent = ParseIndent(nIndent);
    strLine.arg(strIndent).arg(strField, -50).arg(strVal); //Format("%s%-50s = \"%s\""),(LPCTSTR)strIndent,(LPCTSTR)strField,(LPCTSTR)strVal;
	m_pLog->AddLine(strLine);
}

// Display a formatted string field with encoded value
// - Report the value with the field name (strField) and current indent level (nIndent)
void CDecodeDicom::ReportFldStrEnc(unsigned nIndent,QString strField,QString strVal,QString strEncVal)
{
	QString		strIndent;
	QString		strLine;

	strIndent = ParseIndent(nIndent);
    strLine.arg(strIndent).arg(strField, -50).arg(strVal); //Format("%s%-50s = \"%s\""),(LPCTSTR)strIndent,(LPCTSTR)strField,(LPCTSTR)strVal;
    m_pLog->AddLine(strLine);
}

// Locate a field ID in the constant Meta Tag array
//
// INPUT:
// - nTagGroup				= Meta Tag (High)
// - nTagElement				= Meta Tag (Low)
// OUTPUT:
// - nFldInd			= Index into asDicomTags[] array
// RETURN:
// - Returns true if ID was found in array
// NOTE:
// - The constant struct array depends on DICOM_T_END as a terminator for the list
//
bool CDecodeDicom::FindTag(unsigned nTagGroup,unsigned nTagElement,unsigned &nFldInd)
{
	unsigned	nInd=0;
	bool		bDone=false;
	bool		bFound=false;
	while (!bDone) {
		if (asDicomTags[nInd].eTagType == DICOM_T_END) {
			bDone = true;
		} else {
			unsigned nFoundTagH = asDicomTags[nInd].nTagGroup;
			unsigned nFoundTagL = asDicomTags[nInd].nTagElement;
			if ((nTagGroup == nFoundTagH) && (nTagElement == nFoundTagL)) {
				// Matched
				bDone = true;
				bFound = true;
			} else {
				nInd++;
			}

		}
	}
	if (bFound) {
		nFldInd = nInd;
	}
	return bFound;
}



//
// Display a formatted hexadecimal field
// - Report the value with the field name (strField) and current indent level (nIndent)
// - Depending on the length of the buffer, either report the hex value in-line with
//   the field name or else start a new row and begin a hex dump
// - Also report the ASCII representation alongside the hex dump
//
// INPUT:
// - nIndent		= Current indent level for reporting
// - strField		= Field name
// - nPosStart		= Field byte array file position start
// - nLen			= Field byte array length
//
void CDecodeDicom::ReportFldHex(unsigned nIndent,QString strField,unsigned long nPosStart,unsigned nLen)
{
	QString		strIndent;
	unsigned	nByte;
	QString		strPrefix;
	QString		strByteHex;
	QString		strByteAsc;
	QString		strValHex;
	QString		strValAsc;
	QString		strLine;

	strIndent = ParseIndent(nIndent);

	if (nLen == 0) {
		// Print out the header row, but no data will be shown
        strLine.arg(strIndent).arg(strField, -50); // ("%s%-50s = "),(LPCTSTR)strIndent,(LPCTSTR)strField;
		m_pLog->AddLine(strLine);
		// Nothing to report, exit now
		return;
	} else if (nLen <= DC_HEX_MAX_INLINE) {
		// Define prefix for row
        strPrefix.arg(strIndent).arg(strField, -50); // .Format("%s%-50s = "),(LPCTSTR)strIndent,(LPCTSTR)strField;
	} else {
#if 0
		// Print out header row
		strLine.Format("%s%-50s ="),(LPCTSTR)strIndent,(LPCTSTR)strField;
		m_pLog->AddLine(strLine);
		// Define prefix for next row
		//strPrefix.Format("%s"),(LPCTSTR)strIndent;
		strPrefix.Format("%s%-50s   "),(LPCTSTR)strIndent,_T("");
#else
		// Define prefix for row
		//FIXME: Only report field on first row
        strPrefix.arg(strIndent).arg(strField, -50); //.Format("%s%-50s = "),(LPCTSTR)strIndent,(LPCTSTR)strField;
#endif
	}


	// Build up the hex string
	// Limit to DC_HEX_TOTAL bytes
	unsigned	nRowOffset = 0;
	bool		bDone = false;
	unsigned	nLenClip;
    nLenClip = qMin(nLen, unsigned(DC_HEX_TOTAL));
	strValHex = "";
	strValAsc = "";
	while (!bDone) {
		// Have we reached the end of the data we wish to display?
		if (nRowOffset>=nLenClip) {
			bDone = true;
		} else {
			// Reset the cumulative hex/ascii value strings
			strValHex = "";
			strValAsc = "";
			// Build the hex/ascii value strings
			for (unsigned nInd=0;nInd<DC_HEX_MAX_ROW;nInd++) {
				if ((nRowOffset+nInd)<nLenClip) {
					nByte = m_pWBuf->Buf(nPosStart+nRowOffset+nInd);
                    strByteHex.arg(nByte, 2, 16, QChar('0')); // .Format("%02X "),nByte;
					// Only display printable characters
					if (isprint(nByte)) {
                        strByteAsc.arg(nByte); // .Format("%c"),nByte;
					} else {
						strByteAsc = ".";
					}
				} else {
					// Pad out to end of row
                    strByteHex = "   ";
					strByteAsc = " ";
				}
				// Build up the strings
				strValHex += strByteHex;
				strValAsc += strByteAsc;
			}

			// Generate the line with Hex and ASCII representations
            strLine.arg(strPrefix, " | 0x", strValHex, " | ", strValAsc); // .Format("%s | 0x%s | %s"),(LPCTSTR)strPrefix,(LPCTSTR)strValHex,(LPCTSTR)strValAsc;
			m_pLog->AddLine(strLine);

			// Now increment file offset
			nRowOffset += DC_HEX_MAX_ROW;
		}
	}

	// If we had to clip the display length, then show ellipsis now
	if (nLenClip < nLen) {
        strLine.arg(strPrefix, " | ..."); // .Format("%s | ..."),(LPCTSTR)strPrefix;
		m_pLog->AddLine(strLine);
	}


}

// Display a formatted enumerated type field
// - Look up enumerated constant (nVal) for the given field (eEnumField)
// - Report the value with the field name (strField) and current indent level (nIndent)
void CDecodeDicom::ReportFldEnum(unsigned nIndent,QString strField,unsigned nTagGroup,unsigned nTagElement,QString strVal)
{
	QString		strIndent;
	QString		strDesc;
	QString		strLine;
	bool		bFound;

	strIndent = ParseIndent(nIndent);

	bFound = LookupEnum(nTagGroup,nTagElement,strVal,strDesc);
	if (bFound) {
        strLine.arg(strIndent).arg(strField, -50).arg(strDesc); // .Format("%s%-50s = %s"),(LPCTSTR)strIndent,(LPCTSTR)strField,(LPCTSTR)strDesc;
		m_pLog->AddLine(strLine);
	} else {
        Q_ASSERT(false);
	}
}

// Look up the enumerated constant (nVal) for the specified field (eEnumField)
// - Returns "" if the field wasn't found
bool CDecodeDicom::LookupEnum(unsigned nTagGroup,unsigned nTagElement,QString strVal,QString &strDesc)
{
	// Find the enum value
	bool		bFound = false;
	bool		bDone = false;
	unsigned	nEnumInd=0;
	while (!bDone) {
		if ( (asTagSpecial[nEnumInd].nTagGroup == nTagGroup) && (asTagSpecial[nEnumInd].nTagElement == nTagElement) ) {
			if (asTagSpecial[nEnumInd].strVal == strVal) {
				bDone = true;
				bFound = true;
				strDesc = asTagSpecial[nEnumInd].strDefinition;
			}
		}
		if (asTagSpecial[nEnumInd].strVal == "") {
			bDone = true;
		}
		if (!bDone) {
			nEnumInd++;
		}
	}
	return bFound;
}

// ===============================================================================
// CONSTANTS
// ===============================================================================
