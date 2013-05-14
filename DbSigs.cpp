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

#include "stdafx.h"
#include "DbSigs.h"

#include "snoop.h"

#include "Signatures.inl"

// FIXME FIXME
// Want to convert this class to use CObArray for
// extra signatures. Otherwise, we are inefficient with
// memory and may run out of room.

CDbSigs::CDbSigs()
{
	// Count the built-in database
	bool done;

	done = false;
	m_nSigListNum = 0;
	while (!done)
	{
		if (!strcmp(m_sSigList[m_nSigListNum].x_make,"*"))
			done = true;
		else
			m_nSigListNum++;
	}

	// Count number of exceptions in Signatures.inl
	done = false;
	m_nExcMmNoMkrListNum = 0;
	while (!done)
	{
		if (!strcmp(m_sExcMmNoMkrList[m_nExcMmNoMkrListNum].x_make,"*"))
			done = true;
		else
			m_nExcMmNoMkrListNum++;
	}

	done = false;
	m_nExcMmIsEditListNum = 0;
	while (!done)
	{
		if (!strcmp(m_sExcMmIsEditList[m_nExcMmIsEditListNum].x_make,"*"))
			done = true;
		else
			m_nExcMmIsEditListNum++;
	}


	done = false;
	m_nSwIjgListNum = 0;
	while (!done) {
		if (!strcmp(m_sSwIjgList[m_nSwIjgListNum],"*"))
			done = true;
		else
			m_nSwIjgListNum++;
	}

	done = false;
	m_nSwLeadListNum = 0;
	while (!done) {
		if (!strcmp(m_sSwLeadList[m_nSwLeadListNum],"*"))
			done = true;
		else
			m_nSwLeadListNum++;
	}

	done = false;
	m_sXComSwListNum = 0;
	while (!done) {
		if (!strcmp(m_sXComSwList[m_sXComSwListNum],"*"))
			done = true;
		else
			m_sXComSwListNum++;
	}


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

unsigned CDbSigs::GetNumSigsInternal()
{
	return m_nSigListNum;
}

unsigned CDbSigs::GetNumSigsExtra()
{
	return m_nSigListExtraNum;
}



void CDbSigs::DatabaseExtraLoad()
{
	FILE * pDB;
	char ch;
	CString tmpStr;
	CString snoopStr;
	CString	verStr;
	CString secStr;
	bool valid = true;
	bool done = false;

	unsigned	load_num = 0;				// Number of entries to read
	CompSig		db_local_entry;				// Temp entry for load
	bool		db_local_entry_found;		// Temp entry already in built-in DB?
	bool		db_local_trimmed = false;	// Did we trim down the DB?

	m_nSigListExtraNum = 0;

	ASSERT(m_strDbDir != "");
	if (m_strDbDir == "") {
		return;
	}

	// Retrieve from environment user profile path.
	TCHAR szFilePathName[200];
	wsprintf(szFilePathName,"%s\\%s", m_strDbDir,DAT_FILE);

	pDB = fopen(szFilePathName,"rb");

	if (pDB) {

		// Read header
		snoopStr = "";
		while (ch=fgetc(pDB)) { snoopStr += ch; }
		if (snoopStr != "JPEGsnoop") { valid = false; }

		if (valid) {
			// Read version string
			verStr = "";
			while (ch=fgetc(pDB)) { verStr += ch; }

			valid = false;
			//if (verStr == "00") { valid = true; }
			if (verStr == "01") { valid = true; }
			if (verStr == "02") { valid = true; }
		}

		// Should consider trimming down local database if same entry
		// exists in built-in database (for example, user starts running
		// new version of tool).

		while (!done && valid) {

			// Read section header
			secStr = "";
			while (ch=fgetc(pDB)) { secStr += ch; }

			if (secStr == "*DB*") {
				// Read DB count
				fread(&load_num,sizeof(unsigned),1,pDB);

				// For each entry that we read, we should double-check to see
				// if we already have it in the built-in DB. If we do, then
				// don't add it to the runtime DB, and mark the local DB as
				// needing trimming. If so, rewrite the DB.

				for (unsigned ind=0;ind<load_num;ind++) {

					db_local_entry.x_make = "";
					db_local_entry.x_model = "";
					db_local_entry.um_qual = "";
					db_local_entry.c_sig = "";
					db_local_entry.c_sigrot = "";
					db_local_entry.x_subsamp = "";
					db_local_entry.m_swtrim = "";
					db_local_entry.m_swdisp = "";

					db_local_entry_found = false;

					// -------------------------------------------------------
					/*
					// For version 00:
					db_local_entry.x_make = "";
					while (ch=fgetc(pDB)) { db_local_entry.x_make += ch; }

					db_local_entry.x_model = "";
					while (ch=fgetc(pDB)) { db_local_entry.x_model += ch; }

					//fread(&m_sSigListExtra[ind].editor,sizeof(unsigned),1,pDB);
					tmpStr = "";
					while (ch=fgetc(pDB)) { tmpStr += ch; }
					db_local_entry.m_editor = atoi(tmpStr);

					db_local_entry.um_qual = "";
					while (ch=fgetc(pDB)) { db_local_entry.um_qual += ch; }

					db_local_entry.c_sig = "";
					while (ch=fgetc(pDB)) { db_local_entry.c_sig += ch; }

					
					// In older version of DB, these entries won't exist
					db_local_entry.x_subsamp = "";
					db_local_entry.m_swtrim = "";
					db_local_entry.m_swdisp = "";
					*/

					// -------------------------------------------------------


					if (verStr == "01") {
						// For version 01:

						while (ch=fgetc(pDB)) { db_local_entry.x_make += ch; }
						while (ch=fgetc(pDB)) { db_local_entry.x_model += ch; }

						tmpStr = "";
						while (ch=fgetc(pDB)) { tmpStr += ch; }
						db_local_entry.m_editor = atoi(tmpStr);

						while (ch=fgetc(pDB)) { db_local_entry.um_qual += ch; }
						while (ch=fgetc(pDB)) { db_local_entry.c_sig += ch; }

						// In older version of DB, these entries won't exist
						while (ch=fgetc(pDB)) { db_local_entry.x_subsamp += ch; }
						while (ch=fgetc(pDB)) { db_local_entry.m_swtrim += ch; }
						while (ch=fgetc(pDB)) { db_local_entry.m_swdisp += ch; }

					} else if (verStr == "02") {

						// For version 02:

						while (ch=fgetc(pDB)) { db_local_entry.x_make += ch; }
						while (ch=fgetc(pDB)) { db_local_entry.x_model += ch; }

						tmpStr = "";
						while (ch=fgetc(pDB)) { tmpStr += ch; }
						db_local_entry.m_editor = atoi(tmpStr);

						while (ch=fgetc(pDB)) { db_local_entry.um_qual += ch; }
						while (ch=fgetc(pDB)) { db_local_entry.c_sig += ch; }
						while (ch=fgetc(pDB)) { db_local_entry.c_sigrot += ch; }
						
						// In older version of DB, these entries won't exist
						while (ch=fgetc(pDB)) { db_local_entry.x_subsamp += ch; }
						while (ch=fgetc(pDB)) { db_local_entry.m_swtrim += ch; }
						while (ch=fgetc(pDB)) { db_local_entry.m_swdisp += ch; }
					}

					// -------------------------------------------------------


					// Does the entry already exist in the internal DB?
					db_local_entry_found = SearchSignatureExactInternal(db_local_entry.x_make,db_local_entry.x_model,db_local_entry.c_sig);

					if (!db_local_entry_found) {
						// Add it!
						m_sSigListExtra[m_nSigListExtraNum].flag      = 1; // OK
						m_sSigListExtra[m_nSigListExtraNum].x_make    = db_local_entry.x_make;
						m_sSigListExtra[m_nSigListExtraNum].x_model   = db_local_entry.x_model;
						m_sSigListExtra[m_nSigListExtraNum].m_editor  = db_local_entry.m_editor;
						m_sSigListExtra[m_nSigListExtraNum].um_qual   = db_local_entry.um_qual;
						m_sSigListExtra[m_nSigListExtraNum].c_sig     = db_local_entry.c_sig;
						m_sSigListExtra[m_nSigListExtraNum].c_sigrot  = db_local_entry.c_sigrot;

						m_sSigListExtra[m_nSigListExtraNum].x_subsamp = db_local_entry.x_subsamp;
						m_sSigListExtra[m_nSigListExtraNum].m_swtrim  = db_local_entry.m_swtrim;
						m_sSigListExtra[m_nSigListExtraNum].m_swdisp  = db_local_entry.m_swdisp;
						m_nSigListExtraNum++;
					} else {
						db_local_trimmed = true;
					}

				}
			} else if (secStr == "*Z*") {
				done = true;
			} else {
				valid = false;
			} // secStr

		} // while

		fclose(pDB);

	} // if pDB


	// If we did make changes to the database (trim), then rewrite it!
	if (db_local_trimmed) {
		DatabaseExtraStore();
	}

}


void CDbSigs::DatabaseExtraClean()
{
	m_nSigListExtraNum = 0;
	DatabaseExtraStore();
}

void CDbSigs::DatabaseExtraMarkDelete(unsigned ind)
{
	ASSERT(ind<m_nSigListExtraNum);
	m_sSigListExtra[ind].flag = 9;
}

unsigned CDbSigs::DatabaseExtraGetNum()
{
	return m_nSigListExtraNum;
}

CompSig CDbSigs::DatabaseExtraGet(unsigned ind)
{
	ASSERT(ind<m_nSigListExtraNum);
	return m_sSigListExtra[ind];
}

void CDbSigs::DatabaseExtraStore()
{
	FILE * pDB;

	ASSERT(m_strDbDir != "");
	if (m_strDbDir == "") {
		return;
	}

	// Retrieve from environment user profile path.
	TCHAR szFilePathName[300];
	wsprintf(szFilePathName,"%s\\%s", m_strDbDir,DAT_FILE);

	CString tmpStr;
	CString snoopStr;
	CString	emptyStr = "";
	pDB = fopen(szFilePathName,"wb+");

	ASSERT(pDB);
	if (pDB == NULL) {
		tmpStr.Format("ERROR: Unable to write database [%s]",szFilePathName);
		AfxMessageBox(tmpStr);
		return;
	}

	snoopStr = "JPEGsnoop";
	fwrite((LPCSTR)snoopStr,sizeof(char),strlen(snoopStr)+1,pDB);
	snoopStr = DB_VER_STR;
	fwrite((LPCSTR)snoopStr,sizeof(char),strlen(snoopStr)+1,pDB);
	snoopStr = "*DB*";
	fwrite((LPCSTR)snoopStr,sizeof(char),strlen(snoopStr)+1,pDB);

	// Determine how many entries will remain (after removing marked
	// deleted entries
	unsigned newExtraNum = m_nSigListExtraNum;
	for (unsigned ind=0;ind<m_nSigListExtraNum;ind++) {
		if (m_sSigListExtra[ind].flag == 9) {
			newExtraNum--;
		}
	}

//	fwrite(&m_nSigListExtraNum,sizeof(unsigned),1,pDB);
//	for (unsigned ind=0;ind<m_nSigListExtraNum;ind++) {

	fwrite(&newExtraNum,sizeof(unsigned),1,pDB);
	for (unsigned ind=0;ind<m_nSigListExtraNum;ind++) {
		/*
		fwrite((LPCSTR)m_sSigListExtra[ind].x_make,sizeof(char),strlen(m_sSigListExtra[ind].x_make)+1,pDB);
		fwrite((LPCSTR)m_sSigListExtra[ind].x_model,sizeof(char),strlen(m_sSigListExtra[ind].x_model)+1,pDB);
		tmpStr.Format("%u",m_sSigListExtra[ind].m_editor);
		fwrite((LPCSTR)tmpStr,sizeof(char),strlen(tmpStr)+1,pDB);
		fwrite((LPCSTR)m_sSigListExtra[ind].um_qual,sizeof(char),strlen(m_sSigListExtra[ind].um_qual)+1,pDB);
		fwrite((LPCSTR)m_sSigListExtra[ind].c_sig,sizeof(char),strlen(m_sSigListExtra[ind].c_sig)+1,pDB);
		*/

		// Is it marked for deletion (from Manage dialog?)
		if (m_sSigListExtra[ind].flag == 9) {
			continue;
		}

		// For version 01:
		fwrite((LPCSTR)m_sSigListExtra[ind].x_make,
			sizeof(char),strlen(m_sSigListExtra[ind].x_make)+1,pDB);
		fwrite((LPCSTR)m_sSigListExtra[ind].x_model,
			sizeof(char),strlen(m_sSigListExtra[ind].x_model)+1,pDB);
		tmpStr.Format("%u",m_sSigListExtra[ind].m_editor);				// m_editor = u_source
		fwrite((LPCSTR)tmpStr,sizeof(char),strlen(tmpStr)+1,pDB);
		fwrite((LPCSTR)m_sSigListExtra[ind].um_qual,
			sizeof(char),strlen(m_sSigListExtra[ind].um_qual)+1,pDB);
		fwrite((LPCSTR)m_sSigListExtra[ind].c_sig,
			sizeof(char),strlen(m_sSigListExtra[ind].c_sig)+1,pDB);
		fwrite((LPCSTR)m_sSigListExtra[ind].c_sigrot,
			sizeof(char),strlen(m_sSigListExtra[ind].c_sigrot)+1,pDB);
		fwrite((LPCSTR)m_sSigListExtra[ind].x_subsamp,
			sizeof(char),strlen(m_sSigListExtra[ind].x_subsamp)+1,pDB);
		fwrite((LPCSTR)m_sSigListExtra[ind].m_swtrim,					// m_swtrim = ""
			sizeof(char),strlen(m_sSigListExtra[ind].m_swtrim)+1,pDB);
		fwrite((LPCSTR)m_sSigListExtra[ind].m_swdisp,					// m_swdisp = u_sw
			sizeof(char),strlen(m_sSigListExtra[ind].m_swdisp)+1,pDB);

	}
	snoopStr = "*Z*";
	fwrite((LPCSTR)snoopStr,sizeof(char),strlen(snoopStr)+1,pDB);


	fclose(pDB);
}


//CAL! *** Need to decide if I want to include editors in this search...
bool CDbSigs::SearchSignatureExactInternal(CString make, CString model, CString sig)
{
	bool found_exact = false;
	bool done = false;
	unsigned ind = 0;
	while (!done) {
		if (ind >= m_nSigListNum) {
			done = true;
		} else {

			if ( (m_sSigList[ind].x_make  == make) &&
				(m_sSigList[ind].x_model == model) &&
				((m_sSigList[ind].c_sig  == sig) || (m_sSigList[ind].c_sigrot == sig)) )
			{
				found_exact = true;
				done = true;
			}
			ind++;
		}
	}

	return found_exact;
}

bool CDbSigs::SearchCom(CString strCom)
{
	bool found = false;
	bool done = false;
	unsigned ind = 0;
	if (strCom.GetLength() == 0) {
		done = true;
	}
	while (!done) {
		if (ind >= m_sXComSwListNum) {
			done = true;
		} else {
			if (strCom.Find(m_sXComSwList[ind]) != -1) {
				found = true;
				done = true;
			}
			ind++;
		}
	}
	return found;
}

// Returns total of built-in plus local DB
unsigned CDbSigs::GetDBNumEntries()
{
	return (m_nSigListNum + m_nSigListExtraNum);
}

// Returns total of built-in plus local DB
unsigned CDbSigs::IsDBEntryUser(unsigned ind)
{
	if (ind < m_nSigListNum) {
		return false;
	} else {
		return true;
	}
}

// Return a ptr to the struct containing the indexed entry
bool CDbSigs::GetDBEntry(unsigned ind,CompSig* pEntry)
{
	unsigned ind_offset;
	unsigned ind_max = GetDBNumEntries();
	ASSERT(pEntry);
	ASSERT(ind<ind_max);
	if (ind < m_nSigListNum) {
		pEntry->m_editor  = m_sSigList[ind].m_editor;
		pEntry->x_make    = m_sSigList[ind].x_make;
		pEntry->x_model   = m_sSigList[ind].x_model;
		pEntry->um_qual   = m_sSigList[ind].um_qual;
		pEntry->c_sig     = m_sSigList[ind].c_sig;
		pEntry->c_sigrot  = m_sSigList[ind].c_sigrot;
		pEntry->x_subsamp = m_sSigList[ind].x_subsamp;
		pEntry->m_swtrim  = m_sSigList[ind].m_swtrim;
		pEntry->m_swdisp  = m_sSigList[ind].m_swdisp;
		return true;
	} else {
		ind_offset = ind-m_nSigListNum;
		pEntry->m_editor  = m_sSigListExtra[ind_offset].m_editor;
		pEntry->x_make    = m_sSigListExtra[ind_offset].x_make;
		pEntry->x_model   = m_sSigListExtra[ind_offset].x_model;
		pEntry->um_qual   = m_sSigListExtra[ind_offset].um_qual;
		pEntry->c_sig     = m_sSigListExtra[ind_offset].c_sig;
		pEntry->c_sigrot  = m_sSigListExtra[ind_offset].c_sigrot;
		pEntry->x_subsamp = m_sSigListExtra[ind_offset].x_subsamp;
		pEntry->m_swtrim  = m_sSigListExtra[ind_offset].m_swtrim;
		pEntry->m_swdisp  = m_sSigListExtra[ind_offset].m_swdisp;
		return true;
	}
}

void CDbSigs::DatabaseExtraAdd(CString exif_make,CString exif_model,
							   CString qual,CString sig,CString sigrot,CString css,
							   unsigned user_source,CString user_software)
{
	ASSERT(m_nSigListExtraNum < DBEX_ENTRIES_MAX);
	if (m_nSigListExtraNum >= DBEX_ENTRIES_MAX) {
		CString tmpStr;
		tmpStr.Format("ERROR: Can only store maximum of %u extra signatures in local DB",DBEX_ENTRIES_MAX);
		AfxMessageBox(tmpStr);
		return;
	}

		// Now append it to the local database and resave
		m_sSigListExtra[m_nSigListExtraNum].x_make    = exif_make;
		m_sSigListExtra[m_nSigListExtraNum].x_model   = exif_model;
		m_sSigListExtra[m_nSigListExtraNum].um_qual   = qual;
		m_sSigListExtra[m_nSigListExtraNum].c_sig     = sig;
		m_sSigListExtra[m_nSigListExtraNum].c_sigrot  = sigrot;
		m_sSigListExtra[m_nSigListExtraNum].x_subsamp = css;

		if (user_source == ENUM_SOURCE_CAM) { // digicam
			m_sSigListExtra[m_nSigListExtraNum].m_editor = ENUM_EDITOR_CAM;
			m_sSigListExtra[m_nSigListExtraNum].m_swtrim = "";
			m_sSigListExtra[m_nSigListExtraNum].m_swdisp = "";
		} else if (user_source == ENUM_SOURCE_SW) { // software
			m_sSigListExtra[m_nSigListExtraNum].m_editor = ENUM_EDITOR_SW;
			m_sSigListExtra[m_nSigListExtraNum].m_swtrim = "";
			m_sSigListExtra[m_nSigListExtraNum].m_swdisp = user_software; // Not quite right perfect
			m_sSigListExtra[m_nSigListExtraNum].x_make   = "";
			m_sSigListExtra[m_nSigListExtraNum].x_model  = "";
		} else { // user didn't know
			m_sSigListExtra[m_nSigListExtraNum].m_editor = ENUM_EDITOR_UNSURE;
			m_sSigListExtra[m_nSigListExtraNum].m_swtrim = "";
			m_sSigListExtra[m_nSigListExtraNum].m_swdisp = user_software; // Not quite right perfect
		}

		m_nSigListExtraNum++;

		// Now resave the database
		DatabaseExtraStore();
}

unsigned CDbSigs::GetIjgNum()
{
	return m_nSwIjgListNum;
}

char* CDbSigs::GetIjgEntry(unsigned ind)
{
	return m_sSwIjgList[ind];
}

// Update the directory used for user database
void CDbSigs::SetDbDir(CString strDbDir)
{
	m_strDbDir = strDbDir;
}


// Search exceptions for Make/Model in list of ones that don't have Makernotes
bool CDbSigs::LookupExcMmNoMkr(CString strMake,CString strModel)
{
	bool found = false;
	bool done = false;
	unsigned ind = 0;
	if (strMake.GetLength() == 0) {
		done = true;
	}
	while (!done) {
		if (ind >= m_nExcMmNoMkrListNum) {
			done = true;
		} else {
			// Perform exact match on Make, case sensitive
			// Check Make field and possibly Model field (if non-empty)
			if (strcmp(m_sExcMmNoMkrList[ind].x_make,strMake) != 0) {
				// Make did not match
			} else {
				// Make matched, now check to see if we need
				// to compare the Model string
				if (strlen(m_sExcMmNoMkrList[ind].x_model) == 0) {
					// No need to compare, we're done
					found = true;
					done = true;
				} else {
					// Need to check model as well
					// Since we may like to do a substring match, support wildcards

					// Find position of "*" if it exists in DB entry
					char* pWildcard;
					unsigned nCompareLen;
					pWildcard = strchr(m_sExcMmNoMkrList[ind].x_model,'*');
					if (pWildcard != NULL) {
						// Wildcard present
						nCompareLen = pWildcard - (m_sExcMmNoMkrList[ind].x_model);
					} else {
						// No wildcard, do full match
						nCompareLen = strlen(m_sExcMmNoMkrList[ind].x_model);
					}

					if (strncmp(m_sExcMmNoMkrList[ind].x_model,strModel,nCompareLen) != 0) {
						// No match
					} else {
						// Matched as well, we're done
						found = true;
						done = true;
					}
				}
			}

			ind++;
		}
	}

	return found;
}

// Search exceptions for Make/Model in list of ones that are always edited
bool CDbSigs::LookupExcMmIsEdit(CString strMake,CString strModel)
{
	bool found = false;
	bool done = false;
	unsigned ind = 0;
	if (strMake.GetLength() == 0) {
		done = true;
	}
	while (!done) {
		if (ind >= m_nExcMmIsEditListNum) {
			done = true;
		} else {
			// Perform exact match, case sensitive
			// Check Make field and possibly Model field (if non-empty)
			if (strcmp(m_sExcMmIsEditList[ind].x_make,strMake) != 0) {
				// Make did not match
			} else {
				// Make matched, now check to see if we need
				// to compare the Model string
				if (strlen(m_sExcMmIsEditList[ind].x_model) == 0) {
					// No need to compare, we're done
					found = true;
					done = true;
				} else {
					// Need to check model as well
					if (strcmp(m_sExcMmIsEditList[ind].x_model,strModel) != 0) {
						// No match
					} else {
						// Matched as well, we're done
						found = true;
						done = true;
					}
				}
			}

			ind++;
		}
	}
	
	return found;
}


// -----------------------------------------------------------------------


char* CDbSigs::m_sSwIjgList[] = {
	"GIMP",
	"IrfanView",
	"idImager",
	"FastStone Image Viewer",
	"NeatImage",
	"Paint.NET",
	"Photomatix",
	"XnView",
	"*",
};

char* CDbSigs::m_sSwLeadList[] = {
	"IMatch",
	"*",
};

// Indicators of software located in EXIF.COMMENT field
// Note: I don't include:
//   "LEAD Technologies"
//   "U-Lead Systems"
//   "Intel(R) JPEG Library" (unsure if ever in hardware)
// as it appears in both software and some digicams!
//
// To strip out comment field from files do:
//   e:\new\_graphics\_jpeg\exiftool.exe -r -comment "e:\photos cal\jpeg analysis" 
//     | grep "Comment"
char* CDbSigs::m_sXComSwList[] = {
	"gd-jpeg",
	"Photoshop",
	"ACD Systems",
	"AppleMark",
	"PICResize",
	"NeatImage",
	"*",
};

//CAL! *** NOTE:
// Leica M8 has several entries with diff SW string (for firmware), and all have src=50
// What is the best solution?
// also Olympus E-400

// Actual signature list (m_sSigList) is in Signatures.inl



