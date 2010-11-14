===============================================================================
                             JPEGsnoop - README
                               by Calvin Hass
            http://www.impulseadventure.com/photo/jpeg-snoop.html
===============================================================================

JPEGsnoop is a JPEG decoding and analysis utility written for Windows.
It reports all metadata contained within an image in addition to
the unique compression signatures that can be used to identify edited images.

JPEGsnoop is a portable application that does not require installation.


Contents:
1) Files
2) Acknowledgements


-------------------
1) FILES
-------------------

JPEGsnoop is written in Microsoft Visual C++ 2003 using MFC.

The project is a SDI (Single Document Interface) with Document-View architecture.


Files are arranged as follows:


MFC User Interface
------------------
  JPEGsnoopDoc.*		- SDI Document class.
  JPEGsnoopView.*		- SDI View class (for top split window / textual output)
  JPEGsnoopViewImg.*	- SDI View class (for bottom split window / image output)
  MainFrm.*				- SDI Frame class. Contains status bar, toolbar and
                          code to support the splitter window.

Global Definitions
------------------
  snoop.h				- Global defines and debugging options. Primarily
						  used to denote current release version number.

Application
-----------
  CntrItem.*			- OLE container / RichEdit class
  DbSigs.*				- Compression signature database class
! Dib.*					- DIB (Bitmap) class
  DocLog.*				- Document logging helper routines
  FileTiff.*			- TIFF export routines
  General.*
! HyperlinkStatic.*		- Hyperlink class for dialog box static controls
  ImgDecode.*			- Image Decoder (for Scan segment)
  JfifDecode.*			- JFIF Parser
  JPEGsnoop.*
! Md5.*					- MD5 hash routines, used for compression signature
! Registry.*			- Windows Registry class 
  SnoopConfig.*			- Application configuration class
  stdafx.*				- Windows precompiled headers (auto-created)
! UrlString.*			- URL En/Decoding class
  WindowBuf.*			- File buffer / cache routines

UNUSED:
! CmdLine.*				- Command-line processing
! OperationDlg.*		- Progress dialog with cancel


NOTE: Files marked with "!" denote classes that have reused code from 
      other authors. Please see the acknowledgements section.
      

Supporting Dialog Boxes
-----------------------
  AboutDlg.*			- About dialog. Indicates version number and link to
						  website with updated releases.
  DbManageDlg.*			- Local signature database management dialog. This
						  dialog allows the user to view and delete the compression
						  signatures that have been saved locally.
  DbSubmitDlg.*			- Signature database submit dialog. This dialog provides an
						  opportunity for the user to describe a few characteristics
						  of the image signature that will be sent to the database.
  DecodeDetailDlg.*		- Detailed decode dialog. Asks the user for a starting MCU
						  coordinate and length to begin detailed decoding.
  ExportDlg.*			- Export JPEG dialog. Provides a few options in the JPEG
						  export process.
  ExportTiffDlg.*		- Export TIFF dialog. Provides a few options in the TIFF
						  export process.
  FolderDlg.*			- Generic folder select dialog.
  LookupDlg.*			- Lookup file offset from MCU dialog. User inputs MCU coordinate
						  and lookup returns file offset within scan segment.
  ModelessDlg.*			- Generic modeless dialog.
  NoteDlg.*				- Generic note dialog. This dialog is used to inform the
						  user of runtime messages or warnings.
  OffsetDlg.*			- File offset dialog. Provides the user with the ability to
						  specify a starting file offset for parsing/decoding.
  OperationDlg.*		- Progress dialog for lengthy operations.
  OverlayBufDlg.*		- File overlay dialog. This dialog presents the user with
						  the current hex values at a specific position within the file
						  and provides the user with the ability to overwrite one or
						  more bytes with new hex values.
  SettingsDlg.*			- Configuration settings dialog. Provides all major configuration
						  options to the user. The settings are saved in the registry.
  TermsDlg.*			- Terms and conditions dialog. The dialog presents the user with
						  the EULA and records whether the user accepted it.
  UpdateAvailDlg.*		- Release update available dialog. Informs the user of the
						  existence of a newer version of JPEGsnoop and offers the
						  option of launching a browser window to download the latest
						  version.


Compression Signature Database
------------------------------
  Signatures.inl		- The current built-in compression signatures database.
						  This file is currently generated from a PHP script
						  residing on the ImpulseAdventure.com website that
						  fetches the latest compression signatures from a
						  MySQL database.



-------------------
2) ACKNOWLEDGEMENTS
-------------------

I am very grateful to the following authors whose classes or concepts I have reused in
the development of JPEGsnoop.

  CmdLine.cpp
  - *** NOT USED CURRENTLY
  - SmallerAnimals

  FolderDlg.cpp
  - MSDN Magazine

  Dib.cpp
  - MSDN?

  HyperlinkStatic.cpp
  - http://www.codeguru.com/cpp/controls/staticctrl/article.php/c5801
  - Franz Wong (http://www.codeguru.com/member.php/2259/)

  MD5.cpp
  - By RSA

  OperationDlg.cpp
  - *** NOT USED CURRENTLY
  - http://www.codeproject.com/KB/threads/TemplatedLengthyOperation.aspx
  - Mike O'Neill

  Registry.cpp
  - http://www.codeproject.com/KB/system/registryvars.aspx
  - Steve King
  - License: CPOL "Code Project Open License" http://www.codeproject.com/info/cpol10.aspx

  UrlString.cpp
  - Stephane Erhardt
  - http://www.codeguru.com/cpp/cpp/cpp_mfc/comments.php/c4029/?thread=9533

