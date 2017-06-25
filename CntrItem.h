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

// CntrItem.h : interface of the CJPEGsnoopCntrItem class
//

#pragma once

class CJPEGsnoopDoc;
class CJPEGsnoopView;

class CJPEGsnoopCntrItem : public CRichEditCntrItem
{
	DECLARE_SERIAL(CJPEGsnoopCntrItem)

// Constructors
public:
	CJPEGsnoopCntrItem(REOBJECT* preo = NULL, CJPEGsnoopDoc* pContainer = NULL);
		// Note: pContainer is allowed to be NULL to enable IMPLEMENT_SERIALIZE
		//  IMPLEMENT_SERIALIZE requires the class have a constructor with
		//  zero arguments.  Normally, OLE items are constructed with a
		//  non-NULL document pointer

// Attributes
public:
	CJPEGsnoopDoc* GetDocument()
		{ return reinterpret_cast<CJPEGsnoopDoc*>(CRichEditCntrItem::GetDocument()); }
	CJPEGsnoopView* GetActiveView()
		{ return reinterpret_cast<CJPEGsnoopView*>(CRichEditCntrItem::GetActiveView()); }

	public:
	protected:

// Implementation
public:
	~CJPEGsnoopCntrItem();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
};

