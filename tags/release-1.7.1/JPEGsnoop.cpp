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

// JPEGsnoop is written in Microsoft Visual C++ 2003 using MFC

// JPEGsnoop.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "JPEGsnoop.h"
#include "MainFrm.h"

#include "JPEGsnoopDoc.h"
#include "JPEGsnoopView.h"

#include "snoop.h"

#include "SnoopConfig.h"

#include "AboutDlg.h"
#include "DbSubmitDlg.h"
#include "SettingsDlg.h"
#include "DbManageDlg.h"
#include "TermsDlg.h"
#include "UpdateAvailDlg.h"
#include "ModelessDlg.h"
#include "NoteDlg.h"
#include "HyperlinkStatic.h"
#include ".\jpegsnoop.h"

#include "afxinet.h"			// For internet


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CJPEGsnoopApp

BEGIN_MESSAGE_MAP(CJPEGsnoopApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	// Standard file based document commands
	
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	//CAL! ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	//ON_COMMAND(ID_FILE_NEW, MyOnFileNew)
	ON_COMMAND(ID_FILE_OPEN, MyOnFileOpen)

	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)

	ON_COMMAND(ID_OPTIONS_DHTEXPAND, OnOptionsDhtexpand)
	ON_COMMAND(ID_OPTIONS_MAKERNOTES, OnOptionsMakernotes)
	ON_COMMAND(ID_OPTIONS_CONFIGURATION, OnOptionsConfiguration)
	ON_COMMAND(ID_OPTIONS_CHECKFORUPDATES, OnOptionsCheckforupdates)
	ON_COMMAND(ID_OPTIONS_SIGNATURESEARCH, OnOptionsSignaturesearch)
	ON_COMMAND(ID_TOOLS_MANAGELOCALDB, OnToolsManagelocaldb)
	ON_COMMAND(ID_SCANSEGMENT_DECODEIMAGE, OnScansegmentDecodeimage)
	ON_COMMAND(ID_SCANSEGMENT_FULLIDCT, OnScansegmentFullidct)
	ON_COMMAND(ID_SCANSEGMENT_HISTOGRAMY, OnScansegmentHistogramy)
	ON_COMMAND(ID_SCANSEGMENT_DUMP, OnScansegmentDump)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_DHTEXPAND, OnUpdateOptionsDhtexpand)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_MAKERNOTES, OnUpdateOptionsMakernotes)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_SIGNATURESEARCH, OnUpdateOptionsSignaturesearch)
	ON_UPDATE_COMMAND_UI(ID_SCANSEGMENT_DECODEIMAGE, OnUpdateScansegmentDecodeimage)
	ON_UPDATE_COMMAND_UI(ID_SCANSEGMENT_FULLIDCT, OnUpdateScansegmentFullidct)
	ON_UPDATE_COMMAND_UI(ID_SCANSEGMENT_HISTOGRAMY, OnUpdateScansegmentHistogramy)
	ON_UPDATE_COMMAND_UI(ID_SCANSEGMENT_DUMP, OnUpdateScansegmentDump)
	ON_COMMAND(ID_SCANSEGMENT_NOIDCT, OnScansegmentNoidct)
	ON_UPDATE_COMMAND_UI(ID_SCANSEGMENT_NOIDCT, OnUpdateScansegmentNoidct)
	ON_COMMAND(ID_SCANSEGMENT_HISTOGRAM, OnScansegmentHistogram)
	ON_UPDATE_COMMAND_UI(ID_SCANSEGMENT_HISTOGRAM, OnUpdateScansegmentHistogram)
	ON_COMMAND(ID_OPTIONS_HIDEUKNOWNEXIFTAGS, OnOptionsHideuknownexiftags)
	ON_UPDATE_COMMAND_UI(ID_OPTIONS_HIDEUKNOWNEXIFTAGS, OnUpdateOptionsHideuknownexiftags)
	ON_COMMAND(ID_FILE_BATCHPROCESS, OnFileBatchprocess)
END_MESSAGE_MAP()

// FIXME:
// Would like to change string table JPEGsnoop.rc to add:
//   AFX_IDS_OPENFILE 0xF000 (61440) = _T("Open Image / Movie") for Open Dialog
// but linker complains error RC2151 "cannot reuse string constants"


// ======================================
// Command Line option support
// ======================================
//
// JPEGsnoop.exe <parameters>
//
// One of the following input parameters:
//   -i <fname_in>    : [Mandatory] Defines input JPEG filename
//   -b <directory>   : [Mandatory] Batch mode operation
// Zero or more of the following input parameters:
//   -o <fname_log>   : [Optional]  Defines output log filename
//   -nogui           : [Optional]  Runs in non-GUI mode (window minimized) and exits after operations
//   -b <dir>         : [Optional]  Batch process directory
//   -br <dir>        : [Optional]  Batch process directory (recursive)
//   -ext_all         : [Optional]  Extract all from file
//   -ext_dht_avi     : [Optional]  Force insert DHT for AVI (-ext_all mode)
//   -scan            : [Optional]  Enables Scan Segment decode
//   -maker           : [Optional]  Enables Makernote decode
//   -scandump        : [Optional]  Enables Scan Segment dumping
//   -histo_y         : [Optional]  Enables luminance histogram
//   -dht_exp         : [Optional]  Enables DHT table expansion into huffman bitstrings
//   -exif_hide_unk   : [Optional]  Disables decoding of unknown makernotes
//

// Command-line parser class
class CMyCommandParser : public CCommandLineInfo
{
 	typedef enum	{cla_idle,cla_input,cla_output,cla_err,cla_batchdir} cla_e;
	int				index;
	cla_e			next_arg;
	CSnoopConfig*	m_pCfg;
	CString			strTmp;

public:
	CMyCommandParser(CSnoopConfig* pCfg) {
		m_pCfg = pCfg;
		CCommandLineInfo();
		index=0; // initializes an index to allow make a positional analysis
		next_arg = cla_idle;
	};  

     virtual void ParseParam(LPCTSTR pszParam, BOOL bFlag, BOOL bLast) {
				CString msg;

		 switch(next_arg) {
			case cla_idle:
				if (bFlag && !_tcscmp(pszParam,_T("i"))) {
					next_arg = cla_input;
				}
				else if (bFlag && !_tcscmp(pszParam,_T("o"))) {
					next_arg = cla_output;
				}
				else if (bFlag && !_tcscmp(pszParam,_T("b"))) {
					next_arg = cla_batchdir;
				}
				else if (bFlag && !_tcscmp(pszParam,_T("br"))) {
					m_pCfg->bCmdLineBatchRec = true;
					next_arg = cla_batchdir;
				}
				else if (bFlag && !_tcscmp(pszParam,_T("ext_all"))) {
					m_pCfg->bCmdLineExtractEn = true;
					next_arg = cla_idle;
				}
				else if (bFlag && !_tcscmp(pszParam,_T("ext_dht_avi"))) {
					m_pCfg->bCmdLineExtractDhtAvi = true;
					next_arg = cla_idle;
				}
				else if (bFlag && !_tcscmp(pszParam,_T("nogui"))) {
					m_pCfg->bCmdLineGui = false;	// Force non-GUI mode
					// In non-GUI mode we also want to hide message boxes
					m_pCfg->bInteractive = false;
					next_arg = cla_idle;
				}
				else if (bFlag && !_tcscmp(pszParam,_T("scan"))) {
					m_pCfg->bDecodeScanImg = true;
					next_arg = cla_idle;
				}
				else if (bFlag && !_tcscmp(pszParam,_T("maker"))) {
					m_pCfg->bDecodeMaker = true;
					next_arg = cla_idle;
				}
				else if (bFlag && !_tcscmp(pszParam,_T("scandump"))) {
					m_pCfg->bOutputScanDump = true;
					next_arg = cla_idle;
				}
				else if (bFlag && !_tcscmp(pszParam,_T("histo_y"))) {
					m_pCfg->bDumpHistoY = true;
					next_arg = cla_idle;
				}
				else if (bFlag && !_tcscmp(pszParam,_T("dhtexp"))) {
					m_pCfg->bOutputDHTexpand = true;
					next_arg = cla_idle;
				}
				else if (bFlag && !_tcscmp(pszParam,_T("exif_hide_unk"))) {
					m_pCfg->bExifHideUnknown = true;
					next_arg = cla_idle;
				}
				else if (bFlag) {
					// Unknown flag
					strTmp.Format(_T("ERROR: Unknown command-line flag [-%s]"),pszParam);
					// Don't disable dialog in non-interactive mode as this could be an important error
					AfxMessageBox(strTmp);
					next_arg = cla_err;
				}
				else {
					// Not a flag, so assume it is a drag & drop file open
					m_pCfg->bCmdLineOpenEn = true;
					m_pCfg->strCmdLineOpenFname = pszParam;

					m_nShellCommand = FileOpen;
					m_strFileName = pszParam;
				}

				break;

			case cla_input:
				msg = _T("Input=[");
				msg += pszParam;
				msg += _T("]");
				m_pCfg->bCmdLineOpenEn = true;
				m_pCfg->strCmdLineOpenFname = pszParam;

				m_nShellCommand = FileOpen;
				m_strFileName = pszParam;

				next_arg = cla_idle;
				break;

			// Batch directory processing
			case cla_batchdir:
				msg = _T("BatchDir=[");
				msg += pszParam;
				msg += _T("]");

				// Store the batch directory name
				m_pCfg->bCmdLineBatchEn = true;
				m_pCfg->strCmdLineBatchDirName = pszParam;

				// Set the ShellCommand to FileNothing so that the InitProcess()
				// call doesn't take any action. Instead, we can perform the
				// batch processing ourself.
				m_nShellCommand = FileNothing;
				m_strFileName = _T("");	// Unused

				next_arg = cla_idle;
				break;

			case cla_output:
				msg = _T("Output=[");
				msg += pszParam;
				msg += _T("]");
				m_pCfg->bCmdLineOutputEn = true;
				m_pCfg->strCmdLineOutputFname = pszParam;
				next_arg = cla_idle;
				break;

			case cla_err:
			default:
				break;
		 }

     };
}; 



// CJPEGsnoopApp construction

// Constructor
CJPEGsnoopApp::CJPEGsnoopApp()
{

	// Reset fatal error flag
	m_bFatal = false;

	// Application-level config options
	m_pAppConfig = new CSnoopConfig();
	if (!m_pAppConfig) {
		AfxMessageBox(_T("ERROR: Couldn't allocate memory for SnoopConfig"));
		m_bFatal = true;
	}

	m_pDbSigs = new CDbSigs();
	if (!m_pDbSigs) {
		AfxMessageBox(_T("ERROR: Couldn't allocate memory for DbSigs"));
		m_bFatal = true;
	}

}

// Destructor
CJPEGsnoopApp::~CJPEGsnoopApp()
{

	// Save and then Delete
	if (m_pAppConfig != NULL)
	{
		m_pAppConfig->RegistryStore();

		delete m_pAppConfig;
		m_pAppConfig = NULL;
	}

	if (m_pDbSigs != NULL)
	{
		delete m_pDbSigs;
		m_pDbSigs = NULL;
	}

}

// The one and only CJPEGsnoopApp object

CJPEGsnoopApp theApp;

// CJPEGsnoopApp initialization

BOOL CJPEGsnoopApp::InitInstance()
{
	// InitCommonControls() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	InitCommonControls();

	CWinApp::InitInstance();

	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}
	AfxEnableControlContainer();

	// Check to see if we had any fatal errors yet (e.g. mem alloc)
	if (m_bFatal) {
		return FALSE;
	}


	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored

	// This call actually concatenates CWinApp::m_pszAppName ("JPEGsnoop")
	// with the "REG_COMPANY_NAME" to create the full
	// key path "Software/ImpulseAdventure/JPEGsnoop/Recent File List"
	SetRegistryKey(REG_COMPANY_NAME);
	LoadStdProfileSettings(4);  // Load standard INI file options (including MRU)

	// ------------------------------------
	m_pAppConfig->RegistryLoad();

	// Assign defaults
	m_pAppConfig->UseDefaults();

	// Ensure that the user has previously signed the EULA
	if (!CheckEula()) {
		return false;
	}

	// Has the user enabled checking for program updates?
	if (m_pAppConfig->bUpdateAuto) {
		CheckUpdates(false);
	}

	m_pAppConfig->RegistryStore();

	// Update the User database directory setting
	m_pDbSigs->SetDbDir(m_pAppConfig->strDbDir);


	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views

	// FIXME:
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CJPEGsnoopDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CJPEGsnoopView));
	if (!pDocTemplate)
		return FALSE;
	pDocTemplate->SetContainerInfo(IDR_CNTR_INPLACE);
	AddDocTemplate(pDocTemplate);



	// Parse command line for standard shell commands, DDE, file open
	CMyCommandParser cmdInfo(m_pAppConfig);
	ParseCommandLine(cmdInfo);

	if (m_pAppConfig->bCmdLineGui == false) {
		// If the user has requested non-GUI mode:
		// - Command-line "-nogui"
		// then we will still create a window as a result of OnInitialUpdate()
		// in OpenDocumentFile and OnFileNew() in ProcessShellCommand.
		//
		// So, to ensure that we don't take focus away from other user UI tasks
		// we force the window into MINIMIZED mode.
		// Ref: http://www.codeproject.com/Articles/23419/A-Simple-Method-to-Control-the-Startup-State-of-an
		this->m_nCmdShow = SW_MINIMIZE;
	} else {
		// If the user has requested that we open up a file in GUI mode:
		// - Drag & drop
		// - Command-line "-i" without '-nogui"
		// then we need to ensure that the view has been created before we
		// enter the OnOpenDocument() in OpenDocumentFile(). The reason for this
		// is that OnOpenDocument will call AnalyzeFile() and insert the log into
		// the view. The view is only created at the end of OpenDocumentFile() as
		// a result of OnInitialUpdate().
		//
		// To achieve this, the easiest method is to force a New file operation
		// first. This ensures that we create the view (in OnInitialUpdate)
		// before we launch into any processing.
		//
		// For cosmetic reasons, also call UpdateWindow to ensure that the
		// frame is drawn fully before any further processing.
		OnFileNew();
		m_pMainWnd->ShowWindow(SW_SHOW);
		m_pMainWnd->UpdateWindow();
	}


	// Do my own ProcessShellCommand(). The following is based
	// on part of what exists in appui2.cpp with no real changes.
	// I have dropped off some of the unsupported m_nShellCommand modes.
	// Perhaps this will provide us with an easier means of extending
	// support to other batch commands later.
	// ----------------------------------------------------------
#if 1
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;
#else
	BOOL bResult = TRUE;
	switch (cmdInfo.m_nShellCommand) {
		// Note that FileNew is the default
		case CCommandLineInfo::FileNew:
			if (!OnCmdMsg(ID_FILE_NEW, 0, NULL, NULL))
				OnFileNew();
			if (m_pMainWnd == NULL)
				bResult = FALSE;
			break;

		// NOTE: We expect that the RichEditView is already created!
		case CCommandLineInfo::FileOpen:
			if (!OpenDocumentFile(cmdInfo.m_strFileName))
				bResult = FALSE;
			break;
		default:
			bResult = FALSE;
			break;

	}
	if (!bResult)
		return FALSE;
#endif
	// ----------------------------------------------------------

	// Now handle any other command-line directives that we haven't
	// already covered above
	if (m_pAppConfig->bCmdLineBatchEn) {

		// TODO: Is there a cleaner way of doing this?
		// Force an OnFileNew() to ensure that we have created the "Document" class
		OnFileNew();

		CJPEGsnoopDoc* pMyDoc;
		pMyDoc = GetCurDoc();
		if (pMyDoc) {
			// Pass in batch directory path from user
			// Disable the bAskSettings mode as all have been
			// provided via the command-line
			pMyDoc->DoBatchProcess(false,m_pAppConfig->strCmdLineBatchDirName,m_pAppConfig->bCmdLineBatchRec,m_pAppConfig->bCmdLineExtractEn);
		}
	}

	if (m_pAppConfig->bCmdLineExtractEn) {

		// TODO: Is there a cleaner way of doing this?
		// Force an OnFileNew() to ensure that we have created the "Document" class
		OnFileNew();

		CJPEGsnoopDoc* pMyDoc;
		pMyDoc = GetCurDoc();
		if (pMyDoc) {
			// Pass in batch directory path from user
			pMyDoc->DoExtractEmbeddedJPEG(false,m_pAppConfig->bCmdLineExtractDhtAvi,_T(""));
		}
	}

	// ----------------------------------------------------------

	// If we are in command-line -nogui mode then we want to exit this function
	// with return false to terminate the main program (since we have already
	// completed our tasks in the ProcessShellCommand() above).
	// 
	// The CSingleDocTemplate::OpenDocumentFile() call within OnFileNew() will still cause the view
	// and frame to have been allocated so we can't just exit. Instead, we need
	// to deallocate it first through DestroyWindow(). If we don't do this, then
	// we will get the error "Destroying non-Null m_pMainWnd".
	if (m_pAppConfig->bCmdLineGui == false) {
		m_pMainWnd->DestroyWindow();
		return FALSE;
	}

	// We only arrive here if there is a window to show (bResult)

	// Original wizard code follows

	// The one and only window has been initialized, so show and update it
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	// call DragAcceptFiles only if there's a suffix
	//  In an SDI app, this should occur after ProcessShellCommand
	
	// ----------------

	return TRUE;



}


// If it has been more than "nUpdateAutoDays" days since our
// last check for a recent update to the software, check the
// website for a newer version.
// - If a new version is available, the user is notified but
//   no action or changes are made. The user needs to navigate
//   to the website to download the latest version manually.
//
// INPUT:
// - bForceNow			= Do we ignore day timer and force a check now?
//
void CJPEGsnoopApp::CheckUpdates(bool bForceNow)
{
	CString		strUpdateLastChk = m_pAppConfig->strUpdateLastChk;

    unsigned nCheckYear,nCheckMon,nCheckDay;
	if (strUpdateLastChk.GetLength()==8) {
		nCheckYear = _tstoi(strUpdateLastChk.Mid(0,4));
		nCheckMon  = _tstoi(strUpdateLastChk.Mid(4,2));
		nCheckDay  = _tstoi(strUpdateLastChk.Mid(6,2));
	} else {
		nCheckYear = 1980;
		nCheckMon  = 1;
		nCheckDay  = 1;
	}

	CTime tmeUpdateLastChk(nCheckYear,nCheckMon,nCheckDay,0,0,0);
	CTime tmeToday = CTime::GetCurrentTime();
    CTimeSpan tmePeriod(m_pAppConfig->nUpdateAutoDays, 0, 0, 0);
	CTimeSpan tmeDiff;
	tmeDiff = tmeToday - tmeUpdateLastChk;

	if ((bForceNow) || (tmeDiff >= tmePeriod)) {
		CModelessDlg*	pdlg;
		pdlg = new CModelessDlg;
		if (!pdlg) {
			// Fatal error
			exit(1);
		} 
		pdlg->Create(IDD_MODELESSDLG,NULL);

		CheckUpdatesWww();

		pdlg->OnCancel();

		// Update the timestamp of the last update check
		CString strCurDate;
		strCurDate = tmeToday.Format(_T("%Y%m%d"));
		m_pAppConfig->strUpdateLastChk = strCurDate;
		m_pAppConfig->Dirty();

	}

}

// Scrape the header of the web page to determine if a newer version
// of the software is available.
// - No information is sent to the website other than the current version number
//
// RETURN:
// - Success if connection to web page was OK
//
bool CJPEGsnoopApp::CheckUpdatesWww()
{

	CString	strVerLatest = _T("");
	CString strDateLatest = _T("");

	CString			strSubmitHost;
	CString			strSubmitPage;
	strSubmitHost = IA_HOST;
	strSubmitPage = IA_UPDATES_CHK_PAGE;

	static LPTSTR acceptTypes[2]={_T("*/*"), NULL};
	HINTERNET hINet, hConnection, hData;

	unsigned nLen;

	CString		strFormat;
	CString		strFormData;
	unsigned	nFormDataLen;
	CString		strHeaders =
		_T("Content-Type: application/x-www-form-urlencoded");
	strFormat  = _T("ver=%s");

	//*** Need to sanitize data for URL submission!
	// Search for "&", "?", "="
	strFormData.Format(strFormat,VERSION_STR);
	nFormDataLen = strFormData.GetLength();

	CString			strTmp;

	CHAR			pcBuffer[2048] ;
	CString			strContents ;
	DWORD			dwRead; //dwStatus;
	hINet = InternetOpen(_T("JPEGsnoop/1.0"), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
	if ( !hINet )
	{
		AfxMessageBox(_T("InternetOpen Failed"));
		return false;
	}
	try
	{
		hConnection = InternetConnect( hINet, (LPCTSTR)strSubmitHost, 80, NULL,NULL, INTERNET_SERVICE_HTTP, 0, 1 );
		if ( !hConnection )
		{
			InternetCloseHandle(hINet);
			return false;
		}
		hData = HttpOpenRequest( hConnection, _T("POST"), (LPCTSTR)strSubmitPage, NULL, NULL, NULL, 0, 1 );
		if ( !hData )
		{
			InternetCloseHandle(hConnection);
			InternetCloseHandle(hINet);
			return false;
		}
		// GET HttpSendRequest( hData, NULL, 0, NULL, 0);

		if (!HttpSendRequest( hData, (LPCTSTR)strHeaders, strHeaders.GetLength(), strFormData.GetBuffer(), strFormData.GetLength())) {
			InternetCloseHandle(hConnection);
			InternetCloseHandle(hINet);
			AfxMessageBox(_T("ERROR: Couldn't SendRequest"));
			return false;
		}

		
		// Only read the first 1KB of page
		bool		bScrapeDone = false;
		unsigned	nScrapeLen = 0;
		unsigned	nScrapeMax = 1024;
		while (!bScrapeDone) {
			if (!InternetReadFile( hData, pcBuffer, 255, &dwRead )) {
				bScrapeDone = true;
			} else {
				if ( dwRead == 0 ) {
					bScrapeDone = true;
					break;
				}
				pcBuffer[dwRead] = 0;
				strContents += pcBuffer;
				nScrapeLen += dwRead;
				if (nScrapeLen >= nScrapeMax) {
					bScrapeDone = true;
				}
			}
		}
		
		// Parse the HTTP result and search for the latest
		// version identification string
		nLen = strContents.GetLength();

		CString			strData;
		CString			strParam;
		CString			strVal;
		int				nIndDataStart = -1;
		int				nIndDataEnd = -1;
		unsigned		nIndDataLen = 0;
		bool			bFoundRange = true;
		nIndDataStart = strContents.Find(_T("***("))+4;
		nIndDataEnd = strContents.Find(_T(")***"));

		if ((nIndDataStart == -1) || (nIndDataEnd == -1)) {
			// Couldn't find start or end
			bFoundRange = false;
		}

		if (nIndDataStart > nIndDataEnd) {
			// Start and End positions look wrong
			bFoundRange = false;
		}

		if (bFoundRange) {
			// Found start & end markers
			nIndDataLen = nIndDataEnd-nIndDataStart;
			strData = strContents.Mid(nIndDataStart,nIndDataLen);
			bool		bDone = false;
			bool		bTokDone;
			CString		strCh;
			unsigned	nInd = 0;
			while (!bDone) {
				if (nInd >= nIndDataLen) {
					bDone = true;
				} else {
					bTokDone = false;
					strParam = _T("");
					while (!bTokDone) {
						strCh = strData.GetAt(nInd);
						if (strCh==_T("=")) {
							bTokDone = true;
							nInd+=2; // skip [
						} else {
							strParam += strCh;
							nInd++;
						}
					}

					bTokDone = false;
					strVal = _T("");
					while (!bTokDone) {
						strCh = strData.GetAt(nInd);
						if (strCh==_T("]")) {
							bTokDone = true;
							nInd+=2; // skip ,
						} else {
							strVal += strCh;
							nInd++;
						}
					}

					//strTmp.Format(_T("Param[%s] Val[%s]"),strParam,strVal);
					//AfxMessageBox(strTmp);

					if (strParam == _T("latest_ver")) {
						strVerLatest = strVal;
					}
					if (strParam == _T("latest_date")) {
						strDateLatest = strVal;
					}

				}
			}

		} // found range
		
	}
	catch( CInternetException* e)
	{
		e->ReportError();
		e->Delete();
		//AfxMessageBox(_T("EXCEPTION!"));
	}
	InternetCloseHandle(hConnection);
	InternetCloseHandle(hINet);
	InternetCloseHandle(hData);

	// Report that a later version exists!
	if (strVerLatest != VERSION_STR) {
		CUpdateAvailDlg	dlgUpdate;
		dlgUpdate.strVerCur = VERSION_STR;
		dlgUpdate.strVerLatest = strVerLatest;
		dlgUpdate.strDateLatest = strDateLatest;
		dlgUpdate.bUpdateAutoStill = m_pAppConfig->bUpdateAuto;
		if (dlgUpdate.DoModal() == IDOK) {
			m_pAppConfig->bUpdateAuto = (dlgUpdate.bUpdateAutoStill)?true:false;
			m_pAppConfig->Dirty();
		}
	}

	return true;
}



// Check the End-User License Agreement status
// - Ensure that the EULA has been previously signed
// - If EULA hasn't been signed, then show dialog box
//
// POST:
// - m_pAppConfig->bEulaAccepted
// - m_pAppConfig->bUpdateAuto
//
bool CJPEGsnoopApp::CheckEula()
{
	if (m_pAppConfig->bEulaAccepted) {
		return true;
	} else {
		CTermsDlg	dlg;
		if (dlg.DoModal() == IDOK) {
			// They must have accepted the terms, update the
			// acceptance in the registry
			m_pAppConfig->bEulaAccepted = (dlg.bEulaOk != 0);
			m_pAppConfig->bUpdateAuto = (dlg.bUpdateAuto != 0);
			m_pAppConfig->Dirty();
			m_pAppConfig->RegistryStore();
			return true;
		} else {
			// They didn't accept, so quit the application
			m_pAppConfig->bEulaAccepted = false;
			m_pAppConfig->Dirty();
			m_pAppConfig->RegistryStore();

			// How to quit?
			return false;
		}
	}
}



// App command to run the dialog
void CJPEGsnoopApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}


// CJPEGsnoopApp message handlers

// Present file open dialog
// - Populate file filter list with main decode formats
//
// POST:
// - Calls OpenDocumentFile()
//
void CJPEGsnoopApp::MyOnFileOpen()
{

	bool bStatus = 0;
	TCHAR aszFilter[] =
		_T("JPEG Image (*.jpg;*.jpeg)|*.jpg;*.jpeg|")\
		_T("Thumbnail (*.thm)|*.thm|")\
		_T("AVI Movie (*.avi)|*.avi|")\
		_T("QuickTime Movie (*.mov)|*.mov|")\
		_T("Digital Negative (*.dng)|*.dng|")\
		_T("RAW Image (*.crw;*.cr2;*.nef;*.orf;*.pef)|*.crw;*.cr2;*.nef;*.orf;*.pef|")\
		_T("PDF Files (*.pdf)|*.pdf|")\
		_T("Photoshop Files (*.psd)|*.psd|")\
		_T("All Files (*.*)|*.*||");

	/*
		CFileDialog(BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
		LPCTSTR lpszDefExt = NULL,
		LPCTSTR lpszFileName = NULL,
		DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		LPCTSTR lpszFilter = NULL,
		CWnd* pParentWnd = NULL,
		DWORD dwSize = 0);
		*/

	// BUG: #1008
	CFileDialog FileDlg(TRUE, _T(".jpg"), NULL, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST, aszFilter);

	CString strTitle;
	CString strFileName;
	CString	strPathName;		// Added
	VERIFY(strTitle.LoadString(IDS_CAL_FILEOPEN));
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

	// TODO: Should trap the following for exception (CommDlgExtendedError = FNERR_BUFFERTOOSMALL)
	// For reference, see: http://msdn.microsoft.com/en-us/library/windows/desktop/ms646960%28v=vs.85%29.aspx
	if( FileDlg.DoModal() == IDOK )
	{
		strFileName = FileDlg.GetFileName();
		strPathName = FileDlg.GetPathName();
		//
		// For testing purposes, added this GetCurrentDirectory() check
		TCHAR	szDirCur[501];
		GetCurrentDirectory(500,szDirCur);

		// NOTE:
		// The OpenDocumentFile() call converts any relative path to absolute path before loading the file.
		// It appears that VS 2003 was allowing me to pass strFileName into the OpenDocumentFile() call
		// and the user selected directory in the previous CFileDialog() caused a SetCurrentDirectory()
		// which made the relative to absolute conversion work.
		//
		// In VS 2012, it appears that the CFileDialog() is no longer causing SetCurrentDirectory()
		// to pick up the selected folder, so the relative to absolute conversion is returning a path
		// to the project/executable directory. Net result is that the file doesn't get opened as it
		// cannot be found. Opens from the MRU list will still work, however.
		//
		// In VS 2012, I can theoretically get CFileDialog to work in the old way if I happened to initialize COM
		// with COINIT_MULTITHREADED, but this doesn't seem recommended.
		//
		// Instead, it looks like I can simply pass in the full path (GetPathName()) and everything
		// works fine!

//		OpenDocumentFile(strFileName);
		OpenDocumentFile(strPathName);	// Added

		// if returns NULL, the user has already been alerted
	}

	if (spFilePath) { delete[] spFilePath; }

}

// Basic placeholder for OnFileNew
void CJPEGsnoopApp::MyOnFileNew()
{
	// Default
	CWinApp::OnFileNew();
}

// Retreive the currently active document (if it exists)
CJPEGsnoopDoc* CJPEGsnoopApp::GetCurDoc()
{
	CFrameWnd * pFrame = (CFrameWnd *)(AfxGetApp()->m_pMainWnd);
	ASSERT(pFrame);
	CJPEGsnoopDoc * pMyDoc;
	pMyDoc = (CJPEGsnoopDoc *) pFrame->GetActiveDocument();
	return pMyDoc;
}

// Reprocess the current file
// - Invokes the Reprocess action in the Document
// - Ensures that the document is available first 
void CJPEGsnoopApp::DocReprocess()
{
	CJPEGsnoopDoc* pMyDoc;
	pMyDoc = GetCurDoc();
	if (pMyDoc) {
		pMyDoc->Reprocess();
	}
}

// Mark the image view as needing update
// - Will take effect on next "Reprocess" call.
void CJPEGsnoopApp::DocImageDirty()
{
	CJPEGsnoopDoc* pMyDoc;
	pMyDoc = GetCurDoc();
	if (pMyDoc) {
		pMyDoc->m_pJfifDec->ImgSrcChanged();
	}
}

// -------------------- Handle Menu actions ------------------

// Set menu item toggle for Options DHT Expand
void CJPEGsnoopApp::OnOptionsDhtexpand()
{
	if (m_pAppConfig->bOutputDHTexpand) {
		m_pAppConfig->bOutputDHTexpand = FALSE;
	} else {
		m_pAppConfig->bOutputDHTexpand = TRUE;
	}
	// Mark option as changed for next registry update
	m_pAppConfig->Dirty();

	HandleAutoReprocess();

}

// Set menu item toggle for Options Makernotes
// - Invoke reprocess if enabled
void CJPEGsnoopApp::OnOptionsMakernotes()
{
	if (m_pAppConfig->bDecodeMaker) {
		m_pAppConfig->bDecodeMaker = FALSE;
	} else {
		m_pAppConfig->bDecodeMaker = TRUE;
	}
	// Mark option as changed for next registry update
	m_pAppConfig->Dirty();

	HandleAutoReprocess();

}

// Set menu item toggle for Scan Segment Dump
// - Invoke reprocess if enabled
void CJPEGsnoopApp::OnScansegmentDump()
{
	if (m_pAppConfig->bOutputScanDump) {
		m_pAppConfig->bOutputScanDump = FALSE;
	} else {
		m_pAppConfig->bOutputScanDump = TRUE;
	}
	// Mark option as changed for next registry update
	m_pAppConfig->Dirty();

	HandleAutoReprocess();
}


// Set menu item toggle for Scan Segment Dump
// - Invoke reprocess if enabled
void CJPEGsnoopApp::OnScansegmentDecodeimage()
{
	if (m_pAppConfig->bDecodeScanImg) {
		m_pAppConfig->bDecodeScanImg = FALSE;
	} else {
		m_pAppConfig->bDecodeScanImg = TRUE;
	}
	// Mark option as changed for next registry update
	m_pAppConfig->Dirty();

	HandleAutoReprocess();
}


// Set menu item toggle for Scan Segment Histogram (Y)
// - Mark the image as dirty
// - Invoke reprocess if enabled
void CJPEGsnoopApp::OnScansegmentHistogramy()
{
	if (m_pAppConfig->bDumpHistoY) {
		m_pAppConfig->bDumpHistoY = FALSE;
	} else {
		m_pAppConfig->bDumpHistoY = TRUE;
	}
	// Mark option as changed for next registry update
	m_pAppConfig->Dirty();

	// Since changing the histogram can change the image
	// output, assume that this will cause a change and
	// require us to dirty the scan decode.
	// FIXME: If someone has scan preview on, processes once,
	// turns scan preview off, flips this option twice, and then 
	// turns scan preview on, they will need to recalc the scan.
	// Perhaps fix is to leave all of this up to ImgDec() block,
	// and have it look at the AppConfig options being sent. If
	// different (or source file changes), then it should recalc.

	// Mark image view area as needing update
	DocImageDirty();
	HandleAutoReprocess();
}



// If Automatic Reprocessing is enabled, reprocess the file
// - If Reprocessing is not enabled and the coach message has
//   not been disabled, inform user of option to enable
void CJPEGsnoopApp::HandleAutoReprocess()
{
	if (m_pAppConfig->bReprocessAuto) {
		DocReprocess();
	} else {
		if (m_pAppConfig->bCoachReprocessAuto) {
			// Show the coaching dialog
			CNoteDlg dlg;
			dlg.strMsg = COACH_REPROCESS_AUTO;
			dlg.DoModal();
			m_pAppConfig->bCoachReprocessAuto = !dlg.bCoachOff;
			m_pAppConfig->Dirty();
		}
	}
}

// Set menu item toggle for Scan Segment Full IDCT
// - Mark the image as dirty
// - Invoke reprocess if enabled
void CJPEGsnoopApp::OnScansegmentFullidct()
{
	if (m_pAppConfig->bDecodeScanImgAc) {
		m_pAppConfig->bDecodeScanImgAc = FALSE;
	} else {
		m_pAppConfig->bDecodeScanImgAc = TRUE;
	}
	// Mark option as changed for next registry update
	m_pAppConfig->Dirty();

	// Since changing the AC can change the image
	// output, assume that this will cause a change and
	// require us to dirty the scan decode.

	// Mark image view area as needing update
	DocImageDirty();
	HandleAutoReprocess();
}

// Set menu item toggle for Scan Segment No IDCT
// - Mark the image as dirty
// - Invoke reprocess if enabled
// NOTES:
// - Even though "Noidct" is the opposite of "Fullidct", the menu
//   option simply toggles the state, so we can reuse the handler.
void CJPEGsnoopApp::OnScansegmentNoidct()
{
	OnScansegmentFullidct();
}




// -------------------- Handle Menu status ------------------

// Set menu item status for Options DHT Expand
void CJPEGsnoopApp::OnUpdateOptionsDhtexpand(CCmdUI *pCmdUI)
{
	if (m_pAppConfig->bOutputDHTexpand) {
		pCmdUI->SetCheck(TRUE);
	} else {
		pCmdUI->SetCheck(FALSE);
	}
}

// Set menu item status for Options Makernotes
void CJPEGsnoopApp::OnUpdateOptionsMakernotes(CCmdUI *pCmdUI)
{
	if (m_pAppConfig->bDecodeMaker) {
		pCmdUI->SetCheck(TRUE);
	} else {
		pCmdUI->SetCheck(FALSE);
	}
}


// Set menu item status for Scan Segment Dump
void CJPEGsnoopApp::OnUpdateScansegmentDump(CCmdUI *pCmdUI)
{
	if (m_pAppConfig->bOutputScanDump) {
		pCmdUI->SetCheck(TRUE);
	} else {
		pCmdUI->SetCheck(FALSE);
	}
}


// Set menu item status for Scan Segment Decode Image
void CJPEGsnoopApp::OnUpdateScansegmentDecodeimage(CCmdUI *pCmdUI)
{
	if (m_pAppConfig->bDecodeScanImg) {
		pCmdUI->SetCheck(TRUE);
	} else {
		pCmdUI->SetCheck(FALSE);
	}
}

// Set menu item status for Scan Segment Histogram (Y)
// - May be disabled if Decode Scan Image or Histogram are disabled
void CJPEGsnoopApp::OnUpdateScansegmentHistogramy(CCmdUI *pCmdUI)
{
	if (m_pAppConfig->bDecodeScanImg && m_pAppConfig->bHistoEn) {
		pCmdUI->Enable(true);
	} else {
		pCmdUI->Enable(false);
	}
	if (m_pAppConfig->bDumpHistoY) {
		pCmdUI->SetCheck(TRUE);
	} else {
		pCmdUI->SetCheck(FALSE);
	}
}

// Set menu item status for Scan Segment Full IDCT
// - May be disabled if Decode Scan Image is disabled
void CJPEGsnoopApp::OnUpdateScansegmentFullidct(CCmdUI *pCmdUI)
{
	if (m_pAppConfig->bDecodeScanImg) {
		pCmdUI->Enable(true);
	} else {
		pCmdUI->Enable(false);
	}
	if (m_pAppConfig->bDecodeScanImgAc) {
		pCmdUI->SetCheck(TRUE);
	} else {
		pCmdUI->SetCheck(FALSE);
	}
}

// Set menu item status for Scan Segment No IDCT
// - May be disabled if Decode Scan Image is disabled
// NOTE:
// - Opposite functionality of Fullidct
void CJPEGsnoopApp::OnUpdateScansegmentNoidct(CCmdUI *pCmdUI)
{
	if (m_pAppConfig->bDecodeScanImg) {
		pCmdUI->Enable(true);
	} else {
		pCmdUI->Enable(false);
	}
	if (m_pAppConfig->bDecodeScanImgAc) {
		pCmdUI->SetCheck(FALSE);
	} else {
		pCmdUI->SetCheck(TRUE);
	}
}


// Options Configuration Dialog
// - Show the dialog box
// - Pre-populate with current m_pAppConfig settings
// - If user clicks OK, then revised settings are stored back
//   into the registry
void CJPEGsnoopApp::OnOptionsConfiguration()
{
	CSettingsDlg	setDlg;
	setDlg.m_bUpdateAuto = m_pAppConfig->bUpdateAuto;
	setDlg.m_nUpdateChkDays = m_pAppConfig->nUpdateAutoDays;
	setDlg.m_strDbDir = m_pAppConfig->strDbDir;
	setDlg.m_bReprocessAuto = m_pAppConfig->bReprocessAuto;
	setDlg.m_bDbSubmitNet = m_pAppConfig->bDbSubmitNet;
	setDlg.m_nRptErrMaxScanDecode = m_pAppConfig->nErrMaxDecodeScan;
	//setDlg.m_bReportClip = m_pAppConfig->bStatClipEn;

	if (setDlg.DoModal() == IDOK)
	{
		m_pAppConfig->bUpdateAuto = (setDlg.m_bUpdateAuto != 0);
		m_pAppConfig->nUpdateAutoDays = setDlg.m_nUpdateChkDays;
		m_pAppConfig->strDbDir = setDlg.m_strDbDir;
		m_pAppConfig->bReprocessAuto = (setDlg.m_bReprocessAuto != 0);
		m_pAppConfig->bDbSubmitNet = (setDlg.m_bDbSubmitNet != 0);
		m_pAppConfig->nErrMaxDecodeScan = setDlg.m_nRptErrMaxScanDecode;
		//m_pAppConfig->bStatClipEn = (setDlg.m_bReportClip != 0);
		m_pAppConfig->Dirty();

		// Since the signature database needs to know of the
		// [possibly] updated user database directory, we must
		// set it here.
		m_pDbSigs->SetDbDir(m_pAppConfig->strDbDir);

		// Don't need to force registry write, as it will be done
		// later when exit app, but this ensures that it gets done.
		m_pAppConfig->RegistryStore();
	}
}

// Menu item to check for JPEGsnoop updates
void CJPEGsnoopApp::OnOptionsCheckforupdates()
{
	CheckUpdates(true);
}


// Manage Local Signature Database
// - Present user with database dialog box
// - This dialog enables the user to view and delete entries that are
//   defined and stored within the current installation file.
void CJPEGsnoopApp::OnToolsManagelocaldb()
{
	CDbManageDlg	manageDlg;
	CJPEGsnoopDoc* pMyDoc;
	pMyDoc = GetCurDoc();
	if (pMyDoc) {
		pMyDoc->m_pJfifDec->ImgSrcChanged();

		unsigned	nDbUserEntries;
		CString		strTmp;
		CompSig		sMySig;
		nDbUserEntries = m_pDbSigs->DatabaseExtraGetNum();

		for (unsigned nInd=0;nInd<nDbUserEntries;nInd++) {
			sMySig = m_pDbSigs->DatabaseExtraGet(nInd);

			// TODO: Should we confirm that all entries are marked as valid already?

			if (sMySig.eEditor == ENUM_SOURCE_SW) {
				manageDlg.InsertEntry(nInd,sMySig.strMSwDisp,_T(""),sMySig.strUmQual,sMySig.strCSig);
			} else {
				manageDlg.InsertEntry(nInd,sMySig.strXMake,sMySig.strXModel,sMySig.strUmQual,sMySig.strCSig);
			}
		}


		if (manageDlg.DoModal() == IDOK) {
			// Now determine which entries were deleted
			// Set all flags to invalid
			// Step through remaining list from dialog and fetch indices
			// For each remaining index, set the corresponding valid indicator in the main list
			unsigned nNumExtra = m_pDbSigs->DatabaseExtraGetNum();
			for (unsigned nInd=0;nInd<nNumExtra;nInd++) {
				m_pDbSigs->SetEntryValid(nInd,false);
			}
			CUIntArray	anRemain;
			unsigned	nRemainInd;
			manageDlg.GetRemainIndices(anRemain);
			for (unsigned nInd=0;nInd<(unsigned)anRemain.GetCount();nInd++) {
				nRemainInd = anRemain.GetAt(nInd);
				m_pDbSigs->SetEntryValid(nRemainInd,true);
			}

			m_pDbSigs->DatabaseExtraStore();
			m_pDbSigs->DatabaseExtraLoad();
	

		}

	}

}

// Set menu item toggle for Options Signature Search
// - Invoke reprocess if enabled
void CJPEGsnoopApp::OnOptionsSignaturesearch()
{
	if (m_pAppConfig->bSigSearch) {
		m_pAppConfig->bSigSearch = FALSE;
	} else {
		m_pAppConfig->bSigSearch = TRUE;
	}
	// Mark option as changed for next registry update
	m_pAppConfig->Dirty();
	HandleAutoReprocess();
}

// Set menu item status for Options Signature Search
void CJPEGsnoopApp::OnUpdateOptionsSignaturesearch(CCmdUI *pCmdUI)
{
	if (m_pAppConfig->bSigSearch) {
		pCmdUI->SetCheck(TRUE);
	} else {
		pCmdUI->SetCheck(FALSE);
	}
}


// Set menu item toggle for Scan Segment Histogram
// - Mark the image as dirty
// - Invoke reprocess if enabled
void CJPEGsnoopApp::OnScansegmentHistogram()
{
	if (m_pAppConfig->bHistoEn) {
		m_pAppConfig->bHistoEn = FALSE;
	} else {
		m_pAppConfig->bHistoEn = TRUE;
	}
	// Mark option as changed for next registry update
	m_pAppConfig->Dirty();

	// Since changing the histogram can change the image
	// output, assume that this will cause a change and
	// require us to dirty the scan decode.
	// FIXME: If someone has scan preview on, processes once,
	// turns scan preview off, flips this option twice, and then 
	// turns scan preview on, they will need to recalc the scan.
	// Perhaps fix is to leave all of this up to ImgDec() block,
	// and have it look at the AppConfig options being sent. If
	// different (or source file changes), then it should recalc.

	// Mark image view area as needing update
	DocImageDirty();
	HandleAutoReprocess();
}

// Set menu item status for Options Signature Search
// - May be disabled if Decode Scan Image is disabled
void CJPEGsnoopApp::OnUpdateScansegmentHistogram(CCmdUI *pCmdUI)
{
	if (m_pAppConfig->bDecodeScanImg) {
		pCmdUI->Enable(true);
	} else {
		pCmdUI->Enable(false);
	}
	if (m_pAppConfig->bHistoEn) {
		pCmdUI->SetCheck(TRUE);
	} else {
		pCmdUI->SetCheck(FALSE);
	}
}

// Set menu item toggle for Options Hide Unknown EXIF tags
// - Invoke reprocess if enabled
void CJPEGsnoopApp::OnOptionsHideuknownexiftags()
{
	if (m_pAppConfig->bExifHideUnknown) {
		m_pAppConfig->bExifHideUnknown = FALSE;
	} else {
		m_pAppConfig->bExifHideUnknown = TRUE;
	}
	// Mark option as changed for next registry update
	m_pAppConfig->Dirty();

	HandleAutoReprocess();

}

// Set menu item status for Options Hide Unknown EXIF tags
void CJPEGsnoopApp::OnUpdateOptionsHideuknownexiftags(CCmdUI *pCmdUI)
{
	if (m_pAppConfig->bExifHideUnknown) {
		pCmdUI->SetCheck(TRUE);
	} else {
		pCmdUI->SetCheck(FALSE);
	}
}

// Menu item for File Batch Process
// - Invoke DoBatchProcess() from Document
void CJPEGsnoopApp::OnFileBatchprocess()
{
	// TODO: Add your command handler code here
	CJPEGsnoopDoc* pMyDoc;
	pMyDoc = GetCurDoc();
	if (pMyDoc) {
		// Enable the bAskSettings dialog mode so the user can specify some
		// additional parameters (eg. recursive subdir and "extract all" settings)
		// These parameters are ignored in the function call in bShowSettings mode
		pMyDoc->DoBatchProcess(true,_T(""),false,false);
	}
}

