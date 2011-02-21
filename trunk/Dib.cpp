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

#include "stdafx.h"
#include "dib.h"



CDIB::CDIB() : m_pDIB(NULL)
{
}

CDIB::~CDIB()
{
    if (m_pDIB) delete m_pDIB;
}

void CDIB::Kill()
{
	if (m_pDIB) {
		delete m_pDIB;
		m_pDIB = FALSE;
	}
}

bool CDIB::CreateDIB(DWORD dwWidth,DWORD dwHeight,int nBits)
{
    if (m_pDIB) return FALSE;
    const DWORD dwcBihSize = sizeof(BITMAPINFOHEADER);

    // Calculate the memory required for the DIB
	/* //CAL!
	//CAL! The following cannot be right!! Maybe for smaller
	// bit widths, but definitely not for 32-bit, as we'll
	// get 32-bit wrap!
    DWORD dwSize = dwcBihSize +
                    (2>>nBits) * sizeof(RGBQUAD) +
                    ((nBits * dwWidth) * dwHeight);
	*/

	//CAL! -- start
	// Note that we don't actually use the color table (so 1*QUAD is not actually used)
	// Extra +4 is just in case we're not aligned to word boundary
	DWORD dwSize = dwcBihSize +
                    1 * sizeof(RGBQUAD) +
                    ( (dwWidth * dwHeight) * sizeof(RGBQUAD) ) +
					4;
	//CAL! -- end
	

    m_pDIB = (LPBITMAPINFO)new BYTE[dwSize];
    if (!m_pDIB) return FALSE;


    m_pDIB->bmiHeader.biSize = dwcBihSize;
    m_pDIB->bmiHeader.biWidth = dwWidth;
    m_pDIB->bmiHeader.biHeight = dwHeight;
    m_pDIB->bmiHeader.biBitCount = nBits;
    m_pDIB->bmiHeader.biPlanes = 1;
    m_pDIB->bmiHeader.biCompression = BI_RGB;
    m_pDIB->bmiHeader.biXPelsPerMeter = 1000;
    m_pDIB->bmiHeader.biYPelsPerMeter = 1000;
    m_pDIB->bmiHeader.biClrUsed = 0;
    m_pDIB->bmiHeader.biClrImportant = 0;

    InitializeColors();
    return TRUE;
}


void CDIB::InitializeColors()
{
    if (!m_pDIB) return;
    // This just initializes all colors to black

	//CAL! why does the following add ~ 1760 (0x6E0) instead of 40???
    //LPRGBQUAD lpColors =
    //    (LPRGBQUAD)(m_pDIB+m_pDIB->bmiHeader.biSize);

	//CAL!
	LPRGBQUAD lpColors = m_pDIB->bmiColors;

    for(int i=0;i<GetDIBCols();i++)
    {
        lpColors[i].rgbRed=0;
        lpColors[i].rgbBlue=0;
        lpColors[i].rgbGreen=0;
        lpColors[i].rgbReserved=0;
    }
}

int CDIB::GetDIBCols() const
{
    if (!m_pDIB) return 0;
    //CAL! return (2>>m_pDIB->bmiHeader.biBitCount);

	//CAL!
	// According to the MSDN, the biColors (palette) are not defined
	// for 16, 24 and 32 bit DIBs.
	//   http://msdn.microsoft.com/library/default.asp?url=/library/en-us/gdi/bitmaps_0zn6.asp
	if (m_pDIB->bmiHeader.biBitCount > 8) {
		return 0;
	} else {
		return (2>>m_pDIB->bmiHeader.biBitCount);
	}
}

void* CDIB::GetDIBBitArray() const
{
    if (!m_pDIB) return FALSE;
	/* //CAL!
	// For some reason I don't trust the pointer arithmetic
	// being done here. Doesn't seem to treat as adding bytes.
    return (m_pDIB + m_pDIB->bmiHeader.biSize +
        GetDIBCols() * sizeof(RGBQUAD));
	*/

	//CAL! Extended form to ensure correct addition
	unsigned char* ptr;
	ptr = (unsigned char*)m_pDIB;
	ptr += m_pDIB->bmiHeader.biSize;
	ptr += GetDIBCols() * sizeof(RGBQUAD);
	return (void*)ptr;

}

bool CDIB::CreateDIBFromBitmap(CDC* pDC)
{
    if (!pDC) return FALSE;
    HDC hDC = pDC->GetSafeHdc();

    BITMAP bimapInfo;
    m_bmBitmap.GetBitmap(&bimapInfo);
    if (!CreateDIB(bimapInfo.bmWidth,bimapInfo.bmHeight,
        bimapInfo.bmBitsPixel)) return FALSE;

	/* //CAL!
	//CAL! Don't trust this, so instead going to use actual pointer
    LPRGBQUAD lpColors =
        (LPRGBQUAD)(m_pDIB+m_pDIB->bmiHeader.biSize);
	*/
	//CAL! instead do the following:
	LPRGBQUAD lpColors = m_pDIB->bmiColors;


    SetDIBColorTable(hDC,0,GetDIBCols(),lpColors);

    // This implicitly assumes that the source bitmap
    // is at the 1 pixel per mm resolution
    bool bSuccess = (GetDIBits(hDC,(HBITMAP)m_bmBitmap,
        0,bimapInfo.bmHeight,GetDIBBitArray(),
        m_pDIB,DIB_RGB_COLORS) > 0);
    return bSuccess;
}

bool CDIB::CopyDIB(CDC* pDestDC,int x,int y,float scale)
{
    if (!m_pDIB || !pDestDC) return FALSE;
//    int nOldMapMode = pDestDC->SetMapMode(MM_LOMETRIC);
    int nOldMapMode = pDestDC->SetMapMode(MM_TEXT);

	// NOTE: The following line was added to make the down-sampling
	// (zoom < 100%) look much better. Otherwise, it looked horrible!
	SetStretchBltMode(pDestDC->GetSafeHdc(),COLORONCOLOR);

    bool bOK = StretchDIBits(pDestDC->GetSafeHdc(),
        x,y,
        (unsigned)(m_pDIB->bmiHeader.biWidth * scale),    // Dest Width
        (unsigned)(m_pDIB->bmiHeader.biHeight * scale),  // Dest Height
        0,0,
        m_pDIB->bmiHeader.biWidth,         // Source Width
        m_pDIB->bmiHeader.biHeight,        // Source Height
        GetDIBBitArray(),m_pDIB,DIB_RGB_COLORS,SRCCOPY) > 0;

    pDestDC->SetMapMode(nOldMapMode);
    return bOK;
}

bool CDIB::CopyDibDblBuf(CDC* pDestDC, int x, int y,CRect* rectClient, float scale)
{
HDC          hdcMem;
HBITMAP      hbmMem;
HANDLE       hOld;

//HDC          hdc;

int		win_width = rectClient->Width();
int		win_height = rectClient->Height();


    // Create an off-screen DC for double-buffering
    hdcMem = CreateCompatibleDC(pDestDC->GetSafeHdc());
    hbmMem = CreateCompatibleBitmap(pDestDC->GetSafeHdc(), win_width, win_height);
    hOld   = SelectObject(hdcMem, hbmMem);

    // Draw into hdcMem

    //int nOldMapMode = hdcMem->SetMapMode(MM_TEXT);

	// NOTE: The following line was added to make the down-sampling
	// (zoom < 100%) look much better. Otherwise, it looked horrible!
	//SetStretchBltMode(hdcMem->GetSafeHdc(),COLORONCOLOR);

    bool bOK = StretchDIBits(hdcMem, //hdcMem->GetSafeHdc(),
        x,y,
        (unsigned)(m_pDIB->bmiHeader.biWidth * scale),    // Dest Width
        (unsigned)(m_pDIB->bmiHeader.biHeight * scale),  // Dest Height
        0,0,
        m_pDIB->bmiHeader.biWidth,         // Source Width
        m_pDIB->bmiHeader.biHeight,        // Source Height
        GetDIBBitArray(),m_pDIB,DIB_RGB_COLORS,SRCCOPY) > 0;

    //hdcMem->SetMapMode(nOldMapMode);

    // Transfer the off-screen DC to the screen
    BitBlt(pDestDC->GetSafeHdc(), 0, 0, win_width, win_height, hdcMem, 0, 0, SRCCOPY);

    // Free-up the off-screen DC
    SelectObject(hdcMem, hOld);
    DeleteObject(hbmMem);
    DeleteDC    (hdcMem);

	return bOK;

}


bool CDIB::CopyDibPart(CDC* pDestDC,CRect rectImg,CRect* rectClient, float scale)
{
    if (!m_pDIB || !pDestDC) return FALSE;

	int		nDstW;
	int		nDstH;
	int		nDstX;
	int		nDstY;
	int		nSrcX;
	int		nSrcY;
	int		nSrcW;
	int		nSrcH;
	CRect	rectSrc;
	CRect	rectInter;

	// Determine boundaries of region to copy
//	rectSrc = CRect(nDstX,nDstY,
//		nDstX + (m_pDIB->bmiHeader.biWidth * scale),
//		nDstY + (m_pDIB->bmiHeader.biHeight * scale) );

	rectInter.IntersectRect(rectClient,rectImg);

	// Now determine what original source rect this corresponds to
	//...
	rectSrc = rectInter;
	rectSrc.OffsetRect(-rectImg.left,-rectImg.top);
	nSrcX = rectSrc.left;
	nSrcY = rectSrc.top;
	nSrcW = rectInter.Width();
	nSrcH = rectInter.Height();
	nDstX = rectInter.left;// - rectClient->left;
	nDstY = rectInter.top;// - rectClient->top;
	nDstW = rectInter.Width();
	nDstH = rectInter.Height();

    int nOldMapMode = pDestDC->SetMapMode(MM_TEXT);
/*
	bool bOK = StretchDIBits(pDestDC->GetSafeHdc(),
        rectSrc.left,rectSrc.top,	// Dest X,Y
        rectSrc.Width(),    // Dest Width
        rectSrc.Height(),  // Dest Height
        0,0,	// Source X,Y
        m_pDIB->bmiHeader.biWidth,         // Source Width
        m_pDIB->bmiHeader.biHeight,        // Source Height
        GetDIBBitArray(),m_pDIB,DIB_RGB_COLORS,SRCCOPY) > 0;
*/
	/*
   bool bOK = StretchDIBits(pDestDC->GetSafeHdc(),
        nDstX,nDstY,	// Dest X,Y
        nDstWidth,    // Dest Width
        nDstHeight,  // Dest Height
        nSrcX,nSrcY,	// Source X,Y
        nSrcWidth,         // Source Width
        nSrcHeight,        // Source Height
        GetDIBBitArray(),m_pDIB,DIB_RGB_COLORS,SRCCOPY) > 0;
		*/

	// StretchDIBits() is *really* messed up!
	// See workaround at:
	//   http://wiki.allegro.cc/StretchDIBits
	//


	nSrcX = rectInter.left; //0;
	nSrcY = rectInter.top; //0;
	nSrcW = rectClient->Width();
	nSrcH = rectClient->Height();

	nDstX = rectInter.left; //rectClient->left;
	nDstY = rectInter.top; //rectClient->top;
	nDstW = rectClient->Width();
	nDstH = rectClient->Height();

	int	nBmpH;
	int	nStDstX;
	int	nStDstY;
	int nStDstW;
	int nStDstH;
	int	nStSrcX;
	int	nStSrcY;
	int	nStSrcW;
	int	nStSrcH;

	nBmpH = rectImg.Height();

	nStDstX = nDstX;
	nStDstY = nDstH + nDstY - 1;
	nStDstW = nDstW;
	nStDstH = -nDstH;

	nStSrcX = nSrcX;
	nStSrcY = nBmpH - nSrcY + 1;
	nStSrcW = nSrcW;
	nStSrcH = -nSrcH;


   bool bOK = StretchDIBits(pDestDC->GetSafeHdc(),
        nStDstX,nStDstY,	// Dest X,Y
        nStDstW,    // Dest Width
        nStDstH,  // Dest Height
        nStSrcX,nStSrcY,	// Source X,Y
        nStSrcW,         // Source Width
        nStSrcH,        // Source Height
        GetDIBBitArray(),m_pDIB,DIB_RGB_COLORS,SRCCOPY) > 0;

    pDestDC->SetMapMode(nOldMapMode);
    return bOK;
}



// Attempt to create a double-buffer so that we can do our
// own resize function as the StretchDIBits() seems horrible
// for downsampling!
// *** Following is currently unused as it leads to strange
//     redraw issues, probably due to the client rect boundaries!
bool CDIB::CopyDIBsmall(CDC* pDestDC,int x,int y,float scale)
{
    if (!m_pDIB || !pDestDC) return FALSE;

	// --------------
    CDC dcm;    
    CRect rc;
	pDestDC->GetWindow()->GetClientRect(rc);
    //GetClientRect(rc);
    dcm.CreateCompatibleDC(pDestDC);
    CBitmap bmt;
    bmt.CreateCompatibleBitmap(pDestDC,rc.Width(),rc.Height());
    CBitmap *pBitmapOld = dcm.SelectObject(&bmt);
    
    dcm.Rectangle(rc);// make the work of the OnEraseBkgnd function
        
	// DRAWING start
    int nOldMapMode = dcm.SetMapMode(MM_TEXT);
    bool bOK = StretchDIBits(dcm.GetSafeHdc(),
        x,y,
        (unsigned)(m_pDIB->bmiHeader.biWidth * scale),    // Dest Width
        (unsigned)(m_pDIB->bmiHeader.biHeight * scale),  // Dest Height
        0,0,
        m_pDIB->bmiHeader.biWidth,         // Source Width
        m_pDIB->bmiHeader.biHeight,        // Source Height
        GetDIBBitArray(),m_pDIB,DIB_RGB_COLORS,SRCCOPY) > 0;

    dcm.SetMapMode(nOldMapMode);
	// DRAWING end

	// Copy over
    BitBlt(pDestDC->m_hDC,0,0,rc.Width(),rc.Height(),dcm.m_hDC,0,0,SRCCOPY);

    dcm.SelectObject(pBitmapOld);
    // drop other objects from the dc context memory - if there are.



/*

	// -------------------

	// Create dummy DC
	CDC* pTempDC = new CDC();
	ASSERT(pTempDC);
	pTempDC->CreateCompatibleDC(pDestDC);

    int nOldMapMode = pTempDC->SetMapMode(MM_TEXT);
    bool bOK = StretchDIBits(pTempDC->GetSafeHdc(),
        x,y,
        (unsigned)(m_pDIB->bmiHeader.biWidth * scale),    // Dest Width
        (unsigned)(m_pDIB->bmiHeader.biHeight * scale),  // Dest Height
        0,0,
        m_pDIB->bmiHeader.biWidth,         // Source Width
        m_pDIB->bmiHeader.biHeight,        // Source Height
        GetDIBBitArray(),m_pDIB,DIB_RGB_COLORS,SRCCOPY) > 0;

    pTempDC->SetMapMode(nOldMapMode);

	// Now copy over
    nOldMapMode = pDestDC->SetMapMode(MM_TEXT);
	pDestDC->BitBlt(0, 0, 1200,600, pTempDC, 0, 0, SRCCOPY);
    pDestDC->SetMapMode(nOldMapMode);

*/

    return bOK;
}


/*
EXAMPLE:
#include "dib.h"
CDIB g_DIB;

void CBitmapdemoView::OnInitialUpdate()
{
    CView::OnInitialUpdate();
    CClientDC dc(this);
    g_DIB.m_bmBitmap.LoadBitmap(IDB_TSTBITMAP);
    g_DIB.CreateDIBFromBitmap(&dc);
}

void CBitmapdemoView::OnPrint(CDC* pDC, CPrintInfo* pInfo)
 {
   g_DIB.CopyDIB(pDC,50,-50);
 }

void CBitmapdemoView::OnDraw(CDC* pDC)
 {
   g_DIB.CopyDIB(pDC,50,-50);
 }

*/
