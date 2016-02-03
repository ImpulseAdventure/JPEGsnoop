// JPEGsnoop - JPEG Image Decoder & Analysis Utility
// Copyright (C) 2015 - Calvin Hass
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
// - Dialog box for the terms & conditions (EULA)
// - This dialog must be agreed to before application can be used
//
// ==========================================================================


#pragma once


// CTermsDlg dialog

class CTermsDlg : public CDialog
{
	DECLARE_DYNAMIC(CTermsDlg)

public:
	CTermsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CTermsDlg();

// Dialog Data
	enum { IDD = IDD_TERMSDLG };

protected:
	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
private:
	afx_msg void OnBnClickedOk();

private:
	CString			strEula;
public:
	BOOL			bEulaOk;
	BOOL			bUpdateAuto;

};
