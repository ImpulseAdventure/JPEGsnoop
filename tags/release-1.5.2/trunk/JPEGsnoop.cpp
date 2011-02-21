// JPEGsnoop - JPEG Image Decoder & Analysis Utility
// Copyright (C) 2010 - Calvin Hass
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

//CAL! FIXME
// Would like to change string table JPEGsnoop.rc to add:
//   AFX_IDS_OPENFILE 0xF000 (61440) = "Open Image / Movie" for Open Dialog
// but linker complains error RC2151 "cannot reuse string constants"


// ======================================
// Command Line option support
// ======================================
//
// JPEGsnoop.exe <parameters>
//
// One of the following input parameters:
//   -i <fname_in>    : [Mandatory] Defines input JPEG filename
//   -b <directory>   : [Mandatory] UNSUPPORTED: Batch mode operation
// Zero or more of the following input parameters:
//   -o <fname_log>   : [Optional]  Defines output log filename
//   -nogui           : [Optional]  Disables GUI interface
//   -scan            : [Optional]  Enables Scan Segment decode
//   -maker           : [Optional]  Enables Makernote decode
//   -scandump        : [Optional]  Enables Scan Segment dumping
//   -histo_y         : [Optional]  Enables luminance histogram
//   -dht_exp         : [Optional]  Enables DHT table expansion into huffman bitstrings
//   -exif_hide_unk   : [Optional]  Disables decoding of unknown makernotes


class CMyCommandParser : public CCommandLineInfo
{
 	typedef enum	{cla_idle,cla_input,cla_output,cla_err,cla_batchdir} cla_e;
	int				index;
	cla_e			next_arg;
	CSnoopConfig*	m_pCfg;
	CString			tmpStr;

public:
	CMyCommandParser(CSnoopConfig* pCfg) {
		m_pCfg = pCfg;
		CCommandLineInfo();
		index=0; // initializes an index to allow make a positional analysis
		next_arg = cla_idle;
	};  

     virtual void ParseParam(const char* pszParam, BOOL bFlag, BOOL bLast) {
				CString msg;

		 switch(next_arg) {
			case cla_idle:
				if (bFlag && !strcmp(pszParam,"i")) {
					next_arg = cla_input;
				}
				else if (bFlag && !strcmp(pszParam,"o")) {
					next_arg = cla_output;
				}
				else if (bFlag && !strcmp(pszParam,"b")) {
					next_arg = cla_batchdir;
				}
				else if (bFlag && !strcmp(pszParam,"nogui")) {
					m_pCfg->cmdline_gui = false;
					next_arg = cla_idle;
				}
				else if (bFlag && !strcmp(pszParam,"scan")) {
					m_pCfg->bDecodeScanImg = true;
					next_arg = cla_idle;
				}
				else if (bFlag && !strcmp(pszParam,"maker")) {
					m_pCfg->bDecodeMaker = true;
					next_arg = cla_idle;
				}
				else if (bFlag && !strcmp(pszParam,"scandump")) {
					m_pCfg->bOutputScanDump = true;
					next_arg = cla_idle;
				}
				else if (bFlag && !strcmp(pszParam,"histo_y")) {
					m_pCfg->bDumpHistoY = true;
					next_arg = cla_idle;
				}
				else if (bFlag && !strcmp(pszParam,"dhtexp")) {
					m_pCfg->bOutputDHTexpand = true;
					next_arg = cla_idle;
				}
				else if (bFlag && !strcmp(pszParam,"exif_hide_unk")) {
					m_pCfg->bExifHideUnknown = true;
					next_arg = cla_idle;
				}
				else if (bFlag) {
					// Unknown flag
					tmpStr.Format(_T("ERROR: Unknown command-line flag [-%s]"),pszParam);
					AfxMessageBox(tmpStr);
					next_arg = cla_err;
				}
				else {
					// Not a flag, so assume it is a drag & drop file open
					m_pCfg->cmdline_open = true;
					m_pCfg->cmdline_open_fname = pszParam;

					m_nShellCommand = FileOpen;
					m_strFileName = pszParam;
				}

				break;

			case cla_input:
				msg = "Input=[";
				msg += pszParam;
				msg += "]";
				m_pCfg->cmdline_open = true;
				m_pCfg->cmdline_open_fname = pszParam;

				m_nShellCommand = FileOpen;
				m_strFileName = pszParam;

				next_arg = cla_idle;
				break;

			// TODO: Batch processing
			case cla_batchdir:
				msg = "BatchDir=[";
				msg += pszParam;
				msg += "]";

				// TODO:
				//   Need to create a member variable to store
				//   the batch directory name.
				//m_pCfg->cmdline_batch = true;
				//m_pCfg->cmdline_batch_dirname = pszParam;

				//   Also set the ShellCommand so that the InitProcess()
				//   call can determine which action to take. We should
				//   probably define a new value for the ShellCommand
				//   specific to batch processing.
				//m_nShellCommand = FileOpen;
				//m_strFileName = pszParam;

				next_arg = cla_idle;
				break;

			case cla_output:
				msg = "Output=[";
				msg += pszParam;
				msg += "]";
				m_pCfg->cmdline_output = true;
				m_pCfg->cmdline_output_fname = pszParam;
				next_arg = cla_idle;
				break;

			case cla_err:
			default:
				break;
		 }

     };
}; 



// CJPEGsnoopApp construction

CJPEGsnoopApp::CJPEGsnoopApp()
{


	m_bFatal = false;

	// Application-level config options
	m_pAppConfig = new CSnoopConfig();
	if (!m_pAppConfig) {
		AfxMessageBox("ERROR: Couldn't allocate memory for SnoopConfig");
		m_bFatal = true;
	}

	m_pDbSigs = new CDbSigs();
	if (!m_pDbSigs) {
		AfxMessageBox("ERROR: Couldn't allocate memory for DbSigs");
		m_bFatal = true;
	}


}

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
	SetRegistryKey(_T(REG_COMPANY_NAME));
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

	//CAL! FIXME
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
	//CAL! CCommandLineInfo cmdInfo;
	CMyCommandParser cmdInfo(m_pAppConfig);
	ParseCommandLine(cmdInfo);
	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	/*
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;
	*/


	//CAL!
	// We need rich edit control already present before we start any
	// command-line handling. Therefore, we probably want to force
	// a FileNew so that we get a control to work with. There doesn't
	// seem to be any harm with doing this.
	//
	// BUG: One issue with the above is that it ends up calling
	// OnFileNew() twice because of the CCommandLineInfo code below
	//
	// We also do a show-window so that we don't get partial drawn frames
	// when we are processing a file. Normally, we wouldn't want to display
	// the main window on some of these command-line options, but for now
	// we'll leave it up until I figure out how to hide it.
	OnFileNew();
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();

	//CAL! Do my own ProcessShellCommand(). The following is based
	// on part of what exists in appui2.cpp
	BOOL bResult = TRUE;
	switch (cmdInfo.m_nShellCommand) {
		// Note that FileNew is the default
		case CCommandLineInfo::FileNew:
			if (!OnCmdMsg(ID_FILE_NEW, 0, NULL, NULL))
				OnFileNew();
			if (m_pMainWnd == NULL)
				bResult = FALSE;
			break;

		//CAL! Note: We expect that the RichEditView is already created!
		case CCommandLineInfo::FileOpen:
			if (!OpenDocumentFile(cmdInfo.m_strFileName))
				bResult = FALSE;
			break;
		default:
			bResult = FALSE;
			break;

		//CAL! TODO:
		//  Here is where I should probably add in a handler for
		//  the batch processing invoked from the command line.
		//  Ideally, I'd like to call JPEGsnoopDoc::batchProcess()!
	}
	if (!bResult)
		return FALSE;


	// If we wanted quiet operation, don't leave up the window!
	// We might even want the OpenDocumentFile() to skip the window part as well,
	// if possible...
	if (m_pAppConfig->cmdline_gui == false) {
		return FALSE;
	}
	//CAL! We only leave here if there is a window to show (bResult)

	
	// The one and only window has been initialized, so show and update it
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	// call DragAcceptFiles only if there's a suffix
	//  In an SDI app, this should occur after ProcessShellCommand
	

	return TRUE;



	/* CAL! Original templated code

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;
	// The one and only window has been initialized, so show and update it
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	// call DragAcceptFiles only if there's a suffix
	//  In an SDI app, this should occur after ProcessShellCommand
	return TRUE;

	*/

}


// If it has been more than "nUpdateAutoDays" days since our
// last check for a recent update to the software, check the
// website for a newer version.
void CJPEGsnoopApp::CheckUpdates(bool force_now)
{
	CString		strUpdateLastChk = m_pAppConfig->strUpdateLastChk;

    unsigned nCheckYear,nCheckMon,nCheckDay;
	if (strUpdateLastChk.GetLength()==8) {
		nCheckYear = atoi(strUpdateLastChk.Mid(0,4));
		nCheckMon  = atoi(strUpdateLastChk.Mid(4,2));
		nCheckDay  = atoi(strUpdateLastChk.Mid(6,2));
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

	if ((force_now) || (tmeDiff >= tmePeriod)) {
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
		strCurDate = tmeToday.Format("%Y%m%d");
		m_pAppConfig->strUpdateLastChk = strCurDate;
		m_pAppConfig->Dirty();

	}

}

// Scrape the header of the web page to determine if a newer version
// of the software is available.
bool CJPEGsnoopApp::CheckUpdatesWww()
{

	CString	strVerLatest = "";
	CString strDateLatest = "";

	CString submit_host;
	CString submit_page;
	submit_host = IA_HOST;
	submit_page = IA_UPDATES_CHK_PAGE;

	static LPSTR acceptTypes[2]={"*/*", NULL};
	HINTERNET hINet, hConnection, hData;

	unsigned len;

	CString		strFormat;
	CString		strFormData;
	unsigned	nFormDataLen;
	CString		strHeaders =
		_T("Content-Type: application/x-www-form-urlencoded");
	strFormat  = "ver=%s";

	//*** Need to sanitize data for URL submission!
	// Search for "&", "?", "="
	strFormData.Format(strFormat,VERSION_STR);
	nFormDataLen = strFormData.GetLength();

	CString tmpStr;

	CHAR buffer[2048] ;
	CString m_strContents ;
	DWORD dwRead; //dwStatus;
	hINet = InternetOpen("JPEGsnoop/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
	if ( !hINet )
	{
		AfxMessageBox("InternetOpen Failed");
		return false;
	}
	try
	{
		hConnection = InternetConnect( hINet, (LPCTSTR)submit_host, 80, NULL,NULL, INTERNET_SERVICE_HTTP, 0, 1 );
		if ( !hConnection )
		{
			InternetCloseHandle(hINet);
			return false;
		}
		hData = HttpOpenRequest( hConnection, "POST", (LPCTSTR)submit_page, NULL, NULL, NULL, 0, 1 );
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
			AfxMessageBox("ERROR: Couldn't SendRequest");
			return false;
		}

		
		// Only read the first 1KB of page
		bool		bScrapeDone = false;
		unsigned	nScrapeLen = 0;
		unsigned	nScrapeMax = 1024;
		while (!bScrapeDone) {
			if (!InternetReadFile( hData, buffer, 255, &dwRead )) {
				bScrapeDone = true;
			} else {
				if ( dwRead == 0 ) {
					bScrapeDone = true;
					break;
				}
				buffer[dwRead] = 0;
				m_strContents += buffer;
				nScrapeLen += dwRead;
				if (nScrapeLen >= nScrapeMax) {
					bScrapeDone = true;
				}
			}
		}
		
		// Parse the HTTP result and search for the latest
		// version identification string
		len = m_strContents.GetLength();

		CString	strData;
		CString strParam,strVal;
		int	indDataStart = -1;
		int indDataEnd = -1;
		unsigned indDataLen = 0;
		bool bFoundRange = true;
		indDataStart = m_strContents.Find("***(")+4;
		indDataEnd = m_strContents.Find(")***");

		if ((indDataStart == -1) || (indDataEnd == -1)) {
			// Couldn't find start or end
			bFoundRange = false;
		}

		if (indDataStart > indDataEnd) {
			// Start and End positions look wrong
			bFoundRange = false;
		}

		if (bFoundRange) {
			// Found start & end markers
			indDataLen = indDataEnd-indDataStart;
			strData = m_strContents.Mid(indDataStart,indDataLen);
			bool done = false;
			bool tok_done;
			CString strCh;
			unsigned ind = 0;
			while (!done) {
				if (ind >= indDataLen) {
					done = true;
				} else {
					tok_done = false;
					strParam = "";
					while (!tok_done) {
						strCh = strData.GetAt(ind);
						if (strCh=="=") {
							tok_done = true;
							ind+=2; // skip [
						} else {
							strParam += strCh;
							ind++;
						}
					}

					tok_done = false;
					strVal = "";
					while (!tok_done) {
						strCh = strData.GetAt(ind);
						if (strCh=="]") {
							tok_done = true;
							ind+=2; // skip ,
						} else {
							strVal += strCh;
							ind++;
						}
					}

					//tmpStr.Format("Param[%s] Val[%s]",strParam,strVal);
					//AfxMessageBox(tmpStr);

					if (strParam == "latest_ver") {
						strVerLatest = strVal;
					}
					if (strParam == "latest_date") {
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
		//AfxMessageBox("EXCEPTION!");
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

CString CJPEGsnoopApp::RemoveTokenWithSeparators(CString& string, LPCTSTR charset) {
  CString token = string.SpanExcluding(charset);
  string = string.Mid(token.GetLength());
  return token;
}

CString CJPEGsnoopApp::RemoveTokenFromCharset(CString& string, LPCTSTR charset) {
  CString token = string.SpanIncluding(charset);
  string = string.Mid(token.GetLength());
  return token;
}

// Ensure that the EULA has been previously signed, and if not, present
// the user with a dialog box to sign.
bool CJPEGsnoopApp::CheckEula()
{
	if (m_pAppConfig->bEulaAccepted) {
		return true;
	} else {
		CTermsDlg	dlg;
		if (dlg.DoModal() == IDOK) {
			// They must have accepted the terms, update the
			// acceptance in the registry
			m_pAppConfig->bEulaAccepted = dlg.bEulaOk;
			m_pAppConfig->bUpdateAuto = dlg.bUpdateAuto;
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

void CJPEGsnoopApp::MyOnFileOpen()
{

	bool status = 0;
	char strFilter[] =
		"JPEG Image (*.jpg;*.jpeg)|*.jpg;*.jpeg|"\
		"Thumbnail (*.thm)|*.thm|"\
		"AVI Movie (*.avi)|*.avi|"\
		"QuickTime Movie (*.mov)|*.mov|"\
		"Digital Negative (*.dng)|*.dng|"\
		"RAW Image (*.crw;*.cr2;*.nef;*.orf;*.pef)|*.crw;*.cr2;*.nef;*.orf;*.pef|"\
		"PDF Files (*.pdf)|*.pdf|"\
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

	//CAL!: BUG 1008
	CFileDialog FileDlg(TRUE, ".jpg", NULL, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST, strFilter);

	CString title;
	CString sFileName;
	VERIFY(title.LoadString(IDS_CAL_FILEOPEN));
	FileDlg.m_ofn.lpstrTitle = title;
	
	if( FileDlg.DoModal() == IDOK )
	{
		sFileName = FileDlg.GetFileName();
		OpenDocumentFile(sFileName);
		// if returns NULL, the user has already been alerted
	}


}

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

// Inform the document to reprocess the file (if it exists)
void CJPEGsnoopApp::DocReprocess()
{
	CJPEGsnoopDoc* pMyDoc;
	pMyDoc = GetCurDoc();
	if (pMyDoc) {
		pMyDoc->Reprocess();
	}
}

// Mark the image view as needing update. Will take
// effect on next "Reprocess" call.
void CJPEGsnoopApp::DocImageDirty()
{
	CJPEGsnoopDoc* pMyDoc;
	pMyDoc = GetCurDoc();
	if (pMyDoc) {
		pMyDoc->m_pJfifDec->ImgSrcChanged();
	}
}

// -------------------- Handle Menu actions ------------------

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

// Even though "Noidct" is the opposite of "Fullidct", the menu
// option simply toggles the state, so we can reuse the handler.
void CJPEGsnoopApp::OnScansegmentNoidct()
{
	OnScansegmentFullidct();
}




// -------------------- Handle Menu status ------------------

void CJPEGsnoopApp::OnUpdateOptionsDhtexpand(CCmdUI *pCmdUI)
{
	if (m_pAppConfig->bOutputDHTexpand) {
		pCmdUI->SetCheck(TRUE);
	} else {
		pCmdUI->SetCheck(FALSE);
	}
}

void CJPEGsnoopApp::OnUpdateOptionsMakernotes(CCmdUI *pCmdUI)
{
	if (m_pAppConfig->bDecodeMaker) {
		pCmdUI->SetCheck(TRUE);
	} else {
		pCmdUI->SetCheck(FALSE);
	}
}


void CJPEGsnoopApp::OnUpdateScansegmentDump(CCmdUI *pCmdUI)
{
	if (m_pAppConfig->bOutputScanDump) {
		pCmdUI->SetCheck(TRUE);
	} else {
		pCmdUI->SetCheck(FALSE);
	}
}


void CJPEGsnoopApp::OnUpdateScansegmentDecodeimage(CCmdUI *pCmdUI)
{
	if (m_pAppConfig->bDecodeScanImg) {
		pCmdUI->SetCheck(TRUE);
	} else {
		pCmdUI->SetCheck(FALSE);
	}
}


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

// Opposite functionality of Fullidct
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


// Launch the configuration options dialog box
// and pre-populate it with the current m_pAppConfig
// settings. If the user hits OK, then the revised
// setttings are stored back into the registry.
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
		m_pAppConfig->bUpdateAuto = setDlg.m_bUpdateAuto;
		m_pAppConfig->nUpdateAutoDays = setDlg.m_nUpdateChkDays;
		m_pAppConfig->strDbDir = setDlg.m_strDbDir;
		m_pAppConfig->bReprocessAuto = setDlg.m_bReprocessAuto;
		m_pAppConfig->bDbSubmitNet = setDlg.m_bDbSubmitNet;
		m_pAppConfig->nErrMaxDecodeScan = setDlg.m_nRptErrMaxScanDecode;
		//m_pAppConfig->bStatClipEn = setDlg.m_bReportClip;
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

void CJPEGsnoopApp::OnOptionsCheckforupdates()
{
	CheckUpdates(true);
}


// Present the user with the Manage Local signature database dialog.
// This dialog enables the user to view and delete entries that are
// defined and stored within the current installation file.
void CJPEGsnoopApp::OnToolsManagelocaldb()
{
	CDbManageDlg	manageDlg;
	CJPEGsnoopDoc* pMyDoc;
	pMyDoc = GetCurDoc();
	if (pMyDoc) {
		pMyDoc->m_pJfifDec->ImgSrcChanged();

		unsigned	n_DbUserEntries;
		CString		strTmp;
		CompSig		mySig;
		n_DbUserEntries = m_pDbSigs->DatabaseExtraGetNum();

		for (unsigned ind=0;ind<n_DbUserEntries;ind++) {
			mySig = m_pDbSigs->DatabaseExtraGet(ind);

			if (mySig.m_editor == ENUM_SOURCE_SW) {
				manageDlg.InsertEntry(ind,mySig.m_swdisp,"",mySig.um_qual,mySig.c_sig);
			} else {
				manageDlg.InsertEntry(ind,mySig.x_make,mySig.x_model,mySig.um_qual,mySig.c_sig);
			}
		}

		if (manageDlg.DoModal() == IDOK) {
			// Now determine which entries were deleted
			CUIntArray	anDeleted;
			manageDlg.GetDeleted(anDeleted);
			for (unsigned ind=0;ind<(unsigned)anDeleted.GetCount();ind++) {
                m_pDbSigs->DatabaseExtraMarkDelete(anDeleted.GetAt(ind));

			}

			m_pDbSigs->DatabaseExtraStore();
			m_pDbSigs->DatabaseExtraLoad();
	

		}

	}

}

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

void CJPEGsnoopApp::OnUpdateOptionsSignaturesearch(CCmdUI *pCmdUI)
{
	if (m_pAppConfig->bSigSearch) {
		pCmdUI->SetCheck(TRUE);
	} else {
		pCmdUI->SetCheck(FALSE);
	}
}


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

void CJPEGsnoopApp::OnUpdateOptionsHideuknownexiftags(CCmdUI *pCmdUI)
{
	if (m_pAppConfig->bExifHideUnknown) {
		pCmdUI->SetCheck(TRUE);
	} else {
		pCmdUI->SetCheck(FALSE);
	}
}

void CJPEGsnoopApp::OnFileBatchprocess()
{
	// TODO: Add your command handler code here
	CJPEGsnoopDoc* pMyDoc;
	pMyDoc = GetCurDoc();
	if (pMyDoc) {
		pMyDoc->batchProcess();
	}
}
