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

// JPEGsnoopViewImg.cpp : implementation file
//

#include "stdafx.h"
#include "JPEGsnoop.h"
#include "JPEGsnoopDoc.h"
#include "JPEGsnoopViewImg.h"
#include ".\jpegsnoopviewimg.h"


// CJPEGsnoopViewImg

IMPLEMENT_DYNCREATE(CJPEGsnoopViewImg, CScrollView)

CJPEGsnoopViewImg::CJPEGsnoopViewImg()
{
	//_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); //CAL!

	CWindowDC dc(NULL);

	// Create main log font
	memset(&m_logfont, 0, sizeof(m_logfont));
	m_nPointSize = 120;
	//_tcscpy(m_logfont.lfFaceName,_T("Courier New"));
	_tcscpy(m_logfont.lfFaceName,_T("Arial"));

	m_logfont.lfHeight = ::MulDiv(m_nPointSize,
		dc.GetDeviceCaps(LOGPIXELSY),720);
	m_logfont.lfPitchAndFamily = FIXED_PITCH;

	m_pFont = new CFont;
	m_pFont->CreateFontIndirect(&m_logfont);

}

CJPEGsnoopViewImg::~CJPEGsnoopViewImg()
{
	if (m_pFont != NULL)
	{
		delete m_pFont;
	}
}


BEGIN_MESSAGE_MAP(CJPEGsnoopViewImg, CScrollView)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
ON_WM_MOUSEWHEEL()
ON_WM_MOUSEMOVE()
ON_WM_ERASEBKGND()
END_MESSAGE_MAP()


// CJPEGsnoopViewImg drawing

void CJPEGsnoopViewImg::OnInitialUpdate()
{
	CScrollView::OnInitialUpdate();

	CJPEGsnoopDoc* pDoc = (CJPEGsnoopDoc*)GetDocument();
	ASSERT_VALID(pDoc);


	CSize		sizeTotal(0,0);
	sizeTotal.cx = 500; //CAL! FIXME
	sizeTotal.cy = 200; //CAL! FIXME
	SetScrollSizes(MM_TEXT, sizeTotal);

}


void CJPEGsnoopViewImg::OnDraw(CDC* pDC)
{
	CJPEGsnoopDoc* pDoc = (CJPEGsnoopDoc*)GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: add draw code for native data here
	CimgDecode*		pImgDec		= pDoc->m_pImgDec;

	CPoint	ScrolledPos = GetScrollPosition();
	CRect	rectClient;
	CSize	szNewScrollSize;
    
	GetClientRect(&rectClient);

	pImgDec->ViewOnDraw(pDC,rectClient,ScrolledPos,m_pFont,szNewScrollSize);
	SetScrollSizes(MM_TEXT, szNewScrollSize);

}

// CJPEGsnoopViewImg diagnostics

#ifdef _DEBUG
void CJPEGsnoopViewImg::AssertValid() const
{
	CScrollView::AssertValid();
}

void CJPEGsnoopViewImg::Dump(CDumpContext& dc) const
{
	CScrollView::Dump(dc);
}
#endif //_DEBUG


// CJPEGsnoopViewImg message handlers


CimgDecode* CJPEGsnoopViewImg::GetImgDec()
{
	CJPEGsnoopDoc* pDoc = (CJPEGsnoopDoc*)GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return NULL;

	// TODO: add draw code for native data here
	CimgDecode*		pImgDec		= pDoc->m_pImgDec;
	return pImgDec;
}


// FIXME: Migrate into ImgDec!
bool CJPEGsnoopViewImg::InPreviewArea(CPoint point,CPoint &ptPix)
{
	int			pix_x;
	int			pix_y;
	float		fZoom;
	unsigned	nImgPosX;
	unsigned	nImgPosY;
	unsigned	nPixX;		// 8x8 block number (not MCU unless CSS=1x1)
	unsigned	nPixY;

	fZoom = GetImgDec()->m_nZoom;
	ASSERT(fZoom != 0);
	nImgPosX = GetImgDec()->m_nPreviewPosX;
	nImgPosY = GetImgDec()->m_nPreviewPosY;

	CPoint	ScrolledPos = GetScrollPosition();
	CString tmpStr;

	pix_x = point.x;
	pix_y = point.y;
	pix_x -= (nImgPosX - ScrolledPos.x);
	pix_y -= (nImgPosY - ScrolledPos.y);

	ptPix.x = 0;
	ptPix.y = 0;

	// Is the preview even displayed?
	//if (!GetImgDec()->my_DIB_ready) {
	//	return false;
	//}
	// FIXME

	// Note that m_nPreviewSize already includes effects of zoom level
	if ( ((pix_x >= 0) && ((unsigned)pix_x < GetImgDec()->m_nPreviewSizeX)) &&
		 ((pix_y >= 0) && ((unsigned)pix_y < GetImgDec()->m_nPreviewSizeY)) )
	{
		// Undo zoom
		nPixX = (unsigned)(pix_x / fZoom);
		nPixY = (unsigned)(pix_y / fZoom);
	
		ptPix.x = nPixX;
		ptPix.y = nPixY;

		return true;
	}
	return false;
}

// The following call is one I created to automatically recenter the image
// *** Not currently used as I cannot figure out how to have this called
//     on redraw.
void CJPEGsnoopViewImg::SetScrollCenter(float fZoomOld, float fZoomNew)
{
	unsigned	nImgPosX;
	unsigned	nImgPosY;

	ASSERT(fZoomOld != 0);
	ASSERT(fZoomNew != 0);
	nImgPosX = GetImgDec()->m_nPreviewPosX;
	nImgPosY = GetImgDec()->m_nPreviewPosY;

	CPoint	ScrolledPos = GetScrollPosition();
	ScrolledPos.Offset(-nImgPosX,-nImgPosY);

	ScrolledPos.x = (long)(ScrolledPos.x * fZoomNew / fZoomOld);
	ScrolledPos.y = (long)(ScrolledPos.y * fZoomNew / fZoomOld);

	ScrolledPos.Offset(+nImgPosX,+nImgPosY);

	ScrollToPosition(ScrolledPos);
}


int CJPEGsnoopViewImg::MeasureFontHeight(CFont* pFont, CDC* pDC)
{
	// how tall is the identified font in the identified DC?

	CFont* pOldFont;
	pOldFont = pDC->SelectObject(pFont);

	CRect rectDummy;
	CString strRender = _T("1234567890ABCDEF- abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ[]");
	int nHeight = pDC->DrawText(strRender, -1, rectDummy,
		DT_TOP | DT_SINGLELINE | DT_CALCRECT);
	pDC->SelectObject(pOldFont);

	return nHeight;
}



void CJPEGsnoopViewImg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	//AfxMessageBox("LButtonDown!");

	CScrollView::OnLButtonDown(nFlags, point);
}

void CJPEGsnoopViewImg::OnLButtonUp(UINT nFlags, CPoint point)
{
	CString	tmpStr;
	//unsigned nByte;
	//unsigned nBit;
	CPoint ptPix;
	CPoint ptMcu;
	CPoint ptBlk;

	if (InPreviewArea(point,ptPix)) {
		
		// Set the marker
		ptBlk = GetImgDec()->PixelToBlk(ptPix);
		GetImgDec()->SetMarkerBlk(ptBlk.x,ptBlk.y);

		//Invalidate();

	} else {
		GetImgDec()->SetStatusText("");
	}
	
	
	CScrollView::OnLButtonUp(nFlags, point);
}

void CJPEGsnoopViewImg::OnRButtonUp(UINT nFlags, CPoint point)
{

	CScrollView::OnLButtonUp(nFlags, point);
}


BOOL CJPEGsnoopViewImg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: Add your message handler code here and/or call default
	CPoint	ScrolledPos = GetScrollPosition();
	ScrolledPos.Offset(0,-zDelta);
	ScrollToPosition(ScrolledPos);

	// Want to update the status text
	OnMouseMove(0,pt);

	return CScrollView::OnMouseWheel(nFlags, zDelta, pt);
}

void CJPEGsnoopViewImg::OnMouseMove(UINT nFlags, CPoint point)
{
	CString	tmpStr;
	unsigned nByte;
	unsigned nBit;
	CPoint ptPix;
	CPoint ptMcu;
	CPoint ptBlk;
	int		nY1,nCb1,nCr1;

	// FIXME: Migrate the following into ImgDec!
	if (InPreviewArea(point,ptPix)) {

		ptMcu = GetImgDec()->PixelToMcu(ptPix);
		GetImgDec()->LookupFilePosMcu(ptMcu.x,ptMcu.y,nByte,nBit);

		ptBlk = GetImgDec()->PixelToBlk(ptPix);
		GetImgDec()->LookupBlkYCC(ptBlk.x,ptBlk.y,nY1,nCb1,nCr1);

/*
		tmpStr.Format("MCU [%04u,%04u] File: 0x%08X:%u\tYCC=[%05d,%05d,%05d]",
			ptMcu.x,ptMcu.y,nByte,nBit,
			nY1,nCb1,nCr1);
		GetImgDec()->SetStatusText(tmpStr);
*/
		tmpStr.Format("MCU [%04u,%04u]",ptMcu.x,ptMcu.y);
		GetImgDec()->SetStatusMcuText(tmpStr);

		tmpStr.Format("File: 0x%08X:%u",nByte,nBit);
		GetImgDec()->SetStatusFilePosText(tmpStr);

		tmpStr.Format("YCC DC=[%05d,%05d,%05d]",nY1,nCb1,nCr1);
		GetImgDec()->SetStatusYccText(tmpStr);

	} else {
//		GetImgDec()->SetStatusText("");
		GetImgDec()->SetStatusMcuText("");
		GetImgDec()->SetStatusFilePosText("");
		GetImgDec()->SetStatusYccText("");
	}

	CScrollView::OnMouseMove(nFlags, point);
}

BOOL CJPEGsnoopViewImg::OnEraseBkgnd(CDC* pDC)
{
	// TODO: Add your message handler code here and/or call default

	// Apparently this helps remove flicker!
	// Disabled for now
	//CAL! return 1;

	return CScrollView::OnEraseBkgnd(pDC);
}
