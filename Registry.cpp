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
// The following code is derived from the following project on CodeProject:
//
//		Title:		Another registry class
//		Author:		SteveKing
//		URL:		http://www.codeproject.com/Articles/2521/Another-registry-class
//		Date:		Apr 25, 2003
//		License:	CPOL (Code Project Open License)
//
// ====================================================================================================


#include "stdafx.h"
#include "Registry.h"

#ifdef _MFC_VER
//MFC is available - also use the MFC-based classes	

CRegDWORD::CRegDWORD(void)
{
	m_value = 0;
	m_defaultvalue = 0;
	m_key = "";
	m_base = HKEY_CURRENT_USER;
	m_read = FALSE;
	m_force = FALSE;
}

/**
 * Constructor.
 * @param key the path to the key, including the key. example: "Software\\Company\\SubKey\\MyValue"
 * @param def the default value used when the key does not exist or a read error occured
 * @param force set to TRUE if no cache should be used, i.e. always read and write directly from/to registry
 * @param base a predefined base key like HKEY_LOCAL_MACHINE. see the SDK documentation for more information.
 */
CRegDWORD::CRegDWORD(CString key, DWORD def, BOOL force, HKEY base)
{
	m_value = 0;
	m_defaultvalue = def;
	m_force = force;
	m_base = base;
	m_read = FALSE;
	key.TrimLeft(_T("\\"));
	m_path = key.Left(key.ReverseFind(_T('\\')));
	m_path.TrimRight(_T("\\"));
	m_key = key.Right(key.GetLength() - key.ReverseFind(_T('\\')));
	m_key.Trim(_T("\\"));
	read();
}

CRegDWORD::~CRegDWORD(void)
{
	//write();
}

DWORD	CRegDWORD::read()
{
	ASSERT(m_key != _T(""));
	if (RegOpenKeyEx(m_base, m_path, 0, KEY_EXECUTE, &m_hKey)==ERROR_SUCCESS)
	{
		int size = sizeof(m_value);
		DWORD type;
		if (RegQueryValueEx(m_hKey, m_key, NULL, &type, (BYTE*) &m_value,(LPDWORD) &size)==ERROR_SUCCESS)
		{
			ASSERT(type==REG_DWORD);
			m_read = TRUE;
			RegCloseKey(m_hKey);
			return m_value;
		}
		else
		{
			RegCloseKey(m_hKey);
			return m_defaultvalue;
		}
	}
	return m_defaultvalue;
}

void CRegDWORD::write()
{
	ASSERT(m_key != _T(""));
	DWORD disp;
	if (RegCreateKeyEx(m_base, m_path, 0, _T(""), REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &m_hKey, &disp)!=ERROR_SUCCESS)
	{
		return;
	}
	if (RegSetValueEx(m_hKey, m_key, 0, REG_DWORD,(const BYTE*) &m_value, sizeof(m_value))==ERROR_SUCCESS)
	{
		m_read = TRUE;
	}
	RegCloseKey(m_hKey);
}


CRegDWORD::operator DWORD()
{
	if ((m_read)&&(!m_force))
		return m_value;
	else
	{
		return read();
	}
}

CRegDWORD& CRegDWORD::operator =(DWORD d)
{
	if ((d==m_value)&&(!m_force))
	{
		//no write to the registry required, its the same value
		return *this;
	}
	m_value = d;
	write();
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////

CRegString::CRegString(void)
{
	m_value = _T("");
	m_defaultvalue = _T("");
	m_key = _T("");
	m_base = HKEY_CURRENT_USER;
	m_read = FALSE;
	m_force = FALSE;
}

/**
 * Constructor.
 * @param key the path to the key, including the key. example: "Software\\Company\\SubKey\\MyValue"
 * @param def the default value used when the key does not exist or a read error occured
 * @param force set to TRUE if no cache should be used, i.e. always read and write directly from/to registry
 * @param base a predefined base key like HKEY_LOCAL_MACHINE. see the SDK documentation for more information.
 */
CRegString::CRegString(CString key, CString def, BOOL force, HKEY base)
{
	m_value = "";
	m_defaultvalue = def;
	m_force = force;
	m_base = base;
	m_read = FALSE;
	key.TrimLeft(_T("\\"));
	m_path = key.Left(key.ReverseFind(_T('\\')));
	m_path.TrimRight(_T("\\"));
	m_key = key.Right(key.GetLength() - key.ReverseFind(_T('\\')));
	m_key.Trim(_T("\\"));
	read();
}

CRegString::~CRegString(void)
{
	//write();
}

CString	CRegString::read()
{
	ASSERT(m_key != _T(""));
	if (RegOpenKeyEx(m_base, m_path, 0, KEY_EXECUTE, &m_hKey)==ERROR_SUCCESS)
	{
		int size = 0;
		DWORD type;
		RegQueryValueEx(m_hKey, m_key, NULL, &type, NULL, (LPDWORD) &size);
		TCHAR* pStr = new TCHAR[size];
		if (RegQueryValueEx(m_hKey, m_key, NULL, &type, (BYTE*) pStr,(LPDWORD) &size)==ERROR_SUCCESS)
		{
			m_value = CString(pStr);
			delete [] pStr;
			ASSERT(type==REG_SZ);
			m_read = TRUE;
			RegCloseKey(m_hKey);
			return m_value;
		}
		else
		{
			delete [] pStr;
			RegCloseKey(m_hKey);
			return m_defaultvalue;
		}
	}
	return m_defaultvalue;
}

void CRegString::write()
{
	ASSERT(m_key != _T(""));
	DWORD disp;
	if (RegCreateKeyEx(m_base, m_path, 0, _T(""), REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &m_hKey, &disp)!=ERROR_SUCCESS)
	{
		return;
	}
#ifdef _UNICODE
	if (RegSetValueEx(m_hKey, m_key, 0, REG_SZ, (BYTE *)(LPCTSTR)m_value, (m_value.GetLength()+1)*2)==ERROR_SUCCESS)
#else
	if (RegSetValueEx(m_hKey, m_key, 0, REG_SZ, (BYTE *)(LPCTSTR)m_value, m_value.GetLength()+1)==ERROR_SUCCESS)
#endif
	{
		m_read = TRUE;
	}
	RegCloseKey(m_hKey);
}

CRegString::operator CString()
{
	if ((m_read)&&(!m_force))
		return m_value;
	else
	{
		return read();
	}
}

CRegString& CRegString::operator =(CString s)
{
	if ((s==m_value)&&(!m_force))
	{
		//no write to the registry required, its the same value
		return *this;
	}
	m_value = s;
	write();
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////

CRegRect::CRegRect(void)
{
	m_value = CRect(0,0,0,0);
	m_defaultvalue = CRect(0,0,0,0);
	m_key = _T("");
	m_base = HKEY_CURRENT_USER;
	m_read = FALSE;
	m_force = FALSE;
}

/**
 * Constructor.
 * @param key the path to the key, including the key. example: "Software\\Company\\SubKey\\MyValue"
 * @param def the default value used when the key does not exist or a read error occured
 * @param force set to TRUE if no cache should be used, i.e. always read and write directly from/to registry
 * @param base a predefined base key like HKEY_LOCAL_MACHINE. see the SDK documentation for more information.
 */
CRegRect::CRegRect(CString key, CRect def, BOOL force, HKEY base)
{
	m_value = CRect(0,0,0,0);
	m_defaultvalue = def;
	m_force = force;
	m_base = base;
	m_read = FALSE;
	key.TrimLeft(_T("\\"));
	m_path = key.Left(key.ReverseFind(_T('\\')));
	m_path.TrimRight(_T("\\"));
	m_key = key.Right(key.GetLength() - key.ReverseFind(_T('\\')));
	m_key.Trim(_T("\\"));
	read();
}

CRegRect::~CRegRect(void)
{
	//write();
}

CRect	CRegRect::read()
{
	ASSERT(m_key != _T(""));
	if (RegOpenKeyEx(m_base, m_path, 0, KEY_EXECUTE, &m_hKey)==ERROR_SUCCESS)
	{
		int size = 0;
		DWORD type;
		RegQueryValueEx(m_hKey, m_key, NULL, &type, NULL, (LPDWORD) &size);
		LPRECT pRect = (LPRECT)new char[size];
		if (RegQueryValueEx(m_hKey, m_key, NULL, &type, (BYTE*) pRect,(LPDWORD) &size)==ERROR_SUCCESS)
		{
			m_value = CRect(pRect);
			delete [] pRect;
			ASSERT(type==REG_BINARY);
			m_read = TRUE;
			RegCloseKey(m_hKey);
			return m_value;
		}
		else
		{
			delete [] pRect;
			RegCloseKey(m_hKey);
			return m_defaultvalue;
		}
	}
	return m_defaultvalue;
}

void CRegRect::write()
{
	ASSERT(m_key != _T(""));
	DWORD disp;
	if (RegCreateKeyEx(m_base, m_path, 0, _T(""), REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &m_hKey, &disp)!=ERROR_SUCCESS)
	{
		return;
	}
	
	if (RegSetValueEx(m_hKey, m_key, 0, REG_BINARY, (BYTE *)(LPRECT)m_value, sizeof(m_value))==ERROR_SUCCESS)
	{
		m_read = TRUE;
	}
	RegCloseKey(m_hKey);
}

CRegRect::operator CRect()
{
	if ((m_read)&&(!m_force))
		return m_value;
	else
	{
		return read();
	}
}

CRegRect& CRegRect::operator =(CRect s)
{
	if ((s==m_value)&&(!m_force))
	{
		//no write to the registry required, its the same value
		return *this;
	}
	m_value = s;
	write();
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////

CRegPoint::CRegPoint(void)
{
	m_value = CPoint(0,0);
	m_defaultvalue = CPoint(0,0);
	m_key = "";
	m_base = HKEY_CURRENT_USER;
	m_read = FALSE;
	m_force = FALSE;
}

/**
 * Constructor.
 * @param key the path to the key, including the key. example: "Software\\Company\\SubKey\\MyValue"
 * @param def the default value used when the key does not exist or a read error occured
 * @param force set to TRUE if no cache should be used, i.e. always read and write directly from/to registry
 * @param base a predefined base key like HKEY_LOCAL_MACHINE. see the SDK documentation for more information.
 */
CRegPoint::CRegPoint(CString key, CPoint def, BOOL force, HKEY base)
{
	m_value = CPoint(0,0);
	m_defaultvalue = def;
	m_force = force;
	m_base = base;
	m_read = FALSE;
	key.TrimLeft(_T("\\"));
	m_path = key.Left(key.ReverseFind(_T('\\')));
	m_path.TrimRight(_T("\\"));
	m_key = key.Right(key.GetLength() - key.ReverseFind(_T('\\')));
	m_key.Trim(_T("\\"));
	read();
}

CRegPoint::~CRegPoint(void)
{
	//write();
}

CPoint	CRegPoint::read()
{
	ASSERT(m_key != _T(""));
	if (RegOpenKeyEx(m_base, m_path, 0, KEY_EXECUTE, &m_hKey)==ERROR_SUCCESS)
	{
		int size = 0;
		DWORD type;
		RegQueryValueEx(m_hKey, m_key, NULL, &type, NULL, (LPDWORD) &size);
		POINT* pPoint = (POINT *)new char[size];
		if (RegQueryValueEx(m_hKey, m_key, NULL, &type, (BYTE*) pPoint,(LPDWORD) &size)==ERROR_SUCCESS)
		{
			m_value = CPoint(*pPoint);
			delete [] pPoint;
			ASSERT(type==REG_BINARY);
			m_read = TRUE;
			RegCloseKey(m_hKey);
			return m_value;
		}
		else
		{
			delete [] pPoint;
			RegCloseKey(m_hKey);
			return m_defaultvalue;
		}
	}
	return m_defaultvalue;
}

void CRegPoint::write()
{
	ASSERT(m_key != _T(""));
	DWORD disp;
	if (RegCreateKeyEx(m_base, m_path, 0, _T(""), REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &m_hKey, &disp)!=ERROR_SUCCESS)
	{
		return;
	}
	
	if (RegSetValueEx(m_hKey, m_key, 0, REG_BINARY, (BYTE *)(POINT *)&m_value, sizeof(m_value))==ERROR_SUCCESS)
	{
		m_read = TRUE;
	}
	RegCloseKey(m_hKey);
}

CRegPoint::operator CPoint()
{
	if ((m_read)&&(!m_force))
		return m_value;
	else
	{
		return read();
	}
}

CRegPoint& CRegPoint::operator =(CPoint s)
{
	if ((s==m_value)&&(!m_force))
	{
		//no write to the registry required, its the same value
		return *this;
	}
	m_value = s;
	write();
	return *this;
}

#endif

/////////////////////////////////////////////////////////////////////

CRegStdString::CRegStdString(void)
{
	m_value = _T("");
	m_defaultvalue = _T("");
	m_key = _T("");
	m_base = HKEY_CURRENT_USER;
	m_read = FALSE;
	m_force = FALSE;
}

/**
 * Constructor.
 * @param key the path to the key, including the key. example: "Software\\Company\\SubKey\\MyValue"
 * @param def the default value used when the key does not exist or a read error occured
 * @param force set to TRUE if no cache should be used, i.e. always read and write directly from/to registry
 * @param base a predefined base key like HKEY_LOCAL_MACHINE. see the SDK documentation for more information.
 */
CRegStdString::CRegStdString(stdstring key, stdstring def, BOOL force, HKEY base)
{
	m_value = _T("");
	m_defaultvalue = def;
	m_force = force;
	m_base = base;
	m_read = FALSE;

	stdstring::size_type pos = key.find_last_of(_T('\\'));
    m_path = key.substr(0, pos);
	m_key = key.substr(pos + 1);
	read();
}

CRegStdString::~CRegStdString(void)
{
	//write();
}

stdstring	CRegStdString::read()
{
	if (RegOpenKeyEx(m_base, m_path.c_str(), 0, KEY_EXECUTE, &m_hKey)==ERROR_SUCCESS)
	{
		int size = 0;
		DWORD type;
		RegQueryValueEx(m_hKey, m_key.c_str(), NULL, &type, NULL, (LPDWORD) &size);
		TCHAR* pStr = new TCHAR[size];
		if (RegQueryValueEx(m_hKey, m_key.c_str(), NULL, &type, (BYTE*) pStr,(LPDWORD) &size)==ERROR_SUCCESS)
		{
			m_value.assign(pStr);
			delete [] pStr;
			m_read = TRUE;
			RegCloseKey(m_hKey);
			return m_value;
		}
		else
		{
			delete [] pStr;
			RegCloseKey(m_hKey);
			return m_defaultvalue;
		}
	}
	return m_defaultvalue;
}

void CRegStdString::write()
{
	DWORD disp;
	if (RegCreateKeyEx(m_base, m_path.c_str(), 0, _T(""), REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &m_hKey, &disp)!=ERROR_SUCCESS)
	{
		return;
	}
	if (RegSetValueEx(m_hKey, m_key.c_str(), 0, REG_SZ, (BYTE *)m_value.c_str(), (DWORD)m_value.size()+1)==ERROR_SUCCESS)
	{
		m_read = TRUE;
	}
	RegCloseKey(m_hKey);
}

CRegStdString::operator LPCTSTR()
{
	if ((m_read)&&(!m_force))
		return m_value.c_str();
	else
		return read().c_str();
}

CRegStdString::operator stdstring()
{
	if ((m_read)&&(!m_force))
		return m_value;
	else
	{
		return read();
	}
}

CRegStdString& CRegStdString::operator =(stdstring s)
{
	if ((s==m_value)&&(!m_force))
	{
		//no write to the registry required, its the same value
		return *this;
	}
	m_value = s;
	write();
	return *this;
}

/////////////////////////////////////////////////////////////////////

CRegStdWORD::CRegStdWORD(void)
{
	m_value = 0;
	m_defaultvalue = 0;
	m_key = _T("");
	m_base = HKEY_CURRENT_USER;
	m_read = FALSE;
	m_force = FALSE;
}

/**
 * Constructor.
 * @param key the path to the key, including the key. example: "Software\\Company\\SubKey\\MyValue"
 * @param def the default value used when the key does not exist or a read error occured
 * @param force set to TRUE if no cache should be used, i.e. always read and write directly from/to registry
 * @param base a predefined base key like HKEY_LOCAL_MACHINE. see the SDK documentation for more information.
 */
CRegStdWORD::CRegStdWORD(stdstring key, DWORD def, BOOL force, HKEY base)
{
	m_value = 0;
	m_defaultvalue = def;
	m_force = force;
	m_base = base;
	m_read = FALSE;

	stdstring::size_type pos = key.find_last_of(_T('\\'));
    m_path = key.substr(0, pos);
	m_key = key.substr(pos + 1);
	read();
}

CRegStdWORD::~CRegStdWORD(void)
{
	//write();
}

DWORD	CRegStdWORD::read()
{
	if (RegOpenKeyEx(m_base, m_path.c_str(), 0, KEY_EXECUTE, &m_hKey)==ERROR_SUCCESS)
	{
		int size = sizeof(m_value);
		DWORD type;
		if (RegQueryValueEx(m_hKey, m_key.c_str(), NULL, &type, (BYTE*) &m_value,(LPDWORD) &size)==ERROR_SUCCESS)
		{
			m_read = TRUE;
			RegCloseKey(m_hKey);
			return m_value;
		}
		else
		{
			RegCloseKey(m_hKey);
			return m_defaultvalue;
		}
	}
	return m_defaultvalue;
}

void CRegStdWORD::write()
{
	DWORD disp;
	if (RegCreateKeyEx(m_base, m_path.c_str(), 0, _T(""), REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &m_hKey, &disp)!=ERROR_SUCCESS)
	{
		return;
	}
	if (RegSetValueEx(m_hKey, m_key.c_str(), 0, REG_DWORD,(const BYTE*) &m_value, sizeof(m_value))==ERROR_SUCCESS)
	{
		m_read = TRUE;
	}
	RegCloseKey(m_hKey);
}

CRegStdWORD::operator DWORD()
{
	if ((m_read)&&(!m_force))
		return m_value;
	else
	{
		return read();
	}
}
