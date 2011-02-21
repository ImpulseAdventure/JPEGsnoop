// JPEGsnoop - JPEG Image Decoder & Analysis Utility
// Copyright (C) 2010 - Calvin Hass
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

#pragma once

#include "ImgDecode.h"

// CLookupDlg dialog

class CLookupDlg : public CDialog
{
	DECLARE_DYNAMIC(CLookupDlg)

public:
	CLookupDlg(CWnd* pParent = NULL);   // standard constructor
	CLookupDlg(CWnd* pParent, CimgDecode* pImgDec, unsigned nSizeX, unsigned nSizeY);
	virtual ~CLookupDlg();

// Dialog Data
	enum { IDD = IDD_LOOKUP };


protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
private:
	afx_msg void OnBnClickedBtnCalc();

	UINT	m_nPixX;
	UINT	m_nPixY;
	CString m_sOffset;
	UINT	m_nTestVal;

	UINT	m_nSizeX;
	UINT	m_nSizeY;

	CString m_sRngX;
	CString m_sRngY;

	CimgDecode*	m_pImgDec;
};
