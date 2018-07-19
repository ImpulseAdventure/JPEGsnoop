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
// - Dialog box used to look up a file offset from a pixel coordinate
//
// ==========================================================================


#pragma once

// CLookupDlg dialog

class CLookupDlg : public CDialog
{
	DECLARE_DYNAMIC(CLookupDlg)

public:
	CLookupDlg(CWnd* pParent = NULL);   // standard constructor
	CLookupDlg(CWnd* pParent, unsigned nSizeX, unsigned nSizeY);
	virtual ~CLookupDlg();

// Dialog Data
	enum { IDD = IDD_LOOKUP };


protected:
	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
private:
	afx_msg void	OnBnClickedBtnCalc();

public:
	// Callback function for LookupFilePosPix()
	void SetCbLookup(void* pClassCbLookup,
					void (*pCbLookup)(void* pClassCbBuf, unsigned nX,unsigned nY,unsigned &nByte,unsigned &nBit));
private:
	// References to callback function for LookupFilePosPix()
	void*			m_pClassCbLookup;
	void			(*m_pCbLookup)(void* pClassCbLookup,unsigned nX,unsigned nY,unsigned &nByte,unsigned &nBit);

private:
	UINT			m_nPixX;
	UINT			m_nPixY;
	CString			m_strOffset;
	UINT			m_nTestVal;

	UINT			m_nSizeX;
	UINT			m_nSizeY;

	CString			m_strRngX;
	CString			m_strRngY;
};
