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

// ==========================================================================
// CLASS DESCRIPTION:
// - JPEGsnoop SDI View class for the main text log window
//
// ==========================================================================


// JPEGsnoopView.h : interface of the CJPEGsnoopView class
//


#pragma once

#include "JPEGsnoopDoc.h"

class CJPEGsnoopCntrItem;

class CJPEGsnoopView : public CRichEditView
{
protected: // create from serialization only
	CJPEGsnoopView();
	DECLARE_DYNCREATE(CJPEGsnoopView)

// Attributes
public:
	CJPEGsnoopDoc*	GetDocument() const;

// Operations
public:

// Overrides
public:
	virtual BOOL	PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual void	OnInitialUpdate(); // called first time after construct
	virtual BOOL	OnPreparePrinting(CPrintInfo* pInfo);

// Implementation
public:
	virtual			~CJPEGsnoopView();
#ifdef _DEBUG
	virtual void	AssertValid() const;
	virtual void	Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	afx_msg void	OnDestroy();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void	OnDropFiles(HDROP hDropInfo);
};

#ifndef _DEBUG  // debug version in JPEGsnoopView.cpp
inline CJPEGsnoopDoc* CJPEGsnoopView::GetDocument() const
   { return reinterpret_cast<CJPEGsnoopDoc*>(m_pDocument); }
#endif

