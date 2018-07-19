CC=cl
CFLAGS=-c -DSTRICT -G3 -Ow -W3 -Zp -Tp /I"source"
CFLAGSMT=-c /D_WIN64 /D_AFXDLL /MD /nologo /W3 /WX- /O2 /Oi /Ot /Oy- /DWIN32 /D_WINDOWS /DNDEBUG /D_UNICODE /DUNICODE /Gm- /GS- /Gy /arch:SSE2 /fp:precise /Zc:wchar_t /Zc:forScope /Fax64\Release\ /Fox64\Release\ /Fdx64\Release\vc100.pdb /Gd /errorReport:none /I"source"
LINKER=link
GUIFLAGS=-SUBSYSTEM:windows /OUT:x64\Release\JPEGsnoop.exe /NOLOGO  /PDB:"x64\Release\JPEGsnoop.pdb" /SUBSYSTEM:WINDOWS /OPT:REF /OPT:NOICF /TLBID:1 /DYNAMICBASE /NXCOMPAT /MACHINE:X64 /ERRORREPORT:NONE /ENTRY:wWinMainCRTStartup "/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'"
DLLFLAGS=-SUBSYSTEM:windows -DLL
GUILIBS=Wininet.lib
RC=rc
RCVARS=/DWIN32 /D_WIN64  /DNDEBUG /D_UNICODE /DUNICODE /D_AFXDLL   /l0x0409 /Ix64\Release\ /nologo /fox64\Release\JPEGsnoop.res
MT=mt
MTSTUFF=/nologo /verbose  -manifest x64\Release\JPEGsnoop.exe.manifest

SRC=source/

docks : trail x64\Release\JPEGsnoop.exe
	$(MT) $(MTSTUFF) -outputresource:x64\Release\JPEGsnoop.exe

x64\Release\JPEGsnoop.exe : x64\Release\JPEGsnoop.obj x64\Release\JPEGsnoopCore.obj  x64\Release\MainFrm.obj x64\Release\AboutDlg.obj x64\Release\BatchDlg.obj x64\Release\CntrItem.obj x64\Release\DbManageDlg.obj x64\Release\DbSigs.obj x64\Release\DbSubmitDlg.obj x64\Release\DecodeDetailDlg.obj x64\Release\Dib.obj x64\Release\DocLog.obj x64\Release\ExportDlg.obj x64\Release\ExportTiffDlg.obj x64\Release\FileTiff.obj x64\Release\FolderDlg.obj x64\Release\General.obj x64\Release\HyperlinkStatic.obj x64\Release\ImgDecode.obj x64\Release\JfifDecode.obj x64\Release\JPEGsnoopDoc.obj x64\Release\JPEGsnoopView.obj x64\Release\JPEGsnoopViewImg.obj x64\Release\LookupDlg.obj x64\Release\Md5.obj x64\Release\ModelessDlg.obj x64\Release\NoteDlg.obj x64\Release\OffsetDlg.obj x64\Release\OverlayBufDlg.obj x64\Release\Registry.obj x64\Release\SettingsDlg.obj x64\Release\SnoopConfig.obj  x64\Release\TermsDlg.obj x64\Release\UpdateAvailDlg.obj x64\Release\UrlString.obj x64\Release\WindowBuf.obj x64\Release\DecodePs.obj x64\Release\DecodeDicomTags.obj x64\Release\DecodeDicom.obj x64\Release\JPEGsnoop.res
    $(LINKER) $(GUIFLAGS) x64\Release\JPEGsnoop.obj x64\Release\JPEGsnoopCore.obj x64\Release\MainFrm.obj x64\Release\AboutDlg.obj x64\Release\BatchDlg.obj x64\Release\CntrItem.obj x64\Release\DbManageDlg.obj x64\Release\DbSigs.obj x64\Release\DbSubmitDlg.obj x64\Release\DecodeDetailDlg.obj x64\Release\Dib.obj x64\Release\DocLog.obj x64\Release\ExportDlg.obj x64\Release\ExportTiffDlg.obj x64\Release\FileTiff.obj x64\Release\FolderDlg.obj x64\Release\General.obj x64\Release\HyperlinkStatic.obj x64\Release\ImgDecode.obj x64\Release\JfifDecode.obj x64\Release\JPEGsnoopDoc.obj x64\Release\JPEGsnoopView.obj x64\Release\JPEGsnoopViewImg.obj x64\Release\LookupDlg.obj x64\Release\Md5.obj x64\Release\ModelessDlg.obj x64\Release\NoteDlg.obj x64\Release\OffsetDlg.obj x64\Release\OverlayBufDlg.obj x64\Release\Registry.obj x64\Release\SettingsDlg.obj x64\Release\SnoopConfig.obj x64\Release\TermsDlg.obj x64\Release\UpdateAvailDlg.obj x64\Release\UrlString.obj x64\Release\WindowBuf.obj  x64\Release\DecodePs.obj x64\Release\DecodeDicom.obj x64\Release\DecodeDicomTags.obj x64\Release\JPEGsnoop.res $(GUILIBS)
 
trail:
	-@ if NOT EXIST "x64" mkdir "x64"
	-@ if NOT EXIST "x64\Release" mkdir "x64\Release"

x64\Release\JPEGsnoop.obj : $(SRC)JPEGsnoop.cpp $(SRC)JPEGsnoop.h $(SRC)JPEGsnoopDoc.h $(SRC)NoteDlg.h $(SRC)HyperlinkStatic.h $(SRC)ModelessDlg.h $(SRC)SettingsDlg.h $(SRC)UpdateAvailDlg.h $(SRC)JPEGsnoopView.h $(SRC)TermsDlg.h $(SRC)DbManageDlg.h $(SRC)StdAfx.h $(SRC)DbSubmitDlg.h $(SRC)snoop.h $(SRC)SnoopConfig.h $(SRC)resource.h $(SRC)MainFrm.h
	 $(CC) $(CFLAGSMT) $(SRC)JPEGsnoop.cpp
x64\Release\JPEGsnoopCore.obj : $(SRC)JPEGsnoopCore.cpp $(SRC)JPEGsnoopCore.h $(SRC)JPEGsnoop.h
	 $(CC) $(CFLAGSMT) $(SRC)JPEGsnoopCore.cpp

x64\Release\MainFrm.obj : $(SRC)MainFrm.cpp $(SRC)MainFrm.h $(SRC)StdAfx.h $(SRC)resource.h 
     $(CC) $(CFLAGSMT) $(SRC)MainFrm.cpp

x64\Release\AboutDlg.obj : $(SRC)AboutDlg.cpp $(SRC)AboutDlg.h $(SRC)StdAfx.h $(SRC)resource.h 
     $(CC) $(CFLAGSMT)   $(SRC)AboutDlg.cpp
 
x64\Release\BatchDlg.obj : $(SRC)BatchDlg.cpp $(SRC)BatchDlg.h $(SRC)StdAfx.h $(SRC)resource.h 
     $(CC) $(CFLAGSMT)  $(SRC)BatchDlg.cpp
 
x64\Release\CntrItem.obj : $(SRC)CntrItem.cpp $(SRC)CntrItem.h $(SRC)StdAfx.h $(SRC)resource.h 
     $(CC) $(CFLAGSMT)   $(SRC)CntrItem.cpp

x64\Release\DbManageDlg.obj : $(SRC)DbManageDlg.cpp $(SRC)DbManageDlg.h $(SRC)StdAfx.h $(SRC)resource.h 
     $(CC) $(CFLAGSMT)   $(SRC)DbManageDlg.cpp

x64\Release\DbSigs.obj : $(SRC)DbSigs.cpp $(SRC)DbSigs.h $(SRC)StdAfx.h $(SRC)resource.h 
     $(CC) $(CFLAGSMT)   $(SRC)DbSigs.cpp

x64\Release\DbSubmitDlg.obj : $(SRC)DbSubmitDlg.cpp  $(SRC)DbSubmitDlg.h $(SRC)StdAfx.h $(SRC)resource.h 
     $(CC) $(CFLAGSMT)  $(SRC)DbSubmitDlg.cpp
 
x64\Release\DecodeDetailDlg.obj : $(SRC)DecodeDetailDlg.cpp  $(SRC)DecodeDetailDlg.h $(SRC)StdAfx.h $(SRC)resource.h 
     $(CC) $(CFLAGSMT)  $(SRC)DecodeDetailDlg.cpp
x64\Release\DecodeDicom.obj :$(SRC)DecodeDicom.cpp  $(SRC)DecodeDicom.h $(SRC)StdAfx.h $(SRC)resource.h 
     $(CC) $(CFLAGSMT)  $(SRC)DecodeDicom.cpp
x64\Release\DecodeDicomTags.obj :$(SRC)DecodeDicomTags.cpp $(SRC)StdAfx.h $(SRC)resource.h 
     $(CC) $(CFLAGSMT)  $(SRC)DecodeDicomTags.cpp
x64\Release\DecodePs.obj :$(SRC)DecodePs.cpp $(SRC)DecodePs.h $(SRC)StdAfx.h $(SRC)resource.h 
     $(CC) $(CFLAGSMT)  $(SRC)DecodePs.cpp

x64\Release\Dib.obj : $(SRC)Dib.cpp $(SRC)Dib.h $(SRC)StdAfx.h $(SRC)resource.h 
     $(CC) $(CFLAGSMT)  $(SRC)Dib.cpp
 
x64\Release\DocLog.obj :$(SRC)DocLog.cpp  $(SRC)DocLog.h $(SRC)StdAfx.h $(SRC)resource.h 
     $(CC) $(CFLAGSMT)  $(SRC)DocLog.cpp
 
x64\Release\ExportDlg.obj :$(SRC)ExportDlg.cpp  $(SRC)ExportDlg.h $(SRC)StdAfx.h $(SRC)resource.h 
     $(CC) $(CFLAGSMT)  $(SRC)ExportDlg.cpp
 
x64\Release\ExportTiffDlg.obj :$(SRC)ExportTiffDlg.cpp  $(SRC)ExportTiffDlg.h $(SRC)StdAfx.h $(SRC)resource.h 
     $(CC) $(CFLAGSMT)   $(SRC)ExportTiffDlg.cpp

x64\Release\FileTiff.obj : $(SRC)FileTiff.cpp $(SRC)FileTiff.h $(SRC)StdAfx.h $(SRC)resource.h 
     $(CC) $(CFLAGSMT)  $(SRC)FileTiff.cpp
 
x64\Release\FolderDlg.obj :$(SRC)FolderDlg.cpp  $(SRC)FolderDlg.h $(SRC)StdAfx.h $(SRC)resource.h 
     $(CC) $(CFLAGSMT)   $(SRC)FolderDlg.cpp

x64\Release\General.obj : $(SRC)General.cpp $(SRC)General.h $(SRC)StdAfx.h $(SRC)resource.h 
     $(CC) $(CFLAGSMT)  $(SRC)General.cpp
 
x64\Release\HyperlinkStatic.obj :$(SRC)HyperlinkStatic.cpp  $(SRC)HyperlinkStatic.h $(SRC)StdAfx.h $(SRC)resource.h 
     $(CC) $(CFLAGSMT)   $(SRC)HyperlinkStatic.cpp

x64\Release\ImgDecode.obj : $(SRC)ImgDecode.cpp $(SRC)ImgDecode.h $(SRC)StdAfx.h $(SRC)resource.h 
     $(CC) $(CFLAGSMT)   $(SRC)ImgDecode.cpp

x64\Release\JfifDecode.obj : $(SRC)JfifDecode.cpp $(SRC)JfifDecode.h $(SRC)StdAfx.h $(SRC)resource.h 
     $(CC) $(CFLAGSMT)   $(SRC)JfifDecode.cpp
x64\Release\JPEGsnoopDoc.obj :$(SRC)JPEGsnoopDoc.cpp  $(SRC)JPEGsnoopDoc.h $(SRC)StdAfx.h $(SRC)resource.h 
    $(CC) $(CFLAGSMT)   $(SRC)JPEGsnoopDoc.cpp
x64\Release\JPEGsnoopView.obj : $(SRC)JPEGsnoopView.cpp $(SRC)JPEGsnoopView.h $(SRC)StdAfx.h $(SRC)resource.h 
    $(CC) $(CFLAGSMT)   $(SRC)JPEGsnoopView.cpp
x64\Release\JPEGsnoopViewImg.obj : $(SRC)JPEGsnoopViewImg.cpp $(SRC)JPEGsnoopViewImg.h $(SRC)StdAfx.h $(SRC)resource.h 
    $(CC) $(CFLAGSMT)   $(SRC)JPEGsnoopViewImg.cpp
x64\Release\LookupDlg.obj : $(SRC)LookupDlg.cpp $(SRC)LookupDlg.h $(SRC)StdAfx.h $(SRC)resource.h 
    $(CC) $(CFLAGSMT)  $(SRC)LookupDlg.cpp
x64\Release\Md5.obj :$(SRC)Md5.cpp  $(SRC)Md5.h $(SRC)StdAfx.h $(SRC)resource.h 
     $(CC) $(CFLAGSMT)  $(SRC)Md5.cpp
x64\Release\ModelessDlg.obj :$(SRC)ModelessDlg.cpp  $(SRC)ModelessDlg.h $(SRC)StdAfx.h $(SRC)resource.h 
     $(CC) $(CFLAGSMT)   $(SRC)ModelessDlg.cpp
x64\Release\NoteDlg.obj : $(SRC)NoteDlg.cpp $(SRC)NoteDlg.h $(SRC)StdAfx.h $(SRC)resource.h 
     $(CC) $(CFLAGSMT)   $(SRC)NoteDlg.cpp
x64\Release\OffsetDlg.obj :$(SRC)OffsetDlg.cpp  $(SRC)OffsetDlg.h $(SRC)StdAfx.h $(SRC)resource.h 
     $(CC) $(CFLAGSMT)   $(SRC)OffsetDlg.cpp
x64\Release\OverlayBufDlg.obj : $(SRC)OverlayBufDlg.cpp $(SRC)OverlayBufDlg.h $(SRC)StdAfx.h $(SRC)resource.h 
     $(CC) $(CFLAGSMT)   $(SRC)OverlayBufDlg.cpp
x64\Release\Registry.obj :$(SRC)Registry.cpp $(SRC)Registry.h $(SRC)StdAfx.h $(SRC)resource.h 
     $(CC) $(CFLAGSMT)   $(SRC)Registry.cpp
x64\Release\SettingsDlg.obj :$(SRC)SettingsDlg.cpp  $(SRC)SettingsDlg.h $(SRC)StdAfx.h $(SRC)resource.h 
     $(CC) $(CFLAGSMT)   $(SRC)SettingsDlg.cpp
x64\Release\SnoopConfig.obj :$(SRC)SnoopConfig.cpp  $(SRC)SnoopConfig.h $(SRC)StdAfx.h $(SRC)resource.h 
  $(CC) $(CFLAGSMT)   $(SRC)SnoopConfig.cpp

x64\Release\TermsDlg.obj  : $(SRC)TermsDlg.cpp $(SRC)TermsDlg.h $(SRC)StdAfx.h $(SRC)resource.h 
     $(CC) $(CFLAGSMT)   $(SRC)TermsDlg.cpp
x64\Release\UpdateAvailDlg.obj : $(SRC)UpdateAvailDlg.cpp $(SRC)UpdateAvailDlg.h $(SRC)StdAfx.h $(SRC)resource.h 
     $(CC) $(CFLAGSMT)   $(SRC)UpdateAvailDlg.cpp
x64\Release\UrlString.obj : $(SRC)UrlString.cpp $(SRC)UrlString.h $(SRC)StdAfx.h $(SRC)resource.h 
     $(CC) $(CFLAGSMT)   $(SRC)UrlString.cpp
x64\Release\WindowBuf.obj : $(SRC)WindowBuf.cpp $(SRC)WindowBuf.h $(SRC)StdAfx.h $(SRC)resource.h 
     $(CC) $(CFLAGSMT)   $(SRC)WindowBuf.cpp
x64\Release\JPEGsnoop.res : $(SRC)JPEGsnoop.rc $(SRC)resource.h res\JPEGsnoop.rc2 
	$(RC) $(RCVARS) $(SRC)JPEGsnoop.rc




