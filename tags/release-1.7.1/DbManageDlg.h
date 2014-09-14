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

// ==========================================================================
// CLASS DESCRIPTION:
// - Dialog box that allows user to manage the local signatures database
// - The caller will invoke InsertEntry() on all extra database signatures
//   which can then be added to the listbox via PopulateList() during OnInitDialog().
// - A sideband parallel array is used to record the original index positions
//   of each entry in the listbox so that we can easily determine which entries
//   have been deleted once the dialog has completed.
// - The parallel array can be queried after the dialog box has closed via GetRemainIndices()
//   to return the list of custom signature indices that remain. This allows
//   the master list to be trimmed down (ie. entries removed).
// ==========================================================================


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
	void			InsertEntry(unsigned ind,CString strMake, CString strModel, CString strQual, CString strSig);
	void			PopulateList();
	void			GetRemainIndices(CUIntArray& anRemain);

private:
	virtual BOOL	OnInitDialog();
	afx_msg void	OnBnClickedRemove();
	afx_msg void	OnBnClickedRemoveall();

private:
	CListBox		m_ctlListBox;		// Listbox representing custom signatures
	CStringArray	m_asToInsert;		// Entries to be added to Listbox via PopulateList()

	CUIntArray		m_anListBoxInd;		// Index of entries currently in listBox  (e.g 1,2,4,7,15)



};
