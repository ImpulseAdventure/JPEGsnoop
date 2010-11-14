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

#include "StdAfx.h"

// General Global Functions


CString Dec2Bin(unsigned nVal,unsigned nLen,bool bSpace)
{
	unsigned	nBit;
	CString		strBin = "";
	for (int nInd=nLen-1;nInd>=0;nInd--)
	{
		nBit = ( nVal & (1 << nInd) ) >> nInd;
		strBin += (nBit==1)?"1":"0";
		if ( ((nInd % 8) == 0) && (nInd != 0) ) {
			if (bSpace) {
				strBin += " ";
			}
		}
	}
	return strBin;
}
