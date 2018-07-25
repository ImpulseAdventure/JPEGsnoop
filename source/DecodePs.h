// JPEGsnoop - JPEG Image Decoder & Analysis Utility
// Copyright (C) 2017 - Calvin Hass
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
// - This module parses the Photoshop PSD and 8BIM segments (APP13)
// - No rendering is done
//
// ==========================================================================

#ifndef DECODEPS_H
#define DECODEPS_H

#include "WindowBuf.h"
//#include "snoop.h"

class CDocLog;
class CSnoopConfig;

//
// Structure used for each IPTC field
// - NUM     : binary number
// - STR     : alphabetic characters, graphic characters, numeric character
// - HEX     :
enum teIptcType
{
  IPTC_T_NUM,
  IPTC_T_NUM1,
  IPTC_T_NUM2,
  IPTC_T_HEX,
  IPTC_T_STR,
  IPTC_T_UNK,
  IPTC_T_END
};

struct tsIptcField
{
  uint32_t nRecord;
  uint32_t nDataSet;
  teIptcType eFldType;
  QString strFldName;
};

// Structure used for each Image Resource Block (8BIM) record
enum teBimType
{
  BIM_T_UNK, BIM_T_STR, BIM_T_HEX,
  BIM_T_IPTC_NAA,
  BIM_T_JPEG_QUAL,
  BIM_T_PS_SLICES,
  BIM_T_PS_THUMB_RES,
  BIM_T_PS_DESCRIPTOR,
  BIM_T_PS_VER_INFO,
  BIM_T_PS_RESOLUTION_INFO,
  BIM_T_PS_PRINT_SCALE,
  BIM_T_PS_PIXEL_ASPECT_RATIO,
  BIM_T_PS_DOC_SPECIFIC_SEED,
  BIM_T_PS_GRID_GUIDES,
  BIM_T_PS_GLOBAL_ANGLE,
  BIM_T_PS_GLOBAL_ALTITUDE,
  BIM_T_PS_PRINT_FLAGS,
  BIM_T_PS_PRINT_FLAGS_INFO,
  BIM_T_PS_COPYRIGHT_FLAG,
  BIM_T_PS_LAYER_STATE_INFO,
  BIM_T_PS_LAYER_GROUP_INFO,
  BIM_T_PS_LAYER_GROUP_ENABLED,
  BIM_T_PS_LAYER_SELECT_ID,
  BIM_T_PS_STR_UNI,
  BIM_T_PS_STR_ASC,
  BIM_T_PS_STR_ASC_LONG,
  BIM_T_END
};

struct tsBimRecord
{
  uint32_t nCode;               // Code value or start code for range
  uint32_t nCodeEnd;            // 0x0000 if not a range, else specifies last code value
  teBimType eBimType;
  QString strRecordName;
};

// Structure used for each enumerated field in IRB decoding
enum teBimEnumField
{
  BIM_T_ENUM_UNK,
  BIM_T_ENUM_FILE_HDR_COL_MODE,
  BIM_T_ENUM_RESOLUTION_INFO_RES_UNIT,
  BIM_T_ENUM_RESOLUTION_INFO_WIDTH_UNIT,
  BIM_T_ENUM_PRINT_SCALE_STYLE,
  BIM_T_ENUM_GRID_GUIDE_DIR,
  BIM_T_ENUM_BLEND_MODE_KEY,
  BIM_T_ENUM_END
};

struct tsBimEnum
{
  teBimEnumField eBimEnum;
  uint32_t nVal;
  QString strVal;
};

// Byte Swap enable for Photoshop decoding
#define PS_BSWAP	false

// Define the 8BIM/IRB hex output display characteristics
static const uint32_t PS_HEX_MAX_INLINE = 16;   // Threshold for displaying hex in-line with field name
static const uint32_t PS_HEX_MAX_ROW = 16;      // Maximum number of bytes to report per line
static const uint32_t PS_HEX_TOTAL = 128;       // Total number of bytes to report before clipping

// Define the maximum length Unicode string to display
static const uint32_t PS_MAX_UNICODE_STRLEN = 256;

// Information about layer and channels within it
struct tsLayerInfo
{
  uint32_t nNumChans;
  uint32_t *pnChanLen;
  uint32_t *pnChanID;
  uint32_t nWidth;
  uint32_t nHeight;
};

// From Layer Info
struct tsLayerAllInfo
{
  uint32_t nNumLayers;
  tsLayerInfo *psLayers;
};

// From File Header
struct tsImageInfo
{
  uint32_t nImageWidth;
  uint32_t nImageHeight;
  uint32_t nNumChans;
  uint32_t nDepthBpp;
};

class CDecodePs
{
public:
  CDecodePs(CwindowBuf *pWBuf, CDocLog * pLog, CSnoopConfig *pAppConfig);
  ~CDecodePs(void);

  void Reset();

  bool DecodePsd(uint32_t nPos, QImage * pDibTemp, int32_t &nWidth, int32_t &nHeight);
  bool PhotoshopParseImageResourceBlock(uint32_t &nPos, uint32_t nIndent);

  bool m_bPsd;
  uint32_t m_nQualitySaveAs;
  uint32_t m_nQualitySaveForWeb;

  bool m_bDisplayLayer;
  uint32_t m_nDisplayLayerInd;
  bool m_bDisplayImage;

private:
  CDecodePs &operator = (const CDecodePs&);
  CDecodePs(CDecodePs&);

  QString PhotoshopParseGetLStrAsc(uint32_t &nPos);
  QString PhotoshopParseIndent(uint32_t nIndent);
  void PhotoshopParseReportNote(uint32_t nIndent, QString strNote);
  QString PhotoshopParseLookupEnum(teBimEnumField eEnumField, uint32_t nVal);
  void PhotoshopParseReportFldNum(uint32_t nIndent, QString strField, uint32_t nVal, QString strUnits);
  void PhotoshopParseReportFldBool(uint32_t nIndent, QString strField, uint32_t nVal);
  void PhotoshopParseReportFldEnum(uint32_t nIndent, QString strField, teBimEnumField eEnumField, uint32_t nVal);
  void PhotoshopParseReportFldFixPt(uint32_t nIndent, QString strField, uint32_t nVal, QString strUnits);
  void PhotoshopParseReportFldFloatPt(uint32_t nIndent, QString strField, uint32_t nVal, QString strUnits);
  void PhotoshopParseReportFldDoublePt(uint32_t nIndent, QString strField, uint32_t nVal1, uint32_t nVal2, QString strUnits);
  void PhotoshopParseReportFldStr(uint32_t nIndent, QString strField, QString strVal);
  void PhotoshopParseReportFldOffset(uint32_t nIndent, QString strField, uint32_t nOffset);
  void PhotoshopParseReportFldHex(uint32_t nIndent, QString strField, uint32_t nPosStart, uint32_t nLen);
  void PhotoshopParseThumbnailResource(uint32_t &nPos, uint32_t nIndent);
  void PhotoshopParseSliceHeader(uint32_t &nPos, uint32_t nIndent, uint32_t nPosEnd);
  void PhotoshopParseSliceResource(uint32_t &nPos, uint32_t nIndent, uint32_t nPosEnd);
  void PhotoshopParseDescriptor(uint32_t &nPos, uint32_t nIndent);
  void PhotoshopParseList(uint32_t &nPos, uint32_t nIndent);
  void PhotoshopParseInteger(uint32_t &nPos, uint32_t nIndent);
  void PhotoshopParseBool(uint32_t &nPos, uint32_t nIndent);
  void PhotoshopParseEnum(uint32_t &nPos, uint32_t nIndent);
  void PhotoshopParseStringUni(uint32_t &nPos, uint32_t nIndent);
  void PhotoshopParseHandleOsType(QString strOsType, uint32_t &nPos, uint32_t nIndent);
  void PhotoshopParseFileHeader(uint32_t &nPos, uint32_t nIndent, tsImageInfo * psImageInfo);
  void PhotoshopParseColorModeSection(uint32_t &nPos, uint32_t nIndent);
  bool PhotoshopParseImageResourcesSection(uint32_t &nPos, uint32_t nIndent);
  void PhotoshopParseVersionInfo(uint32_t &nPos, uint32_t nIndent);
  void PhotoshopParseResolutionInfo(uint32_t &nPos, uint32_t nIndent);
  void PhotoshopParsePrintScale(uint32_t &nPos, uint32_t nIndent);
  void PhotoshopParsePixelAspectRatio(uint32_t &nPos, uint32_t nIndent);
  void PhotoshopParseDocSpecificSeed(uint32_t &nPos, uint32_t nIndent);
  void PhotoshopParseGridGuides(uint32_t &nPos, uint32_t nIndent);
  void PhotoshopParseGlobalAngle(uint32_t &nPos, uint32_t nIndent);
  void PhotoshopParseGlobalAltitude(uint32_t &nPos, uint32_t nIndent);
  void PhotoshopParsePrintFlags(uint32_t &nPos, uint32_t nIndent);
  void PhotoshopParsePrintFlagsInfo(uint32_t &nPos, uint32_t nIndent);
  void PhotoshopParseCopyrightFlag(uint32_t &nPos, uint32_t nIndent);
  void PhotoshopParseLayerStateInfo(uint32_t &nPos, uint32_t nIndent);
  void PhotoshopParseLayerGroupInfo(uint32_t &nPos, uint32_t nIndent, uint32_t nLen);
  void PhotoshopParseLayerGroupEnabled(uint32_t &nPos, uint32_t nIndent, uint32_t nLen);
  void PhotoshopParseLayerSelectId(uint32_t &nPos, uint32_t nIndent);
  void PhotoshopParseJpegQuality(uint32_t &nPos, uint32_t nIndent, uint32_t nPosEnd);

  bool PhotoshopParseLayerMaskInfo(uint32_t &nPos, uint32_t nIndent, QImage * pDibTemp);
  bool PhotoshopParseLayerInfo(uint32_t &nPos, uint32_t nIndent, QImage * pDibTemp);
  bool PhotoshopParseLayerRecord(uint32_t &nPos, uint32_t nIndent, tsLayerInfo * psLayerInfo);
  bool PhotoshopParseLayerMask(uint32_t &nPos, uint32_t nIndent);
  bool PhotoshopParseLayerBlendingRanges(uint32_t &nPos, uint32_t nIndent);
  bool PhotoshopParseChannelImageData(uint32_t &nPos, uint32_t nIndent, uint32_t nWidth, uint32_t nHeight, uint32_t nChan,
                                      unsigned char *pDibBits);
  bool PhotoshopParseGlobalLayerMaskInfo(uint32_t &nPos, uint32_t nIndent);
  bool PhotoshopParseAddtlLayerInfo(uint32_t &nPos, uint32_t nIndent);
  bool PhotoshopParseImageData(uint32_t &nPos, uint32_t nIndent, tsImageInfo * psImageInfo, unsigned char *pDibBits);

  bool PhotoshopDecodeRowUncomp(uint32_t &nPos, uint32_t nWidth, uint32_t nHeight, uint32_t nRow, uint32_t nChanID,
                                unsigned char *pDibBits);
  bool PhotoshopDecodeRowRle(uint32_t &nPos, uint32_t nWidth, uint32_t nHeight, uint32_t nRow, uint32_t nRowLen,
                             uint32_t nChanID, unsigned char *pDibBits);

  QString PhotoshopDispHexWord(uint32_t nVal);

  // 8BIM
  QString PhotoshopParseGetBimLStrUni(uint32_t nPos, uint32_t &nPosOffset);
  bool FindBimRecord(uint32_t nBimId, uint32_t &nFldInd);

  // IPTC
  void DecodeIptc(uint32_t &nPos, uint32_t nLen, uint32_t nIndent);
  bool LookupIptcField(uint32_t nRecord, uint32_t nDataSet, uint32_t &nFldInd);
  QString DecodeIptcValue(teIptcType eIptcType, uint32_t nFldCnt, uint32_t nPos);

  quint8 Buf(uint32_t offset, bool bClean);

  // Configuration
  CSnoopConfig * m_pAppConfig;

  // General classes required for decoding
  CwindowBuf *m_pWBuf;
  CDocLog *m_pLog;

  bool m_bAbort;                // Abort continued decode?
};

#endif
