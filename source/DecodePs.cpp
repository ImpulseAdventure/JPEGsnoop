// JPEGsnoop - JPEG Image Decoder & Analysis Utility
// Copyright (C) 2018 - Calvin Hass
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

#include <QDebug>

#include<cwchar>

#include "DecodePs.h"
#include "snoop.h"
//#include "JPEGsnoop.h"          // for m_pAppConfig get
#include "SnoopConfig.h"

#include "WindowBuf.h"

// Forward declarations
//struct tsIptcField    asIptcFields[];
//struct tsBimRecord    asBimRecords[];
//struct tsBimEnum      asBimEnums[];

// ===============================================================================
// CONSTANTS
// ===============================================================================

// Define all of the currently-known IPTC fields, used when we
// parse the APP13 marker
//
// - Reference: IPTC-NAA Information Interchange Model Version 4
// - See header for struct encoding
struct tsIptcField asIptcFields[] = {
  {1, 0, IPTC_T_NUM2, "Model Version"},
  {1, 5, IPTC_T_STR, "Destination"},
  {1, 20, IPTC_T_NUM2, "File Format"},
  {1, 22, IPTC_T_NUM2, "File Format Version"},
  {1, 30, IPTC_T_STR, "Service Identifier"},
  {1, 40, IPTC_T_STR, "Envelope Number"},
  {1, 50, IPTC_T_STR, "Product I.D."},
  {1, 60, IPTC_T_STR, "Envelope Priority"},
  {1, 70, IPTC_T_STR, "Date Sent"},
  {1, 80, IPTC_T_STR, "Time Sent"},
  {1, 90, IPTC_T_STR, "Coded Character Set"},
  {1, 100, IPTC_T_STR, "UNO"},
  {1, 120, IPTC_T_NUM2, "ARM Identifier"},
  {1, 122, IPTC_T_NUM2, "ARM Version"},
  {2, 0, IPTC_T_NUM2, "Record Version"},
  {2, 3, IPTC_T_STR, "Object Type Reference"},
  {2, 4, IPTC_T_STR, "Object Attrib Reference"},
  {2, 5, IPTC_T_STR, "Object Name"},
  {2, 7, IPTC_T_STR, "Edit Status"},
  {2, 8, IPTC_T_STR, "Editorial Update"},
  {2, 10, IPTC_T_STR, "Urgency"},
  {2, 12, IPTC_T_STR, "Subject Reference"},
  {2, 15, IPTC_T_STR, "Category"},
  {2, 20, IPTC_T_STR, "Supplemental Category"},
  {2, 22, IPTC_T_STR, "Fixture Identifier"},
  {2, 25, IPTC_T_STR, "Keywords"},
  {2, 26, IPTC_T_STR, "Content Location Code"},
  {2, 27, IPTC_T_STR, "Content Location Name"},
  {2, 30, IPTC_T_STR, "Release Date"},
  {2, 35, IPTC_T_STR, "Release Time"},
  {2, 37, IPTC_T_STR, "Expiration Date"},
  {2, 38, IPTC_T_STR, "Expiration Time"},
  {2, 40, IPTC_T_STR, "Special Instructions"},
  {2, 42, IPTC_T_STR, "Action Advised"},
  {2, 45, IPTC_T_UNK, "Reference Service"},
  {2, 47, IPTC_T_UNK, "Reference Date"},
  {2, 50, IPTC_T_UNK, "Reference Number"},
  {2, 55, IPTC_T_STR, "Date Created"},
  {2, 60, IPTC_T_STR, "Time Created"},
  {2, 62, IPTC_T_STR, "Digital Creation Date"},
  {2, 63, IPTC_T_STR, "Digital Creation Time"},
  {2, 65, IPTC_T_STR, "Originating Program"},
  {2, 70, IPTC_T_STR, "Program Version"},
  {2, 75, IPTC_T_STR, "Object Cycle"},
  {2, 80, IPTC_T_STR, "By-line"},
  {2, 85, IPTC_T_STR, "By-line Title"},
  {2, 90, IPTC_T_STR, "City"},
  {2, 92, IPTC_T_STR, "Sub-location"},
  {2, 95, IPTC_T_STR, "Province/State"},
  {2, 100, IPTC_T_STR, "Country/Primary Location Code"},
  {2, 101, IPTC_T_STR, "Country/Primary Location Name"},
  {2, 103, IPTC_T_STR, "Original Transmission Reference"},
  {2, 105, IPTC_T_STR, "Headline"},
  {2, 110, IPTC_T_STR, "Credit"},
  {2, 115, IPTC_T_STR, "Source"},
  {2, 116, IPTC_T_STR, "Copyright Notice"},
  {2, 118, IPTC_T_STR, "Contact"},
  {2, 120, IPTC_T_STR, "Caption/Abstract"},
  {2, 122, IPTC_T_STR, "Writer/Editor"},
  {2, 125, IPTC_T_UNK, "Rasterized Caption"},
  {2, 130, IPTC_T_STR, "Image Type"},
  {2, 131, IPTC_T_STR, "Image Orientation"},
  {2, 135, IPTC_T_STR, "Language Identifier"},
  {2, 150, IPTC_T_STR, "Audio Type"},
  {2, 151, IPTC_T_STR, "Audio Sampling Rate"},
  {2, 152, IPTC_T_STR, "Audio Sampling Resolution"},
  {2, 153, IPTC_T_STR, "Audio Duration"},
  {2, 154, IPTC_T_STR, "Audio Outcue"},
  {2, 200, IPTC_T_NUM2, "ObjectData Preview File Format"},
  {2, 201, IPTC_T_NUM2, "ObjectData Preview File Format Version"},
  {2, 202, IPTC_T_UNK, "ObjectData Preview Data"},
  {7, 10, IPTC_T_NUM1, "Size Mode"},
  {7, 20, IPTC_T_NUM, "Max Subfile Size"},
  {7, 90, IPTC_T_NUM1, "ObjectData Size Announced"},
  {7, 95, IPTC_T_NUM, "Maximum ObjectData Size"},
  {8, 10, IPTC_T_UNK, "Subfile"},
  {9, 10, IPTC_T_NUM, "Confirmed ObjectData Size"},
  {0, 0, IPTC_T_END, "DONE"},
};

// Adobe Photoshop File Formats Specification (October 2013)
// - Image Resource Blocks IRB (8BIM)
// - See header for struct encoding
struct tsBimRecord asBimRecords[] = {
  {0x03E8, 0x0000, BIM_T_UNK, "-"},
  {0x03E9, 0x0000, BIM_T_HEX, "Macintosh print manager print info record"},
  {0x03EB, 0x0000, BIM_T_HEX, "Indexed color table"},
  {0x03ED, 0x0000, BIM_T_PS_RESOLUTION_INFO, "ResolutionInfo structure"},
  {0x03EE, 0x0000, BIM_T_HEX, "Names of alpha channels"},
  {0x03EF, 0x0000, BIM_T_HEX, "DisplayInfo structure"},
  {0x03F0, 0x0000, BIM_T_HEX, "Caption"},
  {0x03F1, 0x0000, BIM_T_HEX, "Border information"},
  {0x03F2, 0x0000, BIM_T_HEX, "Background color"},
  {0x03F3, 0x0000, BIM_T_PS_PRINT_FLAGS, "Print flags"},
  {0x03F4, 0x0000, BIM_T_HEX, "Grayscale and multichannel halftoning information"},
  {0x03F5, 0x0000, BIM_T_HEX, "Color halftoning information"},
  {0x03F6, 0x0000, BIM_T_HEX, "Duotone halftoning information"},
  {0x03F7, 0x0000, BIM_T_HEX, "Grayscale and multichannel transfer function"},
  {0x03F8, 0x0000, BIM_T_HEX, "Color transfer functions"},
  {0x03F9, 0x0000, BIM_T_HEX, "Duotone transfer functions"},
  {0x03FA, 0x0000, BIM_T_HEX, "Duotone image information"},
  {0x03FC, 0x0000, BIM_T_HEX, "-"},
  {0x03FD, 0x0000, BIM_T_HEX, "EPS options"},
  {0x03FE, 0x0000, BIM_T_HEX, "Quick Mask information"},
  {0x03FF, 0x0000, BIM_T_HEX, "-"},
  {0x0400, 0x0000, BIM_T_PS_LAYER_STATE_INFO, "Layer state information"},
  {0x0401, 0x0000, BIM_T_HEX, "Working path"},
  {0x0402, 0x0000, BIM_T_PS_LAYER_GROUP_INFO, "Layers group information"},
  {0x0403, 0x0000, BIM_T_HEX, "-"},
  {0x0404, 0x0000, BIM_T_IPTC_NAA, "IPTC-NAA record"},
  {0x0405, 0x0000, BIM_T_HEX, "Image mode (raw format files)"},
  {0x0406, 0x0000, BIM_T_JPEG_QUAL, "JPEG quality"},
  {0x0408, 0x0000, BIM_T_PS_GRID_GUIDES, "Grid and guides information"},
  {0x0409, 0x0000, BIM_T_HEX, "Thumbnail resource (PS 4.0)"},
  {0x040A, 0x0000, BIM_T_PS_COPYRIGHT_FLAG, "Copyright flag"},
  {0x040B, 0x0000, BIM_T_HEX, "URL"},
  {0x040C, 0x0000, BIM_T_PS_THUMB_RES, "Thumbnail resources"},
  {0x040D, 0x0000, BIM_T_PS_GLOBAL_ANGLE, "Global Angle"},
  {0x040E, 0x0000, BIM_T_HEX, "Color samplers resource"},
  {0x040F, 0x0000, BIM_T_HEX, "ICC Profile"},
  {0x0410, 0x0000, BIM_T_HEX, "Watermark"},
  {0x0411, 0x0000, BIM_T_HEX, "ICC Untagged Profile"},
  {0x0412, 0x0000, BIM_T_HEX, "Effects visible"},
  {0x0413, 0x0000, BIM_T_HEX, "Spot Halftone"},
  {0x0414, 0x0000, BIM_T_PS_DOC_SPECIFIC_SEED, "Document-specific IDs seed number"},
  {0x0415, 0x0000, BIM_T_PS_STR_UNI, "Unicode Alpha Names"},
  {0x0416, 0x0000, BIM_T_HEX, "Indexed Color Table Count"},
  {0x0417, 0x0000, BIM_T_HEX, "Transparency Index"},
  {0x0419, 0x0000, BIM_T_PS_GLOBAL_ALTITUDE, "Global Altitude"},
  {0x041A, 0x0000, BIM_T_PS_SLICES, "Slices"},
  {0x041B, 0x0000, BIM_T_PS_STR_UNI, "Workflow URL"},
  {0x041C, 0x0000, BIM_T_HEX, "Jump to XPEP"},
  {0x041D, 0x0000, BIM_T_HEX, "Alpha Identifiers"},
  {0x041E, 0x0000, BIM_T_HEX, "URL List"},
  {0x0421, 0x0000, BIM_T_PS_VER_INFO, "Version Info"},
  {0x0422, 0x0000, BIM_T_HEX, "Exif data 1"},
  {0x0423, 0x0000, BIM_T_HEX, "Exif data 3"},
  {0x0424, 0x0000, BIM_T_PS_STR_ASC_LONG, "XMP metadata"},
  {0x0425, 0x0000, BIM_T_HEX, "Caption digest"},
  {0x0426, 0x0000, BIM_T_PS_PRINT_SCALE, "Print scale"},
  {0x0428, 0x0000, BIM_T_PS_PIXEL_ASPECT_RATIO, "Pixel Aspect Ratio"},
  {0x0429, 0x0000, BIM_T_HEX, "Layer Comps"},
  {0x042A, 0x0000, BIM_T_HEX, "Alternate Duotone Colors"},
  {0x042B, 0x0000, BIM_T_HEX, "Alternate Spot Colors"},
  {0x042D, 0x0000, BIM_T_PS_LAYER_SELECT_ID, "Layer Selection IDs"},
  {0x042E, 0x0000, BIM_T_HEX, "HDR Toning information"},
  {0x042F, 0x0000, BIM_T_HEX, "Print info"},
  {0x0430, 0x0000, BIM_T_PS_LAYER_GROUP_ENABLED, "Layer Groups Enabled ID"},
  {0x0431, 0x0000, BIM_T_HEX, "Color samplers resource"},
  {0x0432, 0x0000, BIM_T_HEX, "Measurement Scale"},
  {0x0433, 0x0000, BIM_T_HEX, "Timeline Information"},
  {0x0434, 0x0000, BIM_T_HEX, "Sheet Disclosure"},
  {0x0435, 0x0000, BIM_T_HEX, "DisplayInfo structure (FloatPt colors)"},
  {0x0436, 0x0000, BIM_T_HEX, "Onion Skins"},
  {0x0438, 0x0000, BIM_T_HEX, "Count Information"},
  {0x043A, 0x0000, BIM_T_HEX, "Print Information"},
  {0x043B, 0x0000, BIM_T_HEX, "Print Style"},
  {0x043C, 0x0000, BIM_T_HEX, "Macintosh NSPrintInfo"},
  {0x043D, 0x0000, BIM_T_HEX, "Windows DEVMODE"},
  {0x043E, 0x0000, BIM_T_HEX, "Auto Save File Path"},
  {0x043F, 0x0000, BIM_T_HEX, "Auto Save Format"},
  {0x0440, 0x0000, BIM_T_HEX, "Path Selection State"},
  {0x07D0, 0x0BB6, BIM_T_HEX, "Path Information (saved paths)"},
  {0x0BB7, 0x0000, BIM_T_HEX, "Name of clipping path"},
  {0x0BB8, 0x0000, BIM_T_HEX, "Origin Path Info"},
  {0x0FA0, 0x1387, BIM_T_HEX, "Plug-In resources"},
  {0x1B58, 0x0000, BIM_T_HEX, "Image Ready variables"},
  {0x1B59, 0x0000, BIM_T_HEX, "Image Ready data sets"},
  {0x1F40, 0x0000, BIM_T_HEX, "Lightroom workflow"},
  {0x2710, 0x0000, BIM_T_PS_PRINT_FLAGS_INFO, "Print flags information"},
  {0x0000, 0x0000, BIM_T_END, ""},
};

// Adobe Photoshop enumerated constants
// - See header for struct encoding
struct tsBimEnum asBimEnums[] = {
  {BIM_T_ENUM_FILE_HDR_COL_MODE, 0, "Bitmap"},
  {BIM_T_ENUM_FILE_HDR_COL_MODE, 1, "Grayscale"},
  {BIM_T_ENUM_FILE_HDR_COL_MODE, 2, "Indexed"},
  {BIM_T_ENUM_FILE_HDR_COL_MODE, 3, "RGB"},
  {BIM_T_ENUM_FILE_HDR_COL_MODE, 4, "CMYK"},
  {BIM_T_ENUM_FILE_HDR_COL_MODE, 7, "Multichannel"},
  {BIM_T_ENUM_FILE_HDR_COL_MODE, 8, "Duotone"},
  {BIM_T_ENUM_FILE_HDR_COL_MODE, 9, "Lab"},
  {BIM_T_ENUM_RESOLUTION_INFO_RES_UNIT, 1, "pixels per inch"},
  {BIM_T_ENUM_RESOLUTION_INFO_RES_UNIT, 2, "pixels per cm"},
  {BIM_T_ENUM_RESOLUTION_INFO_WIDTH_UNIT, 1, "inch"},
  {BIM_T_ENUM_RESOLUTION_INFO_WIDTH_UNIT, 2, "cm"},
  {BIM_T_ENUM_RESOLUTION_INFO_WIDTH_UNIT, 3, "picas"},
  {BIM_T_ENUM_RESOLUTION_INFO_WIDTH_UNIT, 4, "columns"},
  {BIM_T_ENUM_PRINT_SCALE_STYLE, 0, "centered"},
  {BIM_T_ENUM_PRINT_SCALE_STYLE, 1, "size to fit"},
  {BIM_T_ENUM_PRINT_SCALE_STYLE, 2, "user defined"},
  {BIM_T_ENUM_GRID_GUIDE_DIR, 0, "vertical"},
  {BIM_T_ENUM_GRID_GUIDE_DIR, 1, "horizontal"},
  {BIM_T_ENUM_BLEND_MODE_KEY, 'pass', "pass through"},
  {BIM_T_ENUM_BLEND_MODE_KEY, 'norm', "normal"},
  {BIM_T_ENUM_BLEND_MODE_KEY, 'diss', "dissolve"},
  {BIM_T_ENUM_BLEND_MODE_KEY, 'dark', "darken"},
  {BIM_T_ENUM_BLEND_MODE_KEY, 'mul ', "multiply"},
  {BIM_T_ENUM_BLEND_MODE_KEY, 'idiv', "color burn"},
  {BIM_T_ENUM_BLEND_MODE_KEY, 'lbrn', "linear burn"},
  {BIM_T_ENUM_BLEND_MODE_KEY, 'dkCl', "darker color"},
  {BIM_T_ENUM_BLEND_MODE_KEY, 'lite', "lighten"},
  {BIM_T_ENUM_BLEND_MODE_KEY, 'scrn', "screen"},
  {BIM_T_ENUM_BLEND_MODE_KEY, 'div ', "color dodge"},
  {BIM_T_ENUM_BLEND_MODE_KEY, 'lddg', "linear dodge"},
  {BIM_T_ENUM_BLEND_MODE_KEY, 'lgCl', "lighter color"},
  {BIM_T_ENUM_BLEND_MODE_KEY, 'over', "overlay"},
  {BIM_T_ENUM_BLEND_MODE_KEY, 'sLit', "soft light"},
  {BIM_T_ENUM_BLEND_MODE_KEY, 'hLit', "hard light"},
  {BIM_T_ENUM_BLEND_MODE_KEY, 'vLit', "vivid light"},
  {BIM_T_ENUM_BLEND_MODE_KEY, 'lLit', "linear light"},
  {BIM_T_ENUM_BLEND_MODE_KEY, 'pLit', "pin light"},
  {BIM_T_ENUM_BLEND_MODE_KEY, 'hMix', "hard mix"},
  {BIM_T_ENUM_BLEND_MODE_KEY, 'diff', "difference"},
  {BIM_T_ENUM_BLEND_MODE_KEY, 'smud', "exclusion"},
  {BIM_T_ENUM_BLEND_MODE_KEY, 'fsub', "subtract"},
  {BIM_T_ENUM_BLEND_MODE_KEY, 'fdiv', "divide"},
  {BIM_T_ENUM_BLEND_MODE_KEY, 'hue ', "hue"},
  {BIM_T_ENUM_BLEND_MODE_KEY, 'sat ', "saturation"},
  {BIM_T_ENUM_BLEND_MODE_KEY, 'colr', "color"},
  {BIM_T_ENUM_BLEND_MODE_KEY, 'lum ', "luminosity"},
  {BIM_T_ENUM_END, 0, "???"},
};

CDecodePs::CDecodePs(CwindowBuf *pWBuf, CDocLog *pLog, CSnoopConfig *pAppConfig) : m_pWBuf(pWBuf), m_pLog(pLog), m_pAppConfig(pAppConfig)
{
  // HACK
  // Only select a single layer to decode into the DIB
  // FIXME: Need to allow user control over this setting
  m_bDisplayLayer = false;
  m_nDisplayLayerInd = 0;
  m_bDisplayImage = true;

  Reset();
}

void CDecodePs::Reset()
{
  m_bPsd = false;
  m_nQualitySaveAs = 0;
  m_nQualitySaveForWeb = 0;
  m_bAbort = false;
}

CDecodePs::~CDecodePs(void)
{
}

// Provide a short-hand alias for the m_pWBuf buffer
// INPUT:
// - offset                                     File offset to read from
// - bClean                                     Forcibly disables any redirection to Fake DHT table
//
// POST:
// - m_pLog
//
// RETURN:
// - Byte from file
//
uint8_t CDecodePs::Buf(uint32_t offset, bool bClean = false)
{
  return m_pWBuf->Buf(offset, bClean);
}

// Determine if the file is a Photoshop PSD
// If so, parse the headers. Generally want to start at start of file (nPos=0).
bool CDecodePs::DecodePsd(uint32_t nPos, QImage * pDibTemp, int32_t &nWidth, int32_t &nHeight)
{
  QString strTmp;
  QString strSig;

  m_bPsd = false;

  uint32_t nVer;

  strSig = m_pWBuf->BufReadStrn(nPos, 4);
  nPos += 4;
  nVer = m_pWBuf->BufRdAdv2(nPos, PS_BSWAP);

  if((strSig == "8BPS") && (nVer == 1))
  {
    m_bPsd = true;
    m_pLog->AddLine("");
    m_pLog->AddLineHdr("*** Photoshop PSD File Decoding ***");
    m_pLog->AddLine("Decoding Photoshop format...");
    m_pLog->AddLine("");
  }
  else
  {
    return false;
  }

  // Rewind back to header (signature)
  nPos -= 6;

  bool bDecOk = true;

  tsImageInfo sImageInfo;

  PhotoshopParseFileHeader(nPos, 3, &sImageInfo);
  nWidth = sImageInfo.nImageWidth;
  nHeight = sImageInfo.nImageHeight;

  PhotoshopParseColorModeSection(nPos, 3);

  if(bDecOk)
    bDecOk &= PhotoshopParseImageResourcesSection(nPos, 3);

  if(bDecOk)
    bDecOk &= PhotoshopParseLayerMaskInfo(nPos, 3, pDibTemp);

  if(bDecOk)
  {
    unsigned char *pDibBits = nullptr;

#ifdef PS_IMG_DEC_EN
    if((pDibTemp) && (m_bDisplayImage))
    {
      // Allocate the device-independent bitmap (DIB)
      // - Although we are creating a 32-bit DIB, it should also
      //   work to run in 16- and 8-bit modes

      // Start by killing old DIB since we might be changing its dimensions
//@@      pDibTemp->Kill();

      // Allocate the new DIB
//@@      bool bDibOk = pDibTemp->CreateDIB(nWidth, nHeight, 32);

      // Should only return false if already exists or failed to allocate
//@@     if(bDibOk)
      {
        // Fetch the actual pixel array
//@@        pDibBits = (unsigned char *) (pDibTemp->GetDIBBitArray());
      }

      pDibTemp = new QImage(nWidth, nHeight, QImage::Format_RGB32);
    }
#endif

    bDecOk &= PhotoshopParseImageData(nPos, 3, &sImageInfo, pDibBits);
    PhotoshopParseReportFldOffset(3, "Image data decode complete:", nPos);
  }

  PhotoshopParseReportNote(3, "");

  if(!bDecOk)
  {
    m_pLog->AddLineErr("ERROR: There was a problem during decode. Aborting.");
    return false;
  }

  return true;
}

// Locate a field ID in the constant 8BIM / Image Resource array
// 
// INPUT:
// - nBimId                             = Image Resource ID (16-bit unsigned)
// OUTPUT:
// - nFldInd                    = Index into asBimRecords[] array
// RETURN:
// - Returns true if ID was found in array
// NOTE:
// - The constant struct array depends on BIM_T_END as a terminator for the list
//
bool CDecodePs::FindBimRecord(uint32_t nBimId, uint32_t &nFldInd)
{
  uint32_t nInd = 0;

  bool bDone = false;
  bool bFound = false;

  while(!bDone)
  {
    if(asBimRecords[nInd].eBimType == BIM_T_END)
    {
      bDone = true;
    }
    else
    {
      // Support detection of code ranges
      uint32_t nCodeStart = asBimRecords[nInd].nCode;

      uint32_t nCodeEnd = asBimRecords[nInd].nCodeEnd;

      if((nCodeEnd == 0) && (nCodeStart == nBimId))
      {
        // Match with single code (ie. not ranged entry)
        bDone = true;
        bFound = true;
      }
      else if((nCodeEnd != 0) && (nCodeStart <= nBimId) && (nCodeEnd >= nBimId))
      {
        // Match with code in ranged entry
        bDone = true;
        bFound = true;
      }
      else
      {
        nInd++;
      }
    }
  }

  if(bFound)
  {
    nFldInd = nInd;
  }

  return bFound;
}

// Search the constant array of IPTC fields for the Record:DataSet index
//
// OUTPUT:
// - nFldInd  = The index into the array of the located entry (if found)
// RETURN
// - Successfully found the Record:DataSet value
//
bool CDecodePs::LookupIptcField(uint32_t nRecord, uint32_t nDataSet, uint32_t &nFldInd)
{
  uint32_t nInd = 0;

  bool bDone = false;
  bool bFound = false;

  while(!bDone)
  {
    if(asIptcFields[nInd].eFldType == IPTC_T_END)
    {
      bDone = true;
    }
    else
    {
      if((asIptcFields[nInd].nRecord == nRecord) && (asIptcFields[nInd].nDataSet == nDataSet))
      {
        bDone = true;
        bFound = true;
      }
      else
      {
        nInd++;
      }
    }
  }

  if(bFound)
  {
    nFldInd = nInd;
  }

  return bFound;
}

// Generate the custom-formatted string representing the IPTC field name and value
//
// INPUT:
// - eIptcType                          = Enumeration representing the IPTC field type
// - nFldCnt                            = The number of elements in this field to report
// - nPos                                       = The starting file offset for this field
// RETURN:
// - The IPTC formattted string
// NOTE:
// - The IPTC field type is used to determine how to represent the input values
//
QString CDecodePs::DecodeIptcValue(teIptcType eIptcType, uint32_t nFldCnt, uint32_t nPos)
{
  //uint32_t      nFldInd = 0;
  uint32_t nInd;
  uint32_t nVal;

  QString strField = "";
  QString strByte = "";
  QString strVal = "";

  switch (eIptcType)
  {
    case IPTC_T_NUM:
    case IPTC_T_NUM1:
    case IPTC_T_NUM2:
      nVal = m_pWBuf->BufX(nPos, nFldCnt, PS_BSWAP);
      nPos += nFldCnt;
      nPos += nFldCnt;
      strVal = QString("%1").arg(nVal);
      break;

    case IPTC_T_HEX:
      strVal = "[";

      for(nInd = 0; nInd < nFldCnt; nInd++)
      {
        strByte = QString("0x%1 ").arg(Buf(nPos++), 2, 16, QChar('0'));
        strVal += strByte;
      }

      strVal += "]";
      break;

    case IPTC_T_STR:
      strVal = "\"";

      for(nInd = 0; nInd < nFldCnt; nInd++)
      {
        strByte = QString("%1").arg(Buf(nPos++));
        strVal += strByte;
      }

      strVal += "\"";
      break;

    case IPTC_T_UNK:
      strVal = "???";

    default:
      break;
  }

  return strVal;
}

// Decode the IPTC metadata segment
// INPUT:
//      nLen    : Length of the 8BIM:IPTC resource data
void CDecodePs::DecodeIptc(uint32_t &nPos, uint32_t nLen, uint32_t nIndent)
{
  QString strIndent;
  QString strTmp;
  QString strIptcTypeName;
  QString strIptcField;
  QString strIptcVal;
  QString strByte;

  uint32_t nPosStart;

  bool bDone;

  uint32_t nTagMarker, nRecordNumber, nDataSetNumber, nDataFieldCnt;

  strIndent = PhotoshopParseIndent(nIndent);
  nPosStart = nPos;
  bDone = true;

  if(nPos <= nPosStart + nLen)
  {
    // TODO: Should probably check to see if we have at least 5 bytes?
    bDone = false;
  }

  while(!bDone)
  {
    nTagMarker = Buf(nPos + 0);
    nRecordNumber = Buf(nPos + 1);
    nDataSetNumber = Buf(nPos + 2);
    nDataFieldCnt = Buf(nPos + 3) * 256 + Buf(nPos + 4);
    nPos += 5;

    if(nTagMarker == 0x1C)
    {
      uint32_t nFldInd;

      teIptcType eIptcType;

      if(LookupIptcField(nRecordNumber, nDataSetNumber, nFldInd))
      {
        strIptcField = QString("%1").arg(asIptcFields[nFldInd].strFldName, -35);
        eIptcType = asIptcFields[nFldInd].eFldType;
      }
      else
      {
        strIptcField = QString("%1").arg("?", -35);
        eIptcType = IPTC_T_UNK;
      }

      strIptcVal = DecodeIptcValue(eIptcType, nDataFieldCnt, nPos);
      strTmp = QString("IPTC [%1:%2] %3 = %4").arg(strIndent)
          .arg(nRecordNumber, 3, 10, QChar('0'))
          .arg(nDataSetNumber, 3, 10, QChar('0'))
          .arg(strIptcField)
          .arg(strIptcVal);
      m_pLog->AddLine(strTmp);
      nPos += nDataFieldCnt;
    }
    else
    {
      // Unknown Tag Marker
      // I have seen at least one JPEG file that had an IPTC block with all zeros.
      // In this example, the TagMarker check for 0x1C would fail.
      // Since we don't know how to parse it, abort now.
      strTmp = QString("ERROR: Unknown IPTC TagMarker [0x%1] @ 0x%2. Skipping parsing.")
          .arg(nTagMarker, 2, 16, QChar('0'))
          .arg(nPos - 5, 8, 16, QChar('0'));
      m_pLog->AddLineErr(strTmp);

#ifdef DEBUG_LOG
      QString strDebug;

      strDebug = QString("## File=[%1] Block=[%2] Error=[%3]\n")
          .arg(m_pAppConfig->strCurFname, -100)
          .arg("PsDecode", -10)
          .arg(strTmp);
      qDebug() << (strDebug);
#endif

      // Adjust length to end
      nPos = nPosStart + nLen;

      // Now abort
      bDone = true;
    }

    if(nPos >= nPosStart + nLen)
    {
      bDone = true;
    }
  }
}

// Create indent string
// - Returns a sequence of spaces according to the indent level
QString CDecodePs::PhotoshopParseIndent(uint32_t nIndent)
{
  QString strIndent = "";

  for(uint32_t nInd = 0; nInd < nIndent; nInd++)
  {
    strIndent += "  ";
  }

  return strIndent;
}

// Get ASCII string according to Photoshop format
// - String length prefix
// - If length is 0 then fixed 4-character string
// - Otherwise it is defined length string (no terminator required)
QString CDecodePs::PhotoshopParseGetLStrAsc(uint32_t &nPos)
{
  uint32_t nStrLen;

  QString strVal = "";

  nStrLen = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);

  if(nStrLen != 0)
  {
    strVal = m_pWBuf->BufReadStrn(nPos, nStrLen);
    nPos += nStrLen;
  }
  else
  {
    // 4 byte string
    strVal = QString("%1%2%3%4")
        .arg(Buf(nPos + 0))
        .arg(Buf(nPos + 1))
        .arg(Buf(nPos + 2))
        .arg(Buf(nPos + 3));
    nPos += 4;
  }

  return strVal;
}

// Read a UNICODE string starting at nPos according to Photoshop Unicode string format
// - Start with 4 byte length
// - Follow with UNICODE string
// OUTPUT:
// - The byte offset to advance the file pointer
// RETURN:
// - Unicode string
QString CDecodePs::PhotoshopParseGetBimLStrUni(uint32_t nPos, uint32_t &nPosOffset)
{
  QString strVal;

  uint32_t nStrLenActual, nStrLenTrunc;

  char anStrBuf[(PS_MAX_UNICODE_STRLEN + 1) * 2];

//  wchar_t acStrBuf[(PS_MAX_UNICODE_STRLEN + 1)];

  char nChVal;

  nPosOffset = 0;
  nStrLenActual = m_pWBuf->BufX(nPos + nPosOffset, 4, PS_BSWAP);
  nPosOffset += 4;
  nStrLenTrunc = nStrLenActual;

  // Initialize return
  strVal = "";

  if(nStrLenTrunc > 0)
  {
    // Read unicode bytes into byte array
    // Truncate the string, leaving room for terminator
    if(nStrLenTrunc > PS_MAX_UNICODE_STRLEN)
    {
      nStrLenTrunc = PS_MAX_UNICODE_STRLEN;
    }

    for(uint32_t nInd = 0; nInd < nStrLenTrunc; nInd++)
    {
      // Reverse the order of the bytes
      nChVal = Buf(nPos + nPosOffset + (nInd * 2) + 0);
      anStrBuf[(nInd * 2) + 1] = nChVal;
      nChVal = Buf(nPos + nPosOffset + (nInd * 2) + 1);
      anStrBuf[(nInd * 2) + 0] = nChVal;
    }

    // TODO: Replace with call to ByteStr2Unicode()
    // Ensure it is terminated
    anStrBuf[nStrLenTrunc * 2 + 0] = 0;
    anStrBuf[nStrLenTrunc * 2 + 1] = 0;
    // Copy into unicode string
    // Ensure that it is terminated first!
//    wcscpy(acStrBuf,anStrBuf);
    // Copy into QString
    strVal = QString(anStrBuf);
  }

  // Update the file position offset
  nPosOffset += nStrLenActual * 2;
  // Return
  return strVal;
}

// Display a formatted note line
// - Apply the current indent level (nIndent)
void CDecodePs::PhotoshopParseReportNote(uint32_t nIndent, QString strNote)
{
  QString strIndent;
  QString strLine;

  strIndent = PhotoshopParseIndent(nIndent);
  strLine = QString("%1%2")
      .arg(strIndent)
      .arg(strNote, -50);
  m_pLog->AddLine(strLine);
}

// Display a formatted numeric field with optional units string
// - Report the value with the field name (strField) and current indent level (nIndent)
void CDecodePs::PhotoshopParseReportFldNum(uint32_t nIndent, QString strField, uint32_t nVal, QString strUnits)
{
  QString strIndent;
  QString strVal;
  QString strLine;

  strIndent = PhotoshopParseIndent(nIndent);
  strVal = QString("%1").arg(nVal);
  strLine = QString("%1%2 = %3 %4")
      .arg(strIndent)
      .arg(strField, -50)
      .arg(strVal).arg(strUnits);
  m_pLog->AddLine(strLine);
}

// Display a formatted boolean field
// - Report the value with the field name (strField) and current indent level (nIndent)
void CDecodePs::PhotoshopParseReportFldBool(uint32_t nIndent, QString strField, uint32_t nVal)
{
  QString strIndent;
  QString strVal;
  QString strLine;

  strIndent = PhotoshopParseIndent(nIndent);
  strVal = (nVal != 0) ? "true" : "false";
  strLine = QString("%1%2 = %3")
      .arg(strIndent)
      .arg(strField, -50)
      .arg(strVal);
  m_pLog->AddLine(strLine);
}

//
// Display a formatted hexadecimal field
// - Report the value with the field name (strField) and current indent level (nIndent)
// - Depending on the length of the buffer, either report the hex value in-line with
//   the field name or else start a new row and begin a hex dump
// - Also report the ASCII representation alongside the hex dump
//
// INPUT:
// - nIndent            = Current indent level for reporting
// - strField           = Field name
// - nPosStart          = Field byte array file position start
// - nLen                       = Field byte array length
//
void CDecodePs::PhotoshopParseReportFldHex(uint32_t nIndent, QString strField, uint32_t nPosStart, uint32_t nLen)
{
  QString strIndent;
  QString strPrefix;
  QString strByteHex;
  QString strByteAsc;
  QString strValHex;
  QString strValAsc;
  QString strLine;

  int32_t nByte;

  strIndent = PhotoshopParseIndent(nIndent);

  if(nLen == 0)
  {
    // Print out the header row, but no data will be shown
    strLine = QString("%1%2 = ").arg(strIndent).arg(strField, -50);
    m_pLog->AddLine(strLine);
    // Nothing to report, exit now
    return;
  }
  else if(nLen <= PS_HEX_MAX_INLINE)
  {
    // Define prefix for row
    strPrefix = QString("%1%2 = ").arg(strIndent).arg(strField, -50);
  }
  else
  {
    // Print out header row
    strLine = QString("%1%2s =").arg(strIndent).arg(strField, -50);
    m_pLog->AddLine(strLine);
    // Define prefix for next row
    strPrefix = QString("%1").arg(strIndent);
  }

  // Build up the hex string
  // Limit to PS_HEX_TOTAL bytes
  uint32_t nRowOffset = 0;

  bool bDone = false;

  uint32_t nLenClip;

  nLenClip = qMin(nLen, PS_HEX_TOTAL);
  strValHex = "";
  strValAsc = "";

  while(!bDone)
  {
    // Have we reached the end of the data we wish to display?
    if(nRowOffset >= nLenClip)
    {
      bDone = true;
    }
    else
    {
      // Reset the cumulative hex/ascii value strings
      strValHex = "";
      strValAsc = "";

      // Build the hex/ascii value strings
      for(uint32_t nInd = 0; nInd < PS_HEX_MAX_ROW; nInd++)
      {
        if((nRowOffset + nInd) < nLenClip)
        {
          nByte = m_pWBuf->Buf(nPosStart + nRowOffset + nInd);
          strByteHex = QString("%1 ").arg(nByte, 2, 16, QChar('0'));

          // Only display printable characters
          if(isprint(nByte))
          {
            strByteAsc = QString("%1").arg(nByte);
          }
          else
          {
            strByteAsc = ".";
          }
        }
        else
        {
          // Pad out to end of row
          strByteHex = QString("   ");
          strByteAsc = " ";
        }

        // Build up the strings
        strValHex += strByteHex;
        strValAsc += strByteAsc;
      }

      // Generate the line with Hex and ASCII representations
      strLine = QString("%1 | 0x%2 | %3").arg(strPrefix).arg(strValHex).arg(strValAsc);
      m_pLog->AddLine(strLine);

      // Now increment file offset
      nRowOffset += PS_HEX_MAX_ROW;
    }
  }

  // If we had to clip the display length, then show ellipsis now
  if(nLenClip < nLen)
  {
    strLine = QString("%1 | ...").arg(strPrefix);
    m_pLog->AddLine(strLine);
  }

}

QString CDecodePs::PhotoshopDispHexWord(uint32_t nVal)
{
  QString strValHex;
  QString strValAsc;
  QString strByteHex;
  QString strByteAsc;
  QString strLine;

  uint32_t nByte;

  // Reset the cumulative hex/ascii value strings
  strValHex = "";
  strValAsc = "";

  // Build the hex/ascii value strings
  for(uint32_t nInd = 0; nInd <= 3; nInd++)
  {
    nByte = (nVal & 0xFF000000) >> 24;
    nVal <<= 8;

    strByteHex = QString("%1 ").arg(nByte, 2, 16, QChar('0'));

    // Only display printable characters
    if(isprint(nByte))
    {
      strByteAsc = QString("%1").arg(nByte);
    }
    else
    {
      strByteAsc = ".";
    }

    // Build up the strings
    strValHex += strByteHex;
    strValAsc += strByteAsc;
  }

  // Generate the line with Hex and ASCII representations
  strLine = QString("0x%1 | %2").arg(strValHex).arg(strValAsc);

  return strLine;
}

// Look up the enumerated constant (nVal) for the specified field (eEnumField)
// - Returns "" if the field wasn't found
QString CDecodePs::PhotoshopParseLookupEnum(teBimEnumField eEnumField, uint32_t nVal)
{
  QString strVal = "";

  // Find the enum value
  bool bDone = false;

  bool bFound = false;

  uint32_t nEnumInd = 0;

  while(!bDone)
  {
    if(asBimEnums[nEnumInd].eBimEnum == eEnumField)
    {
      if(asBimEnums[nEnumInd].nVal == nVal)
      {
        bDone = true;
        bFound = true;
        strVal = asBimEnums[nEnumInd].strVal;
      }
    }

    if(asBimEnums[nEnumInd].eBimEnum == BIM_T_ENUM_END)
    {
      bDone = true;
    }

    if(!bDone)
    {
      nEnumInd++;
    }
  }

  // If enum wasn't found, then replace by hex value
  if(!bFound)
  {
    QString strWord = PhotoshopDispHexWord(nVal);

    strVal = QString("? [%1]").arg(strWord);
  }
  return strVal;
}

// Display a formatted enumerated type field
// - Look up enumerated constant (nVal) for the given field (eEnumField)
// - Report the value with the field name (strField) and current indent level (nIndent)
void CDecodePs::PhotoshopParseReportFldEnum(uint32_t nIndent, QString strField, teBimEnumField eEnumField, uint32_t nVal)
{
  QString strIndent;
  QString strVal;
  QString strLine;

  strIndent = PhotoshopParseIndent(nIndent);
  strVal = PhotoshopParseLookupEnum(eEnumField, nVal);
  strLine = QString("%1%2 = %3").arg(strIndent).arg(strField, -50).arg(strVal);
  m_pLog->AddLine(strLine);
}

// Display a formatted fixed-point field
// - Convert a 32-bit unsigned integer into Photoshop fixed point
// - Assume fixed point representation is 16-bit integer : 16-bit fraction
// - Report the value with the field name (strField) and current indent level (nIndent)
void CDecodePs::PhotoshopParseReportFldFixPt(uint32_t nIndent, QString strField, uint32_t nVal, QString strUnits)
{
  QString strIndent;
  QString strVal;
  QString strLine;

  double fVal;

  fVal = (static_cast<double>(nVal) / 65536.0);
  strIndent = PhotoshopParseIndent(nIndent);
  strVal = QString("%1").arg(fVal);
  strLine = QString("%1%2 = %3 %4")
      .arg(strIndent)
      .arg(strField, -50)
      .arg(strVal)
      .arg(strUnits);
  m_pLog->AddLine(strLine);
}

// Display a formatted fixed-point field
// - Convert a 32-bit unsigned integer into Photoshop floating point
// - Report the value with the field name (strField) and current indent level (nIndent)
void CDecodePs::PhotoshopParseReportFldFloatPt(uint32_t nIndent, QString strField, uint32_t nVal, QString strUnits)
{
  QString strIndent;
  QString strVal;
  QString strLine;

  float fVal;

  // Convert 4 byte unsigned int to floating-point
  // TODO: Need to check this since Photoshop web spec doesn't
  // indicate how floating points are stored.
  union tUnionTmp
  {
    uint8_t nVal[4];
    float fVal;
  };

  tUnionTmp myUnion;

  // NOTE: Empirically determined the byte order for double representation
  myUnion.nVal[3] = static_cast <uint8_t> ((nVal & 0xFF000000) >> 24);
  myUnion.nVal[2] = static_cast <uint8_t> ((nVal & 0x00FF0000) >> 16);
  myUnion.nVal[1] = static_cast <uint8_t> ((nVal & 0x0000FF00) >> 8);
  myUnion.nVal[0] = static_cast <uint8_t> ((nVal & 0x000000FF) >> 0);
  fVal = myUnion.fVal;

  strIndent = PhotoshopParseIndent(nIndent);
  strVal = QString("%1").arg(fVal, 0, 'f', 5);
  strLine = QString("%1%2 = %3 %4")
      .arg(strIndent)
      .arg(strField, -50)
      .arg(strVal).arg(strUnits);
  m_pLog->AddLine(strLine);
}

// Display a formatted double-precision floating-point field
// - Convert two 32-bit unsigned integers into Photoshop double-precision floating point
// - Report the value with the field name (strField) and current indent level (nIndent)
void CDecodePs::PhotoshopParseReportFldDoublePt(uint32_t nIndent, QString strField, uint32_t nVal1, uint32_t nVal2, QString strUnits)
{
  QString strIndent;
  QString strVal;
  QString strLine;

  double dVal;

  // Convert 2x4 byte unsigned int to double-point
  // TODO: Need to check this since Photoshop web spec doesn't
  // indicate how double points are stored.
  union tUnionTmp
  {
    uint8_t nVal[8];
    double dVal;
  };

  tUnionTmp myUnion;

  // NOTE: Empirically determined the byte order for double representation
  myUnion.nVal[7] = static_cast <uint8_t> ((nVal1 & 0xFF000000) >> 24);
  myUnion.nVal[6] = static_cast <uint8_t> ((nVal1 & 0x00FF0000) >> 16);
  myUnion.nVal[5] = static_cast <uint8_t> ((nVal1 & 0x0000FF00) >> 8);
  myUnion.nVal[4] = static_cast <uint8_t> ((nVal1 & 0x000000FF) >> 0);
  myUnion.nVal[3] = static_cast <uint8_t> ((nVal2 & 0xFF000000) >> 24);
  myUnion.nVal[2] = static_cast <uint8_t> ((nVal2 & 0x00FF0000) >> 16);
  myUnion.nVal[1] = static_cast <uint8_t> ((nVal2 & 0x0000FF00) >> 8);
  myUnion.nVal[0] = static_cast <uint8_t> ((nVal2 & 0x000000FF) >> 0);
  dVal = myUnion.dVal;

  strIndent = PhotoshopParseIndent(nIndent);
  strVal = QString("%1").arg(dVal, 0, 'f', 5);
  strLine = QString("%1%2 = %3 %4")
      .arg(strIndent)
      .arg(strField, -50)
      .arg(strVal)
      .arg(strUnits);
  m_pLog->AddLine(strLine);
}

// Display a formatted string field
// - Report the value with the field name (strField) and current indent level (nIndent)
void CDecodePs::PhotoshopParseReportFldStr(uint32_t nIndent, QString strField, QString strVal)
{
  QString strIndent;
  QString strLine;

  strIndent = PhotoshopParseIndent(nIndent);
  strLine = QString("%1%2 = \"%3\"").arg(strIndent).arg(strField, -50).arg(strVal);
  m_pLog->AddLine(strLine);
}

// Display a formatted file offset field
// - Report the offset with the field name (strField) and current indent level (nIndent)
void CDecodePs::PhotoshopParseReportFldOffset(uint32_t nIndent, QString strField, uint32_t nOffset)
{
  QString strIndent;
  QString strLine;

  strIndent = PhotoshopParseIndent(nIndent);
  strLine = QString("%1%2 @ 0x%3").arg(strIndent).arg(strField, -50).arg(nOffset, 8, 16, QChar('0'));
  m_pLog->AddLine(strLine);
}

// Parse the Photoshop IRB Thumbnail Resource
// - NOTE: Returned nPos doesn't take into account JFIF data
void CDecodePs::PhotoshopParseThumbnailResource(uint32_t &nPos, uint32_t nIndent)
{
  uint32_t nVal;

  nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Format", nVal, "");
  nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Width of thumbnail", nVal, "pixels");
  nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Height of thumbnail", nVal, "pixels");
  nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Widthbytes", nVal, "bytes");
  nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Total size", nVal, "bytes");
  nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Size after compression", nVal, "bytes");
  nVal = m_pWBuf->BufRdAdv2(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Bits per pixel", nVal, "bits");
  nVal = m_pWBuf->BufRdAdv2(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Number of planes", nVal, "");

  PhotoshopParseReportFldOffset(nIndent, "JFIF data", nPos);

  // NOTE: nPos doesn't take into account JFIF data!
}

// Parse the Photoshop IRB Version Info
// INPUT:
// - nPos               = File position at start of block
// - nIndent    = Indent level for formatted output
// OUTPUT:
// - nPos               = File position after reading the block
//
void CDecodePs::PhotoshopParseVersionInfo(uint32_t &nPos, uint32_t nIndent)
{
  uint32_t nVal;

  QString strVal;

  uint32_t nPosOffset;

  nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Version", nVal, "");
  nVal = m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "hasRealMergedData", nVal, "");
  strVal = PhotoshopParseGetBimLStrUni(nPos, nPosOffset);
  nPos += nPosOffset;
  PhotoshopParseReportFldStr(nIndent, "Writer name", strVal);
  strVal = PhotoshopParseGetBimLStrUni(nPos, nPosOffset);
  nPos += nPosOffset;
  PhotoshopParseReportFldStr(nIndent, "Reader name", strVal);
  nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "File version", nVal, "");
}

// Parse the Photoshop IRB Print Scale
// INPUT:
// - nPos               = File position at start of block
// - nIndent    = Indent level for formatted output
// OUTPUT:
// - nPos               = File position after reading the block
//
void CDecodePs::PhotoshopParsePrintScale(uint32_t &nPos, uint32_t nIndent)
{
  uint32_t nVal;

  QString strVal;

  nVal = m_pWBuf->BufRdAdv2(nPos, PS_BSWAP);
  PhotoshopParseReportFldEnum(nIndent, "Style", BIM_T_ENUM_PRINT_SCALE_STYLE, nVal);
  nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  PhotoshopParseReportFldFloatPt(nIndent, "X location", nVal, "");
  nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  PhotoshopParseReportFldFloatPt(nIndent, "Y location", nVal, "");
  nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  PhotoshopParseReportFldFloatPt(nIndent, "Scale", nVal, "");
}

// Parse the Photoshop IRB Global Angle
// INPUT:
// - nPos               = File position at start of block
// - nIndent    = Indent level for formatted output
// OUTPUT:
// - nPos               = File position after reading the block
//
void CDecodePs::PhotoshopParseGlobalAngle(uint32_t &nPos, uint32_t nIndent)
{
  uint32_t nVal;

  QString strVal;

  nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Global Angle", nVal, "degrees");
}

// Parse the Photoshop IRB Global Altitude
// INPUT:
// - nPos               = File position at start of block
// - nIndent    = Indent level for formatted output
// OUTPUT:
// - nPos               = File position after reading the block
//
void CDecodePs::PhotoshopParseGlobalAltitude(uint32_t &nPos, uint32_t nIndent)
{
  uint32_t nVal;

  QString strVal;

  nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Global Altitude", nVal, "");
}

// Parse the Photoshop IRB Print Flags
// INPUT:
// - nPos               = File position at start of block
// - nIndent    = Indent level for formatted output
// OUTPUT:
// - nPos               = File position after reading the block
//
void CDecodePs::PhotoshopParsePrintFlags(uint32_t &nPos, uint32_t nIndent)
{
  uint32_t nVal;

  QString strVal;

  nVal = m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);
  PhotoshopParseReportFldBool(nIndent, "Labels", nVal);
  nVal = m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);
  PhotoshopParseReportFldBool(nIndent, "Crop marks", nVal);
  nVal = m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);
  PhotoshopParseReportFldBool(nIndent, "Color bars", nVal);
  nVal = m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);
  PhotoshopParseReportFldBool(nIndent, "Registration marks", nVal);
  nVal = m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);
  PhotoshopParseReportFldBool(nIndent, "Negative", nVal);
  nVal = m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);
  PhotoshopParseReportFldBool(nIndent, "Flip", nVal);
  nVal = m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);
  PhotoshopParseReportFldBool(nIndent, "Interpolate", nVal);
  nVal = m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);
  PhotoshopParseReportFldBool(nIndent, "Caption", nVal);
  nVal = m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);
  PhotoshopParseReportFldBool(nIndent, "Print flags", nVal);
}

// Parse the Photoshop IRB Print Flags Information
// INPUT:
// - nPos               = File position at start of block
// - nIndent    = Indent level for formatted output
// OUTPUT:
// - nPos               = File position after reading the block
//
void CDecodePs::PhotoshopParsePrintFlagsInfo(uint32_t &nPos, uint32_t nIndent)
{
  uint32_t nVal;

  QString strVal;

  nVal = m_pWBuf->BufRdAdv2(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Version", nVal, "");
  nVal = m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Center crop marks", nVal, "");
  nVal = m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Reserved", nVal, "");
  nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Bleed width value", nVal, "");
  nVal = m_pWBuf->BufRdAdv2(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Bleed width scale", nVal, "");
}

// Parse the Photoshop IRB Copyright Flag
// INPUT:
// - nPos               = File position at start of block
// - nIndent    = Indent level for formatted output
// OUTPUT:
// - nPos               = File position after reading the block
//
void CDecodePs::PhotoshopParseCopyrightFlag(uint32_t &nPos, uint32_t nIndent)
{
  uint32_t nVal;

  QString strVal;

  nVal = m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);
  PhotoshopParseReportFldBool(nIndent, "Copyright flag", nVal);
}

// Parse the Photoshop IRB Aspect Ratio
// INPUT:
// - nPos               = File position at start of block
// - nIndent    = Indent level for formatted output
// OUTPUT:
// - nPos               = File position after reading the block
//
void CDecodePs::PhotoshopParsePixelAspectRatio(uint32_t &nPos, uint32_t nIndent)
{
  uint32_t nVal, nVal1, nVal2;

  QString strVal;

  nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Version", nVal, "");
  nVal1 = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  nVal2 = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  PhotoshopParseReportFldDoublePt(nIndent, "X/Y Ratio", nVal1, nVal2, "");
}

// Parse the Photoshop IRB Document-Specific Seed
// INPUT:
// - nPos               = File position at start of block
// - nIndent    = Indent level for formatted output
// OUTPUT:
// - nPos               = File position after reading the block
//
void CDecodePs::PhotoshopParseDocSpecificSeed(uint32_t &nPos, uint32_t nIndent)
{
  uint32_t nVal;

  QString strVal;

  nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Base value", nVal, "");
}

// Parse the Photoshop IRB Grid and Guides
// INPUT:
// - nPos               = File position at start of block
// - nIndent    = Indent level for formatted output
// OUTPUT:
// - nPos               = File position after reading the block
//
void CDecodePs::PhotoshopParseGridGuides(uint32_t &nPos, uint32_t nIndent)
{
  uint32_t nVal;

  QString strVal;

  nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Version", nVal, "");
  nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Grid Horizontal", nVal, "");
  nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Grid Vertical", nVal, "");
  uint32_t nNumGuides = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);

  PhotoshopParseReportFldNum(nIndent, "Number of Guide Resources", nNumGuides, "");

  if(nNumGuides > 0)
    PhotoshopParseReportNote(nIndent, "-----");

  for(uint32_t nInd = 0; nInd < nNumGuides; nInd++)
  {
    strVal = QString("Guide #%1:").arg(nInd);
    PhotoshopParseReportNote(nIndent, strVal);

    nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
    PhotoshopParseReportFldNum(nIndent + 1, "Location", nVal, "");
    nVal = m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);
    PhotoshopParseReportFldEnum(nIndent + 1, "Direction", BIM_T_ENUM_GRID_GUIDE_DIR, nVal);
  }

  if(nNumGuides > 0)
    PhotoshopParseReportNote(nIndent, "-----");
}

// Parse the Photoshop IRB Resolution Info
// INPUT:
// - nPos               = File position at start of block
// - nIndent    = Indent level for formatted output
// OUTPUT:
// - nPos               = File position after reading the block
//
void CDecodePs::PhotoshopParseResolutionInfo(uint32_t &nPos, uint32_t nIndent)
{
  uint32_t nVal, nUnit;

  QString strVal, strUnit;

  nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  nUnit = m_pWBuf->BufRdAdv2(nPos, PS_BSWAP);
  strUnit = PhotoshopParseLookupEnum(BIM_T_ENUM_RESOLUTION_INFO_RES_UNIT, nUnit);
  PhotoshopParseReportFldFixPt(nIndent, "Horizontal resolution", nVal, strUnit);
  nVal = m_pWBuf->BufRdAdv2(nPos, PS_BSWAP);
  PhotoshopParseReportFldEnum(nIndent, "Width unit", BIM_T_ENUM_RESOLUTION_INFO_WIDTH_UNIT, nVal);
  nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  nUnit = m_pWBuf->BufRdAdv2(nPos, PS_BSWAP);
  strUnit = PhotoshopParseLookupEnum(BIM_T_ENUM_RESOLUTION_INFO_RES_UNIT, nUnit);
  PhotoshopParseReportFldFixPt(nIndent, "Vertical resolution", nVal, strUnit);
  nVal = m_pWBuf->BufRdAdv2(nPos, PS_BSWAP);
  PhotoshopParseReportFldEnum(nIndent, "Height unit", BIM_T_ENUM_RESOLUTION_INFO_WIDTH_UNIT, nVal);
}

// Parse the Photoshop IRB Layer State Info
// INPUT:
// - nPos               = File position at start of block
// - nIndent    = Indent level for formatted output
// OUTPUT:
// - nPos               = File position after reading the block
//
void CDecodePs::PhotoshopParseLayerStateInfo(uint32_t &nPos, uint32_t nIndent)
{
  uint32_t nVal;

  QString strVal;

  nVal = m_pWBuf->BufRdAdv2(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Target layer", nVal, "");
}

// Parse the Photoshop IRB Layer Group Info
// INPUT:
// - nPos               = File position at start of block
// - nIndent    = Indent level for formatted output
// - nLen               = Length of IRB entry (to determine number of layers)
// OUTPUT:
// - nPos               = File position after reading the block
//
void CDecodePs::PhotoshopParseLayerGroupInfo(uint32_t &nPos, uint32_t nIndent, uint32_t nLen)
{
  uint32_t nVal;

  QString strVal;

  // NOTE:
  // According to the Photoshop documentation:
  // - 2 bytes per layer containing a group ID for the dragging groups.
  // - Layers in a group have the same group ID.
  // It was not clear whether there was a separate indication of the number of layers
  // For now, assume we can derive it from the total IRB length
  uint32_t nNumLayers;

  nNumLayers = nLen / 2;

  for(uint32_t nInd = 0; nInd < nNumLayers; nInd++)
  {
    strVal = QString("Layer #%1:").arg(nInd);
    PhotoshopParseReportNote(nIndent, strVal);
    nVal = m_pWBuf->BufRdAdv2(nPos, PS_BSWAP);
    PhotoshopParseReportFldNum(nIndent + 1, "Layer Group", nVal, "");
  }
}

// Parse the Photoshop IRB Layer Groups Enabled
// INPUT:
// - nPos               = File position at start of block
// - nIndent    = Indent level for formatted output
// - nLen               = Length of IRB entry (to determine number of layers)
// OUTPUT:
// - nPos               = File position after reading the block
//
void CDecodePs::PhotoshopParseLayerGroupEnabled(uint32_t &nPos, uint32_t nIndent, uint32_t nLen)
{
  uint32_t nVal;

  QString strVal;

  // NOTE:
  // According to the Photoshop documentation:
  // - 1 byte for each layer in the document, repeated by length of the resource.
  // - NOTE: Layer groups have start and end markers
  // For now, assume we can derive it from the total IRB length
  uint32_t nNumLayers;

  nNumLayers = nLen / 1;

  for(uint32_t nInd = 0; nInd < nNumLayers; nInd++)
  {
    strVal = QString("Layer #%1:").arg(nInd);
    PhotoshopParseReportNote(nIndent, strVal);
    nVal = m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);
    PhotoshopParseReportFldNum(nIndent + 1, "Layer Group Enabled ID", nVal, "");
  }
}

// Parse the Photoshop IRB Layer Select IDs
// INPUT:
// - nPos               = File position at start of block
// - nIndent    = Indent level for formatted output
// OUTPUT:
// - nPos               = File position after reading the block
//
void CDecodePs::PhotoshopParseLayerSelectId(uint32_t &nPos, uint32_t nIndent)
{
  uint32_t nVal;

  QString strVal;

  uint32_t nNumLayer = m_pWBuf->BufRdAdv2(nPos, PS_BSWAP);

  PhotoshopParseReportFldNum(nIndent, "Num selected", nNumLayer, "");

  for(uint32_t nInd = 0; nInd < nNumLayer; nInd++)
  {
    nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
    PhotoshopParseReportFldNum(nIndent + 1, "Layer ID", nVal, "");
  }
}

// Parse the Photoshop IRB File Header
// INPUT:
// - nPos               = File position at start of block
// - nIndent    = Indent level for formatted output
// OUTPUT:
// - nPos               = File position after reading the block
//
void CDecodePs::PhotoshopParseFileHeader(uint32_t &nPos, uint32_t nIndent, tsImageInfo * psImageInfo)
{
  Q_ASSERT(psImageInfo);

  uint32_t nVal;

  QString strVal;

  PhotoshopParseReportNote(nIndent, "File Header Section:");
  nIndent++;

  QString strSig = m_pWBuf->BufReadStrn(nPos, 4);

  nPos += 4;
  PhotoshopParseReportFldStr(nIndent, "Signature", strSig);

  nVal = m_pWBuf->BufRdAdv2(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Version", nVal, "");
  nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Reserved1", nVal, "");
  nVal = m_pWBuf->BufRdAdv2(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Reserved2", nVal, "");
  nVal = m_pWBuf->BufRdAdv2(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Num channels in image", nVal, "");
  psImageInfo->nNumChans = nVal;
  nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Image height", nVal, "pixels");
  psImageInfo->nImageHeight = nVal;
  nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Image width", nVal, "pixels");
  psImageInfo->nImageWidth = nVal;
  nVal = m_pWBuf->BufRdAdv2(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Depth", nVal, "bits per pixel");
  psImageInfo->nDepthBpp = nVal;
  nVal = m_pWBuf->BufRdAdv2(nPos, PS_BSWAP);
  PhotoshopParseReportFldEnum(nIndent, "Color mode", BIM_T_ENUM_FILE_HDR_COL_MODE, nVal);

}

// Parse the Photoshop IRB Color Mode Section (from File Header)
// INPUT:
// - nPos               = File position at start of block
// - nIndent    = Indent level for formatted output
// OUTPUT:
// - nPos               = File position after reading the block
//
void CDecodePs::PhotoshopParseColorModeSection(uint32_t &nPos, uint32_t nIndent)
{
  QString strVal;

  PhotoshopParseReportNote(nIndent, "Color Mode Data Section:");
  nIndent++;

  uint32_t nColModeLen = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);

  PhotoshopParseReportFldNum(nIndent, "Length", nColModeLen, "");

  if(nColModeLen != 0)
  {
    PhotoshopParseReportFldOffset(nIndent, "Color data", nPos);
  }

  // Skip data
  nPos += nColModeLen;
}

// -------------

// Parse the Photoshop IRB Layer and Mask Info Section (from File Header)
// INPUT:
// - nPos               = File position at start of block
// - nIndent    = Indent level for formatted output
// - pDibTemp   = DIB for image display
// OUTPUT:
// - nPos               = File position after reading the block
//
bool CDecodePs::PhotoshopParseLayerMaskInfo(uint32_t &nPos, uint32_t nIndent, QImage *pDibTemp)
{
  QString strVal;

  bool bDecOk = true;

  PhotoshopParseReportNote(nIndent, "Layer and Mask Information Section:");
  nIndent++;

  uint32_t nLayerMaskLen = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  uint32_t nPosStart = nPos;
  uint32_t nPosEnd = nPosStart + nLayerMaskLen;

  PhotoshopParseReportFldNum(nIndent, "Length", nLayerMaskLen, "");

  if(nLayerMaskLen == 0)
  {
    return true;
  }
  else
  {
    if(bDecOk)
      bDecOk &= PhotoshopParseLayerInfo(nPos, nIndent, pDibTemp);

    if(bDecOk)
      bDecOk &= PhotoshopParseGlobalLayerMaskInfo(nPos, nIndent);

    if(bDecOk)
    {
      //while ((bDecOk) && (nPos < nPosEnd)) {
      //while ((bDecOk) && ((nPos+12) < nPosEnd)) {
      while((bDecOk) && (nPosStart + nLayerMaskLen - nPos > 12))
      {
        bDecOk &= PhotoshopParseAddtlLayerInfo(nPos, nIndent);
      }
    }
  }

  if(bDecOk)
  {
    // Skip data
    nPos = nPosEnd;
  }

  return bDecOk;
}

// Parse the Photoshop IRB Layer Info Section
// INPUT:
// - nPos               = File position at start of block
// - nIndent    = Indent level for formatted output
// - pDibTemp   = DIB for image display
// OUTPUT:
// - nPos               = File position after reading the block
//
bool CDecodePs::PhotoshopParseLayerInfo(uint32_t &nPos, uint32_t nIndent, QImage * pDibTemp)
{
  QString strVal;

  bool bDecOk = true;

  PhotoshopParseReportNote(nIndent, "Layer Info:");
  nIndent++;

  uint32_t nLayerLen = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);

  PhotoshopParseReportFldNum(nIndent, "Length", nLayerLen, "");

  if(nLayerLen == 0)
  {
    return bDecOk;
  }

  // Round up length to multiple of 2
  if(nLayerLen % 2 != 0)
  {
    nLayerLen++;
  }

  // Save file position
  uint32_t nPosStart = nPos;

  // According to Adobe, "Layer count" is defined as follows:
  // - If it is a negative number, its absolute value is the number of layers and the
  //   first alpha channel contains the transparency data for the merged result.
  // Therefore, we'll treat it as signed short and take absolute value
  uint16_t nLayerCountU = m_pWBuf->BufRdAdv2(nPos, PS_BSWAP);
  int16_t nLayerCountS = static_cast<int16_t>(nLayerCountU);

  uint32_t nLayerCount = abs(nLayerCountS);

  PhotoshopParseReportFldNum(nIndent, "Layer count", nLayerCount, "");

  if(nLayerCountU & 0x8000)
  {
    PhotoshopParseReportNote(nIndent, "First alpha channel contains transparency for merged result");
  }

  tsLayerAllInfo sLayerAllInfo;

  sLayerAllInfo.nNumLayers = nLayerCount;
  sLayerAllInfo.psLayers = new tsLayerInfo[nLayerCount];
  Q_ASSERT(sLayerAllInfo.psLayers);

  for(uint32_t nLayerInd = 0; (bDecOk) && (nLayerInd < nLayerCount); nLayerInd++)
  {
    // Clear the channel array
    sLayerAllInfo.psLayers[nLayerInd].pnChanLen = nullptr;
    QString strTmp;

    strTmp = QString("Layer #%1").arg(nLayerInd);
    PhotoshopParseReportFldOffset(nIndent, strTmp, nPos);

    if(bDecOk)
      bDecOk &= PhotoshopParseLayerRecord(nPos, nIndent, &sLayerAllInfo.psLayers[nLayerInd]);
  }

  PhotoshopParseReportNote(nIndent, "Channel Image Data:");
  QString strLine;

  uint32_t nNumChans;
  uint32_t nWidth;
  uint32_t nHeight;
  uint32_t nPosLastLayer = 0;
  uint32_t nPosLastChan = 0;

  for(uint32_t nLayerInd = 0; (bDecOk) && (nLayerInd < nLayerCount); nLayerInd++)
  {
    nNumChans = sLayerAllInfo.psLayers[nLayerInd].nNumChans;
    nWidth = sLayerAllInfo.psLayers[nLayerInd].nWidth;
    nHeight = sLayerAllInfo.psLayers[nLayerInd].nHeight;

    unsigned char *pDibBits = nullptr;

#ifdef PS_IMG_DEC_EN
    if((pDibTemp) && (m_bDisplayLayer) && (nLayerInd == m_nDisplayLayerInd))
    {
      // Allocate the device-independent bitmap (DIB)
      // - Although we are creating a 32-bit DIB, it should also
      //   work to run in 16- and 8-bit modes

      // Start by killing old DIB since we might be changing its dimensions
//@@      pDibTemp->Kill();

      // Allocate the new DIB
//@@      bool bDibOk = pDibTemp->CreateDIB(nWidth, nHeight, 32);

      // Should only return false if already exists or failed to allocate
//@@      if(bDibOk)
      {
        // Fetch the actual pixel array
//@@        pDibBits = (unsigned char *) (pDibTemp->GetDIBBitArray());
      }
    }
#endif

    nPosLastLayer = nPos;

    for(uint32_t nChanInd = 0; (bDecOk) && (nChanInd < nNumChans); nChanInd++)
    {
      nPosLastChan = nPos;
      strLine = QString("Layer %1/%2, Channel %3/%4")
          .arg(nLayerInd + 1, 3)
          .arg(nLayerCount, 3)
          .arg(nChanInd + 1, 2).arg(nNumChans, 2);
      //m_pLog->AddLine(strLine);
      PhotoshopParseReportNote(nIndent + 1, strLine);

      if(bDecOk)
      {
        // Fetch the channel ID to determine what RGB channel to map to
        // - Layer 0 (background):           0->R, 1->G, 2->B
        // - Layer n (other):      65535->A, 0->R, 1->G, 2->B
        uint32_t nChanID = sLayerAllInfo.psLayers[nLayerInd].pnChanID[nChanInd];

        // Parse, and optionally paint the DIB
        // NOTE: For uncompressed channels, the width & height generally appear to be zero
        //       According to the spec:
        //       If the compression code is 0, the image data is just the raw image data,
        //       whose size is calculated as (LayerBottom-LayerTop)* (LayerRight-LayerLeft)

        bDecOk &= PhotoshopParseChannelImageData(nPos, nIndent + 1, nWidth, nHeight, nChanID, pDibBits);
      }

      strLine = QString("CurPos @ 0x%1, bDecOk=%2, LastLayer @ 0x%3, LastChan @ 0x%4")
          .arg(nPos, 8, 16, QChar('0'))
          .arg(bDecOk).arg(nPosLastLayer, 8, 16, QChar('0'))
          .arg(nPosLastChan, 8, 16, QChar('0'));
      //m_pLog->AddLine(strLine);
      //PhotoshopParseReportNote(nIndent+1,strLine);
    }
  }

  // Pad out to specified length
  int32_t nPad;

  nPad = nPosStart + nLayerLen - nPos;

  if(nPad > 0)
  {
    nPos += nPad;
  }

  // Deallocate
  if(sLayerAllInfo.psLayers)
  {
    for(uint32_t nLayerInd = 0; (nLayerInd < sLayerAllInfo.nNumLayers); nLayerInd++)
    {
      nNumChans = sLayerAllInfo.psLayers[nLayerInd].nNumChans;

      for(uint32_t nChanInd = 0; (bDecOk) && (nChanInd < nNumChans); nChanInd++)
      {
        delete[]sLayerAllInfo.psLayers[nLayerInd].pnChanLen;
        sLayerAllInfo.psLayers[nLayerInd].pnChanLen = nullptr;
        delete[]sLayerAllInfo.psLayers[nLayerInd].pnChanID;
        sLayerAllInfo.psLayers[nLayerInd].pnChanID = nullptr;
      }
    }

    delete[]sLayerAllInfo.psLayers;
    sLayerAllInfo.psLayers = nullptr;
  }

  return bDecOk;
}

// Parse the Photoshop IRB Layer Record
// INPUT:
// - nPos               = File position at start of block
// - nIndent    = Indent level for formatted output
// OUTPUT:
// - nPos               = File position after reading the block
//
bool CDecodePs::PhotoshopParseLayerRecord(uint32_t &nPos, uint32_t nIndent, tsLayerInfo * pLayerInfo)
{
  QString strVal;

  bool bDecOk = true;

  Q_ASSERT(pLayerInfo);

  PhotoshopParseReportNote(nIndent, "Layer Record:");
  nIndent++;

  uint32_t nRect1, nRect2, nRect3, nRect4;

  nRect1 = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  nRect2 = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  nRect3 = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  nRect4 = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Rect Top", nRect1, "");
  PhotoshopParseReportFldNum(nIndent, "Rect Left", nRect2, "");
  PhotoshopParseReportFldNum(nIndent, "Rect Bottom", nRect3, "");
  PhotoshopParseReportFldNum(nIndent, "Rect Right", nRect4, "");

  pLayerInfo->nHeight = nRect3 - nRect1;
  pLayerInfo->nWidth = nRect4 - nRect2;
  //nHeight = nRect3-nRect1;

  uint32_t nNumChans = m_pWBuf->BufRdAdv2(nPos, PS_BSWAP);

  PhotoshopParseReportFldNum(nIndent, "Number of channels", nNumChans, "");

  //nChans = nNumChans;
  pLayerInfo->nNumChans = nNumChans;

  Q_ASSERT(pLayerInfo->pnChanLen == nullptr);
  pLayerInfo->pnChanLen = new uint32_t[nNumChans];

  Q_ASSERT(pLayerInfo->pnChanLen);
  pLayerInfo->pnChanID = new uint32_t[nNumChans];

  Q_ASSERT(pLayerInfo->pnChanID);

  for(uint32_t nChanInd = 0; nChanInd < nNumChans; nChanInd++)
  {
    uint32_t nChanID = m_pWBuf->BufRdAdv2(nPos, PS_BSWAP);

    uint32_t nChanDataLen = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);

    QString strField;

    strField = QString("Channel index #%1").arg(nChanInd);
    strVal = QString("ID=%1 DataLength=0x%2").arg(nChanID, 5).arg(nChanDataLen, 8, 16, QChar('0'));
    PhotoshopParseReportFldStr(nIndent, strField, strVal);
    pLayerInfo->pnChanID[nChanInd] = nChanID;
    pLayerInfo->pnChanLen[nChanInd] = nChanDataLen;
  }

  QString strBlendModeSig = m_pWBuf->BufReadStrn(nPos, 4);

  nPos += 4;
  PhotoshopParseReportFldStr(nIndent, "Blend mode signature", strBlendModeSig);
  uint32_t nBlendModeKey = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);

  PhotoshopParseReportFldEnum(nIndent, "Blend mode key", BIM_T_ENUM_BLEND_MODE_KEY, nBlendModeKey);
  uint32_t nOpacity = m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);

  PhotoshopParseReportFldNum(nIndent, "Opacity", nOpacity, "(0=transparent ... 255=opaque)");
  m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);   // unsigned nClipping
  m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);   // unsigned nFlags
  m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);   // unsigned nFiller
  uint32_t nExtraDataLen = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);

  uint32_t nPosExtra = nPos;

  if(bDecOk)
    bDecOk &= PhotoshopParseLayerMask(nPos, nIndent);

  if(bDecOk)
    bDecOk &= PhotoshopParseLayerBlendingRanges(nPos, nIndent);

  if(bDecOk)
  {
    uint32_t nLayerNameLen = m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);

    QString strLayerName = m_pWBuf->BufReadStrn(nPos, nLayerNameLen);

    nPos += nLayerNameLen;
    // According to spec, length is padded to multiple of 4 bytes,
    uint32_t nSkip = 0;

    nSkip = (4 - ((1 + nLayerNameLen) % 4)) % 4;
    nPos += nSkip;
  }

  // Now use up the "extra bytes" with "Additional Layer Info"
  while((bDecOk) && (nPos < nPosExtra + nExtraDataLen))
  {
    if(bDecOk)
      bDecOk &= PhotoshopParseAddtlLayerInfo(nPos, nIndent);
  }

  return bDecOk;
}

// Parse the Photoshop IRB Layer Mask
// INPUT:
// - nPos               = File position at start of block
// - nIndent    = Indent level for formatted output
// OUTPUT:
// - nPos               = File position after reading the block
//
bool CDecodePs::PhotoshopParseLayerMask(uint32_t &nPos, uint32_t nIndent)
{
  QString strVal;

  bool bDecOk = true;

  PhotoshopParseReportNote(nIndent, "Layer Mask / Adjustment layer data:");
  nIndent++;

  uint32_t nLayerMaskLen = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);

  if(nLayerMaskLen == 0)
  {
    return bDecOk;
  }

  uint32_t nRectA1, nRectA2, nRectA3, nRectA4;
  uint32_t nRectB1, nRectB2, nRectB3, nRectB4;

  nRectA1 = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  nRectA2 = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  nRectA3 = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  nRectA4 = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);

  uint32_t nTmp;

  m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);   // unsigned nDefaultColor
  uint32_t nFlags = m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);

  //FIXME: Is this correct?
  if(nLayerMaskLen == 20)
  {
    m_pWBuf->BufRdAdv2(nPos, PS_BSWAP); // unsigned nPad
  }

  if(nFlags & (1 << 4))
  {
    uint32_t nMaskParams = m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);

    if(nMaskParams & (1 << 0))
    {
      // User mask density, 1 byte
      nTmp = m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);
    }

    if(nMaskParams & (1 << 1))
    {
      // User mask feather, 8 bytes, double
      nTmp = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
      nTmp = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
    }

    if(nMaskParams & (1 << 2))
    {
      // Vector mask density, 1 byte
      nTmp = m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);
    }

    if(nMaskParams & (1 << 3))
    {
      // Vector mask feather, 8 bytes, double
      nTmp = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
      nTmp = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
    }

    m_pWBuf->BufRdAdv2(nPos, PS_BSWAP); // unsigned nPadding
    m_pWBuf->BufRdAdv1(nPos, PS_BSWAP); // unsigned nRealFlags
    m_pWBuf->BufRdAdv1(nPos, PS_BSWAP); // unsigned nRealUserMaskBackground
    nRectB1 = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
    nRectB2 = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
    nRectB3 = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
    nRectB4 = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  }

  return bDecOk;
}

// Parse the Photoshop IRB Layer blending ranges data
// INPUT:
// - nPos               = File position at start of block
// - nIndent    = Indent level for formatted output
// OUTPUT:
// - nPos               = File position after reading the block
//
bool CDecodePs::PhotoshopParseLayerBlendingRanges(uint32_t &nPos, uint32_t nIndent)
{
  QString strVal;

  bool bDecOk = true;

  PhotoshopParseReportNote(nIndent, "Layer blending ranges data:");
  nIndent++;

  uint32_t nLayerBlendLen = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);

  if(nLayerBlendLen == 0)
  {
    return bDecOk;
  }

  m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);   // unsigned nCompGrayBlendSrc
  m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);   // unsigned nCompGrayBlendDstRng
  uint32_t nNumChans;

  nNumChans = (nLayerBlendLen - 8) / 8;

  for(uint32_t nChanInd = 0; nChanInd < nNumChans; nChanInd++)
  {
    m_pWBuf->BufRdAdv4(nPos, PS_BSWAP); // unsigned nSrcRng
    m_pWBuf->BufRdAdv4(nPos, PS_BSWAP); // unsigned nDstRng
  }

  return bDecOk;
}

// Parse the Photoshop IRB Channel image data
// INPUT:
// - nPos               = File position at start of block
// - nIndent    = Indent level for formatted output
// - nChan      = Channel being parsed (used for DIB mapping)
// - pDibTemp = DIB for rendering (NULL if disabled)
// OUTPUT:
// - nPos               = File position after reading the block
//
bool CDecodePs::PhotoshopParseChannelImageData(uint32_t &nPos, uint32_t nIndent, uint32_t nWidth, uint32_t nHeight,
                                               uint32_t nChan, unsigned char *pDibBits)
{
  bool bDecOk = true;

  uint32_t nCompressionMethod = m_pWBuf->BufRdAdv2(nPos, PS_BSWAP);

  PhotoshopParseReportFldNum(nIndent + 1, "Compression method", nCompressionMethod, "");

  if(nCompressionMethod == 1)
  {
    // *** RLE Compression ***
    if(nHeight == 0)
    {
      return true;
    }

    // Read the line lengths into an array
    // -   LOOP [nHeight]
    // -     16-bit: row length
    // -   ENDLOOP
    uint32_t *anRowLen;

    anRowLen = new uint32_t[nHeight];

    Q_ASSERT(anRowLen);

    for(uint32_t nRow = 0; nRow < nHeight; nRow++)
    {
      anRowLen[nRow] = m_pWBuf->BufRdAdv2(nPos, PS_BSWAP);
    }

    // Read the compressed data
    for(uint32_t nRow = 0; (bDecOk) && (nRow < nHeight); nRow++)
    {
      uint32_t nRowLen = anRowLen[nRow];
      bDecOk = PhotoshopDecodeRowRle(nPos, nWidth, nHeight, nRow, nRowLen, nChan, pDibBits);
    }                           //nRow

    // Deallocate
    if(anRowLen)
    {
      delete[]anRowLen;
      anRowLen = nullptr;
    }
  }
  else if(nCompressionMethod == 0)
  {
    // *** RAW (no compression)
    if(nHeight == 0)
    {
      return true;
    }

    for(uint32_t nRow = 0; (bDecOk) && (nRow < nHeight); nRow++)
    {
      bDecOk = PhotoshopDecodeRowUncomp(nPos, nWidth, nHeight, nRow, nChan, pDibBits);
    }
  }
  else
  {
    m_pLog->AddLineWarn("Unsupported compression method. Stopping.");
    bDecOk = false;
    return bDecOk;
  }

  return bDecOk;
}

bool CDecodePs::PhotoshopDecodeRowUncomp(uint32_t &nPos, uint32_t nWidth, uint32_t nHeight, uint32_t nRow, uint32_t nChanID,
                                         unsigned char *pDibBits)
{
  bool bDecOk = true;

  unsigned char nVal;

  uint32_t nRowActual;

  uint32_t nPixByte;

  for(uint32_t nCol = 0; (bDecOk) && (nCol < nWidth); nCol++)
  {
    nVal = m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);

#ifdef PS_IMG_DEC_EN
    if(pDibBits)
    {
      nRowActual = nHeight - nRow - 1;  // Need to flip vertical for DIB
      nPixByte = (nRowActual * nWidth + nCol) * sizeof(QRgb);

      // Assign the RGB pixel map
      pDibBits[nPixByte + 3] = 0;

      if(nChanID == 0)
      {
        pDibBits[nPixByte + 2] = nVal;
      }
      else if(nChanID == 1)
      {
        pDibBits[nPixByte + 1] = nVal;
      }
      else if(nChanID == 2)
      {
        pDibBits[nPixByte + 0] = nVal;
      }
      else
      {
      }
    }                           // pDibBits
#endif
  }                             // nCol

  return bDecOk;
}

bool CDecodePs::PhotoshopDecodeRowRle(uint32_t &nPos, uint32_t nWidth, uint32_t nHeight, uint32_t nRow, uint32_t nRowLen,
                                      uint32_t nChanID, unsigned char *pDibBits)
{
  bool bDecOk = true;

  char nRleRunS;

  unsigned char nRleRun;
  unsigned char nRleVal;

  uint32_t nRleRunCnt;
  uint32_t nRowOffsetComp;      // Row offset (compressed size)
  uint32_t nRowOffsetDecomp;    // Row offset (decompressed size)
  uint32_t nRowOffsetDecompLast;
  uint32_t nRowActual;
  uint32_t nPixByte;

  // Decompress the row data
  nRowOffsetComp = 0;
  nRowOffsetDecomp = 0;
  nRowOffsetDecompLast = 0;

  while((bDecOk) && (nRowOffsetComp < nRowLen))
  {
    // Save the position in the row before decompressing
    // the current RLE encoded entry
    nRowOffsetDecompLast = nRowOffsetDecomp;

    nRleRun = m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);
    nRleRunS = static_cast<char>(nRleRun);
    nRowOffsetComp++;

    if(nRleRunS < 0)
    {
      // Replicate the next byte
      nRleRunCnt = 1 - nRleRunS;
      nRleVal = m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);
      nRowOffsetComp++;
      nRowOffsetDecomp += nRleRunCnt;

#ifdef PS_IMG_DEC_EN
      if(pDibBits)
      {
        nRowActual = nHeight - nRow - 1;        // Need to flip vertical for DIB

        for(uint32_t nRunInd = 0; nRunInd < nRleRunCnt; nRunInd++)
        {
          nPixByte = (nRowActual * nWidth + nRowOffsetDecompLast + nRunInd) * sizeof(QRgb);

          // Assign the RGB pixel map
          pDibBits[nPixByte + 3] = 0;

          if(nChanID == 0)
          {
            pDibBits[nPixByte + 2] = nRleVal;
          }
          else if(nChanID == 1)
          {
            pDibBits[nPixByte + 1] = nRleVal;
          }
          else if(nChanID == 2)
          {
            pDibBits[nPixByte + 0] = nRleVal;
          }
          else
          {
          }
        }                       // nRunInd
      }                         // pDibBits
#endif
    }
    else
    {
      // Copy the next bytes as-is
      nRleRunCnt = 1 + nRleRunS;

      for(uint32_t nRunInd = 0; nRunInd < nRleRunCnt; nRunInd++)
      {
        nRleVal = m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);
        nRowOffsetComp++;
        nRowOffsetDecomp++;

#ifdef PS_IMG_DEC_EN
        if(pDibBits)
        {
          nRowActual = nHeight - nRow - 1;      // Need to flip vertical for DIB
          nPixByte = (nRowActual * nWidth + nRowOffsetDecompLast + nRunInd) * sizeof(QRgb);

          // Assign the RGB pixel map
          pDibBits[nPixByte + 3] = 0;

          if(nChanID == 0)
          {
            pDibBits[nPixByte + 2] = nRleVal;
          }
          else if(nChanID == 1)
          {
            pDibBits[nPixByte + 1] = nRleVal;
          }
          else if(nChanID == 2)
          {
            pDibBits[nPixByte + 0] = nRleVal;
          }
          else
          {
          }
        }                       // pDibBits
#endif

      }                         // nRunInd
    }                           // nRleRunS
  }                             // nRowOffsetComp

  // Now that we've finished the row, compare the decompressed size
  // to the expected width
  if(nRowOffsetDecomp != nWidth)
  {
    bDecOk = false;
    Q_ASSERT(false);
  }

  return bDecOk;
}

// Parse the Photoshop IRB Layer and Mask Info Section (from File Header)
// INPUT:
// - nPos               = File position at start of block
// - nIndent    = Indent level for formatted output
// OUTPUT:
// - nPos               = File position after reading the block
//
// NOTE:
// - Image decoding (into DIB) is enabled if pDibBits is not NULL
//
bool CDecodePs::PhotoshopParseImageData(uint32_t &nPos, uint32_t nIndent, tsImageInfo * psImageInfo, unsigned char *pDibBits)
{
  Q_ASSERT(psImageInfo);

  QString strVal;

  //PhotoshopParseReportNote(nIndent,"Image data section:");
  PhotoshopParseReportFldOffset(nIndent, "Image data section:", nPos);

  bool bDecOk = true;

  //----
  uint32_t nWidth = psImageInfo->nImageWidth;
  uint32_t nHeight = psImageInfo->nImageHeight;
  uint32_t nNumChans = psImageInfo->nNumChans;

  //----

  uint32_t nCompressionMethod = m_pWBuf->BufRdAdv2(nPos, PS_BSWAP);

  PhotoshopParseReportFldNum(nIndent + 1, "Compression method", nCompressionMethod, "");

  // -----------------------------------------------------

  if(nCompressionMethod == 1)
  {
    // *** RLE Compression ***
    if(nHeight == 0)
    {
      return true;
    }

    // Read the line lengths into an array
    // - LOOP [nNumChans]
    // -   LOOP [nHeight]
    // -     16-bit: row length
    // -   ENDLOOP
    // - ENDLOOP
    uint32_t *anRowLen;

    anRowLen = new uint32_t[nNumChans * nHeight];

    Q_ASSERT(anRowLen);

    for(uint32_t nRow = 0; nRow < (nNumChans * nHeight); nRow++)
    {
      anRowLen[nRow] = m_pWBuf->BufRdAdv2(nPos, PS_BSWAP);
    }

    // Read the compressed data
    for(uint32_t nChan = 0; nChan < nNumChans; nChan++)
    {
      for(uint32_t nRow = 0; (bDecOk) && (nRow < nHeight); nRow++)
      {
        uint32_t nRowLen = anRowLen[(nChan * nHeight) + nRow];

        bDecOk = PhotoshopDecodeRowRle(nPos, nWidth, nHeight, nRow, nRowLen, nChan, pDibBits);
      }                         // nRow
    }                           // nChan

    // Deallocate
    if(anRowLen)
    {
      delete[]anRowLen;
      anRowLen = nullptr;
    }
  }
  else if(nCompressionMethod == 0)
  {
    // *** RAW (no compression)

    // NOTE: BUG BUG BUG
    // It seems that most examples I see with RAW layers have nHeight=nWidth=0
    // or else the nChan=0xFFFF. Why aren't width & height defined?

    if(nHeight * nNumChans == 0)
    {
      return true;
    }

    for(uint32_t nChan = 0; nChan < nNumChans; nChan++)
    {
      for(uint32_t nRow = 0; (bDecOk) && (nRow < nHeight); nRow++)
      {
        bDecOk = PhotoshopDecodeRowUncomp(nPos, nWidth, nHeight, nRow, nChan, pDibBits);
      }                         //nRow
    }                           //nChan
  }
  else
  {
    m_pLog->AddLineWarn("Unsupported compression method. Stopping.");
    bDecOk = false;
    return bDecOk;
  }

  return bDecOk;
}

// Parse the Photoshop IRB Global layer mask info
// INPUT:
// - nPos               = File position at start of block
// - nIndent    = Indent level for formatted output
// OUTPUT:
// - nPos               = File position after reading the block
//
bool CDecodePs::PhotoshopParseGlobalLayerMaskInfo(uint32_t &nPos, uint32_t nIndent)
{
  QString strVal;

  bool bDecOk = true;

  PhotoshopParseReportNote(nIndent, "Global layer mask info:");
  nIndent++;

  uint32_t nInfoLen = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);

  if(nInfoLen == 0)
  {
    return bDecOk;
  }
  uint32_t nPosStart = nPos;

  m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);   // unsigned nOverlayColorSpace
  m_pWBuf->BufRdAdv2(nPos, PS_BSWAP);   // unsigned nColComp1
  m_pWBuf->BufRdAdv2(nPos, PS_BSWAP);   // unsigned nColComp2
  m_pWBuf->BufRdAdv2(nPos, PS_BSWAP);   // unsigned nColComp3
  m_pWBuf->BufRdAdv2(nPos, PS_BSWAP);   // unsigned nColComp4
  m_pWBuf->BufRdAdv2(nPos, PS_BSWAP);   // unsigned nOpacity
  m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);   // unsigned nKind

  // Variable Filler: zeros
  //nPos += nInfoLen - 17;
  nPos = nPosStart + nInfoLen;

  return bDecOk;
}

// Parse the Photoshop IRB Additional layer info
// INPUT:
// - nPos               = File position at start of block
// - nIndent    = Indent level for formatted output
// OUTPUT:
// - nPos               = File position after reading the block
//
bool CDecodePs::PhotoshopParseAddtlLayerInfo(uint32_t &nPos, uint32_t nIndent)
{
  bool bDecOk = true;

  uint32_t nPosStart;

  PhotoshopParseReportNote(nIndent, "Additional layer info:");
  nIndent++;

  QString strSig = m_pWBuf->BufReadStrn(nPos, 4);

  nPos += 4;

  if(strSig != "8BIM")
  {
    // Signature did not match!
    QString strError;

    strError = QString("ERROR: Addtl Layer Info signature unknown [%1] @ 0x%2").arg(strSig).arg(nPos - 4, 8, 16, QChar('0'));
    PhotoshopParseReportNote(nIndent, strError);

#ifdef DEBUG_LOG
    QString strDebug;

    strDebug =
      QString("## File=[%1] Block=[%2] Error=[%3]\n").arg(m_pAppConfig->strCurFname, -100).arg("PsDecode", -10).arg(strError);
    qDebug() << (strDebug);
#else
    Q_ASSERT(false);
#endif
    bDecOk = false;
    return false;
  }

  QString strKey = m_pWBuf->BufReadStrn(nPos, 4);

  nPos += 4;
  PhotoshopParseReportFldStr(nIndent, "Key", strKey);
  bool bLen8B = false;

  if(strKey == "Lr16")
  {
    // FIXME: Might need some additional processing here?
    Q_ASSERT(false);
  }
  /*
     //CAL! FIXME: Empirically, I didn't see 8B length being used by "FMsk" in psd-flame-icon.psd @ 0x7A458
     if (strKey == "LMsk") { bLen8B = true; }
     if (strKey == "Lr16") { bLen8B = true; }
     if (strKey == "Lr32") { bLen8B = true; }
     if (strKey == "Layr") { bLen8B = true; }
     if (strKey == "Mt16") { bLen8B = true; }
     if (strKey == "Mt32") { bLen8B = true; }
     if (strKey == "Mtrn") { bLen8B = true; }
     if (strKey == "Alph") { bLen8B = true; }
     if (strKey == "FMsk") { bLen8B = true; }
     if (strKey == "lnk2") { bLen8B = true; }
     if (strKey == "FEid") { bLen8B = true; }
     if (strKey == "FXid") { bLen8B = true; }
     if (strKey == "PxSD") { bLen8B = true; }
   */

  uint32_t nLen;

  if(!bLen8B)
  {
    nLen = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  }
  else
  {
    // DUMMY for now
    //FIXME: Untested
    nLen = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
    Q_ASSERT(nLen == 0);
    nLen = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  }
  PhotoshopParseReportFldNum(nIndent, "Length", nLen, "");
  if(nLen > 0)
  {
    PhotoshopParseReportFldHex(nIndent, strKey, nPos, nLen);
  }

  uint32_t nVal;

  QString sStr;

  uint32_t nPosOffset = 0;

  if(strKey == "luni")
  {
    sStr = PhotoshopParseGetBimLStrUni(nPos, nPosOffset);
    PhotoshopParseReportFldStr(nIndent, "Layer Name (Unicode)", sStr);
  }
  else if(strKey == "lnsr")
  {
    nVal = m_pWBuf->BufX(nPos, 4, PS_BSWAP);
    PhotoshopParseReportFldNum(nIndent, "Layer Name Source ID", nVal, "");
  }
  else if(strKey == "lyid")
  {
    nVal = m_pWBuf->BufX(nPos, 4, PS_BSWAP);
    PhotoshopParseReportFldNum(nIndent, "Layer ID", nVal, "");
  }
  else if(strKey == "clbl")
  {
    nVal = m_pWBuf->BufX(nPos, 4, PS_BSWAP);
    PhotoshopParseReportFldBool(nIndent, "Blend clipped elements", nVal);
  }
  else if(strKey == "infx")
  {
    nVal = m_pWBuf->BufX(nPos, 4, PS_BSWAP);
    PhotoshopParseReportFldBool(nIndent, "Blend interior elements", nVal);
  }
  else if(strKey == "knko")
  {
    nVal = m_pWBuf->BufX(nPos, 4, PS_BSWAP);
    PhotoshopParseReportFldBool(nIndent, "Knockout", nVal);
  }

  nPosStart = nPos;

  // Skip data
  nPos = nPosStart + nLen;

  // FIXME: It appears that we need to pad out to ensure that
  // length is multiple of 4 (not file position).
  uint32_t nMod;

  nMod = (nLen % 4);
  if(nMod != 0)
  {
    nPos += (4 - nMod);
  }

  return bDecOk;
}

// -------------

// Parse the Photoshop IRB Image Resources Section
// - Iteratively works through the individual Image Resource Blocks (IRB)
//
// INPUT:
// - nPos               = File position at start of block
// - nIndent    = Indent level for formatted output
// OUTPUT:
// - nPos               = File position after reading the block
//
bool CDecodePs::PhotoshopParseImageResourcesSection(uint32_t &nPos, uint32_t nIndent)
{
  QString strVal;

  bool bDecOk = true;

  PhotoshopParseReportNote(nIndent, "Image Resources Section:");
  nIndent++;

  uint32_t nPosSectionStart = 0;

  uint32_t nPosSectionEnd = 0;

  uint32_t nImgResLen = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);

  PhotoshopParseReportFldNum(nIndent, "Length", nImgResLen, "");

  nPosSectionStart = nPos;
  nPosSectionEnd = nPos + nImgResLen;

  while(nPos < nPosSectionEnd)
  {
    bDecOk &= PhotoshopParseImageResourceBlock(nPos, nIndent);
    if(!bDecOk)
    {
      return false;
    }
  }
  return bDecOk;
}

// Parse the Photoshop IRB (Image Resource Block)
// - Decode the 8BIM ID and invoke the appropriate IRB parsing handler
//
// INPUT:
// - nPos               = File position at start of block
// - nIndent    = Indent level for formatted output
// OUTPUT:
// - nPos               = File position after reading the block
//
bool CDecodePs::PhotoshopParseImageResourceBlock(uint32_t &nPos, uint32_t nIndent)
{
  QString strVal;

  //bool          bDecOk = true;

  //PhotoshopParseReportNote(nIndent,"Image Resource Block:");
  //nIndent++;

  //
  // - Signature  (4B)                                            "8BIM"
  // - NameLen    (2B)                                            Generally 0
  // - Name               (NameLen Bytes, min 1 Byte)     Usually empty (NULL)
  // - BimLen             (4B)                                            Length of 8BIM data
  // - Data               (1..BimLen)                                     Field data
  //

  QString strSig = m_pWBuf->BufReadStrn(nPos, 4);

  nPos += 4;

  if(strSig != "8BIM")
  {
    // Signature did not match!
    QString strError;

    strError = QString("ERROR: IRB signature unknown [%1]").arg(strSig);
    PhotoshopParseReportNote(nIndent, strError);

#ifdef DEBUG_LOG
    QString strDebug;

    strDebug =
      QString("## File=[%1] Block=[%2] Error=[%3]\n")
        .arg(m_pAppConfig->strCurFname, -100)
        .arg("PsDecode", -10)
        .arg(strError);
    qDebug() << (strDebug);
#else
    Q_ASSERT(false);
#endif

    return false;;
  }

  uint32_t nBimId = m_pWBuf->BufRdAdv2(nPos, PS_BSWAP);
  uint32_t nResNameLen = m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);

  QString strResName = m_pWBuf->BufReadStrn(nPos, nResNameLen);

  nPos += nResNameLen;

  // If size wasn't even, pad here
  if((1 + nResNameLen) % 2 != 0)
  {
    nPos += 1;
  }

  uint32_t nBimLen = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);

  QString strTmp;
  QString strBimName;
  QString strByte;

  // Lookup 8BIM defined name
  QString strBimDefName = "";

  bool bBimKnown = false;

  uint32_t nFldInd = 0;

  bBimKnown = FindBimRecord(nBimId, nFldInd);

  if(bBimKnown)
  {
    strBimDefName = asBimRecords[nFldInd].strRecordName;
  }

  strTmp = QString("8BIM: [0x%1] Name=\"%2\" Len=[0x%3] DefinedName=\"%4\"")
      .arg(nBimId, 4, 16, QChar('0'))
      .arg(strBimName)
      .arg(nBimLen, 4, 16, QChar('0'))
      .arg(strBimDefName);
  PhotoshopParseReportNote(nIndent, strTmp);

  nIndent++;

  if(nBimLen == 0)
  {
    // If length is zero, then skip this block
    PhotoshopParseReportNote(nIndent, "Length is zero. Skipping.");
  }
  else if(bBimKnown)
  {
    // Save the file pointer
    uint32_t nPosSaved;

    nPosSaved = nPos;

    // Calculate the end of the record
    // - This is used for parsing records that have a conditional parsing
    //   of additional fields "if length permits".
    uint32_t nPosEnd;

    nPosEnd = nPos + nBimLen - 1;

    switch (asBimRecords[nFldInd].eBimType)
    {

      case BIM_T_STR:
        strVal = DecodeIptcValue(IPTC_T_STR, nBimLen, nPos);
        PhotoshopParseReportFldStr(nIndent, strBimDefName, strVal);
        break;
      case BIM_T_HEX:
        PhotoshopParseReportFldHex(nIndent, strBimDefName, nPos, nBimLen);
        nPos += nBimLen;
        break;

      case BIM_T_PS_THUMB_RES:
        PhotoshopParseThumbnailResource(nPos, nIndent);
        // Since this block has image data that we are not going to parse,
        // force the end position to match the length
        nPos = nPosSaved + nBimLen;
        break;
      case BIM_T_PS_SLICES:
        PhotoshopParseSliceHeader(nPos, nIndent, nPosEnd);
        break;
      case BIM_T_PS_DESCRIPTOR:
        PhotoshopParseDescriptor(nPos, nIndent);
        break;
      case BIM_T_PS_VER_INFO:
        PhotoshopParseVersionInfo(nPos, nIndent);
        break;
      case BIM_T_PS_PRINT_SCALE:
        PhotoshopParsePrintScale(nPos, nIndent);
        break;
      case BIM_T_PS_PIXEL_ASPECT_RATIO:
        PhotoshopParsePixelAspectRatio(nPos, nIndent);
        break;
      case BIM_T_PS_DOC_SPECIFIC_SEED:
        PhotoshopParseDocSpecificSeed(nPos, nIndent);
        break;
      case BIM_T_PS_RESOLUTION_INFO:
        PhotoshopParseResolutionInfo(nPos, nIndent);
        break;
      case BIM_T_PS_GRID_GUIDES:
        PhotoshopParseGridGuides(nPos, nIndent);
        break;
      case BIM_T_PS_GLOBAL_ANGLE:
        PhotoshopParseGlobalAngle(nPos, nIndent);
        break;
      case BIM_T_PS_GLOBAL_ALTITUDE:
        PhotoshopParseGlobalAltitude(nPos, nIndent);
        break;
      case BIM_T_PS_PRINT_FLAGS:
        PhotoshopParsePrintFlags(nPos, nIndent);
        break;
      case BIM_T_PS_PRINT_FLAGS_INFO:
        PhotoshopParsePrintFlagsInfo(nPos, nIndent);
        break;
      case BIM_T_PS_COPYRIGHT_FLAG:
        PhotoshopParseCopyrightFlag(nPos, nIndent);
        break;
      case BIM_T_PS_LAYER_STATE_INFO:
        PhotoshopParseLayerStateInfo(nPos, nIndent);
        break;
      case BIM_T_PS_LAYER_GROUP_INFO:
        PhotoshopParseLayerGroupInfo(nPos, nIndent, nBimLen);
        break;
      case BIM_T_PS_LAYER_GROUP_ENABLED:
        PhotoshopParseLayerGroupEnabled(nPos, nIndent, nBimLen);
        break;
      case BIM_T_PS_LAYER_SELECT_ID:
        PhotoshopParseLayerSelectId(nPos, nIndent);
        break;

      case BIM_T_PS_STR_UNI:
        PhotoshopParseStringUni(nPos, nIndent);
        break;
      case BIM_T_PS_STR_ASC:
        strVal += m_pWBuf->BufReadStrn(nPos, nBimLen);
        nPos += nBimLen;
        PhotoshopParseReportFldStr(nIndent, strBimDefName, strVal);
        break;
      case BIM_T_PS_STR_ASC_LONG:
        strVal = "\n";
        strVal += m_pWBuf->BufReadStrn(nPos, nBimLen);
        nPos += nBimLen;
        PhotoshopParseReportFldStr(nIndent, strBimDefName, strVal);
        break;

      case BIM_T_JPEG_QUAL:    // (0x0406)
        // JPEG Quality
        PhotoshopParseJpegQuality(nPos, nIndent, nPosEnd);
        break;

      case BIM_T_IPTC_NAA:     // (0x0404)
        // IPTC-NAA
        DecodeIptc(nPos, nBimLen, nIndent);
        break;

      default:
        return false;
        break;
    }                           // switch

    // Check to see if we ended up with a length mismatch after decoding
    if(nPos > nPosEnd + 1)
    {
      // Length mismatch detected: we read too much (versus length)
      strTmp = QString("ERROR: Parsing exceeded expected length. Stopping decode. BIM=[%1], CurPos=[0x%2], ExpPosEnd=[0x%3], ExpLen=[%4]")
          .arg(strBimDefName)
          .arg(nPos, 8, 16, QChar('0'))
          .arg(nPosEnd + 1, 8, 16, QChar('0'))
          .arg(nBimLen);
      m_pLog->AddLineErr(strTmp);
#ifdef DEBUG_LOG
      QString strDebug;

      strDebug = QString("## File=[%1] Block=[%2] Error=[%3]\n")
          .arg(m_pAppConfig->strCurFname, -100)
          .arg("PsDecode", -10)
          .arg(strTmp);
      qDebug() << (strDebug);
#else
      Q_ASSERT(false);
#endif
      // TODO: Add interactive error message here

      // Now we should roll-back the file position to the position indicated
      // by the length. This will help us stay on track for next decoding.
      nPos = nPosEnd + 1;

      return false;
    }
    else if(nPos != nPosEnd + 1)
    {
      // Length mismatch detected: we read too little (versus length)
      // This is generally an indication that either I haven't accurately captured the
      // specific block parsing format or else the specification is loose.
      strTmp = QString("WARNING: Parsing offset length mismatch. Current pos=[0x%1], expected end pos=[0x%2], expect length=[%3]")
          .arg(nPos, 8, 16, QChar('0'))
          .arg(nPosEnd + 1, 8, 16, QChar('0'))
          .arg(nBimLen);
      m_pLog->AddLineWarn(strTmp);
#ifdef DEBUG_LOG
      QString strDebug;

      strDebug = QString("## File=[%1] Block=[%2] Error=[%3]\n")
          .arg(m_pAppConfig->strCurFname, -100)
          .arg("PsDecode", -10)
          .arg(strTmp);
      qDebug() << (strDebug);
#endif
      return false;
    }

    // Reset the file marker
    nPos = nPosSaved;
  }                             // bBimKnown

  // Skip rest of 8BIM
  nPos += nBimLen;

  // FIXME: correct to make for even parity?
  if((nBimLen % 2) != 0)
    nPos++;

  return true;
}

// Parse the Photoshop IRB Slice Header
// INPUT:
// - nPos               = File position at start of block
// - nIndent    = Indent level for formatted output
// - nPosEnd    = File position at end of block (last byte address)
// OUTPUT:
// - nPos               = File position after reading the block
//
void CDecodePs::PhotoshopParseSliceHeader(uint32_t &nPos, uint32_t nIndent, uint32_t nPosEnd)
{
  uint32_t nVal;

  QString strVal;

  uint32_t nPosOffset;

  PhotoshopParseReportNote(nIndent, "Slice Header:");
  nIndent++;

  uint32_t nSliceVer = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);

  PhotoshopParseReportFldNum(nIndent, "Version", nSliceVer, "");

  if(nSliceVer == 6)
  {
    nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
    PhotoshopParseReportFldNum(nIndent, "Bound Rect (top)", nVal, "");
    nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
    PhotoshopParseReportFldNum(nIndent, "Bound Rect (left)", nVal, "");
    nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
    PhotoshopParseReportFldNum(nIndent, "Bound Rect (bottom)", nVal, "");
    nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
    PhotoshopParseReportFldNum(nIndent, "Bound Rect (right)", nVal, "");
    strVal = PhotoshopParseGetBimLStrUni(nPos, nPosOffset);
    nPos += nPosOffset;
    PhotoshopParseReportFldStr(nIndent, "Name of group of slices", strVal);
    uint32_t nNumSlices = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);

    PhotoshopParseReportFldNum(nIndent, "Number of slices", nNumSlices, "");

    // Slice resource block
    if(nNumSlices > 0)
      PhotoshopParseReportNote(nIndent, "-----");
    for(uint32_t nSliceInd = 0; nSliceInd < nNumSlices; nSliceInd++)
    {
      QString strSliceInd;

      strSliceInd = QString("Slice #%1:").arg(nSliceInd);
      PhotoshopParseReportNote(nIndent, strSliceInd);
      PhotoshopParseSliceResource(nPos, nIndent + 1, nPosEnd);
    }
    if(nNumSlices > 0)
      PhotoshopParseReportNote(nIndent, "-----");
  }
  else if((nSliceVer == 7) || (nSliceVer == 8))
  {
    nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
    PhotoshopParseReportFldNum(nIndent, "Descriptor version", nVal, "");
    PhotoshopParseDescriptor(nPos, nIndent);
  }
}

// Parse the Photoshop IRB Slice Resource
// INPUT:
// - nPos               = File position at start of block
// - nIndent    = Indent level for formatted output
// - nPosEnd    = File position at end of block (last byte address)
// OUTPUT:
// - nPos               = File position after reading the block
// NOTE:
// - The nPosEnd is supplied as this resource block depends on some
//   conditional field parsing that is "as length allows"
//
void CDecodePs::PhotoshopParseSliceResource(uint32_t &nPos, uint32_t nIndent, uint32_t nPosEnd)
{
  uint32_t nVal;

  QString strVal;

  uint32_t nPosOffset;

  PhotoshopParseReportNote(nIndent, "Slice Resource:");
  nIndent++;

  nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "ID", nVal, "");
  nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Group ID", nVal, "");
  uint32_t nOrigin = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);

  PhotoshopParseReportFldNum(nIndent, "Origin", nOrigin, "");

  if(nOrigin == 1)
  {
    nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
    PhotoshopParseReportFldNum(nIndent, "Associated Layer ID", nVal, "");
  }

  strVal = PhotoshopParseGetBimLStrUni(nPos, nPosOffset);
  nPos += nPosOffset;
  PhotoshopParseReportFldStr(nIndent, "Name", strVal);
  nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Type", nVal, "");
  nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Position (top)", nVal, "");
  nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Position (left)", nVal, "");
  nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Position (bottom)", nVal, "");
  nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Position (right)", nVal, "");
  strVal = PhotoshopParseGetBimLStrUni(nPos, nPosOffset);
  nPos += nPosOffset;
  PhotoshopParseReportFldStr(nIndent, "URL", strVal);
  strVal = PhotoshopParseGetBimLStrUni(nPos, nPosOffset);
  nPos += nPosOffset;
  PhotoshopParseReportFldStr(nIndent, "Target", strVal);
  strVal = PhotoshopParseGetBimLStrUni(nPos, nPosOffset);
  nPos += nPosOffset;
  PhotoshopParseReportFldStr(nIndent, "Message", strVal);
  strVal = PhotoshopParseGetBimLStrUni(nPos, nPosOffset);
  nPos += nPosOffset;
  PhotoshopParseReportFldStr(nIndent, "Alt Tag", strVal);
  nVal = m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);
  PhotoshopParseReportFldBool(nIndent, "Cell text is HTML", nVal);
  strVal = PhotoshopParseGetBimLStrUni(nPos, nPosOffset);
  nPos += nPosOffset;
  PhotoshopParseReportFldStr(nIndent, "Cell text", strVal);
  nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Horizontal alignment", nVal, "");
  nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Vertical alignment", nVal, "");
  nVal = m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Alpha color", nVal, "");
  nVal = m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Red", nVal, "");
  nVal = m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Green", nVal, "");
  nVal = m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Blue", nVal, "");

  // Per standard, we are only supposed to parse additional fields
  // "as length allows". This is not a particularly good format
  // definition. At this point we'll perform a check to see if
  // we have exhausted the length of the block. If not, continue
  // to parse the next fields.
  // TODO: Do we need to be any more precise than this? (ie. check
  // after reading the descriptor version?)
  if(nPos <= nPosEnd)
  {
    nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
    PhotoshopParseReportFldNum(nIndent, "Descriptor version", nVal, "");

    PhotoshopParseDescriptor(nPos, nIndent);
  }
  else
  {
    // We ran out of space in the block so end now
  }
}

// Parse the Photoshop IRB JPEG Quality
// INPUT:
// - nPos               = File position at start of block
// - nIndent    = Indent level for formatted output
// - nPosEnd    = File position at end of block (last byte address)
// OUTPUT:
// - nPos               = File position after reading the block
// NOTE:
// - This IRB is private, so reverse-engineered and may not be per spec
//
void CDecodePs::PhotoshopParseJpegQuality(uint32_t &nPos, uint32_t nIndent, uint32_t)
{
  uint32_t nVal;

  QString strVal;

  uint32_t nSaveFormat;

  // Save As Quality
  // Index 0: Quality level
  nVal = m_pWBuf->BufRdAdv2(nPos, PS_BSWAP);

  switch (nVal)
  {
    case 0xFFFD:
      m_nQualitySaveAs = 1;
      break;
    case 0xFFFE:
      m_nQualitySaveAs = 2;
      break;
    case 0xFFFF:
      m_nQualitySaveAs = 3;
      break;
    case 0x0000:
      m_nQualitySaveAs = 4;
      break;
    case 0x0001:
      m_nQualitySaveAs = 5;
      break;
    case 0x0002:
      m_nQualitySaveAs = 6;
      break;
    case 0x0003:
      m_nQualitySaveAs = 7;
      break;
    case 0x0004:
      m_nQualitySaveAs = 8;
      break;
    case 0x0005:
      m_nQualitySaveAs = 9;
      break;
    case 0x0006:
      m_nQualitySaveAs = 10;
      break;
    case 0x0007:
      m_nQualitySaveAs = 11;
      break;
    case 0x0008:
      m_nQualitySaveAs = 12;
      break;
    default:
      m_nQualitySaveAs = 0;
      break;
  }

  if(m_nQualitySaveAs != 0)
  {
    PhotoshopParseReportFldNum(nIndent, "Photoshop Save As Quality", m_nQualitySaveAs, "");
  }

  // TODO: I have observed a file that had this block with length=2, which would end here
  // - Perhaps we should check against length in case some older files had a shorter format
  //   to this IRB?

  // Index 1: Format
  nSaveFormat = m_pWBuf->BufRdAdv2(nPos, PS_BSWAP);

  switch (nSaveFormat)
  {
    case 0x0000:
      strVal = "Standard";
      break;
    case 0x0001:
      strVal = "Optimized";
      break;
    case 0x0101:
      strVal = "Progressive";
      break;
    default:
      strVal = "???";
      break;
  }

  PhotoshopParseReportFldStr(nIndent, "Photoshop Save Format", strVal);

  // Index 2: Progressive Scans
  // - Only meaningful if Progressive mode
  nVal = m_pWBuf->BufRdAdv2(nPos, PS_BSWAP);

  switch (nVal)
  {
    case 0x0001:
      strVal = "3 Scans";
      break;
    case 0x0002:
      strVal = "4 Scans";
      break;
    case 0x0003:
      strVal = "5 Scans";
      break;
    default:
      strVal = "???";
      break;
  }

  PhotoshopParseReportFldStr(nIndent, "Photoshop Save Progressive Scans", strVal);

  // Not sure what this byte is for
  nVal = m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "???", nVal, "");
}

// Decode the OSType field and invoke the appropriate IRB parsing handler
//
// INPUT:
// - nPos               = File position at start of block
// - nIndent    = Indent level for formatted output
// OUTPUT:
// - nPos               = File position after reading the block
//
void CDecodePs::PhotoshopParseHandleOsType(QString strOsType, uint32_t &nPos, uint32_t nIndent)
{
  if(strOsType == "obj ")
  {
    //PhotoshopParseReference(nPos,nIndent);
  }
  else if(strOsType == "Objc")
  {
    PhotoshopParseDescriptor(nPos, nIndent);
  }
  else if(strOsType == "VlLs")
  {
    PhotoshopParseList(nPos, nIndent);
  }
  else if(strOsType == "doub")
  {
    //PhotoshopParseDouble(nPos,nIndent);
  }
  else if(strOsType == "UntF")
  {
    //PhotoshopParseUnitFloat(nPos,nIndent);
  }
  else if(strOsType == "TEXT")
  {
    PhotoshopParseStringUni(nPos, nIndent);
  }
  else if(strOsType == "enum")
  {
    PhotoshopParseEnum(nPos, nIndent);
  }
  else if(strOsType == "long")
  {
    PhotoshopParseInteger(nPos, nIndent);
  }
  else if(strOsType == "bool")
  {
    PhotoshopParseBool(nPos, nIndent);
  }
  else if(strOsType == "GlbO")
  {
    PhotoshopParseDescriptor(nPos, nIndent);
  }
  else if(strOsType == "type")
  {
    //PhotoshopParseClass(nPos,nIndent);
  }
  else if(strOsType == "GlbC")
  {
    //PhotoshopParseClass(nPos,nIndent);
  }
  else if(strOsType == "alis")
  {
    //PhotoshopParseAlias(nPos,nIndent);
  }
  else if(strOsType == "tdta")
  {
    //PhotoshopParseRawData(nPos,nIndent);
  }
  else
  {
#ifdef DEBUG_LOG
    QString strError;
    QString strDebug;

    strError = QString("ERROR: Unsupported OSType [%1]").arg(strOsType);
    strDebug = QString("## File=[%1] Block=[%2] Error=[%3]\n")
        .arg(m_pAppConfig->strCurFname, -100)
        .arg("PsDecode", -10)
        .arg(strError);
    qDebug() << (strDebug);
#else
    Q_ASSERT(false);
#endif
  }
}

// Parse the Photoshop IRB OSType Descriptor
// INPUT:
// - nPos               = File position at start of entry
// - nIndent    = Indent level for formatted output
// OUTPUT:
// - nPos               = File position after reading the entry
//
void CDecodePs::PhotoshopParseDescriptor(uint32_t &nPos, uint32_t nIndent)
{
  QString strVal;

  uint32_t nPosOffset;

  PhotoshopParseReportNote(nIndent, "Descriptor:");
  nIndent++;

  strVal = PhotoshopParseGetBimLStrUni(nPos, nPosOffset);
  nPos += nPosOffset;
  PhotoshopParseReportFldStr(nIndent, "Name from classID", strVal);

  strVal = PhotoshopParseGetLStrAsc(nPos);
  PhotoshopParseReportFldStr(nIndent, "classID", strVal);

  uint32_t nDescNumItems = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);

  PhotoshopParseReportFldNum(nIndent, "Num items in descriptor", nDescNumItems, "");

  if(nDescNumItems > 0)
    PhotoshopParseReportNote(nIndent, "-----");

  for(uint32_t nDescInd = 0; nDescInd < nDescNumItems; nDescInd++)
  {
    QString strDescInd;

    strDescInd = QString("Descriptor item #%1:").arg(nDescInd);
    PhotoshopParseReportNote(nIndent, strDescInd);

    strVal = PhotoshopParseGetLStrAsc(nPos);
    PhotoshopParseReportFldStr(nIndent + 1, "Key", strVal);

    QString strOsType;

    strOsType = QString("%1%2%3%4")
        .arg(Buf(nPos + 0))
        .arg(Buf(nPos + 1))
        .arg(Buf(nPos + 2))
        .arg(Buf(nPos + 3));
    nPos += 4;
    PhotoshopParseReportFldStr(nIndent + 1, "OSType key", strOsType);

    PhotoshopParseHandleOsType(strOsType, nPos, nIndent + 1);
  }

  if(nDescNumItems > 0)
    PhotoshopParseReportNote(nIndent, "-----");
}

// Parse the Photoshop IRB OSType List
// INPUT:
// - nPos               = File position at start of entry
// - nIndent    = Indent level for formatted output
// OUTPUT:
// - nPos               = File position after reading the entry
//
void CDecodePs::PhotoshopParseList(uint32_t &nPos, uint32_t nIndent)
{
  QString strVal;
  QString strLine;

  uint32_t nNumItems = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);

  PhotoshopParseReportFldNum(nIndent, "Num items in list", nNumItems, "");

  if(nNumItems > 0)
    PhotoshopParseReportNote(nIndent, "-----");

  for(uint32_t nItemInd = 0; nItemInd < nNumItems; nItemInd++)
  {
    QString strItemInd;

    strItemInd = QString("Item #%1:").arg(nItemInd);
    PhotoshopParseReportNote(nIndent, strItemInd);

    QString strOsType;

    strOsType = QString("%1%2%3%4")
        .arg(Buf(nPos + 0))
        .arg(Buf(nPos + 1))
        .arg(Buf(nPos + 2))
        .arg(Buf(nPos + 3));
    nPos += 4;
    PhotoshopParseReportFldStr(nIndent + 1, "OSType key", strVal);

    PhotoshopParseHandleOsType(strOsType, nPos, nIndent + 1);
  }

  if(nNumItems > 0)
    PhotoshopParseReportNote(nIndent, "-----");
}

// Parse the Photoshop IRB OSType Integer
// INPUT:
// - nPos               = File position at start of entry
// - nIndent    = Indent level for formatted output
// OUTPUT:
// - nPos               = File position after reading the entry
//
void CDecodePs::PhotoshopParseInteger(uint32_t &nPos, uint32_t nIndent)
{
  uint32_t nVal;

  QString strVal;
  QString strLine;

  nVal = m_pWBuf->BufRdAdv4(nPos, PS_BSWAP);
  PhotoshopParseReportFldNum(nIndent, "Value", nVal, "");
}

// Parse the Photoshop IRB OSType Boolean
// INPUT:
// - nPos               = File position at start of entry
// - nIndent    = Indent level for formatted output
// OUTPUT:
// - nPos               = File position after reading the entry
//
void CDecodePs::PhotoshopParseBool(uint32_t &nPos, uint32_t nIndent)
{
  uint32_t nVal;

  QString strVal;
  QString strLine;

  nVal = m_pWBuf->BufRdAdv1(nPos, PS_BSWAP);
  PhotoshopParseReportFldBool(nIndent, "Value", nVal);
}

// Parse the Photoshop IRB OSType Enumerated type
// INPUT:
// - nPos               = File position at start of entry
// - nIndent    = Indent level for formatted output
// OUTPUT:
// - nPos               = File position after reading the entry
//
void CDecodePs::PhotoshopParseEnum(uint32_t &nPos, uint32_t nIndent)
{
  QString strVal;
  QString strLine;

  strVal = PhotoshopParseGetLStrAsc(nPos);
  PhotoshopParseReportFldStr(nIndent, "Type", strVal);

  strVal = PhotoshopParseGetLStrAsc(nPos);
  PhotoshopParseReportFldStr(nIndent, "Enum", strVal);
}

// Parse the Photoshop IRB Unicode String
// INPUT:
// - nPos               = File position at start of entry
// - nIndent    = Indent level for formatted output
// OUTPUT:
// - nPos               = File position after reading the entry
// NOTE:
// - The string is in Photoshop Unicode format (length first)
//
void CDecodePs::PhotoshopParseStringUni(uint32_t &nPos, uint32_t nIndent)
{
  QString strVal;

  uint32_t nPosOffset;

  strVal = PhotoshopParseGetBimLStrUni(nPos, nPosOffset);
  nPos += nPosOffset;
  PhotoshopParseReportFldStr(nIndent, "String", strVal);
}
