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
// - Dialog box for Batch image processing
//
// ==========================================================================


#pragma once


// FIXME:
// For some reason, the IDD_BATCHDLG constant reports C2065 "undeclared identifier"
// So as a workaround, I am explicitly referencing "Resource.h" here
#include "Resource.h"


// CBatchDlg dialog

class CBatchDlg : public CDialog
{
	DECLARE_DYNAMIC(CBatchDlg)

public:
	CBatchDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CBatchDlg();

// Dialog Data
	enum { IDD = IDD_BATCHDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void	OnBnClickedBtnDirSrcBrowse();
	afx_msg void	OnBnClickedBtnDirDstBrowse();

	CString			m_strDir;
	BOOL			m_bProcessSubdir;
	BOOL			m_bExtractAll;
	CString			m_strDirSrc;
	CString			m_strDirDst;
};
