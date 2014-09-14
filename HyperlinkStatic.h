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
// The following code is derived from the following project on CodeGuru:
//
//		Title:		HyperlinkStatic
//		Author:		Franz Wong
//		URL:		http://www.codeguru.com/cpp/controls/staticctrl/article.php/c5801
//		Date:		Jan 14, 2003
//
// ====================================================================================================


#if !defined(AFX_HYPERLINKSTATIC_H__32A71426_1315_407C_9D90_A484C5589D80__INCLUDED_)
#define AFX_HYPERLINKSTATIC_H__32A71426_1315_407C_9D90_A484C5589D80__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


// HyperlinkStatic.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CHyperlinkStatic window

class CHyperlinkStatic : public CStatic
{
// Construction
public:
	CHyperlinkStatic();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHyperlinkStatic)
	protected:
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CHyperlinkStatic();

	// Generated message map functions
protected:
	//{{AFX_MSG(CHyperlinkStatic)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnPaint();
	afx_msg void OnDestroy();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	//}}AFX_MSG
	afx_msg LRESULT OnMouseLeave(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
public:
	void SetHyperlink(CString strHyperlink);
	void SetCaption(CString strCaption);
private:
	CString _strCaption, _strHyperlink;
	CFont _fontCaption;
	CSize _sizeCaption;
	bool _bCreateFont, _bMouseInControl, _bGetCaptionSize;
	HCURSOR	_hHandCursor, _hArrowCursor;

	void CreateFont();
	void GetCaptionSize();
	bool InCaptionRange(CPoint &point);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HYPERLINKSTATIC_H__32A71426_1315_407C_9D90_A484C5589D80__INCLUDED_)
