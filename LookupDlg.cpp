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

// LookupDlg.cpp : implementation file
//

#include "stdafx.h"
#include "jpegsnoop.h"
#include "LookupDlg.h"
#include ".\lookupdlg.h"


// CLookupDlg dialog


IMPLEMENT_DYNAMIC(CLookupDlg, CDialog)
CLookupDlg::CLookupDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLookupDlg::IDD, pParent)
	, m_nPixX(0)
	, m_nPixY(0)
	, m_sOffset(_T(""))
	, m_sRngX(_T(""))
	, m_sRngY(_T(""))
	, m_nSizeX(0)
	, m_nSizeY(0)
	, m_pImgDec(NULL)
{
}

CLookupDlg::CLookupDlg(CWnd* pParent,CimgDecode* pImgDec, unsigned nSizeX, unsigned nSizeY)
	: CDialog(CLookupDlg::IDD, pParent)
	, m_nPixX(0)
	, m_nPixY(0)
	, m_sOffset(_T("Not Calculated"))
	, m_sRngX(_T(""))
	, m_sRngY(_T(""))
	, m_nSizeX(nSizeX)
	, m_nSizeY(nSizeY)
{
	m_sRngX.Format("(0..%u)",m_nSizeX-1);
	m_sRngY.Format("(0..%u)",m_nSizeY-1);
	m_pImgDec = pImgDec;
}

CLookupDlg::~CLookupDlg()
{
}

void CLookupDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_X, m_nPixX);
	DDX_Text(pDX, IDC_Y, m_nPixY);
	DDX_Text(pDX, IDC_OFFSET, m_sOffset);
	DDX_Text(pDX, IDC_RNGX, m_sRngX);
	DDX_Text(pDX, IDC_RNGY, m_sRngY);
}


BEGIN_MESSAGE_MAP(CLookupDlg, CDialog)
	ON_BN_CLICKED(IDC_BTN_CALC, OnBnClickedBtnCalc)
END_MESSAGE_MAP()


// CLookupDlg message handlers

void CLookupDlg::OnBnClickedBtnCalc()
{
	ASSERT (m_pImgDec);
	unsigned nByte,nBit;
	UpdateData();

	if (m_nPixX > m_nSizeX-1) {
		AfxMessageBox("Pixel X coord out of range");
	} else if (m_nPixY > m_nSizeY-1) {
		AfxMessageBox("Pixel Y coord out of range");
	} else {
		//offset = theApp.m_pImgDec->LookupFilePosPix(m_nPixX,m_nPixY,nByte,nBit);
		//m_sOffset.Format("0x%08X",offset);
		if (m_pImgDec) {
			m_pImgDec->LookupFilePosPix(m_nPixX,m_nPixY,nByte,nBit);
			m_sOffset.Format("0x%08X : %u",nByte,nBit);
			UpdateData(FALSE);
		}
	}
}

