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

#include "stdafx.h"
#include "jpegsnoop.h"
#include "AboutDlg.h"


CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
, m_staticVerNum(_T(""))
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_URL, m_staticURL);
	DDX_Control(pDX, IDC_URL_DOC, m_staticURLdoc);
	DDX_Text(pDX, IDC_VER_NUM, m_staticVerNum);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()



// CjpegsnoopApp message handlers


BOOL CAboutDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here

	// Dynamically change static text of version number
	CString strTmp;

	strTmp.Format(_T("Version %s"),VERSION_STR);
	m_staticVerNum.SetString(strTmp);
	m_staticVerNum.Format(_T("Version %s"),VERSION_STR);

	UpdateData(FALSE);


	// Update the URLs
	m_staticURL.SetHyperlink(_T("http://www.impulseadventure.com/photo/"));
	strTmp.Format(_T("http://www.impulseadventure.com/photo/jpeg-snoop.html?ver=%s"),VERSION_STR);
	m_staticURLdoc.SetHyperlink(strTmp);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

