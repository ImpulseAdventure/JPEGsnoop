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

// OverlayBufDlg.cpp : implementation file
//

#include "stdafx.h"
#include "jpegsnoop.h"
#include "OverlayBufDlg.h"
#include ".\overlaybufdlg.h"

#include "General.h"

// COverlayBufDlg dialog

IMPLEMENT_DYNAMIC(COverlayBufDlg, CDialog)
COverlayBufDlg::COverlayBufDlg(CWnd* pParent /*=NULL*/)
	: CDialog(COverlayBufDlg::IDD, pParent)
	, m_sOffset(_T(""))
	, m_sValueCurHex(_T(""))
	, m_sValueNewHex(_T(""))
	, m_nLen(0)
	, m_bEn(FALSE)
	, m_sValueCurBin(_T(""))
{
	// FIXME: Should probably mark this as invalid
	m_bApply = false;
	m_nOffset = 0;

	// Initialize callback functions to NULL
	m_pCbBuf = NULL;
	m_pClassCbBuf = NULL;
}

COverlayBufDlg::COverlayBufDlg(CWnd* pParent,
							   bool bEn, unsigned nOffset, unsigned nLen, CString sNewHex, CString sNewBin)
	: CDialog(COverlayBufDlg::IDD, pParent)
	, m_sOffset(_T(""))
	, m_sValueCurHex(_T(""))
	, m_sValueNewHex(_T(""))
	, m_nLen(0)
	, m_bEn(FALSE)
{
	m_bEn = bEn;
	m_nOffset = nOffset;
	m_nLen = nLen;
	m_sValueNewHex = sNewHex;
	m_sValueNewBin = sNewBin;

	// Initialize callback functions to NULL
	m_pCbBuf = NULL;
	m_pClassCbBuf = NULL;

	m_bApply = false;

	// Now recalc string fields
	m_sOffset.Format(_T("0x%08X"),m_nOffset);

}


COverlayBufDlg::~COverlayBufDlg()
{
}

void COverlayBufDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_OVRPOS, m_sOffset);
	DDX_Text(pDX, IDC_OVRCUR, m_sValueCurHex);
	DDX_Text(pDX, IDC_OVRNEW, m_sValueNewHex);
	DDX_Text(pDX, IDC_OVRLEN, m_nLen);
	DDX_Check(pDX, IDC_OVREN, m_bEn);
	DDX_Text(pDX, IDC_OVRCURBIN, m_sValueCurBin);
}


BEGIN_MESSAGE_MAP(COverlayBufDlg, CDialog)
	ON_BN_CLICKED(IDC_OVR_LOAD, OnBnClickedOvrLoad)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDC_APPLY, OnBnClickedApply)
END_MESSAGE_MAP()


// Set callback function for Buf()
void COverlayBufDlg::SetCbBuf(void* pClassCbBuf,
							  BYTE (*pCbBuf)(void* pClassCbBuf, unsigned long nNum, bool bBool)
							  )
{
	// Save pointer to class and function
	m_pClassCbBuf = pClassCbBuf;
	m_pCbBuf = pCbBuf;
}


// COverlayBufDlg message handlers



void COverlayBufDlg::OnBnClickedOvrLoad()
{
	// TODO: Add your control notification handler code here
	CString		strTmp;
	unsigned	anCurVal[17];

	ASSERT(m_pCbBuf);
	if (!m_pCbBuf) {
		return;
	}

	UpdateData();
	m_nOffset = _tcstoul(m_sOffset,NULL,16);

	// get the data at the file position
	for (unsigned nInd=0;nInd<16;nInd++) {
		// Request the "clean" data as otherwise we'll see the
		// last overlay data, which might be confusing!
		if (m_pCbBuf) {
			// Use callback function for Buf()
			anCurVal[nInd] = m_pCbBuf(m_pClassCbBuf,m_nOffset+nInd,true);
		} else {
			anCurVal[nInd] = 0; // FIXME:

		}
	}

	strTmp.Format(_T("0x %02X %02X %02X %02X %02X %02X %02X %02X ..."),
		anCurVal[0],anCurVal[1],anCurVal[2],anCurVal[3],
		anCurVal[4],anCurVal[5],anCurVal[6],anCurVal[7]);
	m_sValueCurHex = strTmp;

	CString strTmpBin = _T("0b ");
	strTmpBin += Dec2Bin(anCurVal[0],8);	strTmpBin += _T(" ");
	strTmpBin += Dec2Bin(anCurVal[1],8);	strTmpBin += _T(" ");
	strTmpBin += Dec2Bin(anCurVal[2],8);	strTmpBin += _T(" ");
	strTmpBin += Dec2Bin(anCurVal[3],8);	strTmpBin += _T(" ");
	strTmpBin += Dec2Bin(anCurVal[4],8);	strTmpBin += _T(" ");
	strTmpBin += Dec2Bin(anCurVal[5],8);	strTmpBin += _T(" ");
	strTmpBin += Dec2Bin(anCurVal[6],8);	strTmpBin += _T(" ");
	strTmpBin += Dec2Bin(anCurVal[7],8);	strTmpBin += _T(" ...");
	m_sValueCurBin = strTmpBin;

	UpdateData(false);
}

void COverlayBufDlg::OnBnClickedOk()
{
	UpdateData();
	m_nOffset = _tcstoul(m_sOffset,NULL,16);

	m_bApply = false;
	OnOK();
}

void COverlayBufDlg::OnBnClickedApply()
{
	UpdateData();
	m_nOffset = _tcstoul(m_sOffset,NULL,16);

	m_bApply = true;
	OnOK();
}

BOOL COverlayBufDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CRect	rect;
	// TODO:  Add extra initialization here
	GetWindowRect(&rect);
	SetWindowPos(NULL,50,50,rect.Width(),rect.Height(),0);

	// Get the initial values
	OnBnClickedOvrLoad();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
