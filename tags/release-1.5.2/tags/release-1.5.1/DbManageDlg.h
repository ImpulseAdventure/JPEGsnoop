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

#pragma once
#include "afxwin.h"


// CDbManageDlg dialog

class CDbManageDlg : public CDialog
{
	DECLARE_DYNAMIC(CDbManageDlg)

public:
	CDbManageDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDbManageDlg();

// Dialog Data
	enum { IDD = IDD_DBMANAGEDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	void	InsertEntry(unsigned ind,CString strMake, CString strModel, CString strQual, CString strSig);
	void	PopulateList();

	void		GetDeleted(CUIntArray& anDeleted);

private:
	CListBox		m_listBox;
	CStringArray	m_asToInsert;		// Entries ready for PopulateList()

	unsigned		m_nEntriesOrigMax;
	CUIntArray		m_anEntriesInd;		// Index of entries currently  (e.g 1,2,4,7,15)
	CStringArray	m_asEntriesVal;		// Values of entries currently (e.g A,X,D,R,G)

	afx_msg void OnBnClickedRemove();
	afx_msg void OnBnClickedRemoveall();
	virtual BOOL OnInitDialog();


};
