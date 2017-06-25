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
// - TIFF file export routines
//
// ==========================================================================


#pragma once

#include "snoop.h"

#define TIFF_TYPE_BYTE			0x00
#define TIFF_TYPE_ASCII			0x01
#define TIFF_TYPE_SHORT			0x03
#define TIFF_TYPE_LONG			0x04
#define TIFF_TYPE_RATIONAL		0x05


#define TIFF_TAG_IMG_WIDTH			0x0100
#define TIFF_TAG_IMG_HEIGHT			0x0101
#define TIFF_TAG_BITS_PER_SAMP		0x0102
#define TIFF_TAG_COMPRESSION		0x0103
#define TIFF_TAG_PHOTOM_INTERP		0x0106
#define TIFF_TAG_STRIP_OFFSETS		0x0111
#define TIFF_TAG_ORIENTATION		0x0112
#define TIFF_TAG_SAMP_PER_PIX		0x0115
#define TIFF_TAG_ROWS_PER_STRIP		0x0116
#define TIFF_TAG_STRIP_BYTE_COUNTS	0x0117
#define TIFF_TAG_X_RESOLUTION		0x011A
#define TIFF_TAG_Y_RESOLUTION		0x011B
#define TIFF_TAG_PLANAR_CONFIG		0x011C
#define TIFF_TAG_RESOLUTION_UNIT	0x0128

// For YCC Images
#define TIFF_TAG_YCC_COEFFS			0x0211
#define TIFF_TAG_YCC_SUBSAMP		0x0212
#define TIFF_TAG_POSITIONING		0x0213
#define TIFF_TAG_REF_BLACK_WHITE	0x0214



class FileTiff
{

public:
	FileTiff();
	~FileTiff();

	void		WriteFile(CString sFnameOut,bool bModeYcc,bool bMode16b,void* pBitmap,unsigned nSizeX,unsigned nSizeY);
	void		WriteIfd(unsigned nSizeX,unsigned nSizeY,bool bModeYcc,bool bMode16b);
	void		WriteIfdEntrySingle(unsigned short nTag,unsigned short nType,unsigned nValOffset);
	void		WriteIfdEntryMult(unsigned short nTag,unsigned short nType,unsigned nNumVals,unsigned* nVals);

	unsigned	GetTypeLen(unsigned nType);

	void		WriteVal8(BYTE nVal);
	void		WriteVal16(unsigned short nVal);
	void		WriteVal32(unsigned int nVal);
	void		WriteIfdExtraBuf8(BYTE nVal);
	void		WriteIfdExtraBuf16(unsigned short nVal);
	void		WriteIfdExtraBuf32(unsigned int nVal);

private:
	CFile*		m_pFileOutput;

	unsigned	m_nPtrIfdExtra;
	unsigned	m_nPtrImg;
	unsigned	m_nPos;

	bool			m_bPreCalc;
	unsigned short	m_nNumIfd;
	BYTE*			m_pIfdExtraBuf;
	unsigned		m_nIfdExtraLen;

};
