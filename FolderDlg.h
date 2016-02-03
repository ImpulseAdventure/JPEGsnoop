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
// The following code is based on an example CFolderDialog class that appears in MSDN:
//
//		Title:		CFolderDialog (C++ at Work: Counting MDI Children, Browsing for Folders)
//		Authors:	Paul DiLascia
//		URL:		http://msdn.microsoft.com/en-us/magazine/cc163789.aspx
//		Date:		Jun 2005
// ====================================================================================================


////////////////////////////////////////////////////////////////
// MSDN Magazine -- June 2005
// If this code works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
// Compiles with Visual Studio .NET 2003 (V7.1) on Windows XP. Tab size=3.
//
#pragma once
//#include "debug.h" // debugging tools

//////////////////
// BRTRACEFN is like TRACEFN but only does anything if
// CFolderDialog::bTRACE is on. See Debug.h.
//
#ifdef _DEBUG
#define BFTRACE															\
	if (CFolderDialog::bTRACE)										\
		TRACE
#else
#define BFTRACE
#endif

//////////////////
// Class to encapsulate SHBrowseForFolder. To use, instantiate in your app
// and call BrowseForFolder, which returns a PIDL. You can call GetPathName
// to get the path name from the PIDL. For example:
//
//		CFolderDialog dlg(this);
//		LPCITEMIDLIST pidl = dlg.BrowseForFolder(...);
//		CString path = dlg.GetPathName(pidl);
//
//	You can also derive your own class from CFolderDialog to override virtual
//	message handler functions like OnInitialized and OnSelChanged to do stuff
//	when various things happen. This replaces the callback mechanism for
//	SHBrowseForFolder. You call various wrapper functions from your hanlers to
//	send messages to the browser window. For example:
//
//		int CMyFolderDialog::OnInitialized()
//		{
//			CFolderDialog::OnInitialized();
//			SetStatusText(_T("Nice day, isn't it?"));
//			SetOKText(L"Choose Me!");
//			return 0;
//		}
//
// You can set CFolderDialog::bTRACE=TRUE to turn on debugging TRACE
// diagnostics to help you understand what's going on.
//
class CFolderDialog : public CWnd {
public:
	static BOOL bTRACE;		// controls tracing

	CFolderDialog(CWnd* pWnd);
	~CFolderDialog();

	LPCITEMIDLIST BrowseForFolder(LPCTSTR title, UINT flags,
		LPCITEMIDLIST pidRoot=NULL, BOOL bFilter=FALSE);

	CString GetDisplayName() { return m_sDisplayName; }

	// helpers
	static CString GetPathName(LPCITEMIDLIST pidl);
	static CString GetDisplayNameOf(IShellFolder* psf, LPCITEMIDLIST pidl, DWORD uFlags);
	static void FreePIDL(LPCITEMIDLIST pidl);

	void SetStartPath(CString strPath); //CAL!
private:
	CString		m_strStartPath;	//CAL!

protected:
	BROWSEINFO m_brinfo;						 // internal structure for SHBrowseForFolder
	CString m_sDisplayName;					 // display name of folder chosen
	BOOL m_bFilter;							 // do custom filtering?
	CComQIPtr<IShellFolder> m_shfRoot;	 // handy to have root folder

	static int CALLBACK CallbackProc(HWND hwnd, UINT msg, LPARAM lp, LPARAM lpData);

	virtual int OnMessage(UINT msg, LPARAM lp);	// internal catch-all

	// Virtual message handlers: override these instead of using callback
	virtual void OnInitialized();
	virtual void OnIUnknown(IUnknown* punk);
	virtual void OnSelChanged(LPCITEMIDLIST pidl);
	virtual BOOL OnValidateFailed(LPCTSTR lpsz);

	// Wrapper functions for folder dialog messages--call these only from
	// virtual handler functions above!

	// Enable or disable the OK button
	void EnableOK(BOOL bEnable) {
		SendMessage(BFFM_ENABLEOK,0,bEnable);
	}

	// The Microsoft documentation is wrong for this: text in LPARAM, not WPARAM!
	void SetOKText(LPCWSTR lpText) {
		SendMessage(BFFM_SETOKTEXT,0,(LPARAM)lpText);
	}

	// Set selected item from string or PIDL.
	// The documentation says lpText must be Unicode, but it can be LPCTSTR.
	void SetSelection(LPCTSTR lpText) {
		SendMessage(BFFM_SETSELECTION,TRUE,(LPARAM)lpText);
	}
	void SetSelection(LPCITEMIDLIST pidl) {
		SendMessage(BFFM_SETSELECTION,FALSE,(LPARAM)pidl);
	}

	// Expand item from string or PIDL
	void SetExpanded(LPCWSTR lpText) {
		SendMessage(BFFM_SETEXPANDED,TRUE,(LPARAM)lpText);
	}
	void SetExpanded(LPCITEMIDLIST pidl) {
		SendMessage(BFFM_SETEXPANDED,FALSE,(LPARAM)pidl);
	}

	// Set status window text
	void SetStatusText(LPCTSTR pText) {
		SendMessage(BFFM_SETSTATUSTEXT,0,(LPARAM)pText);
	}

	// Override for custom filtering. You must call BrowseForFolder with bFilter=TRUE.
	virtual HRESULT OnGetEnumFlags(IShellFolder* psf,
		LPCITEMIDLIST pidlFolder,
		DWORD *pgrfFlags);
	virtual HRESULT OnShouldShow(IShellFolder* psf,
		LPCITEMIDLIST pidlFolder,
		LPCITEMIDLIST pidlItem);

   // COM interfaces. The only one currently is IFolderFilter
	DECLARE_INTERFACE_MAP()

	// IUnknown--all nested interfaces call these
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();
	STDMETHOD(QueryInterface)(REFIID iid, LPVOID* ppvObj);

	// COM interface IFolderFilter, used to do custom filtering
	BEGIN_INTERFACE_PART(FolderFilter, IFolderFilter)
   STDMETHOD(GetEnumFlags) (IShellFolder* psf,
		LPCITEMIDLIST pidlFolder,
		HWND *pHwnd,
		DWORD *pgrfFlags);
   STDMETHOD(ShouldShow) (IShellFolder* psf,
		LPCITEMIDLIST pidlFolder,
		LPCITEMIDLIST pidlItem);
	END_INTERFACE_PART(FolderFilter)

	DECLARE_DYNAMIC(CFolderDialog)
};