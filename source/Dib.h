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

#ifndef DIB_H
#define DIB_H

#include <QBitmap>
#include <QObject>
#include <QRect>

// TODO: Note that this file (Dib) will likely be removed soon
#include <windows.h>

#define BI_RGB 0

// TODO: Confirm appropriate mapping (LONG & DWORD)
typedef long LONG;
typedef unsigned char BYTE;
typedef unsigned long DWORD;
typedef unsigned short WORD;

// TODO: Remove the following as already defined in windows.h
#if 0

typedef struct tagBITMAPFILEHEADER
{
    WORD    bfType; // 2  /* Magic identifier */
    DWORD   bfSize; // 4  /* File size in bytes */
    WORD    bfReserved1; // 2
    WORD    bfReserved2; // 2
    DWORD   bfOffBits; // 4 /* Offset to image data, bytes */
} __attribute__((packed)) BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER
{
    DWORD    biSize; // 4 /* Header size in bytes */
    LONG     biWidth; // 4 /* Width of image */
    LONG     biHeight; // 4 /* Height of image */
    WORD     biPlanes; // 2 /* Number of colour planes */
    WORD     biBitCount; // 2 /* Bits per pixel */
    DWORD    biCompression; // 4 /* Compression type */
    DWORD    biSizeImage; // 4 /* Image size in bytes */
    LONG     biXPelsPerMeter; // 4
    LONG     biYPelsPerMeter; // 4 /* Pixels per meter */
    DWORD    biClrUsed; // 4 /* Number of colours */
    DWORD    biClrImportant; // 4 /* Important colours */
} BITMAPINFOHEADER;

typedef struct tagRGBQUAD
{
    unsigned char    rgbBlue;
    unsigned char    rgbGreen;
    unsigned char    rgbRed;
    unsigned char    rgbReserved;
} RGBQUAD, *LPRGBQUAD;

typedef struct tagBITMAPINFO
{
  BITMAPINFOHEADER bmiHeader;
  RGBQUAD          bmiColors[1];
} BITMAPINFO, *LPBITMAPINFO;

#endif //

typedef struct
{
        BYTE    b;
        BYTE    g;
        BYTE    r;
} RGB_data; // RGB TYPE, plz also make sure the order

class CDIB : public QObject
{
public:
    CDIB();
    virtual ~CDIB();

    void			Kill();
    bool			CreateDIB(quint32 dwWidth,quint32 dwHeight,unsigned short nBits);
//  bool			CreateDIBFromBitmap(CDC* pDC);
    void			InitializeColors();
    int				GetDIBCols() const;
    void*			GetDIBBitArray() const;
//  bool			CopyDIB(CDC* pDestDC,int x,int y,float scale=1);
//  bool			CopyDibDblBuf(CDC* pDestDC, int x, int y,QRect* rectClient, float scale);
//  bool			CopyDIBsmall(CDC* pDestDC,int x,int y,float scale=1);
//  bool			CopyDibPart(CDC* pDestDC,QRect rectImg,QRect* rectClient, float scale);

  QBitmap			m_bmBitmap;

private:
  CDIB &operator = (const CDIB&);
  CDIB(CDIB&);

  LPBITMAPINFO	m_pDIB;
};

#endif
