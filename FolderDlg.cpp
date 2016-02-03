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


// ====================================================================================================
// SOURCE CODE ACKNOWLEDGEMENT
// ====================================================================================================
// The following code is based on an example CFolderDialog class that appears in MSDN:
//
//		Title:		CFolderDialog (C++ at Work: Counting MDI Children, Browsing for Folders)
//		Authors:	Paul DiLascia
//		URL:		http://msdn.microsoft.com/en-us/magazine/cc163789.aspx
//		Date:		Jun 2005
// ====================================================================================================



//////////////////////////////////////////////////////////////////
// MSDN Magazine -- June 2005
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
// Compiles with Visual Studio .NET 2003 (V7.1) on Windows XP. Tab size=3.
//
#include "stdafx.h"
#include "FolderDlg.h"
#include <shlwapi.h>

// You must link shlwapi.lib for StrRetToBuf
#pragma comment(lib, "shlwapi.lib")

//CAL! Commented out the following to avoid issues with DEBUG_INTERFACE_NAME
#if 0 //CAL!
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BOOL CFolderDialog::bTRACE=0; // controls tracing

//////////////////
// For deugging: names of interfaces easier to read than GUIDs!
//
DEBUG_BEGIN_INTERFACE_NAMES()
	DEBUG_INTERFACE_NAME(IFolderFilterSite)
	DEBUG_INTERFACE_NAME(IFolderFilter)
DEBUG_END_INTERFACE_NAMES();

#endif

IMPLEMENT_DYNAMIC(CFolderDialog, CCmdTarget);

//////////////////
// ctor: initialize most stuff to NULL
//
CFolderDialog::CFolderDialog(CWnd* pWnd)
{

	//CAL! Modified the following to allow us to call this
	//     routine without a parent window defined. This
	//     is a temporary workaround so that I can call
	//     CFolderDialog() from the JPEGsnoopDoc class.

//CAL!	ASSERT(pWnd);
	memset(&m_brinfo,0,sizeof(m_brinfo));
//CAL!	m_brinfo.hwndOwner=pWnd->m_hWnd;		 // use parent window
	m_bFilter = FALSE;						 // default: no filtering
	SHGetDesktopFolder(&m_shfRoot);		 // get root IShellFolder

	//CAL! Add support for start path
	m_strStartPath = "";
}

//////////////////
// dtor: detach browser window before it's destroyed!
//
CFolderDialog::~CFolderDialog()
{
}

//////////////////
// Browse for folder. Args are same as for SHBrowseForFolder, but with extra
// bFilter that tells whether to do custom filtering. Note this requires
// BIF_NEWDIALOGSTYLE, which is inconsistent with some other flags--be
// careful!
//
LPCITEMIDLIST CFolderDialog::BrowseForFolder(LPCTSTR title, UINT flags,
	LPCITEMIDLIST root, BOOL bFilter)
{
	//BFTRACE(_T("CFolderDialog::BrowseForFolder\n"));
	TCHAR* buf = m_sDisplayName.GetBuffer(MAX_PATH);
	m_brinfo.pidlRoot = root;
	m_brinfo.pszDisplayName = buf;
	m_brinfo.lpszTitle = title;
	m_brinfo.ulFlags = flags;
	m_brinfo.lpfn = CallbackProc;
	m_brinfo.lParam = (LPARAM)this;

	// filtering only supported for new-style dialogs
	m_bFilter = bFilter;
	ASSERT(!bFilter||(m_brinfo.ulFlags & BIF_NEWDIALOGSTYLE));

	LPCITEMIDLIST pidl = SHBrowseForFolder(&m_brinfo); // do it
	m_sDisplayName.ReleaseBuffer();

	return pidl;
}

//////////////////
// Handy function to get the string pathname from pidl.
//
CString CFolderDialog::GetPathName(LPCITEMIDLIST pidl)
{
	CString path;
	TCHAR* buf = path.GetBuffer(MAX_PATH);
	SHGetPathFromIDList(pidl, buf);
	path.ReleaseBuffer();
	return path;
}

//CAL! Added
void CFolderDialog::SetStartPath(CString strPath)
{
	m_strStartPath = strPath;
}

//////////////////
// Handy function to get the display name from pidl.
//
CString CFolderDialog::GetDisplayNameOf(IShellFolder* psf, LPCITEMIDLIST pidl,
	DWORD uFlags)
{
	CString dn;
	STRRET strret;								 // special struct for GetDisplayNameOf
	strret.uType = STRRET_CSTR;			 // get as CSTR
	if (SUCCEEDED(psf->GetDisplayNameOf(pidl, uFlags, &strret))) {
		StrRetToBuf(&strret, pidl, dn.GetBuffer(MAX_PATH), MAX_PATH);
		dn.ReleaseBuffer();
	}
	return dn;
}

//////////////////
// Free PIDL using shell's IMalloc
//
void CFolderDialog::FreePIDL(LPCITEMIDLIST pidl)
{
	CComQIPtr<IMalloc> iMalloc;	// shell's IMalloc
	HRESULT hr = SHGetMalloc(&iMalloc);
	ASSERT(SUCCEEDED(hr));
	iMalloc->Free((void*)pidl);
}

//////////////////
// Internal callback proc used for SHBrowseForFolder passes control to
// appropriate virtual function after attaching browser window.
//
int CALLBACK CFolderDialog::CallbackProc(HWND hwnd,
	UINT msg, LPARAM lp, LPARAM lpData)
{
	CFolderDialog* pDlg = (CFolderDialog*)lpData;
	ASSERT(pDlg);
	if (pDlg->m_hWnd!=hwnd) {
		if (pDlg->m_hWnd)
			pDlg->UnsubclassWindow();
		pDlg->SubclassWindow(hwnd);
	}
	return pDlg->OnMessage(msg, lp);
}

//////////////////
// Handle notification from browser window: parse args and pass to specific
// virtual handler function.
//
int CFolderDialog::OnMessage(UINT msg, LPARAM lp)
{
	switch (msg) {
	case BFFM_INITIALIZED:
		OnInitialized();
		//CAL!
		// Add support for initial start directory
		if (!m_strStartPath.IsEmpty()) {
			LPARAM	lpStartPath;
			lpStartPath = (LPARAM)(m_strStartPath.GetBuffer(1));
			m_strStartPath.ReleaseBuffer();
			::SendMessage(this->m_hWnd,BFFM_SETSELECTION,TRUE,lpStartPath);
		}
		//CAL!
		return 0;
	case BFFM_IUNKNOWN:
		OnIUnknown((IUnknown*)lp);
		return 0;
	case BFFM_SELCHANGED:
		OnSelChanged((LPCITEMIDLIST)lp);
		return 0;
	case BFFM_VALIDATEFAILED:
		return OnValidateFailed((LPCTSTR)lp);
	default:
		TRACE(_T("***Warning: unknown message %d in CFolderDialog::OnMessage\n"));
	}
	return 0;
}

/////////////////
// Browser window initialized.
//
void CFolderDialog::OnInitialized()
{
	//BFTRACE(_T("CFolderDialog::OnInitialized\n"));
}

/////////////////
// Browser is notifying me with its IUnknown: use it to set filter if
// requested. Note this can be called with punk=NULL when shutting down!
//
void CFolderDialog::OnIUnknown(IUnknown* punk)
{
	//BFTRACE(_T("CFolderDialog::OnIUnknown: %p\n"), punk);
	if (punk && m_bFilter) {
		CComQIPtr<IFolderFilterSite> iffs;
		VERIFY(SUCCEEDED(punk->QueryInterface(IID_IFolderFilterSite, (void**)&iffs)));
		iffs->SetFilter((IFolderFilter*)&m_xFolderFilter);
		// smart pointer automatically Releases iffs,
		// no longer needed once you call SetFilter
	}
}

//////////////////
// User selected a different folder.
//
void CFolderDialog::OnSelChanged(LPCITEMIDLIST pidl)
{
	//BFTRACE(_T("CFolderDialog::OnSelChanged: %s\n"),
	//	GetDisplayNameOf(m_shfRoot, pidl, SHGDN_FORPARSING));
}

//////////////////
// User attempted to enter a name in the edit box that isn't a folder.
//
BOOL CFolderDialog::OnValidateFailed(LPCTSTR lpsz)
{
	//BFTRACE(_T("CFolderDialog::OnValidateFailed: %s\n"), lpsz);
	return TRUE; // don't close dialog.
}

//////////////////
// Used for custom filtering. You must override to specify filter flags.
//
HRESULT CFolderDialog::OnGetEnumFlags(
	IShellFolder* psf,			// this folder's IShellFolder
	LPCITEMIDLIST pidlFolder,	// folder's PIDL
	DWORD *pgrfFlags)				// [out] return flags you want to allow
{
	//BFTRACE(_T("CFolderDialog::OnGetEnumFlags(%p): %s\n"),
	//	psf, GetPathName(pidlFolder));
	return S_OK;
}

//////////////////
// Used for custom filtering. You must override to filter items.
//
HRESULT CFolderDialog::OnShouldShow(
	IShellFolder* psf,			// This folder's IShellFolder
	LPCITEMIDLIST pidlFolder,	// PIDL for folder containing item
	LPCITEMIDLIST pidlItem)		// PIDL for item
{
	//BFTRACE(_T("CFolderDialog::OnShouldShow(%p): %s: %s\n"), psf,
	//	GetDisplayNameOf(psf,pidlFolder,SHGDN_NORMAL),
	//	GetDisplayNameOf(psf,pidlItem,SHGDN_NORMAL));
	return S_OK;
}

//////////////// Standard MFC IUnknown -- nested classes call these ////////////////

STDMETHODIMP_(ULONG) CFolderDialog::AddRef()
{
	//BFTRACE(_T("CFolderDialog(%p)::AddRef\n"),this);
	return ExternalAddRef();
}

STDMETHODIMP_(ULONG) CFolderDialog::Release()
{
	//BFTRACE(_T("CFolderDialog(%p)::Release\n"), this);
	return ExternalRelease();
}

STDMETHODIMP CFolderDialog::QueryInterface(REFIID iid, LPVOID* ppvRet)
{
	if (ppvRet==NULL)
		return E_INVALIDARG;
	//BFTRACE(_T("CFolderDialog(%p)::QueryInterface(%s)\n"),this,_TR(iid));
	HRESULT hr = ExternalQueryInterface(&iid, ppvRet);
	//BFTRACE(_T(">CFolderDialog::QueryInterface returns %s, *ppv=%p\n"),_TR(hr),*ppvRet);
   return hr;
}

//////////////////////////////// IFolderFilter ////////////////////////////////
//
// Implementation passes control to parent class CFolderDialog (pThis)
//
BEGIN_INTERFACE_MAP(CFolderDialog, CCmdTarget)
	INTERFACE_PART(CFolderDialog, IID_IFolderFilter, FolderFilter)
END_INTERFACE_MAP()

STDMETHODIMP_(ULONG) CFolderDialog::XFolderFilter::AddRef()
{
	METHOD_PROLOGUE(CFolderDialog, FolderFilter);
	return pThis->AddRef();
}

STDMETHODIMP_(ULONG) CFolderDialog::XFolderFilter::Release()
{
	METHOD_PROLOGUE(CFolderDialog, FolderFilter);
	return pThis->Release();
}

STDMETHODIMP CFolderDialog::XFolderFilter::QueryInterface(REFIID iid, LPVOID* ppv)
{
	METHOD_PROLOGUE(CFolderDialog, FolderFilter);
	return pThis->QueryInterface(iid, ppv);
}

// Note: pHwnd is always NULL here as far as I can tell.
STDMETHODIMP CFolderDialog::XFolderFilter::GetEnumFlags(IShellFolder* psf,
	LPCITEMIDLIST pidlFolder, HWND *pHwnd, DWORD *pgrfFlags)
{
	METHOD_PROLOGUE(CFolderDialog, FolderFilter);
	return pThis->OnGetEnumFlags(psf, pidlFolder, pgrfFlags);
}

STDMETHODIMP CFolderDialog::XFolderFilter::ShouldShow(IShellFolder* psf,
	LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlItem)
{
	METHOD_PROLOGUE(CFolderDialog, FolderFilter);
	return pThis->OnShouldShow(psf, pidlFolder, pidlItem);
}