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


// The following code was from UrlString by Stephane Erhardt:
// http://www.codeguru.com/cpp/cpp/cpp_mfc/comments.php/c4029/?thread=9533


/*****************************************************************************
Module :     UrlString.cpp
Notices:     Written 2006 by Stephane Erhardt
Description: CPP URL Encoder/Decoder
*****************************************************************************/
#include "stdafx.h"
#include "UrlString.h"

/*****************************************************************************/
CUrlString::CUrlString()
{
	m_csUnsafe = _T("%=\"<>\\^[]`+$,@:;/!#?&'");
	for(int iChar = 1; iChar < 33; iChar++)
		m_csUnsafe += (char)iChar;
	for(int iChar = 124; iChar < 256; iChar++)
		m_csUnsafe += (char)iChar;
}

/*****************************************************************************/
CString CUrlString::Encode(CString csDecoded)
{
	CString csCharEncoded, csCharDecoded;
	CString csEncoded = csDecoded;

	for(int iPos = 0; iPos < m_csUnsafe.GetLength(); iPos++)
	{
		csCharEncoded.Format(_T("%%%02X"), m_csUnsafe[iPos]);
		csCharDecoded = m_csUnsafe[iPos];
		csEncoded.Replace(csCharDecoded, csCharEncoded);
	}
	return csEncoded;
}

/*****************************************************************************/
CString CUrlString::Decode(CString csEncoded)
{
	CString csUnsafeEncoded = Encode(m_csUnsafe);
	CString csDecoded = csEncoded;
	CString csCharEncoded, csCharDecoded;

	for(int iPos = 0; iPos < csUnsafeEncoded.GetLength(); iPos += 3)
	{
		csCharEncoded = csUnsafeEncoded.Mid(iPos, 3);
		csCharDecoded = (char)strtol(csUnsafeEncoded.Mid(iPos + 1, 2), NULL, 16);
		csDecoded.Replace(csCharEncoded, csCharDecoded);
	}
	return csDecoded;
}