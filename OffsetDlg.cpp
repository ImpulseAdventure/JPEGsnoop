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

// dlg_offset.cpp : implementation file
//

#include "stdafx.h"
#include "JPEGsnoop.h"
#include "OffsetDlg.h"


// COffsetDlg dialog

IMPLEMENT_DYNAMIC(COffsetDlg, CDialog)
COffsetDlg::COffsetDlg(CWnd* pParent /*=NULL*/)
	: CDialog(COffsetDlg::IDD, pParent)
	, m_sOffsetVal(_T(""))
{
	m_nBaseMode = 0;		// 0=hex, 1=dec
	m_nRadioBaseMode = 0;
}

COffsetDlg::~COffsetDlg()
{
}


// Set the initial file offset
void COffsetDlg::SetOffset(unsigned nPos)
{
	m_nOffsetVal = nPos;
	OffsetNum2Str();
}

// Fetch the current offset value from the dialog
unsigned COffsetDlg::GetOffset()
{
	return m_nOffsetVal;
}


void COffsetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_OFFSETVAL, m_sOffsetVal);
	DDX_Radio(pDX, IDC_BASEH, m_nRadioBaseMode);
}


BEGIN_MESSAGE_MAP(COffsetDlg, CDialog)
	ON_BN_CLICKED(IDC_BASEH, OnBnClickedBaseh)
	ON_BN_CLICKED(IDC_BASED, OnBnClickedBased)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


// COffsetDlg message handlers


// Convert internal offset number to dialog string
void COffsetDlg::OffsetNum2Str()
{
	CString strVal;

	if (m_nBaseMode == 0) {
		// Hex
		strVal.Format(_T("0x%08X"),m_nOffsetVal);
		m_sOffsetVal = strVal;
	} else {
		// Dec
		strVal.Format(_T("%u"),m_nOffsetVal);
		m_sOffsetVal = strVal;
	}

}

// Convert the current dialog offset string into
// the internal offset value. If data validation
// fails, then display a dialog box.
//
// RETURN:
// - True if validation OK
//
bool COffsetDlg::OffsetStr2Num()
{
	CString		strVal;


	if (m_nBaseMode == 0) {
		// Hex
		if (!Str2Uint32(m_sOffsetVal,16,m_nOffsetVal)) {
			AfxMessageBox(_T("Invalid hex string"));
			return false;
		}
	} else {
		// Decimal
		if (!Str2Uint32(m_sOffsetVal,10,m_nOffsetVal)) {
			AfxMessageBox(_T("Invalid decimal string"));
			return false;
		}
	}
	return true;

}

// Change dialog mode to hex
// - If in decimal mode, convert from decimal to hex
// - Report data validation error
void COffsetDlg::OnBnClickedBaseh()
{
	CString		strVal;
	bool		bOk;

	UpdateData(true);
	if (m_nBaseMode == 0) {
		// If it was hex before, don't do anything
	} else {
		// If it was decimal before, convert it!
		bOk = OffsetStr2Num();
		m_nBaseMode = 0;
		if (!bOk) {
			// There was a validation error
			// - Force the value to zero
			// TODO: Would be nice if we could instead
			// prevent the radio button from transitioning state.
			m_nOffsetVal = 0;
		}
	}
	OffsetNum2Str();
	UpdateData(false);
}

// Change dialog mode to decimal
// - If in hex mode, convert from hex to decimal
// - Report data validation error
void COffsetDlg::OnBnClickedBased()
{
	CString strVal;
	bool	bOk;

	UpdateData(true);
	if (m_nBaseMode == 1) {
		// If it was decimal before, don't do anything
	} else {
		// If it was hex before, convert it!
		bOk = OffsetStr2Num();
		m_nBaseMode = 1;
		if (!bOk) {
			// There was a validation error
			// - Force the value to zero
			// TODO: Would be nice if we could instead
			// prevent the radio button from transitioning state.
			m_nOffsetVal = 0;
		}
	}
	OffsetNum2Str();
	UpdateData(false);
}

void COffsetDlg::OnBnClickedOk()
{
	// Store the new local value before we return to dialog caller
	UpdateData();

	bool bOk = OffsetStr2Num();

	// Only leave the dialog if the data validation was successful
	if (bOk) {
		OnOK();
	}

}
