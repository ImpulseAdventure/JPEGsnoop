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

// BatchDlg.cpp : implementation file
//

#include "stdafx.h"
#include "JPEGsnoop.h"
#include "BatchDlg.h"


// CBatchDlg dialog

IMPLEMENT_DYNAMIC(CBatchDlg, CDialog)
CBatchDlg::CBatchDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CBatchDlg::IDD, pParent)
	, m_strDir(_T(""))
	, m_bProcessSubdir(FALSE)
{
}

CBatchDlg::~CBatchDlg()
{
}

void CBatchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_DIR, m_strDir);
	DDX_Check(pDX, IDC_CHECK_SUBDIRS, m_bProcessSubdir);
}


BEGIN_MESSAGE_MAP(CBatchDlg, CDialog)
END_MESSAGE_MAP()


// CBatchDlg message handlers
