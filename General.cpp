// JPEGsnoop - JPEG Image Decoder & Analysis Utility
// Copyright (C) 2014 - Calvin Hass
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

#include "StdAfx.h"

// ---------------------------------------
// Global functions
// ---------------------------------------


CString Dec2Bin(unsigned nVal,unsigned nLen,bool bSpace)
{
	unsigned	nBit;
	CString		strBin = _T("");
	for (int nInd=nLen-1;nInd>=0;nInd--)
	{
		nBit = ( nVal & (1 << nInd) ) >> nInd;
		strBin += (nBit==1)?_T("1"):_T("0");
		if ( ((nInd % 8) == 0) && (nInd != 0) ) {
			if (bSpace) {
				strBin += _T(" ");
			}
		}
	}
	return strBin;
}

// Perform byteswap which is used to create packed image array
// before write-out of 16b values to disk
unsigned short Swap16(unsigned short nVal)
{
	unsigned nValHi,nValLo;
	nValHi = (nVal & 0xFF00)>>8;
	nValLo = (nVal & 0x00FF);
	return (nValLo<<8) + nValHi;
}

// Simple helper routine to test whether an indexed bit is set
bool TestBit(unsigned nVal,unsigned nBit)
{
	unsigned nTmp;
	nTmp = (nVal & (1<<nBit));
	if (nTmp != 0) {
		return true;
	} else {
		return false;
	}
}

// Convert between unsigned integer to a 4-byte character string
// (also known as FourCC codes). This field type is used often in
// ICC profile entries.
CString Uint2Chars(unsigned nVal)
{
	CString strTmp;
	char c3,c2,c1,c0;
	c3 = char((nVal & 0xFF000000)>>24);
	c2 = char((nVal & 0x00FF0000)>>16);
	c1 = char((nVal & 0x0000FF00)>>8);
	c0 = char(nVal & 0x000000FF);
	c3 = (c3 == 0)?'.':c3;
	c2 = (c2 == 0)?'.':c2;
	c1 = (c1 == 0)?'.':c1;
	c0 = (c0 == 0)?'.':c0;
	strTmp.Format(_T("'%c%c%c%c' (0x%08X)"),
		c3,c2,c1,c0,nVal);
	return strTmp;
}

// Convert between unsigned int and a dotted-byte notation
CString Uint2DotByte(unsigned nVal)
{
	CString strTmp;
	strTmp.Format(_T("'%u.%u.%u.%u' (0x%08X)"),
		((nVal & 0xFF000000)>>24),
		((nVal & 0x00FF0000)>>16),
		((nVal & 0x0000FF00)>>8),
		(nVal & 0x000000FF),
		nVal);
	return strTmp;
}

/*
// Convert a byte array into a unicode CString
// - Clips to MAX_UNICODE_STRLEN
//
// INPUT:
// - pBuf				= Byte array containing encoded unicode string
// - nBufLen			= Number of unicode characters (16b) to read
// RETURN:
// - CString containing unicode characters
//
#define MAX_UNICODE_STRLEN	255
CString ByteStr2Unicode(BYTE* pBuf, unsigned nBufLen)
{
	CString		strUni;
	BYTE		anStrBuf[(MAX_UNICODE_STRLEN+1)*2];
	wchar_t		acStrBuf[(MAX_UNICODE_STRLEN+1)];

	// Copy bytes into local buffer to ensure it is truncated and
	// properly terminated
	unsigned	nStrPos = 0;
	BYTE		nByte0,nByte1;
	bool		bDone = false;
	while (!bDone) {
		if (nStrPos>nBufLen) {
			// Exceeded the indicated string length
			bDone = true;
		} else if (nStrPos>MAX_UNICODE_STRLEN) {
			// Exceeded our maximum string conversion length
			bDone = true;
		} else {
			// Fetch the next two bytes
			nByte0 = pBuf[(nStrPos*2)+0];
			nByte1 = pBuf[(nStrPos*2)+1];
			// Check in case we see terminator
			if ((nByte0==0) && (nByte1==0)) {
				// Don't copy over it now as we always pad with terminator after
				bDone = true;
			}
		}
		// If we haven't found a reason to stop, copy over the bytes
		if (!bDone) {
			anStrBuf[(nStrPos*2)+0] = nByte0;
			anStrBuf[(nStrPos*2)+1] = nByte1;
			// Increment index
			nStrPos++;
		}
	}
	// Now ensure we are terminated properly by enforcing terminator
	anStrBuf[(nStrPos*2)+0] = 0;
	anStrBuf[(nStrPos*2)+1] = 0;

	// Copy into unicode string
	// - This routine requires it to be terminated first!
	lstrcpyW(acStrBuf,(LPCWSTR)anStrBuf);

	// Finally copy back into CString
	strUni = acStrBuf;
	return strUni;
}
*/


// ---------------------------------------
// Global constants
// ---------------------------------------

// ZigZag DQT coefficient reordering matrix as defined by the ITU T.81 standard.
extern const unsigned glb_anZigZag[64] =
{
	 0, 1, 8,16, 9, 2, 3,10,
	17,24,32,25,18,11, 4, 5,
	12,19,26,33,40,48,41,34,
	27,20,13, 6, 7,14,21,28,
	35,42,49,56,57,50,43,36,
	29,22,15,23,30,37,44,51,
	58,59,52,45,38,31,39,46,
	53,60,61,54,47,55,62,63
};

// Reverse ZigZag reordering matrix, based on ITU T.81
extern const unsigned glb_anUnZigZag[64] =
{
	 0, 1, 5, 6,14,15,27,28,
	 2, 4, 7,13,16,26,29,42,
	 3, 8,12,17,25,30,41,43,
	 9,11,18,24,31,40,44,53,
	10,19,23,32,39,45,52,54,
	20,22,33,38,46,51,55,60,
	21,34,37,47,50,56,59,61,
	35,36,48,49,57,58,62,63
};


// This matrix is used to convert a DQT table into its
// rotated form (ie. by 90 degrees), used in the signature
// search functionality.
extern const unsigned glb_anQuantRotate[64] =
{
	0, 8,16,24,32,40,48,56,
	1, 9,17,25,33,41,49,57,
	2,10,18,26,34,42,50,58,
	3,11,19,27,35,43,51,59,
	4,12,20,28,36,44,52,60,
	5,13,21,29,37,45,53,61,
	6,14,22,30,38,46,54,62,
	7,15,23,31,39,47,55,63,
};

// The ITU-T standard provides some sample quantization
// tables (for luminance and chrominance) that are often
// the basis for many different quantization tables through
// a scaling function.
extern const unsigned glb_anStdQuantLum[64] =
{
	16, 11, 10, 16, 24, 40, 51, 61,
	12, 12, 14, 19, 26, 58, 60, 55,
	14, 13, 16, 24, 40, 57, 69, 56,
	14, 17, 22, 29, 51, 87, 80, 62,
	18, 22, 37, 56, 68,109,103, 77,
	24, 35, 55, 64, 81,104,113, 92,
	49, 64, 78, 87,103,121,120,101,
	72, 92, 95, 98,112,100,103, 99
};

extern const unsigned glb_anStdQuantChr[64] =
{
	17, 18, 24, 47, 99, 99, 99, 99,
	18, 21, 26, 66, 99, 99, 99, 99,
	24, 26, 56, 99, 99, 99, 99, 99,
	47, 66, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99
};



