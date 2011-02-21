// JPEGsnoop - JPEG Image Decoder & Analysis Utility
// Copyright (C) 2010 - Calvin Hass
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

// ModelessDlg.cpp : implementation file
//

#include "stdafx.h"
#include "JPEGsnoop.h"
#include "ModelessDlg.h"
#include ".\modelessdlg.h"


// CModelessDlg dialog

IMPLEMENT_DYNAMIC(CModelessDlg, CDialog)
CModelessDlg::CModelessDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CModelessDlg::IDD, pParent)
{
}

CModelessDlg::~CModelessDlg()
{
}

void CModelessDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CModelessDlg, CDialog)
END_MESSAGE_MAP()


// CModelessDlg message handlers


void CModelessDlg::OnCancel()
{
	//CAL! The following OnCancel() routine is apparently not supposed to be
	// called for a modeless dialog. Instead, we are supposed to call
	// DestroyWindow().

	//CDialog::OnCancel();
	DestroyWindow();
}
