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
	, m_strOffset(_T(""))
	, m_strRngX(_T(""))
	, m_strRngY(_T(""))
	, m_nSizeX(0)
	, m_nSizeY(0)
{
	// Initialize callback functions to NULL
	m_pCbLookup = NULL;
	m_pClassCbLookup = NULL;
}

CLookupDlg::CLookupDlg(CWnd* pParent, unsigned nSizeX, unsigned nSizeY)
	: CDialog(CLookupDlg::IDD, pParent)
	, m_nPixX(0)
	, m_nPixY(0)
	, m_strOffset(_T("Not Calculated"))
	, m_strRngX(_T(""))
	, m_strRngY(_T(""))
	, m_nSizeX(nSizeX)
	, m_nSizeY(nSizeY)
{
	m_strRngX.Format(_T("(0..%u)"),m_nSizeX-1);
	m_strRngY.Format(_T("(0..%u)"),m_nSizeY-1);

	// Initialize callback functions to NULL
	m_pCbLookup = NULL;
	m_pClassCbLookup = NULL;
}

CLookupDlg::~CLookupDlg()
{
}

void CLookupDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_X, m_nPixX);
	DDX_Text(pDX, IDC_Y, m_nPixY);
	DDX_Text(pDX, IDC_OFFSET, m_strOffset);
	DDX_Text(pDX, IDC_RNGX, m_strRngX);
	DDX_Text(pDX, IDC_RNGY, m_strRngY);
}


BEGIN_MESSAGE_MAP(CLookupDlg, CDialog)
	ON_BN_CLICKED(IDC_BTN_CALC, OnBnClickedBtnCalc)
END_MESSAGE_MAP()


// Set callback function for Buf()
void CLookupDlg::SetCbLookup(void* pClassCbLookup,
							  void (*pCbLookup)(void* pClassCbLookup, unsigned nX,unsigned nY,unsigned &nByte,unsigned &nBit)
							  )
{
	// Save pointer to class and function
	m_pClassCbLookup = pClassCbLookup;
	m_pCbLookup = pCbLookup;
}

// CLookupDlg message handlers

void CLookupDlg::OnBnClickedBtnCalc()
{
	ASSERT(m_pCbLookup);
	unsigned nByte = 0;
	unsigned nBit = 0;
	UpdateData();

	if (m_nPixX > m_nSizeX-1) {
		AfxMessageBox(_T("Pixel X coord out of range"));
	} else if (m_nPixY > m_nSizeY-1) {
		AfxMessageBox(_T("Pixel Y coord out of range"));
	} else {
		if (m_pCbLookup) {
			// Use callback function for lookup
			m_pCbLookup(m_pClassCbLookup,m_nPixX,m_nPixY,nByte,nBit);
			m_strOffset.Format(_T("0x%08X : %u"),nByte,nBit);
			UpdateData(FALSE);
		}
	}
}

