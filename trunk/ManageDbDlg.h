#pragma once


// CManageDbDlg dialog

class CManageDbDlg : public CDialog
{
	DECLARE_DYNAMIC(CManageDbDlg)

public:
	CManageDbDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CManageDbDlg();

// Dialog Data
	enum { IDD = IDD_MANAGEDBDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
