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
// - JPEGsnoop SDI document class
//
// ==========================================================================


// JPEGsnoopDoc.h : interface of the CJPEGsnoopDoc class
//


#pragma once

#include "JPEGsnoopCore.h"

#include "FolderDlg.h"
#include "BatchDlg.h"


#define DOCLOG_MAX_LINES 30000


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
	virtual BOOL	OnNewDocument();
	virtual void	Serialize(CArchive& ar);
	virtual CRichEditCntrItem* CreateClientItem(REOBJECT* preo) const;

// Implementation
public:
	virtual			~CJPEGsnoopDoc();
#ifdef _DEBUG
	virtual void	AssertValid() const;
	virtual void	Dump(CDumpContext& dc) const;
#endif

	void			SetupView(CRichEditView* pView);

	int				AppendToLog(CString strTxt, COLORREF sColor);
	int				InsertQuickLog();

	CStatusBar*		GetStatusBar();

	void			DoBatchProcess(CString strBatchDir,bool bRecSubdir,bool bExtractAll);

	BOOL			ReadLine(CString& strLine, int nLength, LONG lOffset = -1L);

	void			Reset();
	BOOL			Reprocess();




protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()


private:

	CSnoopConfig*	m_pAppConfig;		// Pointer to application config

	// Input JPEG file
	CFile*			m_pFile;
	ULONGLONG		m_lFileSize;

	// The following is a mirror of m_strPathName, but only set during Open
	// This is used only during OnSaveDocument() to ensure that we are
	// not overwriting our input file.
	CString			m_strPathNameOpened;
	
	CRichEditView*	m_pView;				// Preserved reference to View

public:
	// Allocate the processing core
	// - Public access by CJPEGsnoopViewImg
	CJPEGsnoopCore* m_pCore;

public:

	// Public accessors from CJPEGsnoopApp
	void			J_ImgSrcChanged();

	// Public accessors from CJPEGsnoopViewImg
	void			I_ViewOnDraw(CDC* pDC,CRect rectClient,CPoint ptScrolledPos,CFont* pFont, CSize &szNewScrollSize);
	void			I_GetPreviewPos(unsigned &nX,unsigned &nY);
	void			I_GetPreviewSize(unsigned &nX,unsigned &nY);
	float			I_GetPreviewZoom();


	// Callback functions
	static BYTE		CbWrap_B_Buf(void* pWrapClass,
						unsigned long nNum,bool bBool);
	static void		CbWrap_I_LookupFilePosPix(void* pWrapClass,
						unsigned int nX, unsigned int nY, unsigned int &nByte, unsigned int &nBit);

public:
	void			DoGuiExtractEmbeddedJPEG();
private:
	virtual void	DeleteContents();
	void			RedrawLog();

public:
	// OnOpenDocument() is public for View:OnDropFiles()
	virtual BOOL	OnOpenDocument(LPCTSTR lpszPathName);
private:
	virtual BOOL	OnSaveDocument(LPCTSTR lpszPathName);
	afx_msg void	OnFileOffset();
	afx_msg void	OnToolsDecode();
	afx_msg void	OnToolsAddcameratodb();
	afx_msg void	OnUpdateToolsAddcameratodb(CCmdUI *pCmdUI);
	afx_msg void	OnToolsSearchforward();
	afx_msg void	OnToolsSearchreverse();
	afx_msg void	OnUpdateToolsSearchforward(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateToolsSearchreverse(CCmdUI *pCmdUI);
	afx_msg void	OnFileReprocess();
	afx_msg void	OnFileOpenimage();
	afx_msg void	OnFileSaveAs();
	afx_msg void	OnPreviewRng(UINT nID);
	afx_msg void	OnUpdatePreviewRng(CCmdUI *pCmdUI);
	afx_msg void	OnZoomRng(UINT nID);
	afx_msg void	OnUpdateZoomRng(CCmdUI * pCmdUI);
	afx_msg void	OnToolsSearchexecutablefordqt();
	afx_msg void	OnUpdateFileReprocess(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateFileSaveAs(CCmdUI *pCmdUI);
	afx_msg void	OnToolsExtractembeddedjpeg();
	afx_msg void	OnUpdateToolsExtractembeddedjpeg(CCmdUI *pCmdUI);
	afx_msg void	OnToolsFileoverlay();
	afx_msg void	OnUpdateToolsFileoverlay(CCmdUI *pCmdUI);
	afx_msg void	OnToolsLookupmcuoffset();
	afx_msg void	OnUpdateToolsLookupmcuoffset(CCmdUI *pCmdUI);
	afx_msg void	OnOverlaysMcugrid();
	afx_msg void	OnUpdateOverlaysMcugrid(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateIndicatorYcc(CCmdUI* pCmdUI);
	afx_msg void	OnUpdateIndicatorMcu(CCmdUI* pCmdUI);
	afx_msg void	OnUpdateIndicatorFilePos(CCmdUI* pCmdUI);
	afx_msg void	OnScansegmentDetaileddecode();
	afx_msg void	OnUpdateScansegmentDetaileddecode(CCmdUI *pCmdUI);
	afx_msg void	OnToolsExporttiff();
	afx_msg void	OnUpdateToolsExporttiff(CCmdUI *pCmdUI);
};


