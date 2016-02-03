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

// ==========================================================================
// CLASS DESCRIPTION:
// - JPEGsnoop main frame window
// - Splitter window
// - Status bar
//
// ==========================================================================


// MainFrm.h : interface of the CMainFrame class
//


#pragma once

#include "JPEGsnoopView.h"
#include "JPEGsnoopViewImg.h"


class CMainFrame : public CFrameWnd
{
	
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

// Attributes
public:

// Operations
public:

// Overrides
public:
	virtual BOOL	PreCreateWindow(CREATESTRUCT& cs);

// Implementation
public:
	virtual			~CMainFrame();
#ifdef _DEBUG
	virtual void	AssertValid() const;
	virtual void	Dump(CDumpContext& dc) const;
#endif

private:
	int				FindMenuItem(CMenu* Menu, LPCTSTR MenuString);
protected:
	// Generated message map functions
	afx_msg int		OnCreate(LPCREATESTRUCT lpCreateStruct);
	virtual BOOL	OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	DECLARE_MESSAGE_MAP()
private:
	afx_msg void	OnSize(UINT nType, int cx, int cy);

private:
	// Splitter
	CSplitterWnd	m_mainSplitter;
	BOOL			m_bInitSplitter;

protected:  // control bar embedded members
	CStatusBar		m_wndStatusBar;
	CToolBar		m_wndToolBar;

};


