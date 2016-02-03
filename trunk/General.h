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

#ifndef _GENERAL_H_
#define _GENERAL_H_

// General Global Functions
CString			Dec2Bin(unsigned nVal,unsigned nLen,bool bSpace=true);
unsigned short	Swap16(unsigned short nVal);
CString			Uint2Chars(unsigned nVal);
CString			Uint2DotByte(unsigned nVal);
bool			TestBit(unsigned nVal,unsigned nBit);
//CString			ByteStr2Unicode(BYTE* pBuf, unsigned nBufLen);
bool			Str2Uint32(CString strVal,unsigned nBase,unsigned &nVal);



// General Global Constants
extern const unsigned glb_anZigZag[64];
extern const unsigned glb_anUnZigZag[64];

extern const unsigned glb_anQuantRotate[64];
extern const unsigned glb_anStdQuantLum[64];
extern const unsigned glb_anStdQuantChr[64];

#endif