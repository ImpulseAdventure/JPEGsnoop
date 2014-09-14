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
// - JPEGsnoop SDI View class for the main image preview window
// - Handles user mouse-click events to mark MCUs
// - Tracks mouse position to provide continuous read-out in the status bar
//
// ==========================================================================


#pragma once

#include "JPEGsnoopDoc.h"
#include "Dib.h"
#include "snoop.h"

#include "ImgDecode.h"

// CJPEGsnoopViewImg view



class CJPEGsnoopViewImg : public CScrollView
{
	DECLARE_DYNCREATE(CJPEGsnoopViewImg)

protected:
	CJPEGsnoopViewImg();           // protected constructor used by dynamic creation
	virtual ~CJPEGsnoopViewImg();

public:
#ifdef _DEBUG
	virtual void	AssertValid() const;
	virtual void	Dump(CDumpContext& dc) const;
#endif

protected:
	virtual void	OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void	OnInitialUpdate();     // first time after construct

	DECLARE_MESSAGE_MAP()

private:
	CimgDecode*		GetImgDec();
	bool			InPreviewArea(CPoint point,CPoint &ptPix);
	void			SetScrollCenter(float fZoomOld, float fZoomNew);
	int				MeasureFontHeight(CFont* pFont, CDC* pDC);

	afx_msg void	OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void	OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void	OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL	OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void	OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL	OnEraseBkgnd(CDC* pDC);

private:
	CFont*			m_pFont;
	LOGFONT			m_logfont;
	int				m_nPointSize;

};


