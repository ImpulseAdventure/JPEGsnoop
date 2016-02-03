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

#include "StdAfx.h"
#include ".\doclog.h"

#include "JPEGsnoopDoc.h"

//
// Initialize the log
//
CDocLog::CDocLog(void)
{
	m_pDoc = NULL;
	m_bEn = true;

	// Default to local log
	m_bUseDoc = false;

	// Start in quick mode
#ifdef QUICKLOG
	m_bLogQuickMode = true;
#else
	m_bLogQuickMode = false;
#endif

}

CDocLog::~CDocLog(void)
{
}


// Enable logging
void CDocLog::Enable() {
	m_bEn = true;
};

// Disable logging
void CDocLog::Disable() {
	m_bEn = false;
};

// Enable or disable the quick log mode
//
// INPUT:
// - bQuick			= If true, write to log buffer, if false, write to CDocument
//
void CDocLog::SetQuickMode(bool bQuick)
{
	m_bLogQuickMode = bQuick;
}

// Get the current quick log mode
//
// RETURN:
// - Are we in quick mode?
//
bool CDocLog::GetQuickMode()
{
	return m_bLogQuickMode;
}

void CDocLog::Clear()
{
	m_saLogQuickTxt.RemoveAll();
	m_naLogQuickCol.RemoveAll();
}

// Establish linkage between log and the CDocument
// Passing NULL will force use of local log instead
void CDocLog::SetDoc(CDocument* pDoc)
{
	m_pDoc = pDoc;
	if (pDoc) {
		// Use RichEditCtrl for the log
		m_bUseDoc = true;
		// Empty the buffer
		Clear();
	} else {
		// Use local log
		m_bUseDoc = false;
	}
}

// Add a basic text line to the log
void CDocLog::AddLine(CString strTxt)
{
	COLORREF		sCol;
	if (m_bEn) {
		sCol = RGB(1, 1, 1);
		// TODO: Do I really need newline in these line outputs?
		if (m_bUseDoc) {
			if (m_bLogQuickMode) {
				AppendToLogLocal(strTxt+_T("\n"),sCol);
			} else {
				CJPEGsnoopDoc*	pSnoopDoc = (CJPEGsnoopDoc*)m_pDoc;
				pSnoopDoc->AppendToLog(strTxt+_T("\n"),sCol);
			}
		} else {
			AppendToLogLocal(strTxt+_T("\n"),sCol);
		}
	}
}

// Add a header text line to the log
void CDocLog::AddLineHdr(CString strTxt)
{
	COLORREF		sCol = RGB(1, 1, 255);
	if (m_bEn) {
		if (m_bUseDoc) {
			if (m_bLogQuickMode) {
				AppendToLogLocal(strTxt+_T("\n"),sCol);
			} else {
			CJPEGsnoopDoc*	pSnoopDoc = (CJPEGsnoopDoc*)m_pDoc;
			pSnoopDoc->AppendToLog(strTxt+_T("\n"),sCol);
			}
		} else {
			AppendToLogLocal(strTxt+_T("\n"),sCol);
		}
	}
}

// Add a header description text line to the log
void CDocLog::AddLineHdrDesc(CString strTxt)
{
	COLORREF		sCol = RGB(32, 32, 255);
	if (m_bEn) {
		if (m_bUseDoc) {
			if (m_bLogQuickMode) {
				AppendToLogLocal(strTxt+_T("\n"),sCol);
			} else {
			CJPEGsnoopDoc*	pSnoopDoc = (CJPEGsnoopDoc*)m_pDoc;
			pSnoopDoc->AppendToLog(strTxt+_T("\n"),sCol);
			}
		} else {
			AppendToLogLocal(strTxt+_T("\n"),sCol);
		}
	}
}

// Add a warning text line to the log
void CDocLog::AddLineWarn(CString strTxt)
{
	COLORREF		sCol = RGB(128, 1, 1);
	if (m_bEn) {
		if (m_bUseDoc) {
			if (m_bLogQuickMode) {
				AppendToLogLocal(strTxt+_T("\n"),sCol);
			} else {
				CJPEGsnoopDoc*	pSnoopDoc = (CJPEGsnoopDoc*)m_pDoc;
				pSnoopDoc->AppendToLog(strTxt+_T("\n"),sCol);
			}
		} else {
			AppendToLogLocal(strTxt+_T("\n"),sCol);
		}
	}
}

// Add an error text line to the log
void CDocLog::AddLineErr(CString strTxt)
{
	COLORREF		sCol = RGB(255, 1, 1);
	if (m_bEn) {
		if (m_bUseDoc) {
			if (m_bLogQuickMode) {
				AppendToLogLocal(strTxt+_T("\n"),sCol);
			} else {
				CJPEGsnoopDoc*	pSnoopDoc = (CJPEGsnoopDoc*)m_pDoc;
				pSnoopDoc->AppendToLog(strTxt+_T("\n"),sCol);
			}
		} else {
			AppendToLogLocal(strTxt+_T("\n"),sCol);
		}
	}
}

// Add a "good" indicator text line to the log
void CDocLog::AddLineGood(CString strTxt)
{
	COLORREF		sCol = RGB(16, 128, 16);
	if (m_bEn) {
		if (m_bUseDoc) {
			if (m_bLogQuickMode) {
				AppendToLogLocal(strTxt+_T("\n"),sCol);
			} else {
				CJPEGsnoopDoc*	pSnoopDoc = (CJPEGsnoopDoc*)m_pDoc;
				pSnoopDoc->AppendToLog(strTxt+_T("\n"),sCol);
			}
		} else {
			AppendToLogLocal(strTxt+_T("\n"),sCol);
		}
	}
}

// ======================================================================

unsigned CDocLog::AppendToLogLocal(CString strTxt, COLORREF sColor)
{
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

// Get the number of lines in the local log or quick buffer
unsigned CDocLog::GetNumLinesLocal()
{
	return m_saLogQuickTxt.GetCount();
}

// Fetch a line from the local log
// Returns false if line number is out of range
bool CDocLog::GetLineLogLocal(unsigned nLine,CString &strOut,COLORREF &sCol)
{
	unsigned numLines = m_saLogQuickTxt.GetCount();
	if (nLine >= numLines) {
		return false;
	}
	strOut = m_saLogQuickTxt[nLine];
	sCol = m_naLogQuickCol[nLine];
	return true;
}

// Save the current log to text file with a simple implementation
//
// - This routine is implemented with a simple output mechanism rather
//   than leveraging CRichEditCtrl::DoSave() so that we can perform
//   this operation in command-line mode or batch mode operations.
//
void CDocLog::DoLogSave(CString strLogName)
{
	CStdioFile*	pLog;

	// Open the file for output
	ASSERT(strLogName != _T(""));

	//xxx old comments
	// This save method will only work if we were in Quick Log mode
	// where we have recorded the log to a string buffer and not
	// directly to the RichEdit
	// Note that m_bLogQuickMode is only toggled on during processing
	// so we can't check the status here (except seeing that there are no
	// lines in the array)

	//TODO: Ensure file doesn't exist and only overwrite if specified in command-line?

	// TODO:
	// Confirm that we are not writing to the same file we opened
	// m_strPathName
	//xxx ASSERT(strLogName != m_strPathName);

	try
	{
		// Open specified file
		pLog = new CStdioFile(strLogName, CFile::modeCreate| CFile::modeWrite | CFile::typeText | CFile::shareDenyNone);
	}
	catch (CFileException* e)
	{
		TCHAR msg[MAX_BUF_EX_ERR_MSG];
		CString strError;
		e->GetErrorMessage(msg,MAX_BUF_EX_ERR_MSG);
		e->Delete();
		strError.Format(_T("ERROR: Couldn't open file for write [%s]: [%s]"),
			(LPCTSTR)strLogName, (LPCTSTR)msg);
		// FIXME: Find an alternate method of signaling error in command-line mode
		AfxMessageBox(strError);
		pLog = NULL;

		return;

	}

	// Step through the current log buffer
	CString		strLine;
	COLORREF	sCol;
	unsigned nQuickLines = GetNumLinesLocal();
	for (unsigned nLine=0;nLine<nQuickLines;nLine++)
	{
		GetLineLogLocal(nLine,strLine,sCol);
		pLog->WriteString(strLine);
	}

	// Close the file
	if (pLog) {
		pLog->Close();
		delete pLog;
	}

}
