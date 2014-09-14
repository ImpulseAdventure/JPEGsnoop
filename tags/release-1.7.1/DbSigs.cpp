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

#include "stdafx.h"
#include "DbSigs.h"

#include "snoop.h"

#include "Signatures.inl"

#define	MAX_BUF_EX_ERR_MSG	512
#define	MAX_BUF_SET_FILE	131072


// TODO:
// - Convert m_sSigListExtra[] to use CObArray instead of managing it
//   directly. Otherwise we are inefficient with memory and could potentially
//   not allocate enough space.


// Initialize the signature database array sizes
//
// PRE:
// - m_sSigList[]
// - m_sExcMmNoMkrList[]
// - m_sExcMmIsEditList[]
// - m_sSwIjgList[]
// - m_sXComSwList[]
//
// POST:
// - m_nSigListNum
// - m_nSigListExtraNum
// - m_nExcMmNoMkrListNum
// - m_nExcMmIsEditListNum
// - m_nSwIjgListNum
// - m_nXcomSwListNum
// - m_strDbDir
//
CDbSigs::CDbSigs()
{
	// Count the built-in database
	bool bDone;

	bDone = false;
	m_nSigListNum = 0;
	while (!bDone)
	{
		if (!_tcscmp(m_sSigList[m_nSigListNum].strXMake,_T("*")))
			bDone = true;
		else
			m_nSigListNum++;
	}

	// Count number of exceptions in Signatures.inl
	bDone = false;
	m_nExcMmNoMkrListNum = 0;
	while (!bDone)
	{
		if (!_tcscmp(m_sExcMmNoMkrList[m_nExcMmNoMkrListNum].strXMake,_T("*")))
			bDone = true;
		else
			m_nExcMmNoMkrListNum++;
	}

	bDone = false;
	m_nExcMmIsEditListNum = 0;
	while (!bDone)
	{
		if (!_tcscmp(m_sExcMmIsEditList[m_nExcMmIsEditListNum].strXMake,_T("*")))
			bDone = true;
		else
			m_nExcMmIsEditListNum++;
	}


	bDone = false;
	m_nSwIjgListNum = 0;
	while (!bDone) {
		if (!_tcscmp(m_sSwIjgList[m_nSwIjgListNum],_T("*")))
			bDone = true;
		else
			m_nSwIjgListNum++;
	}

	bDone = false;
	m_nXcomSwListNum = 0;
	while (!bDone) {
		if (!_tcscmp(m_sXComSwList[m_nXcomSwListNum],_T("*")))
			bDone = true;
		else
			m_nXcomSwListNum++;
	}


	// Reset extra database
	m_nSigListExtraNum = 0;

	// Default to user database dir not set yet
	// This will cause a fail if database load/store
	// functions are called before SetDbDir()
	m_strDbDir = _T("");

}

CDbSigs::~CDbSigs()
{
}

unsigned CDbSigs::GetNumSigsInternal()
{
	return m_nSigListNum;
}

unsigned CDbSigs::GetNumSigsExtra()
{
	return m_nSigListExtraNum;
}

// Read an unsigned integer (4B) from the buffer
bool CDbSigs::BufReadNum(PBYTE pBuf,unsigned &nOut,unsigned nMaxBytes,unsigned &nOffsetBytes)
{
	ASSERT(pBuf);
	// TODO: check for buffer bounds
	nOut = (unsigned)pBuf[nOffsetBytes];
	nOffsetBytes += sizeof(unsigned);
	return true;
}

// Write an unsigned integer (4B) to the buffer
bool CDbSigs::BufWriteNum(PBYTE pBuf,unsigned nIn,unsigned nMaxBytes,unsigned &nOffsetBytes)
{
	ASSERT(pBuf);
	// TODO: check for buffer bounds
	PBYTE		pBufBase;
	unsigned*	pBufInt;
	pBufBase = &pBuf[nOffsetBytes];
	pBufInt = (unsigned*)pBufBase;
	pBufInt[0] = nIn;
	nOffsetBytes += sizeof(unsigned);
	return true;
}


// Attempt to read a line from the buffer
// This is a replacement for CStdioFile::ReadLine()
// Both 16-bit unicode and 8-bit SBCS encoding modes are supported (via bUni)
// Offset parameter is incremented accordingly
// Supports both newline and NULL for string termination
bool CDbSigs::BufReadStr(PBYTE pBuf,CString &strOut,unsigned nMaxBytes,bool bUni,unsigned &nOffsetBytes)
{
	ASSERT(pBuf);
	CString		strOutTmp;
	bool		bDone;

	char		chAsc;
	wchar_t		chUni;
	unsigned	nCharSz = ((bUni)?sizeof(wchar_t):sizeof(char));

	bDone = false;
	strOut = _T("");
	// Ensure we don't overrun the buffer by calculating the last
	// byte index required for each iteration.
	for (unsigned nInd=nOffsetBytes;(!bDone)&&(nInd+nCharSz-1<nMaxBytes);nInd+=nCharSz) {
		if (bUni) {
			chUni = pBuf[nInd];
			if ( (chUni != '\n') && (chUni != 0) ) {
				strOut += chUni;
			} else {
				bDone = true;
				nOffsetBytes = nInd+nCharSz;
			}
		} else {
			chAsc = pBuf[nInd];
			if ( (chAsc != '\n') && (chAsc != 0) ) {
				strOut += chAsc;
			} else {
				bDone = true;
				nOffsetBytes = nInd+nCharSz;
			}
		}
	}

	if (!bDone) {
		nOffsetBytes = nMaxBytes;
		// The input was not terminated, so we're still going to return what we got so far
		return false;
	}

	return true;
}

// Return true if we managed to write entire string including terminator
// without overrunning nMaxBytes
bool CDbSigs::BufWriteStr(PBYTE pBuf,CString strIn,unsigned nMaxBytes,bool bUni,unsigned &nOffsetBytes)
{
	ASSERT(pBuf);

	bool		bRet = false;
	char		chAsc;
	wchar_t		chUni;
	unsigned	nCharSz = ((bUni)?sizeof(wchar_t):sizeof(char));
	PBYTE		pBufBase;
	LPWSTR		pBufUni;
	LPSTR		pBufAsc;

	pBufBase = pBuf + nOffsetBytes;
	pBufUni = (LPWSTR)pBufBase;
	pBufAsc = (LPSTR)pBufBase;

#ifdef UNICODE
	// Create non-Unicode version of string
	// Ref: http://social.msdn.microsoft.com/Forums/vstudio/en-US/85f02321-de88-47d2-98c8-87daa839a98e/how-to-convert-cstring-to-const-char-?forum=vclanguage
	// Added constant specifier
	LPCSTR		pszNonUnicode;
	USES_CONVERSION;
	// Not specifying code page explicitly but assume content
	// should be ASCII. Default code page is probably Windows-1252.
	pszNonUnicode = CW2A( strIn.LockBuffer( ) );
	strIn.UnlockBuffer( );
#endif


	unsigned	nStrLen;
	unsigned	nChInd;
	nStrLen = strIn.GetLength();
	for (nChInd=0;(nChInd<nStrLen)&&(nOffsetBytes+nCharSz-1<nMaxBytes);nChInd++) {
		if (bUni) {
			// Normal handling for unicode
			chUni = strIn.GetAt(nChInd);
			pBufUni[nChInd] = chUni; 
		} else {

#ifdef UNICODE
			// To avoid Warning C4244: Conversion from 'wchar_t' to 'char' possible loss of data
			// We need to implement conversion here
			// Ref: http://stackoverflow.com/questions/4786292/converting-unicode-strings-and-vice-versa

			// Since we have compiled for unicode, the CString character fetch
			// will be unicode char. Therefore we need to use ANSI-converted form.
			chAsc = pszNonUnicode[nChInd];
#else
			// Since we have compiled for non-Unicode, the CString character fetch
			// will be single byte char
			chAsc = strIn.GetAt(nChInd);
#endif
			pBufAsc[nChInd] = chAsc; 
		}
		// Advance pointers
		nOffsetBytes += nCharSz;
	}

	// Now terminate if we have space
	if ((nOffsetBytes + nCharSz-1) < nMaxBytes) {
		if (bUni) {
			chUni = wchar_t(0);
			pBufUni[nChInd] = chUni;
		} else {
			chAsc = char(0);
			pBufAsc[nChInd] = chAsc;
		}
		// Advance pointers
		nOffsetBytes += nCharSz;

		// Since we managed to include terminator, return is successful
		bRet = true;
	}

	// Return true if we finished the string write (without exceeding nMaxBytes)
	// or false otherwise
	return bRet;
}


void CDbSigs::DatabaseExtraLoad()
{
	CFile		*pInFile = NULL;
	PBYTE		pBuf = NULL;
	unsigned	nBufLenBytes = 0;
	unsigned	nBufOffset = 0;

	ASSERT(m_strDbDir != _T(""));
	if (m_strDbDir == _T("")) {
		return;
	}

	// Retrieve from environment user profile path.
	TCHAR szFilePathName[200];
	_stprintf_s(szFilePathName,_T("%s\\%s"),(LPCTSTR)m_strDbDir,DAT_FILE);

	// Open the input file
	try
	{
		// Open specified file
		pInFile = new CFile(szFilePathName, CFile::modeRead );
	}
	catch (CFileException* e)
	{
		CString		strError;
		TCHAR		msg[MAX_BUF_EX_ERR_MSG];
		e->GetErrorMessage(msg,MAX_BUF_EX_ERR_MSG);
		e->Delete();
		strError.Format(_T("ERROR: Couldn't open file: [%s]"),(LPCTSTR)msg);
		OutputDebugString(strError);
		AfxMessageBox(strError);
		pInFile = NULL;
		return;
	}

	// Allocate the input buffer
	pBuf = new BYTE [MAX_BUF_SET_FILE];
	ASSERT(pBuf);

	// Load the buffer
	nBufLenBytes = pInFile->Read(pBuf,MAX_BUF_SET_FILE);

	ASSERT(nBufLenBytes>0);
	// TODO: Error check for config file longer than max buffer!
	ASSERT(nBufLenBytes<MAX_BUF_SET_FILE);
	nBufOffset = 0;

	CString		strLine;
	CString		strParam;
	CString		strVal;

	CString		strTmp;
	CString		strVer;
	CString		strSec;

	bool		bErr = false;
	bool		bDone = false;
	BOOL		bRet;

	unsigned	nBufOffsetTmp;
	bool		bFileOk = false;
	bool		bFileModeUni = false;

	bool		bValid;

	unsigned	nNumLoad = 0;				// Number of entries to read
	CompSig		sDbLocalEntry;				// Temp entry for load
	bool		bDbLocalEntryFound;		// Temp entry already in built-in DB?
	bool		bDbLocalTrimmed = false;	// Did we trim down the DB?


	// Determine if config file is in Unicode or SBCS
	// If the file is in SBCS (legacy) then back it up
	// and then write-back in unicode mode.

	// Test unicode mode
	nBufOffsetTmp = 0;
	bRet = BufReadStr(pBuf,strLine,nBufLenBytes,true,nBufOffsetTmp);
	if (strLine.Compare(_T("JPEGsnoop"))==0) {
		bFileOk = true;
		bFileModeUni = true;
		nBufOffset = nBufOffsetTmp;
	}

	// Test SBCS mode
	nBufOffsetTmp = 0;
	bRet = BufReadStr(pBuf,strLine,nBufLenBytes,false,nBufOffsetTmp);
	if (strLine.Compare(_T("JPEGsnoop"))==0) {
		bFileOk = true;
		nBufOffset = nBufOffsetTmp;
	}

	if (!bFileOk) {
		AfxMessageBox(_T("WARNING: Database file corrupt. Proceeding with defaults."));
		return;
	}
	
	m_nSigListExtraNum = 0;

	// Version
	bRet = BufReadStr(pBuf,strVer,nBufLenBytes,bFileModeUni,nBufOffset);
	bValid = false;
	if (strVer == _T("00")) { bValid = false; }
	if (strVer == _T("01")) { bValid = true; }
	if (strVer == _T("02")) { bValid = true; }
	if (strVer == _T("03")) { bValid = true; }

	// Should consider trimming down local database if same entry
	// exists in built-in database (for example, user starts running
	// new version of tool).

	while (!bDone && bValid) {

		// Read section header

		bRet = BufReadStr(pBuf,strSec,nBufLenBytes,bFileModeUni,nBufOffset);

		if (strSec == _T("*DB*")) {
			// Read DB count
			bRet = BufReadNum(pBuf,nNumLoad,nBufLenBytes,nBufOffset);

			// For each entry that we read, we should double-check to see
			// if we already have it in the built-in DB. If we do, then
			// don't add it to the runtime DB, and mark the local DB as
			// needing trimming. If so, rewrite the DB.

			for (unsigned ind=0;ind<nNumLoad;ind++) {

				sDbLocalEntry.bValid = false;
				sDbLocalEntry.strXMake = _T("");
				sDbLocalEntry.strXModel = _T("");
				sDbLocalEntry.strUmQual = _T("");
				sDbLocalEntry.strCSig = _T("");
				sDbLocalEntry.strCSigRot = _T("");
				sDbLocalEntry.strXSubsamp = _T("");
				sDbLocalEntry.strMSwTrim = _T("");
				sDbLocalEntry.strMSwDisp = _T("");

				bDbLocalEntryFound = false;


				if (strVer == _T("01")) {
					// For version 01:

					bRet = BufReadStr(pBuf,sDbLocalEntry.strXMake,nBufLenBytes,bFileModeUni,nBufOffset);
					bRet = BufReadStr(pBuf,sDbLocalEntry.strXModel,nBufLenBytes,bFileModeUni,nBufOffset);

					strTmp = _T("");
					bRet = BufReadStr(pBuf,strTmp,nBufLenBytes,bFileModeUni,nBufOffset);
					sDbLocalEntry.eEditor = static_cast<teEditor>(_tstoi(strTmp));

					bRet = BufReadStr(pBuf,sDbLocalEntry.strUmQual,nBufLenBytes,bFileModeUni,nBufOffset);
					bRet = BufReadStr(pBuf,sDbLocalEntry.strCSig,nBufLenBytes,bFileModeUni,nBufOffset);

					// In older version of DB, these entries won't exist
					bRet = BufReadStr(pBuf,sDbLocalEntry.strXSubsamp,nBufLenBytes,bFileModeUni,nBufOffset);
					bRet = BufReadStr(pBuf,sDbLocalEntry.strMSwTrim,nBufLenBytes,bFileModeUni,nBufOffset);
					bRet = BufReadStr(pBuf,sDbLocalEntry.strMSwDisp,nBufLenBytes,bFileModeUni,nBufOffset);

				} else if ( (strVer == _T("02")) || (strVer == _T("03")) ) {

					// For version 02 or 03:
					// NOTE: Difference between 02 and 03:
					//       - 02 : SBCS format
					//       - 03 : Unicode format
					bRet = BufReadStr(pBuf,sDbLocalEntry.strXMake,nBufLenBytes,bFileModeUni,nBufOffset);
					bRet = BufReadStr(pBuf,sDbLocalEntry.strXModel,nBufLenBytes,bFileModeUni,nBufOffset);

					strTmp = _T("");
					bRet = BufReadStr(pBuf,strTmp,nBufLenBytes,bFileModeUni,nBufOffset);
					sDbLocalEntry.eEditor = static_cast<teEditor>(_tstoi(strTmp));

					bRet = BufReadStr(pBuf,sDbLocalEntry.strUmQual,nBufLenBytes,bFileModeUni,nBufOffset);
					bRet = BufReadStr(pBuf,sDbLocalEntry.strCSig,nBufLenBytes,bFileModeUni,nBufOffset);
					bRet = BufReadStr(pBuf,sDbLocalEntry.strCSigRot,nBufLenBytes,bFileModeUni,nBufOffset);

					
					// In older version of DB, these entries won't exist
					bRet = BufReadStr(pBuf,sDbLocalEntry.strXSubsamp,nBufLenBytes,bFileModeUni,nBufOffset);
					bRet = BufReadStr(pBuf,sDbLocalEntry.strMSwTrim,nBufLenBytes,bFileModeUni,nBufOffset);
					bRet = BufReadStr(pBuf,sDbLocalEntry.strMSwDisp,nBufLenBytes,bFileModeUni,nBufOffset);
				}

				// -------------------------------------------------------


				// Does the entry already exist in the internal DB?
				bDbLocalEntryFound = SearchSignatureExactInternal(sDbLocalEntry.strXMake,sDbLocalEntry.strXModel,sDbLocalEntry.strCSig);

				if (!bDbLocalEntryFound) {
					// Add it!
					m_sSigListExtra[m_nSigListExtraNum].bValid    = true;
					m_sSigListExtra[m_nSigListExtraNum].strXMake    = sDbLocalEntry.strXMake;
					m_sSigListExtra[m_nSigListExtraNum].strXModel   = sDbLocalEntry.strXModel;
					m_sSigListExtra[m_nSigListExtraNum].eEditor  = sDbLocalEntry.eEditor;
					m_sSigListExtra[m_nSigListExtraNum].strUmQual   = sDbLocalEntry.strUmQual;
					m_sSigListExtra[m_nSigListExtraNum].strCSig     = sDbLocalEntry.strCSig;
					m_sSigListExtra[m_nSigListExtraNum].strCSigRot  = sDbLocalEntry.strCSigRot;

					m_sSigListExtra[m_nSigListExtraNum].strXSubsamp = sDbLocalEntry.strXSubsamp;
					m_sSigListExtra[m_nSigListExtraNum].strMSwTrim  = sDbLocalEntry.strMSwTrim;
					m_sSigListExtra[m_nSigListExtraNum].strMSwDisp  = sDbLocalEntry.strMSwDisp;
					m_nSigListExtraNum++;
				} else {
					bDbLocalTrimmed = true;
				}

			}
		} else if (strSec == _T("*Z*")) {
			bDone = true;
		} else {
			bValid = false;
		} // strSec

	} // while




	// ----------------------

	if (pInFile) {
		pInFile->Close();
		delete pInFile;
		pInFile = NULL;
	}

	if (pBuf) {
		delete [] pBuf;
		pBuf = NULL;
	}

	// If we did make changes to the database (trim), then rewrite it!
	// Ensure that we have closed the file above before we rewrite it
	// otherwise we could end up with a sharing violation
	if (bDbLocalTrimmed) {
		DatabaseExtraStore();
	}

	// Now is this an old config file version? If so,
	// create a backup and then rewrite the new version
	if (bValid) {
		if (strVer != DB_VER_STR) {
			// Copy old config file
			TCHAR szFilePathNameBak[200];
			_stprintf_s(szFilePathNameBak,_T("%s\\%s.bak"),(LPCTSTR)m_strDbDir,DAT_FILE);
			CopyFile(szFilePathName,szFilePathNameBak,false);
			// Now rewrite file in latest version
			DatabaseExtraStore();
			// Notify user
			strTmp.Format(_T("Upgraded signature database. Backup in:\n%s"),szFilePathNameBak);
			AfxMessageBox(strTmp);
		}
	}
}

void CDbSigs::DatabaseExtraClean()
{
	m_nSigListExtraNum = 0;
	DatabaseExtraStore();
}

unsigned CDbSigs::DatabaseExtraGetNum()
{
	return m_nSigListExtraNum;
}

CompSig CDbSigs::DatabaseExtraGet(unsigned nInd)
{
	ASSERT(nInd<m_nSigListExtraNum);
	return m_sSigListExtra[nInd];
}

void CDbSigs::DatabaseExtraStore()
{
	CFile		*pOutFile = NULL;
	PBYTE		pBuf = NULL;
	unsigned	nBufLenBytes = 0;
	unsigned	nBufOffset = 0;

	bool		bModeUni = true;	// Save in Unicode format

	ASSERT(m_strDbDir != _T(""));
	if (m_strDbDir == _T("")) {
		return;
	}

	// Retrieve from environment user profile path.
	TCHAR szFilePathName[200];
	_stprintf_s(szFilePathName,_T("%s\\%s"),(LPCTSTR)m_strDbDir,DAT_FILE);


	// Open the output file
	try
	{
		// Open specified file
		pOutFile = new CFile(szFilePathName, CFile::modeWrite | CFile::typeBinary | CFile::modeCreate );
	}
	catch (CFileException* e)
	{
		CString		strError;
		TCHAR		msg[MAX_BUF_EX_ERR_MSG];
		e->GetErrorMessage(msg,MAX_BUF_EX_ERR_MSG);
		e->Delete();
		strError.Format(_T("ERROR: Couldn't open file: [%s]"),(LPCTSTR)msg);
		OutputDebugString(strError);
		AfxMessageBox(strError);
		pOutFile = NULL;
		return;
	}

	// Allocate the output buffer
	pBuf = new BYTE [MAX_BUF_SET_FILE];
	ASSERT(pBuf);

	nBufOffset = 0;

	CString		strLine;
	CString		strParam;
	CString		strVal;
	bool		bErr = false;
	bool		bDone = false;
	BOOL		bRet;

	unsigned	nMaxBufBytes = MAX_BUF_SET_FILE;

	bRet = BufWriteStr(pBuf,_T("JPEGsnoop"),nMaxBufBytes,bModeUni,nBufOffset);
	bRet = BufWriteStr(pBuf,_T(DB_VER_STR),nMaxBufBytes,bModeUni,nBufOffset);
	bRet = BufWriteStr(pBuf,_T("*DB*"),nMaxBufBytes,bModeUni,nBufOffset);

	// Determine how many entries will remain (after removing marked
	// deleted entries
	unsigned nNewExtraNum = m_nSigListExtraNum;
	for (unsigned nInd=0;nInd<m_nSigListExtraNum;nInd++) {
		if (!m_sSigListExtra[nInd].bValid) {
			nNewExtraNum--;
		}
	}

	bRet = BufWriteNum(pBuf,nNewExtraNum,nMaxBufBytes,nBufOffset);

	for (unsigned nInd=0;nInd<m_nSigListExtraNum;nInd++) {

		// Is it marked for deletion (from Manage dialog?)
		if (!m_sSigListExtra[nInd].bValid) {
			continue;
		}

		CString	strTmp;

		// For version 02?

		bRet = BufWriteStr(pBuf,m_sSigListExtra[nInd].strXMake,nMaxBufBytes,bModeUni,nBufOffset);
		bRet = BufWriteStr(pBuf,m_sSigListExtra[nInd].strXModel,nMaxBufBytes,bModeUni,nBufOffset);
		strTmp.Format(_T("%u"),m_sSigListExtra[nInd].eEditor);				// eEditor = u_source
		bRet = BufWriteStr(pBuf,strTmp,nMaxBufBytes,bModeUni,nBufOffset);
		bRet = BufWriteStr(pBuf,m_sSigListExtra[nInd].strUmQual,nMaxBufBytes,bModeUni,nBufOffset);
		bRet = BufWriteStr(pBuf,m_sSigListExtra[nInd].strCSig,nMaxBufBytes,bModeUni,nBufOffset);
		bRet = BufWriteStr(pBuf,m_sSigListExtra[nInd].strCSigRot,nMaxBufBytes,bModeUni,nBufOffset);
		bRet = BufWriteStr(pBuf,m_sSigListExtra[nInd].strXSubsamp,nMaxBufBytes,bModeUni,nBufOffset);
		bRet = BufWriteStr(pBuf,m_sSigListExtra[nInd].strMSwTrim,nMaxBufBytes,bModeUni,nBufOffset);
		bRet = BufWriteStr(pBuf,m_sSigListExtra[nInd].strMSwDisp,nMaxBufBytes,bModeUni,nBufOffset);

	}

	bRet = BufWriteStr(pBuf,_T("*Z*"),nMaxBufBytes,bModeUni,nBufOffset);

	// --------------------------

	// Now write out the buffer
	if (pOutFile) {
		pOutFile->Write(pBuf,nBufOffset);

		pOutFile->Close();
		delete pOutFile;
		pOutFile = NULL;
	}

	if (pBuf) {
		delete [] pBuf;
		pBuf = NULL;
	}
	
}

void CDbSigs::DatabaseExtraAdd(CString strExifMake,CString strExifModel,
							   CString strQual,CString strSig,CString strSigRot,CString strCss,
							   teSource eUserSource,CString strUserSoftware)
{
	ASSERT(m_nSigListExtraNum < DBEX_ENTRIES_MAX);
	if (m_nSigListExtraNum >= DBEX_ENTRIES_MAX) {
		CString strTmp;
		strTmp.Format(_T("ERROR: Can only store maximum of %u extra signatures in local DB"),DBEX_ENTRIES_MAX);
		AfxMessageBox(strTmp);
		return;
	}

		// Now append it to the local database and resave
		m_sSigListExtra[m_nSigListExtraNum].strXMake    = strExifMake;
		m_sSigListExtra[m_nSigListExtraNum].strXModel   = strExifModel;
		m_sSigListExtra[m_nSigListExtraNum].strUmQual   = strQual;
		m_sSigListExtra[m_nSigListExtraNum].strCSig     = strSig;
		m_sSigListExtra[m_nSigListExtraNum].strCSigRot  = strSigRot;
		m_sSigListExtra[m_nSigListExtraNum].strXSubsamp = strCss;

		if (eUserSource == ENUM_SOURCE_CAM) { // digicam
			m_sSigListExtra[m_nSigListExtraNum].eEditor = ENUM_EDITOR_CAM;
			m_sSigListExtra[m_nSigListExtraNum].strMSwTrim = _T("");
			m_sSigListExtra[m_nSigListExtraNum].strMSwDisp = _T("");
		} else if (eUserSource == ENUM_SOURCE_SW) { // software
			m_sSigListExtra[m_nSigListExtraNum].eEditor = ENUM_EDITOR_SW;
			m_sSigListExtra[m_nSigListExtraNum].strMSwTrim = _T("");
			m_sSigListExtra[m_nSigListExtraNum].strMSwDisp = strUserSoftware; // Not quite right perfect
			m_sSigListExtra[m_nSigListExtraNum].strXMake   = _T("");
			m_sSigListExtra[m_nSigListExtraNum].strXModel  = _T("");
		} else { // user didn't know
			m_sSigListExtra[m_nSigListExtraNum].eEditor = ENUM_EDITOR_UNSURE;
			m_sSigListExtra[m_nSigListExtraNum].strMSwTrim = _T("");
			m_sSigListExtra[m_nSigListExtraNum].strMSwDisp = strUserSoftware; // Not quite right perfect
		}

		m_nSigListExtraNum++;

		// Now resave the database
		DatabaseExtraStore();
}

// TODO: Should we include editors in this search?
bool CDbSigs::SearchSignatureExactInternal(CString strMake, CString strModel, CString strSig)
{
	bool		bFoundExact = false;
	bool		bDone = false;
	unsigned	nInd = 0;
	while (!bDone) {
		if (nInd >= m_nSigListNum) {
			bDone = true;
		} else {

			if ( (m_sSigList[nInd].strXMake  == strMake) &&
				(m_sSigList[nInd].strXModel == strModel) &&
				((m_sSigList[nInd].strCSig  == strSig) || (m_sSigList[nInd].strCSigRot == strSig)) )
			{
				bFoundExact = true;
				bDone = true;
			}
			nInd++;
		}
	}

	return bFoundExact;
}

bool CDbSigs::SearchCom(CString strCom)
{
	bool bFound = false;
	bool bDone = false;
	unsigned nInd = 0;
	if (strCom.GetLength() == 0) {
		bDone = true;
	}
	while (!bDone) {
		if (nInd >= m_nXcomSwListNum) {
			bDone = true;
		} else {
			if (strCom.Find(m_sXComSwList[nInd]) != -1) {
				bFound = true;
				bDone = true;
			}
			nInd++;
		}
	}
	return bFound;
}

// Returns total of built-in plus local DB
unsigned CDbSigs::GetDBNumEntries()
{
	return (m_nSigListNum + m_nSigListExtraNum);
}

// Returns total of built-in plus local DB
unsigned CDbSigs::IsDBEntryUser(unsigned nInd)
{
	if (nInd < m_nSigListNum) {
		return false;
	} else {
		return true;
	}
}

// Return a ptr to the struct containing the indexed entry
bool CDbSigs::GetDBEntry(unsigned nInd,CompSig* pEntry)
{
	unsigned nIndOffset;
	unsigned nIndMax = GetDBNumEntries();
	ASSERT(pEntry);
	ASSERT(nInd<nIndMax);
	if (nInd < m_nSigListNum) {
		pEntry->eEditor  = m_sSigList[nInd].eEditor;
		pEntry->strXMake    = m_sSigList[nInd].strXMake;
		pEntry->strXModel   = m_sSigList[nInd].strXModel;
		pEntry->strUmQual   = m_sSigList[nInd].strUmQual;
		pEntry->strCSig     = m_sSigList[nInd].strCSig;
		pEntry->strCSigRot  = m_sSigList[nInd].strCSigRot;
		pEntry->strXSubsamp = m_sSigList[nInd].strXSubsamp;
		pEntry->strMSwTrim  = m_sSigList[nInd].strMSwTrim;
		pEntry->strMSwDisp  = m_sSigList[nInd].strMSwDisp;
		return true;
	} else {
		nIndOffset = nInd-m_nSigListNum;
		pEntry->eEditor  = m_sSigListExtra[nIndOffset].eEditor;
		pEntry->strXMake    = m_sSigListExtra[nIndOffset].strXMake;
		pEntry->strXModel   = m_sSigListExtra[nIndOffset].strXModel;
		pEntry->strUmQual   = m_sSigListExtra[nIndOffset].strUmQual;
		pEntry->strCSig     = m_sSigListExtra[nIndOffset].strCSig;
		pEntry->strCSigRot  = m_sSigListExtra[nIndOffset].strCSigRot;
		pEntry->strXSubsamp = m_sSigListExtra[nIndOffset].strXSubsamp;
		pEntry->strMSwTrim  = m_sSigListExtra[nIndOffset].strMSwTrim;
		pEntry->strMSwDisp  = m_sSigListExtra[nIndOffset].strMSwDisp;
		return true;
	}
}

void CDbSigs::SetEntryValid(unsigned nInd,bool bValid)
{
	// TODO: Bounds check
	ASSERT(nInd < m_nSigListExtraNum);
	m_sSigListExtra[nInd].bValid = bValid;
}


unsigned CDbSigs::GetIjgNum()
{
	return m_nSwIjgListNum;
}

LPTSTR CDbSigs::GetIjgEntry(unsigned nInd)
{
	return m_sSwIjgList[nInd];
}

// Update the directory used for user database
void CDbSigs::SetDbDir(CString strDbDir)
{
	m_strDbDir = strDbDir;
}


// Search exceptions for Make/Model in list of ones that don't have Makernotes
bool CDbSigs::LookupExcMmNoMkr(CString strMake,CString strModel)
{
	bool bFound = false;
	bool bDone = false;
	unsigned nInd = 0;
	if (strMake.GetLength() == 0) {
		bDone = true;
	}
	while (!bDone) {
		if (nInd >= m_nExcMmNoMkrListNum) {
			bDone = true;
		} else {
			// Perform exact match on Make, case sensitive
			// Check Make field and possibly Model field (if non-empty)
			if (_tcscmp(m_sExcMmNoMkrList[nInd].strXMake,strMake) != 0) {
				// Make did not match
			} else {
				// Make matched, now check to see if we need
				// to compare the Model string
				if (_tcslen(m_sExcMmNoMkrList[nInd].strXModel) == 0) {
					// No need to compare, we're bDone
					bFound = true;
					bDone = true;
				} else {
					// Need to check model as well
					// Since we may like to do a substring match, support wildcards

					// FnInd position of "*" if it exists in DB entry
					LPTSTR pWildcard;
					unsigned nCompareLen;
					pWildcard = _tcschr(m_sExcMmNoMkrList[nInd].strXModel,'*');
					if (pWildcard != NULL) {
						// Wildcard present
						nCompareLen = pWildcard - (m_sExcMmNoMkrList[nInd].strXModel);
					} else {
						// No wildcard, do full match
						nCompareLen = _tcslen(m_sExcMmNoMkrList[nInd].strXModel);
					}

					if (_tcsnccmp(m_sExcMmNoMkrList[nInd].strXModel,strModel,nCompareLen) != 0) {
						// No match
					} else {
						// Matched as well, we're bDone
						bFound = true;
						bDone = true;
					}
				}
			}

			nInd++;
		}
	}

	return bFound;
}

// Search exceptions for Make/Model in list of ones that are always edited
bool CDbSigs::LookupExcMmIsEdit(CString strMake,CString strModel)
{
	bool bFound = false;
	bool bDone = false;
	unsigned nInd = 0;
	if (strMake.GetLength() == 0) {
		bDone = true;
	}
	while (!bDone) {
		if (nInd >= m_nExcMmIsEditListNum) {
			bDone = true;
		} else {
			// Perform exact match, case sensitive
			// Check Make field and possibly Model field (if non-empty)
			if (_tcscmp(m_sExcMmIsEditList[nInd].strXMake,strMake) != 0) {
				// Make did not match
			} else {
				// Make matched, now check to see if we need
				// to compare the Model string
				if (_tcslen(m_sExcMmIsEditList[nInd].strXModel) == 0) {
					// No need to compare, we're bDone
					bFound = true;
					bDone = true;
				} else {
					// Need to check model as well
					if (_tcscmp(m_sExcMmIsEditList[nInd].strXModel,strModel) != 0) {
						// No match
					} else {
						// Matched as well, we're bDone
						bFound = true;
						bDone = true;
					}
				}
			}

			nInd++;
		}
	}
	
	return bFound;
}


// -----------------------------------------------------------------------
// Sample string indicator database
// -----------------------------------------------------------------------


// Sample list of software programs that also use the IJG encoder
LPTSTR CDbSigs::m_sSwIjgList[] = {
	_T("GIMP"),
	_T("IrfanView"),
	_T("idImager"),
	_T("FastStone Image Viewer"),
	_T("NeatImage"),
	_T("Paint.NET"),
	_T("Photomatix"),
	_T("XnView"),
	_T("*"),
};


// Sample list of software programs marked by EXIF.COMMENT field
//
// NOTE: Have not included the following indicators as they
//       also appear in some digicams in addition to software encoders.
//   "LEAD Technologies"
//   "U-Lead Systems"
//   "Intel(R) JPEG Library" (unsure if ever in hardware)
//
LPTSTR CDbSigs::m_sXComSwList[] = {
	_T("gd-jpeg"),
	_T("Photoshop"),
	_T("ACD Systems"),
	_T("AppleMark"),
	_T("PICResize"),
	_T("NeatImage"),
	_T("*"),
};


// Software signature list (m_sSigList) is located in "Signatures.inl"

