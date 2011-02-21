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

// OperationDlg.cpp : implementation file
//

#include "stdafx.h"
#include "OperationDlg.h"


// COperationDlg dialog

IMPLEMENT_DYNAMIC(COperationDlg, CDialog)
COperationDlg::COperationDlg(CWnd* pParent /*=NULL*/)
	: CDialog(COperationDlg::IDD, pParent)
{
	m_pParent = static_cast<CTransferDlg*>( pParent );
	m_bAbort = FALSE;
	m_fInitializeOperation = NULL;
	m_fNextIteration = NULL;
	m_fGetProgress = NULL;

}

COperationDlg::~COperationDlg()
{
}

void COperationDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_PROGRESS, m_ctlProgress);
	DDX_Control(pDX, IDC_PROGRESS_BAR, m_ctlProgressBar);
}


BEGIN_MESSAGE_MAP(COperationDlg, CDialog)
	ON_BN_CLICKED(IDC_BTN_ABORT, OnBnClickedBtnAbort)
END_MESSAGE_MAP()


// COperationDlg message handlers

void COperationDlg::OnBnClickedBtnAbort()
{
	m_bAbort = TRUE;
}


void COperationDlg::SetFunctions(PFuncInit f1, PFuncIter f2, PFuncProg f3)
{
	// sets function pointers

	m_fInitializeOperation = f1;
	m_fNextIteration = f2;
	m_fGetProgress = f3;

}


BOOL COperationDlg::RunIterated(BOOL bDisableMainWnd /*=TRUE*/)
{
	return Run(TRUE, bDisableMainWnd);
}


BOOL COperationDlg::RunUntilDone(BOOL bDisableMainWnd /*=TRUE*/)
{
	return Run(FALSE, bDisableMainWnd);
}


BOOL COperationDlg::Run(BOOL bRunIterated, BOOL bDisableMainWnd)
{

	ASSERT( m_pParent != NULL );
	ASSERT( m_bAbort == FALSE );
	ASSERT( m_fInitializeOperation != NULL );
	ASSERT( m_fNextIteration != NULL );
	ASSERT( m_fGetProgress != NULL );

	//
	// disable main window, if flagged

//	::AfxGetMainWnd()->EnableWindow( !bDisableMainWnd );
	m_pParent->EnableWindow( !bDisableMainWnd );


	this->Create(COperationDlg::IDD, m_pParent);
	this->ShowWindow( SW_SHOW );	// shouldn't be needed unless we forget to set 
									// WS_VISIBLE in dialog template


	UINT nIter = 0;
	BOOL bRet, bFinished = FALSE;
	CString info;
	MSG iMsg;

	// now do the lengthy operation; initialize first

	UINT nCount = (m_pParent->*m_fInitializeOperation)();
	CString	strProgress;
	m_ctlProgressBar.SetRange32(0,nCount);

	while ( !m_bAbort && !bFinished )
	{
		// update text of progress indicator
/*
		if ( bRunIterated )
		{
			info.Format("Working on Iteration %d of %d", nIter, nCount );
		}
		else
		{
			info.Format("Working on Iteration %d of approximately %d", nIter, nCount );
		}
*/

		strProgress = (m_pParent->*m_fGetProgress)();
		info = strProgress;

		m_ctlProgressBar.SetPos(nIter);

		m_ctlProgress.SetWindowText( info );	// update status for the user

		bRet = (m_pParent->*m_fNextIteration)();	// do the next iteration

		//
		// pump messages

		while ( ::PeekMessage( &iMsg, NULL, NULL, NULL, PM_NOREMOVE ) )
		{
			::AfxGetThread()->PumpMessage();
		}
		
		// determine whether we're finished

		++nIter;

		if ( bRunIterated )
		{
			bFinished = nIter>nCount;
		}
		else
		{
			bFinished = !bRet;
		}

	}

	//
	// re-enable main window and exit

//	::AfxGetMainWnd()->EnableWindow( TRUE );
	m_pParent->EnableWindow( TRUE );

	this->DestroyWindow();

	return !m_bAbort;

}
