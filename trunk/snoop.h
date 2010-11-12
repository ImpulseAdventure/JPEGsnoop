// JPEGsnoop - JPEG Image Decoder & Analysis Utility
// Copyright (C) 2010 - Calvin Hass
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

#pragma once


// Program Version / Release Number
// - Format: "X.Y.Z"          - (full public release)
// - Format: "X.Y.Z (beta W)" - (beta release)
// - Note that when the version number is incremented, I need
//   to also update the corresponding version numbers in the
//   JPEGsnoop.rc resource under "Version.VS_VERSION_INFO".
#define VERSION_STR "1.5.1"

// Version number for the database signatures
// - This version number has been provided in case I decide
//   to change the database format. If a change to the format is
//   required, then I can use this version number to help build a
//   converter/import routine.
#define DB_SIG_VER 0x01


// User Local Database filename
#define DAT_FILE "JPEGsnoop_db.dat"

// In QUICKLOG mode, we support batching up of writes to the
// RichEdit window, which greatly speeds up the output
#define QUICKLOG

// Define the type of Internet support
// - WINHTTP is only supported in newer OS versions
#define WWW_WININET
//#define WWW_WINHTTP

// The following DEFINEs are used in debugging and local batch operations
//#define DEBUG_SIG		// Print debugging info for signature generation
//#define BATCH_DO_RECURSE
//#define BATCH_DO_DBSUBMIT
//#define BATCH_DO_DBSUBMIT_ALL


// Internet access parameters
// - If the user has enabled sharing of the signature data with the online
//   repository, then the following host and URLs are used.
// - The URLs that are used to indicate new release availability are also
//   provided here. These pages have specific formatting that JPEGsnoop
//   parses to identify if a newer version is available.
#define IA_HOST "www.impulseadventure.com"
#define IA_DB_SUBMIT_PAGE "/photo/jpeg-snoop-submit.php"
#define IA_UPDATES_CHK_PAGE "/photo/jpeg-snoop.html"
#define IA_UPDATES_DL_PAGE "http://www.impulseadventure.com/photo/jpeg-snoop.html#download"

// Registry settings
#define REG_KEY_PATH		"Software\\ImpulseAdventure\\JPEGsnoop\\"
#define REG_COMPANY_NAME	"ImpulseAdventure"
#define REG_SW_NAME			"JPEGsnoop"




// Version History
// - Moved to separate file


// Preview Modes
enum preview_mode_enum {
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

#define PRV_ZOOMBEGIN 0
#define PRV_ZOOM_12 1
#define PRV_ZOOM_25 2
#define PRV_ZOOM_50 3
#define PRV_ZOOM_100 4
#define PRV_ZOOM_150 5
#define PRV_ZOOM_200 6
#define PRV_ZOOM_300 7
#define PRV_ZOOM_400 8
#define PRV_ZOOM_800 9
#define PRV_ZOOMEND 10


#define ENUM_SOURCE_UNSET 0
#define ENUM_SOURCE_CAM 1
#define ENUM_SOURCE_SW 2
#define ENUM_SOURCE_UNSURE 3

#define ENUM_MAKER_UNSET 0
#define ENUM_MAKER_PRESENT 1
#define ENUM_MAKER_NONE 2

#define EDITED_UNSET 0
#define EDITED_YES 1
#define EDITED_NO 2
#define EDITED_UNSURE 3
#define EDITED_YESPROB 4	// Probably edited

#define DB_ADD_SUGGEST_UNSET 0
#define DB_ADD_SUGGEST_CAM 1
#define DB_ADD_SUGGEST_SW 2

#define ENUM_EDITOR_UNSET 0
#define ENUM_EDITOR_CAM 1
#define ENUM_EDITOR_SW 2
#define ENUM_EDITOR_UNSURE 3

#define ENUM_LANDSCAPE_UNSET 0
#define ENUM_LANDSCAPE_YES 1
#define ENUM_LANDSCAPE_NO 2


#define COACH_REPROCESS_AUTO "You have changed a processing option. To see these changes, "\
	"you need to Reprocess the file or enable [Auto Reprocessing] in Configuration."

#define COACH_DECODE_IDCT_DC "Currently only decoding low-res view (DC-only). Full-resolution image decode "\
	"can be enabled in [Options->Scan Segment->Full IDCT], but it is slower."
#define COACH_DECODE_IDCT_AC "Currently decoding high-res view (AC+DC), which can be slow. For faster "\
	"operation, low-resolution image decode can be enabled in [Options->Scan Segment->No IDCT]."

