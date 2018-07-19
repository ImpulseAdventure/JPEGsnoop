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

// JPEGsnoopDoc.cpp : implementation of the CJPEGsnoopDoc class
//

#include "stdafx.h"
#include "JPEGsnoop.h"

#include "JPEGsnoopDoc.h"
#include "CntrItem.h"

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

	// Ideally this would be passed by constructor, but simply access
	// directly for now.
	CJPEGsnoopApp*	pApp;
	pApp = (CJPEGsnoopApp*)AfxGetApp();
	ASSERT(pApp);
    m_pAppConfig = pApp->m_pAppConfig;
	ASSERT(m_pAppConfig);

	if (DEBUG_EN) m_pAppConfig->DebugLogAdd(_T("CJPEGsnoopDoc::CJPEGsnoopDoc() Begin"));

	// Allocate the processing core
	m_pCore = new CJPEGsnoopCore;
	ASSERT(m_pCore);
	if (!m_pCore) {
		AfxMessageBox(_T("ERROR: Not enough memory for Processing Core"));
		exit(1);
	}

	// Setup link to CDocument for document log
	ASSERT(glb_pDocLog);
	glb_pDocLog->SetDoc(this);
	if (DEBUG_EN) m_pAppConfig->DebugLogAdd(_T("CJPEGsnoopDoc::CJPEGsnoopDoc() Checkpoint 1"));
	
    // Reset all members
	Reset();

	if (DEBUG_EN) m_pAppConfig->DebugLogAdd(_T("CJPEGsnoopDoc::CJPEGsnoopDoc() Checkpoint 5"));

	if (DEBUG_EN) m_pAppConfig->DebugLogAdd(_T("CJPEGsnoopDoc::CJPEGsnoopDoc() End"));
}

// Cleanup all of the allocated classes
CJPEGsnoopDoc::~CJPEGsnoopDoc()
{

	if (m_pCore != NULL) {
		delete m_pCore;
		m_pCore = NULL;
	}

	// Sever link to CDocument in log
	ASSERT(glb_pDocLog);
	glb_pDocLog->SetDoc(NULL);

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
	m_strPathNameOpened = _T("");

	// Indicate to JFIF ProcessFile() that document has changed
	// and that the scan decode needs to be redone if it
	// is to be displayed.
	m_pCore->J_ImgSrcChanged();

	// Clean up the quick log
	glb_pDocLog->Clear();

}

// NOTE
// Currently, the status bar assignment is done in both the
// OnNewDocument() and OnOpenDocument(). There is probably a
// single entrypoint that would be more suitable to use (one
// that guarantees that the window has been created, not in
// the constructor).


// Main entry point for creation of a new document
BOOL CJPEGsnoopDoc::OnNewDocument()
{
	if (DEBUG_EN) m_pAppConfig->DebugLogAdd(_T("CJPEGsnoopDoc::OnNewDocument() Start"));
	if (!CRichEditDoc::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)
	Reset();

	// ---------------------------------------
	// Get the status bar and configure the decoder to link up to it
	CStatusBar* pStatBar;
	pStatBar = GetStatusBar();

	// Hook up the status bar
	m_pCore->J_SetStatusBar(pStatBar);
	m_pCore->I_SetStatusBar(pStatBar);
	// ---------------------------------------

	if (DEBUG_EN) m_pAppConfig->DebugLogAdd(_T("CJPEGsnoopDoc::OnNewDocument() End"));

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
// - m_pView (view from RichEdit must already be established)
//
int CJPEGsnoopDoc::AppendToLog(CString strTxt, COLORREF sColor)
{

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

	return 0;
}

// Call RedrawWindow() on RichEdit window
void CJPEGsnoopDoc::RedrawLog()
{
	ASSERT(m_pView);
	if (!m_pView) return;
	CRichEditCtrl* pCtrl = &m_pView->GetRichEditCtrl();
	ASSERT(pCtrl);

	pCtrl->RedrawWindow();
}

// Transfer the contents of the QuickLog buffer to the RichEdit control
// for display.
//
// In command-line nogui mode we skip this step as there may not be a
// RichEdit control assigned (m_pView will be NULL). Instead, we will
// later call DoLogSave() to release the buffer.
//
// PRE:
// - m_pView (view from RichEdit must already be established)
//
int CJPEGsnoopDoc::InsertQuickLog()
{

	// Assume we are in GUI mode
	if (!theApp.m_pAppConfig->bGuiMode) {
		ASSERT(false);
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
	unsigned nQuickLines = glb_pDocLog->GetNumLinesLocal();
	COLORREF nCurCol = RGB(0,0,0);
	COLORREF nLastCol = RGB(255,255,255);

	for (unsigned nLine=0;nLine<nQuickLines;nLine++)
	{
		CString		strTxt;
		COLORREF	sCol;
		glb_pDocLog->GetLineLogLocal(nLine,strTxt,sCol);
		nCurCol = sCol;

		if (nCurCol != nLastCol) {
			// Initialize character format structure
			cf.cbSize = sizeof(CHARFORMAT);
			cf.dwMask = CFM_COLOR;
			cf.dwEffects = 0; // To disable CFE_AUTOCOLOR
			cf.crTextColor = nCurCol;
			// Set the character format
			pCtrl->SetSelectionCharFormat(cf);
		}
		pCtrl->ReplaceSel(strTxt);

	}
	pCtrl->SetRedraw(true);
	pCtrl->RedrawWindow();
 
	// Empty the quick log since we've used it now
	glb_pDocLog->Clear();
  
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

// NOTE:
// - When calling AnalyzeClose() from CDocument, may need to address the following:
//     // Mark the doc as clean so that we don't get questioned to save anytime
//     // we change the file or quit.
//     SetModifiedFlag(false);


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
// - strBatchDir		The root folder of the batch process
// - bRecSubdir			Flag to descend recursively into sub-directories
// - bExtractAll		Flag to extract all JPEGs from files
//
void CJPEGsnoopDoc::DoBatchProcess(CString strBatchDir,bool bRecSubdir,bool bExtractAll)
{
	bRecSubdir;		// Unreferenced param

	CFolderDialog		myFolderDlg(NULL);
	CString				strRootDir;
	CString				strDir;
	bool				bSubdirs = false;

	CString				strDirSrc;
	CString				strDirDst;
	
	// Bring up dialog to select subdir recursion
	bool		bAllOk= true;

	// Here is where we ask about recursion
	// We also ask whether "extract all" mode is desired
	CBatchDlg myBatchDlg;
	myBatchDlg.m_bProcessSubdir = false;
	myBatchDlg.m_strDirSrc = m_pAppConfig->strBatchLastInput;
	myBatchDlg.m_strDirDst = m_pAppConfig->strBatchLastOutput;
	myBatchDlg.m_bExtractAll = bExtractAll;
	if (myBatchDlg.DoModal() == IDOK) {
		// Fetch the settings from the dialog
		bSubdirs = (myBatchDlg.m_bProcessSubdir != 0);
		bExtractAll = (myBatchDlg.m_bExtractAll != 0);
		strDirSrc = myBatchDlg.m_strDirSrc;
		strDirDst = myBatchDlg.m_strDirDst;
		// Update the config
		m_pAppConfig->strBatchLastInput = myBatchDlg.m_strDirSrc;
		m_pAppConfig->strBatchLastOutput = myBatchDlg.m_strDirDst;
		m_pAppConfig->Dirty(true);
	} else {
		bAllOk = false;
	}

	// In Batch Processing mode, we'll temporarily turn off the Interactive mode
	// so that any errors that arise in decoding are only output to the log files
	// and not to an alert dialog box.
	bool	bInteractiveSaved = m_pAppConfig->bInteractive;
	m_pAppConfig->bInteractive = false;

	if (bAllOk) {


		// Allocate core engine
		CJPEGsnoopCore* pSnoopCore = NULL;
		pSnoopCore = new CJPEGsnoopCore;
		ASSERT(pSnoopCore);
		if (!pSnoopCore) {
			return;
		}

		// Enable and hook up the status bar
		CStatusBar* pStatBar;
		pStatBar = GetStatusBar();
		pSnoopCore->SetStatusBar(pStatBar);

		// Indicate long operation ahead!
		CWaitCursor wc;

		// TODO: Add a "Cancel Dialog" for the batch operation
		// Example code that I can use for single-stepping the operation:
		//
		// // === START
		// COperationDlg LengthyOp(this);
		// LengthyOp.SetFunctions( PrepareOperation, NextIteration, GetProgress );
		//
		// // Until-done based
		// BOOL bOk = LengthyOp.RunUntilDone( true );
		// // === END


		// Generate the batch file list
		pSnoopCore->GenBatchFileList(strDirSrc,strDirDst,bSubdirs,bExtractAll);

		// Now that we've created the list of files, start processing them
		unsigned	nBatchFileCount;
		nBatchFileCount = pSnoopCore->GetBatchFileCount();

		// TODO: Clear the current RichEdit log
		DeleteContents();

		CString		strStat;
		for (unsigned nFileInd=0;nFileInd<nBatchFileCount;nFileInd++) {
			// Update the status report in window
			CString		strSrcFname;
			strSrcFname = pSnoopCore->GetBatchFileInfo(nFileInd);
			strStat.Format(_T("Batch processing file %6u of %6u (%6.2f %%): [%s]"),
				nFileInd+1,nBatchFileCount,(100.0*(nFileInd+1)/nBatchFileCount),
				(LPCTSTR)strSrcFname);

			// Inject progress message into main log window
			AppendToLog(strStat+_T("\n"),RGB(1,1,1));
			RedrawLog();

			// Process file
			pSnoopCore->DoBatchFileProcess(nFileInd,true,bExtractAll);
		}

		AppendToLog(_T("Batch processing complete"),RGB(1,255,1));
		RedrawLog();

		// TODO: Add the most recent file to the current RichEdit log

		// TODO: Clean up after last log output

		// Alert the user that we are done
		// TODO: Is this the right flag to check?
		if (bInteractiveSaved) {
			AfxMessageBox(_T("Batch Processing Complete!"));
		}

		// Deallocate the core
		if (pSnoopCore) {
			delete pSnoopCore;
			pSnoopCore = NULL;
		}

	}

	// Restore the Interactive mode setting
	m_pAppConfig->bInteractive = bInteractiveSaved;

}


// --------------------------------------------------------------------
// --- END OF BATCH PROCESSING
// --------------------------------------------------------------------



// Menu command for specifying a file offset to start decoding process
// - Present a dialog box that allows user to specify the address
//
// TODO: Perhaps this menu item should be disabled when no file has been analyzed
//
void CJPEGsnoopDoc::OnFileOffset()
{
	COffsetDlg offsetDlg;

	// NOTE: This function assumes that we've previously opened a file
	// Otherwise, there isn't much point in setting the offset value
	// since it gets reset to 0 when we open a new file manually!

	if (!m_pCore->IsAnalyzed()) {
		return;
	}
	if (!m_pCore->AnalyzeOpen()) {
		return;
	}

	offsetDlg.SetOffset(theApp.m_pAppConfig->nPosStart);
	if (offsetDlg.DoModal() == IDOK) {
		theApp.m_pAppConfig->nPosStart = offsetDlg.GetOffset();

		// Clean out document first (especially buffer)
		DeleteContents();
		// Process the file from new offset
		m_pCore->AnalyzeFileDo();
		InsertQuickLog();

		// Indicate that scan data needs to be re-analyzed
		m_pCore->J_ImgSrcChanged();

		// Now try to update all views. This is particularly important
		// for View 2 (ScrollView) as it does not automatically invalidate
		UpdateAllViews(NULL);	
	}

	if (m_pCore->IsAnalyzed()) {
		m_pCore->AnalyzeClose();
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

	m_pCore->J_GetDecodeSummary(strDecHash,strDecHashRot,strDecImgExifMake,strDecImgExifModel,strDecImgQualExif,strDecSoftware,eDecDbReqSuggest);

	if (strDecHash == _T("NONE"))
	{
		// No valid signature, can't submit!
		CString strTmp = _T("No valid signature could be created, so DB submit is temporarily disabled");
		glb_pDocLog->AddLineErr(strTmp);
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

		// Prior to v1.8.0 the nUserSrcPre was compared against the ENUM_SOURCE_* values
		// - This led to an incorrect mapping to the database value
		// - As of v1.8.0 (and DB signature version "03"), this has been corrected
		//   and a "js_vers" parameter is now passed to the external DB to help resolve
		//   the error for older versions.
		switch (nUserSrcPre) {
			case 0:
				eUserSrc = ENUM_SOURCE_CAM;
				break;
			case 1:
				eUserSrc = ENUM_SOURCE_SW;
				break;
			case 2:
				eUserSrc = ENUM_SOURCE_UNSURE;
				break;
			default:
				eUserSrc = ENUM_SOURCE_UNSURE;
				break;

		}

		m_pCore->J_PrepareSendSubmit(strQual,eUserSrc,strUserSoftware,strUserNotes);
	}
}

// Menu enable status for Tools -> Add camera to database
//
void CJPEGsnoopDoc::OnUpdateToolsAddcameratodb(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pCore->IsAnalyzed());
}

// Menu command for searching forward in file for JPEG
// - The search process looks for the JFIF_SOI marker
//
void CJPEGsnoopDoc::OnToolsSearchforward()
{
	// Search for start:
	unsigned long	nSearchPos = 0;
	unsigned long	nStartPos;
	bool			bSearchResult;
	CString			strTmp;

	// Get status bar pointer
	CStatusBar* pStatBar;
	pStatBar = GetStatusBar();

	// NOTE: m_bFileAnalyzed doesn't actually refer to underlying file....
	//  so therefore it is not reliable.

	// If file got renamed or moved, AnalyzeOpen() will return false
	if (m_pCore->AnalyzeOpen() == false) {
		return;
	}

	nStartPos = theApp.m_pAppConfig->nPosStart;
	m_pCore->J_ImgSrcChanged();

	// Update status bar
	if (pStatBar) {
		strTmp.Format(_T("Searching forward..."));
		pStatBar->SetPaneText(0,strTmp);
	}

	m_pCore->B_SetStatusBar(GetStatusBar());
	bSearchResult = m_pCore->B_BufSearch(nStartPos,0xFFD8FF,3,true,nSearchPos);

	// Update status bar
	if (pStatBar) {
		strTmp.Format(_T("Done"));
		pStatBar->SetPaneText(0,strTmp);
	}

	if (bSearchResult) {
		theApp.m_pAppConfig->nPosStart = nSearchPos;
		Reprocess();
	} else {
		if (m_pAppConfig->bInteractive)
			AfxMessageBox(_T("No SOI Marker found in Forward search"));
	}
	m_pCore->AnalyzeClose();

}

// Menu command for searching reverse in file for JPEG image
// - The search process looks for the JFIF_SOI marker
//
void CJPEGsnoopDoc::OnToolsSearchreverse()
{
	// Search for start:

	unsigned long	nSearchPos = 0;
	unsigned long	nStartPos;
	bool			bSearchResult;
	CString			strTmp;

	// Get status bar pointer
	CStatusBar* pStatBar;
	pStatBar = GetStatusBar();

	// If file got renamed or moved, AnalyzeOpen() will return false
	if (m_pCore->AnalyzeOpen() == false) {
		return;
	}

	nStartPos = theApp.m_pAppConfig->nPosStart;
	m_pCore->J_ImgSrcChanged();

	// Don't attempt to search past start of file
	if (nStartPos > 0) {

		// Update status bar
		if (pStatBar) {
			strTmp.Format(_T("Searching reverse..."));
			pStatBar->SetPaneText(0,strTmp);
		}

		m_pCore->B_SetStatusBar(GetStatusBar());
		bSearchResult = m_pCore->B_BufSearch(nStartPos,0xFFD8FF,3,false,nSearchPos);

		// Update status bar
		if (pStatBar) {
			strTmp.Format(_T("Done"));
			pStatBar->SetPaneText(0,strTmp);
		}

		if (bSearchResult) {
			theApp.m_pAppConfig->nPosStart = nSearchPos;
			Reprocess();
		} else {
			theApp.m_pAppConfig->nPosStart = 0;
			if (m_pAppConfig->bInteractive)
				AfxMessageBox(_T("No SOI Marker found in Reverse search"));
			Reprocess();
		}
	}
	m_pCore->AnalyzeClose();

}

// Menu enable status for Tools -> Search forward
//
void CJPEGsnoopDoc::OnUpdateToolsSearchforward(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pCore->IsAnalyzed());
}

// Menu enable status for Tools -> Search reverse
//
void CJPEGsnoopDoc::OnUpdateToolsSearchreverse(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pCore->IsAnalyzed());
}

// Reprocess the current file
// - This command will initiate all processing of the currently-opened file
// - Show a coach message for first time ever
// - Update both views (invalidate and redraw)
//
BOOL CJPEGsnoopDoc::Reprocess()
{
	BOOL	bRet = false;

	// ------------------------------------------------
	// Give coach message
	// ------------------------------------------------

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

	// ------------------------------------------------
	// Reprocess file
	// ------------------------------------------------

	// Clean out document first (especially buffer)
	DeleteContents();
	// Now we can reprocess the file
	bRet = m_pCore->AnalyzeFile(m_strPathName);
	// Insert the log
	InsertQuickLog();

	// ------------------------------------------------
	// Update views
	// ------------------------------------------------

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

	return bRet;
}



// Menu command to reprocess the current file
// - This command will initiate all processing of the currently-opened file
//
void CJPEGsnoopDoc::OnFileReprocess()
{
	m_pCore->J_ImgSrcChanged();
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

	// ---------------------------------------
	// Get the status bar and configure the decoder to link up to it
	CStatusBar* pStatBar;
	pStatBar = GetStatusBar();

	// Hook up the status bar
	m_pCore->J_SetStatusBar(pStatBar);
	m_pCore->I_SetStatusBar(pStatBar);
	// ---------------------------------------

	// Now process the file!
	bStatus = Reprocess();

	return bStatus;
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

	//bool status = 0;
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
	m_pCore->I_SetPreviewMode(nInd);
	UpdateAllViews(NULL);
}

// Menu enable status for Channel Preview
// - Supports a range in sub-items
//
void CJPEGsnoopDoc::OnUpdatePreviewRng(CCmdUI* pCmdUI)
{
	unsigned nID;
	nID = pCmdUI->m_nID - ID_PREVIEW_RGB + PREVIEW_RGB;
	if (m_pCore->I_GetPreviewMode() == nID) {
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
		m_pCore->I_SetPreviewZoom(true,false,false,0);
		UpdateAllViews(NULL);
	} else if (nID == ID_IMAGEZOOM_ZOOMOUT) {
		m_pCore->I_SetPreviewZoom(false,true,false,0);
		UpdateAllViews(NULL);
	} else {
		nInd = nID - ID_IMAGEZOOM_12 + PRV_ZOOM_12;
		m_pCore->I_SetPreviewZoom(false,false,true,nInd);
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
		nGetZoom = m_pCore->I_GetPreviewZoomMode();
		unsigned nMenuOffset;
		nMenuOffset = pCmdUI->m_nID - ID_IMAGEZOOM_12;
		pCmdUI->SetCheck(m_pCore->I_GetPreviewZoomMode() == nID);
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

	//bool bStatus = 0;
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
		TCHAR msg[MAX_BUF_EX_ERR_MSG];
		CString strError;
		e->GetErrorMessage(msg,MAX_BUF_EX_ERR_MSG);
		e->Delete();
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

	glb_pDocLog->AddLineHdr(_T("*** Searching Executable for DQT ***"));
	strTmp.Format(_T("  Filename: [%s]"),(LPCTSTR)strPathName);
	glb_pDocLog->AddLine(strTmp);
	strTmp.Format(_T("  Size:     [%lu]"),lFileSize);
	glb_pDocLog->AddLine(strTmp);

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

	//bool			bDoneFile = false;
	//long			nFileInd = 0;
	unsigned		nEntriesWidth;
	bool			bEntriesByteSwap;
	unsigned long	nFoundPos=0;
	bool			bFound = false;
	//bool			bFoundEntry = false;

	BYTE			abSearchMatrix[(64*4)+4];
	unsigned		nSearchMatrixLen;
	unsigned		nSearchSetMax;

	unsigned		nVal;

	bool			bAllSame = true;
	bool			bBaseline = true;

	nSearchSetMax = 1;
	if (m_pCore->IsAnalyzed()) {
		nVal = m_pCore->I_GetDqtEntry(0,0);
		for (unsigned ind=1;ind<64;ind++) {
			if (m_pCore->I_GetDqtEntry(0,ind) != nVal) {
				bAllSame = false;
			}
			// Are there any DQT entries > 8 bits?
			if (m_pCore->I_GetDqtEntry(0,ind) > 255) {
				bBaseline = false;
			}

		}

		if (bAllSame) {
			strTmp.Format(_T("  NOTE: Because the JPEG's DQT Luminance table is constant nValue (0x%02X),_T("),nVal);
			glb_pDocLog->AddLineWarn(strTmp);
			glb_pDocLog->AddLineWarn(_T(")        matching for this table has been disabled."));
			glb_pDocLog->AddLineWarn(_T("        Please select a different reference image."));
			nSearchSetMax = 1;
		} else {
			nSearchSetMax = 2;
		}
	}


	glb_pDocLog->AddLine(_T("  Searching for DQT Luminance tables:"));
	for (unsigned nZigZagSet=0;nZigZagSet<2;nZigZagSet++) {
		strTmp.Format(_T("    DQT Ordering: %s"),(nZigZagSet?_T("post-zigzag"):_T("pre-zigzag")));
		glb_pDocLog->AddLine(strTmp);
		for (unsigned nSearchSet=0;nSearchSet<nSearchSetMax;nSearchSet++) {
			if (nSearchSet==0) {
				strTmp.Format(_T("      Matching [JPEG Standard]"));
				glb_pDocLog->AddLine(strTmp);
			} else {
				strTmp.Format(_T("      Matching [%s]"),(LPCTSTR)m_strPathName);
				glb_pDocLog->AddLine(strTmp);
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
					glb_pDocLog->AddLine(strTmp);
					if (!bBaseline) {
						strTmp.Format(_T("          DQT Table is not baseline, skipping 1-byte search"));
						glb_pDocLog->AddLine(strTmp);
						continue;
					}
				} else {
					strTmp.Format(_T("        Searching patterns with %u-byte DQT entries%s"),
						nEntriesWidth,(bEntriesByteSwap)?_T(", endian byteswap"):_T(""));
					glb_pDocLog->AddLine(strTmp);
				}
	
				nSearchMatrixLen = (nEntriesWidth*64);
	
				for (unsigned nInd=0;nInd<nSearchMatrixLen;nInd++) {
					abSearchMatrix[nInd] = 0;
				}
				for (unsigned nInd=0;nInd<64;nInd++) {
	
					// Select between the zig-zag and non-zig-zag element index
					unsigned nDqtInd = m_pCore->J_GetDqtZigZagIndex(nInd,(nZigZagSet!=0));
					if (nSearchSet == 0) {
						// Standard JPEG table
						nVal = m_pCore->J_GetDqtQuantStd(nDqtInd);
					} else {
						// Matching JPEG table
						nVal = m_pCore->I_GetDqtEntry(0,nDqtInd);
					}
	
					for (unsigned nEntryInd=0;nEntryInd<nEntriesWidth;nEntryInd++) {
						abSearchMatrix[(nInd*nEntriesWidth)+nEntryInd] = 0;
					}



					// Handle the various combinations of entry width, coefficient
					// width and byteswap modes. Expand out the combinations to 
					// ensure explicit array bounds are clear.
					if (nEntriesWidth == 1) {
						if (bBaseline) {
							abSearchMatrix[(nInd*1)] = (nVal & 0xFF);
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
						glb_pDocLog->AddLineGood(strTmp);
						nStartOffset = nFoundPos+1;
					} else {
						bDoneAllMatches = true;
					}
				}
	
	
			} // nSearchPattern
		} // nSearchSet
	} // nZigZagSet

	glb_pDocLog->AddLine(_T("  Done Search"));

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
	pCmdUI->Enable(m_pCore->IsAnalyzed());
}

// Menu enable status for File->Save As
//
void CJPEGsnoopDoc::OnUpdateFileSaveAs(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pCore->IsAnalyzed());
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
	DoGuiExtractEmbeddedJPEG();
}


// Extracts an embedded JPEG from the current file
// Also support extraction of all embedded JPEG from the file (default for non-interactive
// and optional in interactive mode).
//
// NOTE: In "Extract All" mode, all file types are processed.
//
void CJPEGsnoopDoc::DoGuiExtractEmbeddedJPEG()
{
	bool			bAllOk = true;
	CExportDlg		dlgExport;
	CString			strTmp;
	//unsigned int	nFileSize = 0;

	// Original function params
	//bool			bInteractive = true;
	//bool			bInsDhtAvi = false;
	CString			strOutPath = _T("");

	bool			bOverlayEn = false;
	bool			bForceSoi = false;
	bool			bForceEoi = false;
	bool			bIgnoreEoi = false;
	bool			bExtractAllEn = false;
	bool			bDhtAviInsert = false;
	CString			strOffsetStart = _T("");

	// Get operation configuration from user
	// ------------------------------------------

	// Determine if file is AVI
	// If so, default to DHT AVI insert mode
	// NOTE: Calling GetAviMode() requires that the CDocument JFIF decoder
	//       instance has processed the file.
	bool	bIsAvi,bIsMjpeg;
	m_pCore->J_GetAviMode(bIsAvi,bIsMjpeg);
	dlgExport.m_bDhtAviInsert = bIsMjpeg;

	dlgExport.m_bOverlayEn = false;	// Changed default on 08/22/2011
	dlgExport.m_bForceSoi = false;
	dlgExport.m_bForceEoi = false;
	dlgExport.m_bIgnoreEoi = false;
	dlgExport.m_bExtractAllEn = false;
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


	// Confirm output filename
	// ------------------------------------------

	CString		strInputFname;
	CString		strExportFname;

	// Preserve CDocument settings
	BOOL		bSavedRTF = m_bRTF;

	// Override the RTF setting
	m_bRTF = false;

	// Generate default export filename
	strInputFname = m_strPathName;
	strExportFname = strInputFname + _T(".export.jpg");

	// Save File Dialog
	TCHAR aszFilter[] =
		_T("JPEG Image (*.jpg)|*.jpg|")\
		_T("All Files (*.*)|*.*||");

	CFileDialog FileDlg(FALSE, _T(".jpg"), strExportFname, OFN_OVERWRITEPROMPT, aszFilter);

	CString title;
	//VERIFY(title.LoadString(IDS_CAL_FILESAVE));
	title = _T("Save Exported JPEG file as");
	FileDlg.m_ofn.lpstrTitle = title;
	
	INT_PTR nDlgResult = FileDlg.DoModal();
	if (nDlgResult == IDOK) {
		// Update the export filename
		strExportFname = FileDlg.GetPathName();
	}

	// Now revert the CDocument members
	m_bRTF = bSavedRTF;

	if (nDlgResult != IDOK) {
		// User wanted to abort
		strTmp.Format(_T("  User cancelled"));
		glb_pDocLog->AddLineErr(strTmp);

		bAllOk = false;
		return;
	}


	// Extract file operation
	// ------------------------------------------

	// Clear the log as DoExtractEmbeddedJPEG will re-analyze file into log
	DeleteContents();

	m_pCore->DoExtractEmbeddedJPEG(strInputFname,strExportFname,bOverlayEn,bForceSoi,bForceEoi,bIgnoreEoi,bExtractAllEn,bDhtAviInsert,strOutPath);

	// As the extraction adds some status lines, update the log
	InsertQuickLog();

}



// Menu enable status for Tools -> Extract embedded JPEG
void CJPEGsnoopDoc::OnUpdateToolsExtractembeddedjpeg(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pCore->IsAnalyzed());
}

// Create static wrapper for B_Buf callback function
BYTE CJPEGsnoopDoc::CbWrap_B_Buf(void* pWrapClass,unsigned long nNum,bool bBool)
{
	CJPEGsnoopDoc* mySelf = (CJPEGsnoopDoc*) pWrapClass;
	return mySelf->m_pCore->B_Buf(nNum,bBool);
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

		if (m_pCore->AnalyzeOpen() == false) {
			return;
		}

		// Set overlay dialog starting values...
		bCurEn = m_pCore->B_OverlayGet(0,pCurData,nCurLen,nCurStart);
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

		COverlayBufDlg dlgOverlay(NULL,bCurEn,nCurStart,nCurLen,strCurDataHex,strCurDataBin);
		// Set the call-back function
		dlgOverlay.SetCbBuf((void*)this,CJPEGsnoopDoc::CbWrap_B_Buf);

		if (dlgOverlay.DoModal() == IDOK) {

			// Convert and store the params
			nOffset  = dlgOverlay.m_nOffset;
			nValLen = dlgOverlay.m_nLen; // bits
			strValNew = dlgOverlay.m_sValueNewHex;

			for (unsigned nInd=0;nInd<_tcslen(strValNew)/2;nInd++)
			{
				strTmpByte = strValNew.Mid(nInd*2,2);
				nTmpByte = _tcstoul(strTmpByte,NULL,16);
				anValData[nInd] = (nTmpByte & 0xFF);
			}

			if (dlgOverlay.m_bEn) {
				// Note that we don't know the actual MCU, so
				// we'll just leave it at MCU [0,0] for now.
				// We can distinguish by nMcuLen = 0 (custom).
				m_pCore->B_OverlayInstall(0,anValData,nValLen,nOffset,
					0,0,0,0,0,0,0);
			} else {
				// Assume that if I'm using this dialog method to access the overlay
				// that I'm only ever using a single overlay! Therefore, to be safe
				// I'm going to call OverlayRemoveAll() instead of OverlayRemove() (which just
				// deletes last one).
				m_pCore->B_OverlayRemoveAll();
			}

			m_pCore->J_ImgSrcChanged();

			Reprocess();

		} else {
		}

		m_pCore->AnalyzeClose();

		if (!dlgOverlay.m_bApply) {
			bDone = true;
		}

	} // while
}


// Menu enable status for Tools -> File overlay
void CJPEGsnoopDoc::OnUpdateToolsFileoverlay(CCmdUI *pCmdUI)
{
	if (m_pCore->IsAnalyzed()) {
		pCmdUI->Enable(true);
	} else {
		pCmdUI->Enable(false);
	}
}

// Create static wrapper for I_LookupFilePosPix callback function
void CJPEGsnoopDoc::CbWrap_I_LookupFilePosPix(void* pWrapClass,
											  unsigned int nX, unsigned int nY, unsigned int &nByte, unsigned int &nBit)
{
	CJPEGsnoopDoc* mySelf = (CJPEGsnoopDoc*) pWrapClass;
	return mySelf->m_pCore->I_LookupFilePosPix(nX,nY,nByte,nBit);
}

// Menu command to look up an MCU's file offset
void CJPEGsnoopDoc::OnToolsLookupmcuoffset()
{
	// Don't need to use AnalyzeOpen() here as this function
	// simply checks the MCU map variable.

	unsigned nPicWidth,nPicHeight;
	m_pCore->I_GetImageSize(nPicWidth,nPicHeight);
	CLookupDlg lookupDlg(NULL,nPicWidth,nPicHeight);
	// Set the call-back function
	lookupDlg.SetCbLookup((void*)this,CJPEGsnoopDoc::CbWrap_I_LookupFilePosPix);

	if (lookupDlg.DoModal() == IDOK) {
	} else {
	}
}

// Menu enable status for Tools -> Lookup MCU offset
void CJPEGsnoopDoc::OnUpdateToolsLookupmcuoffset(CCmdUI *pCmdUI)
{
	if (m_pCore->IsAnalyzed()) {
		pCmdUI->Enable(true);
	} else {
		pCmdUI->Enable(false);
	}
}

// Menu command to enable/disable the MCU overlay grid
void CJPEGsnoopDoc::OnOverlaysMcugrid()
{
	m_pCore->I_SetPreviewOverlayMcuGridToggle();
	UpdateAllViews(NULL);
}


// Menu enable status for MCU overlay grid
void CJPEGsnoopDoc::OnUpdateOverlaysMcugrid(CCmdUI *pCmdUI)
{
	if (m_pCore->I_GetPreviewOverlayMcuGrid()) {
		pCmdUI->SetCheck(true);
	} else {
		pCmdUI->SetCheck(false);
	}
}





// Menu enable status for YCC indicator
void CJPEGsnoopDoc::OnUpdateIndicatorYcc(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(TRUE);
	pCmdUI->SetText(m_pCore->I_GetStatusYccText());
}

// Menu enable status for MCU indicator
void CJPEGsnoopDoc::OnUpdateIndicatorMcu(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(TRUE);
	pCmdUI->SetText(m_pCore->I_GetStatusMcuText());
}

// Menu enable status for file position indicator
void CJPEGsnoopDoc::OnUpdateIndicatorFilePos(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(TRUE);
	pCmdUI->SetText(m_pCore->I_GetStatusFilePosText());
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
	m_pCore->I_GetDetailVlc(bEn,dlgDetail.m_nMcuX,dlgDetail.m_nMcuY,dlgDetail.m_nMcuLen);
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

	nMarkerCnt = m_pCore->I_GetMarkerCount();

	// Double-point, gives range
	if (nMarkerCnt >= 2) {
		// Get last two block selections
		ptScan1 = m_pCore->I_GetMarkerBlk(nMarkerCnt-2);
		ptScan2 = m_pCore->I_GetMarkerBlk(nMarkerCnt-1);
		// Convert block index to MCU
		ptScan1.x *= BLK_SZ_X;
		ptScan1.y *= BLK_SZ_Y;
		ptScan2.x *= BLK_SZ_X;
		ptScan2.y *= BLK_SZ_Y;
		ptMcuStart	= m_pCore->I_PixelToMcu(ptScan1);
		ptMcuEnd	= m_pCore->I_PixelToMcu(ptScan2);
		nStartMcuX	= ptMcuStart.x;
		nStartMcuY	= ptMcuStart.y;
		nEndMcuX	= ptMcuEnd.x;
		nEndMcuY	= ptMcuEnd.y;

		// Check to see if order needs to be reversed
		if ( (nStartMcuY > nEndMcuY) || ( (nStartMcuY == nEndMcuY) && (nStartMcuX > nEndMcuX) ) ) {
			// Reverse order
			nStartMcu	= m_pCore->I_McuXyToLinear(ptMcuEnd);
			nEndMcu		= m_pCore->I_McuXyToLinear(ptMcuStart);
		} else {
			nStartMcu	= m_pCore->I_McuXyToLinear(ptMcuStart);
			nEndMcu		= m_pCore->I_McuXyToLinear(ptMcuEnd);
		}

		// Calculate length from linear positions
		nMcuRngLen = nEndMcu-nStartMcu + 1;

	// Single point, gives start point
	} else if (nMarkerCnt >= 1) {
		ptScan1 = m_pCore->I_GetMarkerBlk(nMarkerCnt-1);
		// Convert block index to MCU
		ptScan1.x *= BLK_SZ_X;
		ptScan1.y *= BLK_SZ_Y;
		ptMcuStart	= m_pCore->I_PixelToMcu(ptScan1);
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
		m_pCore->I_SetDetailVlc((dlgDetail.m_bEn!=0),dlgDetail.m_nMcuX,dlgDetail.m_nMcuY,dlgDetail.m_nMcuLen);
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

	// Initialize min/max range values (to 16-bit signed limits)
	short			nValMaxY  = -32768;
	short			nValMinY  = +32767;
	short			nValMaxCb = -32768;
	short			nValMinCb = +32767;
	short			nValMaxCr = -32768;
	short			nValMinCr = +32767;

	m_pCore->I_GetImageSize(nSizeX,nSizeY);
	m_pCore->I_GetBitmapPtr(pBitmapRgb);
	m_pCore->I_GetPixMapPtrs(pBitmapYccY,pBitmapYccCb,pBitmapYccCr);
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
	pCmdUI->Enable(m_pCore->IsAnalyzed());
}


// -------------------------
// Public accessors

void CJPEGsnoopDoc::J_ImgSrcChanged()
{
	m_pCore->J_ImgSrcChanged();
}

void CJPEGsnoopDoc::I_ViewOnDraw(CDC* pDC,CRect rectClient,CPoint ptScrolledPos,CFont* pFont, CSize &szNewScrollSize)
{
	m_pCore->I_ViewOnDraw(pDC,rectClient,ptScrolledPos,pFont,szNewScrollSize);
}

void CJPEGsnoopDoc::I_GetPreviewPos(unsigned &nX,unsigned &nY)
{
	m_pCore->I_GetPreviewPos(nX,nY);
}

void CJPEGsnoopDoc::I_GetPreviewSize(unsigned &nX,unsigned &nY)
{
	m_pCore->I_GetPreviewSize(nX,nY);
}

float CJPEGsnoopDoc::I_GetPreviewZoom()
{
	return m_pCore->I_GetPreviewZoom();
}
