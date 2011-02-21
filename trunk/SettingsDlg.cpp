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

// SettingsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "JPEGsnoop.h"
#include "SettingsDlg.h"
#include ".\settingsdlg.h"

#include "FolderDlg.h"
#include "JPEGsnoop.h"
#include "SnoopConfig.h"


// CSettingsDlg dialog

IMPLEMENT_DYNAMIC(CSettingsDlg, CDialog)
CSettingsDlg::CSettingsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSettingsDlg::IDD, pParent)
	, m_strDbDir(_T(""))
	, m_bUpdateAuto(FALSE)
	, m_bReprocessAuto(FALSE)
	, m_nUpdateChkDays(0)
	, m_bDbSubmitNet(FALSE)
	, m_nRptErrMaxScanDecode(0)
{
}

CSettingsDlg::~CSettingsDlg()
{
}

void CSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_DB_DIR, m_strDbDir);
	DDX_Check(pDX, IDC_UPDATE_CHK, m_bUpdateAuto);
	DDX_Check(pDX, IDC_REPROCESS_AUTO, m_bReprocessAuto);
	DDX_Text(pDX, IDC_UPDATE_CHK_DAYS, m_nUpdateChkDays);
	DDV_MinMaxUInt(pDX, m_nUpdateChkDays, 1, 365);
	DDX_Check(pDX, IDC_DBSUBMIT_NET, m_bDbSubmitNet);
	DDX_Text(pDX, IDC_RPTERRMAX_SCANDECODE, m_nRptErrMaxScanDecode);
}


BEGIN_MESSAGE_MAP(CSettingsDlg, CDialog)
	ON_BN_CLICKED(IDC_DB_DIR_BROWSE, OnBnClickedDbDirBrowse)
	ON_BN_CLICKED(IDC_DB_DIR_DEFAULT, OnBnClickedDbDirDefault)
	ON_BN_CLICKED(IDC_COACH_RESET, OnBnClickedCoachReset)
END_MESSAGE_MAP()


// CSettingsDlg message handlers

void CSettingsDlg::OnBnClickedDbDirBrowse()
{
	// TODO: Add your control notification handler code here

	/*
	CFolderDialog dlg(this);
	LPCITEMIDLIST pidl = dlg.BrowseForFolder(_T("Pick a folder, any folder:"),
		BIF_RETURNONLYFSDIRS | BIF_STATUSTEXT);
	CString msg;
	if (pidl)
		msg.Format(_T("You chose: %s"), dlg.GetPathName(pidl));
	else
		msg = _T("You canceled without selecting a folder.");

	dlg.FreePIDL(pidl);	// free PIDL when done--important!

	AfxMessageBox(msg);
	*/

	CString strDir;
	strDir = SelectFolder("Please select folder for User Database");
	if (strDir == "") {
		//AfxMessageBox("You cancelled");
	} else {
		//AfxMessageBox(strDir);
		m_strDbDir = strDir;
		UpdateData(false);
	}
}

CString CSettingsDlg::SelectFolder(const CString& strMessage)
{
	CString Result;
	
	BROWSEINFO BrowseInfo;
	memset(&BrowseInfo, 0, sizeof(BrowseInfo));
	
	char szBuffer[MAX_PATH];
	
	BrowseInfo.hwndOwner      = m_hWnd;
	BrowseInfo.pszDisplayName = szBuffer;
	BrowseInfo.lpszTitle      = strMessage;
	BrowseInfo.ulFlags        = BIF_RETURNONLYFSDIRS;
	
	// Throw display dialog
	LPITEMIDLIST pList = SHBrowseForFolder(&BrowseInfo);
	ASSERT(strlen(szBuffer) < sizeof(szBuffer));
	
	if (pList != NULL)
	{
		// Convert from MIDLISt to real string path
		SHGetPathFromIDList(pList, szBuffer);
		Result = szBuffer;
		
		// Global pointer to the shell's IMalloc interface.  
		LPMALLOC pMalloc; 
		
		// Get the shell's allocator and free the PIDL returned by
		// SHBrowseForFolder.
		if (SUCCEEDED(SHGetMalloc(&pMalloc))) 
			pMalloc->Free(pList);
	}
	
	return Result;
}

LPITEMIDLIST CSettingsDlg::ConvertPathToLpItemIdList(const char *pszPath)
{
    LPITEMIDLIST  pidl;
    LPSHELLFOLDER pDesktopFolder;
    OLECHAR       olePath[MAX_PATH];
    ULONG         chEaten;
    ULONG         dwAttributes;
    HRESULT       hr;

    if (SUCCEEDED(SHGetDesktopFolder(&pDesktopFolder)))
    {
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszPath, -1, olePath, 
                            MAX_PATH);
        hr = pDesktopFolder->ParseDisplayName(NULL,NULL,olePath,&chEaten,
                                              &pidl,&dwAttributes);
        pDesktopFolder->Release();
    }
    return pidl;
}

void CSettingsDlg::OnBnClickedDbDirDefault()
{
	CJPEGsnoopApp*	pApp;
	pApp = (CJPEGsnoopApp*)AfxGetApp();
	m_strDbDir = pApp->m_pAppConfig->GetDefaultDbDir();
	UpdateData(false);
}

void CSettingsDlg::OnBnClickedCoachReset()
{
	CJPEGsnoopApp*	pApp;
	pApp = (CJPEGsnoopApp*)AfxGetApp();
	pApp->m_pAppConfig->CoachReset();
}
