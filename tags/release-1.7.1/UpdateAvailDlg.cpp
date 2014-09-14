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

// UpdateAvailDlg.cpp : implementation file
//

#include "stdafx.h"
#include "JPEGsnoop.h"
#include "UpdateAvailDlg.h"
#include ".\updateavaildlg.h"


// CUpdateAvailDlg dialog

IMPLEMENT_DYNAMIC(CUpdateAvailDlg, CDialog)
CUpdateAvailDlg::CUpdateAvailDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CUpdateAvailDlg::IDD, pParent)
	, strVerCur(_T(""))
	, strVerLatest(_T(""))
	, bUpdateAutoStill(FALSE)
	, strDateLatest(_T(""))
{
}

CUpdateAvailDlg::~CUpdateAvailDlg()
{
}

void CUpdateAvailDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_VER_CUR, strVerCur);
	DDX_Text(pDX, IDC_VER_LATEST, strVerLatest);
	DDX_Check(pDX, IDC_UPDATE_AUTO_STILL, bUpdateAutoStill);
	DDX_Text(pDX, IDC_DATE_LATEST, strDateLatest);
}


BEGIN_MESSAGE_MAP(CUpdateAvailDlg, CDialog)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
END_MESSAGE_MAP()


// CUpdateAvailDlg message handlers

void CUpdateAvailDlg::OnBnClickedButton1()
{
	// Open website
	ShellExecute(0, _T("open"), IA_UPDATES_DL_PAGE, 0, 0, SW_SHOWNORMAL);
}
