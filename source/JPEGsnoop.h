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
// - Main application code entry-point
//
// ==========================================================================


// JPEGsnoop.h : main header file for the JPEGsnoop application
//
#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

#include "snoop.h"
#include "JPEGsnoopDoc.h"
#include "SnoopConfig.h"

#include "afxwinappex.h"	//xxx

#include "JPEGsnoopCore.h"

// CJPEGsnoopApp:
// See JPEGsnoop.cpp for the implementation of this class
//

// Define global variable for application log
extern CDocLog*	glb_pDocLog;


class CJPEGsnoopApp : public CWinApp
{
public:
	CJPEGsnoopApp();
	~CJPEGsnoopApp();


// Overrides
public:
	virtual BOOL	InitInstance();

// Implementation
	afx_msg void	OnAppAbout();

	DECLARE_MESSAGE_MAP()

private:
	void			MyOnFileOpen();
	void			MyOnFileNew();

	void			DoCmdLineCore();
	void			CmdLineHelp();
	void			CmdLineMessage(CString strMsg);
	void			CmdLineDoneMessage();

	void			CheckUpdates(bool bForceNow);
	bool			CheckUpdatesWww();
	bool			CheckEula();
	CString			RemoveTokenWithSeparators(CString& strText, LPCTSTR szCharSet);
	CString			RemoveTokenFromCharset(CString& strText, LPCTSTR szCharSet);
	CJPEGsnoopDoc*	GetCurDoc();
	void			DocReprocess();
	void			DocImageDirty();
	void			HandleAutoReprocess();

	HINSTANCE		LoadAppLangResourceDLL();	// MFC override

	afx_msg void	OnOptionsDhtexpand();
	afx_msg void	OnOptionsMakernotes();
	afx_msg void	OnOptionsScandump();
	afx_msg void	OnOptionsDecodescan();
	afx_msg void	OnOptionsHistoydump();
	afx_msg void	OnUpdateOptionsDhtexpand(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateOptionsMakernotes(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateOptionsScandump(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateOptionsDecodescan(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateOptionsHistoydump(CCmdUI *pCmdUI);
	afx_msg void	OnOptionsConfiguration();
	afx_msg void	OnOptionsCheckforupdates();
	afx_msg void	OnToolsManagelocaldb();
	afx_msg void	OnOptionsSignaturesearch();
	afx_msg void	OnUpdateOptionsSignaturesearch(CCmdUI *pCmdUI);
	afx_msg void	OnOptionsDecodeac();
	afx_msg void	OnUpdateOptionsDecodeac(CCmdUI *pCmdUI);
	afx_msg void	OnScansegmentDecodeimage();
	afx_msg void	OnScansegmentFullidct();
	afx_msg void	OnScansegmentHistogramy();
	afx_msg void	OnScansegmentDump();
	afx_msg void	OnUpdateScansegmentDecodeimage(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateScansegmentFullidct(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateScansegmentHistogramy(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateScansegmentDump(CCmdUI *pCmdUI);
	afx_msg void	OnScansegmentNoidct();
	afx_msg void	OnUpdateScansegmentNoidct(CCmdUI *pCmdUI);
	afx_msg void	OnScansegmentHistogram();
	afx_msg void	OnUpdateScansegmentHistogram(CCmdUI *pCmdUI);
	afx_msg void	OnOptionsHideuknownexiftags();
	afx_msg void	OnUpdateOptionsHideuknownexiftags(CCmdUI *pCmdUI);
	afx_msg void	OnFileBatchprocess();

public:
	// Main config options
	CSnoopConfig*	m_pAppConfig;

	// Needs to be accessed by JfifDec
	CDbSigs*		m_pDbSigs;

private:
	bool			m_bFatal;		// Fatal error occurred (e.g. mem alloc)

public:
	afx_msg void OnOptionsRelaxedparsing();
	afx_msg void OnUpdateOptionsRelaxedparsing(CCmdUI *pCmdUI);
};

extern CJPEGsnoopApp theApp;