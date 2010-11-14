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
	, m_pWBuf(NULL)
	, m_sValueCurBin(_T(""))
{
	//CAL! Should probably mark this as invalid!!!
	m_bApply = false;
	m_nOffset = 0;
}

COverlayBufDlg::COverlayBufDlg(CWnd* pParent, CwindowBuf* pWBuf, bool bEn, 
					unsigned nOffset, unsigned nLen, CString sNewHex, CString sNewBin)
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

	m_pWBuf = pWBuf;

	m_bApply = false;

	// Now recalc string fields
	m_sOffset.Format("0x%08X",m_nOffset);

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


// COverlayBufDlg message handlers



void COverlayBufDlg::OnBnClickedOvrLoad()
{
	// TODO: Add your control notification handler code here
	CString tmpStr;
	unsigned cur_val[17];

	ASSERT(m_pWBuf);

	UpdateData();
	m_nOffset = strtol(m_sOffset,NULL,16);

	// get the data at the file position
	for (unsigned i=0;i<16;i++) {
		// Request the "clean" data as otherwise we'll see the
		// last overlay data, which might be confusing!

		//CAL! FIXME
		if (m_pWBuf) {
			cur_val[i] = m_pWBuf->Buf(m_nOffset+i,true);
		} else {
			cur_val[i] = 00; //CAL! FIXME

		}
	}

//	tmpStr.Format("0x%08X = 0x %02X %02X %02X %02X %02X %02X %02X %02X ...",
//		m_nOffset,cur_val[0],cur_val[1],cur_val[2],cur_val[3],
//		cur_val[4],cur_val[5],cur_val[6],cur_val[7]);
	tmpStr.Format("0x %02X %02X %02X %02X %02X %02X %02X %02X ...",
		cur_val[0],cur_val[1],cur_val[2],cur_val[3],
		cur_val[4],cur_val[5],cur_val[6],cur_val[7]);
	m_sValueCurHex = tmpStr;

	CString tmpStrBin = "0b ";
	tmpStrBin += Dec2Bin(cur_val[0],8);	tmpStrBin += " ";
	tmpStrBin += Dec2Bin(cur_val[1],8);	tmpStrBin += " ";
	tmpStrBin += Dec2Bin(cur_val[2],8);	tmpStrBin += " ";
	tmpStrBin += Dec2Bin(cur_val[3],8);	tmpStrBin += " ";
	tmpStrBin += Dec2Bin(cur_val[4],8);	tmpStrBin += " ";
	tmpStrBin += Dec2Bin(cur_val[5],8);	tmpStrBin += " ";
	tmpStrBin += Dec2Bin(cur_val[6],8);	tmpStrBin += " ";
	tmpStrBin += Dec2Bin(cur_val[7],8);	tmpStrBin += " ...";
	m_sValueCurBin = tmpStrBin;

	UpdateData(false);
}

void COverlayBufDlg::OnBnClickedOk()
{
	UpdateData();
	m_nOffset = strtol(m_sOffset,NULL,16);

	m_bApply = false;
	OnOK();
}

/*
CString COverlayBufDlg::Dec2Bin(unsigned nVal,unsigned nLen)
{
	unsigned	nBit;
	unsigned	n1,n2,n3;
	CString		strBin = "";
	for (int nInd=nLen-1;nInd>=0;nInd--)
	{
		n1 = (1 << nInd);
		n2 = ( nVal & n1 );
		n3 = n2 >> nInd;
		nBit = ( nVal & (1 << nInd) ) >> nInd;
		nBit = n3;
		strBin += (nBit==1)?"1":"0";
		if ( ((nInd % 8) == 0) && (nInd != 0) ) {
			strBin += " ";
		}
	}
	return strBin;
}
*/

void COverlayBufDlg::OnBnClickedApply()
{
	UpdateData();
	m_nOffset = strtol(m_sOffset,NULL,16);

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
