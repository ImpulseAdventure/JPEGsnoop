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

// DbManageDlg.cpp : implementation file
//

#include "stdafx.h"
#include "JPEGsnoop.h"
#include "DbManageDlg.h"
#include ".\dbmanagedlg.h"


// CDbManageDlg dialog

IMPLEMENT_DYNAMIC(CDbManageDlg, CDialog)
CDbManageDlg::CDbManageDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDbManageDlg::IDD, pParent)
{
	m_asToInsert.SetSize(0,10);
	m_anEntriesInd.SetSize(0,10);
	m_asEntriesVal.SetSize(0,10);

	m_nEntriesOrigMax = 0;

}

CDbManageDlg::~CDbManageDlg()
{
}

void CDbManageDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST, m_listBox);
}


BEGIN_MESSAGE_MAP(CDbManageDlg, CDialog)
	ON_BN_CLICKED(IDC_REMOVE, OnBnClickedRemove)
	ON_BN_CLICKED(IDC_REMOVEALL, OnBnClickedRemoveall)
END_MESSAGE_MAP()


// CDbManageDlg message handlers

void CDbManageDlg::OnBnClickedRemove()
{
	// TODO: Add your control notification handler code here
	int pos;
	pos = m_listBox.GetCurSel();

	// Check for case where no line is selected!
	if (pos != -1) {

		m_listBox.DeleteString(pos); 

		m_anEntriesInd.RemoveAt(pos);
		m_asEntriesVal.RemoveAt(pos);

	}
}

void CDbManageDlg::InsertEntry(unsigned ind,CString strMake, CString strModel, CString strQual, CString strSig)
{
	CString tmpStr;
	tmpStr.Format("Make: [%s]\tModel: [%s]\tQual: [%s]",strMake,strModel,strQual);
	m_asToInsert.Add(tmpStr);
	m_anEntriesInd.Add(ind);
	m_asEntriesVal.Add(tmpStr);
	m_nEntriesOrigMax++;
}

// Copy entries from array to 
void CDbManageDlg::PopulateList()
{
	CString	tmpStr;
	m_nEntriesOrigMax = m_asToInsert.GetCount();
	for (unsigned ind=0;ind<m_nEntriesOrigMax;ind++)
	{
		tmpStr = m_asToInsert.GetAt(ind);
		m_listBox.AddString(tmpStr);

		m_anEntriesInd.Add(ind);
		m_asEntriesVal.Add(tmpStr);
	}

}



void CDbManageDlg::OnBnClickedRemoveall()
{
	m_anEntriesInd.RemoveAll();
	m_asEntriesVal.RemoveAll();
	m_listBox.ResetContent();
}

BOOL CDbManageDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	int	lpiTabs[4];
//	lpiTabs[0] = 120;
//	lpiTabs[1] = 220;
//	lpiTabs[2] = 300;

	lpiTabs[0] = 150;
	lpiTabs[1] = 250;
	lpiTabs[2] = 300;
	m_listBox.SetTabStops(3,lpiTabs);

	// TODO:  Add extra initialization here
	PopulateList();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}



void CDbManageDlg::GetDeleted(CUIntArray& anDeleted)
{
	unsigned ind_cur;
	unsigned ind_orig;
	unsigned ind_new;
	ind_new = 0;
	ind_cur = 0;

	for (ind_orig=0;ind_orig<m_nEntriesOrigMax;ind_orig++) {

		// NOTE: We cannot call *.GetAt(#) when the IntArray is
		// of length 0! (this will happen if we do a "Delete All").
		// In debug mode this will result in a debugger exception.

		// Therefore, we'll check for this condition and fake out
		// the ind_new to be something that will fall through to
		// the next clause which adds it to the deleted list

		if (!m_anEntriesInd.IsEmpty()) {
			ind_new = m_anEntriesInd.GetAt(ind_cur);
		} else {
			// Fake out the index so that it doesn't match original position
			ind_new = 99999;
		}

		if (ind_orig==ind_new) {
			// Entries match, advance current pointer along
			ind_cur++;
		} else {
			// Entry missing, mark as deleted, but don't
			// advance current pointer
			anDeleted.Add(ind_orig);
		}
	}
}