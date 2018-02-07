// JPEGsnoop - JPEG Image Decoder & Analysis Utility
// Copyright (C) 2017 - Calvin Hass
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

// TermsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "JPEGsnoop.h"
#include "TermsDlg.h"
#include ".\termsdlg.h"


// CTermsDlg dialog

IMPLEMENT_DYNAMIC(CTermsDlg, CDialog)
CTermsDlg::CTermsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTermsDlg::IDD, pParent)
	, strEula(_T(""))
	, bEulaOk(FALSE)
	, bUpdateAuto(FALSE)
{
	bEulaOk = false;
	bUpdateAuto = true;


	// Present the user with the GPLv2 license
	// - This is done primarily to inform the user of their rights and
	//   freedoms while reminding them that there are no warranties or
	//   guarantees supplied with the free software per GPLv2.

	strEula = _T("");
	strEula += _T("JPEGsnoop\r\n");
	strEula += _T("Copyright (C) 2018 Calvin Hass\r\n");
	strEula += _T("\r\n");
	strEula += _T("This program is free software; you can redistribute it and/or modify\r\n");
	strEula += _T("it under the terms of the GNU General Public License as published by\r\n");
	strEula += _T("the Free Software Foundation; either version 2 of the License, or\r\n");
	strEula += _T("(at your option) any later version.\r\n");
	strEula += _T("\r\n");
	strEula += _T("This program is distributed in the hope that it will be useful,\r\n");
	strEula += _T("but WITHOUT ANY WARRANTY; without even the implied warranty of\r\n");
	strEula += _T("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\r\n");
	strEula += _T("GNU General Public License for more details.\r\n");
	strEula += _T("\r\n");
	strEula += _T("You should have received a copy of the GNU General Public License\r\n");
	strEula += _T("along with this program.  If not, see <http://www.gnu.org/licenses/>.\r\n");

}

CTermsDlg::~CTermsDlg()
{
}

void CTermsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EULA, strEula);
	DDX_Check(pDX, IDC_EULA_OK, bEulaOk);
	DDX_Check(pDX, IDC_UPDATE_AUTO, bUpdateAuto);
}


BEGIN_MESSAGE_MAP(CTermsDlg, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


// CTermsDlg message handlers

void CTermsDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	UpdateData(true);
	if (bEulaOk) {
		OnOK();
	} else {
		AfxMessageBox(_T("You need to agree to the terms or else click EXIT"));
	}
}

