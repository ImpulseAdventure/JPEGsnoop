// JPEGsnoop - JPEG Image Decoder & Analysis Utility
// Copyright (C) 2018 - Calvin Hass
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

#include <QString>

#define DBEX_ENTRIES_MAX 300
#define DB_VER_STR "03"

#include "snoop.h"

// Signature exception structure with metadata fields
struct CompExcMm
{
  QString strXMake;             // EXIF Make
  QString strXModel;            // EXIF Model
};

// Signature structure for hardcoded table
struct CompSigConst
{
  teEditor eEditor;             // Digicam vs software/editor
  QString strXMake;             // Blank for editors (set to strMSwDisp)
  QString strXModel;            // Blank for editors
  QString strUmQual;
  QString strCSig;              // Signature
  QString strCSigRot;           // Signature of rotated DQTs
  QString strXSubsamp;          // Blank for editors
  QString strMSwTrim;           // Blank for digicam
  QString strMSwDisp;           // Blank for digicam
};

// Signature structure for runtime table (can use QStrings)
struct CompSig
{
  bool bValid;                  // Set to FALSE for removal
  teEditor eEditor;
  QString strXMake;             // Blank for editors
  QString strXModel;            // Blank for editors
  QString strUmQual;
  QString strCSig;
  QString strCSigRot;
  QString strXSubsamp;          // Blank for editors
  QString strMSwTrim;           // Blank for digicam
  QString strMSwDisp;           // Blank for digicam
};

class CDbSigs
{
public:
  CDbSigs();
  ~CDbSigs();

  unsigned GetNumSigsInternal();
  unsigned GetNumSigsExtra();

  unsigned GetDBNumEntries();
  bool GetDBEntry(unsigned nInd, CompSig * pEntry);
  unsigned IsDBEntryUser(unsigned nInd);

  void SetEntryValid(unsigned nInd, bool bValid);

  void DatabaseExtraClean();
  void DatabaseExtraLoad();
  void DatabaseExtraStore();

  unsigned DatabaseExtraGetNum();
  CompSig DatabaseExtraGet(unsigned nInd);

  void DatabaseExtraAdd(QString strExifMake, QString strExifModel,
                        QString strQual, QString strSig, QString strSigRot, QString strCss,
                        teSource eUserSource, QString strUserSoftware);

  bool BufReadNum(quint8 * pBuf, unsigned &nOut, unsigned nMaxBytes, unsigned &nOffsetBytes);
  bool BufReadStr(quint8 * pBuf, QString & strOut, unsigned nMaxBytes, bool bUni, unsigned &nOffsetBytes);
  bool BufWriteNum(quint8 * pBuf, unsigned nIn, unsigned nMaxBytes, unsigned &nOffsetBytes);
  bool BufWriteStr(quint8 * pBuf, QString strIn, unsigned nMaxBytes, bool bUni, unsigned &nOffsetBytes);

  bool SearchSignatureExactInternal(QString strMake, QString strModel, QString strSig);
  bool SearchCom(QString strCom);

  bool LookupExcMmNoMkr(QString strMake, QString strModel);
  bool LookupExcMmIsEdit(QString strMake, QString strModel);

  unsigned GetIjgNum();
  QString GetIjgEntry(unsigned nInd);

  void SetDbDir(QString strDbDir);
  void SetFirstRun(bool bFirstRun);

private:
  CompSig m_sSigListExtra[DBEX_ENTRIES_MAX];  // Extra entries
  unsigned m_nSigListExtraNum;

  unsigned m_nSigListNum;
  static const CompSigConst m_sSigList[];       // Built-in entries

  unsigned m_nExcMmNoMkrListNum;
  static const CompExcMm m_sExcMmNoMkrList[];

  unsigned m_nExcMmIsEditListNum;
  static const CompExcMm m_sExcMmIsEditList[];

  unsigned m_nSwIjgListNum;


  unsigned m_nXcomSwListNum;

  QString m_strDbDir;           // Database directory
  QStringList m_sXComSwList;
  QStringList m_sSwIjgList;

  bool m_bFirstRun;             // First time running app?
};
