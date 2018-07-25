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

#include <QDataStream>
#include <QDebug>
#include <QFile>

#include "DocLog.h"
#include "General.h"
//#include "JPEGsnoop.h"
#include "snoop.h"
#include "SnoopConfig.h"

#include "DbSigs.h"

#include "Signatures.inl"

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
CDbSigs::CDbSigs(CDocLog *pLog, CSnoopConfig *pAppConfig) : m_pDocLog(pLog), m_pAppConfig(pAppConfig)
{
	// Count the built-in database
	bool bDone;

	m_bFirstRun = false;

	bDone = false;
	m_nSigListNum = 0;

	while (!bDone)
	{
    if (m_sSigList[m_nSigListNum].strXMake == "*")
    {
      bDone = true;
    }
		else
    {
			m_nSigListNum++;
      m_qSigList.append(m_sSigList[m_nSigListNum]);
    }
	}

  QFile sigFile(m_pAppConfig->dbPath() + "Signatures.dat");
  sigFile.open(QIODevice::WriteOnly);
  QDataStream out(&sigFile);
//  out << m_qSigList;
  qDebug() << m_qSigList.count();

	// Count number of exceptions in Signatures.inl
	bDone = false;
	m_nExcMmNoMkrListNum = 0;

	while (!bDone)
	{
    if (m_sExcMmNoMkrList[m_nExcMmNoMkrListNum].strXMake == "*")
      bDone = true;
		else
			m_nExcMmNoMkrListNum++;
	}

	bDone = false;
	m_nExcMmIsEditListNum = 0;

	while (!bDone)
	{
    if (m_sExcMmIsEditList[m_nExcMmIsEditListNum].strXMake == "*")
      bDone = true;
		else
			m_nExcMmIsEditListNum++;
	}

	bDone = false;
	m_nSwIjgListNum = 0;

  // Sample list of software programs that also use the IJG encoder

  m_sSwIjgList
    << "GIMP"
    << "IrfanView"
    << "idImager"
    << "FastStone Image Viewer"
    << "NeatImage"
    << "Paint.NET"
    << "Photomatix"
    << "XnView";

  m_nSwIjgListNum = m_sSwIjgList.count();

	bDone = false;
	m_nXcomSwListNum = 0;

  // Sample list of software programs marked by EXIF.COMMENT field
  //
  // NOTE: Have not included the following indicators as they also appear in some digicams in addition to software encoders.
  //   "LEAD Technologies"
  //   "U-Lead Systems"
  //   "Intel(R) JPEG Library" (unsure if ever in hardware)
  //

  m_sXComSwList
    << "gd-jpeg"
    << "Photoshop"
    << "ACD Systems"
    << "AppleMark"
    << "PICResize"
    << "NeatImage";

  m_nXcomSwListNum = m_sXComSwList.count();

	// Reset extra database
	m_nSigListExtraNum = 0;

	// Default to user database dir not set yet
	// This will cause a fail if database load/store
	// functions are called before SetDbDir()
  m_strDbDir = "";
}

CDbSigs::~CDbSigs()
{
}

// Is this the first time running the application?
// If so, we skip certain warning messages (such as lack of existing user DB file)
void CDbSigs::SetFirstRun(bool bFirstRun)
{
	m_bFirstRun = bFirstRun;
}

int32_t CDbSigs::GetNumSigsInternal()
{
	return m_nSigListNum;
}

int32_t CDbSigs::GetNumSigsExtra()
{
	return m_nSigListExtraNum;
}

// Read an unsigned integer (4B) from the buffer
bool CDbSigs::BufReadNum(uint8_t *pBuf,uint32_t &nOut,uint32_t,uint32_t &nOffsetBytes)
{
  Q_ASSERT(pBuf);
	// TODO: check for buffer bounds
  nOut = static_cast<uint32_t>(pBuf[nOffsetBytes]);
  nOffsetBytes += sizeof(uint32_t);
	return true;
}

// Write an unsigned integer (4B) to the buffer
bool CDbSigs::BufWriteNum(uint8_t *pBuf,uint32_t nIn,uint32_t,uint32_t &nOffsetBytes)
{
  Q_ASSERT(pBuf);
	// TODO: check for buffer bounds
  uint8_t *	pBufBase;
  uint32_t*	pBufInt;
	pBufBase = &pBuf[nOffsetBytes];
  pBufInt = reinterpret_cast<uint32_t *>(pBufBase);
	pBufInt[0] = nIn;
  nOffsetBytes += sizeof(uint32_t);
	return true;
}

// Attempt to read a line from the buffer
// This is a replacement for CStdioFile::ReadLine()
// Both 16-bit unicode and 8-bit SBCS encoding modes are supported (via bUni)
// Offset parameter is incremented accordingly
// Supports both newline and NULL for string termination
bool CDbSigs::BufReadStr(uint8_t *pBuf,QString &strOut,uint32_t nMaxBytes,bool bUni,uint32_t &nOffsetBytes)
{
  Q_ASSERT(pBuf);
  QString		strOutTmp;
	bool		bDone;

	char		chAsc;
	wchar_t		chUni;
  uint32_t	nCharSz = ((bUni)?sizeof(wchar_t):sizeof(char));

	bDone = false;
  strOut = "";

  // Ensure we don't overrun the buffer by calculating the last byte index required for each iteration.
  for (uint32_t nInd=nOffsetBytes; (!bDone) && (nInd + nCharSz - 1 < nMaxBytes); nInd += nCharSz)
  {
    if (bUni)
    {
			chUni = pBuf[nInd];

      if ( (chUni != '\n') && (chUni != 0) )
      {
				strOut += chUni;
      }
      else
      {
				bDone = true;
				nOffsetBytes = nInd+nCharSz;
			}
    }
    else
    {
			chAsc = pBuf[nInd];

      if ( (chAsc != '\n') && (chAsc != 0) )
      {
				strOut += chAsc;
      }
      else
      {
				bDone = true;
				nOffsetBytes = nInd+nCharSz;
			}
		}
	}

  if (!bDone)
  {
		nOffsetBytes = nMaxBytes;
		// The input was not terminated, so we're still going to return what we got so far
		return false;
	}

	return true;
}

// Return true if we managed to write entire string including terminator without overrunning nMaxBytes
bool CDbSigs::BufWriteStr(unsigned char *pBuf,QString strIn,uint32_t nMaxBytes,bool bUni,uint32_t &nOffsetBytes)
{
  Q_ASSERT(pBuf);

	bool		bRet = false;
	char		chAsc;
	wchar_t		chUni;
  uint32_t	nCharSz = ((bUni)?sizeof(wchar_t):sizeof(char));
  unsigned char *pBufBase;
  wchar_t	*pBufUni;
  unsigned char		*pBufAsc;

	pBufBase = pBuf + nOffsetBytes;
  pBufUni = (wchar_t *)pBufBase;
  pBufAsc = pBufBase;

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

  uint32_t	nStrLen;
  uint32_t	nChInd;
  nStrLen = strIn.size();
/*
  for (nChInd=0;(nChInd<nStrLen)&&(nOffsetBytes+nCharSz-1<nMaxBytes);nChInd++)
  {
    if (bUni)
    {
			// Normal handling for unicode
      chUni = strIn[nChInd];
			pBufUni[nChInd] = chUni; 
    }
    else
    {
#ifdef UNICODE
			// To avoid Warning C4244: Conversion from 'wchar_t' to 'char' possible loss of data
			// We need to implement conversion here
			// Ref: http://stackoverflow.com/questions/4786292/converting-unicode-strings-and-vice-versa

      // Since we have compiled for unicode, the QString character fetch
			// will be unicode char. Therefore we need to use ANSI-converted form.
			chAsc = pszNonUnicode[nChInd];
#else
      // Since we have compiled for non-Unicode, the QString character fetch
			// will be single byte char
      chAsc = strIn[nChInd];
#endif
			pBufAsc[nChInd] = chAsc; 
		}

		// Advance pointers
		nOffsetBytes += nCharSz;
	}

	// Now terminate if we have space
  if ((nOffsetBytes + nCharSz-1) < nMaxBytes)
  {
    if (bUni)
    {
      chUni = wchar0;
			pBufUni[nChInd] = chUni;
    }
    else
    {
			chAsc = char(0);
			pBufAsc[nChInd] = chAsc;
		}

    // Advance pointers
		nOffsetBytes += nCharSz;

		// Since we managed to include terminator, return is successful
		bRet = true;
  } */

	// Return true if we finished the string write (without exceeding nMaxBytes)
	// or false otherwise
	return bRet;
}

void CDbSigs::DatabaseExtraLoad()
{
  uint8_t *	pBuf = NULL;
  uint32_t	nBufLenBytes = 0;
  uint32_t	nBufOffset = 0;
  QString		strError;

  Q_ASSERT(m_strDbDir != "");

  if (m_strDbDir == "")
  {
		return;
	}

	// Open the input file
  QFile pInFile(m_strDbDir + QString(DAT_FILE));

  if(pInFile.open(QIODevice::ReadOnly) == false)
	{
		// To avoid any user confusion, we disable this warning message if this
		// is the first time the program has been run (when we would expect that the user
		// database is not present). After first run, the user database will have been
		// created.
		// Take from SnoopConfig.bEulaAccepted
    if (!m_bFirstRun)
    {
      strError = QString("Couldn't find User Signature Database\n\n[%1]\n\nCreating default database").arg(pInFile.errorString());
      qDebug() << (strError);
//@@			AfxMessageBox(strError);
		}

		// Now create default database file in current DB location
		DatabaseExtraStore();

		return;
	}

	// Allocate the input buffer
  pBuf = new unsigned char[MAX_BUF_SET_FILE];
  Q_ASSERT(pBuf);

	// Load the buffer
  nBufLenBytes = pInFile.read((char *)pBuf,MAX_BUF_SET_FILE);

  Q_ASSERT(nBufLenBytes>0);
	// TODO: Error check for config file longer than max buffer!
  Q_ASSERT(nBufLenBytes<MAX_BUF_SET_FILE);
	nBufOffset = 0;

  QString		strLine;
  QString		strParam;
  QString		strVal;

  QString		strTmp;
  QString		strVer;
  QString		strSec;

	//bool		bErr = false;
	bool		bDone = false;
  bool		bRet;

  uint32_t	nBufOffsetTmp;
	bool		bFileOk = false;
	bool		bFileModeUni = false;

	bool		bValid;

  uint32_t	nNumLoad = 0;				// Number of entries to read
	CompSig		sDbLocalEntry;				// Temp entry for load
	bool		bDbLocalEntryFound;			// Temp entry already in built-in DB?
	bool		bDbLocalTrimmed = false;	// Did we trim down the DB?


	// Determine if config file is in Unicode or SBCS
	// If the file is in SBCS (legacy) then back it up
	// and then write-back in unicode mode.

	// Test unicode mode
	nBufOffsetTmp = 0;
	bRet = BufReadStr(pBuf,strLine,nBufLenBytes,true,nBufOffsetTmp);

  if (strLine == "JPEGsnoop")
  {
		bFileOk = true;
		bFileModeUni = true;
		nBufOffset = nBufOffsetTmp;
	}

	// Test SBCS mode
	nBufOffsetTmp = 0;
	bRet = BufReadStr(pBuf,strLine,nBufLenBytes,false,nBufOffsetTmp);

  if (strLine == "JPEGsnoop")
  {
		bFileOk = true;
		nBufOffset = nBufOffsetTmp;
	}

  if (!bFileOk)
  {
    strError = QString("WARNING: User Signature Database corrupt\n[%1]\nProceeding with defaults.").arg(m_strDbDir + QString(DAT_FILE));
    qDebug() << (strError);
//		AfxMessageBox(strError);

		// Close the file to ensure no sharing violation
    pInFile.close();

		// Copy old config file
    pInFile.copy(m_strDbDir + QString(DAT_FILE) + ".bak");
    // Now rewrite file in latest version
    DatabaseExtraStore();

		// Notify user
    strTmp = QString("Created default User Signature Database. Backup of old DB in [%1]").arg(m_strDbDir + QString(DAT_FILE) + ".bak");
//@@		AfxMessageBox(strTmp);
		return;
	}
	
	m_nSigListExtraNum = 0;

	// Version
	bRet = BufReadStr(pBuf,strVer,nBufLenBytes,bFileModeUni,nBufOffset);
	bValid = false;

  if (strVer == "00")
  {
    bValid = false;
  }

  if (strVer == "01")
  {
    bValid = true;
  }

  if (strVer == "02")
  {
    bValid = true;
  }

  if (strVer == "03")
  {
    bValid = true;
  }

	// Should consider trimming down local database if same entry
	// exists in built-in database (for example, user starts running
	// new version of tool).

  while (!bDone && bValid)
  {
		// Read section header

		bRet = BufReadStr(pBuf,strSec,nBufLenBytes,bFileModeUni,nBufOffset);

    if (strSec == "*DB*")
    {
			// Read DB count
			bRet = BufReadNum(pBuf,nNumLoad,nBufLenBytes,nBufOffset);

			// For each entry that we read, we should double-check to see
			// if we already have it in the built-in DB. If we do, then
			// don't add it to the runtime DB, and mark the local DB as
			// needing trimming. If so, rewrite the DB.

      for (uint32_t ind=0;ind<nNumLoad;ind++)
      {
				sDbLocalEntry.bValid = false;
        sDbLocalEntry.strXMake = "";
        sDbLocalEntry.strXModel = "";
        sDbLocalEntry.strUmQual = "";
        sDbLocalEntry.strCSig = "";
        sDbLocalEntry.strCSigRot = "";
        sDbLocalEntry.strXSubsamp = "";
        sDbLocalEntry.strMSwTrim = "";
        sDbLocalEntry.strMSwDisp = "";

				bDbLocalEntryFound = false;

        if (strVer == "01")
        {
					// For version 01:

					bRet = BufReadStr(pBuf,sDbLocalEntry.strXMake,nBufLenBytes,bFileModeUni,nBufOffset);
					bRet = BufReadStr(pBuf,sDbLocalEntry.strXModel,nBufLenBytes,bFileModeUni,nBufOffset);

          strTmp = "";
					bRet = BufReadStr(pBuf,strTmp,nBufLenBytes,bFileModeUni,nBufOffset);
//@@					sDbLocalEntry.eEditor = static_cast<teEditor>(_tstoi(strTmp));

					bRet = BufReadStr(pBuf,sDbLocalEntry.strUmQual,nBufLenBytes,bFileModeUni,nBufOffset);
					bRet = BufReadStr(pBuf,sDbLocalEntry.strCSig,nBufLenBytes,bFileModeUni,nBufOffset);

					// In older version of DB, these entries won't exist
					bRet = BufReadStr(pBuf,sDbLocalEntry.strXSubsamp,nBufLenBytes,bFileModeUni,nBufOffset);
					bRet = BufReadStr(pBuf,sDbLocalEntry.strMSwTrim,nBufLenBytes,bFileModeUni,nBufOffset);
					bRet = BufReadStr(pBuf,sDbLocalEntry.strMSwDisp,nBufLenBytes,bFileModeUni,nBufOffset);

        }
        else if ( (strVer == "02") || (strVer == "03") )
        {
					// For version 02 or 03:
					// NOTE: Difference between 02 and 03:
					//       - 02 : SBCS format
					//       - 03 : Unicode format
					bRet = BufReadStr(pBuf,sDbLocalEntry.strXMake,nBufLenBytes,bFileModeUni,nBufOffset);
					bRet = BufReadStr(pBuf,sDbLocalEntry.strXModel,nBufLenBytes,bFileModeUni,nBufOffset);

          strTmp = "";
					bRet = BufReadStr(pBuf,strTmp,nBufLenBytes,bFileModeUni,nBufOffset);
//@@					sDbLocalEntry.eEditor = static_cast<teEditor>(_tstoi(strTmp));

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

        if (!bDbLocalEntryFound)
        {
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
        }
        else
        {
					bDbLocalTrimmed = true;
				}
			}
    }
    else if (strSec == "*Z*")
    {
			bDone = true;
    }
    else
    {
			bValid = false;
		} // strSec
	} // while

	// ----------------------

  pInFile.close();

  if (pBuf)
  {
		delete [] pBuf;
		pBuf = NULL;
	}

	// If we did make changes to the database (trim), then rewrite it!
	// Ensure that we have closed the file above before we rewrite it
	// otherwise we could end up with a sharing violation
  if (bDbLocalTrimmed)
  {
		DatabaseExtraStore();
	}

	// Now is this an old config file version? If so,
	// create a backup and then rewrite the new version
//	if (bValid) {
//		if (strVer != DB_VER_STR) {
//			// Copy old config file
//			TCHAR szFilePathNameBak[200];
//      _stprintf_s(szFilePathNameBak,"%s\\%s.bak",m_strDbDir,DAT_FILE);
//			CopyFile(szFilePathName,szFilePathNameBak,false);
//			// Now rewrite file in latest version
//			DatabaseExtraStore();
//			// Notify user
//      strError = QString(("Upgraded User Signature Database. Backup in:\n%s",szFilePathNameBak);
//			AfxMessageBox(strError);
//		}
//	}
}

void CDbSigs::DatabaseExtraClean()
{
	m_nSigListExtraNum = 0;
	DatabaseExtraStore();
}

uint32_t CDbSigs::DatabaseExtraGetNum()
{
	return m_nSigListExtraNum;
}

CompSig CDbSigs::DatabaseExtraGet(uint32_t nInd)
{
  Q_ASSERT(nInd<m_nSigListExtraNum);
	return m_sSigListExtra[nInd];
}

void CDbSigs::DatabaseExtraStore()
{
  uint8_t *	pBuf = NULL;
  //uint32_t	nBufLenBytes = 0;
  uint32_t	nBufOffset = 0;

	bool		bModeUni = true;	// Save in Unicode format

  Q_ASSERT(m_strDbDir != "");
  if (m_strDbDir == "") {
		return;
	}

	// Open the output file
    QFile pOutFile(m_strDbDir + QString(DAT_FILE));

  if(pOutFile.open(QIODevice::WriteOnly) == false)
	{
    QString		strError;
    strError = QString("ERROR: Couldn't open file: [%s]").arg(pOutFile.errorString());
    qDebug() << (strError);
//@@		AfxMessageBox(strError);
		return;
	}

	// Allocate the output buffer
  pBuf = new unsigned char [MAX_BUF_SET_FILE];
  Q_ASSERT(pBuf);

	nBufOffset = 0;

  QString		strLine;
  QString		strParam;
  QString		strVal;
	//bool		bErr = false;
	//bool		bDone = false;
  bool		bRet;

  uint32_t	nMaxBufBytes = MAX_BUF_SET_FILE;

  bRet = BufWriteStr(pBuf,"JPEGsnoop",nMaxBufBytes,bModeUni,nBufOffset);
  bRet = BufWriteStr(pBuf,DB_VER_STR,nMaxBufBytes,bModeUni,nBufOffset);
  bRet = BufWriteStr(pBuf,"*DB*",nMaxBufBytes,bModeUni,nBufOffset);

  // Determine how many entries will remain (after removing marked deleted entries
  uint32_t nNewExtraNum = m_nSigListExtraNum;

  for (uint32_t nInd=0;nInd<m_nSigListExtraNum;nInd++)
  {
    if (!m_sSigListExtra[nInd].bValid)
    {
			nNewExtraNum--;
		}
	}

	bRet = BufWriteNum(pBuf,nNewExtraNum,nMaxBufBytes,nBufOffset);

  for (uint32_t nInd=0;nInd<m_nSigListExtraNum;nInd++) {

		// Is it marked for deletion (from Manage dialog?)
		if (!m_sSigListExtra[nInd].bValid) {
			continue;
		}

    QString	strTmp;

		// For version 02?

		bRet = BufWriteStr(pBuf,m_sSigListExtra[nInd].strXMake,nMaxBufBytes,bModeUni,nBufOffset);
		bRet = BufWriteStr(pBuf,m_sSigListExtra[nInd].strXModel,nMaxBufBytes,bModeUni,nBufOffset);
    strTmp = QString("%1").arg(m_sSigListExtra[nInd].eEditor);				// eEditor = u_source
		bRet = BufWriteStr(pBuf,strTmp,nMaxBufBytes,bModeUni,nBufOffset);
		bRet = BufWriteStr(pBuf,m_sSigListExtra[nInd].strUmQual,nMaxBufBytes,bModeUni,nBufOffset);
		bRet = BufWriteStr(pBuf,m_sSigListExtra[nInd].strCSig,nMaxBufBytes,bModeUni,nBufOffset);
		bRet = BufWriteStr(pBuf,m_sSigListExtra[nInd].strCSigRot,nMaxBufBytes,bModeUni,nBufOffset);
		bRet = BufWriteStr(pBuf,m_sSigListExtra[nInd].strXSubsamp,nMaxBufBytes,bModeUni,nBufOffset);
		bRet = BufWriteStr(pBuf,m_sSigListExtra[nInd].strMSwTrim,nMaxBufBytes,bModeUni,nBufOffset);
		bRet = BufWriteStr(pBuf,m_sSigListExtra[nInd].strMSwDisp,nMaxBufBytes,bModeUni,nBufOffset);

	}

  bRet = BufWriteStr(pBuf,"*Z*",nMaxBufBytes,bModeUni,nBufOffset);

	// --------------------------

	// Now write out the buffer
    pOutFile.write((char *)pBuf);

    pOutFile.close();

	if (pBuf) {
		delete [] pBuf;
		pBuf = NULL;
	}
	
}

void CDbSigs::DatabaseExtraAdd(QString strExifMake,QString strExifModel,
                 QString strQual,QString strSig,QString strSigRot,QString strCss,
                 teSource eUserSource,QString strUserSoftware)
{
  Q_ASSERT(m_nSigListExtraNum < DBEX_ENTRIES_MAX);

  if (m_nSigListExtraNum >= DBEX_ENTRIES_MAX)
  {
    QString strTmp;
    strTmp = QString("ERROR: Can only store maximum of %1 extra signatures in local DB").arg(DBEX_ENTRIES_MAX);
//@@		AfxMessageBox(strTmp);
		return;
	}

		// Now append it to the local database and resave
		m_sSigListExtra[m_nSigListExtraNum].strXMake    = strExifMake;
		m_sSigListExtra[m_nSigListExtraNum].strXModel   = strExifModel;
		m_sSigListExtra[m_nSigListExtraNum].strUmQual   = strQual;
		m_sSigListExtra[m_nSigListExtraNum].strCSig     = strSig;
		m_sSigListExtra[m_nSigListExtraNum].strCSigRot  = strSigRot;
		m_sSigListExtra[m_nSigListExtraNum].strXSubsamp = strCss;

    if (eUserSource == ENUM_SOURCE_CAM)
    { // digicam
			m_sSigListExtra[m_nSigListExtraNum].eEditor = ENUM_EDITOR_CAM;
      m_sSigListExtra[m_nSigListExtraNum].strMSwTrim = "";
      m_sSigListExtra[m_nSigListExtraNum].strMSwDisp = "";
    }
    else if (eUserSource == ENUM_SOURCE_SW)
    { // software
			m_sSigListExtra[m_nSigListExtraNum].eEditor = ENUM_EDITOR_SW;
      m_sSigListExtra[m_nSigListExtraNum].strMSwTrim = "";
			m_sSigListExtra[m_nSigListExtraNum].strMSwDisp = strUserSoftware; // Not quite right perfect
      m_sSigListExtra[m_nSigListExtraNum].strXMake   = "";
      m_sSigListExtra[m_nSigListExtraNum].strXModel  = "";
    }
    else
    { // user didn't know
			m_sSigListExtra[m_nSigListExtraNum].eEditor = ENUM_EDITOR_UNSURE;
      m_sSigListExtra[m_nSigListExtraNum].strMSwTrim = "";
			m_sSigListExtra[m_nSigListExtraNum].strMSwDisp = strUserSoftware; // Not quite right perfect
		}

		m_nSigListExtraNum++;

		// Now resave the database
		DatabaseExtraStore();
}

// TODO: Should we include editors in this search?
bool CDbSigs::SearchSignatureExactInternal(QString strMake, QString strModel, QString strSig)
{
	bool		bFoundExact = false;
	bool		bDone = false;
  int32_t	nInd = 0;

  while (!bDone)
  {
    if (nInd >= m_nSigListNum)
    {
			bDone = true;
    }
    else
    {
      if((m_sSigList[nInd].strXMake  == strMake) &&
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

bool CDbSigs::SearchCom(QString strCom)
{
	bool bFound = false;

  if (strCom.size() > 0)
  {
    for(int i = 0; i < m_sXComSwList.size(); i++)
    {
      if(m_sXComSwList.at(i).contains(strCom))
      {
        bFound = true;
        break;
      }
    }
  }

	return bFound;
}

// Returns total of built-in plus local DB
int32_t CDbSigs::GetDBNumEntries()
{
	return (m_nSigListNum + m_nSigListExtraNum);
}

// Returns total of built-in plus local DB
uint32_t CDbSigs::IsDBEntryUser(uint32_t nInd)
{
  if (nInd < m_nSigListNum)
  {
		return false;
  }
  else
  {
		return true;
	}
}

// Return a ptr to the struct containing the indexed entry
bool CDbSigs::GetDBEntry(int32_t nInd,CompSig* pEntry)
{
  uint32_t nIndOffset;
  int32_t nIndMax = GetDBNumEntries();
  Q_ASSERT(pEntry);
  Q_ASSERT(nInd<nIndMax);

  if (nInd < m_nSigListNum)
  {
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
  }
  else
  {
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

void CDbSigs::SetEntryValid(uint32_t nInd,bool bValid)
{
	// TODO: Bounds check
  Q_ASSERT(nInd < m_nSigListExtraNum);
	m_sSigListExtra[nInd].bValid = bValid;
}


uint32_t CDbSigs::GetIjgNum()
{
	return m_nSwIjgListNum;
}

QString CDbSigs::GetIjgEntry(uint32_t nInd)
{
	return m_sSwIjgList[nInd];
}

// Update the directory used for user database
void CDbSigs::SetDbDir(QString strDbDir)
{
	m_strDbDir = strDbDir;
}


// Search exceptions for Make/Model in list of ones that don't have Makernotes
bool CDbSigs::LookupExcMmNoMkr(QString strMake,QString strModel)
{
	bool bFound = false;
	bool bDone = false;
  uint32_t nInd = 0;

  if (strMake.size() == 0)
  {
		bDone = true;
	}

  while (!bDone)
  {
    if (nInd >= m_nExcMmNoMkrListNum)
    {
			bDone = true;
    }
    else
    {
			// Perform exact match on Make, case sensitive
			// Check Make field and possibly Model field (if non-empty)
      if (m_sExcMmNoMkrList[nInd].strXMake == strMake)
      {
        // Make matched, now check to see if we need to compare the Model string
        if (m_sExcMmNoMkrList[nInd].strXModel != strModel)
        {
					// No need to compare, we're bDone
					bFound = true;
					bDone = true;
        }
        else
        {
          // Need to check model as well. Since we may like to do a substring match, support wildcards

          // Find position of "*" if it exists in DB entry
          int pWildcard;
          uint32_t nCompareLen;
          pWildcard = m_sExcMmNoMkrList[nInd].strXModel.indexOf(QChar('*'));

          if (pWildcard != -1)
          {
						// Wildcard present
            nCompareLen = m_sExcMmNoMkrList[nInd].strXModel.size() - pWildcard;  //!! - 1
          }
          else
          {
						// No wildcard, do full match
            nCompareLen = m_sExcMmNoMkrList[nInd].strXModel.size();
					}

          if (m_sExcMmNoMkrList[nInd].strXModel.left(nCompareLen) == strModel)
          {
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
bool CDbSigs::LookupExcMmIsEdit(QString strMake,QString strModel)
{
	bool bFound = false;
	bool bDone = false;
  uint32_t nInd = 0;

  if (strMake.size() == 0)
  {
		bDone = true;
	}

  while (!bDone)
  {
    if (nInd >= m_nExcMmIsEditListNum)
    {
			bDone = true;
    }
    else
    {
			// Perform exact match, case sensitive
			// Check Make field and possibly Model field (if non-empty)
      if (m_sExcMmIsEditList[nInd].strXMake == strMake)
      {
				// Make matched, now check to see if we need
				// to compare the Model string
        if (m_sExcMmIsEditList[nInd].strXModel.size() == 0)
        {
					// No need to compare, we're bDone
					bFound = true;
					bDone = true;
        }
        else
        {
					// Need to check model as well
          if (m_sExcMmIsEditList[nInd].strXModel == strModel)
          {
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

// Software signature list (m_sSigList) is located in "Signatures.inl"

