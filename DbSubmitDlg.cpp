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

// DbSubmitDlg.cpp : implementation file
//

#include "stdafx.h"
#include "jpegsnoop.h"
#include "DbSubmitDlg.h"
#include ".\dbsubmitdlg.h"


// CDbSubmitDlg dialog

IMPLEMENT_DYNAMIC(CDbSubmitDlg, CDialog)
CDbSubmitDlg::CDbSubmitDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDbSubmitDlg::IDD, pParent)
	, m_strSig(_T(""))
	, m_strExifModel(_T(""))
	, m_strExifSoftware(_T(""))
	, m_strExifMake(_T(""))
	, m_strQual(_T(""))
	, m_strUserSoftware(_T(""))
	, m_nSource(0)
	, m_strNotes(_T(""))
{
}

CDbSubmitDlg::~CDbSubmitDlg()
{
}

void CDbSubmitDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_SIG, m_strSig);
	DDX_Text(pDX, IDC_MODEL, m_strExifModel);
	DDX_Text(pDX, IDC_SOFTWARE, m_strExifSoftware);
	DDX_Text(pDX, IDC_MAKE, m_strExifMake);
	DDX_Text(pDX, IDC_QUAL, m_strQual);
	DDX_Text(pDX, IDC_USER_SOFTWARE, m_strUserSoftware);
	DDX_Radio(pDX, IDC_RADIO_CAM, m_nSource);
	DDX_Text(pDX, IDC_NOTES, m_strNotes);
}


BEGIN_MESSAGE_MAP(CDbSubmitDlg, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


// CDbSubmitDlg message handlers

void CDbSubmitDlg::OnBnClickedOk()
{
	UpdateData(TRUE);
	// If "Software" selected, then make sure title is filled in!
	if (m_nSource == 1) {
		if (m_strUserSoftware == _T("")) {
			AfxMessageBox(_T("You indicated that this file is processed. Please enter software name."));
			return;
		}
	} else if (m_nSource == 0) {
		// File is apparently "original from digicam"
		if ((m_strExifMake == _T("???")) || (m_strExifModel == _T("???"))) {
			// FIXME:
			// Determine how to handle this scenario. User indicated that the file
			// is "original from digicam" but there is no make / model metadata
			// present. In some cases this could simply be an extracted frame
			// from an AVI file (in which case the AVI container is indicated in the
			// Extras field).
			//AfxMessageBox("You indicated that this file is original, but Make/Model is unspecified.");
			//return;
		}
	}

	// TODO: Add your control notification handler code here
	OnOK();
}
