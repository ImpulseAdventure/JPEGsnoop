#pragma once

//CAL! BUG: I have no idea why, but for some reason the IDD_BATCHDLG
// constant is "undeclared identifier" below. So, as a temporary
// workaround, I have explicitly referenced "Resource.h".
#include "Resource.h"

// CBatchDlg dialog

class CBatchDlg : public CDialog
{
	DECLARE_DYNAMIC(CBatchDlg)

public:
	CBatchDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CBatchDlg();

// Dialog Data
	enum { IDD = IDD_BATCHDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CString m_strDir;
	BOOL m_bProcessSubdir;
};
