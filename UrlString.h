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

// ====================================================================================================
// SOURCE CODE ACKNOWLEDGEMENT
// ====================================================================================================
// The following code is derived from the following project on CodeGuru:
//
//		Title:		CUrlString: A very simple MFC class to Encode and Decode an url string
//		Author:		Stephane Erhardt
//		URL:		http://www.codeguru.com/cpp/cpp/cpp_mfc/article.php/c4029/URL-Encoding.htm (comments)
//		Date:		Mar 03, 2006
//
// ====================================================================================================

/*****************************************************************************
Module :     UrlString.h
Notices:     Written 2006 by Stephane Erhardt
Description: H URL Encoder/Decoder
*****************************************************************************/
#ifndef __CURLSTRING_H_
#define __CURLSTRING_H_

class CUrlString
{
private:
	CString m_csUnsafe;

public:
	CUrlString();
	virtual ~CUrlString() { };
	CString Encode(CString csDecoded);
	CString Decode(CString csEncoded);
};

#endif //__CURLSTRING_H_
