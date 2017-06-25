// JPEGsnoop - JPEG Image Decoder & Analysis Utility
// Copyright (C) 2017 - Calvin Hass
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
// - Dialog box for configuring a file decoder overlay
// - Enables local overwrite of the file buffer during decode for testing purposes
//
// ==========================================================================


#pragma once


// COverlayBufDlg dialog

class COverlayBufDlg : public CDialog
{
	DECLARE_DYNAMIC(COverlayBufDlg)

public:
	COverlayBufDlg(CWnd* pParent = NULL);   // standard constructor
	COverlayBufDlg(CWnd* pParent, 
		bool bEn, unsigned nOffset, unsigned nLen, CString sNewHex, CString sNewBin);
	virtual ~COverlayBufDlg();


// Dialog Data
	enum { IDD = IDD_OVERLAYBUFDLG };

protected:
	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
private:
	afx_msg void	OnBnClickedOvrLoad();
	afx_msg void	OnBnClickedOk();
	afx_msg void	OnBnClickedApply();
	virtual BOOL	OnInitDialog();

public:
	// Callback function for Buf()
	void SetCbBuf(void* pClassCbBuf,
					BYTE (*pCbBuf)(void* pClassCbBuf, unsigned long nNum, bool bBool));
private:
	// References to callback function for Buf()
	void*			m_pClassCbBuf;
	BYTE			(*m_pCbBuf)(void* pClassCbBuf, unsigned long nNum, bool bBool);

public:
	unsigned		m_nOffset;
	unsigned		m_nLen;
	bool			m_bApply;		// When OnOK(), indicate apply and redo dialog
	BOOL			m_bEn;
	CString			m_sValueNewHex;
private:
	CString			m_sOffset;
	CString			m_sValueCurHex;
	CString			m_sValueCurBin;
	CString			m_sValueNewBin;

};
