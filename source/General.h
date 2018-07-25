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

#ifndef _GENERAL_H_
#define _GENERAL_H_

#include <QString>

// General Global Functions
QString Dec2Bin(uint32_t nVal, uint32_t nLen, bool bSpace = true);
uint16_t Swap16(uint16_t nVal);
QString Uint2Chars(uint32_t nVal);
QString Uint2DotByte(uint32_t nVal);
bool TestBit(uint32_t nVal, uint32_t nBit);
//QString ByteStr2Unicode(quint8* pBuf, uint32_t nBufLen);
bool Str2Uint32(QString strVal, uint32_t nBase, uint32_t & nVal);
//bool Uni2AscBuf(quint8* pBuf,QString strIn,uint32_t nMaxBytes,uint32_t &nOffsetBytes);

// General Global Constants
extern const uint32_t glb_anZigZag[64];
extern const uint32_t glb_anUnZigZag[64];
extern const uint32_t glb_anQuantRotate[64];
extern const uint32_t glb_anStdQuantLum[64];
extern const uint32_t glb_anStdQuantChr[64];

#endif
