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
// - Simple wrapper for document log routines
//
// ==========================================================================


#pragma once

class CDocLog
{
public:
	CDocLog();
	~CDocLog(void);

	void		AddLine(CString str);
	void		AddLineHdr(CString str);
	void		AddLineHdrDesc(CString str);
	void		AddLineWarn(CString str);
	void		AddLineErr(CString str);
	void		AddLineGood(CString str);

	void		Enable();
	void		Disable();
	void		SetQuickMode(bool bQuick);
	bool		GetQuickMode();

	void		SetDoc(CDocument *pDoc);
	void		Clear();

	unsigned	GetNumLinesLocal();
	bool		GetLineLogLocal(unsigned nLine,CString &strOut,COLORREF &sCol);

	void		DoLogSave(CString strLogName);

private:
	unsigned	AppendToLogLocal(CString strTxt, COLORREF sColor);

private:

	bool			m_bUseDoc;		// Use Document or local buffer
	CDocument*		m_pDoc;
	bool			m_bEn;

	// Local buffer
	CStringArray	m_saLogQuickTxt;
	CUIntArray		m_naLogQuickCol;

	bool			m_bLogQuickMode;	// In m_bUseDoc=TRUE, do we write to local buffer instead?

};
