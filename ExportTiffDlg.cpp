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

// ExportTiffDlg.cpp : implementation file
//

#include "stdafx.h"
#include "JPEGsnoop.h"
#include "ExportTiffDlg.h"
#include ".\exporttiffdlg.h"


// CExportTiffDlg dialog

IMPLEMENT_DYNAMIC(CExportTiffDlg, CDialog)
CExportTiffDlg::CExportTiffDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CExportTiffDlg::IDD, pParent)
	, m_nCtlFmt(0)
	, m_sFname(_T(""))
{
}

CExportTiffDlg::~CExportTiffDlg()
{
}

void CExportTiffDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_RAD_RGB8, m_nCtlFmt);
	DDX_Text(pDX, IDC_EDIT_FNAME, m_sFname);
}


BEGIN_MESSAGE_MAP(CExportTiffDlg, CDialog)
END_MESSAGE_MAP()


// CExportTiffDlg message handlers

BOOL CExportTiffDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here
	m_nCtlFmt = 0;
	UpdateData(false);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
