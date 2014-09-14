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

// TermsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "JPEGsnoop.h"
#include "TermsDlg.h"
#include ".\termsdlg.h"


// CTermsDlg dialog

IMPLEMENT_DYNAMIC(CTermsDlg, CDialog)
CTermsDlg::CTermsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTermsDlg::IDD, pParent)
	, strEula(_T(""))
	, bEulaOk(FALSE)
	, bUpdateAuto(FALSE)
{
	bEulaOk = false;
	bUpdateAuto = true;

	strEula = _T("");
strEula += _T("\r\n");
strEula += _T("ImpulseAdventure Software - License Agreement\r\n");
strEula += _T("\r\n");
strEula += _T("END USER LICENSE AGREEMENT (EULA) - Do not run the supplied software, if you DO NOT agree with the terms of this agreement.\r\n");
strEula += _T("\r\n");
strEula += _T("Software License Agreement\r\n");
strEula += _T("\r\n");

strEula += _T("IMPORTANT-READ CAREFULLY: This ImpulseAdventure Software End-User License Agreement ('EULA') is a legal agreement between you (either an individual or a single entity) and ImpulseAdventure for all SOFTWARE produced by ImpulseAdventure, which includes the documentation, any associated SOFTWARE components, any media, any printed materials, and any 'online' or electronic documentation ('SOFTWARE'). By installing, copying, or otherwise ");
strEula += _T("using the SOFTWARE, you agree to be bound by the terms of this EULA. If you do not agree to the terms of this EULA, do not download and install or use the SOFTWARE.\r\n");
strEula += _T("\r\n");
strEula += _T("1. The SOFTWARE is licensed, not sold.\r\n");

strEula += _T("(a) The software is released as 'Freeware'. This means that you can use it for as long as you like.\r\n");

strEula += _T("(b) The software is free for personal and commercial use.\r\n");
strEula += _T("\r\n");
strEula += _T("2. GRANT OF LICENSE.\r\n");
strEula += _T("\r\n");
strEula += _T("ImpulseAdventure grants you a limited, non-exclusive, non-transferable, royalty-free license to use the executable code of the ");
strEula += _T("SOFTWARE on your computer as long as the terms of this agreement are respected.\r\n");
strEula += _T("\r\n");
strEula += _T("3. DISTRIBUTION.\r\n");
strEula += _T("\r\n");
strEula += _T("You are hereby licensed to make copies of the SOFTWARE as you wish; give exact copies of the original SOFTWARE to ");
strEula += _T("anyone; and distribute the SOFTWARE in its unmodified form via electronic means (Internet, BBS's, Shareware distribution libraries, CD-ROMs, etc.). You may NOT charge a distribution fee for the package and you must not represent ");
strEula += _T("in any way that you are selling the SOFTWARE itself. Your distribution of the SOFTWARE will not entitle you to any compensation from ImpulseAdventure. You must distribute a copy of this EULA with any copy of the SOFTWARE and anyone ");
strEula += _T("to whom you distribute the SOFTWARE is subject to this EULA.\r\n");
strEula += _T("\r\n");
strEula += _T("(3 - a) Distribution on the Internet\r\n");
strEula += _T("Direct link (Hot link) is not allowed except some authorized software archives. All download links must be accompanied by a link to http://www.impulseadventure.com/photo/ . Please copy whole software and place it on your server.\r\n");
strEula += _T("\r\n");
strEula += _T("4. TRANSFERRING COPYRIGHTED MATERIAL.\r\n");
strEula += _T("\r\n");
strEula += _T("Not all files display if it has been authorized for copying or distribution. Copying or distributing copyrighted files may violate copyright laws. Compliance with the copyright law remains your responsibility. Users who infringe on copyrights may be held liable by the copyright owner. If ImpulseAdventure becomes aware that you are using its products to infringe on copyrights owned by others, we will terminate your license in accordance with the Digital Millennium Copyright Act.\r\n");
strEula += _T("\r\n");
strEula += _T("5. RESTRICTIONS.\r\n");
strEula += _T("\r\n");
strEula += _T("You may not reverse engineer, de-compile, or disassemble the SOFTWARE. You may not rent, lease, or lend the SOFTWARE (Do Not forget the software presented here are freeware). You may permanently transfer all of your rights under this EULA, provided the recipient agrees to the terms of this EULA. You may not use the SOFTWARE to perform any unauthorized transfer of information (e.g. transfer of files in violation of a copyright) or for any illegal purpose.\r\n");
strEula += _T("\r\n");
strEula += _T("6. SUPPORT SERVICES.\r\n");
strEula += _T("\r\n");
strEula += _T("ImpulseAdventure may provide you with support services related to the SOFTWARE. Use of Support Services is governed by ImpulseAdventure policies and programs described in the user manual, in online documentation, and/or other materials provided by ImpulseAdventure, as they may be modified from time to time. Any supplemental SOFTWARE code provided to you as part of the Support Services shall be considered part of the SOFTWARE and subject to the terms and conditions of this EULA. With respect to technical information you provide to ImpulseAdventure as part of the Support Services, ImpulseAdventure may use such information for its business purposes, including for product support and development. ImpulseAdventure will not utilize such technical information in a form that personally identifies you.\r\n");
strEula += _T("\r\n");
strEula += _T("(6 - a) SUPPORT, BUG REPORTS, SUGGESTIONS\r\n");
strEula += _T("You are encouraged to send bug reports and suggestions. The software are not supported as you see above. Hence, your technical questions may or may not be answered. Questions, bug reports, comments and suggestions should all be sent to jpegsnoop@impulseadventure.com, with a subject line that includes 'JPEGsnoop'.\r\n");
strEula += _T("\r\n");
strEula += _T("7. TERMINATION.\r\n");
strEula += _T("\r\n");
strEula += _T("Without prejudice to any other rights, ImpulseAdventure may terminate this EULA if you fail to comply with the terms and conditions of this EULA. In such event, you must destroy all copies of the SOFTWARE.\r\n");
strEula += _T("\r\n");
strEula += _T("8. COPYRIGHT.\r\n");
strEula += _T("\r\n");
strEula += _T("The SOFTWARE is protected by copyright laws and international treaty provisions. You acknowledge that no title to the intellectual property in the SOFTWARE is transferred to you. You further acknowledge that title and full ownership rights to the SOFTWARE will remain the exclusive property of ImpulseAdventure and you will not acquire any rights to the SOFTWARE ");
strEula += _T("except as expressly set forth in this license. You agree that any copies of the SOFTWARE will contain the same proprietary notices which appear on and in the SOFTWARE.\r\n");
strEula += _T("\r\n");
strEula += _T("9. NO WARRANTIES.\r\n");
strEula += _T("\r\n");
strEula += _T("ImpulseAdventure expressly disclaims any warranty for the SOFTWARE. THE SOFTWARE AND ANY RELATED DOCUMENTATION IS PROVIDED 'AS IS' WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING, WITHOUT LIMITATION, HE IMPLIED WARRANTIES OR MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR NO INFRINGEMENT. THE ENTIRE RISK ARISING OUT OF ");
strEula += _T("USE OR PERFORMANCE OF THE SOFTWARE REMAINS WITH YOU.THE AUTHOR (ImpulseAdventure) IS NOT OBLIGATED TO PROVIDE ANY UPDATES TO THE SOFTWARE.\r\n");
strEula += _T("\r\n");
strEula += _T("10. LIMITATION OF LIABILITY.\r\n");
strEula += _T("\r\n");
strEula += _T("IN NO EVENT SHALL ImpulseAdventure OR ITS SUPPLIERS BE LIABLE TO YOU FOR ANY CONSEQUENTIAL, SPECIAL, INCIDENTAL, OR INDIRECT DAMAGES OF ANY KIND ARISING OUT OF THE DELIVERY, PERFORMANCE, OR USE OF THE SOFTWARE, EVEN IF ImpulseAdventure HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES. IN ANY EVENT, ImpulseAdventure'S LIABILITY FOR ANY CLAIM, WHETHER IN CONTRACT, TORT, OR ANY OTHER THEORY OF LIABILITY WILL NOT EXCEED THE GREATER OF U.S.$0.00. (The software is freeware).\r\n");
strEula += _T("\r\n");
strEula += _T("11. MISCELLANEOUS.\r\n");
strEula += _T("Should you have any questions concerning this EULA, or if you desire to contact ImpulseAdventure for any reason, please contact by electronic mail at: jpegsnoop@impulseadventure.com\r\n");


}

CTermsDlg::~CTermsDlg()
{
}

void CTermsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EULA, strEula);
	DDX_Check(pDX, IDC_EULA_OK, bEulaOk);
	DDX_Check(pDX, IDC_UPDATE_AUTO, bUpdateAuto);
}


BEGIN_MESSAGE_MAP(CTermsDlg, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


// CTermsDlg message handlers

void CTermsDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	UpdateData(true);
	if (bEulaOk) {
		OnOK();
	} else {
		AfxMessageBox(_T("You need to agree to the terms or else click EXIT"));
	}
}

