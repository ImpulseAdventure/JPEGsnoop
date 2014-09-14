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
// - Dialog box for adjusting the starting file decode offset
//
// ==========================================================================


#pragma once
#include "afxwin.h"


// COffsetDlg dialog

class COffsetDlg : public CDialog
{
	DECLARE_DYNAMIC(COffsetDlg)

public:
	COffsetDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~COffsetDlg();
// Dialog Data
	enum { IDD = IDD_OFFSETDLG };

protected:
	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()


public:
	void			SetOffset(unsigned nPos);
	unsigned		GetOffset();

private:
	void			CalInitialDraw();

private:
	afx_msg void	OnBnClickedOk();
	afx_msg void	OnBnClickedBaseh();
	afx_msg void	OnBnClickedBased();

private:
	unsigned		m_nOffsetVal;
	int				m_nRadioBaseMode;
	unsigned		m_nBaseMode;
	CString			m_sOffsetVal;

};
