// JPEGsnoop - JPEG Image Decoder & Analysis Utility
// Copyright (C) 2014 - Calvin Hass
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

// Constructor allocates dynamic structures and resets state
CJPEGsnoopDoc::CJPEGsnoopDoc()
: m_pView(NULL)
{
	m_pLog = new CDocLog(this);
	if (!m_pLog) {
		AfxMessageBox(_T("ERROR: Not enough memory for Document"));
		exit(1);
	}

	m_pWBuf = new CwindowBuf();
	if (!m_pWBuf) {
		AfxMessageBox(_T("ERROR: Not enough memory for Document"));
		exit(1);
	}

	// Allocate the JPEG decoder
	m_pImgDec = new CimgDecode(m_pLog,m_pWBuf);
	if (!m_pWBuf) {
		AfxMessageBox(_T("ERROR: Not enough memory for Image Decoder"));
		exit(1);
	}
	m_pJfifDec = new CjfifDecode(m_pLog,m_pWBuf,m_pImgDec);
	if (!m_pWBuf) {
		AfxMessageBox(_T("ERROR: Not enough memory for JFIF Decoder"));
		exit(1);
	}

	// Ideally this would be passed by constructor, but simply access
	// directly for now.
	CJPEGsnoopApp*	pApp;
	pApp = (CJPEGsnoopApp*)AfxGetApp();
    m_pAppConfig = pApp->m_pAppConfig;
	ASSERT(m_pAppConfig);

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
// - Clear all major document state
// - Clear log
//
void CJPEGsnoopDoc::Reset()
{
	// Reset all members
	m_pFile = NULL;
	m_lFileSize = 0L;

	// No log data available until we open & process a file
	m_bFileOpened = FALSE;
	m_strPathNameOpened = _T("");

	// Indicate to JFIF ProcessFile() that document has changed
	// and that the scan decode needs to be redone if it
	// is to be displayed.
	m_pJfifDec->ImgSrcChanged();

	// Clean up the quick log
	m_saLogQuickTxt.RemoveAll();
	m_naLogQuickCol.RemoveAll();

}

// Main entry point for creation of a new document
BOOL CJPEGsnoopDoc::OnNewDocument()
{
	if (!CRichEditDoc::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)
	Reset();

	return TRUE;
}

// Default CreateClientItem() implementation
CRichEditCntrItem* CJPEGsnoopDoc::CreateClientItem(REOBJECT* preo) const
{
	return new CJPEGsnoopCntrItem(preo, const_cast<CJPEGsnoopDoc*>(this));
}




// CJPEGsnoopDoc serialization
//
// NOTE:
// - This is called during the standard OnOpenDocument() and presumably
//   OnSaveDocument(). Currently it is not implemented.
//
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
//
// PRE:
// - m_bLogQuickMode
// - m_pView (view from RichEdit must already be established)
//
// POST:
// - m_saLogQuickTxt
// - m_naLogQuickCol
//
int CJPEGsnoopDoc::AppendToLog(CString strTxt, COLORREF sColor)
{
	// Note that in "nogui" mode we expect that QuickMode
	// will be active while we are adding to the log
	if (m_bLogQuickMode) {

		// Don't exceed a realistic maximum!
		unsigned numLines = m_saLogQuickTxt.GetCount();
		if (numLines == DOCLOG_MAX_LINES) {
			m_saLogQuickTxt.Add(_T("*** TOO MANY LINES IN REPORT -- TRUNCATING ***"));
			m_naLogQuickCol.Add((unsigned)sColor);
			return 0;
		} else if (numLines > DOCLOG_MAX_LINES) {
			return 0;
		}

		m_saLogQuickTxt.Add(strTxt);
		m_naLogQuickCol.Add((unsigned)sColor);


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
	cf.crTextColor = sColor;
	// Set insertion point to end of text
	nInsertionPoint = pCtrl->GetWindowTextLength();
	pCtrl->SetSel(nInsertionPoint, -1);
	// Set the character format
	pCtrl->SetSelectionCharFormat(cf);
	// Replace selection. Because we have nothing 
	// selected, this will simply insert
	// the string at the current caret position.
	pCtrl->ReplaceSel(strTxt);
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


// Transfer the contents of the QuickLog buffer to the RichEdit control
// for display.
//
// In command-line nogui mode we skip this step as there may not be a
// RichEdit control assigned (m_pView will be NULL). Instead, we will
// later call DoMyLogSave() to release the buffer.
//
// PRE:
// - m_bLogQuickMode
// - m_pView (view from RichEdit must already be established)
//
int CJPEGsnoopDoc::InsertQuickLog()
{
	if (!m_bLogQuickMode) {
		return 0;
	}

	if (!theApp.m_pAppConfig->bCmdLineGui) {
		// -nogui mode
		// In this mode we don't want to use the RichEdit
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

// Add a basic text line to the log
void CJPEGsnoopDoc::AddLine(CString strTxt)
{
	AppendToLog(strTxt+_T("\n"),RGB(1, 1, 1));
}

// Add a header text line to the log
void CJPEGsnoopDoc::AddLineHdr(CString strTxt)
{
	AppendToLog(strTxt+_T("\n"),RGB(1, 1, 255));
}

// Add a header description text line to the log
void CJPEGsnoopDoc::AddLineHdrDesc(CString strTxt)
{
	AppendToLog(strTxt+_T("\n"),RGB(32, 32, 255));
}

// Add a warning text line to the log
void CJPEGsnoopDoc::AddLineWarn(CString strTxt)
{
	AppendToLog(strTxt+_T("\n"),RGB(128, 1, 1));
}

// Add an error text line to the log
void CJPEGsnoopDoc::AddLineErr(CString strTxt)
{
	AppendToLog(strTxt+_T("\n"),RGB(255, 1, 1));
}

// Add a "good" indicator text line to the log
void CJPEGsnoopDoc::AddLineGood(CString strTxt)
{
	AppendToLog(strTxt+_T("\n"),RGB(16, 128, 16));
}

// ***************************************

// Fetch a pointer to the frame's status bar
//
// RETURN:
// - Pointer to the main frame's status bar
//
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


// Open the file named in m_strPathName
// - Close / cleanup if another file was already open
//
// PRE:
// - m_strPathName
//
// POST:
// - m_pFile
// - m_lFileSize
// - m_pWBuf loaded
//
// RETURN:
// - Indicates if file open was successful
//
BOOL CJPEGsnoopDoc::AnalyzeOpen()
{
	ASSERT(m_strPathName != _T(""));
	if (m_strPathName == _T("")) {
		// Force dialog box to show as this is a major error
		AfxMessageBox(_T("ERROR: AnalyzeOpen() but m_strPathName empty"));
		return false;
	}

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
		TCHAR msg[512];
		CString strError;
		e->GetErrorMessage(msg,sizeof(msg));
		// Note: msg includes m_strPathName
		strError.Format(_T("ERROR: Couldn't open file: [%s]"),msg);
		m_pLog->AddLineErr(strError);
		if (m_pAppConfig->bInteractive)
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

// Close the current file
// - Invalidate the buffer
//
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

// Perform the file analysis
// - Configure the links between the status bar and the decoders
// - Display any coach messages (if enabled)
// - Output the log header text
// - Process the current file via ProcessFile()
// - Dump the output to the log (if in QuickLog mode)
// - Force a redraw
//
// PRE:
// - m_pJfifDec
// - m_pImgDec
// - m_lFileSize
//
// POST:
// - m_bLogQuickMode
//
void CJPEGsnoopDoc::AnalyzeFileDo()
{

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

	CString strTmp;
	AddLine(_T(""));
	strTmp.Format(_T("JPEGsnoop %s by Calvin Hass"),VERSION_STR);
	AddLine(strTmp);
	AddLine(_T("  http://www.impulseadventure.com/photo/"));
	AddLine(_T("  -------------------------------------"));
	AddLine(_T(""));
	strTmp.Format(_T("  Filename: [%s]"),(LPCTSTR)m_strPathName);
	AddLine(strTmp);
	strTmp.Format(_T("  Filesize: [%llu] Bytes"),m_lFileSize);
	AddLine(strTmp);
	AddLine(_T(""));



	// Perform the actual decoding
	if (m_lFileSize>0x8000000UL) {
		AddLineErr(_T("ERROR: Files larger than 2GB not supported in this version of JPEGsnoop."));
	} else if (m_lFileSize == 0) {
		AddLineErr(_T("ERROR: File length is zero, no decoding done."));
	} else {
		m_pJfifDec->ProcessFile(m_pFile);
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
	
}

// Analyze the current file
// - Open file
// - Analyze file
// - Close file
//
// RETURN:
// - Status from opening file
//
BOOL CJPEGsnoopDoc::AnalyzeFile()
{
	BOOL bRetVal;

	// Assumes that we have set up the member vars already
	// Perform the actual processing. We have this in a routine
	// so that we can quickly recalculate the log file again
	// if an option changes.

	bRetVal = AnalyzeOpen();
	if (bRetVal) {
		// Only now that we have successfully opened the document
		// should be mark the flag as such. This flag is used by
		// other menu items to know whether or not the file is ready.
		m_bFileOpened = TRUE;
		AnalyzeFileDo();
	}
	AnalyzeClose();

	// In the last part of AnalyzeClose(), we mark the file
	// as not modified, so that we don't get prompted to save.

	return bRetVal;

}


// Read a line from the current opened file (m_pFile)
// UNUSED
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
		TRACE2("CJPEGsnoopDoc::ReadLine returns FALSE Seek (%8.8lX, %8.8lX)\n",
			lOffset, lPosition);
		return FALSE;
	}

	BYTE* pszBuffer = new BYTE[nLength];
	if (!pszBuffer) {
		AfxMessageBox(_T("ERROR: Not enough memory for Document ReadLine"));
		exit(1);
	}

	int nReturned = m_pFile->Read(pszBuffer, nLength);
	
	// The Read is supposed to only return a maximum of nLength bytes
	// This check helps avoid a Read Overrun Code Analysis warning below
	//   warning : C6385: Reading invalid data from 'pszBuffer':  the readable size
	//   is 'nLength*1' bytes, but '2' bytes may be read.
	if (nReturned > nLength) {
		return FALSE;
	}

	if (nReturned <= 0)
	{
		TRACE2("CJPEGsnoopDoc::ReadLine returns FALSE Read (%d, %d)\n",
			nLength,
			nReturned);
		if (pszBuffer) {
			delete [] pszBuffer;
		}
		return FALSE;
	}

	CString strTemp;
	CString strCharsIn;

	strTemp.Format(_T("%8.8I64X - "), lPosition);
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

	if (pszBuffer) {
		delete [] pszBuffer;
	}

	return TRUE;
}


// --------------------------------------------------------------------
// --- START OF BATCH PROCESSING
// --------------------------------------------------------------------



// The root of the batch recursion. It simply jumps into the
// recursion loop but initializes the search to start with an
// empty search result.
//
// INPUT:
// - bAskSettings		If set, show dialog to user asking for batch process settings
// - strBatchDir		The root folder of the batch process
// - bRecSubdir			Flag to descend recursively into sub-directories
// - bExtractAll		Flag to extract all JPEGs from files
//
void CJPEGsnoopDoc::DoBatchProcess(bool bAskSettings,CString strBatchDir,bool bRecSubdir,bool bExtractAll)
{
	CFolderDialog		myFolderDlg(NULL);
	CString				strRootDir;
	CString				strDir;
	bool				bSubdirs = false;

	CString				strDirSrc;
	CString				strDirDst;
	
	// Bring up dialog to select subdir recursion
	bool		bAllOk= true;
	if (bAskSettings) {
		// Here is where we ask about recursion
		// We also ask whether "extract all" mode is desired
		CBatchDlg myBatchDlg;
		myBatchDlg.m_bProcessSubdir = false;
		myBatchDlg.m_strDir = strDir;
		myBatchDlg.m_bExtractAll = bExtractAll;
		if (myBatchDlg.DoModal() == IDOK) {
			// Fetch the settings from the dialog
			bSubdirs = (myBatchDlg.m_bProcessSubdir != 0);
			bExtractAll = (myBatchDlg.m_bExtractAll != 0);
			strDirSrc = myBatchDlg.m_strDirSrc;
			strDirDst = myBatchDlg.m_strDirDst;
		} else {
			bAllOk = false;
		}
	} else {
		bSubdirs = bRecSubdir;
		// bExtractAll set by param
		strDirSrc = strBatchDir;
		strDirDst = strBatchDir;
	}

	// In Batch Processing mode, we'll temporarily turn off the Interactive mode
	// so that any errors that arise in decoding are only output to the log files
	// and not to an alert dialog box.
	bool	bInteractiveSaved = m_pAppConfig->bInteractive;
	m_pAppConfig->bInteractive = false;

	if (bAllOk) {

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
		DoBatchRecurseLoop(strDirSrc,strDirDst,_T(""),bSubdirs,bExtractAll);

		// TODO: Clean up after last log output

		// Alert the user that we are done
		if (bAskSettings) {
			AfxMessageBox(_T("Batch Processing Complete!"));
		}

	}

	// Restore the Interactive mode setting
	m_pAppConfig->bInteractive = bInteractiveSaved;

}



// Recursive routine that searches for files and folders
// Used in batch file processing mode
//
// INPUT:
// - strSrcRootName		Starting path for input files
// - strDstRootName		Starting path for output files
// - strPathName			Path (below strSrcRootName) and filename for current input file
// - bSubdirs			Flag to descend into all sub-directories
// - bExtractAll		Flag to extract all JPEG from file
//
void CJPEGsnoopDoc::DoBatchRecurseLoop(CString strSrcRootName,CString strDstRootName,CString strPathName,bool bSubdirs,bool bExtractAll)
{
	// The following code snippet is based on MSDN code:
	// http://msdn.microsoft.com/en-us/library/scx99850%28VS.80%29.aspx

	CFileFind	finder;
	CString		strWildcard;
	CString		strDirName;
	CString		strSubPath;
	CString		strPath;

	// build a string with wildcards
	if (strPathName.IsEmpty()) {
		strWildcard.Format(_T("%s\\*.*"),(LPCTSTR)strSrcRootName);
	} else {
		strWildcard.Format(_T("%s\\%s\\*.*"),(LPCTSTR)strSrcRootName,(LPCTSTR)strPathName);
	}

	// start working for files
	BOOL bWorking =	finder.FindFile(strWildcard);

	while (bWorking)
	{
		bWorking = finder.FindNextFile();

		// skip	. and .. files;	otherwise, we'd
		// recur infinitely!

		if (finder.IsDots())
			continue;

		strPath	= finder.GetFilePath();

		// Now need to remove "root" from "path" to recalculate
		// new relative subpath
		strDirName = finder.GetFileName();
		if (strPathName.IsEmpty()) {
			strSubPath = strDirName;
		} else {
			strSubPath.Format(_T("%s\\%s"),(LPCTSTR)strPathName,(LPCTSTR)strDirName);
		}

		// if it's a directory,	recursively	search it
		if (finder.IsDirectory())
		{
			if (bSubdirs) {
				DoBatchRecurseLoop(strSrcRootName,strDstRootName,strSubPath,bSubdirs,bExtractAll);
			}
		} else {
			// GetFilePath() includes both the path	& filename
			// when	called on a	file entry,	so there is	no need
			// to specifically call	GetFileName()
			//		 CString strFname =	finder.GetFileName();

			// Perform the actual processing on the file
			DoBatchRecurseSingle(strSrcRootName,strDstRootName,strSubPath,bExtractAll);
		}
	}

	finder.Close();
}

// Perform processing on file selected by the batch recursion
// process DoBatchRecurseLoop().
//
// INPUT:
// - strSrcRootName		Starting path for input files
// - strDstRootName		Starting path for output files
// - strPathName		Path (below szSrcRootName) and filename for current input file
// - bExtractAll		Flag to extract all JPEG from file
//
void CJPEGsnoopDoc::DoBatchRecurseSingle(CString strSrcRootName,CString strDstRootName,CString strPathName,bool bExtractAll)
{
	bool		bDoSubmit = false;
	unsigned	nInd;

	CString		strFnameSrc;
	CString		strFnameDst;
	CString		strFnameOnly;
	CString		strFnameExt;
	CString		strFnameLog;
	CString		strFnameEmbed;

	strFnameSrc.Format(_T("%s\\%s"),(LPCTSTR)strSrcRootName,(LPCTSTR)strPathName);
	strFnameDst.Format(_T("%s\\%s"),(LPCTSTR)strDstRootName,(LPCTSTR)strPathName);

	// Extract the filename (without extension) and extension from
	// the full pathname.
	//
	// FIXME: Rewrite to use Windows APIs to do this or else
	// use the boost library as they will be more robust than
	// the simple searches for '\' and '.' that are done here.
	//  
#if 1
	// -----------
	strFnameOnly = strFnameSrc.Mid(strFnameSrc.ReverseFind('\\')+1);
	nInd = strFnameOnly.ReverseFind('.');
	strFnameOnly = strFnameOnly.Mid(0,nInd);


	// Extract the file extension
	nInd = strFnameSrc.ReverseFind('.');
	strFnameExt = strFnameSrc.Mid(nInd);
	strFnameExt.MakeLower();
	// -----------
#else
	unsigned	nLen = strFnameSrc.GetLength();
	LPTSTR lpstrSrcFullpath = new TCHAR[nLen+1];
	_tcscpy_s(lpstrSrcFullpath,nLen+1,strFnameSrc);

	LPTSTR	lpstrSrcFnameAndExt = NULL;
	lpstrSrcFnameAndExt = PathFindFileName(lpstrSrcFullpath);
	...
#endif

	// Only process files that have an extension that implies JPEG
	// Batch processing is generally limited to files that appear to
	// represent a JPEG image (ie. ".jpg" an ".jpeg").
	//
	// TODO: Enable user to present list of extensions or disable
	// the check altogether.

	bool	bProcessFile = false;

	if ((strFnameExt == _T(".jpg")) || (strFnameExt == _T(".jpeg"))) {
		bProcessFile = true;
	}
	if (bExtractAll) {

		// In the "Extract all" mode during batch processing, we disable
		// the extension check altogether as "Extract all" mode generally
		// implies that the files may not be standalone JPEG image files.
		// They could also be corrupt files that contain JPEG images.
		//
		// TODO: Provide additional configuration options ahead of time.
		bProcessFile = true;
	}

	if (bProcessFile) {

		// Open the file & begin normal processing
		OnOpenDocument(strFnameSrc);

		// Now that we have completed processing, optionally create
		// a log file with the report results. Note that this call
		// automatically overwrites the previous log filename.

		// TODO: Add an option in the batch dialog to specify
		//       action to take if logfile already exists.

		// IMPORTANT NOTE:
		//   It is *essential* that this Append function work properly
		//   as it is used to generate the log filename from the
		//   image filename. Since we may be automatically overwriting
		//   the logfile, it is imperative that we ensure that there is
		//   no chance that the log filename happens to be the original
		//   JPEG filename.
		//
		//   In the interests of paranoia, we perform some extra checks
		//   here to ensure that this doesn't happen.

		strFnameLog = strFnameDst;
		strFnameLog.Append(_T(".txt"));

		// Now double check the filenames to ensure they don't match the image

		// - Is the last 4 characters of the strFname ".txt"?
		if (strFnameLog.Right(4) != _T(".txt")) {
			// Report error message and skip logfile save
			AfxMessageBox(_T("ERROR: Internal error #10100"));
			return;
		}
		// - Is the strFnameLog different from the input filename (strFname)?
		if (strFnameLog == strPathName) {
			// Report error message and skip logfile save
			AfxMessageBox(_T("ERROR: Internal error #10101"));
			return;
		}


		// Create the filepath for any batch embedded JPEG extraction
		strFnameEmbed = strFnameDst;
		strFnameEmbed.Append(_T(".export.jpg"));

		// Now we need to ensure that the destination directory has been created
		// Build up the subdirectories as required
		//
		// Use PathRemoveFileSpec() to find the directory path instead of
		// simply looking for the last '\'. This should be safer.
		unsigned	nLen = strFnameDst.GetLength();
		LPTSTR lpstrDstPathOnly = new TCHAR[nLen+1];
		_tcscpy_s(lpstrDstPathOnly,nLen+1,strFnameDst);
		PathRemoveFileSpec(lpstrDstPathOnly);

		SHCreateDirectoryEx(NULL,lpstrDstPathOnly,NULL);
		
		if (lpstrDstPathOnly) {
			delete [] lpstrDstPathOnly;
			lpstrDstPathOnly = NULL;
		}


		// Now that we've confirmed the filename is safe, proceed with save
		DoDirectSave(strFnameLog);

		// Now support Extract All functionality
		if (bExtractAll) {
			// For now call in non-interactive mode so that we use
			// the default "Extract all" mode and default config
			//
			// TODO: In the "Extract all" mode we should ask the
			// user for the full configuration ahead of the batch
			// process.

			DoExtractEmbeddedJPEG(false,false,strFnameEmbed);
		}

		// Now submit entry to database!
#ifdef BATCH_DO_DBSUBMIT
		bDoSubmit = m_pJfifDec->CompareSignature(true);
		if (bDoSubmit) {
			// FIXME: Check function param values. Might be outdated
			m_pJfifDec->PrepareSendSubmit(m_pJfifDec->m_strImgQualExif,m_pJfifDec->m_eDbReqSuggest,_T(""),_T("BATCH"));
		}
#endif

	}
}



// --------------------------------------------------------------------
// --- END OF BATCH PROCESSING
// --------------------------------------------------------------------



// Menu command for specifying a file offset to start decoding process
// - Present a dialog box that allows user to specify the address
//
void CJPEGsnoopDoc::OnFileOffset()
{
	COffsetDlg offsetDlg;

	// NOTE: This function assumes that we've previously opened a file
	// Otherwise, there isn't much point in setting the offset value
	// since it gets reset to 0 when we open a new file manually!

	if (m_bFileOpened) {
		AnalyzeOpen();
	}

	offsetDlg.SetOffset(theApp.m_pAppConfig->nPosStart);
	if (offsetDlg.DoModal() == IDOK) {
		theApp.m_pAppConfig->nPosStart = offsetDlg.GetOffset();
		m_pJfifDec->ImgSrcChanged();

		Reprocess();

	} else {
	}

	if (m_bFileOpened) {
		AnalyzeClose();
	}

}

// Menu command for Adding camera signature to the signature database
// - This command brings up a dialog box to specify the
//   characteristics of the selected image
// - Upon completion, the current signature is added to the database
//
void CJPEGsnoopDoc::OnToolsAddcameratodb()
{
	CDbSubmitDlg	submitDlg;
	unsigned		nUserSrcPre;
	teSource		eUserSrc;
	CString			strUserSoftware;
	CString			strQual;
	CString			strUserNotes;

	// Return values from JfifDec
	CString			strDecHash;
	CString			strDecHashRot;
	CString			strDecImgExifMake;
	CString			strDecImgExifModel;
	CString			strDecImgQualExif;
	CString			strDecSoftware;
	teDbAdd			eDecDbReqSuggest;

	m_pJfifDec->GetDecodeSummary(strDecHash,strDecHashRot,strDecImgExifMake,strDecImgExifModel,strDecImgQualExif,strDecSoftware,eDecDbReqSuggest);

	if (strDecHash == _T("NONE"))
	{
		// No valid signature, can't submit!
		CString strTmp = _T("No valid signature could be created, so DB submit is temporarily disabled");
		m_pLog->AddLineErr(strTmp);
		if (m_pAppConfig->bInteractive)
			AfxMessageBox(strTmp);
	
		return;
	}

	submitDlg.m_strExifMake = strDecImgExifMake;
	submitDlg.m_strExifModel = strDecImgExifModel;
	submitDlg.m_strExifSoftware = strDecSoftware;
	submitDlg.m_strUserSoftware = strDecSoftware;
	submitDlg.m_strSig = strDecHash; // Only show unrotated sig
	submitDlg.m_strQual = strDecImgQualExif;

	// Does the image appear to be edited? If so, warn
	// the user before submission...

	if (eDecDbReqSuggest == DB_ADD_SUGGEST_CAM) {
		submitDlg.m_nSource = 0; // Camera
	} else if (eDecDbReqSuggest == DB_ADD_SUGGEST_SW) {
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
			case ENUM_SOURCE_CAM:
				eUserSrc = ENUM_SOURCE_CAM;
				break;
			case ENUM_SOURCE_SW:
				eUserSrc = ENUM_SOURCE_SW;
				break;
			case ENUM_SOURCE_UNSURE:
				eUserSrc = ENUM_SOURCE_UNSURE;
				break;
			default:
				eUserSrc = ENUM_SOURCE_UNSURE;
				break;

		}

		m_pJfifDec->PrepareSendSubmit(strQual,eUserSrc,strUserSoftware,strUserNotes);
	}
}

// Menu enable status for Tools -> Add camera to database
//
void CJPEGsnoopDoc::OnUpdateToolsAddcameratodb(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_bFileOpened);
}

// Menu command for searching forward in file for JPEG
// - The search process looks for the JFIF_SOI marker
//
void CJPEGsnoopDoc::OnToolsSearchforward()
{
	// Search for start:
	unsigned long search_pos = 0;
	unsigned long start_pos;
	bool search_result;
	CString strTmp;

	// NOTE: m_bFileOpened doesn't actually refer to underlying file....
	//  so therefore it is not reliable.

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
		// TODO: Are there cases where we want to run this in non-interactive mode?
		AfxMessageBox(_T("No SOI Marker found in Forward search"));
	}
	AnalyzeClose();

}

// Menu command for searching reverse in file for JPEG image
// - The search process looks for the JFIF_SOI marker
//
void CJPEGsnoopDoc::OnToolsSearchreverse()
{
	// Search for start:

	unsigned long search_pos = 0;
	unsigned long start_pos;
	bool search_result;
	CString strTmp;

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
			// TODO: Are there cases where we want to run this in non-interactive mode?
			AfxMessageBox(_T("No SOI Marker found in Reverse search"));
			Reprocess();
		}
	}
	AnalyzeClose();

}

// Menu enable status for Tools -> Search forward
//
void CJPEGsnoopDoc::OnUpdateToolsSearchforward(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_bFileOpened);
}

// Menu enable status for Tools -> Search reverse
//
void CJPEGsnoopDoc::OnUpdateToolsSearchreverse(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_bFileOpened);
}

// Reprocess the current file
// - This command will initiate all processing of the currently-opened file
//
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



// Menu command to reprocess the current file
// - This command will initiate all processing of the currently-opened file
//
void CJPEGsnoopDoc::OnFileReprocess()
{
	m_pJfifDec->ImgSrcChanged();
	Reprocess();
}

// Trap the end of the JPEGsnoop document in case we want to perform
// any other clearing actions.
void CJPEGsnoopDoc::DeleteContents()
{
	// TODO: Add your specialized code here and/or call the base class
	
	CRichEditDoc::DeleteContents();
}

// Open Document handler
//
// This is called from the File->Open... as well as
// from command-line execution and
// during MRU list invocation:
//   CJPEGsnoopDoc::OnOpenDocument()
//   CSingleDocTemplate::OnOpenDocumentFile()
//   CDocManager::OnOpenDocumentFile()
//   CWinApp::OpenDocumentFile()
//   CWinApp::OnOpenRecentFile()
//   ...
//
// NOTES:
//   Want to avoid automatic StreamIn() and serialization that
//   will attempt to be done on the JPEG file into the RichEditCtrl.
//   However, these probably fail (not RTF) or else relies on the
//   serialize function here to do something.
//
BOOL CJPEGsnoopDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	// Before entering this function, the default file dialog
	// has already been presented, so we can't override anything.

	// FIXME:
	//     Not sure if we should use the inherited OnOpenDocument() call
	//     as it will try to interpret the file as well as call the
	//     DeleteContents() on the RichEdit.
	if (!CRichEditDoc::OnOpenDocument(lpszPathName))
		return FALSE;

	// Reset internal state
	Reset();

	SetPathName(lpszPathName,true);	// Sets m_strPathName, validate len, sets window title

	// Save mirror of m_strPathName just so that we can add safety
	// check during OnSaveDocument(). By the time OnSaveDocument() is called,
	// the original m_strPathName has already been modified to indicate the
	// log/output filename.
	m_strPathNameOpened = m_strPathName;

	BOOL bStatus;

	// Reset the offset pointer!
	theApp.m_pAppConfig->nPosStart = 0;

	// Save the current filename into the temporary config space
	// This is only done to assist debugging reporting
	m_pAppConfig->strCurFname = m_strPathName;

	// Now process the file!
	bStatus = AnalyzeFile();


	// Now allow automatic saving if cmdline specified it
	if (theApp.m_pAppConfig->bCmdLineOutputEn) {
		DoDirectSave(theApp.m_pAppConfig->strCmdLineOutputFname);
	}

	return bStatus;
}


// Force a Save in text only format, with no prompting for filename
//
// This method is used in the following cases:
// - Batch processing
// - Command line processing with output log specified
// WARNING: This routine forces an overwrite of the logfile.
//
void CJPEGsnoopDoc::DoDirectSave(CString strLogName)
{
	BOOL	bRTF;
	bRTF = m_bRTF;
	m_bRTF = false;
	if (theApp.m_pAppConfig->bCmdLineGui) {
		// Normal mode. Use CRichEdit save routine
		DoSave(strLogName,false);
	} else {
		// No GUI mode
		DoMyLogSave(strLogName);
	}
	m_bRTF = bRTF;
}

// Save the current log to text file with a simple implementation
//
// - This is done with a simple implementation rather than leveraging
//   the CRichEditCtrl::DoSave()
// - This can be used for command-line nogui and batch operations where we
//   might not have the CRichEditCtrl allocated.
//
void CJPEGsnoopDoc::DoMyLogSave(CString strLogName)
{
	CStdioFile*	pLog;

	// Open the file for output
	ASSERT(strLogName != _T(""));

	// This save method will only work if we were in Quick Log mode
	// where we have recorded the log to a string buffer and not
	// directly to the RichEdit
	// Note that m_bLogQuickMode is only toggled on during processing
	// so we can't check the status here (except seeing that there are no
	// lines in the array)

	//TODO: Ensure file doesn't exist and only overwrite if specified in command-line?

	try
	{
		// Open specified file
		pLog = new CStdioFile(strLogName, CFile::modeCreate| CFile::modeWrite | CFile::typeText | CFile::shareDenyNone);
	}
	catch (CFileException* e)
	{
		TCHAR msg[512];
		CString strError;
		e->GetErrorMessage(msg,sizeof(msg));
		strError.Format(_T("ERROR: Couldn't open file for write [%s]: [%s]"),
			(LPCTSTR)strLogName, (LPCTSTR)msg);
		AfxMessageBox(strError);
		pLog = NULL;

		return;

	}

	// Step through the current log buffer
	CString	strLine;
	unsigned nQuickLines = m_saLogQuickTxt.GetCount();
	for (unsigned ind=0;ind<nQuickLines;ind++)
	{
		strLine = m_saLogQuickTxt.GetAt(ind);
		pLog->WriteString(strLine);
	}

	// Close the file
	if (pLog) {
		pLog->Close();
		delete pLog;
	}

}

// Menu comamnd for File -> Save As
// - Brings up a Save As dialog box before saving
void CJPEGsnoopDoc::OnFileSaveAs()
{

	CString		strPathName;
	BOOL		bRTF;

	// Preserve
	strPathName = m_strPathName;
	bRTF = m_bRTF;

	// FIXME:
	// Need to determine the best method to allow user selection between
	// plain-text and RTF formats and have the filename change when they
	// change the drop-down filter type. Currently the user would have
	// to do that manually.
	//
	// Possible solution: Use OnLBSelChangeNotify override in CFileDialog
	//
	// For now, force plain-text output
	//
	m_bRTF = false;
	m_strPathName = m_strPathName + _T(".txt");

	bool status = 0;
	/*
	TCHAR aszFilter[] =
		_T("Log - Plain Text (*.txt)|*.txt|")\
		//_T("Log - Rich Text (*.rtf)|*.rtf|")\
		_T("All Files (*.*)|*.*||");
	*/
	TCHAR aszFilter[] =
		_T("Log - Plain Text (*.txt)|*.txt|")\
		_T("All Files (*.*)|*.*||");

	CFileDialog FileDlg(FALSE, _T(".txt"), m_strPathName, OFN_OVERWRITEPROMPT, aszFilter);

	CString strTitle;
	CString strFileName;
	VERIFY(strTitle.LoadString(IDS_CAL_FILESAVE));
	FileDlg.m_ofn.lpstrTitle = strTitle;
	
	if( FileDlg.DoModal() == IDOK )
	{
		strFileName = FileDlg.GetPathName();

		// Determine the extension
		// If the user entered an ".rtf" extension, then we allow RTF
		// saving, otherwise we default to plain text
		CString cstrSelFileExt = strFileName.Right(strFileName.GetLength() - strFileName.ReverseFind('.'));
		
		//if (cstrSelFileExt == _T(".rtf")) {
		//	m_bRTF = true;
		//}

		// Perform the save
		DoSave(strFileName,false);

	}

	// Revert
	m_strPathName = strPathName;
	m_bRTF = bRTF;
}



// Menu command for Channel Preview
// - Supports a range in sub-items
//
void CJPEGsnoopDoc::OnPreviewRng(UINT nID)
{
	unsigned nInd;
	nInd = nID - ID_PREVIEW_RGB + PREVIEW_RGB;
	m_pImgDec->SetPreviewMode(nInd);
	UpdateAllViews(NULL);
}

// Menu enable status for Channel Preview
// - Supports a range in sub-items
//
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

// Menu command for Zoom
// - Supports a range in sub-items
//
void CJPEGsnoopDoc::OnZoomRng(UINT nID)
{
	unsigned nInd;
	if (nID == ID_IMAGEZOOM_ZOOMIN) {
		m_pImgDec->SetPreviewZoom(true,false,false,0);
		UpdateAllViews(NULL);
	} else if (nID == ID_IMAGEZOOM_ZOOMOUT) {
		m_pImgDec->SetPreviewZoom(false,true,false,0);
		UpdateAllViews(NULL);
	} else {
		nInd = nID - ID_IMAGEZOOM_12 + PRV_ZOOM_12;
		m_pImgDec->SetPreviewZoom(false,false,true,nInd);
		UpdateAllViews(NULL);
	}
}

// Menu enable status for Zoom
// - Supports a range in sub-items
//
void CJPEGsnoopDoc::OnUpdateZoomRng(CCmdUI* pCmdUI)
{
	unsigned nID;
	if (pCmdUI->m_nID == ID_IMAGEZOOM_ZOOMIN) {
		// No checkmark
	} else if (pCmdUI->m_nID == ID_IMAGEZOOM_ZOOMOUT) {
		// No checkmark
	} else {
		nID = pCmdUI->m_nID - ID_IMAGEZOOM_12 + PRV_ZOOM_12;
		unsigned nGetZoom;
		nGetZoom = m_pImgDec->GetPreviewZoomMode();
		unsigned nMenuOffset;
		nMenuOffset = pCmdUI->m_nID - ID_IMAGEZOOM_12;
		pCmdUI->SetCheck(m_pImgDec->GetPreviewZoomMode() == nID);
	}
}

// Menu command for Searching an Executable for a DQT table
// - File is already open (and DQT present)
// - Look in another file (exe) to see if a DQT exists
//
void CJPEGsnoopDoc::OnToolsSearchexecutablefordqt()
{

	// Don't need to do AnalyzeOpen() because a file pointer to
	// the executable is created here.

	bool bStatus = 0;
	TCHAR astrFilter[] =
		_T("Executable (*.exe)|*.exe|")\
		_T("DLL Library (*.dll)|*.dll|")\
		_T("All Files (*.*)|*.*||");

	CFileDialog FileDlg(TRUE, _T(".exe"), NULL, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST, astrFilter);

	CString strTitle;
	CString strFileName;
	CString strPathName;
	VERIFY(strTitle.LoadString(IDS_CAL_EXE_OPEN));
	FileDlg.m_ofn.lpstrTitle = strTitle;
	
	// Extend the maximum filename length
	// Default is 64 for filename, 260 for path
	// Some users have requested support for longer filenames
	LPTSTR	spFilePath;
	spFilePath = new TCHAR[501];
	spFilePath[0] = TCHAR(0);
	FileDlg.m_pOFN->lpstrFile = spFilePath;
	FileDlg.m_pOFN->nMaxFile = 500;
	FileDlg.m_pOFN->nMaxFileTitle = 300;

	if( FileDlg.DoModal() != IDOK )
	{
		return;
	}

	// We selected a file!
	strFileName = FileDlg.GetFileName();
	strPathName = FileDlg.GetPathName();

	CFile*	pFileExe;
	CString	strTmp;


	ASSERT(strFileName != _T(""));
	if (strFileName == _T("")) {
		if (m_pAppConfig->bInteractive)
			AfxMessageBox(_T("ERROR: SearchExe() but strFileName empty"));
		return;
	}

	try
	{
		// Open specified file
		pFileExe = new CFile(strPathName, CFile::modeRead | CFile::typeBinary | CFile::shareDenyNone);
	}
	catch (CFileException* e)
	{
		TCHAR msg[512];
		CString strError;
		e->GetErrorMessage(msg,sizeof(msg));
		strError.Format(_T("ERROR: Couldn't open file [%s]: [%s]"),
			(LPCTSTR)strFileName, (LPCTSTR)msg);
		if (m_pAppConfig->bInteractive)
			AfxMessageBox(strError);
		pFileExe = NULL;

		return; // FALSE;

	}

	// Set the file size variable
	unsigned long lFileSize = (unsigned long)pFileExe->GetLength();

	// Search the file!

	m_pLog->AddLineHdr(_T("*** Searching Executable for DQT ***"));
	strTmp.Format(_T("  Filename: [%s]"),(LPCTSTR)strPathName);
	m_pLog->AddLine(strTmp);
	strTmp.Format(_T("  Size:     [%lu]"),lFileSize);
	m_pLog->AddLine(strTmp);

	CwindowBuf*	pExeBuf;
	pExeBuf = new CwindowBuf();

	ASSERT(pExeBuf);
	if (!pExeBuf) {
		if (m_pAppConfig->bInteractive)
			AfxMessageBox(_T("ERROR: Not enough memory for EXE Search"));
		return;
	}

	pExeBuf->SetStatusBar(GetStatusBar());
	pExeBuf->BufFileSet(pFileExe);
	pExeBuf->BufLoadWindow(0);

	bool			bDoneFile = false;
	long			nFileInd = 0;
	unsigned		nEntriesWidth;
	bool			bEntriesByteSwap;
	unsigned long	nFoundPos=0;
	bool			bFound = false;
	bool			bFoundEntry = false;

	BYTE			abSearchMatrix[(64*4)+4];
	unsigned		nSearchMatrixLen;
	unsigned		nSearchSetMax;

	unsigned		nVal;

	bool			bAllSame = true;
	bool			bBaseline = true;

	nSearchSetMax = 1;
	if (m_bFileOpened) {
		nVal = m_pImgDec->GetDqtEntry(0,0);
		for (unsigned ind=1;ind<64;ind++) {
			if (m_pImgDec->GetDqtEntry(0,ind) != nVal) {
				bAllSame = false;
			}
			// Are there any DQT entries > 8 bits?
			if (m_pImgDec->GetDqtEntry(0,ind) > 255) {
				bBaseline = false;
			}

		}

		if (bAllSame) {
			strTmp.Format(_T("  NOTE: Because the JPEG's DQT Luminance table is constant nValue (0x%02X),_T("),nVal);
			m_pLog->AddLineWarn(strTmp);
			m_pLog->AddLineWarn(_T(")        matching for this table has been disabled."));
			m_pLog->AddLineWarn(_T("        Please select a different reference image."));
			nSearchSetMax = 1;
		} else {
			nSearchSetMax = 2;
		}
	}


	m_pLog->AddLine(_T("  Searching for DQT Luminance tables:"));
	for (unsigned nZigZagSet=0;nZigZagSet<2;nZigZagSet++) {
		strTmp.Format(_T("    DQT Ordering: %s"),(nZigZagSet?_T("post-zigzag"):_T("pre-zigzag")));
		m_pLog->AddLine(strTmp);
		for (unsigned nSearchSet=0;nSearchSet<nSearchSetMax;nSearchSet++) {
			if (nSearchSet==0) {
				strTmp.Format(_T("      Matching [JPEG Standard]"));
				m_pLog->AddLine(strTmp);
			} else {
				strTmp.Format(_T("      Matching [%s]"),(LPCTSTR)m_strPathName);
				m_pLog->AddLine(strTmp);
			}
	
			// Perform the search using a number of different variations on
			// byteswap and matrix element width
			for (unsigned nSearchPattern=0;nSearchPattern<5;nSearchPattern++) {
	
				switch(nSearchPattern) {
					case 0:
						nEntriesWidth = 1;
						bEntriesByteSwap = false;
						break;
					case 1:
						nEntriesWidth = 2;
						bEntriesByteSwap = false;
						break;
					case 2:
						nEntriesWidth = 2;
						bEntriesByteSwap = true;
						break;
					case 3:
						nEntriesWidth = 4;
						bEntriesByteSwap = false;
						break;
					case 4:
						nEntriesWidth = 4;
						bEntriesByteSwap = true;
						break;
					default:
						nEntriesWidth = 1;
						bEntriesByteSwap = false;
						break;
				}
				if (nEntriesWidth == 1) {
					strTmp.Format(_T("        Searching patterns with %u-byte DQT entries"),
						nEntriesWidth);
					m_pLog->AddLine(strTmp);
					if (!bBaseline) {
						strTmp.Format(_T("          DQT Table is not baseline, skipping 1-byte search"));
						m_pLog->AddLine(strTmp);
						continue;
					}
				} else {
					strTmp.Format(_T("        Searching patterns with %u-byte DQT entries%s"),
						nEntriesWidth,(bEntriesByteSwap)?_T(", endian byteswap"):_T(""));
					m_pLog->AddLine(strTmp);
				}
	
				nSearchMatrixLen = (nEntriesWidth*64);
	
				for (unsigned nInd=0;nInd<nSearchMatrixLen;nInd++) {
					abSearchMatrix[nInd] = 0;
				}
				for (unsigned nInd=0;nInd<64;nInd++) {
	
					// Select between the zig-zag and non-zig-zag element index
					unsigned nDqtInd = m_pJfifDec->GetDqtZigZagIndex(nInd,(nZigZagSet!=0));
					if (nSearchSet == 0) {
						// Standard JPEG table
						nVal = m_pJfifDec->GetDqtQuantStd(nDqtInd);
					} else {
						// Matching JPEG table
						nVal = m_pImgDec->GetDqtEntry(0,nDqtInd);
					}
	
					for (unsigned nEntryInd=0;nEntryInd<nEntriesWidth;nEntryInd++) {
						abSearchMatrix[(nInd*nEntriesWidth)+nEntryInd] = 0;
					}



					// Handle the various combinations of entry width, coefficient
					// width and byteswap modes. Expand out the combinations to 
					// ensure explicit array bounds are clear.
					if (nEntriesWidth == 1) {
						if (bBaseline) {
							abSearchMatrix[(nInd*1)] = nVal;
						} else {
							// Shouldn't get here. 2 byte value in 1 byte search
							// Supposed to exit earlier
							ASSERT(false);
						}
					} else if (nEntriesWidth == 2) {
						if (bBaseline) {
							if (bEntriesByteSwap) {
								abSearchMatrix[(nInd*2)+0] = (nVal & 0x00FF);
							} else {
								abSearchMatrix[(nInd*2)+(2-1)] = (nVal & 0x00FF);
							}
						} else {
							if (bEntriesByteSwap) {
								abSearchMatrix[(nInd*2)+0] = (nVal & 0x00FF);
								abSearchMatrix[(nInd*2)+1] = (nVal & 0xFF00) >> 8;
							} else {
								abSearchMatrix[(nInd*2)+(2-1)] = (nVal & 0x00FF);
								abSearchMatrix[(nInd*2)+(2-2)] = (nVal & 0xFF00) >> 8;
							}
						}
					} else if (nEntriesWidth == 4) {
						if (bBaseline) {
							if (bEntriesByteSwap) {
								abSearchMatrix[(nInd*4)+0] = (nVal & 0x00FF);
							} else {
								abSearchMatrix[(nInd*4)+(4-1)] = (nVal & 0x00FF);
							}
						} else {
							if (bEntriesByteSwap) {
								abSearchMatrix[(nInd*4)+0] = (nVal & 0x00FF);
								abSearchMatrix[(nInd*4)+1] = (nVal & 0xFF00) >> 8;
							} else {
								abSearchMatrix[(nInd*4)+(4-1)] = (nVal & 0x00FF);
								abSearchMatrix[(nInd*4)+(4-2)] = (nVal & 0xFF00) >> 8;
							}
						}
					}

				}
	
				bool bDoneAllMatches = false;
				unsigned long nStartOffset = 0;
				while (!bDoneAllMatches) {
	
					bFound = pExeBuf->BufSearchX(nStartOffset,abSearchMatrix,64*nEntriesWidth,true,nFoundPos);
					if (bFound) {
						strTmp.Format(_T("          Found @ 0x%08X"),nFoundPos);
						m_pLog->AddLineGood(strTmp);
						nStartOffset = nFoundPos+1;
					} else {
						bDoneAllMatches = true;
					}
				}
	
	
			} // nSearchPattern
		} // nSearchSet
	} // nZigZagSet

	m_pLog->AddLine(_T("  Done Search"));

	// Since this command can be run without a normal file open, we
	// need to flush the output if we happen to be in quick log mode
	InsertQuickLog();


	if (pExeBuf) {
		delete pExeBuf;
	}
}



// Menu enable status for File->Reprocess
//
void CJPEGsnoopDoc::OnUpdateFileReprocess(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_bFileOpened);
}

// Menu enable status for File->Save As
//
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
		AfxMessageBox(_T("NOTE: Attempt to overwrite source file prevented"));
		return true;
	}

	//return CRichEditDoc::OnSaveDocument(lpszPathName);

}




// Menu command to extract embedded JPEGs from a file
//
void CJPEGsnoopDoc::OnToolsExtractembeddedjpeg()
{

	// Run in interactive mode. DHT AVI param is ignored.
	DoExtractEmbeddedJPEG(true,false,_T(""));

}


// Extracts an embedded JPEG from the current file
// Also support extraction of all embedded JPEG from the file (default for non-interactive
// and optional in interactive mode).
//
// NOTE: In "Extract All" mode, all file types are processed.
//
// INPUT:
// - bInteractive		Ask user for output details (if true) or use defaults (if false)
// - bInsDhtAvi			Override DHT tables for AVI MJPEG extract
// - strOutPath			Output pathname to use in non-interactive mode. If empty, use same dir as original
//
void CJPEGsnoopDoc::DoExtractEmbeddedJPEG(bool bInteractive,bool bInsDhtAvi,CString strOutPath)
{
	bool			bAllOk = true;
	CExportDlg		dlgExport;
	CString			strTmp;
	unsigned int	nFileSize = 0;

	bool			bOverlayEn = false;
	bool			bForceSoi = false;
	bool			bForceEoi = false;
	bool			bIgnoreEoi = false;
	bool			bExtractAllEn = false;
	bool			bDhtAviInsert = false;
	bool			strOffsetStart = _T("");


	// TODO:
	// Move this configuration dialog out of this function
	// and into the caller. Then migrate the config into the param list.
	// This would enable us to support Batch Extract All while only
	// asking the user once about the Extract All configuration.
	if (bInteractive) {	
		bool	bIsAvi,bIsMjpeg;
		m_pJfifDec->GetAviMode(bIsAvi,bIsMjpeg);
		dlgExport.m_bOverlayEn = false;	// Changed default on 08/22/2011
		dlgExport.m_bForceSoi = false;
		dlgExport.m_bForceEoi = false;
		dlgExport.m_bIgnoreEoi = false;
		dlgExport.m_bExtractAllEn = false;
		dlgExport.m_bDhtAviInsert = bIsMjpeg;
		dlgExport.m_strOffsetStart = strTmp;

		if (dlgExport.DoModal() == IDOK) {
			// OK
			bExtractAllEn = (dlgExport.m_bExtractAllEn != 0);
			bForceSoi = (dlgExport.m_bForceSoi != 0);
			bForceEoi = (dlgExport.m_bForceEoi != 0);
			bIgnoreEoi = (dlgExport.m_bIgnoreEoi != 0);
			bOverlayEn = (dlgExport.m_bOverlayEn != 0);
			bDhtAviInsert = (dlgExport.m_bDhtAviInsert != 0);
		} else {
			bAllOk = false;
		}
	} else {
		bExtractAllEn = true;
		bForceSoi = false;
		bForceEoi = false;
		bIgnoreEoi = false;
		bOverlayEn = false;
		bDhtAviInsert = bInsDhtAvi;
	}

	if (bAllOk) {
		bool	bRet;

		// For export, we need to call AnalyzeOpen()
		AnalyzeOpen();

		strTmp.Format(_T("0x%08X"),m_pJfifDec->GetPosEmbedStart());
       
		// If we are not in "extract all" mode, then check now to see if
		// the current file position looks OK to extract a valid JPEG.
		// If we are in "extract all" mode, skip this step as we will
		// instead start with the search forward.

		if (!bExtractAllEn) {
			// Is the currently-decoded file in a suitable
			// state for JPEG extraction (ie. have we seen all
			// the necessary markers?)
			bRet = m_pJfifDec->ExportJpegPrepare(m_strPathName,bForceSoi,bForceEoi,bIgnoreEoi);
			if (!bRet) {
				return;
			}
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
		m_strPathName = m_strPathName + _T(".export.jpg");

		if (bInteractive) {
			TCHAR aszFilter[] =
				_T("JPEG Image (*.jpg)|*.jpg|")\
				_T("All Files (*.*)|*.*||");
	
			CFileDialog FileDlg(FALSE, _T(".jpg"), m_strPathName, OFN_OVERWRITEPROMPT, aszFilter);
	
			CString title;
			//VERIFY(title.LoadString(IDS_CAL_FILESAVE));
			title = _T("Save Exported JPEG file as");
			FileDlg.m_ofn.lpstrTitle = title;
			
			if( FileDlg.DoModal() == IDOK )
			{
				strEmbedFileName = FileDlg.GetPathName();
			} else {
				// User wanted to abort
				strTmp.Format(_T("  User cancelled"));
				m_pLog->AddLineErr(strTmp);

				// Revert
				m_strPathName = strPathName;
				m_bRTF = bRTF;

				bAllOk = false;
				return;
			}
		} else {
			// Non-interactive mode
			// Use default output filename if no override provided
			if (strOutPath.IsEmpty()) {
				strEmbedFileName = m_strPathName;
			} else {
				strEmbedFileName = strOutPath;
			}
		}

		// Revert
		m_strPathName = strPathName;
		m_bRTF = bRTF;

		// ----------------------------

		if (!bExtractAllEn) {

			// --------------------------------------------------------
			// Perform extraction of single embedded JPEG
			// --------------------------------------------------------

			if (m_lFileSize > 0xFFFFFFFFUL) {
				CString strTmp = _T("Extract file too large. Skipping.");
				m_pLog->AddLineErr(strTmp);
				if (m_pAppConfig->bInteractive)
					AfxMessageBox(strTmp);

			} else {
				nFileSize = static_cast<unsigned>(m_lFileSize);
				bRet = m_pJfifDec->ExportJpegDo(m_strPathName,strEmbedFileName,nFileSize,
					bOverlayEn,bDhtAviInsert,bForceSoi,bForceEoi);
			}
	
			// For export, we need to call AnalyzeClose()
			AnalyzeClose();

		} else {

			// --------------------------------------------------------
			// Perform batch extraction of all embedded JPEGs
			// --------------------------------------------------------

			// Start closed
			AnalyzeClose();

			bool		bDoneBatch = false;
			unsigned	nExportCnt = 1;

			unsigned		nStartPos = 0;
			bool			bSearchResult = false;
			unsigned long	nSearchPos = 0;
			bool			bSkipFrame = false;


			// Determine the root filename
			// Filename selected by user: strEmbedFileName
			CString		strRootFileName;
			int			nExtInd;
			
			nExtInd = strEmbedFileName.ReverseFind('.');
			ASSERT(nExtInd != -1);
			if (nExtInd == -1) {
				CString strTmp;
				strTmp.Format(_T("ERROR: Invalid filename [%s]"),(LPCTSTR)strEmbedFileName);
				m_pLog->AddLineErr(strTmp);
				if (m_pAppConfig->bInteractive)
					AfxMessageBox(strTmp);
				return;
			}

			strRootFileName = strEmbedFileName.Left(nExtInd);

			while (!bDoneBatch) {

				bSkipFrame = false;

				// Assumption is that we are already on first frame because of
				// the ExportJpegPrepare() call above

				// Create filename
				// 6 digits of numbering will support videos at 60 fps for over 4.5hrs.
				strEmbedFileName.Format(_T("%s.%06u.jpg"),(LPCTSTR)strRootFileName,nExportCnt);

				AnalyzeOpen();

				// Is the currently-decoded file in a suitable
				// state for JPEG extraction (ie. have we seen all
				// the necessary markers?)
				bRet = m_pJfifDec->ExportJpegPrepare(m_strPathName,bForceSoi,bForceEoi,bIgnoreEoi);
				if (!bRet) {
					// Skip this particular frame
					bSkipFrame = true;
				}

				if (!bSkipFrame) {

					if (m_lFileSize > 0xFFFFFFFFULL) {
						CString strTmp;
						strTmp.Format(_T("ERROR: Extract file too large. Skipping. [Size=0x%I64X]"), m_lFileSize);
						m_pLog->AddLineErr(strTmp);
						if (m_pAppConfig->bInteractive)
							AfxMessageBox(strTmp);
					} else {
						nFileSize = static_cast<unsigned>(m_lFileSize);
						bRet = m_pJfifDec->ExportJpegDo(m_strPathName,strEmbedFileName,nFileSize,
							bOverlayEn,bDhtAviInsert,bForceSoi,bForceEoi);
					}

					// Still increment frame # even if we skipped export (due to file size)
					nExportCnt++;
				}

				// See if we can find the next embedded JPEG (file must still be open for this)
				nStartPos = theApp.m_pAppConfig->nPosStart;
				m_pJfifDec->ImgSrcChanged();

				bSearchResult = m_pWBuf->BufSearch(nStartPos,0xFFD8FF,3,true,nSearchPos);
				if (bSearchResult) {
					theApp.m_pAppConfig->nPosStart = nSearchPos;
					Reprocess();
				} else {
					// No SOI Marker found in Forward search -> Stop batch
					bDoneBatch = true;
				}

				// For export, we need to call AnalyzeClose()
				AnalyzeClose();

			} // bDoneBatch

			// Close the file in case we aborted above
			AnalyzeClose();

		} // bExtractAllEn



	} // bAllOk

}



// Menu enable status for Tools -> Extract embedded JPEG
void CJPEGsnoopDoc::OnUpdateToolsExtractembeddedjpeg(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_bFileOpened);
}


// Menu command for configuring the file overlays
// - This method allows user to specify one or more bytes to
//   temporarily replace the currently active file processing
//
void CJPEGsnoopDoc::OnToolsFileoverlay()
{
	CString		strDlg;

	unsigned	nOffset;
	CString		strValNew;
	unsigned	nValLen;
	BYTE		anValData[16];

	bool		bCurEn;
	BYTE*		pCurData;
	unsigned	nCurLen;
	unsigned	nCurStart;
	CString		strCurDataHex;
	CString		strCurDataBin;
	CString		strTmp;
	CString		strTmpByte;
	unsigned	nTmpByte;

	// NOTE: This function assumes that we've previously opened a file
	// Otherwise, there isn't much point in setting the offset value
	// since it gets reset to 0 when we open a new file manually!

	bool bDone = false;
	while (!bDone) {

		strCurDataHex = _T("");
		strCurDataBin = _T("");

		if (AnalyzeOpen() == false) {
			return;
		}

		// Set overlay dialog starting values...
		bCurEn = m_pWBuf->OverlayGet(0,pCurData,nCurLen,nCurStart);
		// Were any overlays previously defined?
		if (!bCurEn) {
			// If not, don't try to read the buffer! Simply force string
			strCurDataHex = _T("");
			strCurDataBin = _T("");
			nCurStart = 0;
			nCurLen = 0;
		} else {
			for (unsigned nInd=0;nInd<nCurLen;nInd++) {
				strTmp.Format(_T("%02X"),pCurData[nInd]);
				strCurDataHex += strTmp;

				strCurDataBin += Dec2Bin(pCurData[nInd],8);
				strCurDataBin += _T(" ");
			}
		}

		// Determine binary form


		COverlayBufDlg dlgOverlay(NULL,m_pWBuf,bCurEn,nCurStart,nCurLen,strCurDataHex,strCurDataBin);

		if (dlgOverlay.DoModal() == IDOK) {

			// Convert and store the params
			nOffset  = dlgOverlay.m_nOffset;
			nValLen = dlgOverlay.m_nLen; // bits
			strValNew = dlgOverlay.m_sValueNewHex;

			for (unsigned nInd=0;nInd<_tcslen(strValNew)/2;nInd++)
			{
				strTmpByte = strValNew.Mid(nInd*2,2);
				nTmpByte = _tcstol(strTmpByte,NULL,16);
				anValData[nInd] = nTmpByte;
			}

			if (dlgOverlay.m_bEn) {
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

		if (!dlgOverlay.m_bApply) {
			bDone = true;
		}

	} // while
}


// Menu enable status for Tools -> File overlay
void CJPEGsnoopDoc::OnUpdateToolsFileoverlay(CCmdUI *pCmdUI)
{
	if (m_bFileOpened) {
		pCmdUI->Enable(true);
	} else {
		pCmdUI->Enable(false);
	}
}


// Menu command to look up an MCU's file offset
void CJPEGsnoopDoc::OnToolsLookupmcuoffset()
{
	// Don't need to use AnalyzeOpen() here as this function
	// simply checks the MCU map variable.

	unsigned nPicWidth,nPicHeight;
	m_pImgDec->GetImageSize(nPicWidth,nPicHeight);
	CLookupDlg lookupDlg(NULL,m_pImgDec,nPicWidth,nPicHeight);

	if (lookupDlg.DoModal() == IDOK) {
	} else {
	}
}

// Menu enable status for Tools -> Lookup MCU offset
void CJPEGsnoopDoc::OnUpdateToolsLookupmcuoffset(CCmdUI *pCmdUI)
{
	if (m_bFileOpened) {
		pCmdUI->Enable(true);
	} else {
		pCmdUI->Enable(false);
	}
}

// Menu command to enable/disable the MCU overlay grid
void CJPEGsnoopDoc::OnOverlaysMcugrid()
{
	m_pImgDec->SetPreviewOverlayMcuGridToggle();
	UpdateAllViews(NULL);
}


// Menu enable status for MCU overlay grid
void CJPEGsnoopDoc::OnUpdateOverlaysMcugrid(CCmdUI *pCmdUI)
{
	if (m_pImgDec->GetPreviewOverlayMcuGrid()) {
		pCmdUI->SetCheck(true);
	} else {
		pCmdUI->SetCheck(false);
	}
}





// Menu enable status for YCC indicator
void CJPEGsnoopDoc::OnUpdateIndicatorYcc(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(TRUE);
	pCmdUI->SetText(m_pImgDec->GetStatusYccText());
}

// Menu enable status for MCU indicator
void CJPEGsnoopDoc::OnUpdateIndicatorMcu(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(TRUE);
	pCmdUI->SetText(m_pImgDec->GetStatusMcuText());
}

// Menu enable status for file position indicator
void CJPEGsnoopDoc::OnUpdateIndicatorFilePos(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(TRUE);
	pCmdUI->SetText(m_pImgDec->GetStatusFilePosText());
}


// Menu command for Detailed Decode settings
// - Take last two image click positions (marker) to define the begin and
//   end of the detailed decode region
// - Or allow user to specify the starting MCU and length
// - Update the decoder ranging for the next processing operation
//
void CJPEGsnoopDoc::OnScansegmentDetaileddecode()
{
	CDecodeDetailDlg dlgDetail(NULL);
	
	bool	bEn;
	m_pImgDec->GetDetailVlc(bEn,dlgDetail.m_nMcuX,dlgDetail.m_nMcuY,dlgDetail.m_nMcuLen);
	dlgDetail.m_bEn = bEn;
	

	// Sequence of markers:
	//   #1 - Start MCU
	//   #2 - End MCU
	CPoint		ptScan1,ptScan2;

	unsigned	nStartMcuX=0;
	unsigned	nStartMcuY=0;
	unsigned	nEndMcuX=0;
	unsigned	nEndMcuY=0;
	int			nMcuRngLen=0;

	unsigned	nMarkerCnt;
	CPoint		ptMcuStart,ptMcuEnd;
	unsigned	nStartMcu,nEndMcu;

	nMarkerCnt = m_pImgDec->GetMarkerCount();

	// Double-point, gives range
	if (nMarkerCnt >= 2) {
		// Get last two block selections
		ptScan1 = m_pImgDec->GetMarkerBlk(nMarkerCnt-2);
		ptScan2 = m_pImgDec->GetMarkerBlk(nMarkerCnt-1);
		// Convert block index to MCU
		ptScan1.x *= BLK_SZ_X;
		ptScan1.y *= BLK_SZ_Y;
		ptScan2.x *= BLK_SZ_X;
		ptScan2.y *= BLK_SZ_Y;
		ptMcuStart	= m_pImgDec->PixelToMcu(ptScan1);
		ptMcuEnd	= m_pImgDec->PixelToMcu(ptScan2);
		nStartMcuX	= ptMcuStart.x;
		nStartMcuY	= ptMcuStart.y;
		nEndMcuX	= ptMcuEnd.x;
		nEndMcuY	= ptMcuEnd.y;

		// Check to see if order needs to be reversed
		if ( (nStartMcuY > nEndMcuY) || ( (nStartMcuY == nEndMcuY) && (nStartMcuX > nEndMcuX) ) ) {
			// Reverse order
			nStartMcu	= m_pImgDec->McuXyToLinear(ptMcuEnd);
			nEndMcu		= m_pImgDec->McuXyToLinear(ptMcuStart);
		} else {
			nStartMcu	= m_pImgDec->McuXyToLinear(ptMcuStart);
			nEndMcu		= m_pImgDec->McuXyToLinear(ptMcuEnd);
		}

		// Calculate length from linear positions
		nMcuRngLen = nEndMcu-nStartMcu + 1;

	// Single point, gives start point
	} else if (nMarkerCnt >= 1) {
		ptScan1 = m_pImgDec->GetMarkerBlk(nMarkerCnt-1);
		// Convert block index to MCU
		ptScan1.x *= BLK_SZ_X;
		ptScan1.y *= BLK_SZ_Y;
		ptMcuStart	= m_pImgDec->PixelToMcu(ptScan1);
		nStartMcuX	= ptMcuStart.x;
		nStartMcuY	= ptMcuStart.y;
		nMcuRngLen = 0;
	}

	// Set the "load" values from the last MCU click
	dlgDetail.m_nLoadMcuX = nStartMcuX;
	dlgDetail.m_nLoadMcuY = nStartMcuY;
	dlgDetail.m_nLoadMcuLen = nMcuRngLen;


	// If the user completed the configuration dialog then proceed
	// to update the MCU ranging for the next decode process
	if (dlgDetail.DoModal() == IDOK) {
		m_pImgDec->SetDetailVlc((dlgDetail.m_bEn!=0),dlgDetail.m_nMcuX,dlgDetail.m_nMcuY,dlgDetail.m_nMcuLen);
	}
}

// Menu enable status for Scan Segment -> Detailed Decode
void CJPEGsnoopDoc::OnUpdateScansegmentDetaileddecode(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(true);
}


// Menu command for Export to TIFF
// - Export the currently-decoded JPEG file as a TIFF
//
void CJPEGsnoopDoc::OnToolsExporttiff()
{

	// Get filename
	CString		strFnameOut;

	strFnameOut = m_strPathName + _T(".tif");

	TCHAR aszFilter[] =
		_T("TIFF (*.tif)|*.tif|")\
		_T("All Files (*.*)|*.*||");

	CFileDialog FileDlg(FALSE, _T(".tif"), strFnameOut, OFN_OVERWRITEPROMPT, aszFilter);

	CString strTitle;
	CString strFileName;
	strTitle = _T("Output TIFF Filename");
//	VERIFY(strTitle.LoadString(IDS_CAL_FILESAVE));
	FileDlg.m_ofn.lpstrTitle = strTitle;
	
	if( FileDlg.DoModal() == IDOK )
	{
		strFnameOut = FileDlg.GetPathName();
	} else {
		return;
	}



	FileTiff		myTiff;
	unsigned char*	pBitmapRgb = NULL;
	short*			pBitmapYccY = NULL;
	short*			pBitmapYccCb = NULL;
	short*			pBitmapYccCr = NULL;
	unsigned char*	pBitmapSel8 = NULL;
	unsigned short*	pBitmapSel16 = NULL;
	
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

	m_pImgDec->GetImageSize(nSizeX,nSizeY);
	m_pImgDec->GetBitmapPtr(pBitmapRgb);
	m_pImgDec->GetPixMapPtrs(pBitmapYccY,pBitmapYccCb,pBitmapYccCr);
	pBitmapSel8 = NULL;
	pBitmapSel16 = NULL;

	CExportTiffDlg	myTiffDlg;
	int rVal,nModeSel;
	myTiffDlg.m_sFname = strFnameOut;
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
		AfxMessageBox(_T("ERROR: Can't output 16-bit YCC!"));
		return;
	}

	// Create a separate bitmap array
	if (bMode16b) {
		pBitmapSel16 = new unsigned short[nSizeX*nSizeY*3];
		ASSERT(pBitmapSel16);
		if (!pBitmapSel16) {
			AfxMessageBox(_T("ERROR: Insufficient memory for export"));
			return;
		}
	} else {
		pBitmapSel8 = new unsigned char[nSizeX*nSizeY*3];
		ASSERT(pBitmapSel8);
		if (!pBitmapSel8) {
			AfxMessageBox(_T("ERROR: Insufficient memory for export"));
			return;
		}
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
					pBitmapSel8[nOffsetDst+0] = (nValR & 0x00FF);
					pBitmapSel8[nOffsetDst+1] = (nValG & 0x00FF);
					pBitmapSel8[nOffsetDst+2] = (nValB & 0x00FF);
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
					// Note that this error is trapped earlier
				}
			}
		}
	}

	if (bMode16b) {
		myTiff.WriteFile(strFnameOut,bModeYcc,bMode16b,(void*)pBitmapSel16,nSizeX,nSizeY);
	} else {
		myTiff.WriteFile(strFnameOut,bModeYcc,bMode16b,(void*)pBitmapSel8,nSizeX,nSizeY);
	}

	if (pBitmapSel8) {
		delete [] pBitmapSel8;
		pBitmapSel8 = NULL;
	}
	if (pBitmapSel16) {
		delete [] pBitmapSel16;
		pBitmapSel16 = NULL;
	}

}

// Menu enable status for Tools -> Export to TIFF
void CJPEGsnoopDoc::OnUpdateToolsExporttiff(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_bFileOpened);
}
