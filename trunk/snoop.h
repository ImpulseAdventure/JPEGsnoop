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

// ==========================================================================
// CLASS DESCRIPTION:
// - JPEGsnoop global definitions
//
// ==========================================================================


#pragma once


// Program Version / Release Number
// - Format: "X.Y.Z"          - (full public release)
// - Format: "X.Y.Z (beta W)" - (beta release)
// - Note that when the version number is incremented, I need
//   to also update the corresponding version numbers in the
//   JPEGsnoop.rc resource under "Version.VS_VERSION_INFO".
#define VERSION_STR _T("1.7.5")

// Version number for the database signatures
// - This version number has been provided in case I decide
//   to change the database format. If a change to the format is
//   required, then I can use this version number to help build a
//   converter/import routine.
#define DB_SIG_VER 0x01

// Enable debug log?
//#define DEBUG_LOG_OUT
#define DEBUG_EN		0

//#define DEBUG_YCCK

// Disable code that isn't fully implemented yet
//#define TODO

// Enable Photoshop image/layer rendering
// TODO: Not fully implemented yet
#define PS_IMG_DEC_EN

// User Local Database filename
#define DAT_FILE _T("JPEGsnoop_db.dat")

// In QUICKLOG mode, we support batching up of writes to the
// RichEdit window, which greatly speeds up the output
// TODO: Might also use this for command-line nogui mode
#define QUICKLOG

// Define the type of Internet support
// - WINHTTP is only supported in newer OS versions
#define WWW_WININET
//#define WWW_WINHTTP

// The following DEFINEs are used in debugging and local batch operations
//#define DEBUG_SIG				// Print debugging info for signature generation
//#define BATCH_DO_DBSUBMIT
//#define BATCH_DO_DBSUBMIT_ALL


// Internet access parameters
// - If the user has enabled sharing of the signature data with the online
//   repository, then the following host and URLs are used.
// - The URLs that are used to indicate new release availability are also
//   provided here. These pages have specific formatting that JPEGsnoop
//   parses to identify if a newer version is available.
#define IA_HOST				_T("www.impulseadventure.com")
#define IA_DB_SUBMIT_PAGE	_T("/photo/jpeg-snoop-submit.php")
#define IA_UPDATES_CHK_PAGE	_T("/photo/jpeg-snoop.html")
#define IA_UPDATES_DL_PAGE	_T("http://www.impulseadventure.com/photo/jpeg-snoop.html#download")

// Registry settings
#define REG_KEY_PATH		_T("Software\\ImpulseAdventure\\JPEGsnoop\\")
#define REG_COMPANY_NAME	_T("ImpulseAdventure")
#define REG_SW_NAME			_T("JPEGsnoop")

// Extra logging for debugging
#define DEBUG_LOG

// For errors (eg. in exceptions), define message buffer length
#define	MAX_BUF_EX_ERR_MSG	512

// Preview Modes
enum tePreviewMode {
	PREVIEW_NONE=0,			// Preview not calculated yet
	PREVIEW_RGB,
	PREVIEW_YCC,
	PREVIEW_R,
	PREVIEW_G,
	PREVIEW_B,
	PREVIEW_Y,
	PREVIEW_CB,
	PREVIEW_CR,
};

enum tePreviewZoom {
	PRV_ZOOMBEGIN=0,
	PRV_ZOOM_12,
	PRV_ZOOM_25,
	PRV_ZOOM_50,
	PRV_ZOOM_100,
	PRV_ZOOM_150,
	PRV_ZOOM_200,
	PRV_ZOOM_300,
	PRV_ZOOM_400,
	PRV_ZOOM_800,
	PRV_ZOOMEND,
};

// DB Signature modes
enum teSource {
	ENUM_SOURCE_UNSET=0,
	ENUM_SOURCE_CAM,
	ENUM_SOURCE_SW,
	ENUM_SOURCE_UNSURE,
};

enum teMaker {
	ENUM_MAKER_UNSET=0,
	ENUM_MAKER_PRESENT,
	ENUM_MAKER_NONE,
};

enum teEdited {
	EDITED_UNSET=0,
	EDITED_YES,
	EDITED_NO,
	EDITED_UNSURE,
	EDITED_YESPROB,	// Probably edited
};

enum teDbAdd {
	DB_ADD_SUGGEST_UNSET=0,
	DB_ADD_SUGGEST_CAM,
	DB_ADD_SUGGEST_SW,
};

enum teEditor {
	ENUM_EDITOR_UNSET=0,
	ENUM_EDITOR_CAM,
	ENUM_EDITOR_SW,
	ENUM_EDITOR_UNSURE,
};

enum teLandscape {
	ENUM_LANDSCAPE_UNSET=0,
	ENUM_LANDSCAPE_YES,
	ENUM_LANDSCAPE_NO,
};

typedef enum {DEC_OFFSET_START,DEC_OFFSET_SRCH1,DEC_OFFSET_SRCH2,DEC_OFFSET_POS} teOffsetMode;

// Define a few coach messages

#define COACH_REPROCESS_AUTO _T("You have changed a processing option. To see these changes, ")\
	_T("you need to Reprocess the file or enable [Auto Reprocessing] in Configuration.")

#define COACH_DECODE_IDCT_DC _T("Currently only decoding low-res view (DC-only). Full-resolution image decode ")\
	_T("can be enabled in [Options->Scan Segment->Full IDCT], but it is slower.")
#define COACH_DECODE_IDCT_AC _T("Currently decoding high-res view (AC+DC), which can be slow. For faster ")\
	_T("operation, low-resolution image decode can be enabled in [Options->Scan Segment->No IDCT].")

