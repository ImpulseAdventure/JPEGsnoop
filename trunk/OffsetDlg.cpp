// JPEGsnoop - JPEG Image Decoder & Analysis Utility
// Copyright (C) 2014 - Calvin Hass
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

void COffsetDlg::CalInitialDraw()
{
	CString valStr;

	if (m_nBaseMode == 0) {
		// Hex
		valStr.Format(_T("0x%08X"),m_nOffsetVal);
		m_sOffsetVal = valStr;
	} else {
		// Dec
		valStr.Format(_T("%u"),m_nOffsetVal);
		m_sOffsetVal = valStr;
	}

	//UpdateData(false);
}

void COffsetDlg::SetOffset(unsigned nPos)
{
	CString valStr;

	m_nOffsetVal = nPos;

	// Assume that m_nOffsetVal came directly from caller,
	// so we want to update our controls assuming that
	// we are in Hex mode by default.
	valStr.Format(_T("0x%08X"),m_nOffsetVal);
	m_sOffsetVal = valStr;
	CalInitialDraw();
}

unsigned COffsetDlg::GetOffset()
{
	return m_nOffsetVal;
}


void COffsetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//DDX_Control(pDX, IDC_BASEH, btnBaseHex);
	//DDX_Control(pDX, IDC_BASED, btnBaseDec);
	DDX_Text(pDX, IDC_OFFSETVAL, m_sOffsetVal);
	DDX_Radio(pDX, IDC_BASEH, m_nRadioBaseMode);
}


BEGIN_MESSAGE_MAP(COffsetDlg, CDialog)
	ON_BN_CLICKED(IDC_BASEH, OnBnClickedBaseh)
	ON_BN_CLICKED(IDC_BASED, OnBnClickedBased)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


// COffsetDlg message handlers



void COffsetDlg::OnBnClickedBaseh()
{
	CString valStr;

	// Set to Hex
	//btnBaseHex.SetCheck(true);
	//btnBaseDec.SetCheck(false);

	if (m_nBaseMode == 0) {
		// If it was hex before, don't do anything
	} else {
		// If it was dec before, convert it!
		m_nBaseMode = 0;
		UpdateData();
		m_nOffsetVal = _tstoi(m_sOffsetVal);
		valStr.Format(_T("0x%08X"),m_nOffsetVal);
		m_sOffsetVal = valStr;
		UpdateData(false);
	}
}

void COffsetDlg::OnBnClickedBased()
{
	CString valStr;

	// Set to Dec
	//btnBaseHex.SetCheck(false);
	//btnBaseDec.SetCheck(true);

	if (m_nBaseMode == 1) {
		// If it was dec before, don't do anything
	} else {
		// If it was hex before, convert it!
		m_nBaseMode = 1;
		UpdateData();
		m_nOffsetVal = _tcstol(m_sOffsetVal,NULL,16);
		valStr.Format(_T("%u"),m_nOffsetVal);
		m_sOffsetVal = valStr;
		UpdateData(false);
	}
}

void COffsetDlg::OnBnClickedOk()
{
	bool		valid = false;
	unsigned	ind_start = 0;
	unsigned	ind_max;
	TCHAR		ch;
	unsigned	val_new;
	CString		strTmp;

	// Store the new local value (decimal) just before
	// we return to the caller
	UpdateData();
	val_new = 0;
	ind_max = (unsigned)_tcslen(m_sOffsetVal);

	if (m_nBaseMode == 0) {
		// Hex mode
		if (!_tcsnccmp(m_sOffsetVal,_T("0x"),2)) {
			ind_start = 2;
		}
		valid = true;
		for (unsigned i=ind_start;i<ind_max;i++) {
			ch = toupper(m_sOffsetVal.GetAt(i));
			if (!isdigit(ch) && !(ch >= 'A' && ch <= 'F')) {
				valid = false;
				strTmp.Format(_T("ERROR: Invalid hex digit [%c]"),ch);
				AfxMessageBox(strTmp);
			}
		}
		if (valid) {
			val_new = _tcstol(m_sOffsetVal,NULL,16);
		}
	} else {
		// Decimal mode
		valid = true;
		for (unsigned i=0;i<ind_max;i++) {

			ch = m_sOffsetVal.GetAt(i);
			if (!isdigit(ch)) {
				valid = false;
				strTmp.Format(_T("ERROR: Invalid decimal digit [%c]"),ch);
				AfxMessageBox(strTmp);
			}
		}

		if (valid) {
			if (_tstoi(m_sOffsetVal) >= 0) {
				val_new = _tstoi(m_sOffsetVal);
			} else {
				AfxMessageBox(_T("ERROR: Offset must be >= 0"));
				valid = false;
			}
		}
	}

	if (valid) {
		m_nOffsetVal = val_new;
		// Call the normal processing now
		OnOK();
	}
}
