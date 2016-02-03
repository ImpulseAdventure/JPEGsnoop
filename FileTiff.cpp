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

#include "stdafx.h"

#include "FileTiff.h"


FileTiff::FileTiff()
{
	m_pFileOutput = NULL;
	m_nPtrIfdExtra = 0;
	m_nPtrImg = 0;
	m_nPos = 0;
	m_pIfdExtraBuf = NULL;
}

FileTiff::~FileTiff()
{
	if (m_pFileOutput) {
		delete m_pFileOutput;
		m_pFileOutput = NULL;
	}
	if (m_pIfdExtraBuf) {
		delete m_pIfdExtraBuf;
		m_pIfdExtraBuf = NULL;
	}
}

void FileTiff::WriteVal8(BYTE nVal)
{
	if (!m_bPreCalc) {
		m_pFileOutput->Write(&nVal,1);
	}
	m_nPos += 1;
}
void FileTiff::WriteIfdExtraBuf8(BYTE nVal)
{
	if (!m_bPreCalc) {
		ASSERT (m_pIfdExtraBuf);
		m_pIfdExtraBuf[m_nIfdExtraLen+0] = nVal;
	}
	m_nIfdExtraLen += 1;

}

void FileTiff::WriteVal16(unsigned short nVal)
{
	if (!m_bPreCalc) {
		BYTE	nTmp[2];
		nTmp[0] = (nVal & 0xFF00) >> 8;
		nTmp[1] = (nVal & 0x00FF) >> 0;
		m_pFileOutput->Write(&(nTmp[0]),1);
		m_pFileOutput->Write(&(nTmp[1]),1);
	}
	m_nPos += 2;
}
void FileTiff::WriteIfdExtraBuf16(unsigned short nVal)
{
	if (!m_bPreCalc) {
		BYTE	nTmp[2];
		nTmp[0] = (nVal & 0xFF00) >> 8;
		nTmp[1] = (nVal & 0x00FF) >> 0;
		ASSERT (m_pIfdExtraBuf);
		m_pIfdExtraBuf[m_nIfdExtraLen+0] = nTmp[0];
		m_pIfdExtraBuf[m_nIfdExtraLen+1] = nTmp[1];
	}
	m_nIfdExtraLen += 2;
}


void FileTiff::WriteVal32(unsigned int nVal)
{
	if (!m_bPreCalc) {
		BYTE	nTmp[4];
		nTmp[0] = (nVal & 0xFF000000) >> 24;
		nTmp[1] = (nVal & 0x00FF0000) >> 16;
		nTmp[2] = (nVal & 0x0000FF00) >> 8;
		nTmp[3] = (nVal & 0x000000FF) >> 0;
		m_pFileOutput->Write(&(nTmp[0]),1);
		m_pFileOutput->Write(&(nTmp[1]),1);
		m_pFileOutput->Write(&(nTmp[2]),1);
		m_pFileOutput->Write(&(nTmp[3]),1);
	}
	m_nPos += 4;
}
void FileTiff::WriteIfdExtraBuf32(unsigned int nVal)
{
	if (!m_bPreCalc) {
		BYTE	nTmp[4];
		nTmp[0] = (nVal & 0xFF000000) >> 24;
		nTmp[1] = (nVal & 0x00FF0000) >> 16;
		nTmp[2] = (nVal & 0x0000FF00) >> 8;
		nTmp[3] = (nVal & 0x000000FF) >> 0;
		ASSERT (m_pIfdExtraBuf);
		m_pIfdExtraBuf[m_nIfdExtraLen+0] = nTmp[0];
		m_pIfdExtraBuf[m_nIfdExtraLen+1] = nTmp[1];
		m_pIfdExtraBuf[m_nIfdExtraLen+2] = nTmp[2];
		m_pIfdExtraBuf[m_nIfdExtraLen+3] = nTmp[3];
	}
	m_nIfdExtraLen += 4;
}


void FileTiff::WriteIfdEntrySingle(unsigned nTag,unsigned nType,unsigned nValOffset)
{
	unsigned	nNum;

	nNum = 1;
	WriteVal16(nTag);
	WriteVal16(nType);
	WriteVal32(nNum);
	switch (nType) {
		case TIFF_TYPE_BYTE:
			WriteVal8(nValOffset);
			WriteVal8(0x00);
			WriteVal16(0x0000);
			break;
		case TIFF_TYPE_ASCII:
			// The value of the Count part of an ASCII field entry includes the NUL
			// Therefore, it is not valid for a "Single" entry.
			ASSERT(false);
			break;
		case TIFF_TYPE_SHORT:
			WriteVal16(nValOffset);
			WriteVal16(0x0000);
			break;
		case TIFF_TYPE_LONG:
			WriteVal32(nValOffset);
			break;
		default:
			ASSERT(false);
			break;
	}
	m_nNumIfd++;
}

/*
Types
The field types and their sizes are:
1 = BYTE 8-bit unsigned integer.
2 = ASCII 8-bit byte that contains a 7-bit ASCII code; the last byte
must be NUL (binary zero).
3 = SHORT 16-bit (2-byte) unsigned integer.
4 = LONG 32-bit (4-byte) unsigned integer.
5 = RATIONAL Two LONGs: the first represents the numerator of a
fraction; the second, the denominator.
The value of the Count part of an ASCII field entry includes the NUL. If padding
is necessary, the Count does not include the pad byte. Note that there is no initial
“count byte” as in Pascal-style strings.
*/

unsigned FileTiff::GetTypeLen(unsigned nType)
{
	unsigned nLen=0;
	switch(nType) {
		case TIFF_TYPE_BYTE:
		case TIFF_TYPE_ASCII:
			nLen = 1;
			break;
		case TIFF_TYPE_SHORT:
			nLen = 2;
			break;
		case TIFF_TYPE_LONG:
			nLen = 4;
			break;
		case TIFF_TYPE_RATIONAL:
			// We set it to be 4 here instead of 8
			// because nNumVals is 2x nNumVals
			nLen = 4;
			break;
		default:
			ASSERT(false);
			break;
	}
	return nLen;
}

// Use this if we are using external reference
void FileTiff::WriteIfdEntryMult(unsigned nTag,unsigned nType,unsigned nNumVals,unsigned* nVals)
{
	// Calculate total length so we can determine if the
	// values fit inside a DWORD
	unsigned	nTotalLen;
	bool		bInExtra=false;
	nTotalLen = nNumVals * GetTypeLen(nType);
	if (nTotalLen > 4) { bInExtra = true; }

	WriteVal16(nTag);
	WriteVal16(nType);
	WriteVal32((nType!=TIFF_TYPE_RATIONAL)?nNumVals:nNumVals/2);
	if (bInExtra) {
		WriteVal32(m_nPtrIfdExtra+m_nIfdExtraLen);
	}

	// Increment the number of IFD entries
	m_nNumIfd++;


	// Now start writing to IfdExtraBuf
	unsigned	nVal;

	for (unsigned nInd=0;nInd<nNumVals;nInd++) {
		nVal = nVals[nInd];
		switch (nType) {
			case TIFF_TYPE_BYTE:
				if (bInExtra) {
					WriteIfdExtraBuf8(nVal);
				} else {
					WriteVal8(nVal);
				}
				break;
			case TIFF_TYPE_ASCII:
				if (bInExtra) {
					WriteIfdExtraBuf8(nVal);
				} else {
					WriteVal8(nVal);
				}
				break;
			case TIFF_TYPE_SHORT:
				if (bInExtra) {
					WriteIfdExtraBuf16(nVal);
				} else {
					WriteVal16(nVal);
				}
				break;
			case TIFF_TYPE_LONG:
				if (bInExtra) {
					WriteIfdExtraBuf32(nVal);
				} else {
					WriteVal32(nVal);
				}
				break;
			case TIFF_TYPE_RATIONAL:
				// Expect two longs in array, so this gets
				// called twice per rational
				if (bInExtra) {
					WriteIfdExtraBuf32(nVal);
				} else {
					WriteVal32(nVal);
				}
				break;
			default:
				ASSERT(false);
				break;
		}
	}

	// Do we need to pad?
	if ((!bInExtra) && (nTotalLen < 4)) {
		// Need to pad out with NULLs
		for (unsigned nPad=0;nPad<(4-nTotalLen);nPad++) {
			WriteVal8(0x00);
		}
	}


}


void FileTiff::WriteIfd(unsigned nSizeX,unsigned nSizeY,bool bModeYcc,bool bMode16b)
{
	ASSERT(m_pFileOutput);

	unsigned	anVals[16];

	bool		bPreCalcSaved;

	unsigned	nFinalNumIfd;
	unsigned	nFinalIfdExtraLen;
	unsigned	nFinalPtrIfdStart;
	unsigned	nFinalPtrIfdExtra;
	unsigned	nFinalPtrIfdEnd;


	// Save the PreCalc mode
	bPreCalcSaved = m_bPreCalc;

	// Save the position of the start of IFD

	m_nPtrIfdExtra = 0;
	m_nIfdExtraLen = 0;

	ASSERT(m_pIfdExtraBuf==NULL);

	nFinalNumIfd = 0;
	nFinalPtrIfdStart = m_nPos;
	nFinalPtrIfdExtra = 0;
	nFinalPtrIfdEnd = 0;
	nFinalIfdExtraLen = 0;

	for (unsigned nPass=0;nPass<2;nPass++) {

		m_bPreCalc = (nPass == 0)?true:false;

		m_nPos = nFinalPtrIfdStart;

		m_nNumIfd = 0;
		m_nIfdExtraLen = 0;	// Number of bytes used in IfdExtra Buf

		// The following WriteIfdEntry calls will use "m_nPtrIfdExtra"
		// when any reference needs to be made. In PreCalc pass, these
		// will all be invalid as "m_nPtrIfdExtraStart" has not been
		// calculated until we finished the pass. In real pass, we
		// have calculated the "m_nPtrIfdExtraStart" and set up the
		// "m_nPtrIfdExtra" to begin there.
		if (!m_bPreCalc) {
			m_nPtrIfdExtra = nFinalPtrIfdExtra;

			// Allocate the IfdExtra buffer now that
			// we know how large it will be
			m_pIfdExtraBuf = new BYTE[nFinalIfdExtraLen];
			ASSERT(m_pIfdExtraBuf);
		}
		// Number of Dir Entries
		WriteVal16(nFinalNumIfd);

		WriteIfdEntrySingle(TIFF_TAG_IMG_WIDTH,TIFF_TYPE_SHORT,nSizeX);
		WriteIfdEntrySingle(TIFF_TAG_IMG_HEIGHT,TIFF_TYPE_SHORT,nSizeY);
		anVals[0] = (bMode16b)?16:8;
		anVals[1] = (bMode16b)?16:8;
		anVals[2] = (bMode16b)?16:8;
		WriteIfdEntryMult(TIFF_TAG_BITS_PER_SAMP,TIFF_TYPE_SHORT,3,anVals);
		WriteIfdEntrySingle(TIFF_TAG_COMPRESSION,TIFF_TYPE_SHORT,1);
		unsigned nValPhotomInterp = (bModeYcc)?6:2;
		WriteIfdEntrySingle(TIFF_TAG_PHOTOM_INTERP,TIFF_TYPE_SHORT,nValPhotomInterp);

		WriteIfdEntrySingle(TIFF_TAG_STRIP_OFFSETS,TIFF_TYPE_SHORT,m_nPtrImg);	//?
		WriteIfdEntrySingle(TIFF_TAG_ORIENTATION,TIFF_TYPE_SHORT,1);
		WriteIfdEntrySingle(TIFF_TAG_SAMP_PER_PIX,TIFF_TYPE_SHORT,3);
		WriteIfdEntrySingle(TIFF_TAG_ROWS_PER_STRIP,TIFF_TYPE_SHORT,nSizeY);
		unsigned nBytePerPix = (bMode16b)?6:3;
		WriteIfdEntrySingle(TIFF_TAG_STRIP_BYTE_COUNTS,TIFF_TYPE_LONG,nSizeY*nSizeX*nBytePerPix);

		anVals[0]  =   72; anVals[1]  = 1; // 72 DPI
		WriteIfdEntryMult(TIFF_TAG_X_RESOLUTION,TIFF_TYPE_RATIONAL,2,anVals);
		WriteIfdEntryMult(TIFF_TAG_Y_RESOLUTION,TIFF_TYPE_RATIONAL,2,anVals);
		WriteIfdEntrySingle(TIFF_TAG_PLANAR_CONFIG,TIFF_TYPE_SHORT,1);
		WriteIfdEntrySingle(TIFF_TAG_RESOLUTION_UNIT,TIFF_TYPE_SHORT,2);	// Inches


		if (bModeYcc) {
			// Add the extra tags required
			anVals[0] = 299; anVals[1] = 1000;
			anVals[2] = 587; anVals[3] = 1000;
			anVals[4] = 114; anVals[5] = 1000;
			WriteIfdEntryMult(TIFF_TAG_YCC_COEFFS,TIFF_TYPE_RATIONAL,6,anVals);
			anVals[0] = 1;
			anVals[1] = 1;
			WriteIfdEntryMult(TIFF_TAG_YCC_SUBSAMP,TIFF_TYPE_SHORT,2,anVals);
			WriteIfdEntrySingle(TIFF_TAG_POSITIONING,TIFF_TYPE_SHORT,1);
		}

		// For YCC 8b: [0/1, 255/1,128/1, 255/1, 128/1, 255/1]
		if (!bModeYcc) {
			anVals[0]  = 0x00; anVals[1]  = 1; // R - Black
			anVals[2]  = 0xFF; anVals[3]  = 1; // R - White
			anVals[4]  = 0x00; anVals[5]  = 1; // G - Black
			anVals[6]  = 0xFF; anVals[7]  = 1; // G - White
			anVals[8]  = 0x00; anVals[9]  = 1; // B - Black
			anVals[10] = 0xFF; anVals[11] = 1; // B - White
		} else {
			anVals[0]  = 0x00; anVals[1]  = 1; // Y  - Black
			anVals[2]  = 0xFF; anVals[3]  = 1; // Y  - White
			anVals[4]  = 0x00; anVals[5]  = 1; // Cb - Black
			anVals[6]  = 0xFF; anVals[7]  = 1; // Cb - White
			anVals[8]  = 0x00; anVals[9]  = 1; // Cr - Black
			anVals[10] = 0xFF; anVals[11] = 1; // Cr - White
		}
		WriteIfdEntryMult(TIFF_TAG_REF_BLACK_WHITE,TIFF_TYPE_RATIONAL,12,anVals);

		// End of IFD
		WriteVal32(0x00000000);

		// Save the pointer to the start of extra IFD region
		nFinalPtrIfdExtra = m_nPos;

		// Save the count of # IFD entries
		nFinalNumIfd = m_nNumIfd;

		nFinalIfdExtraLen = m_nIfdExtraLen;

		unsigned nVal;
		// Now write out IFD Extra buffer
		if (m_nIfdExtraLen > 0) {
			if (!m_bPreCalc) {
				ASSERT(m_pIfdExtraBuf);
			}

			for (unsigned nBufInd=0;nBufInd<m_nIfdExtraLen;nBufInd++) {
				// In PreCalc phase, fill with placeholder bytes
				nVal = (m_bPreCalc)?0x00:m_pIfdExtraBuf[nBufInd];
				WriteVal8(nVal);
			}
		}


		nFinalPtrIfdEnd = m_nPos;
		m_nPtrImg = nFinalPtrIfdEnd;

	}

	// Elininate the IFD Extra buffer
	if (m_pIfdExtraBuf) {
		delete m_pIfdExtraBuf;
		m_pIfdExtraBuf = NULL;
	}


	// Restore the PreCalc mode
	m_bPreCalc = bPreCalcSaved;

}



void FileTiff::WriteFile(CString sFnameOut,bool bModeYcc,bool bMode16b,void* pBitmap,unsigned nSizeX,unsigned nSizeY)
{
	unsigned char*	pBitmap8;
	unsigned short*	pBitmap16;

	pBitmap8 = reinterpret_cast<unsigned char *>(pBitmap);
	pBitmap16 = reinterpret_cast<unsigned short *>(pBitmap);


	ASSERT(sFnameOut != _T(""));

	try
	{
		// Open specified file
		// Added in shareDenyNone as this apparently helps resolve some people's troubles
		// with an error showing: Couldn't open file "Sharing Violation"
		m_pFileOutput = new CFile(sFnameOut, CFile::modeCreate| CFile::modeWrite | CFile::typeBinary | CFile::shareDenyNone);
	}
	catch (CFileException* e)
	{
		TCHAR msg[MAX_BUF_EX_ERR_MSG];
		CString strError;
		e->GetErrorMessage(msg,MAX_BUF_EX_ERR_MSG);
		e->Delete();
		strError.Format(_T("ERROR: Couldn't open file for write [%s]: [%s]"),
			(LPCTSTR)sFnameOut, msg);
		AfxMessageBox(strError);
		m_pFileOutput = NULL;

		return;

	}

	unsigned	nIfdPtr;

	m_bPreCalc = false;
	m_nPos = 0;

	// This will get updated after pass 1 of WriteIfd()
	m_nPtrImg = 0;

	// TIFF Header
	WriteVal32(0x4D4D002A);

	// IFD Directory Ptr, place at start of file
	nIfdPtr = 8;
	WriteVal32(nIfdPtr);

	// IFD
	WriteIfd(nSizeX,nSizeY,bModeYcc,bMode16b);

	// Skip to start of image
	unsigned nPad;
	nPad = m_nPtrImg - m_nPos;
	for (unsigned nInd=0;nInd<nPad;nInd++) {
		WriteVal8(0x00);
	}

	// Image Data
	if (pBitmap) {
		// The pBitmap arrays have already been arranged into
		// file order, which is often reversed from memory (RGB)
		// sequence.
		if (!bMode16b) {
			m_pFileOutput->Write(pBitmap8,nSizeX*nSizeY*3*1);
			m_nPos += nSizeX*nSizeY*3*1;
		} else {
			m_pFileOutput->Write(pBitmap16,nSizeX*nSizeY*3*2);
			m_nPos += nSizeX*nSizeY*3*2;
		}

	}


	if (!pBitmap) {
		for (unsigned nIndY=0;nIndY<nSizeY;nIndY++) {
			for (unsigned nIndX=0;nIndX<nSizeX;nIndX++) {
				if (!bMode16b) {
					WriteVal8(0x00 + nIndX*32+nIndY*16);	// R
					WriteVal8(0x20 + nIndX*16+nIndY*16);	// G
					WriteVal8(0x80 + nIndX*8+nIndY*16);		// B
				} else {
					WriteVal16(0x00 + nIndX*32+nIndY*16);	// R
					WriteVal16(0x20 + nIndX*16+nIndY*16);	// G
					WriteVal16(0x80 + nIndX*8+nIndY*16);	// B
				}
			}
		}
	}




	m_pFileOutput->Close();


	// Clean up
	// Don't really need to delete m_pFileOutput
	if (m_pFileOutput) {
		delete m_pFileOutput;
		m_pFileOutput = NULL;
	}
}