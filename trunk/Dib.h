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

class CDIB : public CObject
{
public:
    CDIB();
    virtual ~CDIB();

	void Kill();	//CAL! Added

    bool CreateDIB(DWORD dwWidth,DWORD dwHeight,int nBits);
    bool CreateDIBFromBitmap(CDC* pDC);
    void InitializeColors();
    int GetDIBCols() const;
    void* GetDIBBitArray() const;
    bool CopyDIB(CDC* pDestDC,int x,int y,float scale=1);
	bool CopyDibDblBuf(CDC* pDestDC, int x, int y,CRect* rectClient, float scale);
    bool CopyDIBsmall(CDC* pDestDC,int x,int y,float scale=1);
	bool	CopyDibPart(CDC* pDestDC,CRect rectImg,CRect* rectClient, float scale);

public:
    CBitmap             m_bmBitmap;

private:
    LPBITMAPINFO        m_pDIB;
};
