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

#pragma once

#define DBEX_ENTRIES_MAX 300
#define DB_VER_STR "02"

struct CompExcMmNoMkr {
	char*		x_make;				//
	char*		x_model;			//
};

struct CompExcMmIsEdit {
	char*		x_make;				//
	char*		x_model;			//
};

// Signature structure for hardcoded table
struct CompSigConst {
	unsigned	m_editor;			// 0=unset, 1=digicam, 2=software/editor
	char*		x_make;				// Blank for editors (set to m_swdisp)
	char*		x_model;			// Blank for editors
	char*		um_qual;
	char*		c_sig;				// Signature
	char*		c_sigrot;			// Signature of rotated DQTs
	char*		x_subsamp;			// Blank for editors
	char*		m_swtrim;			// Blank for digicam
	char*		m_swdisp;			// Blank for digicam
};

// Signature structure for runtime table (can use CStrings)
struct CompSig {
	unsigned	flag;			// 0=? 1=OK 9=DELETE
	unsigned	m_editor;
	CString		x_make;			// Blank for editors
	CString		x_model;		// Blank for editors
	CString		um_qual;
	CString		c_sig;
	CString		c_sigrot;
	CString		x_subsamp;		// Blank for editors
	CString		m_swtrim;		// Blank for digicam
	CString		m_swdisp;		// Blank for digicam
};



class CDbSigs
{
public:
	CDbSigs();
	~CDbSigs();

	unsigned	GetNumSigsInternal();
	unsigned	GetNumSigsExtra();

	unsigned	GetDBNumEntries();
	bool		GetDBEntry(unsigned ind,CompSig* pEntry);
	unsigned	IsDBEntryUser(unsigned ind);
	void		DatabaseExtraClean();
	void		DatabaseExtraLoad();
	void		DatabaseExtraStore();
	void		DatabaseExtraMarkDelete(unsigned ind);

	unsigned	DatabaseExtraGetNum();
	CompSig		DatabaseExtraGet(unsigned ind);

	void		DatabaseExtraAdd(CString exif_make,CString exif_model,
							   CString qual,CString sig,CString sigrot,CString css,
							   unsigned user_source,CString user_software);

	bool		SearchSignatureExactInternal(CString make, CString model, CString sig);
	bool		SearchCom(CString strCom);

	bool		LookupExcMmNoMkr(CString strMake,CString strModel);
	bool		LookupExcMmIsEdit(CString strMake,CString strModel);

	unsigned	GetIjgNum();
	char*		GetIjgEntry(unsigned ind);

	void		SetDbDir(CString strDbDir);

private:
	CompSig						m_sSigListExtra[DBEX_ENTRIES_MAX];	// Extra entries
	unsigned					m_nSigListExtraNum;

	unsigned					m_nSigListNum;
	static const CompSigConst	m_sSigList[];			// Built-in entries

	unsigned						m_nExcMmNoMkrListNum;
	static const CompExcMmNoMkr		m_sExcMmNoMkrList[];

	unsigned						m_nExcMmIsEditListNum;
	static const CompExcMmIsEdit	m_sExcMmIsEditList[];
	
	unsigned					m_nSwIjgListNum;
	static char*				m_sSwIjgList[];
	unsigned					m_nSwLeadListNum;
	static char*				m_sSwLeadList[];

	unsigned					m_sXComSwListNum;
	static char*				m_sXComSwList[];

	CString						m_strDbDir;				// Database directory

};

