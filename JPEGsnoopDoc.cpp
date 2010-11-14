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

// JPEGsnoopDoc.cpp : implementation of the CJPEGsnoopDoc class
//

#include "stdafx.h"
#include "JPEGsnoop.h"

#include "JPEGsnoopDoc.h"
#include "CntrItem.h"
#include ".\jpegsnoopdoc.h"

#include "WindowBuf.h"

#include "OffsetDlg.h"
#include "DbSubmitDlg.h"
#include "NoteDlg.h"
#include "OverlayBufDlg.h"
#include "LookupDlg.h"
#include "ExportDlg.h"
#include "DecodeDetailDlg.h"
#include "ExportTiffDlg.h"

//#include "OperationDlg.h"

#include "General.h"

#include "FileTiff.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CJPEGsnoopDoc

IMPLEMENT_DYNCREATE(CJPEGsnoopDoc, CRichEditDoc)

BEGIN_MESSAGE_MAP(CJPEGsnoopDoc, CRichEditDoc)
	// Enable default OLE container implementation
	ON_COMMAND(ID_OLE_EDIT_LINKS, CRichEditDoc::OnEditLinks)
	ON_UPDATE_COMMAND_UI(ID_OLE_EDIT_LINKS, CRichEditDoc::OnUpdateEditLinksMenu)
	ON_UPDATE_COMMAND_UI_RANGE(ID_OLE_VERB_FIRST, ID_OLE_VERB_LAST, CRichEditDoc::OnUpdateObjectVerbMenu)

	ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
	//ON_COMMAND(ID_FILE_OPENIMAGE, OnFileOpenimage)
	ON_COMMAND(ID_FILE_OFFSET, OnFileOffset)
	ON_COMMAND(ID_FILE_REPROCESS, OnFileReprocess)
	ON_COMMAND(ID_TOOLS_ADDCAMERATODB, OnToolsAddcameratodb)
	ON_COMMAND(ID_TOOLS_SEARCHFORWARD, OnToolsSearchforward)
	ON_COMMAND(ID_TOOLS_SEARCHREVERSE, OnToolsSearchreverse)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_ADDCAMERATODB, OnUpdateToolsAddcameratodb)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_SEARCHFORWARD, OnUpdateToolsSearchforward)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_SEARCHREVERSE, OnUpdateToolsSearchreverse)

	ON_COMMAND_RANGE(ID_PREVIEW_RGB,ID_PREVIEW_CR,OnPreviewRng)
	ON_UPDATE_COMMAND_UI_RANGE(ID_PREVIEW_RGB,ID_PREVIEW_CR,OnUpdatePreviewRng)

	ON_COMMAND_RANGE(ID_IMAGEZOOM_ZOOMIN,ID_IMAGEZOOM_800,OnZoomRng)
	ON_UPDATE_COMMAND_UI_RANGE(ID_IMAGEZOOM_ZOOMIN,ID_IMAGEZOOM_800,OnUpdateZoomRng)


	ON_COMMAND(ID_TOOLS_SEARCHEXECUTABLEFORDQT, OnToolsSearchexecutablefordqt)
	ON_UPDATE_COMMAND_UI(ID_FILE_REPROCESS, OnUpdateFileReprocess)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_AS, OnUpdateFileSaveAs)
	ON_COMMAND(ID_TOOLS_EXTRACTEMBEDDEDJPEG, OnToolsExtractembeddedjpeg)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_EXTRACTEMBEDDEDJPEG, OnUpdateToolsExtractembeddedjpeg)
	ON_COMMAND(ID_TOOLS_FILEOVERLAY, OnToolsFileoverlay)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_FILEOVERLAY, OnUpdateToolsFileoverlay)
	ON_COMMAND(ID_TOOLS_LOOKUPMCUOFFSET, OnToolsLookupmcuoffset)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_LOOKUPMCUOFFSET, OnUpdateToolsLookupmcuoffset)
	ON_COMMAND(ID_OVERLAYS_MCUGRID, OnOverlaysMcugrid)
	ON_UPDATE_COMMAND_UI(ID_OVERLAYS_MCUGRID, OnUpdateOverlaysMcugrid)



	ON_UPDATE_COMMAND_UI(ID_INDICATOR_YCC, OnUpdateIndicatorYcc)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_MCU, OnUpdateIndicatorMcu)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_FILEPOS, OnUpdateIndicatorFilePos)
	ON_COMMAND(ID_SCANSEGMENT_DETAILEDDECODE, OnScansegmentDetaileddecode)
	ON_UPDATE_COMMAND_UI(ID_SCANSEGMENT_DETAILEDDECODE, OnUpdateScansegmentDetaileddecode)
	ON_COMMAND(ID_TOOLS_EXPORTTIFF, OnToolsExporttiff)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_EXPORTTIFF, OnUpdateToolsExporttiff)
END_MESSAGE_MAP()


// CJPEGsnoopDoc construction/destruction

CJPEGsnoopDoc::CJPEGsnoopDoc()
: m_pView(NULL)
{
	m_pLog = new CDocLog(this);
	if (!m_pLog) {
		AfxMessageBox("ERROR: Not enough memory for Document");
		exit(1);
	}

	m_pWBuf = new CwindowBuf();
	if (!m_pWBuf) {
		AfxMessageBox("ERROR: Not enough memory for Document");
		exit(1);
	}

	// Allocate the JPEG decoder
	m_pImgDec = new CimgDecode(m_pLog,m_pWBuf);
	if (!m_pWBuf) {
		AfxMessageBox("ERROR: Not enough memory for Image Decoder");
		exit(1);
	}
	m_pJfifDec = new CjfifDecode(m_pLog,m_pWBuf,m_pImgDec);
	if (!m_pWBuf) {
		AfxMessageBox("ERROR: Not enough memory for JFIF Decoder");
		exit(1);
	}


    // Reset all members
	Reset();

	// Start in quick mode
#ifdef QUICKLOG
	m_bLogQuickMode = true;
#else
	m_bLogQuickMode = false;
#endif


}

// Cleanup all of the allocated classes
CJPEGsnoopDoc::~CJPEGsnoopDoc()
{
	if (m_pJfifDec != NULL)
	{
		delete m_pJfifDec;
		m_pJfifDec = NULL;
	}

	if (m_pLog != NULL) {
		delete m_pLog;
		m_pLog = NULL;
	}

	if (m_pWBuf != NULL) {
		delete m_pWBuf;
		m_pWBuf = NULL;
	}

	if (m_pImgDec != NULL) {
		delete m_pImgDec;
		m_pImgDec = NULL;
	}



}

// Reset is only called by the constructor, New and Open
void CJPEGsnoopDoc::Reset()
{
	// Reset all members
	m_pFile = NULL;
	m_lFileSize = 0L;

	// No log data available until we open & process a file
	m_bFileOpened = FALSE;
	m_strPathNameOpened = "";

	m_nModeScanDetail = 0;

	// Indicate to JFIF process() that document has changed
	// and that the scan decode needs to be redone if it
	// is to be displayed.
	m_pJfifDec->ImgSrcChanged();

	// Clean up the quick log
	m_saLogQuickTxt.RemoveAll();
	m_naLogQuickCol.RemoveAll();

}

BOOL CJPEGsnoopDoc::OnNewDocument()
{
	if (!CRichEditDoc::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)
	Reset();

	return TRUE;
}
CRichEditCntrItem* CJPEGsnoopDoc::CreateClientItem(REOBJECT* preo) const
{
	return new CJPEGsnoopCntrItem(preo, const_cast<CJPEGsnoopDoc*>(this));
}




// CJPEGsnoopDoc serialization
//CAL! This is called during the standard OnOpenDocument() and presumably
//     OnSaveDocument(). Currently it is not implemented.
void CJPEGsnoopDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}

	// Calling the base class CRichEditDoc enables serialization
	//  of the container document's COleClientItem objects.
	// TODO: set CRichEditDoc::m_bRTF = FALSE if you are serializing as text
	CRichEditDoc::Serialize(ar);
}


// CJPEGsnoopDoc diagnostics

#ifdef _DEBUG
void CJPEGsnoopDoc::AssertValid() const
{
	CRichEditDoc::AssertValid();
}

void CJPEGsnoopDoc::Dump(CDumpContext& dc) const
{
	CRichEditDoc::Dump(dc);
}
#endif //_DEBUG


// CJPEGsnoopDoc commands


// Add a line to the end of the log
int CJPEGsnoopDoc::AppendToLog(CString str, COLORREF color)
{
	if (m_bLogQuickMode) {

		// Don't exceed a realistic maximum!
		unsigned numLines = m_saLogQuickTxt.GetCount();
		if (numLines == DOCLOG_MAX_LINES) {
			m_saLogQuickTxt.Add("*** TOO MANY LINES IN REPORT -- TRUNCATING ***");
			m_naLogQuickCol.Add((unsigned)color);
			m_nDisplayRows++;
			return 0;
		} else if (numLines > DOCLOG_MAX_LINES) {
			return 0;
		}

		m_saLogQuickTxt.Add(str);
		m_naLogQuickCol.Add((unsigned)color);


		return 0;
	}

	ASSERT(m_pView);
	if (!m_pView) return -1;
	CRichEditCtrl* pCtrl = &m_pView->GetRichEditCtrl();
	ASSERT(pCtrl);
	if (!pCtrl) return -1;
	int nOldLines = 0, nNewLines = 0, nScroll = 0;
	long nInsertionPoint = 0;
	CHARFORMAT cf;

	// Save number of lines before insertion of new text
	nOldLines = pCtrl->GetLineCount();
	// Initialize character format structure
	cf.cbSize = sizeof(CHARFORMAT);
	cf.dwMask = CFM_COLOR;
	cf.dwEffects = 0; // To disable CFE_AUTOCOLOR
	cf.crTextColor = color;
	// Set insertion point to end of text
	nInsertionPoint = pCtrl->GetWindowTextLength();
	pCtrl->SetSel(nInsertionPoint, -1);
	// Set the character format
	pCtrl->SetSelectionCharFormat(cf);
	// Replace selection. Because we have nothing 
	// selected, this will simply insert
	// the string at the current caret position.
	pCtrl->ReplaceSel(str);
	// Get new line count
	nNewLines = pCtrl->GetLineCount();
	// Scroll by the number of lines just inserted
	nScroll = nNewLines - nOldLines;
	//pCtrl->LineScroll(nScroll);

	// **********************************************************
	// Very important that we mark the RichEdit log as not
	// being modified. Otherwise we will be asked to save
	// changes... If the user hit Yes, they may overwrite their
	// image file with the log file!
	SetModifiedFlag(false); // Mark as not modified
	// **********************************************************

#ifndef QUICKLOG
	pCtrl->RedrawWindow();
#endif

	return 0;
}

int CJPEGsnoopDoc::InsertQuickLog()
{
	if (!m_bLogQuickMode) {
		return 0;
	}

	ASSERT(m_pView);
	if (!m_pView) return -1;
	CRichEditCtrl* pCtrl = &m_pView->GetRichEditCtrl();
	ASSERT(pCtrl);
	if (!pCtrl) return -1;
	int nOldLines = 0, nNewLines = 0, nScroll = 0;
	long nInsertionPoint = 0;


	CHARFORMAT cf;

	// Save number of lines before insertion of new text
	nOldLines = pCtrl->GetLineCount();


	// Set insertion point to end of text
	nInsertionPoint = pCtrl->GetWindowTextLength();
	pCtrl->SetSel(nInsertionPoint, -1);


	// Replace selection. Because we have nothing 
	// selected, this will simply insert
	// the string at the current caret position.


	pCtrl->SetRedraw(false);
	unsigned nQuickLines = m_saLogQuickTxt.GetCount();
	COLORREF nCurCol = RGB(0,0,0);
	COLORREF nLastCol = RGB(255,255,255);

	for (unsigned ind=0;ind<nQuickLines;ind++)
	{
		nCurCol = m_naLogQuickCol.GetAt(ind);
		if (nCurCol != nLastCol) {
			// Initialize character format structure
			cf.cbSize = sizeof(CHARFORMAT);
			cf.dwMask = CFM_COLOR;
			cf.dwEffects = 0; // To disable CFE_AUTOCOLOR
			cf.crTextColor = nCurCol;
			// Set the character format
			pCtrl->SetSelectionCharFormat(cf);
		}
		pCtrl->ReplaceSel(m_saLogQuickTxt.GetAt(ind));

	}
	pCtrl->SetRedraw(true);
	pCtrl->RedrawWindow();
 
	// Empty the quick log since we've used it now
	m_saLogQuickTxt.RemoveAll();
	m_naLogQuickCol.RemoveAll();
  
	// Get new line count
	nNewLines = pCtrl->GetLineCount();
	// Scroll by the number of lines just inserted
	nScroll = nNewLines - nOldLines;
	//pCtrl->LineScroll(nScroll);

	// Scroll to the top of the window
	pCtrl->LineScroll(-nNewLines);

	// **********************************************************
	// Very important that we mark the RichEdit log as not
	// being modified. Otherwise we will be asked to save
	// changes... If the user hit Yes, they may overwrite their
	// image file with the log file!
	SetModifiedFlag(false); // Mark as not modified
	// **********************************************************

	return 0;
}

// Save the view pointer (from View init)
void CJPEGsnoopDoc::SetupView(CRichEditView* pView)
{
	m_pView = pView;
}

void CJPEGsnoopDoc::AddLine(CString strTxt)
{
	AppendToLog(strTxt+"\n",RGB(1, 1, 1));
}

void CJPEGsnoopDoc::AddLineHdr(CString strTxt)
{
	AppendToLog(strTxt+"\n",RGB(1, 1, 255));
}

void CJPEGsnoopDoc::AddLineHdrDesc(CString strTxt)
{
	AppendToLog(strTxt+"\n",RGB(32, 32, 255));
}

void CJPEGsnoopDoc::AddLineWarn(CString strTxt)
{
	AppendToLog(strTxt+"\n",RGB(128, 1, 1));
}

void CJPEGsnoopDoc::AddLineErr(CString strTxt)
{
	AppendToLog(strTxt+"\n",RGB(255, 1, 1));
}

void CJPEGsnoopDoc::AddLineGood(CString strTxt)
{
	AppendToLog(strTxt+"\n",RGB(16, 128, 16));
}

// ***************************************

CStatusBar* CJPEGsnoopDoc::GetStatusBar()
{
	CWnd *pMainWnd = AfxGetMainWnd();
	if (!pMainWnd) return NULL;

	if (pMainWnd->IsKindOf(RUNTIME_CLASS(CFrameWnd)))
	{
		CWnd* pMessageBar = ((CFrameWnd*)pMainWnd)->GetMessageBar();
		return DYNAMIC_DOWNCAST(CStatusBar,pMessageBar);
	}
	else
		return DYNAMIC_DOWNCAST(CStatusBar,pMainWnd->GetDescendantWindow(AFX_IDW_STATUS_BAR));
}

BOOL CJPEGsnoopDoc::AnalyzeOpen()
{
	ASSERT(m_strPathName != "");
	if (m_strPathName == "") { AfxMessageBox("ERROR: AnalyzeOpen() but m_strPathName empty"); }

	// Clean up if a file is already open
	if (m_pFile != NULL)
	{
		// Mark previous buffer as closed
		m_pWBuf->BufFileUnset();
		m_pFile->Close();
		delete m_pFile;
		m_pFile = NULL;
		m_lFileSize = 0L;
	}

	try
	{
		// Open specified file
		// Added in shareDenyNone as this apparently helps resolve some people's troubles
		// with an error showing: Couldn't open file "Sharing Violation"
		m_pFile = new CFile(m_strPathName, CFile::modeRead | CFile::typeBinary | CFile::shareDenyNone);
	}
	catch (CFileException* e)
	{
		char msg[512];
		CString strError;
		e->GetErrorMessage(msg,sizeof(msg));
		// Note: msg includes m_strPathName
		strError.Format(_T("ERROR: Couldn't open file: [%s]"),msg);
		AfxMessageBox(strError);
		m_pFile = NULL;

		return FALSE;

	}

	// Set the file size variable
	m_lFileSize = m_pFile->GetLength();

	// Don't attempt to load buffer with zero length file!
	if (m_lFileSize==0) {
		return TRUE;
	}

	// Open up the buffer
	m_pWBuf->BufFileSet(m_pFile);
	m_pWBuf->BufLoadWindow(0);

	return TRUE;

}
void CJPEGsnoopDoc::AnalyzeClose()
{
	// Close the buffer window
	m_pWBuf->BufFileUnset();

	// Now that we've finished parsing the file, close it!
	if (m_pFile != NULL)
	{
		m_pFile->Close();
		delete m_pFile;
		m_pFile = NULL;
	}
	
	// Mark the doc as clean so that we don't get questioned to save anytime
	// we change the file or quit.
	//SetModifiedFlag(false);

}

void CJPEGsnoopDoc::AnalyzeFileDo()
{
	//CAL! - start

	// Get the status bar and configure the decoder to link up to it
	CStatusBar* pStatBar;
	pStatBar = GetStatusBar();

	// Hook up the status bar
	m_pJfifDec->SetStatusBar(pStatBar);
	m_pImgDec->SetStatusBar(pStatBar);

	// Show coach message once
	if (theApp.m_pAppConfig->bDecodeScanImg &&
		theApp.m_pAppConfig->bCoachDecodeIdct) {
		// Show the coaching dialog
		CNoteDlg dlg;
		if (theApp.m_pAppConfig->bDecodeScanImgAc) {
			dlg.strMsg = COACH_DECODE_IDCT_AC;
		} else {
			dlg.strMsg = COACH_DECODE_IDCT_DC;
		}
		dlg.DoModal();
		theApp.m_pAppConfig->bCoachDecodeIdct = !dlg.bCoachOff;
		theApp.m_pAppConfig->Dirty();
	}


	// Start in Quick mode
#ifdef QUICKLOG
	m_bLogQuickMode = true;
#else
	m_bLogQuickMode = false;
#endif

	CString tmpStr;
	AddLine(_T(""));
	tmpStr.Format(_T("JPEGsnoop %s by Calvin Hass"),VERSION_STR);
	AddLine(tmpStr);
	AddLine(_T("  http://www.impulseadventure.com/photo/"));
	AddLine(_T("  -------------------------------------"));
	AddLine(_T(""));
	tmpStr.Format(_T("  Filename: [%s]"),m_strPathName);
	AddLine(tmpStr);
	tmpStr.Format(_T("  Filesize: [%lu] Bytes"),m_lFileSize);
	AddLine(tmpStr);
	AddLine(_T(""));



	// Perform the actual decoding
	if (m_lFileSize>(1<<31)) {
		AddLineErr(_T("ERROR: Files larger than 2GB not supported in this version of JPEGsnoop."));
	} else if (m_lFileSize == 0) {
		AddLineErr(_T("ERROR: File length is zero, no decoding done."));
	} else {
		m_pJfifDec->process(m_pFile);
	}

	// In case we are in quick log mode, insert everything at once
	InsertQuickLog();

	// Now get out of quick mode, because user may issue menu commands
	// where we will want to see interactive output
	m_bLogQuickMode = false;

	// Finished the decoding
	

	// Now force a redraw (especially for Img View window)
	// Force a redraw so that we can see animated previews (e.g. AVI)
	// when holding down Fwd/Rev Search hotkey
	
	
	POSITION pos = GetFirstViewPosition();
	if (pos != NULL) {
		CView* pFirstView = GetNextView( pos );
		pFirstView->Invalidate();
		pFirstView->RedrawWindow(NULL,0,RDW_UPDATENOW);
	}
	if (pos != NULL) {
		CView* pSecondView = GetNextView( pos );
		pSecondView->Invalidate();
		pSecondView->RedrawWindow(NULL,0,RDW_UPDATENOW);
	}
	

	/*
   POSITION pos = GetFirstViewPosition();
   while (pos != NULL)
   {
      CView* pView = GetNextView(pos);
      //pView->UpdateWindow();
	  pView->RedrawWindow(NULL,0,RDW_UPDATENOW);
   }  
   */

}

BOOL CJPEGsnoopDoc::AnalyzeFile()
{
	BOOL retval;

	// Assumes that we have set up the member vars already
	// Perform the actual processing. We have this in a routine
	// so that we can quickly recalculate the log file again
	// if an option changes.

	retval = AnalyzeOpen();
	if (retval) {
		// Only now that we have successfully opened the document
		// should be mark the flag as such. This flag is used by
		// other menu items to know whether or not the file is ready.
		m_bFileOpened = TRUE;
		AnalyzeFileDo();
	}
	AnalyzeClose();

	// In the last part of AnalyzeClose(), we mark the file
	// as not modified, so that we don't get prompted to save.

	return retval;

}


BOOL CJPEGsnoopDoc::ReadLine(CString& strLine,
							 int nLength,
							 LONG lOffset /* = -1L */)
{
	ULONGLONG lPosition;

	if (lOffset != -1L)
		lPosition = m_pFile->Seek(lOffset,CFile::begin);
	else
		lPosition = m_pFile->GetPosition();

	if (lPosition == -1L)
	{
		TRACE2("CJPEGsnoopDoc::ReadLine returns FALSE Seek"
			"(%8.8lX, %8.8lX)\n",
			lOffset, lPosition);
		return FALSE;
	}

	BYTE* pszBuffer = new BYTE[nLength];
	if (!pszBuffer) {
		AfxMessageBox("ERROR: Not enough memory for Document ReadLine");
		exit(1);
	}

	int nReturned = m_pFile->Read(pszBuffer, nLength);

	if (nReturned <= 0)
	{
		TRACE2("CJPEGsnoopDoc::ReadLine returns FALSE Read"
			"(%d, %d)\n",
			nLength,
			nReturned);
		delete pszBuffer;
		return FALSE;
	}

	CString strTemp;
	CString strCharsIn;

	strTemp.Format(_T("%8.8lX - "), lPosition);
	strLine = strTemp;

	for (int nIndex = 0; nIndex < nReturned; nIndex++)
	{
		if (nIndex == 0)
			strTemp.Format(_T("%2.2X"), pszBuffer[nIndex]);
		else if (nIndex %16 == 0)
			strTemp.Format(_T("=%2.2X"), pszBuffer[nIndex]);
		else if (nIndex %8 == 0)
			strTemp.Format(_T("-%2.2X"), pszBuffer[nIndex]);
		else
			strTemp.Format(_T(" %2.2X"), pszBuffer[nIndex]);

		if (_istprint(pszBuffer[nIndex]))
			strCharsIn += pszBuffer[nIndex];
		else
			strCharsIn += _T('.');
		strLine += strTemp;	
	}

	if (nReturned < nLength)
	{
		CString strPadding(_T(' '),3*(nLength-nReturned));
		strLine += strPadding;
	}

	strLine += _T("  ");
	strLine += strCharsIn;

	delete pszBuffer;

	return TRUE;
}


// --------------------------------------------------------------------
// --- START OF BATCH PROCESSING
// --------------------------------------------------------------------




// The root of the batch recursion. It simply jumps into the
// recursion loop but initializes the search to start with an
// empty search result.
void CJPEGsnoopDoc::batchProcess()
{
	CFolderDialog	myFolderDlg(NULL);
	CString			strDir;
	bool			bSubdirs;

	LPCITEMIDLIST	myItemIdList;
	myItemIdList = myFolderDlg.BrowseForFolder("Select folder to process",0,0,false);
	strDir = myFolderDlg.GetPathName(myItemIdList);
	
	// If the user did not select CANCEL, then proceed with
	// the batch operation.
	if (strDir != "") {

		// Bring up dialog to select subdir recursion
		CBatchDlg myBatchDlg;
		myBatchDlg.m_bProcessSubdir = false;
		myBatchDlg.m_strDir = strDir;
		if (myBatchDlg.DoModal() == IDOK) {

			// Fetch the settings from the dialog
			bSubdirs = myBatchDlg.m_bProcessSubdir;

			// Indicate long operation ahead!
			CWaitCursor wc;

			// TODO:
			// - What is the best way to provide a "Cancel Dialog" for a
			//   recursive operation? I can easily create the cancel / progress
			//   dialog with operations that have can be single-stepped, but
			//   not ones that require accumulation on the stack.
			// - For now, just leave as-is.

			// Example code that I can use for single-stepping the operation:
			//
			// // === START
			// COperationDlg LengthyOp(this);
			// LengthyOp.SetFunctions( PrepareOperation, NextIteration, GetProgress );
			//
			// // Until-done based
			// BOOL bOk = LengthyOp.RunUntilDone( true );
			// // === END


			// Start the batch operation
			recurseBatch(strDir,bSubdirs);
			

			// TODO: Clean up after last log output

			// Alert the user that we are done
			AfxMessageBox("Batch Processing Complete!");
		}
	}
}



// Recursive routine that searches for files and folders
// Used in batch file processing mode
// INPUT:  szPathName    = Directory path for current search
// INPUT:  bSubdirs      = Are we recursing down into subdirectories?
void CJPEGsnoopDoc::recurseBatch(CString szPathName,bool bSubdirs)
{
	// The following code snippet is based on MSDN code:
	// http://msdn.microsoft.com/en-us/library/scx99850%28VS.80%29.aspx

	CFileFind finder;

	// build a string with wildcards
	CString	strWildcard(szPathName);
	strWildcard	+= _T("\\*.*");

	// start working for files
	BOOL bWorking =	finder.FindFile(strWildcard);

	while (bWorking)
	{
		bWorking = finder.FindNextFile();

		// skip	. and .. files;	otherwise, we'd
		// recur infinitely!

		if (finder.IsDots())
			continue;

		CString	strPath	= finder.GetFilePath();

		// if it's a directory,	recursively	search it
		if (finder.IsDirectory())
		{
			if (bSubdirs) {
				recurseBatch(strPath,bSubdirs);
			}
		} else {
			// GetFilePath() includes both the path	& filename
			// when	called on a	file entry,	so there is	no need
			// to specifically call	GetFileName()
			//		 CString strFname =	finder.GetFileName();

			// Perform the actual processing on the file
			doBatchSingle(strPath);
		}
	}

	finder.Close();
}

// Perform processing on file selected by the batch recursion
// process recurseBatch().
// PRECONDITION: fName is a file (ie. not directory)
void CJPEGsnoopDoc::doBatchSingle(CString fName)
{
	CString		fNameExt;
	bool		bDoSubmit = false;
	unsigned	ind;
	CString		fNameOnly;
	CString		fNameLog;

	// Extract the filename (without extension) from the full pathname
	fNameOnly = fName.Mid(fName.ReverseFind('\\')+1);
	ind = fNameOnly.ReverseFind('.');
	fNameOnly = fNameOnly.Mid(0,ind);


	// Extract the file extension
	ind = fName.ReverseFind('.');
	fNameExt = fName.Mid(ind);
	fNameExt.MakeLower();

	// Only process files that have an extension that implies JPEG
	// TODO: Should enable the user to provide a list of extensions
	// or even disable check altogether.
	if ((fNameExt == ".jpg") || (fNameExt == ".jpeg")) {

		// Open the file & begin normal processing
		OnOpenDocument(fName);

		// Now that we have completed processing, optionally create
		// a log file with the report results. Note that this call
		// automatically overwrites the previous log filename.
		//
		// TODO: Add an option in the batch dialog to specify
		//       action to take if logfile already exists.
		//
		// ==== WARNING! WARNING! WARNING! ====
		//   It is *essential* that this Append function work properly
		//   as it is used to generate the log filename from the
		//   image filename. Since we may be automatically overwriting
		//   the logfile, it is imperative that we ensure that there is
		//   no chance that the log filename happens to be the original
		//   JPEG filename!
		//
		//   This is perhaps being a bit paranoid, but I feel it is
		//   worth adding some additional code here to ensure that 
		//   this doesn't happen.
		// ==== WARNING! WARNING! WARNING! ====

		fNameLog = fName;
		fNameLog.Append(".txt");

		// Now perform the paranoid checks as described above!
		// - Is the last 4 characters of the fName ".txt"?
		if (fNameLog.Right(4) != ".txt") {
			// Report error message and skip logfile save
			AfxMessageBox("ERROR: Internal error #10100");
			return;
		}
		// - Is the fNameLog different from the input filename (fName)?
		if (fNameLog == fName) {
			// Report error message and skip logfile save
			AfxMessageBox("ERROR: Internal error #10101");
			return;
		}

		// Guess the filename is safe, proceed with save
		DoDirectSave(fNameLog);

		// Now submit entry to database!
#ifdef BATCH_DO_DBSUBMIT
		m_pJfifDec->m_strFileName = fNameOnly; // BUG? Should this be "fName"?
		bDoSubmit = m_pJfifDec->CompareSignature(true);
		if (bDoSubmit) {
			m_pJfifDec->PrepareSendSubmit(m_pJfifDec->m_strImgQualExif,m_pJfifDec->m_nDbReqSuggest,"","BATCH");
		}
#endif

	}
}



// --------------------------------------------------------------------
// --- END OF BATCH PROCESSING
// --------------------------------------------------------------------



void CJPEGsnoopDoc::OnFileOffset()
{
	COffsetDlg offsetDlg;
	CString dlgStr;

	// This function assumes that we've previously opened a file!!!
	// Otherwise, there isn't much point in setting the offset value
	// since it gets reset to 0 when we open a new file manually!

	if (m_bFileOpened) {
		AnalyzeOpen();
	}

	offsetDlg.SetOffset(theApp.m_pAppConfig->nPosStart);
	if (offsetDlg.DoModal() == IDOK) {
		theApp.m_pAppConfig->nPosStart = offsetDlg.m_nOffsetVal;
		m_pJfifDec->ImgSrcChanged();

		Reprocess();

	} else {
	}

	if (m_bFileOpened) {
		AnalyzeClose();
	}

}

void CJPEGsnoopDoc::OnToolsAddcameratodb()
{
	CDbSubmitDlg	submitDlg;
	unsigned		nUserSrcPre;
	unsigned		nUserSrc;
	CString			strUserSoftware;
	CString			strQual;
	CString			strUserNotes;

	if (m_pJfifDec->m_strHash == "NONE")
	{
		// No valid signature, can't submit!
		AfxMessageBox("No valid signature could be created, so DB submit is temporarily disabled");
		return;
	}

	submitDlg.m_strExifMake = m_pJfifDec->m_strImgExifMake;
	submitDlg.m_strExifModel = m_pJfifDec->m_strImgExifModel;
	submitDlg.m_strExifSoftware = m_pJfifDec->m_strSoftware;
	submitDlg.m_strUserSoftware = m_pJfifDec->m_strSoftware;
	submitDlg.m_strSig = m_pJfifDec->m_strHash; // Only show unrotated sig
	submitDlg.m_strQual = m_pJfifDec->m_strImgQualExif;

	// Does the image appear to be edited? If so, warn
	// the user before submission...

	if (m_pJfifDec->m_nDbReqSuggest == DB_ADD_SUGGEST_CAM) {
		submitDlg.m_nSource = 0; // Camera
	} else if (m_pJfifDec->m_nDbReqSuggest == DB_ADD_SUGGEST_SW) {
		submitDlg.m_nSource = 1; // Software
	} else {
		submitDlg.m_nSource = 2; // I don't know!
	}


	if (submitDlg.DoModal() == IDOK) {
		strQual = submitDlg.m_strQual;
		nUserSrcPre = submitDlg.m_nSource;
		strUserSoftware = submitDlg.m_strUserSoftware;
		strUserNotes = submitDlg.m_strNotes;

		switch (nUserSrcPre) {
			case 0:
				nUserSrc = ENUM_SOURCE_CAM;
				break;
			case 1:
				nUserSrc = ENUM_SOURCE_SW;
				break;
			case 2:
				nUserSrc = ENUM_SOURCE_UNSURE;
				break;
			default:
				nUserSrc = ENUM_SOURCE_UNSURE;
				break;

		}

		m_pJfifDec->PrepareSendSubmit(strQual,nUserSrc,strUserSoftware,strUserNotes);
	}
}

void CJPEGsnoopDoc::OnUpdateToolsAddcameratodb(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_bFileOpened);
}

void CJPEGsnoopDoc::OnToolsSearchforward()
{
	// Search for start:
	unsigned long search_pos = 0;
	unsigned long start_pos;
	bool search_result;
	CString tmpStr;

	//CAL! NOTE: m_bFileOpened doesn't actually refer to underlying file....
	// not reliable!

	// If file got renamed or moved, AnalyzeOpen() will return false
	if (AnalyzeOpen() == false) {
		return;
	}

	start_pos = theApp.m_pAppConfig->nPosStart;
	m_pJfifDec->ImgSrcChanged();

	search_result = m_pWBuf->BufSearch(start_pos,0xFFD8FF,3,true,search_pos);
	if (search_result) {
		theApp.m_pAppConfig->nPosStart = search_pos;
		Reprocess();
	} else {
		AfxMessageBox("No SOI Marker found in Forward search");
	}
	AnalyzeClose();

}

void CJPEGsnoopDoc::OnToolsSearchreverse()
{
	// Search for start:

	unsigned long search_pos = 0;
	unsigned long start_pos;
	bool search_result;
	CString tmpStr;

	// If file got renamed or moved, AnalyzeOpen() will return false
	if (AnalyzeOpen() == false) {
		return;
	}

	start_pos = theApp.m_pAppConfig->nPosStart;
	m_pJfifDec->ImgSrcChanged();

	// Don't attempt to search past start of file
	if (start_pos > 0) {
		search_result = m_pWBuf->BufSearch(start_pos,0xFFD8FF,3,false,search_pos);
		if (search_result) {
			theApp.m_pAppConfig->nPosStart = search_pos;
			Reprocess();
		} else {
			theApp.m_pAppConfig->nPosStart = 0;
			AfxMessageBox("No SOI Marker found in Reverse search");
			Reprocess();
		}
	}
	AnalyzeClose();

}

void CJPEGsnoopDoc::OnUpdateToolsSearchforward(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_bFileOpened);
}

void CJPEGsnoopDoc::OnUpdateToolsSearchreverse(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_bFileOpened);
}

void CJPEGsnoopDoc::Reprocess()
{
	// Reprocess file
	if (m_bFileOpened) {
		// Clean out document first (especially buffer)
		DeleteContents();
		// Now we can reprocess the file
		AnalyzeFile();
	}

	// Now try to update all views. This is particularly important
	// for View 2 (ScrollView) as it does not automatically invalidate
	UpdateAllViews(NULL);
}



// Reprocess and update all views
void CJPEGsnoopDoc::OnFileReprocess()
{
	m_pJfifDec->ImgSrcChanged();
	Reprocess();
}

//CAL! Temporarily added this just in case I want to do some
// other clearing.
void CJPEGsnoopDoc::DeleteContents()
{
	// TODO: Add your specialized code here and/or call the base class
	
	CRichEditDoc::DeleteContents();
}

// This is called from the File->Open... as well as
// during MRU list invokation:
//   CJPEGsnoopDoc::OnOpenDocument()
//   CSingleDocTemplate::OnOpenDocumentFile()
//   CDocManager::OnOpenDocumentFile()
//   CWinApp::OpenDocumentFile()
//   CWinApp::OnOpenRecentFile()
//   ...
//
// Want to avoid automatic StreamIn() and serialization that
// will attempt to be done on the JPEG file into the RichEditCtrl.
// However, these probably fail (not RTF) or else relies on the
// serialize function here to do something.
BOOL CJPEGsnoopDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	// Before entering this function, the default file dialog
	// has already been presented, so we can't override anything.

	//CAL! Not sure if I want to use the inherited OnOpenDocument() call
	//     because it will try to interpret the file as well
	//     as call the DeleteContents() on the RichEdit.
	// FIXME
	if (!CRichEditDoc::OnOpenDocument(lpszPathName))
		return FALSE;

	// Reset internal state
	Reset();

	// Fully reset the Img decoder state
	m_pImgDec->ResetNewFile();



	SetPathName(lpszPathName,true);	// Sets m_strPathName, validate len, sets window title
	// Save mirror of m_strPathName just so that we can add safety
	// check during OnSaveDocument(). By the time OnSaveDocument() is called,
	// the original m_strPathName has already been modified to indicate the
	// log/output filename.
	m_strPathNameOpened = m_strPathName;

	BOOL status;

	// Reset the offset pointer!
	theApp.m_pAppConfig->nPosStart = 0;
	
	status = AnalyzeFile();


	// Now allow automatic saving if cmdline specified it
	if (theApp.m_pAppConfig->cmdline_output) {
		DoDirectSave(theApp.m_pAppConfig->cmdline_output_fname);
	}

	return status;
}


// Force a Save in text only format, with no prompting for filename
// WARNING: This routine forces an overwrite of the logfile.
void CJPEGsnoopDoc::DoDirectSave(CString strLogName)
{
	BOOL	bRTF;
	bRTF = m_bRTF;
	m_bRTF = false;
	DoSave(strLogName,false);
	m_bRTF = bRTF;
}


// Bring up Save As dialog box before saving.
void CJPEGsnoopDoc::OnFileSaveAs()
{

	CString		strPathName;
	BOOL		bRTF;

	// Preserve
	strPathName = m_strPathName;
	bRTF = m_bRTF;

	// FIXME
	//CAL! What is the best way to allow user to select between Plain-Text 
	// & RTF formats and have the filename change when they change the 
	// filter type? Currently they would have to do that manually.
	// Possible solution: Use OnLBSelChangeNotify override of CFileDialog
	m_bRTF = false;
	m_strPathName = m_strPathName + ".txt";

	bool status = 0;
	/*
	char strFilter[] =
		"Log - Plain Text (*.txt)|*.txt|"\
		//"Log - Rich Text (*.rtf)|*.rtf|"\
		"All Files (*.*)|*.*||";
	*/
	char strFilter[] =
		"Log - Plain Text (*.txt)|*.txt|"\
		"All Files (*.*)|*.*||";

	//CAL! BUG #1008
	CFileDialog FileDlg(FALSE, ".txt", m_strPathName, OFN_OVERWRITEPROMPT, strFilter);

	CString title;
	CString sFileName;
	VERIFY(title.LoadString(IDS_CAL_FILESAVE));
	FileDlg.m_ofn.lpstrTitle = title;
	
	if( FileDlg.DoModal() == IDOK )
	{
		sFileName = FileDlg.GetPathName();

		// Determine the extension
		// If the user entered an ".rtf" extension, then we allow RTF
		// saving, otherwise we default to plain text
		CString cstrSelFileExt = sFileName.Right(sFileName.GetLength() - sFileName.ReverseFind('.'));
		
		//if (cstrSelFileExt == ".rtf") {
		//	m_bRTF = true;
		//}

		// Perform the save
		DoSave(sFileName,false);

	}

	// Revert
	m_strPathName = strPathName;
	m_bRTF = bRTF;
}



void CJPEGsnoopDoc::OnPreviewRng(UINT nID)
{
	unsigned ind;
	ind = nID - ID_PREVIEW_RGB + PREVIEW_RGB;
	m_pImgDec->SetPreviewMode(ind);
	UpdateAllViews(NULL);
}

void CJPEGsnoopDoc::OnUpdatePreviewRng(CCmdUI* pCmdUI)
{
	unsigned nID;
	nID = pCmdUI->m_nID - ID_PREVIEW_RGB + PREVIEW_RGB;
	if (m_pImgDec->GetPreviewMode() == nID) {
		pCmdUI->SetCheck(true);
	} else {
		pCmdUI->SetCheck(false);
	}
}

void CJPEGsnoopDoc::OnZoomRng(UINT nID)
{
	unsigned ind;
	if (nID == ID_IMAGEZOOM_ZOOMIN) {
		m_pImgDec->SetPreviewZoom(true,false,false,0);
		UpdateAllViews(NULL);
	} else if (nID == ID_IMAGEZOOM_ZOOMOUT) {
		m_pImgDec->SetPreviewZoom(false,true,false,0);
		UpdateAllViews(NULL);
	} else {
		ind = nID - ID_IMAGEZOOM_12 + PRV_ZOOM_12;
		m_pImgDec->SetPreviewZoom(false,false,true,ind);
		UpdateAllViews(NULL);
	}
}

void CJPEGsnoopDoc::OnUpdateZoomRng(CCmdUI* pCmdUI)
{
	unsigned nID;
	if (pCmdUI->m_nID == ID_IMAGEZOOM_ZOOMIN) {
		// No checkmark
	} else if (pCmdUI->m_nID == ID_IMAGEZOOM_ZOOMOUT) {
		// No checkmark
	} else {
		nID = pCmdUI->m_nID - ID_IMAGEZOOM_12 + PRV_ZOOM_12;
		unsigned getZoom;
		getZoom = m_pImgDec->GetPreviewZoom();
		unsigned menuOffset;
		menuOffset = pCmdUI->m_nID - ID_IMAGEZOOM_12;
		pCmdUI->SetCheck(m_pImgDec->GetPreviewZoom() == nID);
	}
}

// Assuming a file is already open (and DQT still present),
// have a look in another file (exe) to see if DQT exists
void CJPEGsnoopDoc::OnToolsSearchexecutablefordqt()
{

	// Don't need to do AnalyzeOpen() because a file pointer to
	// the executable is created here.

	bool status = 0;
	char strFilter[] =
		"Executable (*.exe)|*.exe|"\
		"DLL Library (*.dll)|*.dll|"\
		"All Files (*.*)|*.*||";

	/*
		CFileDialog(BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
		LPCTSTR lpszDefExt = NULL,
		LPCTSTR lpszFileName = NULL,
		DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		LPCTSTR lpszFilter = NULL,
		CWnd* pParentWnd = NULL,
		DWORD dwSize = 0);
		*/
	//CAL! BUG #1008
	CFileDialog FileDlg(TRUE, ".exe", NULL, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST, strFilter);

	CString title;
	CString sFileName;
	VERIFY(title.LoadString(IDS_CAL_EXE_OPEN));
	FileDlg.m_ofn.lpstrTitle = title;
	
	if( FileDlg.DoModal() != IDOK )
	{
		return;
	}

	// We selected a file!
	sFileName = FileDlg.GetFileName();

	CFile*	pFileExe;
	CString	tmpStr;


	ASSERT(sFileName != "");
	if (sFileName == "") { AfxMessageBox("ERROR: SearchExe() but sFileName empty"); }

	try
	{
		// Open specified file
		pFileExe = new CFile(sFileName, CFile::modeRead | CFile::typeBinary | CFile::shareDenyNone);
	}
	catch (CFileException* e)
	{
		char msg[512];
		CString strError;
		e->GetErrorMessage(msg,sizeof(msg));
		strError.Format(_T("ERROR: Couldn't open file [%s]: [%s]"),
			sFileName, msg);
		AfxMessageBox(strError);
		pFileExe = NULL;

		return; // FALSE;

	}

	// Set the file size variable
	unsigned long lFileSize = (unsigned long)pFileExe->GetLength();

	// Search the file!

	m_pLog->AddLineHdr("*** Searching Executable for DQT ***");
	tmpStr.Format("  Filename: [%s]",sFileName);
	m_pLog->AddLine(tmpStr);
	tmpStr.Format("  Size:     [%lu]",lFileSize);
	m_pLog->AddLine(tmpStr);

	CwindowBuf*	pExeBuf;
	pExeBuf = new CwindowBuf();

	ASSERT(pExeBuf);
	if (!pExeBuf) {
		AfxMessageBox("ERROR: Not enough memory for EXE Search");
		return;
	}

	pExeBuf->SetStatusBar(GetStatusBar());
	pExeBuf->BufFileSet(pFileExe);
	pExeBuf->BufLoadWindow(0);

	bool		done_file = false;
	long		file_ind = 0;
	unsigned	entries_width;
	bool		entries_byteswap;
	unsigned long	found_pos=0;
	unsigned	pos = 0;
	bool		found = false;
	bool		found_entry = false;

	BYTE		search_arr[(4*64)+4];
	unsigned	search_arr_len;
	unsigned	search_set_max;

	unsigned val;

	bool bAllSame = true;
	bool bBaseline = true;

	search_set_max = 1;
	if (m_bFileOpened) {
		val = m_pImgDec->GetDqtEntry(0,0);
		for (unsigned ind=1;ind<64;ind++) {
			if (m_pImgDec->GetDqtEntry(0,ind) != val) {
				bAllSame = false;
			}
			// Are there any DQT entries > 8 bits?
			if (m_pImgDec->GetDqtEntry(0,ind) > 255) {
				bBaseline = false;
			}

		}

		if (bAllSame) {
			tmpStr.Format("  NOTE: Because the JPEG's DQT Luminance table is constant value (0x%02X),",val);
			m_pLog->AddLineWarn(tmpStr);
			m_pLog->AddLineWarn("        matching for this table has been disabled.");
			m_pLog->AddLineWarn("        Please select a different reference image.");
			search_set_max = 1;
		} else {
			search_set_max = 2;
		}
	}


	m_pLog->AddLine("  Searching for DQT Luminance tables:");
	for (unsigned zigzag_set=0;zigzag_set<2;zigzag_set++) {
		tmpStr.Format("    DQT Ordering: %s",(zigzag_set?"post-zigzag":"pre-zigzag"));
		m_pLog->AddLine(tmpStr);
	for (unsigned search_set=0;search_set<search_set_max;search_set++) {
		if (search_set==0) {
			tmpStr.Format("      Matching [JPEG Standard]");
			m_pLog->AddLine(tmpStr);
		} else {
			tmpStr.Format("      Matching [%s]",m_strPathName);
			m_pLog->AddLine(tmpStr);
		}

		for (unsigned search_pattern=0;search_pattern<5;search_pattern++) {

			switch(search_pattern) {
				case 0:
					entries_width = 1;
					entries_byteswap = false;
					break;
				case 1:
					entries_width = 2;
					entries_byteswap = false;
					break;
				case 2:
					entries_width = 2;
					entries_byteswap = true;
					break;
				case 3:
					entries_width = 4;
					entries_byteswap = false;
					break;
				case 4:
					entries_width = 4;
					entries_byteswap = true;
					break;
				default:
					entries_width = 1;
					entries_byteswap = false;
					break;
			}
			if (entries_width == 1) {
				tmpStr.Format("        Searching patterns with %u-byte DQT entries",
					entries_width);
				m_pLog->AddLine(tmpStr);
				if (!bBaseline) {
					tmpStr.Format("          DQT Table is not baseline, skipping 1-byte search");
					m_pLog->AddLine(tmpStr);
					continue;
				}
			} else {
				tmpStr.Format("        Searching patterns with %u-byte DQT entries%s",
					entries_width,(entries_byteswap)?", endian byteswap":"");
				m_pLog->AddLine(tmpStr);
			}

			search_arr_len = (entries_width*64);

			for (unsigned ind=0;ind<search_arr_len;ind++) {
				search_arr[ind] = 0;
			}
			for (unsigned ind=0;ind<64;ind++) {

				if (search_set == 0) {
					// Standard JPEG table
					val = (zigzag_set)?
						m_pJfifDec->m_sStdQuantLum[ind]:
						m_pJfifDec->m_sStdQuantLum[m_pJfifDec->m_sZigZag[ind]];
				} else {
					// Matching JPEG table
					val = (zigzag_set)?
						m_pImgDec->GetDqtEntry(0,ind):
						m_pImgDec->GetDqtEntry(0,m_pJfifDec->m_sZigZag[ind]);
				}

				for (unsigned entry_ind=0;entry_ind<entries_width;entry_ind++) {
					search_arr[ind*entries_width+entry_ind] = 0;
				}
				if (entries_byteswap) {
					if (val <= 255) {
						search_arr[ind*entries_width+0] = val;
					} else {
						// Accessing +1 is OK because we have already
						// checked for non-baseline plus entries_width=1
						search_arr[ind*entries_width+0] = val & 0xFF;
						search_arr[ind*entries_width+1] = val >> 8;
					}
				} else {
					if (val <= 255) {
						search_arr[ind*entries_width+(entries_width-1)] = val;
					} else {
						// Accessing -1 is OK because we have already
						// checked for non-baseline plus entries_width=1
						search_arr[ind*entries_width+(entries_width-1)] = val & 0xFF;
						search_arr[ind*entries_width+(entries_width-2)] = val >> 8;
					}
				}
			}

			bool done_all_matches = false;
			unsigned long start_offset = 0;
			while (!done_all_matches) {

				found = pExeBuf->BufSearchX(start_offset,search_arr,64*entries_width,true,found_pos);
				if (found) {
					tmpStr.Format("          Found @ 0x%08X",found_pos);
					m_pLog->AddLineGood(tmpStr);
					start_offset = found_pos+1;
				} else {
					done_all_matches = true;
				}
			}


		} // search_pattern
	} // search_set
	} // zigzag_set
	m_pLog->AddLine("  Done Search");

	// Since this command can be run without a normal file open, we
	// need to flush the output if we happen to be in quick log mode
	InsertQuickLog();


	if (pExeBuf) {
		delete pExeBuf;
	}
}



void CJPEGsnoopDoc::OnUpdateFileReprocess(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_bFileOpened);
}

void CJPEGsnoopDoc::OnUpdateFileSaveAs(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_bFileOpened);
}

// NOTE: This routine should ONLY be called if we have selected
// a different name for the file (i.e. not the original JPEG file!)
// The toolbar and menu items are hooked up to Save As, so there
// should be nothing else connected to this.
//
// Just in case, I am checking the filename to see if it matches
// the last opened file. If so, I abort.
BOOL CJPEGsnoopDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
	// Different from opened file, we're ok
	if (lpszPathName != m_strPathNameOpened) {
		return CRichEditDoc::OnSaveDocument(lpszPathName);
	} else {
		// Pretend we were successful so that we don't allow it
		// to try to write the file if not successful!
		AfxMessageBox("NOTE: Attempt to overwrite source file prevented");
		return true;
	}

	//return CRichEditDoc::OnSaveDocument(lpszPathName);

}



// Perform byteswap which is used to create packed image array
// before write-out of 16b values to disk
unsigned short CJPEGsnoopDoc::Swap16(unsigned short nVal)
{
	unsigned nValHi,nValLo;
	nValHi = (nVal & 0xFF00)>>8;
	nValLo = (nVal & 0x00FF);
	return (nValLo<<8) + nValHi;
}

void CJPEGsnoopDoc::OnToolsExtractembeddedjpeg()
{


	CExportDlg	exportDlg;
	CString		tmpStr;

	tmpStr.Format("0x%08X",m_pJfifDec->m_nPosEmbedStart);

	exportDlg.m_bOverlayEn = true;
	exportDlg.m_bForceEoi = false;
	exportDlg.m_bIgnoreEoi = false;
	exportDlg.m_bDhtAviInsert = m_pJfifDec->m_bAviMjpeg;
	exportDlg.m_strOffsetStart = tmpStr;

	if (exportDlg.DoModal() == IDOK) {
		bool	bRet;

		// For export, we need to call AnalyzeOpen()
		AnalyzeOpen();

		// Is the currently-decoded file in a suitable
		// state for JPEG extraction (ie. have we seen all
		// the necessary markers?)
		bRet = m_pJfifDec->ExportJpegPrepare(m_strPathName,exportDlg.m_bForceSoi,exportDlg.m_bForceEoi,exportDlg.m_bIgnoreEoi);
		if (!bRet) {
			return;
		}

		// Ask the user for the output file

		// ----------------------------

		CString		strEmbedFileName;
		CString		strPathName;
		BOOL		bRTF;

		// Preserve
		strPathName = m_strPathName;
		bRTF = m_bRTF;

		m_bRTF = false;
		m_strPathName = m_strPathName + ".export.jpg";

		bool status = 0;
		char strFilter[] =
			"JPEG Image (*.jpg)|*.jpg|"\
			"All Files (*.*)|*.*||";

		// CAL! BUG #1008	
		CFileDialog FileDlg(FALSE, ".jpg", m_strPathName, OFN_OVERWRITEPROMPT, strFilter);

		CString title;
		//VERIFY(title.LoadString(IDS_CAL_FILESAVE));
		title = "Save Exported JPEG file as";
		FileDlg.m_ofn.lpstrTitle = title;
		
		if( FileDlg.DoModal() == IDOK )
		{
			strEmbedFileName = FileDlg.GetPathName();
		} else {
			// User wanted to abort
			tmpStr.Format("  User cancelled");
			m_pLog->AddLineErr(tmpStr);

			// Revert
			m_strPathName = strPathName;
			m_bRTF = bRTF;
			return;
		}

		// Revert
		m_strPathName = strPathName;
		m_bRTF = bRTF;

		// ----------------------------

		// Perform the actual extraction
		unsigned int nFileSize = m_lFileSize;	// TODO: How to handle bit truncation?
		bRet = m_pJfifDec->ExportJpegDo(m_strPathName,strEmbedFileName,nFileSize,exportDlg.m_bOverlayEn,exportDlg.m_bDhtAviInsert,exportDlg.m_bForceSoi,exportDlg.m_bForceEoi);

		// For export, we need to call AnalyzeClose()
		AnalyzeClose();

	} else {
	}



}




void CJPEGsnoopDoc::OnUpdateToolsExtractembeddedjpeg(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_bFileOpened);
}


void CJPEGsnoopDoc::OnToolsFileoverlay()
{
	CString dlgStr;

	unsigned nOffset;
	CString  strValNew;
	unsigned nValLen;
	BYTE     anValData[16];

	bool		bCurEn;
	BYTE*		pCurData;
	unsigned	nCurLen;
	unsigned	nCurStart;
	CString		strCurDataHex;
	CString		strCurDataBin;
	CString		tmpStr;
	CString		tmpByteStr;
	unsigned	tmpByte;

	// This function assumes that we've previously opened a file!!!
	// Otherwise, there isn't much point in setting the offset value
	// since it gets reset to 0 when we open a new file manually!

	bool done = false;
	while (!done) {

		strCurDataHex = "";
		strCurDataBin = "";

		if (AnalyzeOpen() == false) {
			return;
		}

		// Set overlay dialog starting values...
		bCurEn = m_pWBuf->OverlayGet(0,pCurData,nCurLen,nCurStart);
		// Were any overlays previously defined?
		if (!bCurEn) {
			// If not, don't try to read the buffer! Simply force string
			strCurDataHex = "";
			strCurDataBin = "";
			nCurStart = 0;
			nCurLen = 0;
		} else {
			for (unsigned i=0;i<nCurLen;i++) {
				tmpStr.Format("%02X",pCurData[i]);
				strCurDataHex += tmpStr;

				strCurDataBin += Dec2Bin(pCurData[i],8);
				strCurDataBin += " ";
			}
		}

		// Determine binary form


		COverlayBufDlg overlayDlg(NULL,m_pWBuf,bCurEn,nCurStart,nCurLen,strCurDataHex,strCurDataBin);

		if (overlayDlg.DoModal() == IDOK) {

			// Convert and store the params
			nOffset  = overlayDlg.m_nOffset;
			nValLen = overlayDlg.m_nLen; // bits
			strValNew = overlayDlg.m_sValueNewHex;

			for (unsigned i=0;i<strlen(strValNew)/2;i++)
			{
				tmpByteStr = strValNew.Mid(i*2,2);
				tmpByte = strtol(tmpByteStr,NULL,16);
				anValData[i] = tmpByte;
			}

			if (overlayDlg.m_bEn) {
				// Note that we don't know the actual MCU, so
				// we'll just leave it at MCU [0,0] for now.
				// We can distinguish by nMcuLen = 0 (custom).
				m_pWBuf->OverlayInstall(0,anValData,nValLen,nOffset,
					0,0,0,0,0,0,0);
			} else {
				// Assume that if I'm using this dialog method to access the overlay
				// that I'm only ever using a single overlay! Therefore, to be safe
				// I'm going to call OverlayRemoveAll() instead of OverlayRemove() (which just
				// deletes last one).
				m_pWBuf->OverlayRemoveAll();
			}


			m_pJfifDec->ImgSrcChanged();

			Reprocess();

		} else {
		}

		AnalyzeClose();

		if (!overlayDlg.m_bApply) {
			done = true;
		}

	} // while
}

/*
CString CJPEGsnoopDoc::Dec2Bin(unsigned nVal,unsigned nLen)
{
	unsigned	nBit;
	CString		strBin = "";
	for (int nInd=nLen-1;nInd>=0;nInd--)
	{
		nBit = ( nVal & (1 << nInd) ) >> nInd;
		strBin += (nBit==1)?"1":"0";
		if ( ((nInd % 8) == 0) && (nInd != 0) ) {
			strBin += " ";
		}
	}
	return strBin;
}
*/

void CJPEGsnoopDoc::OnUpdateToolsFileoverlay(CCmdUI *pCmdUI)
{
	if (m_bFileOpened) {
		pCmdUI->Enable(true);
	} else {
		pCmdUI->Enable(false);
	}
}


void CJPEGsnoopDoc::OnToolsLookupmcuoffset()
{
	CString dlgStr;

	// Don't need to use AnalyzeOpen() here as this function
	// simply checks the MCU map variable.

	unsigned pic_width  = (m_pImgDec->m_nMcuXMax*m_pImgDec->m_nMcuWidth);
	unsigned pic_height = (m_pImgDec->m_nMcuYMax*m_pImgDec->m_nMcuHeight);
	CLookupDlg lookupDlg(NULL,m_pImgDec,pic_width,pic_height);

	//lookupDlg.m_sRngX.Format("(0..%u)",(theApp.m_pImgDec->m_nMcuXMax*theApp.m_pImgDec->m_nMcuWidth)-1);
	//lookupDlg.m_sRngY.Format("(0..%u)",(theApp.m_pImgDec->m_nMcuYMax*theApp.m_pImgDec->m_nMcuHeight)-1);


	if (lookupDlg.DoModal() == IDOK) {
	} else {
	}
}

void CJPEGsnoopDoc::OnUpdateToolsLookupmcuoffset(CCmdUI *pCmdUI)
{
	if (m_bFileOpened) {
		pCmdUI->Enable(true);
	} else {
		pCmdUI->Enable(false);
	}
}

void CJPEGsnoopDoc::OnOverlaysMcugrid()
{
	m_pImgDec->SetPreviewOverlayMcuGridToggle();
	UpdateAllViews(NULL);
}


void CJPEGsnoopDoc::OnUpdateOverlaysMcugrid(CCmdUI *pCmdUI)
{
	if (m_pImgDec->GetPreviewOverlayMcuGrid()) {
		pCmdUI->SetCheck(true);
	} else {
		pCmdUI->SetCheck(false);
	}
}





void CJPEGsnoopDoc::OnUpdateIndicatorYcc(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(TRUE);
	pCmdUI->SetText(m_pImgDec->GetStatusYccText());
}

void CJPEGsnoopDoc::OnUpdateIndicatorMcu(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(TRUE);
	pCmdUI->SetText(m_pImgDec->GetStatusMcuText());
}

void CJPEGsnoopDoc::OnUpdateIndicatorFilePos(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(TRUE);
	pCmdUI->SetText(m_pImgDec->GetStatusFilePosText());
}



void CJPEGsnoopDoc::OnScansegmentDetaileddecode()
{
	CDecodeDetailDlg detailDlg(NULL);
	
	detailDlg.m_bEn = m_pImgDec->m_bDetailVlc;
	detailDlg.m_nMcuX = m_pImgDec->m_nDetailVlcX;
	detailDlg.m_nMcuY = m_pImgDec->m_nDetailVlcY;
	detailDlg.m_nMcuLen = m_pImgDec->m_nDetailVlcLen;
	

	// Sequence of markers:
	//   #1 - Start MCU
	//   #2 - End MCU
	CPoint	ptScan1,ptScan2;

	unsigned	nStartMcuX=0;
	unsigned	nStartMcuY=0;
	unsigned	nEndMcuX=0;
	unsigned	nEndMcuY=0;
	int			nMcuRngLen=0;
	unsigned	nTmp;

#if 0
	// Single point, gives start point
	if (m_pImgDec->m_nMarkersBlkNum >= 1) {
		ptScan1 = m_pImgDec->m_ptMarkersBlk[m_pImgDec->m_nMarkersBlkNum-1-0];
		nStartMcuX = ptScan1.x / m_pImgDec->m_nCssX;
		nStartMcuY = ptScan1.y / m_pImgDec->m_nCssY;
		nMcuRngLen = 0;
	}
#else
	// Double-point, gives range
	if (m_pImgDec->m_nMarkersBlkNum >= 2) {
		ptScan1 = m_pImgDec->m_ptMarkersBlk[m_pImgDec->m_nMarkersBlkNum-1-1];
		ptScan2 = m_pImgDec->m_ptMarkersBlk[m_pImgDec->m_nMarkersBlkNum-1-0];
		nStartMcuX = ptScan1.x / m_pImgDec->m_nCssX;
		nStartMcuY = ptScan1.y / m_pImgDec->m_nCssY;
		nEndMcuX = ptScan2.x / m_pImgDec->m_nCssX;
		nEndMcuY = ptScan2.y / m_pImgDec->m_nCssY;
		unsigned	nStartMcu,nEndMcu;
		nStartMcu = (nStartMcuY*m_pImgDec->m_nMcuXMax + nStartMcuX);
		nEndMcu = (nEndMcuY*m_pImgDec->m_nMcuXMax + nEndMcuX);

		if (nStartMcu > nEndMcu) {
			// Reverse the order!
			nTmp = nEndMcuX;
			nEndMcuX = nStartMcuX;
			nStartMcuX = nTmp;

			nTmp = nEndMcuY;
			nEndMcuY = nStartMcuY;
			nStartMcuY = nTmp;

			// Recalc linear positions
			nStartMcu = (nStartMcuY*m_pImgDec->m_nMcuXMax + nStartMcuX);
			nEndMcu = (nEndMcuY*m_pImgDec->m_nMcuXMax + nEndMcuX);
		}

		// Calculate length from linear positions
		nMcuRngLen = nEndMcu-nStartMcu + 1;

	}
#endif

	// Set the "load" values from the last MCU click
	detailDlg.m_nLoadMcuX = nStartMcuX;
	detailDlg.m_nLoadMcuY = nStartMcuY;
	detailDlg.m_nLoadMcuLen = nMcuRngLen;


	if (detailDlg.DoModal() == IDOK) {
		m_pImgDec->m_bDetailVlc = detailDlg.m_bEn;
		m_pImgDec->m_nDetailVlcX = detailDlg.m_nMcuX;
		m_pImgDec->m_nDetailVlcY = detailDlg.m_nMcuY;
		m_pImgDec->m_nDetailVlcLen = detailDlg.m_nMcuLen;
	}
}

void CJPEGsnoopDoc::OnUpdateScansegmentDetaileddecode(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(true);
}


void CJPEGsnoopDoc::OnToolsExporttiff()
{

	// Get filename
	CString		sFnameOut;

	sFnameOut = m_strPathName + ".tif";
//	sFnameOut = "C:\\TEMP\\output.tif";

	char strFilter[] =
		"TIFF (*.tif)|*.tif|"\
		"All Files (*.*)|*.*||";

	//CAL! BUG #1008
	CFileDialog FileDlg(FALSE, ".tif", sFnameOut, OFN_OVERWRITEPROMPT, strFilter);

	CString title;
	CString sFileName;
	title = "Output TIFF Filename";
//	VERIFY(title.LoadString(IDS_CAL_FILESAVE));
	FileDlg.m_ofn.lpstrTitle = title;
	
	if( FileDlg.DoModal() == IDOK )
	{
		sFnameOut = FileDlg.GetPathName();
	} else {
		return;
	}



	FileTiff		myTiff;
	unsigned char*	pBitmapRgb;
	short*			pBitmapYccY;
	short*			pBitmapYccCb;
	short*			pBitmapYccCr;
	unsigned char*	pBitmapSel8;
	unsigned short*	pBitmapSel16;
	
	unsigned		nSizeX,nSizeY;
	unsigned		nOffsetSrc,nOffsetDst;
	bool			bModeYcc;	// YCC (true), RGB (false)
	bool			bMode16b;	// 16b (true), 8b (false)
	short			nValY,nValCb,nValCr;
	unsigned short	nValR,nValG,nValB;

	short			nValMinY,nValMinCb,nValMinCr;
	short			nValMaxY,nValMaxCb,nValMaxCr;
	nValMaxY  = (short)0x0000; // - 32768
	nValMinY  = (short)0xFFFF; // + 32767
	nValMaxCb = (short)0x0000; // - 32768
	nValMinCb = (short)0xFFFF; // + 32767
	nValMaxCr = (short)0x0000; // - 32768
	nValMinCr = (short)0xFFFF; // + 32767

	nSizeX = m_pImgDec->m_nBlkXMax*8;
	nSizeY = m_pImgDec->m_nBlkYMax*8;
	pBitmapRgb = m_pImgDec->GetBitmapPtr();
	pBitmapYccY  = m_pImgDec->m_pPixValY;
	pBitmapYccCb = m_pImgDec->m_pPixValCb;
	pBitmapYccCr = m_pImgDec->m_pPixValCr;
	pBitmapSel8 = NULL;
	pBitmapSel16 = NULL;

	CExportTiffDlg	myTiffDlg;
	int rVal,nModeSel;
	myTiffDlg.m_sFname = sFnameOut;
	rVal = myTiffDlg.DoModal();
	nModeSel = myTiffDlg.m_nCtlFmt;

	switch(nModeSel) {
		default:
		case 0:
			bModeYcc = false;
			bMode16b = false;
			break;
		case 1:
			bModeYcc = false;
			bMode16b = true;
			break;
		case 2:
			bModeYcc = true;
			bMode16b = false;
			break;
	}

	if (bModeYcc && bMode16b) {
		AfxMessageBox("ERROR: Can't output 16-bit YCC!");
		return;
	}

	// Create a separate bitmap array
	if (bMode16b) {
		pBitmapSel16 = new unsigned short[nSizeX*nSizeY*3];
		ASSERT(pBitmapSel16);
	} else {
		pBitmapSel8 = new unsigned char[nSizeX*nSizeY*3];
		ASSERT(pBitmapSel8);
	}
	if (!bModeYcc) {
		// RGB mode
		for (unsigned nIndY=0;nIndY<nSizeY;nIndY++) {
			for (unsigned nIndX=0;nIndX<nSizeX;nIndX++) {
				nOffsetDst = (nIndY*nSizeX+nIndX)*3;
				// pBitmapRgb is actually the DIB, which is inverted vertically
				nOffsetSrc = ((nSizeY-1-nIndY)*nSizeX+nIndX)*4;
				nValR = (unsigned short)pBitmapRgb[nOffsetSrc+2];
				nValG = (unsigned short)pBitmapRgb[nOffsetSrc+1];
				nValB = (unsigned short)pBitmapRgb[nOffsetSrc+0];
				if (!bMode16b) {
					// Rearrange into file-order
					pBitmapSel8[nOffsetDst+0] = nValR;
					pBitmapSel8[nOffsetDst+1] = nValG;
					pBitmapSel8[nOffsetDst+2] = nValB;
				} else {
					// Rearrange into file-order
					// Need to do endian byte-swap when outputting
					// 16b values to disk.
					pBitmapSel16[nOffsetDst+0] = Swap16(nValR<<8);
					pBitmapSel16[nOffsetDst+1] = Swap16(nValG<<8);
					pBitmapSel16[nOffsetDst+2] = Swap16(nValB<<8);
				}
			}
		}
	} else {
		// YCC mode
		for (unsigned nIndY=0;nIndY<nSizeY;nIndY++) {
			for (unsigned nIndX=0;nIndX<nSizeX;nIndX++) {
				nOffsetDst = (nIndY*nSizeX+nIndX)*3;
				nOffsetSrc = (nIndY*nSizeX+nIndX)*1;
				nValY  = pBitmapYccY[nOffsetSrc];
				nValCb = pBitmapYccCb[nOffsetSrc];
				nValCr = pBitmapYccCr[nOffsetSrc];

				nValMinY = min(nValMinY,nValY);
				nValMaxY = max(nValMaxY,nValY);
				nValMinCb = min(nValMinCb,nValCb);
				nValMaxCb = max(nValMaxCb,nValCb);
				nValMinCr = min(nValMinCr,nValCr);
				nValMaxCr = max(nValMaxCr,nValCr);

				// Clip the YCC to -1024 .. +1023
				if (nValY  < -1024) { nValY  = -1024; }
				if (nValY  > +1023) { nValY  = +1023; }
				if (nValCb < -1024) { nValCb = -1024; }
				if (nValCb > +1023) { nValCb = +1023; }
				if (nValCr < -1024) { nValCr = -1024; }
				if (nValCr > +1023) { nValCr = +1023; }

				// Expect original YCC range is: -1024 .. +1023
				// For 8bit mode:
				// .. add +1024 = 0..2047
				// .. div /8    = 0..255
				if (!bMode16b) {
					// Rearrange into file-order
					pBitmapSel8[nOffsetDst+0] = (unsigned char)((0x0400+nValY)>>3);
					pBitmapSel8[nOffsetDst+1] = (unsigned char)((0x0400+nValCb)>>3);
					pBitmapSel8[nOffsetDst+2] = (unsigned char)((0x0400+nValCr)>>3);
				} else {
					ASSERT(false);
					// YCC-16b not defined!
				}
			}
		}
	}

	if (bMode16b) {
		myTiff.WriteFile(sFnameOut,bModeYcc,bMode16b,(void*)pBitmapSel16,nSizeX,nSizeY);
	} else {
		myTiff.WriteFile(sFnameOut,bModeYcc,bMode16b,(void*)pBitmapSel8,nSizeX,nSizeY);
	}

	if (pBitmapSel8) {
		delete pBitmapSel8;
		pBitmapSel8 = NULL;
	}
	if (pBitmapSel16) {
		delete pBitmapSel16;
		pBitmapSel16 = NULL;
	}

}

void CJPEGsnoopDoc::OnUpdateToolsExporttiff(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_bFileOpened);
}
