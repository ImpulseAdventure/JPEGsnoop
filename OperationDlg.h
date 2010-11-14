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

#include "JPEGsnoopDoc.h"	// For function ptrs
#include "afxcmn.h"

// COperationDlg dialog

class COperationDlg : public CDialog
{
	DECLARE_DYNAMIC(COperationDlg)

public:
	COperationDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~COperationDlg();


	BOOL RunIterated(BOOL bDisableMainWnd=TRUE);
	BOOL RunUntilDone(BOOL bDisableMainWnd=TRUE);
	BOOL m_bAbort;

	// "callback" functions and typedef's to help with syntax
	typedef UINT (CJPEGsnoopDoc::*PFuncInit)();
	typedef BOOL (CJPEGsnoopDoc::*PFuncIter)();
	typedef CString (CJPEGsnoopDoc::*PFuncProg)();

	PFuncInit	m_fInitializeOperation;
	PFuncIter	m_fNextIteration;
	PFuncProg	m_fGetProgress;
	void SetFunctions(PFuncInit f1, PFuncIter f2, PFuncProg f3);

	CTransferDlg* m_pParent;
private:
	BOOL Run(BOOL bRunIterated, BOOL bDisableMainWnd);


public:
// Dialog Data
	enum { IDD = IDD_OPERATIONDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnAbort();
	CStatic m_ctlProgress;
	CProgressCtrl m_ctlProgressBar;
};
