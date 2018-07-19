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

// JPEGsnoopView.cpp : implementation of the CJPEGsnoopView class
//

#include "stdafx.h"
#include "JPEGsnoop.h"

#include "JPEGsnoopDoc.h"
#include "CntrItem.h"
#include "JPEGsnoopView.h"
#include ".\jpegsnoopview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CJPEGsnoopView

IMPLEMENT_DYNCREATE(CJPEGsnoopView, CRichEditView)

BEGIN_MESSAGE_MAP(CJPEGsnoopView, CRichEditView)
	ON_WM_DESTROY()
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CRichEditView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CRichEditView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CRichEditView::OnFilePrintPreview)
	ON_WM_DROPFILES()
END_MESSAGE_MAP()

// CJPEGsnoopView construction/destruction

CJPEGsnoopView::CJPEGsnoopView()
{

}

CJPEGsnoopView::~CJPEGsnoopView()
{
}

BOOL CJPEGsnoopView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs


	return CRichEditView::PreCreateWindow(cs);

}

// This is called initially in the first time the view
// is drawn, and again after the contents have finished loading
// with OnOpenDocument()
void CJPEGsnoopView::OnInitialUpdate()
{
	CRichEditView::OnInitialUpdate();

	// FIXME This seems like the best place to add this call
	// as we know that the view will now be available
	// Want to allow drag & drop from Explorer into window
	DragAcceptFiles(true);

	// Set the printing margins (720 twips = 1/2 inch)
	SetMargins(CRect(720, 720, 720, 720));

	// Set the word wrap to off
	//   Other modes: WrapNone, WrapToWindow, WrapToTargetDevice
	m_nWordWrap = WrapNone;
	WrapChanged();

	GetRichEditCtrl().SetReadOnly(true);

	// The following is a nice idea, but it isn't sufficent as it
	// doesn't actually handle the URL click message
	//GetRichEditCtrl().SetAutoURLDetect(true);

	//const COLORREF CLOUDBLUE = RGB(128, 184, 223);
	//const COLORREF WHITE = RGB(255, 255, 255);
	const COLORREF BLACK = RGB(1, 1, 1);
	//const COLORREF DKGRAY = RGB(128, 128, 128);
	//const COLORREF PURPLE = RGB(255, 0, 255);

	// Set the default character formatting
	CHARFORMAT cf;
	COLORREF myColor = BLACK; //0x00ff00ff; // 00BBGGRR (purple)
	TCHAR szFaceName[LF_FACESIZE] = _T("Courier New");
	_tcscpy_s(cf.szFaceName, szFaceName);
	CDC* pDC = GetDC();

	// Modify the default character format
	cf.dwMask = CFM_COLOR | CFM_FACE | CFM_SIZE | CFM_BOLD;
	cf.dwEffects = 0;
	cf.crTextColor = myColor;
	cf.bCharSet = ANSI_CHARSET;
	cf.bPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
	cf.yHeight = 12*72*20/pDC->GetDeviceCaps(LOGPIXELSY);

	GetRichEditCtrl().SetDefaultCharFormat(cf);

	GetDocument()->SetupView((CRichEditView*)this);
}


// CJPEGsnoopView printing

BOOL CJPEGsnoopView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}


void CJPEGsnoopView::OnDestroy()
{
	// Deactivate the item on destruction; this is important
	// when a splitter view is being used
   COleClientItem* pActiveItem = GetDocument()->GetInPlaceActiveItem(this);
   if (pActiveItem != NULL && pActiveItem->GetActiveView() == this)
   {
      pActiveItem->Deactivate();
      ASSERT(GetDocument()->GetInPlaceActiveItem(this) == NULL);
   }
   CRichEditView::OnDestroy();
}



// CJPEGsnoopView diagnostics

#ifdef _DEBUG
void CJPEGsnoopView::AssertValid() const
{
	CRichEditView::AssertValid();
}

void CJPEGsnoopView::Dump(CDumpContext& dc) const
{
	CRichEditView::Dump(dc);
}

CJPEGsnoopDoc* CJPEGsnoopView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CJPEGsnoopDoc)));
	return (CJPEGsnoopDoc*)m_pDocument;
}
#endif //_DEBUG


// CJPEGsnoopView message handlers

void CJPEGsnoopView::OnDropFiles(HDROP hDropInfo)
{
	UINT  uNumFiles;
	TCHAR szNextFile [MAX_PATH];

    // Get the # of files being dropped.
    uNumFiles = DragQueryFile ( hDropInfo, 0xFFFFFFFF, NULL, 0 );

	/*
	for ( UINT uFile = 0; uFile < uNumFiles; uFile++ )
	{
        // Get the next filename from the HDROP info.
        if ( DragQueryFile ( hDropInfo, uFile, szNextFile, MAX_PATH ) > 0 )
        {
			// ***
			// Do whatever you want with the filename in szNextFile.
			// ***
        }
    }
	*/

	// FIXME: For now, only support a single file but later may
	// consider somehow allowing multiple files to be processed?
	if (uNumFiles > 0) {
		if ( DragQueryFile ( hDropInfo, 0, szNextFile, MAX_PATH ) > 0 ) {
			GetDocument()->OnOpenDocument(szNextFile);
		}
	}

    // Free up memory.
    DragFinish ( hDropInfo );
	

	// Don't want to call built-in functionality!
	//CRichEditView::OnDropFiles(hDropInfo);
}
