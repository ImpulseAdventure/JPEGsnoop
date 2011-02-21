// BatchDlg.cpp : implementation file
//

#include "stdafx.h"
#include "JPEGsnoop.h"
#include "BatchDlg.h"


// CBatchDlg dialog

IMPLEMENT_DYNAMIC(CBatchDlg, CDialog)
CBatchDlg::CBatchDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CBatchDlg::IDD, pParent)
	, m_strDir(_T(""))
	, m_bProcessSubdir(FALSE)
{
}

CBatchDlg::~CBatchDlg()
{
}

void CBatchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_DIR, m_strDir);
	DDX_Check(pDX, IDC_CHECK_SUBDIRS, m_bProcessSubdir);
}


BEGIN_MESSAGE_MAP(CBatchDlg, CDialog)
END_MESSAGE_MAP()


// CBatchDlg message handlers
