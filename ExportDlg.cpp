// JPEGsnoop - JPEG Image Decoder & Analysis Utility
// Copyright (C) 2015 - Calvin Hass
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

// ExportDlg.cpp : implementation file
//

#include "stdafx.h"
#include "JPEGsnoop.h"
#include "ExportDlg.h"
#include ".\exportdlg.h"


// CExportDlg dialog

IMPLEMENT_DYNAMIC(CExportDlg, CDialog)
CExportDlg::CExportDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CExportDlg::IDD, pParent)
	, m_bOverlayEn(FALSE)
	, m_bDhtAviInsert(FALSE)
	, m_strOffsetStart(_T(""))
	, m_bForceEoi(FALSE)
	, m_bIgnoreEoi(FALSE)
	, m_bForceSoi(FALSE)
	, m_bExtractAllEn(FALSE)
{
}

CExportDlg::~CExportDlg()
{
}

void CExportDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_OVERLAY, m_bOverlayEn);
	DDX_Check(pDX, IDC_DHTAVI, m_bDhtAviInsert);
	DDX_Text(pDX, IDC_OFFSET_START, m_strOffsetStart);
	DDX_Check(pDX, IDC_FORCE_EOI, m_bForceEoi);
	DDX_Check(pDX, IDC_IGNORE_EOI, m_bIgnoreEoi);
	DDX_Check(pDX, IDC_FORCE_SOI, m_bForceSoi);
	DDX_Check(pDX, IDC_EXT_ALL, m_bExtractAllEn);
}


BEGIN_MESSAGE_MAP(CExportDlg, CDialog)
END_MESSAGE_MAP()


