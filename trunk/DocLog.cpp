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

#include "StdAfx.h"
#include ".\doclog.h"

#include "JPEGsnoopDoc.h"

CDocLog::CDocLog(CDocument* pDoc)
{
	m_pDoc = pDoc;
	m_bEn = true;
}

CDocLog::~CDocLog(void)
{
}

void CDocLog::AddLine(CString str) {
	if (m_bEn) {
		CJPEGsnoopDoc*	pSnoopDoc = (CJPEGsnoopDoc*)m_pDoc;
		pSnoopDoc->AddLine(str);
	}
};
void CDocLog::AddLineHdr(CString str) {
	if (m_bEn) {
		CJPEGsnoopDoc*	pSnoopDoc = (CJPEGsnoopDoc*)m_pDoc;
		pSnoopDoc->AddLineHdr(str);
	}
};
void CDocLog::AddLineHdrDesc(CString str) {
	if (m_bEn) {
		CJPEGsnoopDoc*	pSnoopDoc = (CJPEGsnoopDoc*)m_pDoc;
		pSnoopDoc->AddLineHdrDesc(str);
	}
};
void CDocLog::AddLineWarn(CString str) {
	if (m_bEn) {
		CJPEGsnoopDoc*	pSnoopDoc = (CJPEGsnoopDoc*)m_pDoc;
		pSnoopDoc->AddLineWarn(str);
	}
};
void CDocLog::AddLineErr(CString str) {
	if (m_bEn) {
		CJPEGsnoopDoc*	pSnoopDoc = (CJPEGsnoopDoc*)m_pDoc;
		pSnoopDoc->AddLineErr(str);
	}
};
void CDocLog::AddLineGood(CString str) {
	if (m_bEn) {
		CJPEGsnoopDoc*	pSnoopDoc = (CJPEGsnoopDoc*)m_pDoc;
		pSnoopDoc->AddLineGood(str);
	}
};

void CDocLog::Enable() {
	m_bEn = true;
};
void CDocLog::Disable() {
	m_bEn = false;
};

