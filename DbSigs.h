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

// ==========================================================================
// CLASS DESCRIPTION:
// - Class provides management of the signatures database
// - Supports both built-in and user database entries
//
// ==========================================================================


#pragma once

#define DBEX_ENTRIES_MAX 300
#define DB_VER_STR "03"

#include "snoop.h"

// Signature exception structure with metadata fields
struct CompExcMm {
	LPTSTR		strXMake;			// EXIF Make
	LPTSTR		strXModel;			// EXIF Model
};


// Signature structure for hardcoded table
struct CompSigConst {
	teEditor	eEditor;			// Digicam vs software/editor
	LPTSTR		strXMake;			// Blank for editors (set to strMSwDisp)
	LPTSTR		strXModel;			// Blank for editors
	LPTSTR		strUmQual;
	LPTSTR		strCSig;			// Signature
	LPTSTR		strCSigRot;			// Signature of rotated DQTs
	LPTSTR		strXSubsamp;		// Blank for editors
	LPTSTR		strMSwTrim;			// Blank for digicam
	LPTSTR		strMSwDisp;			// Blank for digicam
};


// Signature structure for runtime table (can use CStrings)
struct CompSig {
	bool		bValid;				// Set to FALSE for removal
	teEditor	eEditor;
	CString		strXMake;			// Blank for editors
	CString		strXModel;			// Blank for editors
	CString		strUmQual;
	CString		strCSig;
	CString		strCSigRot;
	CString		strXSubsamp;		// Blank for editors
	CString		strMSwTrim;			// Blank for digicam
	CString		strMSwDisp;			// Blank for digicam
};



class CDbSigs
{
public:
	CDbSigs();
	~CDbSigs();

	unsigned	GetNumSigsInternal();
	unsigned	GetNumSigsExtra();

	unsigned	GetDBNumEntries();
	bool		GetDBEntry(unsigned nInd,CompSig* pEntry);
	unsigned	IsDBEntryUser(unsigned nInd);

	void		SetEntryValid(unsigned nInd,bool bValid);

	void		DatabaseExtraClean();
	void		DatabaseExtraLoad();
	void		DatabaseExtraStore();

	unsigned	DatabaseExtraGetNum();
	CompSig		DatabaseExtraGet(unsigned nInd);

	void		DatabaseExtraAdd(CString strExifMake,CString strExifModel,
							   CString strQual,CString strSig,CString strSigRot,CString strCss,
							   teSource eUserSource,CString strUserSoftware);

	bool		BufReadNum(PBYTE pBuf,unsigned &nOut,unsigned nMaxBytes,unsigned &nOffsetBytes);
	bool		BufReadStr(PBYTE pBuf,CString &strOut,unsigned nMaxBytes,bool bUni,unsigned &nOffsetBytes);
	bool		BufWriteNum(PBYTE pBuf,unsigned nIn,unsigned nMaxBytes,unsigned &nOffsetBytes);
	bool		BufWriteStr(PBYTE pBuf,CString strIn,unsigned nMaxBytes,bool bUni,unsigned &nOffsetBytes);

	bool		SearchSignatureExactInternal(CString strMake, CString strModel, CString strSig);
	bool		SearchCom(CString strCom);

	bool		LookupExcMmNoMkr(CString strMake,CString strModel);
	bool		LookupExcMmIsEdit(CString strMake,CString strModel);

	unsigned	GetIjgNum();
	LPTSTR		GetIjgEntry(unsigned nInd);

	void		SetDbDir(CString strDbDir);
	void		SetFirstRun(bool bFirstRun);

private:
	CompSig						m_sSigListExtra[DBEX_ENTRIES_MAX];	// Extra entries
	unsigned					m_nSigListExtraNum;

	unsigned					m_nSigListNum;
	static const CompSigConst	m_sSigList[];			// Built-in entries

	unsigned					m_nExcMmNoMkrListNum;
	static const CompExcMm		m_sExcMmNoMkrList[];

	unsigned					m_nExcMmIsEditListNum;
	static const CompExcMm		m_sExcMmIsEditList[];
	
	unsigned					m_nSwIjgListNum;
	static LPTSTR				m_sSwIjgList[];

	unsigned					m_nXcomSwListNum;
	static LPTSTR				m_sXComSwList[];

	CString						m_strDbDir;				// Database directory

	bool						m_bFirstRun;			// First time running app?

};

