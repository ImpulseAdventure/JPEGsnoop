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

// JPEGsnoopViewImg.cpp : implementation file
//

#include "stdafx.h"
#include "JPEGsnoop.h"
#include "JPEGsnoopDoc.h"
#include "JPEGsnoopViewImg.h"


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
	_tcscpy_s(m_logfont.lfFaceName,_T("Arial"));

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


CJPEGsnoopCore* CJPEGsnoopViewImg::GetCore()
{
	CJPEGsnoopDoc* pDoc = (CJPEGsnoopDoc*)GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return NULL;

	// TODO: add draw code for native data here
	CJPEGsnoopCore*		pCore		= pDoc->m_pCore;
	return pCore;
}


// CJPEGsnoopViewImg drawing

void CJPEGsnoopViewImg::OnInitialUpdate()
{
	CScrollView::OnInitialUpdate();

	CJPEGsnoopDoc* pDoc = (CJPEGsnoopDoc*)GetDocument();
	ASSERT_VALID(pDoc);


	CSize		sizeTotal(0,0);
	sizeTotal.cx = 500; // FIXME:
	sizeTotal.cy = 200; // FIXME:
	SetScrollSizes(MM_TEXT, sizeTotal);

}


void CJPEGsnoopViewImg::OnDraw(CDC* pDC)
{
	CJPEGsnoopDoc* pDoc = (CJPEGsnoopDoc*)GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: add draw code for native data here
	CJPEGsnoopCore*	pCore = GetCore();
	if (!pCore)
		return;

	CPoint	ScrolledPos = GetScrollPosition();
	CRect	rectClient;
	CSize	szNewScrollSize;
    
	GetClientRect(&rectClient);

	pCore->I_ViewOnDraw(pDC,rectClient,ScrolledPos,m_pFont,szNewScrollSize);
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

	CJPEGsnoopCore*	pCore = GetCore();

	fZoom = pCore->I_GetPreviewZoom();
	pCore->I_GetPreviewPos(nImgPosX,nImgPosY);

	ASSERT(fZoom != 0);

	CPoint	ScrolledPos = GetScrollPosition();
	CString strTmp;

	pix_x = point.x;
	pix_y = point.y;
	pix_x -= (nImgPosX - ScrolledPos.x);
	pix_y -= (nImgPosY - ScrolledPos.y);

	ptPix.x = 0;
	ptPix.y = 0;

	// Is the preview even displayed?
	//if (!GetImgDec()->m_bDibTempReady) {
	//	return false;
	//}
	// FIXME

	// Note that m_nPreviewSize already includes effects of zoom level
	unsigned	nPreviewSzX,nPreviewSzY;
	pCore->I_GetPreviewSize(nPreviewSzX,nPreviewSzY);
	if ( ((pix_x >= 0) && ((unsigned)pix_x < nPreviewSzX)) &&
		 ((pix_y >= 0) && ((unsigned)pix_y < nPreviewSzY)) )
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

// The following routine was intended to automatically recenter the image
// TODO: However, what is the best way to have it called during redraw?
// Currently not used
void CJPEGsnoopViewImg::SetScrollCenter(float fZoomOld, float fZoomNew)
{
	unsigned	nImgPosX;
	unsigned	nImgPosY;

	ASSERT(fZoomOld != 0);
	ASSERT(fZoomNew != 0);
	GetCore()->I_GetPreviewPos(nImgPosX,nImgPosY);

	CPoint	ScrolledPos = GetScrollPosition();
	ScrolledPos.Offset(-(int)nImgPosX,-(int)nImgPosY);

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

	//AfxMessageBox(_T("LButtonDown!"));

	CScrollView::OnLButtonDown(nFlags, point);
}

void CJPEGsnoopViewImg::OnLButtonUp(UINT nFlags, CPoint point)
{
	CString	strTmp;
	CPoint ptPix;
	CPoint ptMcu;
	CPoint ptBlk;

	// Need to ensure that the Preview image was based on a JPEG with
	// MCU/Block map info
	//if (InPreviewArea(point,ptPix)) {
	if ( (GetCore()->I_IsPreviewReady() && InPreviewArea(point,ptPix)) ) {
		
		// Set the marker
		ptBlk = GetCore()->I_PixelToBlk(ptPix);
		GetCore()->I_SetMarkerBlk(ptBlk.x,ptBlk.y);

		//Invalidate();

	} else {
		GetCore()->I_SetStatusText(_T(""));
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
	CString	strTmp;
	unsigned nByte;
	unsigned nBit;
	CPoint ptPix;
	CPoint ptMcu;
	CPoint ptBlk;
	int		nY1,nCb1,nCr1;

	// FIXME: Migrate the following into ImgDec!
	// Need to ensure that the Preview image was based on a JPEG with
	// MCU/Block map info
	if ( (GetCore()->I_IsPreviewReady() && InPreviewArea(point,ptPix)) ) {

		ptMcu = GetCore()->I_PixelToMcu(ptPix);
		GetCore()->I_LookupFilePosMcu(ptMcu.x,ptMcu.y,nByte,nBit);

		ptBlk = GetCore()->I_PixelToBlk(ptPix);
		GetCore()->I_LookupBlkYCC(ptBlk.x,ptBlk.y,nY1,nCb1,nCr1);

/*
		strTmp.Format(_T("MCU [%04u,%04u] File: 0x%08X:%u\tYCC=[%05d,%05d,%05d]"),
			ptMcu.x,ptMcu.y,nByte,nBit,
			nY1,nCb1,nCr1);
		GetCore()->I_SetStatusText(strTmp);
*/
		strTmp.Format(_T("MCU [%04u,%04u]"),ptMcu.x,ptMcu.y);
		GetCore()->I_SetStatusMcuText(strTmp);

		strTmp.Format(_T("File: 0x%08X:%u"),nByte,nBit);
		GetCore()->I_SetStatusFilePosText(strTmp);

		strTmp.Format(_T("YCC DC=[%05d,%05d,%05d]"),nY1,nCb1,nCr1);
		GetCore()->I_SetStatusYccText(strTmp);

	} else {
//		GetCore()->I_SetStatusText(_T(""));
		GetCore()->I_SetStatusMcuText(_T(""));
		GetCore()->I_SetStatusFilePosText(_T(""));
		GetCore()->I_SetStatusYccText(_T(""));
	}

	CScrollView::OnMouseMove(nFlags, point);
}

BOOL CJPEGsnoopViewImg::OnEraseBkgnd(CDC* pDC)
{
	// TODO: Add your message handler code here and/or call default

	// FIXME:
	// Apparently this could help remove flicker! But it is disabled for now.
	//return 1;

	return CScrollView::OnEraseBkgnd(pDC);
}
