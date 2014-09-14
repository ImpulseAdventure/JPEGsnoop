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

// ====================================================================================================
// SOURCE CODE ACKNOWLEDGEMENT
// ====================================================================================================
// The following code was loosely based on an example CDIB class that appears in the following book:
//
//		Title:		Visual C++ 6 Unleashed
//		Authors:	Mickey Williams and David Bennett
//		Publisher:	Sams (July 24, 2000)
//		ISBN-10:	0672312417
//		ISBN-13:	978-0672312410
// ====================================================================================================

#pragma once

class CDIB : public CObject
{
public:
    CDIB();
    virtual ~CDIB();

	void			Kill();
    bool			CreateDIB(DWORD dwWidth,DWORD dwHeight,int nBits);
    bool			CreateDIBFromBitmap(CDC* pDC);
    void			InitializeColors();
    int				GetDIBCols() const;
    void*			GetDIBBitArray() const;
    bool			CopyDIB(CDC* pDestDC,int x,int y,float scale=1);
	bool			CopyDibDblBuf(CDC* pDestDC, int x, int y,CRect* rectClient, float scale);
    bool			CopyDIBsmall(CDC* pDestDC,int x,int y,float scale=1);
	bool			CopyDibPart(CDC* pDestDC,CRect rectImg,CRect* rectClient, float scale);

public:
    CBitmap			m_bmBitmap;

private:
    LPBITMAPINFO	m_pDIB;
};
