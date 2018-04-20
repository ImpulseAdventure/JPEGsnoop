CC=cl
CFLAGS=-c -DSTRICT -G3 -Ow -W3 -Zp -Tp
CFLAGSMT=-c /D_WIN64 /D_AFXDLL /MD /nologo /W3 /WX- /O2 /Oi /Ot /Oy- /DWIN32 /D_WINDOWS /DNDEBUG /D_UNICODE /DUNICODE /Gm- /GS- /Gy /arch:SSE2 /fp:precise /Zc:wchar_t /Zc:forScope /Fax64\Release\ /Fox64\Release\ /Fdx64\Release\vc100.pdb /Gd /errorReport:none 
LINKER=link
GUIFLAGS=-SUBSYSTEM:windows /OUT:x64\Release\JPEGsnoop.exe /NOLOGO  /PDB:"x64\Release\JPEGsnoop.pdb" /SUBSYSTEM:WINDOWS /OPT:REF /OPT:NOICF /TLBID:1 /DYNAMICBASE /NXCOMPAT /MACHINE:X64 /ERRORREPORT:NONE /ENTRY:wWinMainCRTStartup "/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'"
DLLFLAGS=-SUBSYSTEM:windows -DLL
GUILIBS=Wininet.lib
RC=rc
RCVARS=/DWIN32 /D_WIN64  /DNDEBUG /D_UNICODE /DUNICODE /D_AFXDLL   /l0x0409 /Ix64\Release\ /nologo /fox64\Release\JPEGsnoop.res
MT=mt
MTSTUFF=/nologo /verbose  -manifest x64\Release\JPEGsnoop.exe.manifest

docks : trail x64\Release\JPEGsnoop.exe
	$(MT) $(MTSTUFF) -outputresource:x64\Release\JPEGsnoop.exe

x64\Release\JPEGsnoop.exe : x64\Release\JPEGsnoop.obj x64\Release\JPEGsnoopCore.obj  x64\Release\MainFrm.obj x64\Release\AboutDlg.obj x64\Release\BatchDlg.obj x64\Release\CntrItem.obj x64\Release\DbManageDlg.obj x64\Release\DbSigs.obj x64\Release\DbSubmitDlg.obj x64\Release\DecodeDetailDlg.obj x64\Release\Dib.obj x64\Release\DocLog.obj x64\Release\ExportDlg.obj x64\Release\ExportTiffDlg.obj x64\Release\FileTiff.obj x64\Release\FolderDlg.obj x64\Release\General.obj x64\Release\HyperlinkStatic.obj x64\Release\ImgDecode.obj x64\Release\JfifDecode.obj x64\Release\JPEGsnoopDoc.obj x64\Release\JPEGsnoopView.obj x64\Release\JPEGsnoopViewImg.obj x64\Release\LookupDlg.obj x64\Release\Md5.obj x64\Release\ModelessDlg.obj x64\Release\NoteDlg.obj x64\Release\OffsetDlg.obj x64\Release\OverlayBufDlg.obj x64\Release\Registry.obj x64\Release\SettingsDlg.obj x64\Release\SnoopConfig.obj  x64\Release\TermsDlg.obj x64\Release\UpdateAvailDlg.obj x64\Release\UrlString.obj x64\Release\WindowBuf.obj x64\Release\DecodePs.obj x64\Release\DecodeDicomTags.obj x64\Release\DecodeDicom.obj x64\Release\JPEGsnoop.res
    $(LINKER) $(GUIFLAGS) x64\Release\JPEGsnoop.obj x64\Release\JPEGsnoopCore.obj x64\Release\MainFrm.obj x64\Release\AboutDlg.obj x64\Release\BatchDlg.obj x64\Release\CntrItem.obj x64\Release\DbManageDlg.obj x64\Release\DbSigs.obj x64\Release\DbSubmitDlg.obj x64\Release\DecodeDetailDlg.obj x64\Release\Dib.obj x64\Release\DocLog.obj x64\Release\ExportDlg.obj x64\Release\ExportTiffDlg.obj x64\Release\FileTiff.obj x64\Release\FolderDlg.obj x64\Release\General.obj x64\Release\HyperlinkStatic.obj x64\Release\ImgDecode.obj x64\Release\JfifDecode.obj x64\Release\JPEGsnoopDoc.obj x64\Release\JPEGsnoopView.obj x64\Release\JPEGsnoopViewImg.obj x64\Release\LookupDlg.obj x64\Release\Md5.obj x64\Release\ModelessDlg.obj x64\Release\NoteDlg.obj x64\Release\OffsetDlg.obj x64\Release\OverlayBufDlg.obj x64\Release\Registry.obj x64\Release\SettingsDlg.obj x64\Release\SnoopConfig.obj x64\Release\TermsDlg.obj x64\Release\UpdateAvailDlg.obj x64\Release\UrlString.obj x64\Release\WindowBuf.obj  x64\Release\DecodePs.obj x64\Release\DecodeDicom.obj x64\Release\DecodeDicomTags.obj x64\Release\JPEGsnoop.res $(GUILIBS)
 
trail:
	-@ if NOT EXIST "x64" mkdir "x64"
	-@ if NOT EXIST "x64\Release" mkdir "x64\Release"

x64\Release\JPEGsnoop.obj : JPEGsnoop.cpp JPEGsnoop.h JPEGsnoopDoc.h NoteDlg.h HyperlinkStatic.h ModelessDlg.h SettingsDlg.h UpdateAvailDlg.h JPEGsnoopView.h TermsDlg.h DbManageDlg.h StdAfx.h DbSubmitDlg.h snoop.h SnoopConfig.h resource.h MainFrm.h
	 $(CC) $(CFLAGSMT) JPEGsnoop.cpp
x64\Release\JPEGsnoopCore.obj : JPEGsnoopCore.cpp JPEGsnoopCore.h JPEGsnoop.h
	 $(CC) $(CFLAGSMT) JPEGsnoopCore.cpp

x64\Release\MainFrm.obj : MainFrm.cpp MainFrm.h StdAfx.h resource.h 
     $(CC) $(CFLAGSMT) MainFrm.cpp

x64\Release\AboutDlg.obj : AboutDlg.cpp AboutDlg.h StdAfx.h resource.h 
     $(CC) $(CFLAGSMT)   AboutDlg.cpp
 
x64\Release\BatchDlg.obj : BatchDlg.cpp BatchDlg.h StdAfx.h resource.h 
     $(CC) $(CFLAGSMT)  BatchDlg.cpp
 
x64\Release\CntrItem.obj : CntrItem.cpp CntrItem.h StdAfx.h resource.h 
     $(CC) $(CFLAGSMT)   CntrItem.cpp

x64\Release\DbManageDlg.obj : DbManageDlg.cpp DbManageDlg.h StdAfx.h resource.h 
     $(CC) $(CFLAGSMT)   DbManageDlg.cpp

x64\Release\DbSigs.obj : DbSigs.cpp DbSigs.h StdAfx.h resource.h 
     $(CC) $(CFLAGSMT)   DbSigs.cpp

x64\Release\DbSubmitDlg.obj :DbSubmitDlg.cpp  DbSubmitDlg.h StdAfx.h resource.h 
     $(CC) $(CFLAGSMT)  DbSubmitDlg.cpp
 
x64\Release\DecodeDetailDlg.obj :DecodeDetailDlg.cpp  DecodeDetailDlg.h StdAfx.h resource.h 
     $(CC) $(CFLAGSMT)  DecodeDetailDlg.cpp
x64\Release\DecodeDicom.obj :DecodeDicom.cpp  DecodeDicom.h StdAfx.h resource.h 
     $(CC) $(CFLAGSMT)  DecodeDicom.cpp
x64\Release\DecodeDicomTags.obj :DecodeDicomTags.cpp StdAfx.h resource.h 
     $(CC) $(CFLAGSMT)  DecodeDicomTags.cpp
x64\Release\DecodePs.obj :DecodePs.cpp DecodePs.h StdAfx.h resource.h 
     $(CC) $(CFLAGSMT)  DecodePs.cpp

x64\Release\Dib.obj : Dib.cpp Dib.h StdAfx.h resource.h 
     $(CC) $(CFLAGSMT)  Dib.cpp
 
x64\Release\DocLog.obj :DocLog.cpp  DocLog.h StdAfx.h resource.h 
     $(CC) $(CFLAGSMT)  DocLog.cpp
 
x64\Release\ExportDlg.obj :ExportDlg.cpp  ExportDlg.h StdAfx.h resource.h 
     $(CC) $(CFLAGSMT)  ExportDlg.cpp
 
x64\Release\ExportTiffDlg.obj :ExportTiffDlg.cpp  ExportTiffDlg.h StdAfx.h resource.h 
     $(CC) $(CFLAGSMT)   ExportTiffDlg.cpp

x64\Release\FileTiff.obj : FileTiff.cpp FileTiff.h StdAfx.h resource.h 
     $(CC) $(CFLAGSMT)  FileTiff.cpp
 
x64\Release\FolderDlg.obj :FolderDlg.cpp  FolderDlg.h StdAfx.h resource.h 
     $(CC) $(CFLAGSMT)   FolderDlg.cpp

x64\Release\General.obj : General.cpp General.h StdAfx.h resource.h 
     $(CC) $(CFLAGSMT)  General.cpp
 
x64\Release\HyperlinkStatic.obj :HyperlinkStatic.cpp  HyperlinkStatic.h StdAfx.h resource.h 
     $(CC) $(CFLAGSMT)   HyperlinkStatic.cpp

x64\Release\ImgDecode.obj : ImgDecode.cpp ImgDecode.h StdAfx.h resource.h 
     $(CC) $(CFLAGSMT)   ImgDecode.cpp

x64\Release\JfifDecode.obj : JfifDecode.cpp JfifDecode.h StdAfx.h resource.h 
     $(CC) $(CFLAGSMT)   JfifDecode.cpp
x64\Release\JPEGsnoopDoc.obj :JPEGsnoopDoc.cpp  JPEGsnoopDoc.h StdAfx.h resource.h 
    $(CC) $(CFLAGSMT)   JPEGsnoopDoc.cpp
x64\Release\JPEGsnoopView.obj : JPEGsnoopView.cpp JPEGsnoopView.h StdAfx.h resource.h 
    $(CC) $(CFLAGSMT)   JPEGsnoopView.cpp
x64\Release\JPEGsnoopViewImg.obj : JPEGsnoopViewImg.cpp JPEGsnoopViewImg.h StdAfx.h resource.h 
    $(CC) $(CFLAGSMT)   JPEGsnoopViewImg.cpp
x64\Release\LookupDlg.obj : LookupDlg.cpp LookupDlg.h StdAfx.h resource.h 
    $(CC) $(CFLAGSMT)  LookupDlg.cpp
x64\Release\Md5.obj :Md5.cpp  Md5.h StdAfx.h resource.h 
     $(CC) $(CFLAGSMT)  Md5.cpp
x64\Release\ModelessDlg.obj :ModelessDlg.cpp  ModelessDlg.h StdAfx.h resource.h 
     $(CC) $(CFLAGSMT)   ModelessDlg.cpp
x64\Release\NoteDlg.obj : NoteDlg.cpp NoteDlg.h StdAfx.h resource.h 
     $(CC) $(CFLAGSMT)   NoteDlg.cpp
x64\Release\OffsetDlg.obj :OffsetDlg.cpp  OffsetDlg.h StdAfx.h resource.h 
     $(CC) $(CFLAGSMT)   OffsetDlg.cpp
x64\Release\OverlayBufDlg.obj : OverlayBufDlg.cpp OverlayBufDlg.h StdAfx.h resource.h 
     $(CC) $(CFLAGSMT)   OverlayBufDlg.cpp
x64\Release\Registry.obj :Registry.cpp Registry.h StdAfx.h resource.h 
     $(CC) $(CFLAGSMT)   Registry.cpp
x64\Release\SettingsDlg.obj :SettingsDlg.cpp  SettingsDlg.h StdAfx.h resource.h 
     $(CC) $(CFLAGSMT)   SettingsDlg.cpp
x64\Release\SnoopConfig.obj :SnoopConfig.cpp  SnoopConfig.h StdAfx.h resource.h 
  $(CC) $(CFLAGSMT)   SnoopConfig.cpp

x64\Release\TermsDlg.obj  : TermsDlg.cpp TermsDlg.h StdAfx.h resource.h 
     $(CC) $(CFLAGSMT)   TermsDlg.cpp
x64\Release\UpdateAvailDlg.obj : UpdateAvailDlg.cpp UpdateAvailDlg.h StdAfx.h resource.h 
     $(CC) $(CFLAGSMT)   UpdateAvailDlg.cpp
x64\Release\UrlString.obj : UrlString.cpp UrlString.h StdAfx.h resource.h 
     $(CC) $(CFLAGSMT)   UrlString.cpp
x64\Release\WindowBuf.obj : WindowBuf.cpp WindowBuf.h StdAfx.h resource.h 
     $(CC) $(CFLAGSMT)   WindowBuf.cpp
x64\Release\JPEGsnoop.res : JPEGsnoop.rc resource.h res\JPEGsnoop.rc2 
	$(RC) $(RCVARS) JPEGsnoop.rc




