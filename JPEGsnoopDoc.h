// JPEGsnoop - JPEG Image Decoder & Analysis Utility
// Copyright (C) 2010 - Calvin Hass
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

// JPEGsnoopDoc.h : interface of the CJPEGsnoopDoc class
//


#pragma once

#include "JfifDecode.h"
#include "ImgDecode.h"
#include "WindowBuf.h"
#include "DocLog.h"

#include "FolderDlg.h"
#include "BatchDlg.h"


#define DOCLOG_MAX_LINES 30000

#define ITER_TEST_MAX 2000 //800

class CJPEGsnoopDoc : public CRichEditDoc
{
protected: // create from serialization only
	CJPEGsnoopDoc();
	DECLARE_DYNCREATE(CJPEGsnoopDoc)

// Attributes
public:

// Operations
public:

// Overrides
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual CRichEditCntrItem* CreateClientItem(REOBJECT* preo) const;

// Implementation
public:
	virtual ~CJPEGsnoopDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	void	SetupView(CRichEditView* pView);

	void	AddLine(CString strTxt);
	void	AddLineHdr(CString strTxt);
	void	AddLineHdrDesc(CString strTxt);
	void	AddLineWarn(CString strTxt);
	void	AddLineErr(CString strTxt);
	void	AddLineGood(CString strTxt);
	int		AppendToLog(CString str, COLORREF color);
	int		InsertQuickLog();

	CStatusBar* GetStatusBar();

	void	recurseBatch(CString szPathName,bool bSubdirs);
	void	doBatchSingle(CString szFileName);


	BOOL ReadLine(CString& strLine, int nLength, LONG lOffset = -1L);
	BOOL AnalyzeOpen();
	void AnalyzeClose();
	void AnalyzeFileDo();
	BOOL AnalyzeFile();

	void	Reset();
	void	Reprocess();


	// Misc helper
//	CString Dec2Bin(unsigned nVal,unsigned nLen);
	unsigned short	Swap16(unsigned short nVal);


public:
	void	batchProcess();



protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnToolsDecode();


public:
	// Input JPEG file
	CFile*		m_pFile;
	ULONGLONG	m_lFileSize;

	unsigned	m_nDisplayRows;

	// The following is a mirror of m_strPathName, but only set during Open
	// This is used only during OnSaveDocument() to ensure that we are
	// not overwriting our input file.
	CString		m_strPathNameOpened;

	// Output log file
	BOOL		m_bFileOpened;		// Have we opened up a file? (i.e. filename def'd)

	unsigned	m_nModeScanDetail;		// Scan detail mode: 0=off, 1=select pt1, 2=select pt2, 3=done


private:
	
	CRichEditView*	m_pView;		// Preserved reference to View

	bool			m_bLogQuickMode;
	CStringArray	m_saLogQuickTxt;
	CUIntArray		m_naLogQuickCol;




public:
	//CAL! Made public temporarily so that I can access from CJPEGsnoopViewImg
	CjfifDecode*	m_pJfifDec;
	CDocLog*		m_pLog;
	CwindowBuf*		m_pWBuf;
	CimgDecode*		m_pImgDec;

public:
	afx_msg void OnFileOffset();
	afx_msg void OnToolsAddcameratodb();
	afx_msg void OnUpdateToolsAddcameratodb(CCmdUI *pCmdUI);
	afx_msg void OnToolsSearchforward();
	afx_msg void OnToolsSearchreverse();
	afx_msg void OnUpdateToolsSearchforward(CCmdUI *pCmdUI);
	afx_msg void OnUpdateToolsSearchreverse(CCmdUI *pCmdUI);
	afx_msg void OnFileReprocess();
	virtual void DeleteContents();
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	afx_msg void OnFileOpenimage();
	void	DoDirectSave(CString strLogName);
	afx_msg void OnFileSaveAs();

	afx_msg void OnPreviewRng(UINT nID);
	afx_msg void OnUpdatePreviewRng(CCmdUI *pCmdUI);
	afx_msg void OnZoomRng(UINT nID);
	afx_msg void OnUpdateZoomRng(CCmdUI * pCmdUI);
	afx_msg void OnToolsSearchexecutablefordqt();
	afx_msg void OnUpdateFileReprocess(CCmdUI *pCmdUI);
	afx_msg void OnUpdateFileSaveAs(CCmdUI *pCmdUI);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	afx_msg void OnToolsExtractembeddedjpeg();
	afx_msg void OnUpdateToolsExtractembeddedjpeg(CCmdUI *pCmdUI);
	afx_msg void OnToolsFileoverlay();
	afx_msg void OnUpdateToolsFileoverlay(CCmdUI *pCmdUI);
	afx_msg void OnToolsLookupmcuoffset();
	afx_msg void OnUpdateToolsLookupmcuoffset(CCmdUI *pCmdUI);
	afx_msg void OnOverlaysMcugrid();
	afx_msg void OnUpdateOverlaysMcugrid(CCmdUI *pCmdUI);



	afx_msg void OnUpdateIndicatorYcc(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIndicatorMcu(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIndicatorFilePos(CCmdUI* pCmdUI);
	afx_msg void OnScansegmentDetaileddecode();
	afx_msg void OnUpdateScansegmentDetaileddecode(CCmdUI *pCmdUI);
	afx_msg void OnToolsExporttiff();
	afx_msg void OnUpdateToolsExporttiff(CCmdUI *pCmdUI);
};


