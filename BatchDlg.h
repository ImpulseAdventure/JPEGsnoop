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
