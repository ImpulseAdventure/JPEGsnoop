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

#include <string.h>

#include <QCoreApplication>
#include <QtDebug>
#include <QFileInfo>
#include <QtGlobal>
#include <QCryptographicHash>
#include <QByteArray>

#include "JfifDecode.h"
#include "snoop.h"
//#include "JPEGsnoop.h"          // for m_pAppConfig get
#include "SnoopConfig.h"
#include "DbSigs.h"
#include "DocLog.h"
#include "DbSigs.h"
#include "WindowBuf.h"
#include "Md5.h"
#include "UrlString.h"
#include "DbSigs.h"
#include "General.h"

// Maximum number of component values to extract into array for display
static const uint32_t MAX_anValues = 64;

// Macro to avoid multi-character constant definitions
#define FOURC_INT(a,b,c,d)    (((a)<<24) | ((b)<<16) | ((c)<<8) | (d))

//-----------------------------------------------------------------------------
// Initialize the JFIF decoder. Several class pointers are provided
// as parameters, so that we can directly access the output log, the
// file buffer and the image scan decoder.
// Loads up the signature database.
//
// INPUT:
// - pLog                       Ptr to log file class
// - pWBuf                      Ptr to Window Buf class
// - pImgDec            Ptr to Image Decoder class
//
// PRE:
// - Requires that CDocLog, CwindowBuf and CimgDecode classes
//   are already initialized
//
CjfifDecode::CjfifDecode(CDocLog *pLog, CDbSigs *pDbSigs, CwindowBuf *pWBuf, CimgDecode *pImgDec, CSnoopConfig *pAppConfig, QObject *_parent) :
  QObject(_parent), m_pLog(pLog), m_pDbSigs(pDbSigs), m_pWBuf(pWBuf), m_pImgDec(pImgDec), m_pAppConfig(pAppConfig)
{
  if(DEBUG_EN)
    m_pAppConfig->DebugLogAdd("CjfifDecode::CjfifDecode() Begin");

  // Need to zero out the private members
  m_bOutputDB = false;          // mySQL output for web

  // Enable verbose reporting
  m_bVerbose = false;

  m_pImgSrcDirty = true;

  // Generate lookup tables for Huffman codes
  GenLookupHuffMask();

  // Window status bar is not ready yet, wait for call to SetStatusBar()
  m_pStatBar = 0;

  // Reset decoding state
  Reset();

  // Load the local database (if it exists)
//@@  m_pDbSigs->DatabaseExtraLoad();

  // Allocate the Photoshop decoder
  m_pPsDec = new CDecodePs(pWBuf, pLog, m_pAppConfig);

  connect(m_pAppConfig, SIGNAL(ImgSrcChanged()), this, SLOT(ImgSrcChanged()));
  connect(_parent, SIGNAL(ImgSrcChanged()), this, SLOT(ImgSrcChanged()));

  if(!m_pPsDec)
  {
    Q_ASSERT(false);
    return;
  }

#ifdef SUPPORT_DICOM
  // Allocate the DICOM decoder
  m_pDecDicom = new CDecodeDicom(pWBuf, pLog);

  if(!m_pDecDicom)
  {
    Q_ASSERT(false);
    return;
  }

  if(DEBUG_EN)
    m_pAppConfig->DebugLogAdd("CjfifDecode::CjfifDecode() Checkpoint 5");
#endif
}

//-----------------------------------------------------------------------------
// Destructor
CjfifDecode::~CjfifDecode()
{
  // Free the Photoshop decoder
  if(m_pPsDec)
  {
    delete m_pPsDec;

    m_pPsDec = nullptr;
  }

#ifdef SUPPORT_DICOM
  // Free the DICOM decoder
  if(m_pDecDicom)
  {
    delete m_pDecDicom;

    m_pDecDicom = nullptr;
  }
#endif

}

//-----------------------------------------------------------------------------
// Clear out internal members
void CjfifDecode::Reset()
{
  // File handling
  m_nPos = 0;
  m_nPosSos = 0;
  m_nPosEoi = 0;
  m_nPosEmbedStart = 0;
  m_nPosEmbedEnd = 0;
  m_nPosFileEnd = 0;

  // SOS / SOF handling
  m_nSofNumLines_Y = 0;
  m_nSofSampsPerLine_X = 0;
  m_nSofNumComps_Nf = 0;

  // Quantization tables
  ClearDQT();

  // Photoshop
  m_nImgQualPhotoshopSfw = 0;
  m_nImgQualPhotoshopSa = 0;

  m_nApp14ColTransform = -1;

  // Restart marker
  m_nImgRstEn = false;
  m_nImgRstInterval = 0;

  // Basic metadata
  m_strImgExifMake = "???";
  m_nImgExifMakeSubtype = 0;
  m_strImgExifModel = "???";
  m_bImgExifMakernotes = false;
  m_strImgExtras = "";
  m_strComment = "";
  m_strSoftware = "";
  m_bImgProgressive = false;
  m_bImgSofUnsupported = false;
  strcpy(m_acApp0Identifier, "");

  // Derived metadata
  m_strHash = "NONE";
  m_strHashRot = "NONE";
  m_eImgLandscape = ENUM_LANDSCAPE_UNSET;
  m_strImgQualExif = "";
  m_bAvi = false;
  m_bAviMjpeg = false;
  m_bPsd = false;

  // Misc
  m_bImgOK = false;             // Set during SOF to indicate further proc OK
  m_bBufFakeDHT = false;        // Start in normal Buf mode
  m_eImgEdited = EDITED_UNSET;
  m_eDbReqSuggest = DB_ADD_SUGGEST_UNSET;
  m_bSigExactInDB = false;

  // Embedded thumbnail
  m_nImgExifThumbComp = 0;
  m_nImgExifThumbOffset = 0;
  m_nImgExifThumbLen = 0;
  m_strHashThumb = "NONE";      // Will go into DB to say NONE!
  m_strHashThumbRot = "NONE";   // Will go into DB to say NONE!
  m_nImgThumbNumLines = 0;
  m_nImgThumbSampsPerLine = 0;

  // Now clear out any previously generated bitmaps
  // or image decoding parameters
  if(m_pImgDec)
  {
    if(m_pImgSrcDirty)
    {
      m_pImgDec->Reset();
    }
  }

  // Reset the decoding state checks
  // These are to help ensure we don't start decoding SOS
  // if we haven't seen other valid markers yet! Otherwise
  // we could run into very bad loops (e.g. .PSD files)
  // just because we saw FFD8FF first then JFIF_SOS
  m_bStateAbort = false;
  m_bStateSoi = false;
  m_bStateDht = false;
  m_bStateDhtOk = false;
  m_bStateDhtFake = false;
  m_bStateDqt = false;
  m_bStateDqtOk = false;
  m_bStateSof = false;
  m_bStateSofOk = false;
  m_bStateSos = false;
  m_bStateSosOk = false;
  m_bStateEoi = false;
}

//-----------------------------------------------------------------------------
// Asynchronously update a local pointer to the status bar once
// it becomes available. Note that the status bar is not ready by
// the time of the CjfifDecode class constructor call.
//
// INPUT:
// - pStatBar                   Ptr to status bar
//
// POST:
// - m_pStatBar
//
void CjfifDecode::SetStatusBar(QStatusBar * pStatBar)
{
  m_pStatBar = pStatBar;
}

//-----------------------------------------------------------------------------
// Indicate that the source of the image scan data
// has been dirtied. Either the source has changed
// or some of the View2 options have changed.
//
// POST:
// - m_pImgSrcDirty
//
void CjfifDecode::ImgSrcChanged()
{
  m_pImgSrcDirty = true;
}

//-----------------------------------------------------------------------------
// Set the AVI mode flag for this file
//
// POST:
// - m_bAvi
// - m_bAviMjpeg
//
void CjfifDecode::SetAviMode(bool bIsAvi, bool bIsMjpeg)
{
  m_bAvi = bIsAvi;
  m_bAviMjpeg = bIsMjpeg;
}

//-----------------------------------------------------------------------------
// Fetch the AVI mode flag for this file
//
// PRE:
// - m_bAvi
// - m_bAviMjpeg
//
// OUTPUT:
// - bIsAvi
// - bIsMjpeg
//
void CjfifDecode::GetAviMode(bool & bIsAvi, bool & bIsMjpeg)
{
  bIsAvi = m_bAvi;
  bIsMjpeg = m_bAviMjpeg;
}

//-----------------------------------------------------------------------------
// Fetch the starting file position of the embedded thumbnail
//
// PRE:
// - m_nPosEmbedStart
//
// RETURN:
// - File position
//
uint32_t CjfifDecode::GetPosEmbedStart()
{
  return m_nPosEmbedStart;
}

//-----------------------------------------------------------------------------
// Fetch the ending file position of the embedded thumbnail
//
// PRE:
// - m_nPosEmbedEnd
//
// RETURN:
// - File position
//
uint32_t CjfifDecode::GetPosEmbedEnd()
{
  return m_nPosEmbedEnd;
}

//-----------------------------------------------------------------------------
// Determine if the last analysis revealed a JFIF with known markers
//
// RETURN:
// - true if file (at position during analysis) appeared to decode OK
//
bool CjfifDecode::GetDecodeStatus()
{
  return m_bImgOK;
}

//-----------------------------------------------------------------------------
// Fetch a summary of the JFIF decoder results
// These details are used in preparation of signature submission to the DB
//
// PRE:
// - m_strHash
// - m_strHashRot
// - m_strImgExifMake
// - m_strImgExifModel
// - m_strImgQualExif
// - m_strSoftware
// - m_eDbReqSuggest
//
// OUTPUT:
// - strHash
// - strHashRot
// - strImgExifMake
// - strImgExifModel
// - strImgQualExif
// - strSoftware
// - nDbReqSuggest
//
void CjfifDecode::GetDecodeSummary(QString & strHash, QString & strHashRot, QString & strImgExifMake, QString & strImgExifModel,
                                   QString & strImgQualExif, QString & strSoftware, teDbAdd & eDbReqSuggest)
{
  strHash = m_strHash;
  strHashRot = m_strHashRot;
  strImgExifMake = m_strImgExifMake;
  strImgExifModel = m_strImgExifModel;
  strImgQualExif = m_strImgQualExif;
  strSoftware = m_strSoftware;
  eDbReqSuggest = m_eDbReqSuggest;
}

//-----------------------------------------------------------------------------
// Fetch an element from the "standard" luminance quantization table
//
// PRE:
// - glb_anStdQuantLum[]
//
// RETURN:
// - DQT matrix element
//
uint32_t CjfifDecode::GetDqtQuantStd(uint32_t nInd)
{
  if(nInd < MAX_DQT_COEFF)
  {
    return glb_anStdQuantLum[nInd];
  }
  else
  {
#ifdef DEBUG_LOG
    QString strTmp;

    QString strDebug;

    strTmp = QString("GetDqtQuantStd() with nInd out of range. nInd=[%1]").arg(nInd);
    strDebug = QString("## File=[%1] Block=[%2] Error=[%3]").arg(m_pAppConfig->strCurFname, -100).arg("JfifDecode", -10).arg(strTmp);
    qDebug() << strDebug;
#else
    Q_ASSERT(false);
#endif
    return 0;
  }
}

//-----------------------------------------------------------------------------
// Fetch the DQT ordering index (with optional zigzag sequence)
//
// INPUT:
// - nInd                       Coefficient index
// - bZigZag            Use zig-zag ordering
//
// RETURN:
// - Sequence index
//
uint32_t CjfifDecode::GetDqtZigZagIndex(uint32_t nInd, bool bZigZag)
{
  if(nInd < MAX_DQT_COEFF)
  {
    if(bZigZag)
    {
      return nInd;
    }
    else
    {
      return glb_anZigZag[nInd];
    }
  }
  else
  {
#ifdef DEBUG_LOG
    QString strTmp;
    QString strDebug;

    strTmp = QString("GetDqtZigZagIndex() with nInd out of range. nInd=[%1]").arg(nInd);
    strDebug = QString("## File=[%1] Block=[%2] Error=[%3]").arg(m_pAppConfig->strCurFname, -100).arg("JfifDecode", -10).arg(strTmp);
    qDebug() << strDebug;
#else
    Q_ASSERT(false);
#endif
    return 0;
  }
}

//-----------------------------------------------------------------------------
// Reset the DQT tables
//
// POST:
// - m_anImgDqtTbl[][]
// - m_anImgThumbDqt[][]
// - m_adImgDqtQual[]
// - m_abImgDqtSet[]
// - m_abImgDqtThumbSet[]
//
void CjfifDecode::ClearDQT()
{
  for(uint32_t nTblInd = 0; nTblInd < MAX_DQT_DEST_ID; nTblInd++)
  {
    for(uint32_t nCoeffInd = 0; nCoeffInd < MAX_DQT_COEFF; nCoeffInd++)
    {
      m_anImgDqtTbl[nTblInd][nCoeffInd] = 0;
      m_anImgThumbDqt[nTblInd][nCoeffInd] = 0;
    }

    m_adImgDqtQual[nTblInd] = 0;
    m_abImgDqtSet[nTblInd] = false;
    m_abImgDqtThumbSet[nTblInd] = false;
  }
}

//-----------------------------------------------------------------------------
// Set the DQT matrix element
//
// INPUT:
// - dqt0[]                             Matrix array for table 0
// - dqt1[]                             Matrix array for table 1
//
// POST:
// - m_anImgDqtTbl[][]
// - m_eImgLandscape
// - m_abImgDqtSet[]
// - m_strImgQuantCss
//
void CjfifDecode::SetDQTQuick(uint16_t anDqt0[64], uint16_t anDqt1[64])
{
  m_eImgLandscape = ENUM_LANDSCAPE_YES;

  for(uint32_t ind = 0; ind < MAX_DQT_COEFF; ind++)
  {
    m_anImgDqtTbl[0][ind] = anDqt0[ind];
    m_anImgDqtTbl[1][ind] = anDqt1[ind];
  }

  m_abImgDqtSet[0] = true;
  m_abImgDqtSet[1] = true;
  m_strImgQuantCss = "NA";
}

//-----------------------------------------------------------------------------
// Construct a lookup table for the Huffman code masks
// The result is a simple bit sequence of zeros followed by
// an increasing number of 1 bits.
//   00000000...00000001
//   00000000...00000011
//   00000000...00000111
//   ...
//   01111111...11111111
//   11111111...11111111
//
// POST:
// - m_anMaskLookup[]
//
void CjfifDecode::GenLookupHuffMask()
{
  uint32_t mask;

  for(uint32_t len = 0; len < 32; len++)
  {
    mask = (1 << len) - 1;
    mask <<= 32 - len;
    m_anMaskLookup[len] = mask;
  }
}

//-----------------------------------------------------------------------------
// Provide a short-hand alias for the m_pWBuf buffer
// Also support redirection to a local table in case we are
// faking out the DHT (eg. for MotionJPEG files).
//
// PRE:
// - m_bBufFakeDHT                      Flag to include Fake DHT table
// - m_abMJPGDHTSeg[]           DHT table used if m_bBufFakeDHT=true
//
// INPUT:
// - nOffset                            File offset to read from
// - bClean                                     Forcibly disables any redirection to Fake DHT table
//
// POST:
// - m_pLog
//
// RETURN:
// - Byte from file (or local table)
//
uint8_t CjfifDecode::Buf(uint32_t nOffset, bool bClean = false)
{
  // Buffer can be redirected to internal array for AVI DHT
  // tables, so check for it here.
  if(m_bBufFakeDHT)
  {
    return m_abMJPGDHTSeg[nOffset];
  }
  else
  {
    return m_pWBuf->Buf(nOffset, bClean);
  }
}

//-----------------------------------------------------------------------------
// Write out a line to the log buffer if we are in verbose mode
//
// PRE:
// - m_bVerbose                         Verbose mode
//
// INPUT:
// - strLine                            String to output
//
// OUTPUT:
// - none
//
// POST:
// - m_pLog
//
// RETURN:
// - none
//
void CjfifDecode::DbgAddLine(QString strLine)
{
  if(m_bVerbose)
  {
    m_pLog->AddLine(strLine);
  }
}

//-----------------------------------------------------------------------------
// Convert a UINT32 and decompose into 4 bytes, but support
// either endian byte-swap mode
//
// PRE:
// - m_nImgExifEndian           Byte swap mode (0=little, 1=big)
//
// INPUT:
// - nVal                                       Input UINT32
//
// OUTPUT:
// - nByte0                                     Byte #1
// - nByte1                                     Byte #2
// - nByte2                                     Byte #3
// - nByte3                                     Byte #4
//
// RETURN:
// - none
//
void CjfifDecode::UnByteSwap4(uint32_t nVal, uint32_t &nByte0, uint32_t &nByte1, uint32_t &nByte2, uint32_t &nByte3)
{
  if(m_nImgExifEndian == 0)
  {
    // Little Endian
    nByte3 = (nVal & 0xFF000000) >> 24;
    nByte2 = (nVal & 0x00FF0000) >> 16;
    nByte1 = (nVal & 0x0000FF00) >> 8;
    nByte0 = (nVal & 0x000000FF);
  }
  else
  {
    // Big Endian
    nByte0 = (nVal & 0xFF000000) >> 24;
    nByte1 = (nVal & 0x00FF0000) >> 16;
    nByte2 = (nVal & 0x0000FF00) >> 8;
    nByte3 = (nVal & 0x000000FF);
  }
}

//-----------------------------------------------------------------------------
// Perform conversion from 4 bytes into UINT32 with
// endian byte-swapping support
//
// PRE:
// - m_nImgExifEndian           Byte swap mode (0=little, 1=big)
//
// INPUT:
// - nByte0                                             Byte #1
// - nByte1                                             Byte #2
// - nByte2                                             Byte #3
// - nByte3                                             Byte #4
//
// RETURN:
// - UINT32
//
uint32_t CjfifDecode::ByteSwap4(uint32_t nByte0, uint32_t nByte1, uint32_t nByte2, uint32_t nByte3)
{
  uint32_t nVal;

  if(m_nImgExifEndian == 0)
  {
    // Little endian, byte swap required
    nVal = (nByte3 << 24) + (nByte2 << 16) + (nByte1 << 8) + nByte0;
  }
  else
  {
    // Big endian, no swap required
    nVal = (nByte0 << 24) + (nByte1 << 16) + (nByte2 << 8) + nByte3;
  }

  return nVal;
}

//-----------------------------------------------------------------------------
// Perform conversion from 2 bytes into half-word with
// endian byte-swapping support
//
// PRE:
// - m_nImgExifEndian           Byte swap mode (0=little, 1=big)
//
// INPUT:
// - nByte0                                             Byte #1
// - nByte1                                             Byte #2
//
// RETURN:
// - UINT16
//
uint32_t CjfifDecode::ByteSwap2(uint32_t nByte0, uint32_t nByte1)
{
  uint32_t nVal;

  if(m_nImgExifEndian == 0)
  {
    // Little endian, byte swap required
    nVal = (nByte1 << 8) + nByte0;
  }
  else
  {
    // Big endian, no swap required
    nVal = (nByte0 << 8) + nByte1;
  }

  return nVal;
}

//-----------------------------------------------------------------------------
// Decode Canon Makernotes
// Only the most common makernotes are supported; there are a large
// number of makernotes that have not been documented anywhere.
CStr2 CjfifDecode::LookupMakerCanonTag(uint32_t nMainTag, uint32_t nSubTag, uint32_t nVal)
{
  QString strTmp;

  CStr2 sRetVal;

  sRetVal.strTag = "???";
  sRetVal.bUnknown = false;     // Set to true in default clauses
  sRetVal.strVal = QString("%1").arg(nVal);     // Provide default value

  uint32_t nValHi, nValLo;

  nValHi = (nVal & 0xff00) >> 8;
  nValLo = (nVal & 0x00ff);

  switch (nMainTag)
  {
    case 0x0001:
      switch (nSubTag)
      {
        case 0x0001:
          sRetVal.strTag = "Canon.Cs1.Macro";
          break;                // Short Macro mode

        case 0x0002:
          sRetVal.strTag = "Canon.Cs1.Selftimer";
          break;                // Short Self timer

        case 0x0003:
          sRetVal.strTag = "Canon.Cs1.Quality";

          if(nVal == 2)
          {
            sRetVal.strVal = "norm";
          }
          else if(nVal == 3)
          {
            sRetVal.strVal = "fine";
          }
          else if(nVal == 5)
          {
            sRetVal.strVal = "superfine";
          }
          else
          {
            sRetVal.strVal = "?";
          }

          // Save the quality string for later
          m_strImgQualExif = sRetVal.strVal;
          break;                // Short Quality

        case 0x0004:
          sRetVal.strTag = "Canon.Cs1.FlashMode";
          break;                // Short Flash mode setting

        case 0x0005:
          sRetVal.strTag = "Canon.Cs1.DriveMode";
          break;                // Short Drive mode setting

        case 0x0007:
          sRetVal.strTag = "Canon.Cs1.FocusMode";       // Short Focus mode setting

          switch (nVal)
          {
            case 0:
              sRetVal.strVal = "One-shot";
              break;

            case 1:
              sRetVal.strVal = "AI Servo";
              break;

            case 2:
              sRetVal.strVal = "AI Focus";
              break;

            case 3:
              sRetVal.strVal = "Manual Focus";
              break;

            case 4:
              sRetVal.strVal = "Single";
              break;

            case 5:
              sRetVal.strVal = "Continuous";
              break;

            case 6:
              sRetVal.strVal = "Manual Focus";
              break;

            default:
              sRetVal.strVal = "?";
              break;
          }

          break;

        case 0x000a:
          sRetVal.strTag = "Canon.Cs1.ImageSize";       // Short Image size

          if(nVal == 0)
          {
            sRetVal.strVal = "Large";
          }
          else if(nVal == 1)
          {
            sRetVal.strVal = "Medium";
          }
          else if(nVal == 2)
          {
            sRetVal.strVal = "Small";
          }
          else
          {
            sRetVal.strVal = "?";
          }

          break;

        case 0x000b:
          sRetVal.strTag = "Canon.Cs1.EasyMode";
          break;                // Short Easy shooting mode

        case 0x000c:
          sRetVal.strTag = "Canon.Cs1.DigitalZoom";
          break;                // Short Digital zoom

        case 0x000d:
          sRetVal.strTag = "Canon.Cs1.Contrast";
          break;                // Short Contrast setting

        case 0x000e:
          sRetVal.strTag = "Canon.Cs1.Saturation";
          break;                // Short Saturation setting

        case 0x000f:
          sRetVal.strTag = "Canon.Cs1.Sharpness";
          break;                // Short Sharpness setting

        case 0x0010:
          sRetVal.strTag = "Canon.Cs1.ISOSpeed";
          break;                // Short ISO speed setting

        case 0x0011:
          sRetVal.strTag = "Canon.Cs1.MeteringMode";
          break;                // Short Metering mode setting

        case 0x0012:
          sRetVal.strTag = "Canon.Cs1.FocusType";
          break;                // Short Focus type setting

        case 0x0013:
          sRetVal.strTag = "Canon.Cs1.AFPoint";
          break;                // Short AF point selected

        case 0x0014:
          sRetVal.strTag = "Canon.Cs1.ExposureProgram";
          break;                // Short Exposure mode setting

        case 0x0016:
          sRetVal.strTag = "Canon.Cs1.LensType";
          break;                //

        case 0x0017:
          sRetVal.strTag = "Canon.Cs1.Lens";
          break;                // Short 'long' and 'short' focal length of lens (in 'focal m_nImgUnits' and 'focal m_nImgUnits' per mm

        case 0x001a:
          sRetVal.strTag = "Canon.Cs1.MaxAperture";
          break;                //

        case 0x001b:
          sRetVal.strTag = "Canon.Cs1.MinAperture";
          break;                //

        case 0x001c:
          sRetVal.strTag = "Canon.Cs1.FlashActivity";
          break;                // Short Flash activity

        case 0x001d:
          sRetVal.strTag = "Canon.Cs1.FlashDetails";
          break;                // Short Flash details

        case 0x0020:
          sRetVal.strTag = "Canon.Cs1.FocusMode";
          break;                // Short Focus mode setting

        default:
          sRetVal.strTag = QString("Canon.Cs1.x%1").arg(nSubTag, 4, 16, QChar('0'));
          sRetVal.bUnknown = true;
          break;
      }                         // switch nSubTag

      break;

    case 0x0004:
      switch (nSubTag)
      {
        case 0x0002:
          sRetVal.strTag = "Canon.Cs2.ISOSpeed";
          break;                // Short ISO speed used
        case 0x0004:
          sRetVal.strTag = "Canon.Cs2.TargetAperture";
          break;                // Short Target Aperture
        case 0x0005:
          sRetVal.strTag = "Canon.Cs2.TargetShutterSpeed";
          break;                // Short Target shutter speed
        case 0x0007:
          sRetVal.strTag = "Canon.Cs2.WhiteBalance";
          break;                // Short White balance setting
        case 0x0009:
          sRetVal.strTag = "Canon.Cs2.Sequence";
          break;                // Short Sequence number (if in a continuous burst
        case 0x000e:
          sRetVal.strTag = "Canon.Cs2.AFPointUsed";
          break;                // Short AF point used
        case 0x000f:
          sRetVal.strTag = "Canon.Cs2.FlashBias";
          break;                // Short Flash bias
        case 0x0013:
          sRetVal.strTag = "Canon.Cs2.SubjectDistance";
          break;                // Short Subject distance (m_nImgUnits are not clear
        case 0x0015:
          sRetVal.strTag = "Canon.Cs2.ApertureValue";
          break;                // Short Aperture
        case 0x0016:
          sRetVal.strTag = "Canon.Cs2.ShutterSpeedValue";
          break;                // Short Shutter speed
        default:
          sRetVal.strTag = QString("Canon.Cs2.x%1").arg(nSubTag, 4, 16, QChar('0'));
          sRetVal.bUnknown = true;
          break;
      }                         // switch nSubTag

      break;

    case 0x000F:
      // CustomFunctions are different! Tag given by high byte, value by low
      // Index order (usually the nSubTag) is not used.
      sRetVal.strVal = QString("%1").arg(nValLo);       // Provide default value

      switch (nValHi)
      {
        case 0x0001:
          sRetVal.strTag = "Canon.Cf.NoiseReduction";
          break;                // Short Long exposure noise reduction
        case 0x0002:
          sRetVal.strTag = "Canon.Cf.ShutterAeLock";
          break;                // Short Shutter/AE lock buttons
        case 0x0003:
          sRetVal.strTag = "Canon.Cf.MirrorLockup";
          break;                // Short Mirror lockup
        case 0x0004:
          sRetVal.strTag = "Canon.Cf.ExposureLevelIncrements";
          break;                // Short Tv/Av and exposure level
        case 0x0005:
          sRetVal.strTag = "Canon.Cf.AFAssist";
          break;                // Short AF assist light
        case 0x0006:
          sRetVal.strTag = "Canon.Cf.FlashSyncSpeedAv";
          break;                // Short Shutter speed in Av mode
        case 0x0007:
          sRetVal.strTag = "Canon.Cf.AEBSequence";
          break;                // Short AEB sequence/auto cancellation
        case 0x0008:
          sRetVal.strTag = "Canon.Cf.ShutterCurtainSync";
          break;                // Short Shutter curtain sync
        case 0x0009:
          sRetVal.strTag = "Canon.Cf.LensAFStopButton";
          break;                // Short Lens AF stop button Fn. Switch
        case 0x000a:
          sRetVal.strTag = "Canon.Cf.FillFlashAutoReduction";
          break;                // Short Auto reduction of fill flash
        case 0x000b:
          sRetVal.strTag = "Canon.Cf.MenuButtonReturn";
          break;                // Short Menu button return position
        case 0x000c:
          sRetVal.strTag = "Canon.Cf.SetButtonFunction";
          break;                // Short SET button func. when shooting
        case 0x000d:
          sRetVal.strTag = "Canon.Cf.SensorCleaning";
          break;                // Short Sensor cleaning
        case 0x000e:
          sRetVal.strTag = "Canon.Cf.SuperimposedDisplay";
          break;                // Short Superimposed display
        case 0x000f:
          sRetVal.strTag = "Canon.Cf.ShutterReleaseNoCFCard";
          break;                // Short Shutter Release W/O CF Card
        default:
          sRetVal.strTag = QString("Canon.Cf.x%1").arg(nValHi, 4, 16, QChar('0'));
          sRetVal.bUnknown = true;
          break;
      }
                            // switch nSubTag
      break;

/*
	// Other ones assumed to use high-byte/low-byte method:
	case 0x00C0:
    sRetVal.strVal = QString("%u"),nValLo; // Provide default value
		switch(nValHi)
		{
			//case 0x0001: sRetVal.strTag = "Canon.x00C0.???";break; //
			default:
        sRetVal.strTag = QString("Canon.x00C0.x%04X"),nValHi;
				break;
		}
		break;

	case 0x00C1:
    sRetVal.strVal = QString("%u"),nValLo; // Provide default value
		switch(nValHi)
		{
			//case 0x0001: sRetVal.strTag = "Canon.x00C1.???";break; //
			default:
        sRetVal.strTag = QString("Canon.x00C1.x%04X"),nValHi;
				break;
		}
		break;
*/

    case 0x0012:
      switch (nSubTag)
      {
        case 0x0002:
          sRetVal.strTag = "Canon.Pi.ImageWidth";
          break;                //
        case 0x0003:
          sRetVal.strTag = "Canon.Pi.ImageHeight";
          break;                //
        case 0x0004:
          sRetVal.strTag = "Canon.Pi.ImageWidthAsShot";
          break;                //
        case 0x0005:
          sRetVal.strTag = "Canon.Pi.ImageHeightAsShot";
          break;                //
        case 0x0016:
          sRetVal.strTag = "Canon.Pi.AFPointsUsed";
          break;                //
        case 0x001a:
          sRetVal.strTag = "Canon.Pi.AFPointsUsed20D";
          break;                //
        default:
          sRetVal.strTag = QString("Canon.Pi.x%1").arg(nSubTag, 4, 16, QChar('0'));
          sRetVal.bUnknown = true;
          break;
      }                         // switch nSubTag

      break;

    default:
      sRetVal.strTag = QString("Canon.x%1.x%1")
          .arg(nMainTag, 4, 16, QChar('0'))
          .arg(nSubTag, 4, 16, QChar('0'));
      sRetVal.bUnknown = true;
      break;

  }                             // switch mainTag

  return sRetVal;
}

//-----------------------------------------------------------------------------
// Perform decode of EXIF IFD tags including MakerNote tags
//
// PRE:
// - m_strImgExifMake   Used for MakerNote decode
//
// INPUT:
// - strSect                    IFD section
// - nTag                               Tag code value
//
// OUTPUT:
// - bUnknown                   Was the tag unknown?
//
// RETURN:
// - Formatted string
//
QString CjfifDecode::LookupExifTag(QString strSect, uint32_t nTag, bool & bUnknown)
{
  QString strTmp;

  bUnknown = false;

  if(strSect == "IFD0")
  {
    switch (nTag)
    {
      case 0x010E:
        return QString("ImageDescription");
        break;                  // ascii string Describes image
      case 0x010F:
        return QString("Make");
        break;                  // ascii string Shows manufacturer of digicam
      case 0x0110:
        return QString("Model");
        break;                  // ascii string Shows model number of digicam
      case 0x0112:
        return QString("Orientation");
        break;                  // unsigned short 1  The orientation of the camera relative to the scene, when the image was captured. The start point of stored data is, '1' means upper left, '3' lower right, '6' upper right, '8' lower left, '9' undefined.
      case 0x011A:
        return QString("XResolution");
        break;                  // unsigned rational 1  Display/Print resolution of image. Large number of digicam uses 1/72inch, but it has no mean because personal computer doesn't use this value to display/print out.
      case 0x011B:
        return QString("YResolution");
        break;                  // unsigned rational 1
      case 0x0128:
        return QString("ResolutionUnit");
        break;                  // unsigned short 1  Unit of XResolution(0x011a)/YResolution(0x011b. '1' means no-unit, '2' means inch, '3' means centimeter.
      case 0x0131:
        return QString("Software");
        break;                  //  ascii string Shows firmware(internal software of digicam version number.
      case 0x0132:
        return QString("DateTime");
        break;                  // ascii string 20  Date/Time of image was last modified. Data format is "YYYY:MM:DD HH:MM:SS"+0x00, total 20bytes. In usual, it has the same value of DateTimeOriginal(0x9003
      case 0x013B:
        return QString("Artist");
        break;                  // Seems to be here and not only in SubIFD (maybe instead of SubIFD
      case 0x013E:
        return QString("WhitePoint");
        break;                  // unsigned rational 2  Defines chromaticity of white point of the image. If the image uses CIE Standard Illumination D65(known as international standard of 'daylight', the values are '3127/10000,3290/10000'.
      case 0x013F:
        return QString("PrimChromaticities");
        break;                  // unsigned rational 6  Defines chromaticity of the primaries of the image. If the image uses CCIR Recommendation 709 primearies, values are '640/1000,330/1000,300/1000,600/1000,150/1000,0/1000'.
      case 0x0211:
        return QString("YCbCrCoefficients");
        break;                  // unsigned rational 3  When image format is YCbCr, this value shows a constant to translate it to RGB format. In usual, values are '0.299/0.587/0.114'.
      case 0x0213:
        return QString("YCbCrPositioning");
        break;                  // unsigned short 1  When image format is YCbCr and uses 'Subsampling'(cropping of chroma data, all the digicam do that, defines the chroma sample point of subsampling pixel array. '1' means the center of pixel array, '2' means the datum point.
      case 0x0214:
        return QString("ReferenceBlackWhite");
        break;                  // unsigned rational 6  Shows reference value of black point/white point. In case of YCbCr format, first 2 show black/white of Y, next 2 are Cb, last 2 are Cr. In case of RGB format, first 2 show black/white of R, next 2 are G, last 2 are B.
      case 0x8298:
        return QString("Copyright");
        break;                  // ascii string Shows copyright information
      case 0x8769:
        return QString("ExifOffset");
        break;                  //unsigned long 1  Offset to Exif Sub IFD
      case 0x8825:
        return QString("GPSOffset");
        break;                  //unsigned long 1  Offset to Exif GPS IFD
//NEW:
      case 0x9C9B:
        return QString("XPTitle");
        break;
      case 0x9C9C:
        return QString("XPComment");
        break;
      case 0x9C9D:
        return QString("XPAuthor");
        break;
      case 0x9C9e:
        return QString("XPKeywords");
        break;
      case 0x9C9f:
        return QString("XPSubject");
        break;
//NEW: The following were found in IFD0 even though they should just be SubIFD?
      case 0xA401:
        return QString("CustomRendered");
        break;
      case 0xA402:
        return QString("ExposureMode");
        break;
      case 0xA403:
        return QString("WhiteBalance");
        break;
      case 0xA406:
        return QString("SceneCaptureType");
        break;

      default:
        strTmp = QString("IFD0.0x%1").arg(nTag, 4, 16, QChar('0'));
        bUnknown = true;
        return strTmp;
        break;
    }
  }
  else if(strSect == "SubIFD")
  {
    switch (nTag)
    {
      case 0x00fe:
        return QString("NewSubfileType");
        break;                  //  unsigned long 1
      case 0x00ff:
        return QString("SubfileType");
        break;                  //  unsigned short 1
      case 0x012d:
        return QString("TransferFunction");
        break;                  //  unsigned short 3
      case 0x013b:
        return QString("Artist");
        break;                  //  ascii string
      case 0x013d:
        return QString("Predictor");
        break;                  //  unsigned short 1
      case 0x0142:
        return QString("TileWidth");
        break;                  //  unsigned short 1
      case 0x0143:
        return QString("TileLength");
        break;                  //  unsigned short 1
      case 0x0144:
        return QString("TileOffsets");
        break;                  //  unsigned long
      case 0x0145:
        return QString("TileByteCounts");
        break;                  //  unsigned short
      case 0x014a:
        return QString("SubIFDs");
        break;                  //  unsigned long
      case 0x015b:
        return QString("JPEGTables");
        break;                  //  undefined
      case 0x828d:
        return QString("CFARepeatPatternDim");
        break;                  //  unsigned short 2
      case 0x828e:
        return QString("CFAPattern");
        break;                  //  unsigned byte
      case 0x828f:
        return QString("BatteryLevel");
        break;                  //  unsigned rational 1
      case 0x829A:
        return QString("ExposureTime");
        break;
      case 0x829D:
        return QString("FNumber");
        break;
      case 0x83bb:
        return QString("IPTC/NAA");
        break;                  //  unsigned long
      case 0x8773:
        return QString("InterColorProfile");
        break;                  //  undefined
      case 0x8822:
        return QString("ExposureProgram");
        break;
      case 0x8824:
        return QString("SpectralSensitivity");
        break;                  //  ascii string
      case 0x8825:
        return QString("GPSInfo");
        break;                  //  unsigned long 1
      case 0x8827:
        return QString("ISOSpeedRatings");
        break;
      case 0x8828:
        return QString("OECF");
        break;                  //  undefined
      case 0x8829:
        return QString("Interlace");
        break;                  //  unsigned short 1
      case 0x882a:
        return QString("TimeZoneOffset");
        break;                  //  signed short 1
      case 0x882b:
        return QString("SelfTimerMode");
        break;                  //  unsigned short 1
      case 0x9000:
        return QString("ExifVersion");
        break;
      case 0x9003:
        return QString("DateTimeOriginal");
        break;
      case 0x9004:
        return QString("DateTimeDigitized");
        break;
      case 0x9101:
        return QString("ComponentsConfiguration");
        break;
      case 0x9102:
        return QString("CompressedBitsPerPixel");
        break;
      case 0x9201:
        return QString("ShutterSpeedValue");
        break;
      case 0x9202:
        return QString("ApertureValue");
        break;
      case 0x9203:
        return QString("BrightnessValue");
        break;
      case 0x9204:
        return QString("ExposureBiasValue");
        break;
      case 0x9205:
        return QString("MaxApertureValue");
        break;
      case 0x9206:
        return QString("SubjectDistance");
        break;
      case 0x9207:
        return QString("MeteringMode");
        break;
      case 0x9208:
        return QString("LightSource");
        break;
      case 0x9209:
        return QString("Flash");
        break;
      case 0x920A:
        return QString("FocalLength");
        break;
      case 0x920b:
        return QString("FlashEnergy");
        break;                  //  unsigned rational 1
      case 0x920c:
        return QString("SpatialFrequencyResponse");
        break;                  //  undefined
      case 0x920d:
        return QString("Noise");
        break;                  //  undefined
      case 0x9211:
        return QString("ImageNumber");
        break;                  //  unsigned long 1
      case 0x9212:
        return QString("SecurityClassification");
        break;                  //  ascii string 1
      case 0x9213:
        return QString("ImageHistory");
        break;                  //  ascii string
      case 0x9214:
        return QString("SubjectLocation");
        break;                  //  unsigned short 4
      case 0x9215:
        return QString("ExposureIndex");
        break;                  //  unsigned rational 1
      case 0x9216:
        return QString("TIFF/EPStandardID");
        break;                  //  unsigned byte 4
      case 0x927C:
        return QString("MakerNote");
        break;
      case 0x9286:
        return QString("UserComment");
        break;
      case 0x9290:
        return QString("SubSecTime");
        break;                  //  ascii string
      case 0x9291:
        return QString("SubSecTimeOriginal");
        break;                  //  ascii string
      case 0x9292:
        return QString("SubSecTimeDigitized");
        break;                  //  ascii string
      case 0xA000:
        return QString("FlashPixVersion");
        break;
      case 0xA001:
        return QString("ColorSpace");
        break;
      case 0xA002:
        return QString("ExifImageWidth");
        break;
      case 0xA003:
        return QString("ExifImageHeight");
        break;
      case 0xA004:
        return QString("RelatedSoundFile");
        break;
      case 0xA005:
        return QString("ExifInteroperabilityOffset");
        break;
      case 0xa20b:
        return QString("FlashEnergy  unsigned");
        break;                  // rational 1
      case 0xa20c:
        return QString("SpatialFrequencyResponse");
        break;                  //  unsigned short 1
      case 0xA20E:
        return QString("FocalPlaneXResolution");
        break;
      case 0xA20F:
        return QString("FocalPlaneYResolution");
        break;
      case 0xA210:
        return QString("FocalPlaneResolutionUnit");
        break;
      case 0xa214:
        return QString("SubjectLocation");
        break;                  //  unsigned short 1
      case 0xa215:
        return QString("ExposureIndex");
        break;                  //  unsigned rational 1
      case 0xA217:
        return QString("SensingMethod");
        break;
      case 0xA300:
        return QString("FileSource");
        break;
      case 0xA301:
        return QString("SceneType");
        break;
      case 0xa302:
        return QString("CFAPattern");
        break;                  //  undefined 1
      case 0xa401:
        return QString("CustomRendered");
        break;                  // Short Custom image processing
      case 0xa402:
        return QString("ExposureMode");
        break;                  // Short Exposure mode
      case 0xa403:
        return QString("WhiteBalance");
        break;                  // Short White balance
      case 0xa404:
        return QString("DigitalZoomRatio");
        break;                  // Rational Digital zoom ratio
      case 0xa405:
        return QString("FocalLengthIn35mmFilm");
        break;                  // Short Focal length in 35 mm film
      case 0xa406:
        return QString("SceneCaptureType");
        break;                  // Short Scene capture type
      case 0xa407:
        return QString("GainControl");
        break;                  // Rational Gain control
      case 0xa408:
        return QString("Contrast");
        break;                  // Short Contrast
      case 0xa409:
        return QString("Saturation");
        break;                  // Short Saturation
      case 0xa40a:
        return QString("Sharpness");
        break;                  // Short Sharpness
      case 0xa40b:
        return QString("DeviceSettingDescription");
        break;                  // Undefined Device settings description
      case 0xa40c:
        return QString("SubjectDistanceRange");
        break;                  // Short Subject distance range
      case 0xa420:
        return QString("ImageUniqueID");
        break;                  // Ascii Unique image ID

      default:
        strTmp = QString("SubIFD.0x%1").arg(nTag, 4, 16, QChar('0'));
        bUnknown = true;
        return strTmp;
        break;
    }
  }
  else if(strSect == "IFD1")
  {
    switch (nTag)
    {
      case 0x0100:
        return QString("ImageWidth");
        break;                  //  unsigned short/long 1  Shows size of thumbnail image.
      case 0x0101:
        return QString("ImageLength");
        break;                  //  unsigned short/long 1
      case 0x0102:
        return QString("BitsPerSample");
        break;                  //  unsigned short 3  When image format is no compression, this value shows the number of bits per component for each pixel. Usually this value is '8,8,8'
      case 0x0103:
        return QString("Compression");
        break;                  //  unsigned short 1  Shows compression method. '1' means no compression, '6' means JPEG compression.
      case 0x0106:
        return QString("PhotometricInterpretation");
        break;                  //  unsigned short 1  Shows the color space of the image data components. '1' means monochrome, '2' means RGB, '6' means YCbCr.
      case 0x0111:
        return QString("StripOffsets");
        break;                  //  unsigned short/long When image format is no compression, this value shows offset to image data. In some case image data is striped and this value is plural.
      case 0x0115:
        return QString("SamplesPerPixel");
        break;                  //  unsigned short 1  When image format is no compression, this value shows the number of components stored for each pixel. At color image, this value is '3'.
      case 0x0116:
        return QString("RowsPerStrip");
        break;                  //  unsigned short/long 1  When image format is no compression and image has stored as strip, this value shows how many rows stored to each strip. If image has not striped, this value is the same as ImageLength(0x0101.
      case 0x0117:
        return QString("StripByteConunts");
        break;                  //  unsigned short/long  When image format is no compression and stored as strip, this value shows how many bytes used for each strip and this value is plural. If image has not stripped, this value is single and means whole data size of image.
      case 0x011a:
        return QString("XResolution");
        break;                  //  unsigned rational 1  Display/Print resolution of image. Large number of digicam uses 1/72inch, but it has no mean because personal computer doesn't use this value to display/print out.
      case 0x011b:
        return QString("YResolution");
        break;                  //  unsigned rational 1
      case 0x011c:
        return QString("PlanarConfiguration");
        break;                  //  unsigned short 1  When image format is no compression YCbCr, this value shows byte aligns of YCbCr data. If value is '1', Y/Cb/Cr value is chunky format, contiguous for each subsampling pixel. If value is '2', Y/Cb/Cr value is separated and stored to Y plane/Cb plane/Cr plane format.
      case 0x0128:
        return QString("ResolutionUnit");
        break;                  //  unsigned short 1  Unit of XResolution(0x011a)/YResolution(0x011b. '1' means inch, '2' means centimeter.
      case 0x0201:
        return QString("JpegIFOffset");
        break;                  //  unsigned long 1  When image format is JPEG, this value show offset to JPEG data stored.
      case 0x0202:
        return QString("JpegIFByteCount");
        break;                  //  unsigned long 1  When image format is JPEG, this value shows data size of JPEG image.
      case 0x0211:
        return QString("YCbCrCoefficients");
        break;                  //  unsigned rational 3  When image format is YCbCr, this value shows constants to translate it to RGB format. In usual, '0.299/0.587/0.114' are used.
      case 0x0212:
        return QString("YCbCrSubSampling");
        break;                  //  unsigned short 2  When image format is YCbCr and uses subsampling(cropping of chroma data, all the digicam do that, this value shows how many chroma data subsampled. First value shows horizontal, next value shows vertical subsample rate.
      case 0x0213:
        return QString("YCbCrPositioning");
        break;                  //  unsigned short 1  When image format is YCbCr and uses 'Subsampling'(cropping of chroma data, all the digicam do that), this value defines the chroma sample point of subsampled pixel array. '1' means the center of pixel array, '2' means the datum point(0,0.
      case 0x0214:
        return QString("ReferenceBlackWhite");
        break;                  //  unsigned rational 6  Shows reference value of black point/white point. In case of YCbCr format, first 2 show black/white of Y, next 2 are Cb, last 2 are Cr. In case of RGB format, first 2 show black/white of R, next 2 are G, last 2 are B.

      default:
        strTmp = QString("IFD1.0x%1").arg(nTag, 4, 16, QChar('0'));
        bUnknown = true;
        return strTmp;
        break;

    }

  }
  else if(strSect == "InteropIFD")
  {
    switch (nTag)
    {
      case 0x0001:
        return QString("InteroperabilityIndex");
        break;
      case 0x0002:
        return QString("InteroperabilityVersion");
        break;
      case 0x1000:
        return QString("RelatedImageFileFormat");
        break;
      case 0x1001:
        return QString("RelatedImageWidth");
        break;
      case 0x1002:
        return QString("RelatedImageLength");
        break;

      default:
        strTmp = QString("Interop.0x%1").arg(nTag, 4, 16, QChar('0'));
        bUnknown = true;
        return strTmp;
        break;
    }
  }
  else if(strSect == "GPSIFD")
  {
    switch (nTag)
    {
      case 0x0000:
        return QString("GPSVersionID");
        break;
      case 0x0001:
        return QString("GPSLatitudeRef");
        break;
      case 0x0002:
        return QString("GPSLatitude");
        break;
      case 0x0003:
        return QString("GPSLongitudeRef");
        break;
      case 0x0004:
        return QString("GPSLongitude");
        break;
      case 0x0005:
        return QString("GPSAltitudeRef");
        break;
      case 0x0006:
        return QString("GPSAltitude");
        break;
      case 0x0007:
        return QString("GPSTimeStamp");
        break;
      case 0x0008:
        return QString("GPSSatellites");
        break;
      case 0x0009:
        return QString("GPSStatus");
        break;
      case 0x000A:
        return QString("GPSMeasureMode");
        break;
      case 0x000B:
        return QString("GPSDOP");
        break;
      case 0x000C:
        return QString("GPSSpeedRef");
        break;
      case 0x000D:
        return QString("GPSSpeed");
        break;
      case 0x000E:
        return QString("GPSTrackRef");
        break;
      case 0x000F:
        return QString("GPSTrack");
        break;
      case 0x0010:
        return QString("GPSImgDirectionRef");
        break;
      case 0x0011:
        return QString("GPSImgDirection");
        break;
      case 0x0012:
        return QString("GPSMapDatum");
        break;
      case 0x0013:
        return QString("GPSDestLatitudeRef");
        break;
      case 0x0014:
        return QString("GPSDestLatitude");
        break;
      case 0x0015:
        return QString("GPSDestLongitudeRef");
        break;
      case 0x0016:
        return QString("GPSDestLongitude");
        break;
      case 0x0017:
        return QString("GPSDestBearingRef");
        break;
      case 0x0018:
        return QString("GPSDestBearing");
        break;
      case 0x0019:
        return QString("GPSDestDistanceRef");
        break;
      case 0x001A:
        return QString("GPSDestDistance");
        break;
      case 0x001B:
        return QString("GPSProcessingMethod");
        break;
      case 0x001C:
        return QString("GPSAreaInformation");
        break;
      case 0x001D:
        return QString("GPSDateStamp");
        break;
      case 0x001E:
        return QString("GPSDifferential");
        break;

      default:
        strTmp = QString("GPS.0x%").arg(nTag, 4, 16, QChar('0'));
        bUnknown = true;
        return strTmp;
        break;
    }
  }
  else if(strSect == "MakerIFD")
  {

    // Makernotes need special handling
    // We only support a few different manufacturers for makernotes.

    // A few Canon tags are supported in this routine, the rest are
    // handled by the LookupMakerCanonTag() call.
    if(m_strImgExifMake == "Canon")
    {
      switch (nTag)
      {
        case 0x0001:
          return QString("Canon.CameraSettings1");
          break;
        case 0x0004:
          return QString("Canon.CameraSettings2");
          break;
        case 0x0006:
          return QString("Canon.ImageType");
          break;
        case 0x0007:
          return QString("Canon.FirmwareVersion");
          break;
        case 0x0008:
          return QString("Canon.ImageNumber");
          break;
        case 0x0009:
          return QString("Canon.OwnerName");
          break;
        case 0x000C:
          return QString("Canon.SerialNumber");
          break;
        case 0x000F:
          return QString("Canon.CustomFunctions");
          break;
        case 0x0012:
          return QString("Canon.PictureInfo");
          break;
        case 0x00A9:
          return QString("Canon.WhiteBalanceTable");
          break;

        default:
          strTmp = QString("Canon.0x%1").arg(nTag, 4, 16, QChar('0'));
          bUnknown = true;
          return strTmp;
          break;
      }
    }  // Canon
    else if(m_strImgExifMake == "SIGMA")
    {
      switch (nTag)
      {
        case 0x0002:
          return QString("Sigma.SerialNumber");
          break;                // Ascii Camera serial number
        case 0x0003:
          return QString("Sigma.DriveMode");
          break;                // Ascii Drive Mode
        case 0x0004:
          return QString("Sigma.ResolutionMode");
          break;                // Ascii Resolution Mode
        case 0x0005:
          return QString("Sigma.AutofocusMode");
          break;                // Ascii Autofocus mode
        case 0x0006:
          return QString("Sigma.FocusSetting");
          break;                // Ascii Focus setting
        case 0x0007:
          return QString("Sigma.WhiteBalance");
          break;                // Ascii White balance
        case 0x0008:
          return QString("Sigma.ExposureMode");
          break;                // Ascii Exposure mode
        case 0x0009:
          return QString("Sigma.MeteringMode");
          break;                // Ascii Metering mode
        case 0x000a:
          return QString("Sigma.LensRange");
          break;                // Ascii Lens focal length range
        case 0x000b:
          return QString("Sigma.ColorSpace");
          break;                // Ascii Color space
        case 0x000c:
          return QString("Sigma.Exposure");
          break;                // Ascii Exposure
        case 0x000d:
          return QString("Sigma.Contrast");
          break;                // Ascii Contrast
        case 0x000e:
          return QString("Sigma.Shadow");
          break;                // Ascii Shadow
        case 0x000f:
          return QString("Sigma.Highlight");
          break;                // Ascii Highlight
        case 0x0010:
          return QString("Sigma.Saturation");
          break;                // Ascii Saturation
        case 0x0011:
          return QString("Sigma.Sharpness");
          break;                // Ascii Sharpness
        case 0x0012:
          return QString("Sigma.FillLight");
          break;                // Ascii X3 Fill light
        case 0x0014:
          return QString("Sigma.ColorAdjustment");
          break;                // Ascii Color adjustment
        case 0x0015:
          return QString("Sigma.AdjustmentMode");
          break;                // Ascii Adjustment mode
        case 0x0016:
          return QString("Sigma.Quality");
          break;                // Ascii Quality
        case 0x0017:
          return QString("Sigma.Firmware");
          break;                // Ascii Firmware
        case 0x0018:
          return QString("Sigma.Software");
          break;                // Ascii Software
        case 0x0019:
          return QString("Sigma.AutoBracket");
          break;                // Ascii Auto bracket
        default:
          strTmp = QString("Sigma.0x%1").arg(nTag, 4, 16, QChar('0'));
          bUnknown = true;
          return strTmp;
          break;
      }
    }                           // SIGMA
    else if(m_strImgExifMake == "SONY")
    {
      switch (nTag)
      {
        case 0xb021:
          return QString("Sony.ColorTemperature");
          break;
        case 0xb023:
          return QString("Sony.SceneMode");
          break;
        case 0xb024:
          return QString("Sony.ZoneMatching");
          break;
        case 0xb025:
          return QString("Sony.DynamicRangeOptimizer");
          break;
        case 0xb026:
          return QString("Sony.ImageStabilization");
          break;
        case 0xb027:
          return QString("Sony.LensID");
          break;
        case 0xb029:
          return QString("Sony.ColorMode");
          break;
        case 0xb040:
          return QString("Sony.Macro");
          break;
        case 0xb041:
          return QString("Sony.ExposureMode");
          break;
        case 0xb047:
          return QString("Sony.Quality");
          break;
        case 0xb04e:
          return QString("Sony.LongExposureNoiseReduction");
          break;
        default:
          // No real info is known
          strTmp = QString("Sony.0x%1").arg(nTag, 4, 16, QChar('0'));
          bUnknown = true;
          return strTmp;
          break;
      }
    }                           // SONY
    else if(m_strImgExifMake == "FUJIFILM")
    {
      switch (nTag)
      {
        case 0x0000:
          return QString("Fujifilm.Version");
          break;                // Undefined Fujifilm Makernote version
        case 0x1000:
          return QString("Fujifilm.Quality");
          break;                // Ascii Image quality setting
        case 0x1001:
          return QString("Fujifilm.Sharpness");
          break;                // Short Sharpness setting
        case 0x1002:
          return QString("Fujifilm.WhiteBalance");
          break;                // Short White balance setting
        case 0x1003:
          return QString("Fujifilm.Color");
          break;                // Short Chroma saturation setting
        case 0x1004:
          return QString("Fujifilm.Tone");
          break;                // Short Contrast setting
        case 0x1010:
          return QString("Fujifilm.FlashMode");
          break;                // Short Flash firing mode setting
        case 0x1011:
          return QString("Fujifilm.FlashStrength");
          break;                // SRational Flash firing strength compensation setting
        case 0x1020:
          return QString("Fujifilm.Macro");
          break;                // Short Macro mode setting
        case 0x1021:
          return QString("Fujifilm.FocusMode");
          break;                // Short Focusing mode setting
        case 0x1030:
          return QString("Fujifilm.SlowSync");
          break;                // Short Slow synchro mode setting
        case 0x1031:
          return QString("Fujifilm.PictureMode");
          break;                // Short Picture mode setting
        case 0x1100:
          return QString("Fujifilm.Continuous");
          break;                // Short Continuous shooting or auto bracketing setting
        case 0x1210:
          return QString("Fujifilm.FinePixColor");
          break;                // Short Fuji FinePix Color setting
        case 0x1300:
          return QString("Fujifilm.BlurWarning");
          break;                // Short Blur warning status
        case 0x1301:
          return QString("Fujifilm.FocusWarning");
          break;                // Short Auto Focus warning status
        case 0x1302:
          return QString("Fujifilm.AeWarning");
          break;                // Short Auto Exposure warning status
        default:
          strTmp = QString("Fujifilm.0x%1").arg(nTag, 4, 16, QChar('0'));
          bUnknown = true;
          return strTmp;
          break;
      }
    }                           // FUJIFILM
    else if(m_strImgExifMake == "NIKON")
    {
      if(m_nImgExifMakeSubtype == 1)
      {
        // Type 1
        switch (nTag)
        {
          case 0x0001:
            return QString("Nikon1.Version");
            break;              // Undefined Nikon Makernote version
          case 0x0002:
            return QString("Nikon1.ISOSpeed");
            break;              // Short ISO speed setting
          case 0x0003:
            return QString("Nikon1.ColorMode");
            break;              // Ascii Color mode
          case 0x0004:
            return QString("Nikon1.Quality");
            break;              // Ascii Image quality setting
          case 0x0005:
            return QString("Nikon1.WhiteBalance");
            break;              // Ascii White balance
          case 0x0006:
            return QString("Nikon1.Sharpening");
            break;              // Ascii Image sharpening setting
          case 0x0007:
            return QString("Nikon1.Focus");
            break;              // Ascii Focus mode
          case 0x0008:
            return QString("Nikon1.Flash");
            break;              // Ascii Flash mode
          case 0x000f:
            return QString("Nikon1.ISOSelection");
            break;              // Ascii ISO selection
          case 0x0010:
            return QString("Nikon1.DataDump");
            break;              // Undefined Data dump
          case 0x0080:
            return QString("Nikon1.ImageAdjustment");
            break;              // Ascii Image adjustment setting
          case 0x0082:
            return QString("Nikon1.Adapter");
            break;              // Ascii Adapter used
          case 0x0085:
            return QString("Nikon1.FocusDistance");
            break;              // Rational Manual focus distance
          case 0x0086:
            return QString("Nikon1.DigitalZoom");
            break;              // Rational Digital zoom setting
          case 0x0088:
            return QString("Nikon1.AFFocusPos");
            break;              // Undefined AF focus position
          default:
            strTmp = QString("Nikon1.0x%1").arg(nTag, 4, 16, QChar('0'));
            bUnknown = true;
            return strTmp;
            break;
        }
      }
      else if(m_nImgExifMakeSubtype == 2)
      {
        // Type 2
        switch (nTag)
        {
          case 0x0003:
            return QString("Nikon2.Quality");
            break;              // Short Image quality setting
          case 0x0004:
            return QString("Nikon2.ColorMode");
            break;              // Short Color mode
          case 0x0005:
            return QString("Nikon2.ImageAdjustment");
            break;              // Short Image adjustment setting
          case 0x0006:
            return QString("Nikon2.ISOSpeed");
            break;              // Short ISO speed setting
          case 0x0007:
            return QString("Nikon2.WhiteBalance");
            break;              // Short White balance
          case 0x0008:
            return QString("Nikon2.Focus");
            break;              // Rational Focus mode
          case 0x000a:
            return QString("Nikon2.DigitalZoom");
            break;              // Rational Digital zoom setting
          case 0x000b:
            return QString("Nikon2.Adapter");
            break;              // Short Adapter used
          default:
            strTmp = QString("Nikon2.0x%1").arg(nTag, 4, 16, QChar('0'));
            bUnknown = true;
            return strTmp;
            break;
        }
      }
      else if(m_nImgExifMakeSubtype == 3)
      {
        // Type 3
        switch (nTag)
        {
          case 0x0001:
            return QString("Nikon3.Version");
            break;              // Undefined Nikon Makernote version
          case 0x0002:
            return QString("Nikon3.ISOSpeed");
            break;              // Short ISO speed used
          case 0x0003:
            return QString("Nikon3.ColorMode");
            break;              // Ascii Color mode
          case 0x0004:
            return QString("Nikon3.Quality");
            break;              // Ascii Image quality setting
          case 0x0005:
            return QString("Nikon3.WhiteBalance");
            break;              // Ascii White balance
          case 0x0006:
            return QString("Nikon3.Sharpening");
            break;              // Ascii Image sharpening setting
          case 0x0007:
            return QString("Nikon3.Focus");
            break;              // Ascii Focus mode
          case 0x0008:
            return QString("Nikon3.FlashSetting");
            break;              // Ascii Flash setting
          case 0x0009:
            return QString("Nikon3.FlashMode");
            break;              // Ascii Flash mode
          case 0x000b:
            return QString("Nikon3.WhiteBalanceBias");
            break;              // SShort White balance bias
          case 0x000e:
            return QString("Nikon3.ExposureDiff");
            break;              // Undefined Exposure difference
          case 0x000f:
            return QString("Nikon3.ISOSelection");
            break;              // Ascii ISO selection
          case 0x0010:
            return QString("Nikon3.DataDump");
            break;              // Undefined Data dump
          case 0x0011:
            return QString("Nikon3.ThumbOffset");
            break;              // Long Thumbnail IFD offset
          case 0x0012:
            return QString("Nikon3.FlashComp");
            break;              // Undefined Flash compensation setting
          case 0x0013:
            return QString("Nikon3.ISOSetting");
            break;              // Short ISO speed setting
          case 0x0016:
            return QString("Nikon3.ImageBoundary");
            break;              // Short Image boundry
          case 0x0018:
            return QString("Nikon3.FlashBracketComp");
            break;              // Undefined Flash bracket compensation applied
          case 0x0019:
            return QString("Nikon3.ExposureBracketComp");
            break;              // SRational AE bracket compensation applied
          case 0x0080:
            return QString("Nikon3.ImageAdjustment");
            break;              // Ascii Image adjustment setting
          case 0x0081:
            return QString("Nikon3.ToneComp");
            break;              // Ascii Tone compensation setting (contrast
          case 0x0082:
            return QString("Nikon3.AuxiliaryLens");
            break;              // Ascii Auxiliary lens (adapter
          case 0x0083:
            return QString("Nikon3.LensType");
            break;              // Byte Lens type
          case 0x0084:
            return QString("Nikon3.Lens");
            break;              // Rational Lens
          case 0x0085:
            return QString("Nikon3.FocusDistance");
            break;              // Rational Manual focus distance
          case 0x0086:
            return QString("Nikon3.DigitalZoom");
            break;              // Rational Digital zoom setting
          case 0x0087:
            return QString("Nikon3.FlashType");
            break;              // Byte Type of flash used
          case 0x0088:
            return QString("Nikon3.AFFocusPos");
            break;              // Undefined AF focus position
          case 0x0089:
            return QString("Nikon3.Bracketing");
            break;              // Short Bracketing
          case 0x008b:
            return QString("Nikon3.LensFStops");
            break;              // Undefined Number of lens stops
          case 0x008c:
            return QString("Nikon3.ToneCurve");
            break;              // Undefined Tone curve
          case 0x008d:
            return QString("Nikon3.ColorMode");
            break;              // Ascii Color mode
          case 0x008f:
            return QString("Nikon3.SceneMode");
            break;              // Ascii Scene mode
          case 0x0090:
            return QString("Nikon3.LightingType");
            break;              // Ascii Lighting type
          case 0x0092:
            return QString("Nikon3.HueAdjustment");
            break;              // SShort Hue adjustment
          case 0x0094:
            return QString("Nikon3.Saturation");
            break;              // SShort Saturation adjustment
          case 0x0095:
            return QString("Nikon3.NoiseReduction");
            break;              // Ascii Noise reduction
          case 0x0096:
            return QString("Nikon3.CompressionCurve");
            break;              // Undefined Compression curve
          case 0x0097:
            return QString("Nikon3.ColorBalance2");
            break;              // Undefined Color balance 2
          case 0x0098:
            return QString("Nikon3.LensData");
            break;              // Undefined Lens data
          case 0x0099:
            return QString("Nikon3.NEFThumbnailSize");
            break;              // Short NEF thumbnail size
          case 0x009a:
            return QString("Nikon3.SensorPixelSize");
            break;              // Rational Sensor pixel size
          case 0x00a0:
            return QString("Nikon3.SerialNumber");
            break;              // Ascii Camera serial number
          case 0x00a7:
            return QString("Nikon3.ShutterCount");
            break;              // Long Number of shots taken by camera
          case 0x00a9:
            return QString("Nikon3.ImageOptimization");
            break;              // Ascii Image optimization
          case 0x00aa:
            return QString("Nikon3.Saturation");
            break;              // Ascii Saturation
          case 0x00ab:
            return QString("Nikon3.VariProgram");
            break;              // Ascii Vari program

          default:
            strTmp = QString("Nikon3.0x%1").arg(nTag, 4, 16, QChar('0'));
            bUnknown = true;
            return strTmp;
            break;
        }
      }
    }                           // NIKON
  }                             // if strSect

  bUnknown = true;
  return QString("???");
}

//-----------------------------------------------------------------------------
// Interpret the MakerNote header to determine any applicable MakerNote subtype.
//
// PRE:
// - m_strImgExifMake
// - buffer
//
// INPUT:
// - none
//
// RETURN:
// - Decode success
//
// POST:
// - m_nImgExifMakeSubtype
//
bool CjfifDecode::DecodeMakerSubType()
{
  QString strTmp;

  m_nImgExifMakeSubtype = 0;

  if(m_strImgExifMake == "NIKON")
  {
    strTmp = "";
    for(uint32_t nInd = 0; nInd < 5; nInd++)
    {
      strTmp += Buf(m_nPos + nInd);
    }

    if(strTmp == "Nikon")
    {
      if(Buf(m_nPos + 6) == 1)
      {
        // Type 1
        m_pLog->AddLine("    Nikon Makernote Type 1 detected");
        m_nImgExifMakeSubtype = 1;
        m_nPos += 8;
      }
      else if(Buf(m_nPos + 6) == 2)
      {
        // Type 3
        m_pLog->AddLine("    Nikon Makernote Type 3 detected");
        m_nImgExifMakeSubtype = 3;
        m_nPos += 18;
      }
      else
      {
        strTmp = "ERROR: Unknown Nikon Makernote Type";
        m_pLog->AddLineErr(strTmp);

        if(m_pAppConfig->interactive())
        {
          msgBox.setText(strTmp);
          msgBox.exec();
        }

        return false;
      }
    }
    else
    {
      // Type 2
      m_pLog->AddLine("    Nikon Makernote Type 2 detected");
      //m_nImgExifMakeSubtype = 2;
      // tests on D1 seem to indicate that it uses Type 1 headers
      m_nImgExifMakeSubtype = 1;
      m_nPos += 0;
    }

  }
  else if(m_strImgExifMake == "SIGMA")
  {
    strTmp = "";
    for(uint32_t ind = 0; ind < 8; ind++)
    {
      if(Buf(m_nPos + ind) != 0)
        strTmp += Buf(m_nPos + ind);
    }
    if((strTmp == "SIGMA") || (strTmp == "FOVEON"))
    {
      // Valid marker
      // Now skip over the 8-chars and 2 unknown chars
      m_nPos += 10;
    }
    else
    {
      strTmp = "ERROR: Unknown SIGMA Makernote identifier";
      m_pLog->AddLineErr(strTmp);

      if(m_pAppConfig->interactive())
      {
        msgBox.setText(strTmp);
        msgBox.exec();
      }

      return false;
    }

  }                             // SIGMA
  else if(m_strImgExifMake == "FUJIFILM")
  {
    strTmp = "";

    for(uint32_t ind = 0; ind < 8; ind++)
    {
      if(Buf(m_nPos + ind) != 0)
        strTmp += Buf(m_nPos + ind);
    }

    if(strTmp == "FUJIFILM")
    {
      // Valid marker
      // Now skip over the 8-chars and 4 Pointer chars
      // FIXME: Do I need to dereference this pointer?
      m_nPos += 12;
    }
    else
    {
      strTmp = "ERROR: Unknown FUJIFILM Makernote identifier";
      m_pLog->AddLineErr(strTmp);

      if(m_pAppConfig->interactive())
      {
        msgBox.setText(strTmp);
        msgBox.exec();
      }

      return false;
    }

  }                             // FUJIFILM
  else if(m_strImgExifMake == "SONY")
  {
    strTmp = "";

    for(uint32_t ind = 0; ind < 12; ind++)
    {
      if(Buf(m_nPos + ind) != 0)
        strTmp += Buf(m_nPos + ind);
    }

    if(strTmp == "SONY DSC ")
    {
      // Valid marker
      // Now skip over the 9-chars and 3 null chars
      m_nPos += 12;
    }
    else
    {
      strTmp = "ERROR: Unknown SONY Makernote identifier";
      m_pLog->AddLineErr(strTmp);

      if(m_pAppConfig->interactive())
      {
        msgBox.setText(strTmp);
        msgBox.exec();
      }

      return false;
    }
  }                             // SONY

  return true;
}

//-----------------------------------------------------------------------------
// Read two UINT32 from the buffer (8B) and interpret
// as a rational entry. Convert to floating point.
// Byte swap as required
//
// INPUT:
// - pos                Buffer position
// - val                Floating point value
//
// RETURN:
// - Was the conversion successful?
//
bool CjfifDecode::DecodeValRational(uint32_t nPos, double &nVal)
{
  int nValNumer;

  int nValDenom;

  nVal = 0;

  nValNumer = ByteSwap4(Buf(nPos + 0), Buf(nPos + 1), Buf(nPos + 2), Buf(nPos + 3));
  nValDenom = ByteSwap4(Buf(nPos + 4), Buf(nPos + 5), Buf(nPos + 6), Buf(nPos + 7));

  if(nValDenom == 0)
  {
    // Divide by zero!
    return false;
  }
  else
  {
    nVal = static_cast<double>(nValNumer) / static_cast<double>(nValDenom);
    return true;
  }
}

//-----------------------------------------------------------------------------
// Read two UINT32 from the buffer (8B) to create a formatted
// fraction string. Byte swap as required
//
// INPUT:
// - pos                Buffer position
//
// RETURN:
// - Formatted string
//
QString CjfifDecode::DecodeValFraction(uint32_t nPos)
{
  QString strTmp;

  int nValNumer = ReadSwap4(nPos + 0);

  int nValDenom = ReadSwap4(nPos + 4);

  strTmp = QString("%1/%2").arg(nValNumer).arg(nValDenom);
  return strTmp;
}

//-----------------------------------------------------------------------------
// Convert multiple coordinates into a formatted GPS string
//
// INPUT:
// - nCount             Number of coordinates (1,2,3)
// - fCoord1    Coordinate #1
// - fCoord2    Coordinate #2
// - fCoord3    Coordinate #3
//
// OUTPUT:
// - strCoord   The formatted GPS string
//
// RETURN:
// - Was the conversion successful?
//
bool CjfifDecode::PrintValGPS(uint32_t nCount, double fCoord1, double fCoord2, double fCoord3, QString & strCoord)
{
  double fTemp;

  uint32_t nCoordDeg;

  uint32_t nCoordMin;

  double fCoordSec;

  // TODO: Extend to support 1 & 2 coordinate GPS entries
  if(nCount == 3)
  {
    nCoordDeg = uint32_t (fCoord1);

    nCoordMin = uint32_t (fCoord2);

    if(fCoord3 == 0)
    {
      fTemp = fCoord2 - static_cast<double>(nCoordMin);
      fCoordSec = fTemp * static_cast<double>(60.0);
    }
    else
    {
      fCoordSec = fCoord3;
    }

    strCoord = QString("%1 deg %2' %.3f\"").arg(nCoordDeg).arg(nCoordMin).arg(fCoordSec);
    return true;
  }
  else
  {
    strCoord = QString("ERROR: Can't handle %1-comonent GPS coords").arg(nCount);
    return false;
  }

}

//-----------------------------------------------------------------------------
// Read in 3 rational values from the buffer and output as a formatted GPS string
//
// INPUT:
// - pos                Buffer position
//
// OUTPUT:
// - strCoord   The formatted GPS string
//
// RETURN:
// - Was the conversion successful?
//
bool CjfifDecode::DecodeValGPS(uint32_t nPos, QString & strCoord)
{
  double fCoord1 = 0;
  double fCoord2 = 0;
  double fCoord3 = 0;

  bool bRet;

  bRet = true;
  if(bRet)
  {
    bRet = DecodeValRational(nPos, fCoord1);
    nPos += 8;
  }

  if(bRet)
  {
    bRet = DecodeValRational(nPos, fCoord2);
    nPos += 8;
  }

  if(bRet)
  {
    bRet = DecodeValRational(nPos, fCoord3);
    nPos += 8;
  }

  if(!bRet)
  {
    strCoord = QString("???");
    return false;
  }
  else
  {
    return PrintValGPS(3, fCoord1, fCoord2, fCoord3, strCoord);
  }
}

//-----------------------------------------------------------------------------
// Read a UINT16 from the buffer, byte swap as required
//
// INPUT:
// - nPos               Buffer position
//
// RETURN:
// - UINT16 from buffer
//
uint32_t CjfifDecode::ReadSwap2(uint32_t nPos)
{
  return ByteSwap2(Buf(nPos + 0), Buf(nPos + 1));
}

//-----------------------------------------------------------------------------
// Read a UINT32 from the buffer, byte swap as required
//
// INPUT:
// - nPos               Buffer position
//
// RETURN:
// - UINT32 from buffer
//
uint32_t CjfifDecode::ReadSwap4(uint32_t nPos)
{
  return ByteSwap4(Buf(nPos), Buf(nPos + 1), Buf(nPos + 2), Buf(nPos + 3));
}

//-----------------------------------------------------------------------------
// Read a UINT32 from the buffer, force as big endian
//
// INPUT:
// - nPos               Buffer position
//
// RETURN:
// - UINT32 from buffer
//
uint32_t CjfifDecode::ReadBe4(uint32_t nPos)
{
  // Big endian, no swap required
  return (Buf(nPos) << 24) + (Buf(nPos + 1) << 16) + (Buf(nPos + 2) << 8) + Buf(nPos + 3);
}

//-----------------------------------------------------------------------------
// Print hex from array of unsigned char
//
// INPUT:
// - anBytes    Array of unsigned chars
// - nCount             Indicates the number of array entries originally specified
//                              but the printing routine limits it to the maximum array depth
//                              allocated (MAX_anValues) and add an ellipsis "..."
// RETURN:
// - A formatted string
//
QString CjfifDecode::PrintAsHexUC(uint8_t *anBytes, uint32_t nCount)
{
  QString strVal;

  QString strFull;

  strFull = "0x[";
  uint32_t nMaxDisplay = MAX_anValues;

  bool bExceedMaxDisplay;

  bExceedMaxDisplay = (nCount > nMaxDisplay);
  for(uint32_t nInd = 0; nInd < nCount; nInd++)
  {
    if(nInd < nMaxDisplay)
    {
      if((nInd % 4) == 0)
      {
        if(nInd == 0)
        {
          // Don't do anything for first value!
        }
        else
        {
          // Every 16 add big spacer / break
          strFull += " ";
        }
      }
      else
      {
        //strFull += " ";
      }

      strVal = QString("%1").arg(anBytes[nInd], 2, 16, QChar('0'));
      strFull += strVal;
    }

    if((nInd == nMaxDisplay) && (bExceedMaxDisplay))
    {
      strFull += "...";
    }

  }

  strFull += "]";
  return strFull;
}

//-----------------------------------------------------------------------------
// Print hex from array of unsigned bytes
//
// INPUT:
// - anBytes    Array is passed as UINT32* even though it only
//                              represents a byte per entry
// - nCount             Indicates the number of array entries originally specified
//                              but the printing routine limits it to the maximum array depth
//                              allocated (MAX_anValues) and add an ellipsis "..."
//
// RETURN:
// - A formatted string
//
QString CjfifDecode::PrintAsHex8(uint32_t *anBytes, uint32_t nCount)
{
  QString strVal;

  QString strFull;

  uint32_t nMaxDisplay = MAX_anValues;

  bool bExceedMaxDisplay;

  strFull = "0x[";
  bExceedMaxDisplay = (nCount > nMaxDisplay);

  for(uint32_t nInd = 0; nInd < nCount; nInd++)
  {
    if(nInd < nMaxDisplay)
    {
      if((nInd % 4) == 0)
      {
        if(nInd == 0)
        {
          // Don't do anything for first value!
        }
        else
        {
          // Every 16 add big spacer / break
          strFull += " ";
        }
      }

      strVal = QString("%1").arg(anBytes[nInd], 2, 16, QChar('0'));
      strFull += strVal;
    }

    if((nInd == nMaxDisplay) && (bExceedMaxDisplay))
    {
      strFull += "...";
    }

  }

  strFull += "]";
  return strFull;
}

//-----------------------------------------------------------------------------
// Print hex from array of unsigned words
//
// INPUT:
// - anWords            Array of UINT32 is passed
// - nCount                     Indicates the number of array entries originally specified
//                                      but the printing routine limits it to the maximum array depth
//                                      allocated (MAX_anValues) and add an ellipsis "..."
//
// RETURN:
// - A formatted string
//
QString CjfifDecode::PrintAsHex32(uint32_t *anWords, uint32_t nCount)
{
  QString strVal;

  QString strFull;

  strFull = "0x[";
  uint32_t nMaxDisplay = MAX_anValues / 4;      // Reduce number of words to display since each is 32b not 8b

  bool bExceedMaxDisplay;

  bExceedMaxDisplay = (nCount > nMaxDisplay);

  for(uint32_t nInd = 0; nInd < nCount; nInd++)
  {
    if(nInd < nMaxDisplay)
    {
      if(nInd == 0)
      {
        // Don't do anything for first value!
      }
      else
      {
        // Every word add a spacer
        strFull += " ";
      }

      strVal = QString("%1").arg(anWords[nInd], 8, 16, QChar('0'));      // 32-bit words
      strFull += strVal;
    }

    if((nInd == nMaxDisplay) && (bExceedMaxDisplay))
    {
      strFull += "...";
    }

  }

  strFull += "]";
  return strFull;
}

//-----------------------------------------------------------------------------
// Process all of the entries within an EXIF IFD directory
// This is used for the main EXIF IFDs as well as MakerNotes
//
// INPUT:
// - ifdStr                             The IFD section that we are processing
// - pos_exif_start
// - start_ifd_ptr
//
// PRE:
// - m_strImgExifMake
// - m_bImgExifMakeSupported
// - m_nImgExifMakeSubtype
// - m_nImgExifMakerPtr
//
// RETURN:
// - 0                                  Decoding OK
// - 2                                  Decoding failure
//
// POST:
// - m_nPos
// - m_strImgExifMake
// - m_strImgExifModel
// - m_strImgQualExif
// - m_strImgExtras
// - m_strImgExifMake
// - m_nImgExifSubIfdPtr
// - m_nImgExifGpsIfdPtr
// - m_nImgExifInteropIfdPtr
// - m_bImgExifMakeSupported
// - m_bImgExifMakernotes
// - m_nImgExifMakerPtr
// - m_nImgExifThumbComp
// - m_nImgExifThumbOffset
// - m_nImgExifThumbLen
// - m_strSoftware
//
// NOTE:
// - IFD1 typically contains the thumbnail
//
uint32_t CjfifDecode::DecodeExifIfd(QString strIfd, uint32_t nPosExifStart, uint32_t nStartIfdPtr)
{
  // Temp variables
  bool bRet;

  QString strTmp;

  CStr2 strRetVal;

  QString strValTmp;

  double fValReal;

  QString strMaker;

  // Display output variables
  QString strFull;

  QString strValOut;

  bool bExtraDecode;

  // Primary IFD variables
  uint8_t acIfdValOffsetStr[5];

  uint32_t nIfdDirLen;

  uint32_t nIfdTagVal;

  uint32_t nIfdFormat;

  uint32_t nIfdNumComps;

  bool nIfdTagUnknown;

  uint32_t nCompsToDisplay;     // Maximum number of values to capture for display

  uint32_t anValues[MAX_anValues];      // Array of decoded values (Uint32)

  int32_t anValuesS[MAX_anValues];       // Array of decoded values (Int32)

  double afValues[MAX_anValues]; // Array of decoded values (float)

  uint32_t nIfdOffset;          // First DWORD decode, usually offset

  // Clear values array
  for(uint32_t ind = 0; ind < MAX_anValues; ind++)
  {
    anValues[ind] = 0;
    anValuesS[ind] = 0;
    afValues[ind] = 0;
  }

  // ==========================================================================
  // Process IFD directory header
  // ==========================================================================

  // Move the file pointer to the start of the IFD
  m_nPos = nPosExifStart + nStartIfdPtr;

  strTmp = QString("  EXIF %1 @ Absolute 0x%2").arg(strIfd).arg(m_nPos, 8, 16, QChar('0'));
  m_pLog->AddLine(strTmp);

  ////////////

  // NOTE: Nikon type 3 starts out with the ASCII string "Nikon\0"
  //       before the rest of the items.
  // TODO: need to process type1,type2,type3
  // see: http://www.gvsoft.homedns.org/exif/makernote-nikon.html

  strTmp = QString("strIfd=[%1] m_strImgExifMake=[%2]").arg(strIfd).arg(m_strImgExifMake);
  DbgAddLine(strTmp);

  // If this is the MakerNotes section, then we may want to skip
  // altogether. Check to see if we are configured to process this
  // section or if it is a supported manufacturer.

  if(strIfd == "MakerIFD")
  {
    // Mark the image as containing Makernotes
    m_bImgExifMakernotes = true;

    if(!m_pAppConfig->decodeMaker())
    {
      strTmp = QString("    Makernote decode option not enabled.");
      m_pLog->AddLine(strTmp);

      // If user didn't enable makernote decode, don't exit, just
      // hide output. We still want to get at some info (such as Quality setting).
      // At end, we'll need to re-enable it again.
      m_pLog->Disable();
    }

    // If this Make is not supported, we'll need to exit
    if(!m_bImgExifMakeSupported)
    {
      strTmp = QString("    Makernotes not yet supported for [%1]").arg(m_strImgExifMake);
      m_pLog->AddLine(strTmp);

      m_pLog->Enable();
      return 2;
    }

    // Determine the sub-type of the Maker field (if applicable)
    // and advance the m_nPos pointer past the custom header.
    // This call uses the class members: Buf(),m_nPos
    if(!DecodeMakerSubType())
    {
      // If the subtype decode failed, skip the processing
      m_pLog->Enable();
      return 2;
    }

  }

  // ==========================================================================
  // Process IFD directory entries
  // ==========================================================================

  QString strIfdTag;

  // =========== EXIF IFD Header (Start) ===========
  // - Defined in Exif 2.2 Standard (JEITA CP-3451) section 4.6.2
  // - Contents (2 bytes total)
  //   - Number of fields (2 bytes)

  nIfdDirLen = ReadSwap2(m_nPos);
  m_nPos += 2;
  strTmp = QString("    Dir Length = 0x%1").arg(nIfdDirLen, 4, 16, QChar('0'));
  m_pLog->AddLine(strTmp);

  // =========== EXIF IFD Header (End) ===========

  // Start of IFD processing
  // Step through each IFD entry and determine the type and
  // decode accordingly.
  for(uint32_t nIfdEntryInd = 0; nIfdEntryInd < nIfdDirLen; nIfdEntryInd++)
  {
    // By default, show single-line value summary
    // bExtraDecode is used to indicate that additional special
    // parsing output is available for this entry
    bExtraDecode = false;

    strTmp = QString("    Entry #%1:").arg(nIfdEntryInd, 2, 10, QChar('0'));
    DbgAddLine(strTmp);

    // =========== EXIF IFD Interoperability entry (Start) ===========
    // - Defined in Exif 2.2 Standard (JEITA CP-3451) section 4.6.2
    // - Contents (12 bytes total)
    //   - Tag (2 bytes)
    //   - Type (2 bytes)
    //   - Count (4 bytes)
    //   - Value Offset (4 bytes)

    // Read Tag #
    nIfdTagVal = ReadSwap2(m_nPos);
    m_nPos += 2;
    nIfdTagUnknown = false;
    strIfdTag = LookupExifTag(strIfd, nIfdTagVal, nIfdTagUnknown);
    strTmp = QString("      Tag # = 0x%1 = [%2]").arg(nIfdTagVal, 4, 16, QChar('0')).arg(strIfdTag);
    DbgAddLine(strTmp);

    // Read Format (or Type)
    nIfdFormat = ReadSwap2(m_nPos);
    m_nPos += 2;
    strTmp = QString("      Format # = 0x%1").arg(nIfdFormat, 4, 16, QChar('0'));
    DbgAddLine(strTmp);

    // Read number of Components
    nIfdNumComps = ReadSwap4(m_nPos);
    m_nPos += 4;
    strTmp = QString("      # Comps = 0x%1").arg(nIfdNumComps, 4, 16, QChar('0'));
    DbgAddLine(strTmp);

    // Check to see how many components have been listed.
    // This helps trap errors in corrupted IFD segments, otherwise
    // we will hang trying to decode millions of entries!
    // See issue & testcase #1148
    if(nIfdNumComps > 4000)
    {
      // Warn user that we have clippped the component list.
      // Note that this condition is only relevant when we are
      // processing the general array fields. Fields such as MakerNote
      // will also enter this condition so we shouldn't warn in those cases.
      //
      // TODO: Defer this warning message until after we are sure that we
      // didn't handle the large dataset elsewhere.
      // For now, only report this warning if we are not processing MakerNote
      if(strIfdTag != "MakerNote")
      {
        strTmp = QString("      Excessive # components (%1). Limiting to first 4000.").arg(nIfdNumComps);
        m_pLog->AddLineWarn(strTmp);
      }
      nIfdNumComps = 4000;
    }

    // Read Component Value / Offset
    // We first treat it as a string and then re-interpret it as an integer

    // ... first as a string (just in case length <=4)
    for(uint32_t i = 0; i < 4; i++)
    {
      acIfdValOffsetStr[i] = Buf(m_nPos + i);
    }

    acIfdValOffsetStr[4] = '\0';

    // ... now as an unsigned value
    // This assignment is general-purpose, typically used when
    // we know that the IFD Value/Offset is just an offset
    nIfdOffset = ReadSwap4(m_nPos);
    strTmp = QString("      # Val/Offset = 0x%1").arg(nIfdOffset, 8, 16, QChar('0'));
    DbgAddLine(strTmp);

    // =========== EXIF IFD Interoperability entry (End) ===========

    // ==========================================================================
    // Extract the IFD component entries
    // ==========================================================================

    // The EXIF IFD entries can appear in a wide range of
    // formats / data types. The formats that have been
    // publicly documented include:
    //   EXIF_FORMAT_quint8       =  1,
    //   EXIF_FORMAT_ASCII      =  2,
    //   EXIF_FORMAT_SHORT      =  3,
    //   EXIF_FORMAT_LONG       =  4,
    //   EXIF_FORMAT_RATIONAL   =  5,
    //   EXIF_FORMAT_Squint8      =  6,
    //   EXIF_FORMAT_UNDEFINED  =  7,
    //   EXIF_FORMAT_SSHORT     =  8,
    //   EXIF_FORMAT_SLONG      =  9,
    //   EXIF_FORMAT_SRATIONAL  = 10,
    //   EXIF_FORMAT_FLOAT      = 11,
    //   EXIF_FORMAT_DOUBLE     = 12

    // The IFD variable formatter logic operates in two stages:
    // In the first stage, the format type is decoded, which results
    // in a generic decode for the IFD entry. Then, we start a second
    // stage which re-interprets the values for a number of known
    // special types.

    switch (nIfdFormat)
    {
        // ----------------------------------------
        // --- IFD Entry Type: Unsigned Byte
        // ----------------------------------------
      case 1:
        strFull = "        Unsigned Byte=[";
        strValOut = "";

        // Limit display output
        nCompsToDisplay = qMin(uint32_t (MAX_anValues), nIfdNumComps);

        // If only a single value, use decimal, else use hex
        if(nIfdNumComps == 1)
        {
          anValues[0] = Buf(m_nPos + 0);
          strTmp = QString("%1").arg(anValues[0]);
          strValOut += strTmp;
        }
        else
        {
          for(uint32_t nInd = 0; nInd < nCompsToDisplay; nInd++)
          {
            if(nIfdNumComps <= 4)
            {
              // Components fit inside 4B inline region
              anValues[nInd] = Buf(m_nPos + nInd);
            }
            else
            {
              // Since the components don't fit inside 4B inline region
              // we need to dereference
              anValues[nInd] = Buf(nPosExifStart + nIfdOffset + nInd);
            }
          }

          strValOut = PrintAsHex8(anValues, nIfdNumComps);
        }

        strFull += strValOut;
        strFull += "]";
        DbgAddLine(strFull);
        break;

        // ----------------------------------------
        // --- IFD Entry Type: ASCII string
        // ----------------------------------------
      case 2:
        strFull = "        String=";
        strValOut = "";
        char cVal;

        quint8 nVal;

        // Limit display output
        // TODO: Decide what an appropriate string limit would be
        nCompsToDisplay = qMin(uint(250), nIfdNumComps);

        for(uint32_t nInd = 0; nInd < nCompsToDisplay; nInd++)
        {
          if(nIfdNumComps <= 4)
          {
            nVal = acIfdValOffsetStr[nInd];
          }
          else
          {
            // TODO: See if this can be migrated to the "custom decode"
            // section later in the code. Decoding makernotes here is
            // less desirable but unfortunately some Nikon makernotes use
            // a non-standard offset value.
            if((strIfd == "MakerIFD") && (m_strImgExifMake == "NIKON") && (m_nImgExifMakeSubtype == 3))
            {
              // It seems that pointers in the Nikon Makernotes are
              // done relative to the start of Maker IFD
              // But why 10? Is this 10 = 18-8?
              nVal = Buf(nPosExifStart + m_nImgExifMakerPtr + nIfdOffset + 10 + nInd);
            }
            else if((strIfd == "MakerIFD") && (m_strImgExifMake == "NIKON"))
            {
              // It seems that pointers in the Nikon Makernotes are
              // done relative to the start of Maker IFD
              nVal = Buf(nPosExifStart + nIfdOffset + 0 + nInd);
            }
            else
            {
              // Canon Makernotes seem to be relative to the start
              // of the EXIF IFD
              nVal = Buf(nPosExifStart + nIfdOffset + nInd);
            }
          }

          // Just in case the string has been null-terminated early
          // or we have garbage, replace with '.'
          // TODO: Clean this up
          if(nVal != 0)
          {
            cVal = static_cast<char>(nVal);

            if(!isprint(nVal))
            {
              cVal = '.';
            }

            strValOut += cVal;
          }
        }

        strFull += strValOut;
        DbgAddLine(strFull);

        // TODO: Ideally, we would use a different string for display
        // purposes that is wrapped in quotes. Currently "strValOut" is used
        // in other sections of code (eg. in assignment to EXIF Make/Model/Software, etc.)
        // so we don't want to affect that.

        break;

        // ----------------------------------------
        // --- IFD Entry Type: Unsigned Short (2 bytes)
        // ----------------------------------------
      case 3:
        // Limit display output
        nCompsToDisplay = qMin(MAX_anValues, nIfdNumComps);

        // Unsigned Short (2 bytes)
        if(nIfdNumComps == 1)
        {
          strFull = "        Unsigned Short=[";
          // TODO: Confirm endianness is correct here
          // Refer to Exif2-2 spec, page 14.
          // Currently selecting 2 byte conversion from [1:0] out of [3:0]
          anValues[0] = ReadSwap2(m_nPos);
          strValOut = QString("%1").arg(anValues[0]);
          strFull += strValOut;
          strFull += "]";
          DbgAddLine(strFull);
        }
        else if(nIfdNumComps == 2)
        {
          strFull = "        Unsigned Short=[";
          // 2 unsigned shorts in 1 word
          anValues[0] = ReadSwap2(m_nPos + 0);
          anValues[1] = ReadSwap2(m_nPos + 2);
          strValOut = QString("%1, %1").arg(anValues[0]).arg(anValues[1]);
          strFull += strValOut;
          strFull += "]";
          DbgAddLine(strFull);
        }
        else if(nIfdNumComps > MAX_IFD_COMPS)
        {
          strValTmp = QString("    Unsigned Short=[Too many entries (%1) to display]").arg(nIfdNumComps);
          DbgAddLine(strValTmp);
          strValOut = QString("[Too many entries (%1) to display]").arg(nIfdNumComps);
        }
        else
        {
          // Try to handle multiple entries... note that this
          // is used by the Maker notes IFD decode

          strValOut = "";
          strFull = "        Unsigned Short=[";

          for(uint32_t nInd = 0; nInd < nCompsToDisplay; nInd++)
          {
            if(nInd != 0)
            {
              strValOut += ", ";
            }

            anValues[nInd] = ReadSwap2(nPosExifStart + nIfdOffset + (2 * nInd));
            strValTmp = QString("%1").arg(anValues[nInd]);
            strValOut += strValTmp;
          }

          strFull += strValOut;
          strFull += "]";
          DbgAddLine(strFull);
        }

        break;

        // ----------------------------------------
        // --- IFD Entry Type: Unsigned Long (4 bytes)
        // ----------------------------------------
      case 4:
        strFull = "        Unsigned Long=[";
        strValOut = "";

        // Limit display output
        nCompsToDisplay = qMin(MAX_anValues, nIfdNumComps);

        for(uint32_t nInd = 0; nInd < nCompsToDisplay; nInd++)
        {
          if(nIfdNumComps == 1)
          {
            // Components fit inside 4B inline region
            anValues[nInd] = ReadSwap4(m_nPos + (nInd * 4));
          }
          else
          {
            // Since the components don't fit inside 4B inline region
            // we need to dereference
            anValues[nInd] = ReadSwap4(nPosExifStart + nIfdOffset + (nInd * 4));
          }
        }

        strValOut = PrintAsHex32(anValues, nIfdNumComps);

        // If we only have a single component, then display both the hex and decimal
        if(nCompsToDisplay == 1)
        {
          strTmp = QString("%1 / %2").arg(strValOut).arg(anValues[0]);
          strValOut = strTmp;
        }

        break;

        // ----------------------------------------
        // --- IFD Entry Type: Unsigned Rational (8 bytes)
        // ----------------------------------------
      case 5:
        // Unsigned Rational
        strFull = "        Unsigned Rational=[";
        strValOut = "";

        // Limit display output
        nCompsToDisplay = qMin(MAX_anValues, nIfdNumComps);

        for(uint32_t nInd = 0; nInd < nCompsToDisplay; nInd++)
        {
          if(nInd != 0)
          {
            strValOut += ", ";
          }

          strValTmp = DecodeValFraction(nPosExifStart + nIfdOffset + (nInd * 8));
          bRet = DecodeValRational(nPosExifStart + nIfdOffset + (nInd * 8), fValReal);
          afValues[nInd] = fValReal;
          strValOut += strValTmp;
        }

        strFull += strValOut;
        strFull += "]";
        DbgAddLine(strFull);
        break;

        // ----------------------------------------
        // --- IFD Entry Type: Undefined (?)
        // ----------------------------------------
      case 7:
        // Undefined -- assume 1 word long
        // This is supposed to be a series of 8-bit bytes
        // It is usually used for 32-bit pointers (in case of offsets), but could
        // also represent ExifVersion, etc.
        strFull = "        Undefined=[";
        strValOut = "";

        // Limit display output
        nCompsToDisplay = qMin(MAX_anValues, nIfdNumComps);

        if(nIfdNumComps <= 4)
        {
          // This format is not defined, so output as hex for now
          for(uint32_t nInd = 0; nInd < nCompsToDisplay; nInd++)
          {
            anValues[nInd] = Buf(m_nPos + nInd);
          }

          strValOut = PrintAsHex8(anValues, nIfdNumComps);
          strFull += strValOut;
        }
        else
        {

          // Dereference pointer
          for(uint32_t nInd = 0; nInd < nCompsToDisplay; nInd++)
          {
            anValues[nInd] = Buf(nPosExifStart + nIfdOffset + nInd);
          }

          strValOut = PrintAsHex8(anValues, nIfdNumComps);
          strFull += strValOut;
        }

        strFull += "]";
        DbgAddLine(strFull);

        break;

        // ----------------------------------------
        // --- IFD Entry Type: Signed Short (2 bytes)
        // ----------------------------------------
      case 8:
        // Limit display output
        nCompsToDisplay = qMin(MAX_anValues, nIfdNumComps);

        // Signed Short (2 bytes)
        if(nIfdNumComps == 1)
        {
          strFull = "        Signed Short=[";
          // TODO: Confirm endianness is correct here
          // Refer to Exif2-2 spec, page 14.
          // Currently selecting 2 byte conversion from [1:0] out of [3:0]

          // TODO: Ensure that ReadSwap2 handles signed notation properly
          anValuesS[0] = ReadSwap2(m_nPos);
          strValOut = QString("%1").arg(anValuesS[0]);
          strFull += strValOut;
          strFull += "]";
          DbgAddLine(strFull);
        }
        else if(nIfdNumComps == 2)
        {
          strFull = "        Signed Short=[";
          // 2 signed shorts in 1 word

          // TODO: Ensure that ReadSwap2 handles signed notation properly
          anValuesS[0] = ReadSwap2(m_nPos + 0);
          anValuesS[1] = ReadSwap2(m_nPos + 2);
          strValOut = QString("%1, %1").arg(anValuesS[0]).arg(anValuesS[0]);
          strFull += strValOut;
          strFull += "]";
          DbgAddLine(strFull);
        }
        else if(nIfdNumComps > MAX_IFD_COMPS)
        {
          // Only print it out if it has less than MAX_IFD_COMPS entries
          strValTmp = QString("    Signed Short=[Too many entries (%1) to display]").arg(nIfdNumComps);
          DbgAddLine(strValTmp);
          strValOut = QString("[Too many entries (%1) to display]").arg(nIfdNumComps);
        }
        else
        {
          // Try to handle multiple entries... note that this
          // is used by the Maker notes IFD decode

          // Note that we don't call LookupMakerCanonTag() here
          // as that is only needed for the "unsigned short", not
          // "signed short".
          strValOut = "";
          strFull = "        Signed Short=[";

          for(uint32_t nInd = 0; nInd < nCompsToDisplay; nInd++)
          {
            if(nInd != 0)
            {
              strValOut += ", ";
            }

            anValuesS[nInd] = ReadSwap2(nPosExifStart + nIfdOffset + (2 * nInd));
            strValTmp = QString("%1").arg(anValuesS[nInd]);
            strValOut += strValTmp;
          }

          strFull += strValOut;
          strFull += "]";
          DbgAddLine(strFull);
        }

        break;

        // ----------------------------------------
        // --- IFD Entry Type: Signed Rational (8 bytes)
        // ----------------------------------------
      case 10:
        // Signed Rational
        strFull = "        Signed Rational=[";
        strValOut = "";

        // Limit display output
        nCompsToDisplay = qMin(MAX_anValues, nIfdNumComps);

        for(uint32_t nInd = 0; nInd < nCompsToDisplay; nInd++)
        {
          if(nInd != 0)
          {
            strValOut += ", ";
          }

          strValTmp = DecodeValFraction(nPosExifStart + nIfdOffset + (nInd * 8));
          bRet = DecodeValRational(nPosExifStart + nIfdOffset + (nInd * 8), fValReal);
          afValues[nInd] = fValReal;
          strValOut += strValTmp;
        }

        strFull += strValOut;
        strFull += "]";
        DbgAddLine(strFull);
        break;

      default:
        strTmp = QString("ERROR: Unsupported format [%1]").arg(nIfdFormat);
        anValues[0] = ReadSwap4(m_nPos);
        strValOut = QString("0x%1???").arg(anValues[0], 4, 16, QChar('0'));
        m_pLog->Enable();
        return 2;
        break;
    }                           // switch nIfdTagVal

    // ==========================================================================
    // Custom Value String decodes
    // ==========================================================================

    // At this point we might re-format the values, thereby
    // overriding the default strValOut. We have access to the
    //   anValues[]  (array of unsigned int)
    //   anValuesS[] (array of signed int)
    //   afValues[]  (array of float)

    // Re-format special output items
    //   This will override "strValOut" that may have previously been defined

    if((strIfdTag == "GPSLatitude") || (strIfdTag == "GPSLongitude"))
    {
      bRet = PrintValGPS(nIfdNumComps, afValues[0], afValues[1], afValues[2], strValOut);
    }
    else if(strIfdTag == "GPSVersionID")
    {
      strValOut = QString("%1.%2.%3.%4").arg(anValues[0]).arg(anValues[1]).arg(anValues[2]).arg(anValues[3]);
    }
    else if(strIfdTag == "GPSAltitudeRef")
    {
      switch (anValues[0])
      {
        case 0:
          strValOut = "Above Sea Level";
          break;
        case 1:
          strValOut = "Below Sea Level";
          break;
      }
    }
    else if(strIfdTag == "GPSStatus")
    {
      switch (acIfdValOffsetStr[0])
      {
        case 'A':
          strValOut = "Measurement in progress";
          break;
        case 'V':
          strValOut = "Measurement Interoperability";
          break;
      }
    }
    else if(strIfdTag == "GPSMeasureMode")
    {
      switch (acIfdValOffsetStr[0])
      {
        case '2':
          strValOut = "2-dimensional";
          break;
        case '3':
          strValOut = "3-dimensional";
          break;
      }
    }
    else if((strIfdTag == "GPSSpeedRef") || (strIfdTag == "GPSDestDistanceRef"))
    {
      switch (acIfdValOffsetStr[0])
      {
        case 'K':
          strValOut = "km/h";
          break;
        case 'M':
          strValOut = "mph";
          break;
        case 'N':
          strValOut = "knots";
          break;
      }
    }
    else if((strIfdTag == "GPSTrackRef") || (strIfdTag == "GPSImgDirectionRef") || (strIfdTag == "GPSDestBearingRef"))
    {
      switch (acIfdValOffsetStr[0])
      {
        case 'T':
          strValOut = "True direction";
          break;
        case 'M':
          strValOut = "Magnetic direction";
          break;
      }
    }
    else if(strIfdTag == "GPSDifferential")
    {
      switch (anValues[0])
      {
        case 0:
          strValOut = "Measurement without differential correction";
          break;
        case 1:
          strValOut = "Differential correction applied";
          break;
      }
    }
    else if(strIfdTag == "GPSAltitude")
    {
      strValOut = QString("%.3f m").arg(afValues[0]);
    }
    else if(strIfdTag == "GPSSpeed")
    {
      strValOut = QString("%.3f").arg(afValues[0]);
    }
    else if(strIfdTag == "GPSTimeStamp")
    {
      strValOut = QString("%.0f:%.0f:%.2f").arg(afValues[0]).arg(afValues[1]).arg(afValues[2]);
    }
    else if(strIfdTag == "GPSTrack")
    {
      strValOut = QString("%1").arg(afValues[0], 0, 'f', 2);
    }
    else if(strIfdTag == "GPSDOP")
    {
      strValOut = QString("%1").arg(afValues[0], 0, 'f', 4);
    }

    if(strIfdTag == "Compression")
    {
      switch (anValues[0])
      {
        case 1:
          strValOut = "None";
          break;
        case 6:
          strValOut = "JPEG";
          break;
      }
    }
    else if(strIfdTag == "ExposureTime")
    {
      // Assume only one
      strValTmp = strValOut;
      strValOut = QString("%1 s").arg(strValTmp);
    }
    else if(strIfdTag == "FNumber")
    {
      // Assume only one
      strValOut = QString("F%1").arg(afValues[0], 0, 'f', 1);
    }
    else if(strIfdTag == "FocalLength")
    {
      // Assume only one
      strValOut = QString("%1 mm").arg(afValues[0], 0, 'f', 0);
    }
    else if(strIfdTag == "ExposureBiasValue")
    {
      // Assume only one
      // TODO: Need to test negative numbers
      strValOut = QString("%1 eV").arg(afValues[0], 0, 'f', 2);
    }
    else if(strIfdTag == "ExifVersion")
    {
      // Assume only one
      strValOut = QString("%1%2.%3%4")
          .arg(static_cast<char>(anValues[0]))
          .arg(static_cast<char>(anValues[1]))
          .arg(static_cast<char>(anValues[2]))
          .arg(static_cast<char>(anValues[3]));
    }
    else if(strIfdTag == "FlashPixVersion")
    {
      // Assume only one
      strValOut = QString("%1%2.%3%4")
      .arg(static_cast<char>(anValues[0]))
      .arg(static_cast<char>(anValues[1]))
      .arg(static_cast<char>(anValues[2]))
      .arg(static_cast<char>(anValues[3]));
    }
    else if(strIfdTag == "PhotometricInterpretation")
    {
      switch (anValues[0])
      {
        case 1:
          strValOut = "Monochrome";
          break;
        case 2:
          strValOut = "RGB";
          break;
        case 6:
          strValOut = "YCbCr";
          break;
      }
    }
    else if(strIfdTag == "Orientation")
    {
      switch (anValues[0])
      {
        case 1:
          strValOut = "1 = Row 0: top, Col 0: left";
          break;
        case 2:
          strValOut = "2 = Row 0: top, Col 0: right";
          break;
        case 3:
          strValOut = "3 = Row 0: bottom, Col 0: right";
          break;
        case 4:
          strValOut = "4 = Row 0: bottom, Col 0: left";
          break;
        case 5:
          strValOut = "5 = Row 0: left, Col 0: top";
          break;
        case 6:
          strValOut = "6 = Row 0: right, Col 0: top";
          break;
        case 7:
          strValOut = "7 = Row 0: right, Col 0: bottom";
          break;
        case 8:
          strValOut = "8 = Row 0: left, Col 0: bottom";
          break;
      }
    }
    else if(strIfdTag == "PlanarConfiguration")
    {
      switch (anValues[0])
      {
        case 1:
          strValOut = "Chunky format";
          break;
        case 2:
          strValOut = "Planar format";
          break;
      }
    }
    else if(strIfdTag == "YCbCrSubSampling")
    {
      switch (anValues[0] * 65536 + anValues[1])
      {
        case 0x00020001:
          strValOut = "4:2:2";
          break;
        case 0x00020002:
          strValOut = "4:2:0";
          break;
      }
    }
    else if(strIfdTag == "YCbCrPositioning")
    {
      switch (anValues[0])
      {
        case 1:
          strValOut = "Centered";
          break;
        case 2:
          strValOut = "Co-sited";
          break;
      }
    }
    else if(strIfdTag == "ResolutionUnit")
    {
      switch (anValues[0])
      {
        case 1:
          strValOut = "None";
          break;
        case 2:
          strValOut = "Inch";
          break;
        case 3:
          strValOut = "Centimeter";
          break;
      }
    }
    else if(strIfdTag == "FocalPlaneResolutionUnit")
    {
      switch (anValues[0])
      {
        case 1:
          strValOut = "None";
          break;
        case 2:
          strValOut = "Inch";
          break;
        case 3:
          strValOut = "Centimeter";
          break;
      }
    }
    else if(strIfdTag == "ColorSpace")
    {
      switch (anValues[0])
      {
        case 1:
          strValOut = "sRGB";
          break;
        case 0xFFFF:
          strValOut = "Uncalibrated";
          break;
      }
    }
    else if(strIfdTag == "ComponentsConfiguration")
    {
      // Undefined type, assume 4 bytes
      strValOut = "[";
      for(uint32_t vind = 0; vind < 4; vind++)
      {
        if(vind != 0)
        {
          strValOut += " ";
        }
        switch (anValues[vind])
        {
          case 0:
            strValOut += ".";
            break;
          case 1:
            strValOut += "Y";
            break;
          case 2:
            strValOut += "Cb";
            break;
          case 3:
            strValOut += "Cr";
            break;
          case 4:
            strValOut += "R";
            break;
          case 5:
            strValOut += "G";
            break;
          case 6:
            strValOut += "B";
            break;
          default:
            strValOut += "?";
            break;
        }
      }

      strValOut += "]";

    }
    else if((strIfdTag == "XPTitle") ||
            (strIfdTag == "XPComment") || (strIfdTag == "XPAuthor") || (strIfdTag == "XPKeywords") || (strIfdTag == "XPSubject"))
    {
      strValOut = "\"";
      QString strVal;

      strVal = m_pWBuf->BufReadUniStr2(nPosExifStart + nIfdOffset, nIfdNumComps);
      strValOut += strVal;
      strValOut += "\"";
    }
    else if(strIfdTag == "UserComment")
    {
      // Character code
      uint32_t anCharCode[8];

      for(uint32_t vInd = 0; vInd < 8; vInd++)
      {
        anCharCode[vInd] = Buf(nPosExifStart + nIfdOffset + 0 + vInd);
      }

      // Actual string
      strValOut = "\"";
      bool bDone = false;

      uint8_t cTmp;

      for(uint32_t vInd = 0; (vInd < nIfdNumComps - 8) && (!bDone); vInd++)
      {
        cTmp = Buf(nPosExifStart + nIfdOffset + 8 + vInd);

        if(cTmp == 0)
        {
          bDone = true;
        }
        else
        {
          strValOut += cTmp;
        }
      }

      strValOut += "\"";
    }
    else if(strIfdTag == "MeteringMode")
    {
      switch (anValues[0])
      {
        case 0:
          strValOut = "Unknown";
          break;
        case 1:
          strValOut = "Average";
          break;
        case 2:
          strValOut = "CenterWeightedAverage";
          break;
        case 3:
          strValOut = "Spot";
          break;
        case 4:
          strValOut = "MultiSpot";
          break;
        case 5:
          strValOut = "Pattern";
          break;
        case 6:
          strValOut = "Partial";
          break;
        case 255:
          strValOut = "Other";
          break;
      }
    }
    else if(strIfdTag == "ExposureProgram")
    {
      switch (anValues[0])
      {
        case 0:
          strValOut = "Not defined";
          break;
        case 1:
          strValOut = "Manual";
          break;
        case 2:
          strValOut = "Normal program";
          break;
        case 3:
          strValOut = "Aperture priority";
          break;
        case 4:
          strValOut = "Shutter priority";
          break;
        case 5:
          strValOut = "Creative program (depth of field)";
          break;
        case 6:
          strValOut = "Action program (fast shutter speed)";
          break;
        case 7:
          strValOut = "Portrait mode";
          break;
        case 8:
          strValOut = "Landscape mode";
          break;
      }
    }
    else if(strIfdTag == "Flash")
    {
      switch (anValues[0] & 1)
      {
        case 0:
          strValOut = "Flash did not fire";
          break;
        case 1:
          strValOut = "Flash fired";
          break;
      }
      // TODO: Add other bitfields?
    }
    else if(strIfdTag == "SensingMethod")
    {
      switch (anValues[0])
      {
        case 1:
          strValOut = "Not defined";
          break;
        case 2:
          strValOut = "One-chip color area sensor";
          break;
        case 3:
          strValOut = "Two-chip color area sensor";
          break;
        case 4:
          strValOut = "Three-chip color area sensor";
          break;
        case 5:
          strValOut = "Color sequential area sensor";
          break;
        case 7:
          strValOut = "Trilinear sensor";
          break;
        case 8:
          strValOut = "Color sequential linear sensor";
          break;
      }
    }
    else if(strIfdTag == "FileSource")
    {
      switch (anValues[0])
      {
        case 3:
          strValOut = "DSC";
          break;
      }
    }
    else if(strIfdTag == "CustomRendered")
    {
      switch (anValues[0])
      {
        case 0:
          strValOut = "Normal process";
          break;
        case 1:
          strValOut = "Custom process";
          break;
      }
    }
    else if(strIfdTag == "ExposureMode")
    {
      switch (anValues[0])
      {
        case 0:
          strValOut = "Auto exposure";
          break;
        case 1:
          strValOut = "Manual exposure";
          break;
        case 2:
          strValOut = "Auto bracket";
          break;
      }
    }
    else if(strIfdTag == "WhiteBalance")
    {
      switch (anValues[0])
      {
        case 0:
          strValOut = "Auto white balance";
          break;
        case 1:
          strValOut = "Manual white balance";
          break;
      }
    }
    else if(strIfdTag == "SceneCaptureType")
    {
      switch (anValues[0])
      {
        case 0:
          strValOut = "Standard";
          break;
        case 1:
          strValOut = "Landscape";
          break;
        case 2:
          strValOut = "Portrait";
          break;
        case 3:
          strValOut = "Night scene";
          break;
      }
    }
    else if(strIfdTag == "SceneType")
    {
      switch (anValues[0])
      {
        case 1:
          strValOut = "A directly photographed image";
          break;
      }
    }
    else if(strIfdTag == "LightSource")
    {
      switch (anValues[0])
      {
        case 0:
          strValOut = "unknown";
          break;
        case 1:
          strValOut = "Daylight";
          break;
        case 2:
          strValOut = "Fluorescent";
          break;
        case 3:
          strValOut = "Tungsten (incandescent light)";
          break;
        case 4:
          strValOut = "Flash";
          break;
        case 9:
          strValOut = "Fine weather";
          break;
        case 10:
          strValOut = "Cloudy weather";
          break;
        case 11:
          strValOut = "Shade";
          break;
        case 12:
          strValOut = "Daylight fluorescent (D 5700 � 7100K)";
          break;
        case 13:
          strValOut = "Day white fluorescent (N 4600 � 5400K)";
          break;
        case 14:
          strValOut = "Cool white fluorescent (W 3900 � 4500K)";
          break;
        case 15:
          strValOut = "White fluorescent (WW 3200 � 3700K)";
          break;
        case 17:
          strValOut = "Standard light A";
          break;
        case 18:
          strValOut = "Standard light B";
          break;
        case 19:
          strValOut = "Standard light C";
          break;
        case 20:
          strValOut = "D55";
          break;
        case 21:
          strValOut = "D65";
          break;
        case 22:
          strValOut = "D75";
          break;
        case 23:
          strValOut = "D50";
          break;
        case 24:
          strValOut = "ISO studio tungsten";
          break;
        case 255:
          strValOut = "other light source";
          break;
      }
    }
    else if(strIfdTag == "SubjectArea")
    {
      switch (nIfdNumComps)
      {
        case 2:
          // coords
          strValOut = QString("Coords: Center=[%1,%2]").arg(anValues[0]).arg(anValues[1]);
          break;
        case 3:
          // circle
          strValOut = QString("Coords (Circle): Center=[%1,%2] Diameter=%3").arg(anValues[0]).arg(anValues[1]).arg(anValues[2]);
          break;
        case 4:
          // rectangle
          strValOut =
            QString("Coords (Rect): Center=[%1,%2] Width=%3 Height=%4").arg(anValues[0]).arg(anValues[1]).arg(anValues[2]).
            arg(anValues[3]);
          break;
        default:
          // Leave default decode, unexpected value
          break;
      }
    }

    if(strIfdTag == "CFAPattern")
    {
      uint32_t nHorzRepeat, nVertRepeat;
      uint32_t anCfaVal[16][16];
      uint32_t nInd = 0;
      uint32_t nVal;

      QString strLine, strCol;

      nHorzRepeat = anValues[nInd + 0] * 256 + anValues[nInd + 1];
      nVertRepeat = anValues[nInd + 2] * 256 + anValues[nInd + 3];
      nInd += 4;

      if((nHorzRepeat < 16) && (nVertRepeat < 16))
      {
        bExtraDecode = true;
        strTmp = QString("    [%1] =").arg(strIfdTag, -36);
        m_pLog->AddLine(strTmp);

        for(uint32_t nY = 0; nY < nVertRepeat; nY++)
        {
          strLine = QString("     %1  = [ ").arg("", -36);

          for(uint32_t nX = 0; nX < nHorzRepeat; nX++)
          {
            if(nInd < MAX_anValues)
            {
              nVal = anValues[nInd++];
              anCfaVal[nY][nX] = nVal;

              switch (nVal)
              {
                case 0:
                  strCol = "Red";
                  break;
                case 1:
                  strCol = "Grn";
                  break;
                case 2:
                  strCol = "Blu";
                  break;
                case 3:
                  strCol = "Cya";
                  break;
                case 4:
                  strCol = "Mgn";
                  break;
                case 5:
                  strCol = "Yel";
                  break;
                case 6:
                  strCol = "Wht";
                  break;
                default:
                  strCol = QString("x%1").arg(nVal, 2, 16, QChar('0'));
                  break;
              }

              strLine.append(QString("%1 ").arg(strCol));
            }
          }

          strLine.append("]");
          m_pLog->AddLine(strLine);
        }
      }
    }

    if((strIfd == "InteropIFD") && (strIfdTag == "InteroperabilityVersion"))
    {
      // Assume only one
      strValOut = QString("%1%2.%3%4")
      .arg(static_cast<char>(anValues[0]))
      .arg(static_cast<char>(anValues[1]))
      .arg(static_cast<char>(anValues[2]))
      .arg(static_cast<char>(anValues[3]));
    }

    // ==========================================================================

    // ----------------------------------------
    // Handle certain MakerNotes
    //   For Canon, we have a special parser routine to handle these
    // ----------------------------------------
    if(strIfd == "MakerIFD")
    {

      if((m_strImgExifMake == "Canon") && (nIfdFormat == 3) && (nIfdNumComps > 4))
      {
        // Print summary line now, before sub details
        // Disable later summary line
        bExtraDecode = true;

        if((!m_pAppConfig->hideUnknownExif()) || (!nIfdTagUnknown))
        {
          strTmp = QString("    [%1]").arg(strIfdTag, -36);
          m_pLog->AddLine(strTmp);

          // Assume it is a maker field with subentries!

          for(uint32_t ind = 0; ind < nIfdNumComps; ind++)
          {
            // Limit the number of entries (in case there was a decode error
            // or simply too many to report)
            if(ind < MAX_anValues)
            {
              strValOut = QString("#%1=%2 ").arg(ind).arg(anValues[ind]);
              strRetVal = LookupMakerCanonTag(nIfdTagVal, ind, anValues[ind]);
              strMaker = strRetVal.strTag;
              strValTmp = QString("      [%1] = %2").arg(strMaker, -34).arg(strRetVal.strVal);

              if((!m_pAppConfig->hideUnknownExif()) || (!strRetVal.bUnknown))
              {
                m_pLog->AddLine(strValTmp);
              }
            }
            else if(ind == MAX_anValues)
            {
              m_pLog->AddLine("      [... etc ...]");
            }
            else
            {
              // Don't print!
            }
          }
        }

        strValOut = "...";
      }

      // For Nikon & Sigma, we simply support the quality field
      if((strIfdTag == "Nikon1.Quality") ||
         (strIfdTag == "Nikon2.Quality") || (strIfdTag == "Nikon3.Quality") || (strIfdTag == "Sigma.Quality"))
      {
        m_strImgQualExif = strValOut;

        // Collect extra details (for later DB submission)
        strTmp = "";
        strTmp = QString("[%1]:[%1],").arg(strIfdTag).arg(strValOut);
        m_strImgExtras += strTmp;
      }

      // Collect extra details (for later DB submission)
      if(strIfdTag == "Canon.ImageType")
      {
        strTmp = "";
        strTmp = QString("[%1]:[%1],").arg(strIfdTag).arg(strValOut);
        m_strImgExtras += strTmp;
      }
    }

    // ----------------------------------------

    // Now extract some of the important offsets / pointers
    if((strIfd == "IFD0") && (strIfdTag == "ExifOffset"))
    {
      // EXIF SubIFD - Pointer
      m_nImgExifSubIfdPtr = nIfdOffset;
      strValOut = QString("@ 0x%1").arg(nIfdOffset, 4, 16, QChar('0'));
    }

    if((strIfd == "IFD0") && (strIfdTag == "GPSOffset"))
    {
      // GPS SubIFD - Pointer
      m_nImgExifGpsIfdPtr = nIfdOffset;
      strValOut = QString("@ 0x%1").arg(nIfdOffset, 4, 16, QChar('0'));
    }

    // TODO: Add Interoperability IFD (0xA005)?
    if((strIfd == "SubIFD") && (strIfdTag == "ExifInteroperabilityOffset"))
    {
      m_nImgExifInteropIfdPtr = nIfdOffset;
      strValOut = QString("@ 0x%1").arg(nIfdOffset, 4, 16, QChar('0'));
    }

    // Extract software field
    if((strIfd == "IFD0") && (strIfdTag == "Software"))
    {
      m_strSoftware = strValOut;
    }

    // -------------------------
    // IFD0 - ExifMake
    // -------------------------
    if((strIfd == "IFD0") && (strIfdTag == "Make"))
    {
      m_strImgExifMake = strValOut.trimmed();

    }

    // -------------------------
    // IFD0 - ExifModel
    // -------------------------
    if((strIfd == "IFD0") && (strIfdTag == "Model"))
    {
      m_strImgExifModel = strValOut.trimmed();
    }

    if((strIfd == "SubIFD") && (strIfdTag == "MakerNote"))
    {
      // Maker IFD - Pointer
      m_nImgExifMakerPtr = nIfdOffset;
      strValOut = QString("@ 0x%1").arg(nIfdOffset, 4, 16, QChar('0'));
    }

    // -------------------------
    // IFD1 - Embedded Thumbnail
    // -------------------------
    if((strIfd == "IFD1") && (strIfdTag == "Compression"))
    {
      // Embedded thumbnail, compression format
      m_nImgExifThumbComp = ReadSwap4(m_nPos);
    }

    if((strIfd == "IFD1") && (strIfdTag == "JpegIFOffset"))
    {
      // Embedded thumbnail, offset
      m_nImgExifThumbOffset = nIfdOffset + nPosExifStart;
      strValOut = QString("@ +0x%1 = @ 0x%2")
          .arg(nIfdOffset, 4, 16, QChar('0'))
          .arg(m_nImgExifThumbOffset, 4, 16, QChar('0'));
    }

    if((strIfd == "IFD1") && (strIfdTag == "JpegIFByteCount"))
    {
      // Embedded thumbnail, length
      m_nImgExifThumbLen = ReadSwap4(m_nPos);
    }

    // ==========================================================================
    // Determine MakerNote support
    // ==========================================================================

    if(m_strImgExifMake != "")
    {
      // 1) Identify the supported MakerNotes
      // 2) Remap variations of the Maker field (e.g. Nikon)
      //    as some manufacturers have been inconsistent in their
      //    use of the Make field

      m_bImgExifMakeSupported = false;

      if(m_strImgExifMake == "Canon")
      {
        m_bImgExifMakeSupported = true;
      }
      else if(m_strImgExifMake == "PENTAX Corporation")
      {
        m_strImgExifMake = "PENTAX";
      }
      else if(m_strImgExifMake == "NIKON CORPORATION")
      {
        m_strImgExifMake = "NIKON";
        m_bImgExifMakeSupported = true;
      }
      else if(m_strImgExifMake == "NIKON")
      {
        m_strImgExifMake = "NIKON";
        m_bImgExifMakeSupported = true;
      }
      else if(m_strImgExifMake == "SIGMA")
      {
        m_bImgExifMakeSupported = true;
      }
      else if(m_strImgExifMake == "SONY")
      {
        m_bImgExifMakeSupported = true;
      }
      else if(m_strImgExifMake == "FUJIFILM")
      {
        // TODO:
        // FUJIFILM Maker notes apparently use
        // big endian format even though main section uses little.
        // Need to switch if in maker section for FUJI
        // For now, disable support
        m_bImgExifMakeSupported = false;
      }
    }

    // Now advance the m_nPos ptr as we have finished with valoffset
    m_nPos += 4;

    // ==========================================================================
    // SUMMARY REPORT
    // ==========================================================================

    // If we haven't already output a detailed decode of this field
    // then we can output the generic representation here
    if(!bExtraDecode)
    {
      // Provide option to skip unknown fields
      if((!m_pAppConfig->hideUnknownExif()) || (!nIfdTagUnknown))
      {
        // If the tag is an ASCII string, we want to wrap with quote marks
        if(nIfdFormat == 2)
        {
          strTmp = QString("    [%1] = \"%2\"").arg(strIfdTag, -36).arg(strValOut);
        }
        else
        {
          strTmp = QString("    [%1] = %2").arg(strIfdTag, -36).arg(strValOut);
        }

        m_pLog->AddLine(strTmp);
      }
    }

    DbgAddLine("");
  }                             // for nIfdEntryInd

  // =========== EXIF IFD (End) ===========
  // - Defined in Exif 2.2 Standard (JEITA CP-3451) section 4.6.2
  // - Is completed by 4-byte offset to next IFD, which is
  //   read in next iteration.

  m_pLog->Enable();
  return 0;
}

//-----------------------------------------------------------------------------
// Handle APP13 marker
// This includes:
// - Photoshop "Save As" and "Save for Web" quality settings
// - IPTC entries
// PRE:
// - m_nPos
// POST:
// - m_nImgQualPhotoshopSa
uint32_t CjfifDecode::DecodeApp13Ps()
{
  // Photoshop APP13 marker definition doesn't appear to have a
  // well-defined length, so I will read until I encounter a
  // non-"8BIM" entry, then we reset the position counter
  // and move on to the next marker.
  // FIXME: This does not appear to be very robust

  QString strTmp;
  QString strBimName;

  bool bDone = false;

  //uint32_t              nVal = 0x8000;

  QString strVal;
  QString strByte;
  QString strBimSig;

  // Reset PsDec decoder state
  m_pPsDec->Reset();

  while(!bDone)
  {
    // FIXME: Need to check for actual marker section extent, not
    // just the lack of an 8BIM signature as the terminator!

    // Check the signature but don't advance the file pointer
    strBimSig = m_pWBuf->BufReadStrn(m_nPos, 4);

    // Check for signature "8BIM"
    if(strBimSig == "8BIM")
    {
      m_pPsDec->PhotoshopParseImageResourceBlock(m_nPos, 3);
    }
    else
    {
      // Not 8BIM?
      bDone = true;
    }
  }

  // Now that we've finished with the PsDec decoder we can fetch
  // some of the parser state
  // TODO: Migrate into accessor
  m_nImgQualPhotoshopSa = m_pPsDec->m_nQualitySaveAs;
  m_nImgQualPhotoshopSfw = m_pPsDec->m_nQualitySaveForWeb;
  m_bPsd = m_pPsDec->m_bPsd;

  return 0;
}


//-----------------------------------------------------------------------------
// Start decoding a single ICC header segment @ nPos
uint32_t CjfifDecode::DecodeIccHeader(uint32_t nPos)
{
  QString strTmp, strTmp1;

  // Profile header
  uint32_t nProfSz;
  uint32_t nPrefCmmType;
  uint32_t nProfVer;
  uint32_t nProfDevClass;
  uint32_t nDataColorSpace;
  uint32_t nPcs;
  uint32_t anDateTimeCreated[3];
  uint32_t nProfFileSig;
  uint32_t nPrimPlatSig;
  uint32_t nProfFlags;
  uint32_t nDevManuf;
  uint32_t nDevModel;
  uint32_t anDevAttrib[2];
  uint32_t nRenderIntent;
  uint32_t anIllumPcsXyz[3];
  uint32_t nProfCreatorSig;
  uint32_t anProfId[4];
  uint32_t anRsvd[7];

  // Read in all of the ICC header bytes
  nProfSz = ReadBe4(nPos);
  nPos += 4;
  nPrefCmmType = ReadBe4(nPos);
  nPos += 4;
  nProfVer = ReadBe4(nPos);
  nPos += 4;
  nProfDevClass = ReadBe4(nPos);
  nPos += 4;
  nDataColorSpace = ReadBe4(nPos);
  nPos += 4;
  nPcs = ReadBe4(nPos);
  nPos += 4;
  anDateTimeCreated[2] = ReadBe4(nPos);
  nPos += 4;
  anDateTimeCreated[1] = ReadBe4(nPos);
  nPos += 4;
  anDateTimeCreated[0] = ReadBe4(nPos);
  nPos += 4;
  nProfFileSig = ReadBe4(nPos);
  nPos += 4;
  nPrimPlatSig = ReadBe4(nPos);
  nPos += 4;
  nProfFlags = ReadBe4(nPos);
  nPos += 4;
  nDevManuf = ReadBe4(nPos);
  nPos += 4;
  nDevModel = ReadBe4(nPos);
  nPos += 4;
  anDevAttrib[1] = ReadBe4(nPos);
  nPos += 4;
  anDevAttrib[0] = ReadBe4(nPos);
  nPos += 4;
  nRenderIntent = ReadBe4(nPos);
  nPos += 4;
  anIllumPcsXyz[2] = ReadBe4(nPos);
  nPos += 4;
  anIllumPcsXyz[1] = ReadBe4(nPos);
  nPos += 4;
  anIllumPcsXyz[0] = ReadBe4(nPos);
  nPos += 4;
  nProfCreatorSig = ReadBe4(nPos);
  nPos += 4;
  anProfId[3] = ReadBe4(nPos);
  nPos += 4;
  anProfId[2] = ReadBe4(nPos);
  nPos += 4;
  anProfId[1] = ReadBe4(nPos);
  nPos += 4;
  anProfId[0] = ReadBe4(nPos);
  nPos += 4;
  anRsvd[6] = ReadBe4(nPos);
  nPos += 4;
  anRsvd[5] = ReadBe4(nPos);
  nPos += 4;
  anRsvd[4] = ReadBe4(nPos);
  nPos += 4;
  anRsvd[3] = ReadBe4(nPos);
  nPos += 4;
  anRsvd[2] = ReadBe4(nPos);
  nPos += 4;
  anRsvd[1] = ReadBe4(nPos);
  nPos += 4;
  anRsvd[0] = ReadBe4(nPos);
  nPos += 4;

  // Now output the formatted version of the above data structures
  strTmp = QString("        %1 : %2 bytes").arg("Profile Size", -33).arg(nProfSz);
  m_pLog->AddLine(strTmp);

  strTmp = QString("        %1 : %2").arg("Preferred CMM Type", -33).arg(Uint2Chars(nPrefCmmType));
  m_pLog->AddLine(strTmp);

  strTmp =
    QString("        %1 : %2.%3.%4.%5 (0x%6)")
      .arg("Profile Version",-33)
      .arg((nProfVer & 0xF0000000) >> 28)
      .arg((nProfVer & 0x0F000000) >> 24)
      .arg((nProfVer & 0x00F00000) >> 20)
      .arg((nProfVer & 0x000F0000) >> 16)
      .arg(nProfVer, 8, 16, QChar('0'));
  m_pLog->AddLine(strTmp);

  switch (nProfDevClass)
  {
    //CAL! case 'scnr':
    case FOURC_INT('s','c','n','r'):
      strTmp1 = "Input Device profile";
      break;
    case FOURC_INT('m','n','t','r'):
      strTmp1 = "Display Device profile";
      break;
    case FOURC_INT('p','r','t','r'):
      strTmp1 = "Output Device profile";
      break;
    case FOURC_INT('l','i','n','k'):
      strTmp1 = "DeviceLink Device profile";
      break;
    case FOURC_INT('s','p','a','c'):
      strTmp1 = "ColorSpace Conversion profile";
      break;
    case FOURC_INT('a','b','s','t'):
      strTmp1 = "Abstract profile";
      break;
    case FOURC_INT('n','m','c','l'):
      strTmp1 = "Named colour profile";
      break;
    default:
      strTmp1 = QString("? (0x%1)").arg(nProfDevClass, 8, 16, QChar('0'));
      break;
  }
  strTmp = QString("        %1 : %2 (%3)").arg("Profile Device/Class", -33).arg(strTmp1).arg(Uint2Chars(nProfDevClass));
  m_pLog->AddLine(strTmp);

  switch (nDataColorSpace)
  {
    case FOURC_INT('X','Y','Z',' '):
      strTmp1 = "XYZData";
      break;
    case FOURC_INT('L','a','b',' '):
      strTmp1 = "labData";
      break;
    case FOURC_INT('L','u','v',' '):
      strTmp1 = "lubData";
      break;
    case FOURC_INT('Y','C','b','r'):
      strTmp1 = "YCbCrData";
      break;
    case FOURC_INT('Y','x','y',' '):
      strTmp1 = "YxyData";
      break;
    case FOURC_INT('R','G','B',' '):
      strTmp1 = "rgbData";
      break;
    case FOURC_INT('G','R','A','Y'):
      strTmp1 = "grayData";
      break;
    case FOURC_INT('H','S','V',' '):
      strTmp1 = "hsvData";
      break;
    case FOURC_INT('H','L','S',' '):
      strTmp1 = "hlsData";
      break;
    case FOURC_INT('C','M','Y','K'):
      strTmp1 = "cmykData";
      break;
    case FOURC_INT('C','M','Y',' '):
      strTmp1 = "cmyData";
      break;
    case FOURC_INT('2','C','L','R'):
      strTmp1 = "2colourData";
      break;
    case FOURC_INT('3','C','L','R'):
      strTmp1 = "3colourData";
      break;
    case FOURC_INT('4','C','L','R'):
      strTmp1 = "4colourData";
      break;
    case FOURC_INT('5','C','L','R'):
      strTmp1 = "5colourData";
      break;
    case FOURC_INT('6','C','L','R'):
      strTmp1 = "6colourData";
      break;
    case FOURC_INT('7','C','L','R'):
      strTmp1 = "7colourData";
      break;
    case FOURC_INT('8','C','L','R'):
      strTmp1 = "8colourData";
      break;
    case FOURC_INT('9','C','L','R'):
      strTmp1 = "9colourData";
      break;
    case FOURC_INT('A','C','L','R'):
      strTmp1 = "10colourData";
      break;
    case FOURC_INT('B','C','L','R'):
      strTmp1 = "11colourData";
      break;
    case FOURC_INT('C','C','L','R'):
      strTmp1 = "12colourData";
      break;
    case FOURC_INT('D','C','L','R'):
      strTmp1 = "13colourData";
      break;
    case FOURC_INT('E','C','L','R'):
      strTmp1 = "14colourData";
      break;
    case FOURC_INT('F','C','L','R'):
      strTmp1 = "15colourData";
      break;
    default:
      strTmp1 = QString("? (0x%1)").arg(nDataColorSpace, 8, 16, QChar('0'));
      break;
  }
  strTmp = QString("        %1 : %2 (%3)").arg("Data Colour Space", -33).arg(strTmp1).arg(Uint2Chars(nDataColorSpace));
  m_pLog->AddLine(strTmp);

  strTmp = QString("        %1 : %2").arg("Profile connection space (PCS)", -33).arg(Uint2Chars(nPcs));
  m_pLog->AddLine(strTmp);

  strTmp = QString("        %1 : %2").arg("Profile creation date", -33).arg(DecodeIccDateTime(anDateTimeCreated));
  m_pLog->AddLine(strTmp);

  strTmp = QString("        %1 : %2").arg("Profile file signature", -33).arg(Uint2Chars(nProfFileSig));
  m_pLog->AddLine(strTmp);

  switch (nPrimPlatSig)
  {
    case FOURC_INT('A','P','P','L'):
      strTmp1 = "Apple Computer, Inc.";
      break;
    case FOURC_INT('M','S','F','T'):
      strTmp1 = "Microsoft Corporation";
      break;
    case FOURC_INT('S','G','I',' '):
      strTmp1 = "Silicon Graphics, Inc.";
      break;
    case FOURC_INT('S','U','N','W'):
      strTmp1 = "Sun Microsystems, Inc.";
      break;
    default:
      strTmp1 = QString("? (0x%1)").arg(nPrimPlatSig, 8, 16, QChar('0'));
      break;
  }

  strTmp = QString("        %1 : %2 (%3)").arg("Primary platform", -33).arg(strTmp1).arg(Uint2Chars(nPrimPlatSig));
  m_pLog->AddLine(strTmp);

  strTmp = QString("        %1 : 0x%2").arg("Profile flags", -33).arg(nProfFlags, 8, 16, QChar('0'));
  m_pLog->AddLine(strTmp);
  strTmp1 = (TestBit(nProfFlags, 0)) ? "Embedded profile" : "Profile not embedded";
  strTmp = QString("        %1 > %2").arg("Profile flags", -35).arg(strTmp1);
  m_pLog->AddLine(strTmp);
  strTmp1 =
    (TestBit(nProfFlags, 1)) ? "Profile can be used independently of embedded" :
    "Profile can't be used independently of embedded";
  strTmp = QString("        %1 > %2").arg("Profile flags", -35).arg(strTmp1);
  m_pLog->AddLine(strTmp);

  strTmp = QString("        %1 : %2").arg("Device Manufacturer", -33).arg(Uint2Chars(nDevManuf));
  m_pLog->AddLine(strTmp);

  strTmp = QString("        %1 : %2").arg("Device Model", -33).arg(Uint2Chars(nDevModel));
  m_pLog->AddLine(strTmp);

  strTmp = QString("        %1 : 0x%2_%3")
      .arg("Device attributes", -33)
      .arg(anDevAttrib[1], 8, 16, QChar('0'))
      .arg(anDevAttrib[0], 8, 16, QChar('0'));
  m_pLog->AddLine(strTmp);
  strTmp1 = (TestBit(anDevAttrib[0], 0)) ? "Transparency" : "Reflective";
  strTmp = QString("        %1 > %2")
      .arg("Device attributes", -35)
      .arg(strTmp1);
  m_pLog->AddLine(strTmp);
  strTmp1 = (TestBit(anDevAttrib[0], 1)) ? "Matte" : "Glossy";
  strTmp = QString("        %1 > %2")
      .arg("Device attributes", -35)
      .arg(strTmp1);
  m_pLog->AddLine(strTmp);
  strTmp1 = (TestBit(anDevAttrib[0], 2)) ? "Media polarity = positive" : "Media polarity = negative";
  strTmp = QString("        %1 > %2")
      .arg("Device attributes", -35)
      .arg(strTmp1);
  m_pLog->AddLine(strTmp);
  strTmp1 = (TestBit(anDevAttrib[0], 3)) ? "Colour media" : "Black & white media";
  strTmp = QString("        %1 > %2")
      .arg("Device attributes", -35)
      .arg(strTmp1);
  m_pLog->AddLine(strTmp);

  switch (nRenderIntent)
  {
    case 0x00000000:
      strTmp1 = "Perceptual";
      break;
    case 0x00000001:
      strTmp1 = "Media-Relative Colorimetric";
      break;
    case 0x00000002:
      strTmp1 = "Saturation";
      break;
    case 0x00000003:
      strTmp1 = "ICC-Absolute Colorimetric";
      break;
    default:
      strTmp1 = QString("0x%1").arg(nRenderIntent, 8, 16, QChar('0'));
      break;
  }

  strTmp = QString("        %1 : %2").arg("Rendering intent", -33).arg(strTmp1);
  m_pLog->AddLine(strTmp);

  // PCS illuminant

  strTmp = QString("        %1 : %2").arg("Profile creator", -33).arg(Uint2Chars(nProfCreatorSig));
  m_pLog->AddLine(strTmp);

  strTmp =
    QString("        %1 : 0x%2_%3_%4_%5")
      .arg("Profile ID", -33)
      .arg(anProfId[3], 8, 16, QChar('0'))
      .arg(anProfId[2], 8, 16, QChar('0'))
      .arg(anProfId[1], 8, 16, QChar('0'))
      .arg(anProfId[0], 8, 16, QChar('0'));
  m_pLog->AddLine(strTmp);

  return 0;
}

//-----------------------------------------------------------------------------
// Provide special output formatter for ICC Date/Time
// NOTE: It appears that the nParts had to be decoded in the
//       reverse order from what I had expected, so one should
//       confirm that the byte order / endianness is appropriate.
QString CjfifDecode::DecodeIccDateTime(uint32_t anVal[])
{
  QString strDate;

  uint16_t anParts[6];

  anParts[0] = (anVal[2] & 0xFFFF0000) >> 16;   // Year
  anParts[1] = (anVal[2] & 0x0000FFFF); // Mon
  anParts[2] = (anVal[1] & 0xFFFF0000) >> 16;   // Day
  anParts[3] = (anVal[1] & 0x0000FFFF); // Hour
  anParts[4] = (anVal[0] & 0xFFFF0000) >> 16;   // Min
  anParts[5] = (anVal[0] & 0x0000FFFF); // Sec
  strDate = QString("%1-%2-%3 %4:%5:%6")
      .arg(anParts[0], 4, 10, QChar('0'))
      .arg(anParts[1], 2, 10, QChar('0'))
      .arg(anParts[2], 2, 10, QChar('0'))
      .arg(anParts[3], 2, 10, QChar('0'))
      .arg(anParts[4], 2, 10, QChar('0'))
      .arg(anParts[5], 2, 10, QChar('0'));
  return strDate;
}

//-----------------------------------------------------------------------------
// Parser for APP2 ICC profile marker
uint32_t CjfifDecode::DecodeApp2IccProfile(uint32_t nLen)
{
  QString strTmp;

  uint32_t nMarkerSeqNum;       // Byte

  uint32_t nNumMarkers;         // Byte

  uint32_t nPayloadLen;         // Len of this ICC marker payload

  uint32_t nMarkerPosStart;

  nMarkerSeqNum = Buf(m_nPos++);
  nNumMarkers = Buf(m_nPos++);
  nPayloadLen = nLen - 2 - 12 - 2;      // TODO: check?

  strTmp = QString("      Marker Number = %1 of %2").arg(nMarkerSeqNum).arg(nNumMarkers);
  m_pLog->AddLine(strTmp);

  if(nMarkerSeqNum == 1)
  {
    nMarkerPosStart = m_nPos;
    DecodeIccHeader(nMarkerPosStart);
  }
  else
  {
    m_pLog->AddLineWarn("      Only support decode of 1st ICC Marker");
  }

  return 0;
}

//-----------------------------------------------------------------------------
// Parser for APP2 FlashPix marker
uint32_t CjfifDecode::DecodeApp2Flashpix()
{
  QString strTmp;

  uint32_t nFpxVer;
  uint32_t nFpxSegType;
  uint32_t nFpxInteropCnt;
  uint32_t nFpxEntitySz;
  uint32_t nFpxDefault;

  bool bFpxStorage;

  QString strFpxStorageClsStr;

  uint32_t nFpxStIndexCont;
  uint32_t nFpxStOffset;
  uint32_t nFpxStWByteOrder;
  uint32_t nFpxStWFormat;

  QString strFpxStClsidStr;

  uint32_t nFpxStDwOsVer;
  uint32_t nFpxStRsvd;

  QString streamStr;

  nFpxVer = Buf(m_nPos++);
  nFpxSegType = Buf(m_nPos++);

  // FlashPix segments: Contents List or Stream Data

  if(nFpxSegType == 1)
  {
    // Contents List
    strTmp = QString("    Segment: CONTENTS LIST");
    m_pLog->AddLine(strTmp);

    nFpxInteropCnt = (Buf(m_nPos++) << 8) + Buf(m_nPos++);
    strTmp = QString("      Interoperability Count = %1").arg(nFpxInteropCnt);
    m_pLog->AddLine(strTmp);

    for(uint32_t ind = 0; ind < nFpxInteropCnt; ind++)
    {
      strTmp = QString("      Entity Index #%1").arg(ind);
      m_pLog->AddLine(strTmp);
      nFpxEntitySz = (Buf(m_nPos++) << 24) + (Buf(m_nPos++) << 16) + (Buf(m_nPos++) << 8) + Buf(m_nPos++);

      // If the "entity size" field is 0xFFFFFFFF, then it should be treated as
      // a "storage". It looks like we should probably be using this to determine
      // that we have a "storage"
      bFpxStorage = false;

      if(nFpxEntitySz == 0xFFFFFFFF)
      {
        bFpxStorage = true;
      }

      if(!bFpxStorage)
      {
        strTmp = QString("        Entity Size = %1").arg(nFpxEntitySz);
        m_pLog->AddLine(strTmp);
      }
      else
      {
        strTmp = "        Entity is Storage";
        m_pLog->AddLine(strTmp);
      }

      nFpxDefault = Buf(m_nPos++);

      // BUG: #1112
      //streamStr = m_pWBuf->BufReadUniStr(m_nPos);
      streamStr = m_pWBuf->BufReadUniStr2(m_nPos, MAX_BUF_READ_STR);
      m_nPos += 2 * (static_cast<uint32_t>(streamStr.length()) + 1);        // 2x because unicode

      strTmp = QString("        Stream Name = [%1]").arg(streamStr);
      m_pLog->AddLine(strTmp);

      // In the case of "storage", we decode the next 16 bytes as the class
      if(bFpxStorage)
      {

        // FIXME:
        // NOTE: Very strange reordering required here. Doesn't seem consistent
        //       This means that other fields are probably wrong as well (endian)
        strFpxStorageClsStr = QString("%1%2%3%4-%5%6-%7%8-%9%10-%11%12%13%14%15%16")
            .arg(Buf(m_nPos + 3), 2, 16, QChar('0'))
            .arg(Buf(m_nPos + 2), 2, 16, QChar('0'))
            .arg(Buf(m_nPos + 1), 2, 16, QChar('0'))
            .arg(Buf(m_nPos + 0), 2, 16, QChar('0'))
            .arg(Buf(m_nPos + 5), 2, 16, QChar('0'))
            .arg(Buf(m_nPos + 4), 2, 16, QChar('0'))
            .arg(Buf(m_nPos + 7), 2, 16, QChar('0'))
            .arg(Buf(m_nPos + 6), 2, 16, QChar('0'))
            .arg(Buf(m_nPos + 8), 2, 16, QChar('0'))
            .arg(Buf(m_nPos + 9), 2, 16, QChar('0'))
            .arg(Buf(m_nPos + 10), 2, 16, QChar('0'))
            .arg(Buf(m_nPos + 11), 2, 16, QChar('0'))
            .arg(Buf(m_nPos + 12), 2, 16, QChar('0'))
            .arg(Buf(m_nPos + 13), 2, 16, QChar('0'))
            .arg(Buf(m_nPos + 14), 2, 16, QChar('0'))
            .arg(Buf(m_nPos + 15), 2, 16, QChar('0'));
        m_nPos += 16;
        strTmp = QString("        Storage Class = [%1]").arg(strFpxStorageClsStr);
        m_pLog->AddLine(strTmp);
      }
    }

    return 0;
  }
  else if(nFpxSegType == 2)
  {
    // Stream Data
    strTmp = "    Segment: STREAM DATA";
    m_pLog->AddLine(strTmp);

    nFpxStIndexCont = (Buf(m_nPos++) << 8) + Buf(m_nPos++);
    strTmp = QString("      Index in Contents List = %1").arg(nFpxStIndexCont);
    m_pLog->AddLine(strTmp);

    nFpxStOffset = (Buf(m_nPos++) << 24) + (Buf(m_nPos++) << 16) + (Buf(m_nPos++) << 8) + Buf(m_nPos++);
    strTmp = QString("      Offset in stream = %1 (0x%2)").arg(nFpxStOffset).arg(nFpxStOffset, 8, 16, QChar('0'));
    m_pLog->AddLine(strTmp);

    // Now decode the Property Set Header

    // NOTE: Should only decode this if we are doing first part of stream
    // TODO: How do we know this? First reference to index #?

    nFpxStWByteOrder = (Buf(m_nPos++) << 8) + Buf(m_nPos++);
    nFpxStWFormat = (Buf(m_nPos++) << 8) + Buf(m_nPos++);
    nFpxStDwOsVer = (Buf(m_nPos++) << 24) + (Buf(m_nPos++) << 16) + (Buf(m_nPos++) << 8) + Buf(m_nPos++);

    // FIXME:
    // NOTE: Very strange reordering required here. Doesn't seem consistent!
    //       This means that other fields are probably wrong as well (endian)
    strFpxStClsidStr =
      QString("%1%2%3%4-%5%6-%7%8-%9%10-%11%12%13%14%15%16")
        .arg(Buf(m_nPos + 3), 2, 16, QChar('0'))
        .arg(Buf(m_nPos + 2), 2, 16, QChar('0'))
        .arg(Buf(m_nPos + 1), 2, 16, QChar('0'))
        .arg(Buf(m_nPos + 0), 2, 16, QChar('0'))
        .arg(Buf(m_nPos + 5), 2, 16, QChar('0'))
        .arg(Buf(m_nPos + 4), 2, 16, QChar('0'))
        .arg(Buf(m_nPos + 7), 2, 16, QChar('0'))
        .arg(Buf(m_nPos + 6), 2, 16, QChar('0'))
        .arg(Buf(m_nPos + 8), 2, 16, QChar('0'))
        .arg(Buf(m_nPos + 9), 2, 16, QChar('0'))
        .arg(Buf(m_nPos + 10), 2, 16, QChar('0'))
        .arg(Buf(m_nPos + 11), 2, 16, QChar('0'))
        .arg(Buf(m_nPos + 12), 2, 16, QChar('0'))
        .arg(Buf(m_nPos + 13), 2, 16, QChar('0'))
        .arg(Buf(m_nPos + 14), 2, 16, QChar('0'))
        .arg(Buf(m_nPos + 15), 2, 16, QChar('0'));
    m_nPos += 16;
    nFpxStRsvd = (Buf(m_nPos++) << 8) + Buf(m_nPos++);

    strTmp = QString("      ByteOrder = 0x%1").arg(nFpxStWByteOrder, 4, 16, QChar('0'));
    m_pLog->AddLine(strTmp);

    strTmp = QString("      Format = 0x%1").arg(nFpxStWFormat, 4, 16, QChar('0'));
    m_pLog->AddLine(strTmp);

    strTmp = QString("      OSVer = 0x%1").arg(nFpxStDwOsVer, 8, 16, QChar('0'));
    m_pLog->AddLine(strTmp);

    strTmp = QString("      clsid = %1").arg(strFpxStClsidStr);
    m_pLog->AddLine(strTmp);

    strTmp = QString("      reserved = 0x%1").arg(nFpxStRsvd, 8, 16, QChar('0'));
    m_pLog->AddLine(strTmp);

    // ....

    return 2;

  }
  else
  {
    m_pLog->AddLineErr("      Reserved Segment. Stopping.");
    return 1;
  }
}

//-----------------------------------------------------------------------------
// Decode the DHT marker segment (Huffman Tables)
// In some cases (such as for MotionJPEG), we fake out
// the DHT tables (when bInject=true) with a standard table
// as each JPEG frame in the MJPG does not include the DHT.
// In all other cases (bInject=false), we simply read the
// DHT table from the file buffer via Buf()
//
// ITU-T standard indicates that we can expect up to a
// maximum of 16-bit huffman code bitstrings.
void CjfifDecode::DecodeDHT(bool bInject)
{
  uint32_t nLength;
  uint32_t nTmpVal;

  QString strTmp, strFull;

  uint32_t nPosEnd;
  uint32_t nPosSaved = 0;

  bool bRet;

  if(bInject)
  {
    // Redirect Buf() to DHT table in MJPGDHTSeg[]
    // ... so change mode that Buf() call uses
    m_bBufFakeDHT = true;

    // Preserve the "m_nPos" pointer, at end we undo it
    // And we also start at 2 which is just after FFC4 in array
    nPosSaved = m_nPos;
    m_nPos = 2;
  }

  nLength = Buf(m_nPos) * 256 + Buf(m_nPos + 1);
  nPosEnd = m_nPos + nLength;
  m_nPos += 2;
  strTmp = QString("  Huffman table length = %1").arg(nLength);
  m_pLog->AddLine(strTmp);

  uint32_t nDhtClass_Tc;        // Range 0..1
  uint32_t nDhtHuffTblId_Th;    // Range 0..3

  // In various places, added m_bStateAbort check to allow us
  // to escape in case we get in excessive number of DHT entries
  // See BUG FIX #1003

  while((!m_bStateAbort) && (nPosEnd > m_nPos))
  {
    m_pLog->AddLine("  ----");

    nTmpVal = Buf(m_nPos++);
    nDhtClass_Tc = (nTmpVal & 0xF0) >> 4;       // Tc, range 0..1
    nDhtHuffTblId_Th = nTmpVal & 0x0F;  // Th, range 0..3
    strTmp = QString("  Destination ID = %1").arg(nDhtHuffTblId_Th);
    m_pLog->AddLine(strTmp);
    strTmp = QString("  Class = %1 (%2)").arg(nDhtClass_Tc).arg(nDhtClass_Tc ? "AC Table" : "DC / Lossless Table");
    m_pLog->AddLine(strTmp);

    // Add in some error checking to prevent
    if(nDhtClass_Tc >= MAX_DHT_CLASS)
    {
      strTmp = QString("ERROR: Invalid DHT Class (%1). Aborting DHT Load.").arg(nDhtClass_Tc);
      m_pLog->AddLineErr(strTmp);
      m_nPos = nPosEnd;
      //m_bStateAbort = true; // Stop decoding
      break;
    }

    if(nDhtHuffTblId_Th >= MAX_DHT_DEST_ID)
    {
      strTmp = QString("ERROR: Invalid DHT Dest ID (%1). Aborting DHT Load.").arg(nDhtHuffTblId_Th);
      m_pLog->AddLineErr(strTmp);
      m_nPos = nPosEnd;
      //m_bStateAbort = true; // Stop decoding
      break;
    }

    // Read in the array of DHT code lengths
    for(uint32_t i = 1; i <= MAX_DHT_CODELEN; i++)
    {
      m_anDhtNumCodesLen_Li[i] = Buf(m_nPos++); // Li, range 0..255
    }

#define DECODE_DHT_MAX_DHT 256
    uint32_t anDhtCodeVal[DECODE_DHT_MAX_DHT + 1];  // Should only need max 162 codes
    uint32_t nDhtInd;
    uint32_t nDhtCodesTotal;

    // Clear out the code list
    for(nDhtInd = 0; nDhtInd < DECODE_DHT_MAX_DHT; nDhtInd++)
    {
      anDhtCodeVal[nDhtInd] = 0xFFFF;   // Dummy value
    }

    // Now read in all of the DHT codes according to the lengths
    // read in earlier
    nDhtCodesTotal = 0;
    nDhtInd = 0;

    for(uint32_t nIndLen = 1; ((!m_bStateAbort) && (nIndLen <= MAX_DHT_CODELEN)); nIndLen++)
    {
      // Keep a total count of the number of DHT codes read
      nDhtCodesTotal += m_anDhtNumCodesLen_Li[nIndLen];

      strFull = QString("    Codes of length %1 bits (%2 total): ")
          .arg(nIndLen, 2, 10, QChar('0'))
          .arg(m_anDhtNumCodesLen_Li[nIndLen], 3, 10, QChar('0'));

      for(uint32_t nIndCode = 0; ((!m_bStateAbort) && (nIndCode < m_anDhtNumCodesLen_Li[nIndLen])); nIndCode++)
      {
        // Start a new line for every 16 codes
        if((nIndCode != 0) && ((nIndCode % 16) == 0))
        {
          strFull = "                                         ";
        }

        nTmpVal = Buf(m_nPos++);
        strTmp = QString("%1 ").arg(nTmpVal, 2, 16, QChar('0'));
        strFull += strTmp;

        // Only write 16 codes per line
        if((nIndCode % 16) == 15)
        {
          m_pLog->AddLine(strFull);
          strFull = "";
        }

        // Save the huffman code
        // Just in case we have more DHT codes than we expect, trap
        // the range check here, otherwise we'll have buffer overrun!
        if(nDhtInd < DECODE_DHT_MAX_DHT)
        {
          anDhtCodeVal[nDhtInd++] = nTmpVal;    // Vij, range 0..255
        }
        else
        {
          nDhtInd++;
          strTmp = QString("Excessive DHT entries (%1)... skipping").arg(nDhtInd);
          m_pLog->AddLineErr(strTmp);

          if(!m_bStateAbort)
          {
            DecodeErrCheck(true);
          }
        }
      }

      m_pLog->AddLine(strFull);
    }

    strTmp = QString("    Total number of codes: %1").arg(nDhtCodesTotal, 3, 10, QChar('0'));
    m_pLog->AddLine(strTmp);

    uint32_t nDhtLookupInd = 0;

    // Now print out the actual binary strings!
    uint32_t nCodeVal = 0;

    nDhtInd = 0;

    if(m_pAppConfig->expandDht())
    {
      m_pLog->AddLine("");
      m_pLog->AddLine("  Expanded Form of Codes:");
    }

    for(uint32_t nBitLen = 1; ((!m_bStateAbort) && (nBitLen <= 16)); nBitLen++)
    {
      if(m_anDhtNumCodesLen_Li[nBitLen] > 0)
      {
        if(m_pAppConfig->expandDht())
        {
          strTmp = QString("    Codes of length %1 bits:").arg(nBitLen, 2, 10, QChar('0'));
          m_pLog->AddLine(strTmp);
        }

        // Codes exist for this bit-length
        // Walk through and generate the bitvalues
        for(uint32_t bit_ind = 1; ((!m_bStateAbort) && (bit_ind <= m_anDhtNumCodesLen_Li[nBitLen])); bit_ind++)
        {
          uint32_t nDecVal = nCodeVal;
          uint32_t nBinBit;

          char acBinStr[17] = "";

          uint32_t nBinStrLen = 0;

          // If the user has enabled output of DHT expanded tables,
          // report the bit-string sequences.
          if(m_pAppConfig->expandDht())
          {
            for(uint32_t nBinInd = nBitLen; nBinInd >= 1; nBinInd--)
            {
              nBinBit = (nDecVal >> (nBinInd - 1)) & 1;
              acBinStr[nBinStrLen++] = (nBinBit) ? '1' : '0';
            }

            acBinStr[nBinStrLen] = '\0';
            strFull = QString("      %1 = %2").arg(acBinStr).arg(anDhtCodeVal[nDhtInd], 2, 16, QChar('0'));

            // The following are only valid for AC components
            // Bug [3442132]
            if(nDhtClass_Tc == DHT_CLASS_AC)
            {
              if(anDhtCodeVal[nDhtInd] == 0x00)
              {
                strFull += " (EOB)";
              }
              if(anDhtCodeVal[nDhtInd] == 0xF0)
              {
                strFull += " (ZRL)";
              }
            }

            strTmp = QString("%1 (Total Len = %2)").arg(strFull, -40).arg(nBitLen + (anDhtCodeVal[nDhtInd] & 0xF), 2);

            m_pLog->AddLine(strTmp);
          }

          // Store the lookup value
          // Shift left to MSB of 32-bit
          uint32_t nTmpMask = m_anMaskLookup[nBitLen];
          uint32_t nTmpBits = nDecVal << (32 - nBitLen);
          uint32_t nTmpCode = anDhtCodeVal[nDhtInd];

          bRet = m_pImgDec->SetDhtEntry(nDhtHuffTblId_Th, nDhtClass_Tc, nDhtLookupInd, nBitLen, nTmpBits, nTmpMask, nTmpCode);

          DecodeErrCheck(bRet);

          nDhtLookupInd++;

          // Move to the next code
          nCodeVal++;
          nDhtInd++;
        }
      }

      // For each loop iteration (on bit length), we shift the code value
      nCodeVal <<= 1;
    }

    // Now store the dht_lookup_size
    uint32_t nTmpSize = nDhtLookupInd;

    bRet = m_pImgDec->SetDhtSize(nDhtHuffTblId_Th, nDhtClass_Tc, nTmpSize);

    if(!m_bStateAbort)
    {
      DecodeErrCheck(bRet);
    }

    m_pLog->AddLine("");
  }

  if(bInject)
  {
    // Restore position (as if we didn't move)
    m_nPos = nPosSaved;
    m_bBufFakeDHT = false;
  }
}

//-----------------------------------------------------------------------------
// Check return value of previous call. If failed, then ask
// user if they wish to continue decoding. If no, then flag to
// the decoder that we're done (avoids continuous failures)
void CjfifDecode::DecodeErrCheck(bool bRet)
{
  if(!bRet)
  {
    if(m_pAppConfig->interactive())
    {
      msgBox.setText("Do you want to continue decoding?");
      msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

      if(msgBox.exec() == QMessageBox::No)
      {
        m_bStateAbort = true;
      }
    }
  }
}

//-----------------------------------------------------------------------------
// This routine is called after the expected fields of the marker segment
// have been processed. The file position should line up with the offset
// dictated by the marker length. If a mismatch is detected, report an
// error.
//
// RETURN:
// - True if decode error is fatal (configurable)
//
bool CjfifDecode::ExpectMarkerEnd(uint32_t nMarkerStart, uint32_t nMarkerLen)
{
  QString strTmp;

  uint32_t nMarkerEnd = nMarkerStart + nMarkerLen;
  uint32_t nMarkerExtra = nMarkerEnd - m_nPos;

  if(m_nPos < nMarkerEnd)
  {
    // The length indicates that there is more data than we processed
    strTmp = QString("  WARNING: Marker length longer than expected");
    m_pLog->AddLineWarn(strTmp);

    if(!m_pAppConfig->relaxedParsing())
    {
      // Abort
      m_pLog->AddLineErr("  Stopping decode");
      m_pLog->AddLineErr("  Use [Img Search Fwd/Rev] to locate other valid embedded JPEGs");
      return false;
    }
    else
    {
      // Warn and skip
      strTmp = QString("  Skipping remainder [%1 bytes]").arg(nMarkerExtra);
      m_pLog->AddLineWarn(strTmp);
      m_nPos += nMarkerExtra;
    }
  }
  else if(m_nPos > nMarkerEnd)
  {
    // The length indicates that there is less data than we processed
    strTmp = QString("  WARNING: Marker length shorter than expected");
    m_pLog->AddLineWarn(strTmp);

    if(!m_pAppConfig->relaxedParsing())
    {
      // Abort
      m_pLog->AddLineErr("  Stopping decode");
      m_pLog->AddLineErr("  Use [Img Search Fwd/Rev] to locate other valid embedded JPEGs");
      return false;
    }
    else
    {
      // Warn but no skip
      // Note that we can't skip as the length would imply a rollback
      // Most resilient solution is probably to assume length was
      // wrong and continue from where the marker should have ended.
      // For resiliency, attempt two methods to find point to resume:
      // 1) Current position
      // 2) Actual length defined in marker
      if(Buf(m_nPos) == 0xFF)
      {
        // Using actual data expected seems more promising
        m_pLog->AddLineWarn("  Resuming decode");
      }
      else if(Buf(nMarkerEnd) == 0xFF)
      {
        // Using actual length seems more promising
        m_nPos = nMarkerEnd;
        m_pLog->AddLineWarn("  Rolling back pointer to end indicated by length");
        m_pLog->AddLineWarn("  Resuming decode");
      }
      else
      {
        // No luck. Expect marker failure now
        m_pLog->AddLineWarn("  Resuming decode");
      }
    }
  }

  // If we get here, then we haven't seen a fatal issue
  return true;
}

//-----------------------------------------------------------------------------
// Validate an unsigned value to ensure it is in allowable range
// - If the value is outside the range, an error is shown and
//   the parsing stops if relaxed parsing is not enabled
// - An optional override value is provided for the resume case
//
// INPUT:
// - nVal                       Input value (unsigned 32-bit)
// - nMin                       Minimum allowed value
// - nMax                       Maximum allowed value
// - strName            Name of the field
// - bOverride          Should we override the value upon out-of-range?
// - nOverrideVal       Value to override if bOverride and out-of-range
//
// PRE:
// - m_pAppConfig
//
// OUTPUT:
// - nVal                       Output value (including any override)
//
bool CjfifDecode::ValidateValue(uint32_t &nVal, uint32_t nMin, uint32_t nMax, QString strName, bool bOverride,
                                uint32_t nOverrideVal)
{
  QString strErr;

  if((nVal >= nMin) && (nVal <= nMax))
  {
    // Value is within range
    return true;
  }
  else
  {
    if(nVal < nMin)
    {
      strErr = QString("  ERROR: %1 value too small (Actual = %2, Expected >= %3)").arg(strName).arg(nVal).arg(nMin);
      m_pLog->AddLineErr(strErr);
    }
    else if(nVal > nMax)
    {
      strErr = QString("  ERROR: %1 value too large (Actual = %2, Expected <= %3)").arg(strName).arg(nVal).arg(nMax);
      m_pLog->AddLineErr(strErr);
    }

    if(!m_pAppConfig->relaxedParsing())
    {
      // Defined as fatal error
      // TODO: Replace with glb_strMsgStopDecode?
      m_pLog->AddLineErr("  Stopping decode");
      m_pLog->AddLineErr("  Use [Relaxed Parsing] to continue");
      return false;
    }
    else
    {
      // Non-fatal
      if(bOverride)
      {
        // Update value with override
        nVal = nOverrideVal;
        strErr = QString("  WARNING: Forcing value to [%1]").arg(nOverrideVal);
        m_pLog->AddLineWarn(strErr);
        m_pLog->AddLineWarn("  Resuming decode");
      }
      else
      {
        // No override
        strErr = QString("  Resuming decode");
        m_pLog->AddLineWarn(strErr);
      }
      return true;
    }
  }
}

//-----------------------------------------------------------------------------
// This is the primary JFIF marker parser. It reads the
// marker value at the current file position and launches the
// specific parser routine. This routine exits when
#define DECMARK_OK 0
#define DECMARK_ERR 1
#define DECMARK_EOI 2
uint32_t CjfifDecode::DecodeMarker()
{
  char acIdentifier[MAX_IDENTIFIER];

  QString strTmp;
  QString strFull;              // Used for concatenation

  uint32_t nLength;             // General purpose

  uint32_t nTmpVal;
  uint8_t nTmpVal1;

  uint16_t nTmpVal2;

  uint32_t nCode;
  uint32_t nPosEnd;
  uint32_t nPosSaved;      // General-purpose saved position in file
  uint32_t nPosExifStart;
  uint32_t nRet;                // General purpose return value

  bool bRet;

  uint32_t nPosMarkerStart;        // Offset for current marker
  uint32_t nColTransform = 0;   // Color Transform from APP14 marker

  // For DQT
  QString strDqtPrecision = "";

  QString strDqtZigZagOrder = "";

  if(Buf(m_nPos) != 0xFF)
  {
    if(m_nPos == 0)
    {
      // Don't give error message if we've already alerted them of AVI / PSD
      if((!m_bAvi) && (!m_bPsd))
      {
        strTmp = "NOTE: File did not start with JPEG marker. Consider using [Tools->Img Search Fwd] to locate embedded JPEG.";
        m_pLog->AddLineErr(strTmp);
      }
    }
    else
    {
      strTmp =
        QString("ERROR: Expected marker 0xFF, got 0x%1 @ offset 0x%2. Consider using [Tools->Img Search Fwd/Rev].")
          .arg(Buf(m_nPos), 2, 16, QChar('0'))
          .arg(m_nPos, 8, 16, QChar('0'));
      m_pLog->AddLineErr(strTmp);
    }

    m_nPos++;
    return DECMARK_ERR;
  }

  m_nPos++;

  // Read the current marker code
  nCode = Buf(m_nPos++);

  // Handle Marker Padding
  //
  // According to Section B.1.1.2:
  //   "Any marker may optionally be preceded by any number of fill bytes, which are bytes assigned code X�FF�."
  //
  uint32_t nSkipMarkerPad = 0;

  while(nCode == 0xFF)
  {
    // Count the pad
    nSkipMarkerPad++;
    // Read another byte
    nCode = Buf(m_nPos++);
  }

  // Report out any padding
  if(nSkipMarkerPad > 0)
  {
    strTmp = QString("*** Skipped %1 marker pad bytes ***").arg(nSkipMarkerPad);
    m_pLog->AddLineHdr(strTmp);
  }

  // Save the current marker offset
  nPosMarkerStart = m_nPos;

  AddHeader(nCode);

  switch (nCode)
  {
    case JFIF_SOI:             // SOI
      m_bStateSoi = true;
      break;

    case JFIF_APP12:
      // Photoshop DUCKY (Save For Web)
      nLength = Buf(m_nPos) * 256 + Buf(m_nPos + 1);
      //nLength = m_pWBuf->BufX(m_nPos,2,!m_nImgExifEndian);
      strTmp = QString("  Length          = %1").arg(nLength);
      m_pLog->AddLine(strTmp);

      nPosSaved = m_nPos;

      m_nPos += 2;              // Move past length now that we've used it

      strcpy(acIdentifier, m_pWBuf->BufReadStrn(m_nPos, MAX_IDENTIFIER - 1).toLatin1().data());
      acIdentifier[MAX_IDENTIFIER - 1] = 0;     // Null terminate just in case
      strTmp = QString("  Identifier      = [%1]").arg(acIdentifier);
      m_pLog->AddLine(strTmp);
      m_nPos += static_cast<uint32_t>(strlen(acIdentifier)) + 1;

      if(strcmp(acIdentifier, "Ducky") != 0)
      {
        m_pLog->AddLine("    Not Photoshop DUCKY. Skipping remainder.");
      }
      else                      // Photoshop
      {
        // Please see reference on http://cpan.uwinnipeg.ca/htdocs/Image-ExifTool/Image/ExifTool/APP12.pm.html
        // A direct indexed approach should be safe
        m_nImgQualPhotoshopSfw = Buf(m_nPos + 6);
        strTmp = QString("  Photoshop Save For Web Quality = [%1]").arg(m_nImgQualPhotoshopSfw);
        m_pLog->AddLine(strTmp);
      }

      // Restore original position in file to a point
      // after the section
      m_nPos = nPosSaved + nLength;
      break;

    case JFIF_APP14:
      // JPEG Adobe  tag

      nLength = Buf(m_nPos) * 256 + Buf(m_nPos + 1);
      strTmp = QString("  Length            = %1").arg(nLength);
      m_pLog->AddLine(strTmp);

      nPosSaved = m_nPos;

      // Some files had very short segment (eg. nLength=2)
      if(nLength < 2 + 12)
      {
        m_pLog->AddLine("    Segment too short for Identifier. Skipping remainder.");
        m_nPos = nPosSaved + nLength;
        break;
      }

      m_nPos += 2;              // Move past length now that we've used it

      // TODO: Confirm Adobe flag
      m_nPos += 5;

      nTmpVal = Buf(m_nPos + 0) * 256 + Buf(m_nPos + 1);
      strTmp = QString("  DCTEncodeVersion  = %1").arg(nTmpVal);
      m_pLog->AddLine(strTmp);

      nTmpVal = Buf(m_nPos + 2) * 256 + Buf(m_nPos + 3);
      strTmp = QString("  APP14Flags0       = %1").arg(nTmpVal);
      m_pLog->AddLine(strTmp);

      nTmpVal = Buf(m_nPos + 4) * 256 + Buf(m_nPos + 5);
      strTmp = QString("  APP14Flags1       = %1").arg(nTmpVal);
      m_pLog->AddLine(strTmp);

      nColTransform = Buf(m_nPos + 6);

      switch (nColTransform)
      {
        case APP14_COLXFM_UNK_RGB:
          strTmp = QString("  ColorTransform    = %1 [Unknown (RGB or CMYK)]").arg(nColTransform);
          break;
        case APP14_COLXFM_YCC:
          strTmp = QString("  ColorTransform    = %1 [YCbCr]").arg(nColTransform);
          break;
        case APP14_COLXFM_YCCK:
          strTmp = QString("  ColorTransform    = %1 [YCCK]").arg(nColTransform);
          break;
        default:
          strTmp = QString("  ColorTransform    = %1 [???]").arg(nColTransform);
          break;
      }

      m_pLog->AddLine(strTmp);
      m_nApp14ColTransform = (nColTransform & 0xFF);

      // Restore original position in file to a point
      // after the section
      m_nPos = nPosSaved + nLength;

      break;

    case JFIF_APP13:
      // Photoshop (Save As)
      nLength = Buf(m_nPos) * 256 + Buf(m_nPos + 1);
      //nLength = m_pWBuf->BufX(m_nPos,2,!m_nImgExifEndian);
      strTmp = QString("  Length          = %1").arg(nLength);
      m_pLog->AddLine(strTmp);

      nPosSaved = m_nPos;

      // Some files had very short segment (eg. nLength=2)
      if(nLength < 2 + 20)
      {
        m_pLog->AddLine("    Segment too short for Identifier. Skipping remainder.");
        m_nPos = nPosSaved + nLength;
        break;
      }

      m_nPos += 2;              // Move past length now that we've used it

      strcpy(acIdentifier, m_pWBuf->BufReadStrn(m_nPos, MAX_IDENTIFIER - 1).toLatin1().data());
      acIdentifier[MAX_IDENTIFIER - 1] = 0;     // Null terminate just in case
      strTmp = QString("  Identifier      = [%1]").arg(acIdentifier);
      m_pLog->AddLine(strTmp);
      m_nPos += static_cast<uint32_t>(strlen(acIdentifier)) + 1;

      if(strcmp(acIdentifier, "Photoshop 3.0") != 0)
      {
        m_pLog->AddLine("    Not Photoshop. Skipping remainder.");
      }
      else                      // Photoshop
      {
        DecodeApp13Ps();
      }

      // Restore original position in file to a point
      // after the section
      m_nPos = nPosSaved + nLength;
      break;

    case JFIF_APP1:
      nLength = Buf(m_nPos) * 256 + Buf(m_nPos + 1);
      //nLength = m_pWBuf->BufX(m_nPos,2,!m_nImgExifEndian);
      strTmp = QString("  Length          = %1").arg(nLength);
      m_pLog->AddLine(strTmp);

      nPosSaved = m_nPos;

      m_nPos += 2;              // Move past length now that we've used it

      strcpy(acIdentifier, m_pWBuf->BufReadStrn(m_nPos, MAX_IDENTIFIER - 1).toLatin1().data());
      acIdentifier[MAX_IDENTIFIER - 1] = 0;     // Null terminate just in case
      strTmp = QString("  Identifier      = [%1]").arg(acIdentifier);
      m_pLog->AddLine(strTmp);
      m_nPos += static_cast<uint32_t>(strlen(acIdentifier));

      if(strncmp(acIdentifier, "http://ns.adobe.com/xap/1.0/\x00", 29) == 0)
      {                         //@@
        // XMP

        m_pLog->AddLine("    XMP = ");

        m_nPos++;

        // NOTE: This code currently treats the strings from the XMP section
        // as single byte characters. In reality, it should probably be
        // updated to support unicode properly.

        uint32_t nPosMarkerEnd = nPosSaved + nLength - 1;
        uint32_t sXmpLen = nPosMarkerEnd - m_nPos;

        uint8_t cXmpChar;

        bool bNonSpace;

        QString strLine;

        // Reset state
        strLine = "          |";
        bNonSpace = false;

        for(uint32_t nInd = 0; nInd < sXmpLen; nInd++)
        {
          // Get the next char (8-bit byte)
          cXmpChar = m_pWBuf->Buf(m_nPos + nInd);

          // Detect a non-space in line
          if((cXmpChar != 0x20) && (cXmpChar != 0x0A))
          {
            bNonSpace = true;
          }

          // Detect Linefeed, print out line
          if(cXmpChar == 0x0A)
          {
            // Only print line if some non-space elements!
            if(bNonSpace)
            {
              m_pLog->AddLine(strLine);
            }
            // Reset state
            strLine = "          |";
            bNonSpace = false;
          }
          else
          {
            // Add the char
            strLine += cXmpChar;
          }
        }
      }
      else if(strcmp(acIdentifier, "Exif") == 0)        //@@
      {
        // Only decode it further if it is EXIF format

        m_nPos += 2;            // Skip two 00 bytes

        nPosExifStart = m_nPos; // Save m_nPos @ start of EXIF used for all IFD offsets

        // =========== EXIF TIFF Header (Start) ===========
        // - Defined in Exif 2.2 Standard (JEITA CP-3451) section 4.5.2
        // - Contents (8 bytes total)
        //   - Byte order (2 bytes)
        //   - 0x002A (2 bytes)
        //   - Offset of 0th IFD (4 bytes)

        uint8_t acIdentifierTiff[9];

        strFull = "";
        strTmp = "";

        strFull = "  Identifier TIFF = ";

        for(uint32_t i = 0; i < 8; i++)
        {
          acIdentifierTiff[i] = static_cast<uint8_t>(Buf(m_nPos++));
        }

        strTmp = PrintAsHexUC(acIdentifierTiff, 8);
        strFull += strTmp;
        m_pLog->AddLine(strFull);

        switch (acIdentifierTiff[0] * 256 + acIdentifierTiff[1])
        {
          case 0x4949:         // "II"
            // Intel alignment
            m_nImgExifEndian = 0;
            m_pLog->AddLine("  Endian          = Intel (little)");
            break;
          case 0x4D4D:         // "MM"
            // Motorola alignment
            m_nImgExifEndian = 1;
            m_pLog->AddLine("  Endian          = Motorola (big)");
            break;
        }

        // We expect the TAG mark of 0x002A (depending on endian mode)
        uint32_t test_002a;

        test_002a = ByteSwap2(acIdentifierTiff[2], acIdentifierTiff[3]);
        strTmp = QString("  TAG Mark x002A  = 0x%1").arg(test_002a, 4, 16, QChar('0'));
        m_pLog->AddLine(strTmp);

        uint32_t nIfdCount;     // Current IFD #

        uint32_t nOffsetIfd1;

        // Mark pointer to EXIF Sub IFD as 0 so that we can
        // detect if the tag never showed up.
        m_nImgExifSubIfdPtr = 0;
        m_nImgExifMakerPtr = 0;
        m_nImgExifGpsIfdPtr = 0;
        m_nImgExifInteropIfdPtr = 0;

        bool exif_done = false;

        nOffsetIfd1 = ByteSwap4(acIdentifierTiff[4], acIdentifierTiff[5], acIdentifierTiff[6], acIdentifierTiff[7]);

        // =========== EXIF TIFF Header (End) ===========

        // =========== EXIF IFD 0 ===========
        // Do we start the 0th IFD for the "Primary Image Data"?
        // Even though the nOffsetIfd1 pointer should indicate to
        // us where the IFD should start (0x0008 if immediately after
        // EXIF TIFF Header), I have observed JPEG files that
        // do not contain the IFD. Therefore, we must check for this
        // condition by comparing against the APP marker length.
        // Example file: http://img9.imageshack.us/img9/194/90114543.jpg

        if((nPosSaved + nLength) <= (nPosExifStart + nOffsetIfd1))
        {
          // We've run out of space for any IFD, so cancel now
          exif_done = true;
          m_pLog->AddLine("  NOTE: No IFD entries");
        }

        nIfdCount = 0;

        while(!exif_done)
        {
          m_pLog->AddLine("");

          strTmp = QString("IFD%1").arg(nIfdCount);

          // Process the IFD
          nRet = DecodeExifIfd(strTmp, nPosExifStart, nOffsetIfd1);

          // Now that we have gone through all entries in the IFD directory,
          // we read the offset to the next IFD
          nOffsetIfd1 = ByteSwap4(Buf(m_nPos + 0), Buf(m_nPos + 1), Buf(m_nPos + 2), Buf(m_nPos + 3));
          m_nPos += 4;

          strTmp = QString("    Offset to Next IFD = 0x%1").arg(nOffsetIfd1, 8, 16, QChar('0'));
          m_pLog->AddLine(strTmp);

          if(nRet != 0)
          {
            // Error condition (DecodeExifIfd returned error)
            nOffsetIfd1 = 0x00000000;
          }

          if(nOffsetIfd1 == 0x00000000)
          {
            // Either error condition or truly end of IFDs
            exif_done = true;
          }
          else
          {
            nIfdCount++;
          }

        }                       // while ! exif_done

        // If EXIF SubIFD was defined, then handle it now
        if(m_nImgExifSubIfdPtr != 0)
        {
          m_pLog->AddLine("");
          DecodeExifIfd("SubIFD", nPosExifStart, m_nImgExifSubIfdPtr);
        }

        if(m_nImgExifMakerPtr != 0)
        {
          m_pLog->AddLine("");
          DecodeExifIfd("MakerIFD", nPosExifStart, m_nImgExifMakerPtr);
        }

        if(m_nImgExifGpsIfdPtr != 0)
        {
          m_pLog->AddLine("");
          DecodeExifIfd("GPSIFD", nPosExifStart, m_nImgExifGpsIfdPtr);
        }

        if(m_nImgExifInteropIfdPtr != 0)
        {
          m_pLog->AddLine("");
          DecodeExifIfd("InteropIFD", nPosExifStart, m_nImgExifInteropIfdPtr);
        }
      }
      else
      {
        strTmp = QString("Identifier [%1] not supported. Skipping remainder.").arg(acIdentifier);
        m_pLog->AddLine(strTmp);
      }

      //////////

      // Dump out Makernote area

      // TODO: Disabled for now
#if 0
      uint32_t ptr_base;

      if(m_bVerbose)
      {
        if(m_nImgExifMakerPtr != 0)
        {
          // FIXME: Seems that nPosExifStart is not initialized in VERBOSE mode
          ptr_base = nPosExifStart + m_nImgExifMakerPtr;

          m_pLog->AddLine("Exif Maker IFD DUMP");
          strFull = QString("  MarkerOffset @ 0x%08X"), ptr_base;
          m_pLog->AddLine(strFull);
        }
      }
#endif

      // End of dump out makernote area

      // Restore file position
      m_nPos = nPosSaved;

      // Restore original position in file to a point
      // after the section
      m_nPos = nPosSaved + nLength;

      break;

    case JFIF_APP2:
      // Typically used for Flashpix and possibly ICC profiles
      // Photoshop (Save As)
      nLength = Buf(m_nPos) * 256 + Buf(m_nPos + 1);
      //nLength = m_pWBuf->BufX(m_nPos,2,!m_nImgExifEndian);
      strTmp = QString("  Length          = %1").arg(nLength);
      m_pLog->AddLine(strTmp);

      nPosSaved = m_nPos;

      m_nPos += 2;              // Move past length now that we've used it

      strcpy(acIdentifier, m_pWBuf->BufReadStrn(m_nPos, MAX_IDENTIFIER - 1).toLatin1().data());
      acIdentifier[MAX_IDENTIFIER - 1] = 0;     // Null terminate just in case
      strTmp = QString("  Identifier      = [%1]").arg(acIdentifier);
      m_pLog->AddLine(strTmp);
      m_nPos += static_cast<uint32_t>(strlen(acIdentifier)) + 1;

      if(strcmp(acIdentifier, "FPXR") == 0)
      {
        // Photoshop
        m_pLog->AddLine("    FlashPix:");
        DecodeApp2Flashpix();
      }
      else if(strcmp(acIdentifier, "ICC_PROFILE") == 0)
      {
        // ICC Profile
        m_pLog->AddLine("    ICC Profile:");
        DecodeApp2IccProfile(nLength);
      }
      else
      {
        m_pLog->AddLine("    Not supported. Skipping remainder.");
      }

      // Restore original position in file to a point
      // after the section
      m_nPos = nPosSaved + nLength;
      break;

    case JFIF_APP3:
    case JFIF_APP4:
    case JFIF_APP5:
    case JFIF_APP6:
    case JFIF_APP7:
    case JFIF_APP8:
    case JFIF_APP9:
    case JFIF_APP10:
    case JFIF_APP11:
      //case JFIF_APP12: // Handled separately
      //case JFIF_APP13: // Handled separately
      //case JFIF_APP14: // Handled separately
    case JFIF_APP15:
      nLength = Buf(m_nPos) * 256 + Buf(m_nPos + 1);
      //nLength = m_pWBuf->BufX(m_nPos,2,!m_nImgExifEndian);
      m_pLog->AddLine(QString("  Length     = %1").arg(nLength));

      if(m_bVerbose)
      {
        strFull = "";

        for(uint32_t i = 0; i < nLength; i++)
        {
          // Start a new line for every 16 codes
          if((i % 16) == 0)
          {
            strFull = QString("  MarkerOffset [%1]: ").arg(i, 4, 16, QChar('0'));
          }
          else if((i % 8) == 0)
          {
            strFull += "  ";
          }

          nTmpVal = Buf(m_nPos + i);
          strFull += QString("%1 ").arg(nTmpVal, 2, 16, QChar('0'));

          if((i % 16) == 15)
          {
            m_pLog->AddLine(strFull);
            strFull = "";
          }
        }

        m_pLog->AddLine(strFull);

        strFull = "";

        for(uint32_t i = 0; i < nLength; i++)
        {
          // Start a new line for every 16 codes
          if((i % 32) == 0)
          {
            strFull = QString("  MarkerOffset [%1]: ").arg(i, 4, 16, QChar('0'));
          }
          else if((i % 8) == 0)
          {
            strFull += " ";
          }

          nTmpVal1 = Buf(m_nPos + i);

          if(isprint(nTmpVal1))
          {
            strFull += QString("%1").arg(nTmpVal1);
          }
          else
          {
            strFull += ".";
          }

          if((i % 32) == 31)
          {
            m_pLog->AddLine(strFull);
          }
        }

        m_pLog->AddLine(strFull);
      }                         // nVerbose

      m_nPos += nLength;
      break;

    case JFIF_APP0:            // APP0
      nLength = Buf(m_nPos) * 256 + Buf(m_nPos + 1);
      //nLength = m_pWBuf->BufX(m_nPos,2,!m_nImgExifEndian);
      m_nPos += 2;
      strTmp = QString("  Length     = %1").arg(nLength);
      m_pLog->AddLine(strTmp);

      strcpy(m_acApp0Identifier, m_pWBuf->BufReadStrn(m_nPos, MAX_IDENTIFIER - 1).toLatin1().data());
      m_acApp0Identifier[MAX_IDENTIFIER - 1] = 0;       // Null terminate just in case
      strTmp = QString("  Identifier = [%1]").arg(m_acApp0Identifier);
      m_pLog->AddLine(strTmp);

      if(strcmp(m_acApp0Identifier, "JFIF"))
      {
        // Only process remainder if it is JFIF. This marker
        // is also used for application-specific functions.

        m_nPos += static_cast<uint32_t>((strlen(m_acApp0Identifier)) + 1);

        m_nImgVersionMajor = Buf(m_nPos++);
        m_nImgVersionMinor = Buf(m_nPos++);
        strTmp = QString("  version    = [%1.%2]").arg(m_nImgVersionMajor).arg(m_nImgVersionMinor);
        m_pLog->AddLine(strTmp);

        m_nImgUnits = Buf(m_nPos++);

        m_nImgDensityX = Buf(m_nPos) * 256 + Buf(m_nPos + 1);
        //m_nImgDensityX = m_pWBuf->BufX(m_nPos,2,!m_nImgExifEndian);
        m_nPos += 2;
        m_nImgDensityY = Buf(m_nPos) * 256 + Buf(m_nPos + 1);
        //m_nImgDensityY = m_pWBuf->BufX(m_nPos,2,!m_nImgExifEndian);
        m_nPos += 2;
        strTmp = QString("  density    = %1 x %2 ").arg(m_nImgDensityX).arg(m_nImgDensityY);
        strFull = strTmp;

        switch (m_nImgUnits)
        {
          case 0:
            strFull += "(aspect ratio)";
            m_pLog->AddLine(strFull);
            break;

          case 1:
            strFull += "DPI (dots per inch)";
            m_pLog->AddLine(strFull);
            break;

          case 2:
            strFull += "DPcm (dots per cm)";
            m_pLog->AddLine(strFull);
            break;

          default:
            strTmp = QString("ERROR: Unknown ImgUnits parameter [%1]").arg(m_nImgUnits);
            strFull += strTmp;
            m_pLog->AddLineWarn(strFull);
            //return DECMARK_ERR;
            break;
        }

        m_nImgThumbSizeX = Buf(m_nPos++);
        m_nImgThumbSizeY = Buf(m_nPos++);
        strTmp = QString("  thumbnail  = %1 x %2").arg(m_nImgThumbSizeX).arg(m_nImgThumbSizeY);
        m_pLog->AddLine(strTmp);

        // Unpack the thumbnail:
        uint32_t thumbnail_r, thumbnail_g, thumbnail_b;

        if(m_nImgThumbSizeX && m_nImgThumbSizeY)
        {
          for(uint32_t y = 0; y < m_nImgThumbSizeY; y++)
          {
            strFull = QString("   Thumb[%1] = ").arg(y, 3, 10, QChar('0'));

            for(uint32_t x = 0; x < m_nImgThumbSizeX; x++)
            {
              thumbnail_r = Buf(m_nPos++);
              thumbnail_g = Buf(m_nPos++);
              thumbnail_b = Buf(m_nPos++);
              strTmp = QString("(0x%1,0x%2,0x%3) ")
                  .arg(thumbnail_r, 2, 16, QChar('0'))
                  .arg(thumbnail_g, 2, 16, QChar('0'))
                  .arg(thumbnail_b, 2, 16, QChar('0'));
              strFull += strTmp;
              m_pLog->AddLine(strFull);
            }
          }
        }

        // TODO:
        // - In JPEG-B mode (GeoRaster), we will need to fake out
        //   the DHT & DQT tables here. Unfortunately, we'll have to
        //   rely on the user to put us into this mode as there is nothing
        //   in the file that specifies this mode.

        /*
           // TODO: Need to ensure that Faked DHT is correct table

           AddHeader(JFIF_DHT_FAKE);
           DecodeDHT(true);
           // Need to mark DHT tables as OK
           m_bStateDht = true;
           m_bStateDhtFake = true;
           m_bStateDhtOk = true;

           // ... same for DQT
         */

      }
      else if(strncmp(m_acApp0Identifier, "AVI1", 4))   //@@
      {
        // AVI MJPEG type

        // Need to fill in predefined DHT table from spec:
        //   OpenDML file format for AVI, section "Proposed Data Chunk Format"
        //   Described in MMREG.H
        m_pLog->AddLine("  Detected MotionJPEG");
        m_pLog->AddLine("  Importing standard Huffman table...");
        m_pLog->AddLine("");

        AddHeader(JFIF_DHT_FAKE);

        DecodeDHT(true);
        // Need to mark DHT tables as OK
        m_bStateDht = true;
        m_bStateDhtFake = true;
        m_bStateDhtOk = true;

        m_nPos += nLength - 2;  // Skip over, and undo length short read

      }
      else
      {
        // Not JFIF or AVI1
        m_pLog->AddLine("    Not known APP0 type. Skipping remainder.");
        m_nPos += nLength - 2;
      }

      if(!ExpectMarkerEnd(nPosMarkerStart, nLength))
        return DECMARK_ERR;

      break;

    case JFIF_DQT:             // Define quantization tables
      m_bStateDqt = true;

      uint32_t nDqtPrecision_Pq;
      uint32_t nDqtQuantDestId_Tq;

      nLength = Buf(m_nPos) * 256 + Buf(m_nPos + 1);    // Lq
      nPosEnd = m_nPos + nLength;
      m_nPos += 2;
      strTmp = QString("  Table length = %1").arg(nLength);
      m_pLog->AddLine(strTmp);

      while(nPosEnd > m_nPos)
      {
        m_pLog->AddLine("  ----");

        nTmpVal = Buf(m_nPos++);        // Pq | Tq
        nDqtPrecision_Pq = (nTmpVal & 0xF0) >> 4;       // Pq, range 0-1
        nDqtQuantDestId_Tq = nTmpVal & 0x0F;    // Tq, range 0-3

        // Decode per ITU-T.81 standard
#if 1
        if(nDqtPrecision_Pq == 0)
        {
          strDqtPrecision = "8 bits";
        }
        else if(nDqtPrecision_Pq == 1)
        {
          strDqtPrecision = "16 bits";
        }
        else
        {
          m_pLog->AddLineWarn(QString("    Unsupported precision value [%1]").arg(nDqtPrecision_Pq));
          strDqtPrecision = "???";
          // FIXME: Consider terminating marker parsing early
        }

        if(!ValidateValue(nDqtPrecision_Pq, 0, 1, "DQT Precision <Pq>", true, 0))
          return DECMARK_ERR;

        if(!ValidateValue(nDqtQuantDestId_Tq, 0, 3, "DQT Destination ID <Tq>", true, 0))
          return DECMARK_ERR;

        strTmp = QString("  Precision=%1").arg(strDqtPrecision);
        m_pLog->AddLine(strTmp);
#else
        // Decode with additional DQT extension (ITU-T-JPEG-Plus-Proposal_R3.doc)

        if((nDqtPrecision_Pq & 0xE) == 0)
        {
          // Per ITU-T.81 Standard
          if(nDqtPrecision_Pq == 0)
          {
            strDqtPrecision = "8 bits";
          }
          else if(nDqtPrecision_Pq == 1)
          {
            strDqtPrecision = "16 bits";
          }

          strTmp = QString("  Precision=%1"), strDqtPrecision;
          m_pLog->AddLine(strTmp);
        }
        else
        {
          // Non-standard
          // JPEG-Plus-Proposal-R3:
          // - Alternative sub-block-wise sequence
          strTmp = QString("  Non-Standard DQT Extension detected");
          m_pLog->AddLineWarn(strTmp);

          // FIXME: Should prevent attempt to decode until this is implemented

          if(nDqtPrecision_Pq == 0)
          {
            strDqtPrecision = "8 bits";
          }
          else if(nDqtPrecision_Pq == 1)
          {
            strDqtPrecision = "16 bits";
          }
          strTmp = QString("  Precision=%1"), strDqtPrecision;
          m_pLog->AddLine(strTmp);

          if((nDqtPrecision_Pq & 0x2) == 0)
          {
            strDqtZigZagOrder = "Diagonal zig-zag coeff scan seqeunce";
          }
          else if((nDqtPrecision_Pq & 0x2) == 1)
          {
            strDqtZigZagOrder = "Alternate coeff scan seqeunce";
          }

          strTmp = QString("  Coeff Scan Sequence=%s"), strDqtZigZagOrder;
          m_pLog->AddLine(strTmp);

          if((nDqtPrecision_Pq & 0x4) == 1)
          {
            strTmp = QString("  Custom coeff scan sequence");
            m_pLog->AddLine(strTmp);
            // Now expect sequence of 64 coefficient entries
            QString strSequence = "";

            for(uint32_t nInd = 0; nInd < 64; nInd++)
            {
              nTmpVal = Buf(m_nPos++);
              strTmp = QString("%u"), nTmpVal;
              strSequence += strTmp;

              if(nInd != 63)
              {
                strSequence += ", ";
              }
            }

            strTmp = QString("  Custom sequence = [ %s ]"), strSequence;
            m_pLog->AddLine(strTmp);
          }
        }
#endif
        strTmp = QString("  Destination ID=%1").arg(nDqtQuantDestId_Tq);

        if(nDqtQuantDestId_Tq == 0)
        {
          strTmp += " (Luminance)";
        }
        else if(nDqtQuantDestId_Tq == 1)
        {
          strTmp += " (Chrominance)";
        }
        else if(nDqtQuantDestId_Tq == 2)
        {
          strTmp += " (Chrominance)";
        }
        else
        {
          strTmp += " (???)";
        }

        m_pLog->AddLine(strTmp);

        // FIXME: The following is somewhat superseded by ValidateValue() above with the exception of skipping remainder
        if(nDqtQuantDestId_Tq >= MAX_DQT_DEST_ID)
        {
          m_pLog->AddLineErr(QString("ERROR: Destination ID <Tq> = %1, >= %2").arg(nDqtQuantDestId_Tq).arg(MAX_DQT_DEST_ID));

          if(!m_pAppConfig->relaxedParsing())
          {
            m_pLog->AddLineErr("  Stopping decode");
            return DECMARK_ERR;
          }
          else
          {
            // Now skip remainder of DQT
            // FIXME
            m_pLog->AddLineWarn(QString("  Skipping remainder of marker [%1 bytes]").arg(nPosMarkerStart + nLength - m_nPos));
            m_pLog->AddLine("");
            m_nPos = nPosMarkerStart + nLength;
            return DECMARK_OK;
          }
        }

        bool bQuantAllOnes = true;

        double dComparePercent;
        double dSumPercent = 0;
        double dSumPercentSqr = 0;

        for(uint32_t nCoeffInd = 0; nCoeffInd < MAX_DQT_COEFF; nCoeffInd++)
        {
          nTmpVal2 = Buf(m_nPos++);

          if(nDqtPrecision_Pq == 1)
          {
            // 16-bit DQT entries!
            nTmpVal2 <<= 8;
            nTmpVal2 += Buf(m_nPos++);
          }

          m_anImgDqtTbl[nDqtQuantDestId_Tq][glb_anZigZag[nCoeffInd]] = nTmpVal2;

          /* scaling factor in percent */

          // Now calculate the comparison with the Annex sample

          // FIXME: Should probably use check for landscape orientation and
          //        rotate comparison matrix accordingly

          if(nDqtQuantDestId_Tq == 0)
          {
            if(m_anImgDqtTbl[nDqtQuantDestId_Tq][glb_anZigZag[nCoeffInd]] != 0)
            {
              m_afStdQuantLumCompare[glb_anZigZag[nCoeffInd]] =
                static_cast<double>((glb_anStdQuantLum[glb_anZigZag[nCoeffInd]])) /
                static_cast<double>((m_anImgDqtTbl[nDqtQuantDestId_Tq][glb_anZigZag[nCoeffInd]]));
              dComparePercent = 100.0 *
                static_cast<double>((m_anImgDqtTbl[nDqtQuantDestId_Tq][glb_anZigZag[nCoeffInd]])) /
                static_cast<double>((glb_anStdQuantLum[glb_anZigZag[nCoeffInd]]));
            }
            else
            {
              m_afStdQuantLumCompare[glb_anZigZag[nCoeffInd]] = 999.99;
              dComparePercent = 999.99;
            }
          }
          else
          {
            if(m_anImgDqtTbl[nDqtQuantDestId_Tq][glb_anZigZag[nCoeffInd]] != 0)
            {
              m_afStdQuantChrCompare[glb_anZigZag[nCoeffInd]] =
                static_cast<double>((glb_anStdQuantChr[glb_anZigZag[nCoeffInd]])) /
                static_cast<double>((m_anImgDqtTbl[nDqtQuantDestId_Tq][glb_anZigZag[nCoeffInd]]));
              dComparePercent = 100.0 *
                static_cast<double>((m_anImgDqtTbl[nDqtQuantDestId_Tq][glb_anZigZag[nCoeffInd]])) /
                static_cast<double>((glb_anStdQuantChr[glb_anZigZag[nCoeffInd]]));
            }
            else
            {
              m_afStdQuantChrCompare[glb_anZigZag[nCoeffInd]] = 999.99;
            }
          }

          dSumPercent += dComparePercent;
          dSumPercentSqr += dComparePercent * dComparePercent;

          // Check just in case entire table are ones (Quality 100)
          if(m_anImgDqtTbl[nDqtQuantDestId_Tq][glb_anZigZag[nCoeffInd]] != 1)
            bQuantAllOnes = 0;
        }                       // 0..63

        // Note that the DQT table that we are saving is already
        // after doing zigzag reordering:
        // From high freq -> low freq
        // To X,Y, left-to-right, top-to-bottom

        // Flag this DQT table as being set!
        m_abImgDqtSet[nDqtQuantDestId_Tq] = true;

        uint32_t nCoeffInd;

        // Now display the table
        for(uint32_t nDqtY = 0; nDqtY < 8; nDqtY++)
        {
          strFull = QString("    DQT, Row #%1: ").arg(nDqtY);

          for(uint32_t nDqtX = 0; nDqtX < 8; nDqtX++)
          {
            nCoeffInd = nDqtY * 8 + nDqtX;
            strFull += QString("%1 ").arg(m_anImgDqtTbl[nDqtQuantDestId_Tq][nCoeffInd], 3);

            // Store the DQT entry into the Image Decoder
            bRet = m_pImgDec->SetDqtEntry(nDqtQuantDestId_Tq, nCoeffInd, glb_anUnZigZag[nCoeffInd],
                                          m_anImgDqtTbl[nDqtQuantDestId_Tq][nCoeffInd]);
            DecodeErrCheck(bRet);
          }

          // Now add the compare with Annex K
          // Decided to disable this as it was confusing users
          /*
             strFull += "   AnnexRatio: <";
             for (uint32_t nDqtX=0;nDqtX<8;nDqtX++) {
             nCoeffInd = nDqtY*8+nDqtX;
             if (nDqtQuantDestId_Tq == 0) {
             strTmp = QString("%5.1f "),m_afStdQuantLumCompare[nCoeffInd];
             } else {
             strTmp = QString("%5.1f "),m_afStdQuantChrCompare[nCoeffInd];
             }
             strFull += strTmp;
             }
             strFull += ">";
           */

          m_pLog->AddLine(strFull);
        }

        // Perform some statistical analysis of the quality factor
        // to determine the likelihood of the current quantization
        // table being a scaled version of the "standard" tables.
        // If the variance is high, it is unlikely to be the case.
        double dQuality;
        double dVariance;

        dSumPercent /= 64.0;    /* mean scale factor */
        dSumPercentSqr /= 64.0;
        dVariance = dSumPercentSqr - (dSumPercent * dSumPercent);       /* variance */

        // Generate the equivalent IJQ "quality" factor
        if(bQuantAllOnes)       /* special case for all-ones table */
          dQuality = 100.0;
        else if(dSumPercent <= 100.0)
          dQuality = (200.0 - dSumPercent) / 2.0;
        else
          dQuality = 5000.0 / dSumPercent;

        // Save the quality rating for later
        m_adImgDqtQual[nDqtQuantDestId_Tq] = dQuality;
        m_pLog->AddLine(QString("    Approx quality factor = %1 (scaling=%2 variance=%3)")
                        .arg(dQuality, 0, 'f', 2)
                        .arg(dSumPercent, 0, 'f', 2)
                        .arg(dVariance, 0, 'f', 2));
      }

      m_bStateDqtOk = true;

      if(!ExpectMarkerEnd(nPosMarkerStart, nLength))
        return DECMARK_ERR;

      break;

    case JFIF_DAC:             // DAC (Arithmetic Coding)
      nLength = Buf(m_nPos) * 256 + Buf(m_nPos + 1);    // La
      m_nPos += 2;
      strTmp = QString("  Arithmetic coding header length = %1").arg(nLength);
      m_pLog->AddLine(strTmp);

      uint32_t nDAC_n;
      uint32_t nDAC_Tc, nDAC_Tb;
      uint32_t nDAC_Cs;

      nDAC_n = (nLength > 2) ? (nLength - 2) / 2 : 0;

      for(uint32_t nInd = 0; nInd < nDAC_n; nInd++)
      {
        nTmpVal = Buf(m_nPos++);        // Tc,Tb
        nDAC_Tc = (nTmpVal & 0xF0) >> 4;
        nDAC_Tb = (nTmpVal & 0x0F);
        strTmp = QString("  #%1: Table class                  = %2")
            .arg(nInd + 1, 2, 10, QChar('0'))
            .arg(nDAC_Tc);
        m_pLog->AddLine(strTmp);
        strTmp = QString("  #%1: Table destination identifier = %2")
            .arg(nInd, 2, 10, QChar('0'))
            .arg(nDAC_Tb);
        m_pLog->AddLine(strTmp);

        nDAC_Cs = Buf(m_nPos++);        // Cs
        strTmp = QString("  #%1: Conditioning table value     = %2")
            .arg(nInd + 1, 2, 10, QChar('0'))
            .arg(nDAC_Cs);
        m_pLog->AddLine(strTmp);

        if(!ValidateValue(nDAC_Tc, 0, 1, "Table class <Tc>", true, 0))
          return DECMARK_ERR;
        if(!ValidateValue(nDAC_Tb, 0, 3, "Table destination ID <Tb>", true, 0))
          return DECMARK_ERR;

        // Parameter range constraints per Table B.6:
        // ------------|-------------------------|-------------------|------------
        //             |     Sequential DCT      |  Progressive DCT  | Lossless
        //   Parameter |  Baseline    Extended   |                   |
        // ------------|-----------|-------------|-------------------|------------
        //     Cs      |   Undef   | Tc=0: 0-255 | Tc=0: 0-255       | 0-255
        //             |           | Tc=1: 1-63  | Tc=1: 1-63        |
        // ------------|-----------|-------------|-------------------|------------

        // However, to keep it simple (and not depend on lossless mode),
        // we will only check the maximal range
        if(!ValidateValue(nDAC_Cs, 0, 255, "Conditioning table value <Cs>", true, 0))
          return DECMARK_ERR;
      }

      if(!ExpectMarkerEnd(nPosMarkerStart, nLength))
        return DECMARK_ERR;

      break;

    case JFIF_DNL:             // DNL (Define number of lines)
      nLength = Buf(m_nPos) * 256 + Buf(m_nPos + 1);    // Ld
      m_nPos += 2;
      strTmp = QString("  Header length = %1").arg(nLength);
      m_pLog->AddLine(strTmp);

      nTmpVal = Buf(m_nPos) * 256 + Buf(m_nPos + 1);    // NL
      m_nPos += 2;
      strTmp = QString("  Number of lines = %1").arg(nTmpVal);
      m_pLog->AddLine(strTmp);

      if(!ValidateValue(nTmpVal, 1, 65535, "Number of lines <NL>", true, 1))
        return DECMARK_ERR;

      if(!ExpectMarkerEnd(nPosMarkerStart, nLength))
        return DECMARK_ERR;

      break;

    case JFIF_EXP:
      nLength = Buf(m_nPos) * 256 + Buf(m_nPos + 1);    // Le
      m_nPos += 2;
      strTmp = QString("  Header length = %1").arg(nLength);
      m_pLog->AddLine(strTmp);

      uint32_t nEXP_Eh, nEXP_Ev;

      nTmpVal = Buf(m_nPos) * 256 + Buf(m_nPos + 1);    // Eh,Ev
      nEXP_Eh = (nTmpVal & 0xF0) >> 4;
      nEXP_Ev = (nTmpVal & 0x0F);
      m_nPos += 2;
      strTmp = QString("  Expand horizontally = %1").arg(nEXP_Eh);
      m_pLog->AddLine(strTmp);
      strTmp = QString("  Expand vertically   = %1").arg(nEXP_Ev);
      m_pLog->AddLine(strTmp);

      if(!ValidateValue(nEXP_Eh, 0, 1, "Expand horizontally <Eh>", true, 0))
        return DECMARK_ERR;

      if(!ValidateValue(nEXP_Ev, 0, 1, "Expand vertically <Ev>", true, 0))
        return DECMARK_ERR;

      if(!ExpectMarkerEnd(nPosMarkerStart, nLength))
        return DECMARK_ERR;
      break;

    case JFIF_SOF0:            // SOF0 (Baseline DCT)
    case JFIF_SOF1:            // SOF1 (Extended sequential)
    case JFIF_SOF2:            // SOF2 (Progressive)
    case JFIF_SOF3:
    case JFIF_SOF5:
    case JFIF_SOF6:
    case JFIF_SOF7:
    case JFIF_SOF9:
    case JFIF_SOF10:
    case JFIF_SOF11:
    case JFIF_SOF13:
    case JFIF_SOF14:
    case JFIF_SOF15:

      // TODO:
      // - JFIF_DHP should be able to reuse the JFIF_SOF marker parsing
      //   however as we don't support hierarchical image decode, we
      //   would want to skip the update of class members.

      m_bStateSof = true;

      // Determine if this is a SOF mode that we support
      // At this time, we only support Baseline DCT & Extended Sequential Baseline DCT
      // (non-differential) with Huffman coding. Progressive, Lossless,
      // Differential and Arithmetic coded modes are not supported.
      m_bImgSofUnsupported = true;

      if(nCode == JFIF_SOF0)
      {
        m_bImgSofUnsupported = false;
      }

      if(nCode == JFIF_SOF1)
      {
        m_bImgSofUnsupported = false;
      }

      // For reference, note progressive scan files even though
      // we don't currently support their decode
      if(nCode == JFIF_SOF2)
      {
        m_bImgProgressive = true;
      }

      nLength = Buf(m_nPos) * 256 + Buf(m_nPos + 1);    // Lf
      m_nPos += 2;
      strTmp = QString("  Frame header length = %1").arg(nLength);
      m_pLog->AddLine(strTmp);

      m_nSofPrecision_P = Buf(m_nPos++);        // P
      strTmp = QString("  Precision = %1").arg(m_nSofPrecision_P);
      m_pLog->AddLine(strTmp);

      if(!ValidateValue(m_nSofPrecision_P, 2, 16, "Precision <P>", true, 8))
        return DECMARK_ERR;

      m_nSofNumLines_Y = Buf(m_nPos) * 256 + Buf(m_nPos + 1);   // Y
      m_nPos += 2;
      strTmp = QString("  Number of Lines = %1").arg(m_nSofNumLines_Y);
      m_pLog->AddLine(strTmp);

      if(!ValidateValue(m_nSofNumLines_Y, 0, 65535, "Number of Lines <Y>", true, 0))
        return DECMARK_ERR;

      m_nSofSampsPerLine_X = Buf(m_nPos) * 256 + Buf(m_nPos + 1);       // X
      m_nPos += 2;
      strTmp = QString("  Samples per Line = %1").arg(m_nSofSampsPerLine_X);
      m_pLog->AddLine(strTmp);

      if(!ValidateValue(m_nSofSampsPerLine_X, 1, 65535, "Samples per Line <X>", true, 1))
        return DECMARK_ERR;

      strTmp = QString("  Image Size = %1 x %2").arg(m_nSofSampsPerLine_X).arg(m_nSofNumLines_Y);
      m_pLog->AddLine(strTmp);

      // Determine orientation
      //   m_nSofSampsPerLine_X = X
      //   m_nSofNumLines_Y = Y
      m_eImgLandscape = ENUM_LANDSCAPE_YES;

      if(m_nSofNumLines_Y > m_nSofSampsPerLine_X)
        m_eImgLandscape = ENUM_LANDSCAPE_NO;

      strTmp = QString("  Raw Image Orientation = %1").arg(m_eImgLandscape == ENUM_LANDSCAPE_YES ? "Landscape" : "Portrait");
      m_pLog->AddLine(strTmp);

      m_nSofNumComps_Nf = Buf(m_nPos++);        // Nf, range 1..255
      strTmp = QString("  Number of Img components = %1").arg(m_nSofNumComps_Nf);
      m_pLog->AddLine(strTmp);

      if(!ValidateValue(m_nSofNumComps_Nf, 1, 255, "Number of Img components <Nf>", true, 1))
        return DECMARK_ERR;

      uint32_t nCompIdent;
      uint32_t anSofSampFact[MAX_SOF_COMP_NF];

      m_nSofHorzSampFactMax_Hmax = 0;
      m_nSofVertSampFactMax_Vmax = 0;

      // Now clear the output image content (all components)
      // TODO: Migrate some of the bitmap allocation / clearing from
      // DecodeScanImg() into ResetImageContent() and call here
      //m_pImgDec->ResetImageContent();

      // Per JFIF v1.02:
      // - Nf = 1 or 3
      // - C1 = Y
      // - C2 = Cb
      // - C3 = Cr

      for(uint32_t nCompInd = 1; ((!m_bStateAbort) && (nCompInd <= m_nSofNumComps_Nf)); nCompInd++)
      {
        nCompIdent = Buf(m_nPos++);     // Ci, range 0..255
        m_anSofQuantCompId[nCompInd] = nCompIdent;

        //if (!ValidateValue(m_anSofQuantCompId[nCompInd],0,255,"Component ID <Ci>"),true,0) return DECMARK_ERR;

        anSofSampFact[nCompIdent] = Buf(m_nPos++);
        m_anSofQuantTblSel_Tqi[nCompIdent] = Buf(m_nPos++);     // Tqi, range 0..3

        //if (!ValidateValue(m_anSofQuantTblSel_Tqi[nCompIdent],0,3,"Table Destination ID <Tqi>"),true,0) return DECMARK_ERR;

        // NOTE: We protect against bad input here as replication ratios are
        // determined later that depend on dividing by sampling factor (hence
        // possibility of div by 0).
        m_anSofHorzSampFact_Hi[nCompIdent] = (anSofSampFact[nCompIdent] & 0xF0) >> 4;   // Hi, range 1..4
        m_anSofVertSampFact_Vi[nCompIdent] = (anSofSampFact[nCompIdent] & 0x0F);        // Vi, range 1..4

        if(!ValidateValue(m_anSofHorzSampFact_Hi[nCompIdent], 1, 4, "Horizontal Sampling Factor <Hi>", true, 1))
          return DECMARK_ERR;

        if(!ValidateValue(m_anSofVertSampFact_Vi[nCompIdent], 1, 4, "Vertical Sampling Factor <Vi>", true, 1))
          return DECMARK_ERR;
      }

      // Calculate max sampling factors
      for(uint32_t nCompInd = 1; ((!m_bStateAbort) && (nCompInd <= m_nSofNumComps_Nf)); nCompInd++)
      {
        nCompIdent = m_anSofQuantCompId[nCompInd];
        // Calculate maximum sampling factor for the SOF. This is only
        // used for later generation of m_strImgQuantCss an the SOF
        // reporting below. The CimgDecode block is responsible for
        // calculating the maximum sampling factor on a per-scan basis.
        m_nSofHorzSampFactMax_Hmax = qMax(m_nSofHorzSampFactMax_Hmax, m_anSofHorzSampFact_Hi[nCompIdent]);
        m_nSofVertSampFactMax_Vmax = qMax(m_nSofVertSampFactMax_Vmax, m_anSofVertSampFact_Vi[nCompIdent]);
      }

      // Report per-component sampling factors and quantization table selectors
      for(uint32_t nCompInd = 1; ((!m_bStateAbort) && (nCompInd <= m_nSofNumComps_Nf)); nCompInd++)
      {
        nCompIdent = m_anSofQuantCompId[nCompInd];

        // Create subsampling ratio
        // - Protect against division-by-zero
        QString strSubsampH = "?";
        QString strSubsampV = "?";

        if(m_anSofHorzSampFact_Hi[nCompIdent] > 0)
        {
          strSubsampH = QString("%1").arg(m_nSofHorzSampFactMax_Hmax / m_anSofHorzSampFact_Hi[nCompIdent]);
        }

        if(m_anSofVertSampFact_Vi[nCompIdent] > 0)
        {
          strSubsampV = QString("%1").arg(m_nSofVertSampFactMax_Vmax / m_anSofVertSampFact_Vi[nCompIdent]);
        }

        strFull = QString("    Component[%1]: ").arg(nCompInd); // Note i in Ci is 1-based
        strTmp = QString("ID=0x%1, Samp Fac=0x%2 (Subsamp %3 x %4), Quant Tbl Sel=0x%5")
            .arg(nCompIdent, 2, 16, QChar('0'))
            .arg(anSofSampFact[nCompIdent], 2, 16, QChar('0'))
            .arg(strSubsampH)
            .arg(strSubsampV)
            .arg(m_anSofQuantTblSel_Tqi[nCompIdent], 2, 16, QChar('0'));
        strFull += strTmp;

        // Mapping from component index (not ID) to colour channel per JFIF
        if(m_nSofNumComps_Nf == 1)
        {
          // Assume grayscale
          strFull += " (Lum: Y)";
        }
        else if(m_nSofNumComps_Nf == 3)
        {
          // Assume YCC
          if(nCompInd == SCAN_COMP_Y)
          {
            strFull += " (Lum: Y)";
          }
          else if(nCompInd == SCAN_COMP_CB)
          {
            strFull += " (Chrom: Cb)";
          }
          else if(nCompInd == SCAN_COMP_CR)
          {
            strFull += " (Chrom: Cr)";
          }
        }
        else if(m_nSofNumComps_Nf == 4)
        {
          // Assume YCCK
          if(nCompInd == 1)
          {
            strFull += " (Y)";
          }
          else if(nCompInd == 2)
          {
            strFull += " (Cb)";
          }
          else if(nCompInd == 3)
          {
            strFull += " (Cr)";
          }
          else if(nCompInd == 4)
          {
            strFull += " (K)";
          }
        }
        else
        {
          strFull += " (???)";  // Unknown
        }

        m_pLog->AddLine(strFull);
      }

      // Test for bad input, clean up if bad
      for(uint32_t nCompInd = 1; ((!m_bStateAbort) && (nCompInd <= m_nSofNumComps_Nf)); nCompInd++)
      {
        nCompIdent = m_anSofQuantCompId[nCompInd];

        if(!ValidateValue(m_anSofQuantCompId[nCompInd], 0, 255, "Component ID <Ci>", true, 0))
          return DECMARK_ERR;

        if(!ValidateValue(m_anSofQuantTblSel_Tqi[nCompIdent], 0, 3, "Table Destination ID <Tqi>", true, 0))
          return DECMARK_ERR;

        if(!ValidateValue(m_anSofHorzSampFact_Hi[nCompIdent], 1, 4, "Horizontal Sampling Factor <Hi>", true, 1))
          return DECMARK_ERR;

        if(!ValidateValue(m_anSofVertSampFact_Vi[nCompIdent], 1, 4, "Vertical Sampling Factor <Vi>", true, 1))
          return DECMARK_ERR;
      }

      // Finally, assign the cleaned values to the decoder
      for(uint32_t nCompInd = 1; ((!m_bStateAbort) && (nCompInd <= m_nSofNumComps_Nf)); nCompInd++)
      {
        nCompIdent = m_anSofQuantCompId[nCompInd];
        // Store the DQT Table selection for the Image Decoder
        //   Param values: Nf,Tqi
        //   Param ranges: 1..255,0..3
        // Note that the Image Decoder doesn't need to see the Component Identifiers
        bRet = m_pImgDec->SetDqtTables(nCompInd, m_anSofQuantTblSel_Tqi[nCompIdent]);
        DecodeErrCheck(bRet);

        // Store the Precision (to handle 12-bit decode)
        m_pImgDec->SetPrecision(m_nSofPrecision_P);
      }

      if(!m_bStateAbort)
      {
        // Set the component sampling factors (chroma subsampling)
        // FIXME: check ranging
        for(uint32_t nCompInd = 1; nCompInd <= m_nSofNumComps_Nf; nCompInd++)
        {
          // nCompInd is component index (1...Nf)
          // nCompIdent is Component Identifier (Ci)
          // Note that the Image Decoder doesn't need to see the Component Identifiers
          nCompIdent = m_anSofQuantCompId[nCompInd];
          m_pImgDec->SetSofSampFactors(nCompInd, m_anSofHorzSampFact_Hi[nCompIdent], m_anSofVertSampFact_Vi[nCompIdent]);
        }

        // Now mark the image as been somewhat OK (ie. should
        // also be suitable for EmbeddedThumb() and PrepareSignature()
        m_bImgOK = true;

        m_bStateSofOk = true;
      }

      if(!ExpectMarkerEnd(nPosMarkerStart, nLength))
        return DECMARK_ERR;

      break;

    case JFIF_COM:             // COM
      nLength = Buf(m_nPos) * 256 + Buf(m_nPos + 1);
      m_nPos += 2;
      strTmp = QString("  Comment length = %1").arg(nLength);
      m_pLog->AddLine(strTmp);

      // Check for JPEG COM vulnerability
      //   http://marc.info/?l=bugtraq&m=109524346729948
      // Note that the recovery is not very graceful. It will assume that the
      // field is actually zero-length, which will make the next byte trigger the
      // "Expected marker 0xFF" error message and probably abort. There is no
      // obvious way to

      if((nLength == 0) || (nLength == 1))
      {
        strTmp = QString("    JPEG Comment Field Vulnerability detected!");
        m_pLog->AddLineErr(strTmp);
        strTmp = QString("    Skipping data until next marker...");
        m_pLog->AddLineErr(strTmp);
        nLength = 2;

        bool bDoneSearch = false;

        uint32_t nSkipStart = m_nPos;

        while(!bDoneSearch)
        {
          if(Buf(m_nPos) != 0xFF)
          {
            m_nPos++;
          }
          else
          {
            bDoneSearch = true;
          }

          if(m_nPos >= m_pWBuf->GetPosEof())
          {
            bDoneSearch = true;
          }
        }

        strTmp = QString("    Skipped %1 bytes").arg(m_nPos - nSkipStart);
        m_pLog->AddLineErr(strTmp);

        // Break out of case statement
        break;
      }

      // Assume COM field valid length (ie. >= 2)
      strFull = "    Comment=";
      m_strComment = "";

      for(uint32_t ind = 0; ind < nLength - 2; ind++)
      {
        nTmpVal1 = Buf(m_nPos++);

        if(isprint(nTmpVal1))
        {
          strTmp = QString("%1").arg(nTmpVal1);
          m_strComment += strTmp;
        }
        else
        {
          m_strComment += ".";
        }
      }

      strFull += m_strComment;
      m_pLog->AddLine(strFull);

      break;

    case JFIF_DHT:             // DHT
      m_bStateDht = true;
      DecodeDHT(false);
      m_bStateDhtOk = true;
      break;

    case JFIF_SOS:             // SOS
      uint32_t nPosScanStart;      // Byte count at start of scan data segment

      m_bStateSos = true;

      // NOTE: Only want to capture position of first SOS
      //       This should make other function such as AVI frame extract
      //       more robust in case we get multiple SOS segments.
      // We assume that this value is reset when we start a new decode
      if(m_nPosSos == 0)
      {
        m_nPosSos = m_nPos - 2; // Used for Extract. Want to include actual marker
      }

      nLength = Buf(m_nPos) * 256 + Buf(m_nPos + 1);
      m_nPos += 2;

      // Ensure that we have seen proper markers before we try this one!
      if(!m_bStateSofOk)
      {
        strTmp = QString("  ERROR: SOS before valid SOF defined");
        m_pLog->AddLineErr(strTmp);
        return DECMARK_ERR;
      }

      strTmp = QString("  Scan header length = %1").arg(nLength);
      m_pLog->AddLine(strTmp);

      m_nSosNumCompScan_Ns = Buf(m_nPos++);     // Ns, range 1..4
      strTmp = QString("  Number of img components = %1").arg(m_nSosNumCompScan_Ns);
      m_pLog->AddLine(strTmp);

      // Just in case something got corrupted, don't want to get out
      // of range here. Note that this will be a hard abort, and
      // will not resume decoding.
      if(m_nSosNumCompScan_Ns > MAX_SOS_COMP_NS)
      {
        strTmp = QString("  ERROR: Scan decode does not support > %1 components").arg(MAX_SOS_COMP_NS);
        m_pLog->AddLineErr(strTmp);
        return DECMARK_ERR;
      }

      uint32_t nSosCompSel_Cs;
      uint32_t nSosHuffTblSel;
      uint32_t nSosHuffTblSelDc_Td;
      uint32_t nSosHuffTblSelAc_Ta;

      // Max range of components indices is between 1..4
      for(uint32_t nScanCompInd = 1; ((nScanCompInd <= m_nSosNumCompScan_Ns) && (!m_bStateAbort)); nScanCompInd++)
      {
        strFull = QString("    Component[%1]: ").arg(nScanCompInd);
        nSosCompSel_Cs = Buf(m_nPos++); // Cs, range 0..255
        nSosHuffTblSel = Buf(m_nPos++);
        nSosHuffTblSelDc_Td = (nSosHuffTblSel & 0xf0) >> 4;     // Td, range 0..3
        nSosHuffTblSelAc_Ta = (nSosHuffTblSel & 0x0f);  // Ta, range 0..3
        strTmp = QString("selector=0x%1, table=%2(DC),%3(AC)")
            .arg(nSosCompSel_Cs, 2, 16, QChar('0'))
            .arg(nSosHuffTblSelDc_Td)
            .arg(nSosHuffTblSelAc_Ta);
        strFull += strTmp;
        m_pLog->AddLine(strFull);

        bRet = m_pImgDec->SetDhtTables(nScanCompInd, nSosHuffTblSelDc_Td, nSosHuffTblSelAc_Ta);

        DecodeErrCheck(bRet);
      }

      m_nSosSpectralStart_Ss = Buf(m_nPos++);
      m_nSosSpectralEnd_Se = Buf(m_nPos++);
      m_nSosSuccApprox_A = Buf(m_nPos++);

      strTmp = QString("  Spectral selection = %1 .. %2")
          .arg(m_nSosSpectralStart_Ss)
          .arg(m_nSosSpectralEnd_Se);
      m_pLog->AddLine(strTmp);
      strTmp = QString("  Successive approximation = 0x%1").arg(m_nSosSuccApprox_A, 2, 16, QChar('0'));
      m_pLog->AddLine(strTmp);

      if(m_pAppConfig->scanDump())
      {
        m_pLog->AddLine("");
        m_pLog->AddLine("  Scan Data: (after bitstuff removed)");
      }

      // Save the scan data segment position
      nPosScanStart = m_nPos;

      // Skip over the Scan Data segment
      //   Pass 1) Quick, allowing for bOutputScanDump to dump first 640B.
      //   Pass 2) If bDecodeScanImg, we redo the process but in detail decoding.

      // FIXME: Not sure why, but if I skip over Pass 1 (eg if I leave in the
      // following line uncommented), then I get an error at the end of the
      // pass 2 decode (indicating that EOI marker not seen, and expecting
      // marker).
//              if (m_pAppConfig->bOutputScanDump) {

      // --- PASS 1 ---
      bool bSkipDone;

      uint32_t nSkipCount;
      uint32_t nSkipData;
      uint32_t nSkipPos;

      bool bScanDumpTrunc;

      bSkipDone = false;
      nSkipCount = 0;
      nSkipPos = 0;
      bScanDumpTrunc = false;

      strFull = "";

      while(!bSkipDone)
      {
        nSkipCount++;
        nSkipPos++;
        nSkipData = Buf(m_nPos++);

        if(nSkipData == 0xFF)
        {
          // this could either be a marker or a byte stuff
          nSkipData = Buf(m_nPos++);
          nSkipCount++;

          if(nSkipData == 0x00)
          {
            // Byte stuff
            nSkipData = 0xFF;
          }
          else if((nSkipData >= JFIF_RST0) && (nSkipData <= JFIF_RST7))
          {
            // Skip over
          }
          else
          {
            // Marker
            bSkipDone = true;
            m_nPos -= 2;
          }
        }

        if(m_pAppConfig->scanDump() && (!bSkipDone))
        {
          // Only display 20 lines of scan data
          if(nSkipPos > 640)
          {
            if(!bScanDumpTrunc)
            {
              m_pLog->AddLineWarn("    WARNING: Dump truncated.");
              bScanDumpTrunc = true;
            }
          }
          else
          {
            if(((nSkipPos - 1) == 0) || (((nSkipPos - 1) % 32) == 0))
            {
              strFull = "    ";
            }

            strTmp = QString("%1 ").arg(nSkipData, 2, 16, QChar('0'));
            strFull += strTmp;

            if(((nSkipPos - 1) % 32) == 31)
            {
              m_pLog->AddLine(strFull);
              strFull = "";
            }
          }
        }

        // Did we run out of bytes?

        // FIXME:
        // NOTE: This line here doesn't allow us to attempt to
        // decode images that are missing EOI. Maybe this is
        // not the best solution here? Instead, we should be
        // checking m_nPos against file length? .. and not
        // return but "break".
        if(!m_pWBuf->GetBufOk())
        {
          strTmp = QString("ERROR: Ran out of buffer before EOI during phase 1 of Scan decode @ 0x%1").arg(m_nPos, 8, 16, QChar('0'));
          m_pLog->AddLineErr(strTmp);
          break;
        }

      }

      m_pLog->AddLine(strFull);

//              }

      // --- PASS 2 ---
      // If the option is set, start parsing!
      if(m_pAppConfig->decodeImage() && m_bImgSofUnsupported)
      {
        // SOF marker was of type we don't support, so skip decoding
        m_pLog->AddLineWarn("  NOTE: Scan parsing doesn't support this SOF mode.");
#ifndef DEBUG_YCCK
      }
      else if(m_pAppConfig->decodeImage() && (m_nSofNumComps_Nf == 4))
      {
        m_pLog->AddLineWarn("  NOTE: Scan parsing doesn't support CMYK files yet.");
#endif
      }
      else if(m_pAppConfig->decodeImage() && !m_bImgSofUnsupported)
      {
        if(!m_bStateSofOk)
        {
          m_pLog->AddLineWarn("  NOTE: Scan decode disabled as SOF not decoded.");
        }
        else if(!m_bStateDqtOk)
        {
          m_pLog->AddLineWarn("  NOTE: Scan decode disabled as DQT not decoded.");
        }
        else if(!m_bStateDhtOk)
        {
          m_pLog->AddLineWarn("  NOTE: Scan decode disabled as DHT not decoded.");
        }
        else
        {
          m_pLog->AddLine("");

          // Set the primary image details
          m_pImgDec->SetImageDetails(m_nSofSampsPerLine_X, m_nSofNumLines_Y,
                                     m_nSofNumComps_Nf, m_nSosNumCompScan_Ns, m_nImgRstEn, m_nImgRstInterval);

          // Only recalculate the scan decoding if we need to (i.e. file
          // changed, offset changed, scan option changed)
          // TODO: In order to decode multiple scans, we will need to alter the
          // way that m_pImgSrcDirty is set
          if(m_pImgSrcDirty)
          {
            m_pImgDec->DecodeScanImg(nPosScanStart, true, false);
            m_pImgSrcDirty = false;
          }
        }
      }

      m_bStateSosOk = true;

      break;

    case JFIF_DRI:
      uint32_t nVal;

      nLength = Buf(m_nPos) * 256 + Buf(m_nPos + 1);
      strTmp = QString("  Length     = %1").arg(nLength);
      m_pLog->AddLine(strTmp);
      nVal = Buf(m_nPos + 2) * 256 + Buf(m_nPos + 3);

      // According to ITU-T spec B.2.4.4, we only expect
      // restart markers if DRI value is non-zero!
      m_nImgRstInterval = nVal;
      if(nVal != 0)
      {
        m_nImgRstEn = true;
      }
      else
      {
        m_nImgRstEn = false;
      }

      strTmp = QString("  interval   = %1").arg(m_nImgRstInterval);
      m_pLog->AddLine(strTmp);
      m_nPos += 4;

      if(!ExpectMarkerEnd(nPosMarkerStart, nLength))
        return DECMARK_ERR;

        break;

    case JFIF_EOI:             // EOI
      m_pLog->AddLine("");

      // Save the EOI file position
      // NOTE: If the file is missing the EOI, then this variable will be
      //       set to mark the end of file.
      m_nPosEmbedEnd = m_nPos;
      m_nPosEoi = m_nPos;
      m_bStateEoi = true;

      return DECMARK_EOI;

      break;

      // Markers that are not yet supported in JPEGsnoop
    case JFIF_DHP:
      // Markers defined for future use / extensions
    case JFIF_JPG:
    case JFIF_JPG0:
    case JFIF_JPG1:
    case JFIF_JPG2:
    case JFIF_JPG3:
    case JFIF_JPG4:
    case JFIF_JPG5:
    case JFIF_JPG6:
    case JFIF_JPG7:
    case JFIF_JPG8:
    case JFIF_JPG9:
    case JFIF_JPG10:
    case JFIF_JPG11:
    case JFIF_JPG12:
    case JFIF_JPG13:
    case JFIF_TEM:
      // Unsupported marker
      // - Provide generic decode based on length
      nLength = Buf(m_nPos) * 256 + Buf(m_nPos + 1);    // Length
      strTmp = QString("  Header length = %1").arg(nLength);
      m_pLog->AddLine(strTmp);
      m_pLog->AddLineWarn("  Skipping unsupported marker");
      m_nPos += nLength;
      break;

    case JFIF_RST0:
    case JFIF_RST1:
    case JFIF_RST2:
    case JFIF_RST3:
    case JFIF_RST4:
    case JFIF_RST5:
    case JFIF_RST6:
    case JFIF_RST7:
      // We don't expect to see restart markers outside the entropy coded segment.
      // NOTE: RST# are standalone markers, so no length indicator exists
      // But for the sake of robustness, we can check here to see if treating
      // as a standalone marker will arrive at another marker (ie. OK). If not,
      // proceed to assume there is a length indicator.
      strTmp = QString("  WARNING: Restart marker [0xFF%1] detected outside scan").arg(nCode, 2, 16, QChar('0'));
      m_pLog->AddLineWarn(strTmp);

      if(!m_pAppConfig->relaxedParsing())
      {
        // Abort
        m_pLog->AddLineErr("  Stopping decode");
        m_pLog->AddLine("  Use [Img Search Fwd/Rev] to locate other valid embedded JPEGs");
        return DECMARK_ERR;
      }
      else
      {
        // Ignore
        // Check to see if standalone marker treatment looks OK
        if(Buf(m_nPos + 2) == 0xFF)
        {
          // Looks like standalone
          m_pLog->AddLineWarn("  Ignoring standalone marker. Proceeding with decode.");
          m_nPos += 2;
        }
        else
        {
          // Looks like marker with length

          nLength = Buf(m_nPos) * 256 + Buf(m_nPos + 1);
          strTmp = QString("  Header length = %1").arg(nLength);
          m_pLog->AddLine(strTmp);
          m_pLog->AddLineWarn("  Skipping marker");
          m_nPos += nLength;
        }
      }
      break;

    default:
      strTmp = QString("  WARNING: Unknown marker [0xFF%1]").arg(nCode, 2, 16, QChar('0'));
      m_pLog->AddLineWarn(strTmp);

      if(!m_pAppConfig->relaxedParsing())
      {
        // Abort
        m_pLog->AddLineErr("  Stopping decode");
        m_pLog->AddLine("  Use [Img Search Fwd/Rev] to locate other valid embedded JPEGs");
        return DECMARK_ERR;
      }
      else
      {
        // Skip
        nLength = Buf(m_nPos) * 256 + Buf(m_nPos + 1);
        strTmp = QString("  Header length = %1").arg(nLength);
        m_pLog->AddLine(strTmp);
        m_pLog->AddLineWarn("  Skipping marker");
        m_nPos += nLength;
      }
  }

  // Add white-space between each marker
  m_pLog->AddLine(" ");

  // If we decided to abort for any reason, make sure we trap it now.
  // This will stop the ProcessFile() while loop. We can set m_bStateAbort
  // if user says that they want to stop.
  if(m_bStateAbort)
  {
    return DECMARK_ERR;
  }

  return DECMARK_OK;
}

//-----------------------------------------------------------------------------
// Print out a header for the current JFIF marker code
void CjfifDecode::AddHeader(uint32_t nCode)
{
  QString strTmp;

  switch (nCode)
  {
    case JFIF_SOI:
      m_pLog->AddLineHdr("*** Marker: SOI (xFFD8) ***");
      break;

    case JFIF_APP0:
      m_pLog->AddLineHdr("*** Marker: APP0 (xFFE0) ***");
      break;

    case JFIF_APP1:
      m_pLog->AddLineHdr("*** Marker: APP1 (xFFE1) ***");
      break;

    case JFIF_APP2:
      m_pLog->AddLineHdr("*** Marker: APP2 (xFFE2) ***");
      break;

    case JFIF_APP3:
      m_pLog->AddLineHdr("*** Marker: APP3 (xFFE3) ***");
      break;

    case JFIF_APP4:
      m_pLog->AddLineHdr("*** Marker: APP4 (xFFE4) ***");
      break;

    case JFIF_APP5:
      m_pLog->AddLineHdr("*** Marker: APP5 (xFFE5) ***");
      break;

    case JFIF_APP6:
      m_pLog->AddLineHdr("*** Marker: APP6 (xFFE6) ***");
      break;

    case JFIF_APP7:
      m_pLog->AddLineHdr("*** Marker: APP7 (xFFE7) ***");
      break;

    case JFIF_APP8:
      m_pLog->AddLineHdr("*** Marker: APP8 (xFFE8) ***");
      break;

    case JFIF_APP9:
      m_pLog->AddLineHdr("*** Marker: APP9 (xFFE9) ***");
      break;

    case JFIF_APP10:
      m_pLog->AddLineHdr("*** Marker: APP10 (xFFEA) ***");
      break;

    case JFIF_APP11:
      m_pLog->AddLineHdr("*** Marker: APP11 (xFFEB) ***");
      break;

    case JFIF_APP12:
      m_pLog->AddLineHdr("*** Marker: APP12 (xFFEC) ***");
      break;

    case JFIF_APP13:
      m_pLog->AddLineHdr("*** Marker: APP13 (xFFED) ***");
      break;

    case JFIF_APP14:
      m_pLog->AddLineHdr("*** Marker: APP14 (xFFEE) ***");
      break;

    case JFIF_APP15:
      m_pLog->AddLineHdr("*** Marker: APP15 (xFFEF) ***");
      break;

    case JFIF_SOF0:
      m_pLog->AddLineHdr("*** Marker: SOF0 (Baseline DCT) (xFFC0) ***");
      break;

    case JFIF_SOF1:
      m_pLog->AddLineHdr("*** Marker: SOF1 (Extended Sequential DCT, Huffman) (xFFC1) ***");
      break;

    case JFIF_SOF2:
      m_pLog->AddLineHdr("*** Marker: SOF2 (Progressive DCT, Huffman) (xFFC2) ***");
      break;

    case JFIF_SOF3:
      m_pLog->AddLineHdr("*** Marker: SOF3 (Lossless Process, Huffman) (xFFC3) ***");
      break;

    case JFIF_SOF5:
      m_pLog->AddLineHdr("*** Marker: SOF5 (Differential Sequential DCT, Huffman) (xFFC4) ***");
      break;

    case JFIF_SOF6:
      m_pLog->AddLineHdr("*** Marker: SOF6 (Differential Progressive DCT, Huffman) (xFFC5) ***");
      break;

    case JFIF_SOF7:
      m_pLog->AddLineHdr("*** Marker: SOF7 (Differential Lossless Process, Huffman) (xFFC6) ***");
      break;

    case JFIF_SOF9:
      m_pLog->AddLineHdr("*** Marker: SOF9 (Sequential DCT, Arithmetic) (xFFC9) ***");
      break;

    case JFIF_SOF10:
      m_pLog->AddLineHdr("*** Marker: SOF10 (Progressive DCT, Arithmetic) (xFFCA) ***");
      break;

    case JFIF_SOF11:
      m_pLog->AddLineHdr("*** Marker: SOF11 (Lossless Process, Arithmetic) (xFFCB) ***");
      break;

    case JFIF_SOF13:
      m_pLog->AddLineHdr("*** Marker: SOF13 (Differential Sequential, Arithmetic) (xFFCD) ***");
      break;

    case JFIF_SOF14:
      m_pLog->AddLineHdr("*** Marker: SOF14 (Differential Progressive DCT, Arithmetic) (xFFCE) ***");
      break;

    case JFIF_SOF15:
      m_pLog->AddLineHdr("*** Marker: SOF15 (Differential Lossless Process, Arithmetic) (xFFCF) ***");
      break;

    case JFIF_JPG:
      m_pLog->AddLineHdr("*** Marker: JPG (xFFC8) ***");
      break;

    case JFIF_DAC:
      m_pLog->AddLineHdr("*** Marker: DAC (xFFCC) ***");
      break;

    case JFIF_RST0:
    case JFIF_RST1:
    case JFIF_RST2:
    case JFIF_RST3:
    case JFIF_RST4:
    case JFIF_RST5:
    case JFIF_RST6:
    case JFIF_RST7:
      m_pLog->AddLineHdr("*** Marker: RST# ***");
      break;

    case JFIF_DQT:             // Define quantization tables
      m_pLog->AddLineHdr("*** Marker: DQT (xFFDB) ***");
      m_pLog->AddLineHdrDesc("  Define a Quantization Table.");
      break;

    case JFIF_COM:             // COM
      m_pLog->AddLineHdr("*** Marker: COM (Comment) (xFFFE) ***");
      break;

    case JFIF_DHT:             // DHT
      m_pLog->AddLineHdr("*** Marker: DHT (Define Huffman Table) (xFFC4) ***");
      break;

    case JFIF_DHT_FAKE:        // DHT from standard table (MotionJPEG)
      m_pLog->AddLineHdr("*** Marker: DHT from MotionJPEG standard (Define Huffman Table) ***");
      break;

    case JFIF_SOS:             // SOS
      m_pLog->AddLineHdr("*** Marker: SOS (Start of Scan) (xFFDA) ***");
      break;

    case JFIF_DRI:             // DRI
      m_pLog->AddLineHdr("*** Marker: DRI (Restart Interval) (xFFDD) ***");
      break;

    case JFIF_EOI:             // EOI
      m_pLog->AddLineHdr("*** Marker: EOI (End of Image) (xFFD9) ***");
      break;

    case JFIF_DNL:
      m_pLog->AddLineHdr("*** Marker: DNL (Define Number of Lines) (xFFDC) ***");
      break;
    case JFIF_DHP:
      m_pLog->AddLineHdr("*** Marker: DHP (Define Hierarchical Progression) (xFFDE) ***");
      break;
    case JFIF_EXP:
      m_pLog->AddLineHdr("*** Marker: EXP (Expand Reference Components) (xFFDF) ***");
      break;
    case JFIF_JPG0:
      m_pLog->AddLineHdr("*** Marker: JPG0 (JPEG Extension) (xFFF0) ***");
      break;
    case JFIF_JPG1:
      m_pLog->AddLineHdr("*** Marker: JPG1 (JPEG Extension) (xFFF1) ***");
      break;
    case JFIF_JPG2:
      m_pLog->AddLineHdr("*** Marker: JPG2 (JPEG Extension) (xFFF2) ***");
      break;
    case JFIF_JPG3:
      m_pLog->AddLineHdr("*** Marker: JPG3 (JPEG Extension) (xFFF3) ***");
      break;
    case JFIF_JPG4:
      m_pLog->AddLineHdr("*** Marker: JPG4 (JPEG Extension) (xFFF4) ***");
      break;
    case JFIF_JPG5:
      m_pLog->AddLineHdr("*** Marker: JPG5 (JPEG Extension) (xFFF5) ***");
      break;
    case JFIF_JPG6:
      m_pLog->AddLineHdr("*** Marker: JPG6 (JPEG Extension) (xFFF6) ***");
      break;
    case JFIF_JPG7:
      m_pLog->AddLineHdr("*** Marker: JPG7 (JPEG Extension) (xFFF7) ***");
      break;
    case JFIF_JPG8:
      m_pLog->AddLineHdr("*** Marker: JPG8 (JPEG Extension) (xFFF8) ***");
      break;
    case JFIF_JPG9:
      m_pLog->AddLineHdr("*** Marker: JPG9 (JPEG Extension) (xFFF9) ***");
      break;
    case JFIF_JPG10:
      m_pLog->AddLineHdr("*** Marker: JPG10 (JPEG Extension) (xFFFA) ***");
      break;
    case JFIF_JPG11:
      m_pLog->AddLineHdr("*** Marker: JPG11 (JPEG Extension) (xFFFB) ***");
      break;
    case JFIF_JPG12:
      m_pLog->AddLineHdr("*** Marker: JPG12 (JPEG Extension) (xFFFC) ***");
      break;
    case JFIF_JPG13:
      m_pLog->AddLineHdr("*** Marker: JPG13 (JPEG Extension) (xFFFD) ***");
      break;
    case JFIF_TEM:
      m_pLog->AddLineHdr("*** Marker: TEM (Temporary) (xFF01) ***");
      break;

    default:
      strTmp = QString("*** Marker: ??? (Unknown) (xFF%1) ***").arg(nCode, 2, 16, QChar('0'));
      m_pLog->AddLineHdr(strTmp);
      break;
  }

  // Adjust position to account for the word used in decoding the marker!
  strTmp = QString("  OFFSET: 0x%1").arg(m_nPos - 2, 8, 16, QChar('0'));
  m_pLog->AddLine(strTmp);
}

//-----------------------------------------------------------------------------
// Update the status bar with a message
void CjfifDecode::SetStatusText(QString strText)
{
  // Make sure that we have been connected to the status
  // bar of the main window first! Note that it is jpegsnoopDoc
  // that sets this variable.
//  if(m_pStatBar)
//  {
//    m_pStatBar->showMessage(strText);
//  }

  emit(strText, 0);
  QCoreApplication::processEvents();
}

//-----------------------------------------------------------------------------
// Generate a special output form of the current image's
// compression signature and other characteristics. This is only
// used during development and batch import to build the MySQL repository.
void CjfifDecode::OutputSpecial()
{
  QString strTmp;
  QString strFull;

  Q_ASSERT(m_eImgLandscape != ENUM_LANDSCAPE_UNSET);

  // This mode of operation is currently only used
  // to import the local signature database into a MySQL database
  // backend. It simply reports the MySQL commands which can be input
  // into a MySQL client application.
  if(m_bOutputDB)
  {
    m_pLog->AddLine("*** DB OUTPUT START ***");
    m_pLog->AddLine("INSERT INTO `quant` (`key`, `make`, `model`, ");
    m_pLog->AddLine("`qual`, `subsamp`, `lum_00`, `lum_01`, `lum_02`, `lum_03`, `lum_04`, ");
    m_pLog->AddLine("`lum_05`, `lum_06`, `lum_07`, `chr_00`, `chr_01`, `chr_02`, ");
    m_pLog->AddLine("`chr_03`, `chr_04`, `chr_05`, `chr_06`, `chr_07`, `qual_lum`, `qual_chr`) VALUES (");

    strFull = "'*KEY*', ";      // key -- need to override

    // Might need to change m_strImgExifMake to be lowercase
    strTmp = QString("'%1', ").arg(m_strImgExifMake);
    strFull += strTmp;          // make

    strTmp = QString("'%1', ").arg(m_strImgExifModel);
    strFull += strTmp;          // model

    strTmp = QString("'%1', ").arg(m_strImgQualExif);
    strFull += strTmp;          // quality

    strTmp = QString("'%1', ").arg(m_strImgQuantCss);
    strFull += strTmp;          // subsampling

    m_pLog->AddLine(strFull);

    // Step through both quantization tables (0=lum,1=chr)
    uint32_t nMatrixInd;

    for(uint32_t nDqtInd = 0; nDqtInd < 2; nDqtInd++)
    {

      strFull = "";

      for(uint32_t nY = 0; nY < 8; nY++)
      {
        strFull += "'";

        for(uint32_t nX = 0; nX < 8; nX++)
        {
          // Rotate the matrix if necessary!
          nMatrixInd = (m_eImgLandscape != ENUM_LANDSCAPE_NO) ? (nY * 8 + nX) : (nX * 8 + nY);
          strTmp = QString("%1").arg(m_anImgDqtTbl[nDqtInd][nMatrixInd]);
          strFull += strTmp;

          if(nX != 7)
          {
            strFull += ",";
          }
        }

        strFull += "', ";

        if(nY == 3)
        {
          m_pLog->AddLine(strFull);
          strFull = "";
        }
      }

      m_pLog->AddLine(strFull);
    }

    strFull = "";
    // Output quality ratings
    strTmp = QString("'%1', ").arg(m_adImgDqtQual[0]);
    strFull += strTmp;
    // Don't put out comma separator on last line!
    strTmp = QString("'%1'").arg(m_adImgDqtQual[1]);
    strFull += strTmp;
    strFull += ");";
    m_pLog->AddLine(strFull);

    m_pLog->AddLine("*** DB OUTPUT END ***");
  }
}

//-----------------------------------------------------------------------------
// Generate the compression signatures (both unrotated and
// rotated) in advance of submitting to the database.
void CjfifDecode::PrepareSignature()
{
  // Set m_strHash
  PrepareSignatureSingle(false);
  // Set m_strHashRot
  PrepareSignatureSingle(true);
}

// Prepare the image signature for later submission
// NOTE: ASCII vars are used (instead of unicode) to support usage of MD5 library
void CjfifDecode::PrepareSignatureSingle(bool bRotate)
{
  QString strTmp;
  QString strSet;
  QString strHashIn;

//  unsigned char pHashIn[2000];
  unsigned char *pHashIn;

  QString strDqt;

  MD5_CTX sMd5;

  int32_t nLenHashIn;
  uint32_t nInd;

  Q_ASSERT(m_eImgLandscape != ENUM_LANDSCAPE_UNSET);

  pHashIn = new unsigned char[2000];
  // -----------------------------------------------------------
  // Calculate the MD5 hash for online/internal database!
  // signature "00" : DQT0,DQT1,CSS
  // signature "01" : salt,DQT0,DQT1,..DQTx(if defined)

  // Build the source string
  // NOTE: For the purposes of the hash, we need to rotate the DQT tables
  // if we detect that the photo is in portrait orientation! This keeps everything consistent.

  // If no DQT tables have been defined (e.g. could have loaded text file!) then override the sig generation!
  bool bDqtDefined = false;

  for(uint32_t nSet = 0; nSet < 4; nSet++)
  {
    if(m_abImgDqtSet[nSet])
    {
      bDqtDefined = true;
    }
  }

  if(!bDqtDefined)
  {
    m_strHash = "NONE";
    m_strHashRot = "NONE";
    return;
  }

  // NOTE:
  // The following MD5 code depends on an ASCII string for input
  // We are therefore using QStringA for the hash input instead
  // of the generic text functions. No special (non-ASCII)
  // characters are expected in this string.

  if(DB_SIG_VER == 0x00)
  {
    strHashIn = "";
  }
  else
  {
    strHashIn = "JPEGsnoop";
  }

  // Need to duplicate DQT0 if we only have one DQT table
  for(uint32_t nSet = 0; nSet < 4; nSet++)
  {
    if(m_abImgDqtSet[nSet])
    {
      strSet = "";
      strSet = QString("*DQT%1,").arg(nSet);
      strHashIn += strSet;

      for(uint32_t i = 0; i < 64; i++)
      {
        nInd = (!bRotate) ? i : glb_anQuantRotate[i];
        strTmp = QString("%1,").arg(m_anImgDqtTbl[nSet][nInd], 3, 10, QChar('0'));
        strHashIn += strTmp;
      }
    }                           // if DQTx defined
  }                             // loop through sets (DQT0..DQT3)

  // Removed CSS from signature after version 0x00
  if(DB_SIG_VER == 0x00)
  {
    strHashIn += "*CSS,";
    strHashIn += m_strImgQuantCss;
    strHashIn += ",";
  }

  strHashIn += "*END";
  nLenHashIn = strHashIn.length();

  // Display hash input
  for(int32_t i = 0; i < nLenHashIn; i += 80)
  {
    strTmp = "";
    strTmp = QString("In%1: [").arg(i / 80);
    strTmp += strHashIn.mid(i, 80);
    strTmp += "]";
#ifdef DEBUG_SIG
    m_pLog->AddLine(strTmp);
#endif
  }

  // Copy into buffer
  Q_ASSERT(nLenHashIn < 2000);

  for(int32_t i = 0; i < nLenHashIn; i++)
  {
    pHashIn[i] = strHashIn[i].toLatin1();
  }

  // Calculate the hash
  MD5Init(&sMd5, 0);
  MD5Update(&sMd5, pHashIn, nLenHashIn);
  MD5Final(&sMd5);

  // Overwrite top 8 bits for signature version number
  sMd5.digest32[0] = (sMd5.digest32[0] & 0x00FFFFFF) + (DB_SIG_VER << 24);

  // Convert hash to string format
  // The hexadecimal string is converted to Unicode (if that is build directive)
  if(!bRotate)
  {
    m_strHash = QString("%1%2%3%4")
        .arg(sMd5.digest32[0], 8, 16, QChar('0'))
        .arg(sMd5.digest32[1], 8, 16, QChar('0'))
        .arg(sMd5.digest32[2], 8, 16, QChar('0'))
        .arg(sMd5.digest32[3], 8, 16, QChar('0'));
    m_strHash = m_strHash.toUpper();
  }
  else
  {
    m_strHashRot = QString("%1%2%3%4")
        .arg(sMd5.digest32[0], 8, 16, QChar('0'))
        .arg(sMd5.digest32[1], 8, 16, QChar('0'))
        .arg(sMd5.digest32[2], 8, 16, QChar('0'))
        .arg(sMd5.digest32[3], 8, 16, QChar('0'));
    m_strHashRot = m_strHashRot.toUpper();
  }

//  QByteArray in(reinterpret_cast<char *>(pHashIn), nLenHashIn);
//  m_strHash = QCryptographicHash::hash(reinterpret_cast<char *>(pHashIn), QCryptographicHash::Md5).toHex();
}

//-----------------------------------------------------------------------------
// Generate the compression signatures for the thumbnails
void CjfifDecode::PrepareSignatureThumb()
{
  // Generate m_strHashThumb
  PrepareSignatureThumbSingle(false);
  // Generate m_strHashThumbRot
  PrepareSignatureThumbSingle(true);
}

//-----------------------------------------------------------------------------
// Prepare the image signature for later submission
// NOTE: ASCII vars are used (instead of unicode) to support usage of MD5 library
void CjfifDecode::PrepareSignatureThumbSingle(bool bRotate)
{
  QString strTmp;
  QString strSet;
  QString strHashIn;

  unsigned char pHashIn[2000];

  QString strDqt;

  MD5_CTX sMd5;

  int32_t nLenHashIn;
  uint32_t nInd;

  // -----------------------------------------------------------
  // Calculate the MD5 hash for online/internal database!
  // signature "00" : DQT0,DQT1,CSS
  // signature "01" : salt,DQT0,DQT1

  // Build the source string
  // NOTE: For the purposes of the hash, we need to rotate the DQT tables
  // if we detect that the photo is in portrait orientation! This keeps everything
  // consistent.

  // If no DQT tables have been defined (e.g. could have loaded text file!)
  // then override the sig generation!
  bool bDqtDefined = false;

  for(uint32_t nSet = 0; nSet < 4; nSet++)
  {
    if(m_abImgDqtThumbSet[nSet])
    {
      bDqtDefined = true;
    }
  }

  if(!bDqtDefined)
  {
    m_strHashThumb = "NONE";
    m_strHashThumbRot = "NONE";
    return;
  }

  if(DB_SIG_VER == 0x00)
  {
    strHashIn = "";
  }
  else
  {
    strHashIn = "JPEGsnoop";
  }

  //tblSelY = m_anSofQuantTblSel_Tqi[0]; // Y
  //tblSelC = m_anSofQuantTblSel_Tqi[1]; // Cb (should be same as for Cr)

  // Need to duplicate DQT0 if we only have one DQT table

  for(uint32_t nSet = 0; nSet < 4; nSet++)
  {
    if(m_abImgDqtThumbSet[nSet])
    {
      strSet = "";
      strSet = QString("*DQT%1,").arg(nSet);
      strHashIn += strSet;

      for(uint32_t i = 0; i < 64; i++)
      {
        nInd = (!bRotate) ? i : glb_anQuantRotate[i];
        strTmp = QString("%1,").arg(m_anImgThumbDqt[nSet][nInd], 3, 10, QChar('0'));
        strHashIn += strTmp;
      }
    }                           // if DQTx defined
  }                             // loop through sets (DQT0..DQT3)

  // Removed CSS from signature after version 0x00
  if(DB_SIG_VER == 0x00)
  {
    strHashIn += "*CSS,";
    strHashIn += m_strImgQuantCss;
    strHashIn += ",";
  }

  strHashIn += "*END";
  nLenHashIn = strHashIn.length();

//  qDebug() << "Hash" << strHashIn << strHashIn.length() << strHashIn.toLatin1();
//  QByteArray s;
//  s = QCryptographicHash::hash(strHashIn.toLatin1(), QCryptographicHash::Md5);
//  qDebug() << s;

  // Display hash input
  for(int32_t i = 0; i < nLenHashIn; i += 80)
  {
    strTmp = "";
    strTmp = QString("In%1: [").arg(i / 80);
    strTmp += strHashIn.mid(i, 80);
    strTmp += "]";
#ifdef DEBUG_SIG
    //m_pLog->AddLine(strTmp);
#endif
  }

  // Copy into buffer
  Q_ASSERT(nLenHashIn < 2000);

  for(int32_t i = 0; i < nLenHashIn; i++)
  {
    pHashIn[i] = strHashIn[i].toLatin1();
  }

  // Calculate the hash
  MD5Init(&sMd5, 0);
  MD5Update(&sMd5, pHashIn, nLenHashIn);
  MD5Final(&sMd5);

  // Overwrite top 8 bits for signature version number
  sMd5.digest32[0] = (sMd5.digest32[0] & 0x00FFFFFF) + (DB_SIG_VER << 24);

  // Convert hash to string format
  if(!bRotate)
  {
    m_strHashThumb =
      QString("%1%2%3%4")
        .arg(sMd5.digest32[0], 8, 16, QChar('0'))
        .arg(sMd5.digest32[1], 8, 16, QChar('0'))
        .arg(sMd5.digest32[2], 8, 16, QChar('0'))
        .arg(sMd5.digest32[3], 8, 16, QChar('0'));
  }
  else
  {
    m_strHashThumbRot =
      QString("%1%2%3%4")
        .arg(sMd5.digest32[0], 8, 16, QChar('0'))
        .arg(sMd5.digest32[1], 8, 16, QChar('0'))
        .arg(sMd5.digest32[2], 8, 16, QChar('0'))
        .arg(sMd5.digest32[3], 8, 16, QChar('0'));
  }
}

//-----------------------------------------------------------------------------
// Compare the image compression signature & metadata against the database.
// This is the routine that is also responsible for creating an
// "Image Assessment" -- ie. whether the image may have been edited or not.
//
// PRE: m_strHash signature has already been calculated by PrepareSignature()
bool CjfifDecode::CompareSignature(bool bQuiet = false)
{
  QString strTmp;
  QString strHashOut;
  QString locationStr;

  uint32_t ind;

  bool bCurXsw = false;
  bool bCurXmm = false;
  bool bCurXmkr = false;
  bool bCurXextrasw = false;    // EXIF Extra fields match software indicator
  bool bCurXcomsw = false;      // EXIF COM field match software indicator
  bool bCurXps = false;         // EXIF photoshop IRB present
  bool bSrchXsw = false;
  bool bSrchXswUsig = false;
  bool bSrchXmmUsig = false;
  bool bSrchUsig = false;
  bool bMatchIjg = false;

  QString sMatchIjgQual = "";

  Q_ASSERT(m_strHash != "NONE");
  Q_ASSERT(m_strHashRot != "NONE");

  if(bQuiet)
  {
    m_pLog->Disable();
  }

  m_pLog->AddLine("");
  m_pLog->AddLineHdr("*** Searching Compression Signatures ***");
  m_pLog->AddLine("");

  // Output the hash
  strHashOut = "  Signature:           ";
  strHashOut += m_strHash;
  m_pLog->AddLine(strHashOut);
  strHashOut = "  Signature (Rotated): ";
  strHashOut += m_strHashRot;
  m_pLog->AddLine(strHashOut);

  strTmp = QString("  File Offset:         %1 bytes").arg(m_pAppConfig->startPos(), 1);
  m_pLog->AddLine(strTmp);

  // Output the CSS
  strTmp = QString("  Chroma subsampling:  %1").arg(m_strImgQuantCss);
  m_pLog->AddLine(strTmp);

  // Calculate the final fields
  // Add Photoshop IRB entries
  // Note that we always add an entry to the m_strImgExtras even if
  // there are no photoshop tags detected. It will appear as "[PS]:[0/0]"
  strTmp = "";
  strTmp = QString("[PS]:[%1/%2],").arg(m_nImgQualPhotoshopSa).arg(m_nImgQualPhotoshopSfw);
  m_strImgExtras += strTmp;

  // --------------------------------------
  // Determine current entry fields

  // Note that some cameras/phones have an empty Make, but use the Model! (eg. Palm Treo)
  if((m_strImgExifMake == "???") && (m_strImgExifModel == "???"))
  {
    m_pLog->AddLine("  EXIF Make/Model:     NONE");
    bCurXmm = false;
  }
  else
  {
    strTmp = QString("  EXIF Make/Model:     OK   [%1] [%2]").arg(m_strImgExifMake).arg(m_strImgExifModel);
    m_pLog->AddLine(strTmp);
    bCurXmm = true;
  }

  if(m_bImgExifMakernotes)
  {
    m_pLog->AddLine("  EXIF Makernotes:     OK  ");
    bCurXmkr = true;
  }
  else
  {
    m_pLog->AddLine("  EXIF Makernotes:     NONE");
    bCurXmkr = false;
  }

  if(m_strSoftware.length() == 0)
  {
    m_pLog->AddLine("  EXIF Software:       NONE");
    bCurXsw = false;
  }
  else
  {
    strTmp = QString("  EXIF Software:       OK   [%1]").arg(m_strSoftware);
    m_pLog->AddLine(strTmp);

    // EXIF software field is non-empty
    bCurXsw = true;
  }

  m_pLog->AddLine("");

  // --------------------------------------
  // Determine search results

  // All of the rest of the search results require searching
  // through the database entries

  bSrchXswUsig = false;
  bSrchXmmUsig = false;
  bSrchUsig = false;
  bMatchIjg = false;
  sMatchIjgQual = "";

  uint32_t nSigsInternal = m_pDbSigs->GetNumSigsInternal();

  uint32_t nSigsExtra = m_pDbSigs->GetNumSigsExtra();

  strTmp = QString("  Searching Compression Signatures: (%1 built-in, %2 user(*) )").arg(nSigsInternal).arg(nSigsExtra);
  m_pLog->AddLine(strTmp);

  // Now in SIG version 0x01 and later, we are not including
  // the CSS in the signature. Therefore, we need to compare it
  // manually.

  bool curMatchMm;
  bool curMatchSw;
  bool curMatchSig;
  bool curMatchSigCss;

  // Check on Extras field
  // Noted that Canon EOS Viewer Utility (EVU) seems to convert RAWs with
  // the only identifying information being this:
  //  e.g. "[Canon.ImageType]:[CRW:EOS 300D DIGITAL CMOS RAW],_T("
  if(m_strImgExtras.contains("[Canon.ImageType]:[CRW:"))
  {
    bCurXextrasw = true;
  }

  if(m_strImgExtras.contains("[Nikon1.Quality]:[RAW"))
  {
    bCurXextrasw = true;
  }

  if(m_strImgExtras.contains("[Nikon2.Quality]:[RAW"))
  {
    bCurXextrasw = true;
  }

  if(m_strImgExtras.contains("[Nikon3.Quality]:[RAW"))
  {
    bCurXextrasw = true;
  }

  if((m_nImgQualPhotoshopSa != 0) || (m_nImgQualPhotoshopSfw != 0))
  {
    bCurXps = true;
  }

  // Search for known COMment field indicators
  if(m_pDbSigs->SearchCom(m_strComment))
  {
    bCurXcomsw = true;
  }

  m_pLog->AddLine("");
  m_pLog->AddLine("          EXIF.Make / Software        EXIF.Model                            Quality           Subsamp Match?");
  m_pLog->AddLine("          -------------------------   -----------------------------------   ----------------  --------------");

  CompSig pEntry;

  uint32_t ind_max = m_pDbSigs->GetDBNumEntries();

  for(ind = 0; ind < ind_max; ind++)
  {
    m_pDbSigs->GetDBEntry(ind, &pEntry);

    // Reset current entry state
    curMatchMm = false;
    curMatchSw = false;
    curMatchSig = false;
    curMatchSigCss = false;

    // Compare make/model (only for digicams)
    if((pEntry.eEditor == ENUM_EDITOR_CAM) &&
       (bCurXmm == true) && (pEntry.strXMake == m_strImgExifMake) && (pEntry.strXModel == m_strImgExifModel))
    {
      curMatchMm = true;
    }

    // For software entries, do a loose search
    if((pEntry.eEditor == ENUM_EDITOR_SW) &&
       (bCurXsw == true) && (pEntry.strMSwTrim != "") && (m_strSoftware.indexOf(pEntry.strMSwTrim) != -1))
    {
      // Software field matches known software string
      bSrchXsw = true;
      curMatchSw = true;
    }

    // Compare signature (and CSS for digicams)
    if((pEntry.strCSig == m_strHash) || (pEntry.strCSigRot == m_strHash) ||
       (pEntry.strCSig == m_strHashRot) || (pEntry.strCSigRot == m_strHashRot))
    {
      curMatchSig = true;

      // If Database entry is for an editor, sig matches irrespective of CSS
      if(pEntry.eEditor == ENUM_EDITOR_SW)
      {
        bSrchUsig = true;
        curMatchSigCss = true;  // FIXME: do I need this?

        // For special case of IJG
        if(pEntry.strMSwDisp == "IJG Library")
        {
          bMatchIjg = true;
          sMatchIjgQual = pEntry.strUmQual;
        }
      }
      else
      {
        // Database entry is for a digicam, sig match only if CSS matches too
        if(pEntry.strXSubsamp == m_strImgQuantCss)
        {
          bSrchUsig = true;
          curMatchSigCss = true;
        }
        else
        {
          curMatchSigCss = false;
        }
      }                         // editor
    }
    else
    {
      // sig doesn't match
      curMatchSig = false;
      curMatchSigCss = false;
    }

    // For digicams:
    if(curMatchMm && curMatchSigCss)
    {
      bSrchXmmUsig = true;
    }

    // For software:
    if(curMatchSw && curMatchSig)
    {
      bSrchXswUsig = true;
    }

    if(m_pDbSigs->IsDBEntryUser(ind))
    {
      locationStr = "*";
    }
    else
    {
      locationStr = " ";
    }

    // Display entry if it is a good match
    if(curMatchSig)
    {
      if(pEntry.eEditor == ENUM_EDITOR_CAM)
      {
        strTmp =
          QString("    %1%2[%3] [%4] [%5] %6 %7 %8")
            .arg(locationStr)
            .arg("CAM:", 4)
            .arg(pEntry.strXMake.left(25), -25)
            .arg(pEntry.strXModel.left(35), -35)
            .arg(pEntry.strUmQual.left(16), -16)
            .arg(curMatchSigCss ? "Yes" : "No", -5)
            .arg("", -5)
            .arg("", -5);
      }
      else if(pEntry.eEditor == ENUM_EDITOR_SW)
      {
        strTmp =
          QString("    %1%2[%3] [%4] [%5] %6 %7 %8")
            .arg(locationStr)
            .arg("SW :", 4)
            .arg(pEntry.strMSwDisp.left(25), -25)
            .arg("", -35)
            .arg(pEntry.strUmQual.left(16), -16)
            .arg("", -5)
            .arg("", -5)
            .arg("", -5);
      }
      else
      {
        strTmp =
          QString("    %1%2[%3] [%4] [%5] %6 %7 %8")
            .arg(locationStr)
            .arg("?? :", 4)
            .arg(pEntry.strXMake.left(25), -25)
            .arg(pEntry.strXModel.left(35), -35)
            .arg(pEntry.strUmQual.left(16), -16)
            .arg("", -5)
            .arg("", -5)
            .arg("", -5);
      }

      if(curMatchMm || curMatchSw)
      {
        m_pLog->AddLineGood(strTmp);
      }
      else
      {
        m_pLog->AddLine(strTmp);
      }
    }
  }                             // loop through DB

  QString strSw;

  // If it matches an IJG signature, report other possible sources:
  if(bMatchIjg)
  {
    m_pLog->AddLine("");
    m_pLog->AddLine("    The following IJG-based editors also match this signature:");
    uint32_t nIjgNum;

    QString strIjgSw;

    nIjgNum = m_pDbSigs->GetIjgNum();
    for(ind = 0; ind < nIjgNum; ind++)
    {
      strIjgSw = m_pDbSigs->GetIjgEntry(ind);
      strTmp =
        QString("     %1[%2]  %3  [%4] %5 %6 %7")
          .arg("SW :", 4)
          .arg(strIjgSw.left(25), -25)
          .arg("", -35)
          .arg(sMatchIjgQual.left(16), -16)
          .arg("", -5)
          .arg("", -5)
          .arg("", -5);
      m_pLog->AddLine(strTmp);
    }
  }

  //m_pLog->AddLine("          --------------------   -----------------------------------   ----------------  --------------");
  m_pLog->AddLine("");

  if(bCurXps)
  {
    m_pLog->AddLine("  NOTE: Photoshop IRB detected");
  }
  if(bCurXextrasw)
  {
    m_pLog->AddLine("  NOTE: Additional EXIF fields indicate software processing");
  }
  if(bSrchXsw)
  {
    m_pLog->AddLine("  NOTE: EXIF Software field recognized as from editor");
  }
  if(bCurXcomsw)
  {
    m_pLog->AddLine("  NOTE: JFIF COMMENT field is known software");
  }

  // ============================================
  // Image Assessment Algorithm
  // ============================================

  bool bEditDefinite = false;
  bool bEditLikely = false;
  bool bEditNot = false;
  bool bEditNotUnknownSw = false;

  if(bCurXps)
  {
    bEditDefinite = true;
  }

  if(!bCurXmm)
  {
    bEditDefinite = true;
  }

  if(bCurXextrasw)
  {
    bEditDefinite = true;
  }

  if(bCurXcomsw)
  {
    bEditDefinite = true;
  }

  if(bSrchXsw)
  {
    // Software field matches known software string
    bEditDefinite = true;
  }

  if(m_pDbSigs->LookupExcMmIsEdit(m_strImgExifMake, m_strImgExifModel))
  {
    // Make/Model is in exception list of ones that mark known software
    bEditDefinite = true;
  }

  if(!bCurXmkr)
  {
    // If we are missing maker notes, we are almost always dealing with
    // edited images. There are some known exceptions, so far:
    //  - Very old digicams
    //  - Certain camera phones
    //  - Blackberry
    // Perhaps we can make an exception for particular digicams (based on
    // make/model) that this determination will not apply. This means that
    // we open up the doors for these files being edited and not caught.

    if(m_pDbSigs->LookupExcMmNoMkr(m_strImgExifMake, m_strImgExifModel))
    {
      // This is a known exception!
    }
    else
    {
      bEditLikely = true;
    }
  }

  // Filter down remaining scenarios
  if(!bEditDefinite && !bEditLikely)
  {
    if(bSrchXmmUsig)
    {
      // DB cam signature matches DQT & make/model
      if(!bCurXsw)
      {
        // EXIF software field is empty
        //
        // We can now be pretty confident that this file has not
        // been edited by all the means that we are checking
        bEditNot = true;
      }
      else
      {
        // EXIF software field is set
        //
        // This field is often used by:
        //  - Software editors (edited)
        //  - RAW converter software (edited)
        //  - Digicams to indicate firmware (original)
        //  - Phones to indicate firmware (original)
        //
        // However, in generating bEditDefinite, we have already
        // checked for bSrchXsw which looked for known software
        // strings. Therefore, we will primarily be left with
        // firmware strings, etc.
        //
        // We will mark this as NOT EDITED but with caution of unknown SW field
        bEditNot = true;
        bEditNotUnknownSw = true;
      }
    }
    else
    {
      // No DB cam signature matches DQT & make/model
      // According to EXIF data, this file does not appear to be edited,
      // but no compression signatures in the database match this
      // particular make/model. Therefore, result is UNSURE.
    }
  }

  // Now make final assessment call

  // Determine if image has been processed/edited
  m_eImgEdited = EDITED_UNSET;
  if(bEditDefinite)
  {
    m_eImgEdited = EDITED_YES;
  }
  else if(bEditLikely)
  {
    m_eImgEdited = EDITED_YESPROB;
  }
  else if(bEditNot)
  {
    // Images that fall into this category will have:
    //  - No Photoshop tags
    //  - Make/Model is present
    //  - Makernotes present
    //  - No extra software tags (eg. IFD)
    //  - No comment field with known software
    //  - No software field or it does not match known software
    //  - Signature matches DB for this make/model
    m_eImgEdited = EDITED_NO;
  }
  else
  {
    // Images that fall into this category will have:
    //  - Same as EDITED_NO but:
    //  - Signature does not match DB for this make/model
    // In all likelihood, this image will in fact be original
    m_eImgEdited = EDITED_UNSURE;
  }

  // If the file offset is non-zero, then don't ask for submit or show assessment
  if(m_pAppConfig->startPos() != 0)
  {
    m_pLog->AddLine("  ASSESSMENT not done as file offset non-zero");

    if(bQuiet)
    {
      m_pLog->Enable();
    }

    return false;
  }

  // ============================================
  // Display final assessment
  // ============================================

  m_pLog->AddLine("  Based on the analysis of compression characteristics and EXIF metadata:");
  m_pLog->AddLine("");
  if(m_eImgEdited == EDITED_YES)
  {
    m_pLog->AddLine("  ASSESSMENT: Class 1 - Image is processed/edited");
  }
  else if(m_eImgEdited == EDITED_YESPROB)
  {
    m_pLog->AddLine("  ASSESSMENT: Class 2 - Image has high probability of being processed/edited");
  }
  else if(m_eImgEdited == EDITED_NO)
  {
    m_pLog->AddLine("  ASSESSMENT: Class 3 - Image has high probability of being original");
    // In case the EXIF Software field was detected,
    if(bEditNotUnknownSw)
    {
      m_pLog->AddLine("              Note that EXIF Software field is set (typically contains Firmware version)");
    }
  }
  else if(m_eImgEdited == EDITED_UNSURE)
  {
    m_pLog->AddLine("  ASSESSMENT: Class 4 - Uncertain if processed or original");
    m_pLog->AddLine("              While the EXIF fields indicate original, no compression signatures ");
    m_pLog->AddLine("              in the current database were found matching this make/model");
  }
  else
  {
    m_pLog->AddLineErr("  ASSESSMENT: *** Failed to complete ***");
  }

  m_pLog->AddLine("");

  // Determine if user should add entry to DB
  bool bDbReqAdd = false;       // Ask user to add
  bool bDbReqAddAuto = false;   // Automatically add (in batch operation)

  // TODO: This section should be rewritten to reduce complexity

  m_eDbReqSuggest = DB_ADD_SUGGEST_UNSET;

  if(m_eImgEdited == EDITED_NO)
  {
    bDbReqAdd = false;
    m_eDbReqSuggest = DB_ADD_SUGGEST_CAM;
  }
  else if(m_eImgEdited == EDITED_UNSURE)
  {
    bDbReqAdd = true;
    bDbReqAddAuto = true;
    m_eDbReqSuggest = DB_ADD_SUGGEST_CAM;
    m_pLog->AddLine("  Appears to be new signature for known camera.");
    m_pLog->AddLine("  If the camera/software doesn't appear in list above,");
    m_pLog->AddLineWarn("  PLEASE ADD TO DATABASE with [Tools->Add Camera to DB]");
  }
  else if(bCurXps && bSrchUsig)
  {
    // Photoshop and we already have sig
    bDbReqAdd = false;
    m_eDbReqSuggest = DB_ADD_SUGGEST_SW;
  }
  else if(bCurXps && !bSrchUsig)
  {
    // Photoshop and we don't already have sig
    bDbReqAdd = true;
    bDbReqAddAuto = true;
    m_eDbReqSuggest = DB_ADD_SUGGEST_SW;
    m_pLog->AddLine("  Appears to be new signature for Photoshop.");
    m_pLog->AddLine("  If it doesn't appear in list above,");
    m_pLog->AddLineWarn("  PLEASE ADD TO DATABASE with [Tools->Add Camera to DB]");
  }
  else if(bCurXsw && bSrchXsw && bSrchXswUsig)
  {
    bDbReqAdd = false;
    m_eDbReqSuggest = DB_ADD_SUGGEST_SW;
  }
  else if(bCurXextrasw)
  {
    bDbReqAdd = false;
    m_eDbReqSuggest = DB_ADD_SUGGEST_SW;
  }
  else if(bCurXsw && bSrchXsw && !bSrchXswUsig)
  {
    bDbReqAdd = true;
    //bDbReqAddAuto = true;
    m_eDbReqSuggest = DB_ADD_SUGGEST_SW;
    m_pLog->AddLine("  Appears to be new signature for known software.");
    m_pLog->AddLine("  If the camera/software doesn't appear in list above,");
    m_pLog->AddLineWarn("  PLEASE ADD TO DATABASE with [Tools->Add Camera to DB]");
  }
  else if(bCurXmm && bCurXmkr && !bSrchXsw && !bSrchXmmUsig)
  {
    // unsure if cam, so ask user
    bDbReqAdd = true;
    bDbReqAddAuto = true;
    m_eDbReqSuggest = DB_ADD_SUGGEST_CAM;
    m_pLog->AddLine("  This may be a new camera for the database.");
    m_pLog->AddLine("  If this file is original, and camera doesn't appear in list above,");
    m_pLog->AddLineWarn("  PLEASE ADD TO DATABASE with [Tools->Add Camera to DB]");
  }
  else if(!bCurXmm && !bCurXmkr && !bSrchXsw)
  {
    // unsure if SW, so ask user
    bDbReqAdd = true;
    m_eDbReqSuggest = DB_ADD_SUGGEST_SW;
    m_pLog->AddLine("  This may be a new software editor for the database.");
    m_pLog->AddLine("  If this file is processed, and editor doesn't appear in list above,");
    m_pLog->AddLineWarn("  PLEASE ADD TO DATABASE with [Tools->Add Camera to DB]");
  }

  m_pLog->AddLine("");

  // -----------------------------------------------------------

  if(bQuiet)
  {
    m_pLog->Enable();
  }

#ifdef BATCH_DO_DBSUBMIT_ALL
  bDbReqAddAuto = true;
#endif

  // Return a value that indicates whether or not we should add this
  // entry to the database
  return bDbReqAddAuto;
}

//-----------------------------------------------------------------------------
// Build the image data string that will be sent to the database repository
// This data string contains the compression siganture and a few special
// fields such as image dimensions, etc.
//
// If Portrait, Rotates DQT table, Width/Height.
//   m_strImgQuantCss is already rotated by ProcessFile()
// PRE: m_strHash already defined
void CjfifDecode::PrepareSendSubmit(QString strQual, teSource eUserSource, QString strUserSoftware, QString strUserNotes)
{
  // Generate the DQT arrays suitable for posting
  QString strTmp1;
  QString asDqt[4];

  uint32_t nMatrixInd;

  Q_ASSERT(m_strHash != "NONE");
  Q_ASSERT(m_eImgLandscape != ENUM_LANDSCAPE_UNSET);

  for(uint32_t nSet = 0; nSet < 4; nSet++)
  {
    asDqt[nSet] = "";

    if(m_abImgDqtSet[nSet])
    {
      for(uint32_t nInd = 0; nInd < 64; nInd++)
      {
        // FIXME: Still consider rotating DQT table even though we
        // don't know for sure if m_eImgLandscape is accurate
        // Not a big deal if we get it wrong as we still add
        // both pre- and post-rotated sigs.
        nMatrixInd = (m_eImgLandscape != ENUM_LANDSCAPE_NO) ? nInd : glb_anQuantRotate[nInd];

        if((nInd % 8 == 0) && (nInd != 0))
        {
          asDqt[nSet].append("!");
        }

        asDqt[nSet].append(QString("%1").arg(m_anImgDqtTbl[nSet][nMatrixInd]));

        if(nInd % 8 != 7)
        {
          asDqt[nSet].append(",");
        }
      }
    }                           // set defined?
  }                             // up to 4 sets

  uint32_t nOrigW, nOrigH;

  nOrigW = (m_eImgLandscape != ENUM_LANDSCAPE_NO) ? m_nSofSampsPerLine_X : m_nSofNumLines_Y;
  nOrigH = (m_eImgLandscape != ENUM_LANDSCAPE_NO) ? m_nSofNumLines_Y : m_nSofSampsPerLine_X;

  uint32_t nOrigThumbW, nOrigThumbH;

  nOrigThumbW = (m_eImgLandscape != ENUM_LANDSCAPE_NO) ? m_nImgThumbSampsPerLine : m_nImgThumbNumLines;
  nOrigThumbH = (m_eImgLandscape != ENUM_LANDSCAPE_NO) ? m_nImgThumbNumLines : m_nImgThumbSampsPerLine;

  teMaker eMaker;

  eMaker = (m_bImgExifMakernotes) ? ENUM_MAKER_PRESENT : ENUM_MAKER_NONE;

  // Sort sig additions
  // To create some determinism in the database, arrange the sigs
  // to be in numerical order
  QString strSig0, strSig1, strSigThm0, strSigThm1;

  if(m_strHash <= m_strHashRot)
  {
    strSig0 = m_strHash;
    strSig1 = m_strHashRot;
  }
  else
  {
    strSig0 = m_strHashRot;
    strSig1 = m_strHash;
  }

  if(m_strHashThumb <= m_strHashThumbRot)
  {
    strSigThm0 = m_strHashThumb;
    strSigThm1 = m_strHashThumbRot;
  }
  else
  {
    strSigThm0 = m_strHashThumbRot;
    strSigThm1 = m_strHashThumb;
  }

  SendSubmit(m_strImgExifMake, m_strImgExifModel, strQual, asDqt[0], asDqt[1], asDqt[2], asDqt[3], m_strImgQuantCss,
             strSig0, strSig1, strSigThm0, strSigThm1, static_cast<double>(m_adImgDqtQual[0]), static_cast<double>(m_adImgDqtQual[1]), nOrigW, nOrigH,
             m_strSoftware, m_strComment, eMaker, eUserSource, strUserSoftware, m_strImgExtras,
             strUserNotes, m_eImgLandscape, nOrigThumbW, nOrigThumbH);

}

//-----------------------------------------------------------------------------
// Send the compression signature string to the local database file
// in addition to the web repository if the user has enabled it.
void CjfifDecode::SendSubmit(QString strExifMake, QString strExifModel, QString strQual,
                             QString strDqt0, QString strDqt1, QString strDqt2, QString strDqt3,
                             QString strCss,
                             QString strSig, QString strSigRot, QString strSigThumb,
                             QString strSigThumbRot, double fQFact0, double fQFact1, uint32_t nImgW, uint32_t nImgH,
                             QString strExifSoftware, QString strComment, teMaker eMaker,
                             teSource eUserSource, QString strUserSoftware, QString strExtra,
                             QString strUserNotes, uint32_t nExifLandscape, uint32_t nThumbX, uint32_t nThumbY)
{
  // NOTE: This assumes that we've already run PrepareSignature() which usually happens when we process a file.
  Q_ASSERT(strSig != "");
  Q_ASSERT(strSigRot != "");

//  CUrlString curls;

  // Version "03": (v1.8.0+)
  // - Adds JPEGsnoop version (js_ver) to enable signature source correction
  QString DB_SUBMIT_WWW_VER = "03";

#ifndef BATCH_DO
  if(m_bSigExactInDB)
  {
    if(m_pAppConfig->interactive())
      msgBox.setText("Compression signature already in database");
  }
  else
  {
    // Now append it to the local database and resave
//@@    m_pDbSigs->DatabaseExtraAdd(strExifMake, strExifModel, strQual, strSig, strSigRot, strCss, eUserSource, strUserSoftware);

    if(m_pAppConfig->interactive())
    {
      msgBox.setText("Added Compression signature to database");
    }
  }
#endif

  // Is automatic internet update enabled?
//@@  if(!m_pAppConfig->bDbSubmitNet)
  if(!m_pAppConfig->submitDbNet())
  {
    return;
  }

  QString strTmp;
  QString strFormat;
  QString strFormDataPre;
  QString strFormData;

  uint32_t nFormDataLen;
  uint32_t nChecksum = 32;

  QString strSubmitHost;
  QString strSubmitPage;

  strSubmitHost = IA_HOST;
  strSubmitPage = IA_DB_SUBMIT_PAGE;

  for(uint32_t i = 0; i < strlen(IA_HOST); i++)
  {
    nChecksum += strSubmitHost[i].toLatin1();
    nChecksum += 3 * strSubmitPage[i].toLatin1();
  }

  //if ( (m_pAppConfig->bIsWindowsNTorLater) && (nChecksum == 9678) ) {
  if(nChecksum == 9678)
  {
    // Submit to online database
    // - Mark the encoding as UTF-8
    QString strHeaders = "Content-Type: application/x-www-form-urlencoded; charset=utf-8";

    // URL-encoded form variables -
    strFormat = "ver=%1&js_ver=%2&x_make=%3&x_model=%4&um_qual=%5&x_dqt0=%6&x_dqt1=%7&x_dqt2=%8&x_dqt3=%9";
    strFormat += "&x_subsamp=%10&c_sig=%11&c_sigrot=%12&c_qfact0=%13&c_qfact1=%14&x_img_w=%15&x_img_h=%16";
    strFormat += "&x_sw=%17&x_com=%18&x_maker=%19&u_source=%20&u_sw=%21";
    strFormat += "&x_extra=%22&u_notes=%23&c_sigthumb=%24&c_sigthumbrot=%25&x_landscape=%26";
    strFormat += "&x_thumbx=%27&x_thumby=%28";

    strFormDataPre = QString(strFormat)
        .arg(DB_SUBMIT_WWW_VER)
        .arg(VERSION_STR)
        .arg(strExifMake)
        .arg(strExifModel)
        .arg(strQual)
        .arg(strDqt0)
        .arg(strDqt1)
        .arg(strDqt2)
        .arg(strDqt3)
        .arg(strCss)
        .arg(strSig)
        .arg(strSigRot)
        .arg(fQFact0)
        .arg(fQFact1)
        .arg(nImgW)
        .arg(nImgH)
        .arg(strExifSoftware)
        .arg(strComment)
        .arg(eMaker)
        .arg(eUserSource)
        .arg(strUserSoftware)
        .arg(strExtra)
        .arg(strUserNotes)
        .arg(strSigThumb)
        .arg( strSigThumbRot)
        .arg(nExifLandscape)
        .arg(nThumbX)
        .arg(nThumbY);

		//*** Need to sanitize data for URL submission!
		// Search for "&", "?", "="
//@@    strFormData = QString(strFormat)
//        .arg(DB_SUBMIT_WWW_VER)
//        .arg(VERSION_STR)
//        .arg(curls.Encode(strExifMake))
//        .arg(curls.Encode(strExifModel))
//        .arg(strQual)
//        .arg(strDqt0)
//        .arg(strDqt1)
//        .arg(strDqt2)
//        .arg(strDqt3)
//        .arg(strCss)
//        .arg(strSig)
//        .arg(strSigRot)
//        .arg(fQFact0)
//        .arg(fQFact1)
//        .arg(nImgW)
//        .arg(nImgH)
//        .arg(curls.Encode(strExifSoftware))
//        .arg(curls.Encode(strComment))
//        .arg(eMaker)
//        .arg(eUserSource)
//        .arg(curls.Encode(strUserSoftware))
//        .arg(curls.Encode(strExtra))
//        .arg(curls.Encode(strUserNotes))
//        .arg(strSigThumb)
//        .arg(strSigThumbRot)
//        .arg(nExifLandscape)
//        .arg(nThumbX)
//        .arg(nThumbY);

    nFormDataLen = strFormData.length();

#ifdef DEBUG_SIG
    if(m_pAppConfig->interactive())
    {
      msgBox.setText(strFormDataPre);
      msgBox.setText(strFormData);
    }
#endif

#ifdef WWW_WININET
    //static LPSTR astrAcceptTypes[2]={"*/*", NULL};

/*		HINTERNET hINet = NULL;
        HINTERNET hConnection = NULL;
        HINTERNET hData = NULL;

		hINet = InternetOpen("JPEGsnoop/1.0"), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 ;
		if ( !hINet )
		{
      if (m_pAppConfig->interactive())
        msgBox.setText("InternetOpen Failed");
			return;
		}
		try
		{
      hConnection = InternetConnect( hINet, strSubmitHost, 80, NULL,NULL, INTERNET_SERVICE_HTTP, 0, 1 );
			if ( !hConnection )
			{
				InternetCloseHandle(hINet);
				return;
			}
      hData = HttpOpenRequest( hConnection, "POST"), strSubmitPage, NULL, NULL, NULL, 0, 1 ;
			if ( !hData )
			{
				InternetCloseHandle(hConnection);
				InternetCloseHandle(hINet);
				return;
			}
			// GET HttpSendRequest( hData, NULL, 0, NULL, 0);

			// POST requests from HttpSendRequest() don't work well with
			// unicode, so convert from unicode to ANSI
//@@      QString strHeadersA = CW2A(strHeaders,CP_UTF8);
//@@      QString strFormDataA = CW2A(strFormData,CP_UTF8);
			HttpSendRequestA( hData, strHeadersA, strHeadersA.GetLength(), strFormDataA.GetBuffer(), strFormDataA.GetLength());

		}
		catch( CInternetException* e)
		{
			e->ReportError();
			e->Delete();
      //msgBox.setText("EXCEPTION!");
		}
		InternetCloseHandle(hConnection);
		InternetCloseHandle(hINet);
    InternetCloseHandle(hData); */
#endif

#ifdef WWW_WINHTTP
    CInternetSession sSession;
    CHttpConnection *pConnection;
    CHttpFile *pFile;

    bool bResult;

    DWORD dwRet;

    // *** NOTE: Will not work on Windows 95/98!
    // This section is avoided in early OSes otherwise we get an Illegal Op
    try
    {
      pConnection = sSession.GetHttpConnection(submit_host);
      Q_ASSERT(pConnection);
      pFile = pConnection->OpenRequest(CHttpConnection::HTTP_VERB_POST, submit_page);
      Q_ASSERT(pFile);
      bResult = pFile->SendRequest(strHeaders, (LPVOID)  strFormData, strFormData.GetLength());
      Q_ASSERT(bResult != 0);
      pFile->QueryInfoStatusCode(dwRet);
      Q_ASSERT(dwRet == HTTP_STATUS_OK);

      // Clean up!
      if(pFile)
      {
        pFile->Close();
        delete pFile;

        pFile = nullptr;
      }
      if(pConnection)
      {
        pConnection->Close();
        delete pConnection;

        pConnection = nullptr;
      }
      sSession.Close();

    }

    catch(CInternetException * pEx)
    {
      // catch any exceptions from WinINet
      TCHAR szErr[MAX_BUF_EX_ERR_MSG];

      szErr[0] = '\0';

      if(!pEx->GetErrorMessage(szErr, MAX_BUF_EX_ERR_MSG))
        _tcscpy(szErr, "Unknown error");

      TRACE("Submit Failed! - %s", szErr);

      if(m_pAppConfig->interactive())
        msgBox.setText(szErr);

      pEx->Delete();

      if(pFile)
        delete pFile;

      if(pConnection)
        delete pConnection;

      session.Close();
      return;
    }
#endif

  }
}

//-----------------------------------------------------------------------------
// Parse the embedded JPEG thumbnail. This routine is a much-reduced
// version of the main JFIF parser, in that it focuses primarily on the
// DQT tables.
void CjfifDecode::DecodeEmbeddedThumb()
{
  QString strTmp;
  QString strMarker;

  uint32_t nPosSaved;
  uint32_t nPosSaved_sof;
  uint32_t nPosEnd;

  bool bDone;

  uint32_t nCode;

  bool bRet;

  QString strFull;

  uint32_t nDqtPrecision_Pq;
  uint32_t nDqtQuantDestId_Tq;
  uint32_t nImgPrecision;
  uint32_t nLength;
  uint32_t nTmpVal;

  bool bScanSkipDone;
  bool bErrorAny = false;
  bool bErrorThumbLenZero = false;

  uint32_t nSkipCount;

  nPosSaved = m_nPos;

  // Examine the EXIF embedded thumbnail (if it exists)
  if(m_nImgExifThumbComp == 6)
  {
    m_pLog->AddLine("");
    m_pLog->AddLineHdr("*** Embedded JPEG Thumbnail ***");
    strTmp = QString("  Offset: 0x%1").arg(m_nImgExifThumbOffset, 8, 16, QChar('0'));
    m_pLog->AddLine(strTmp);
    strTmp = QString("  Length: 0x%1 (%2)").arg(m_nImgExifThumbLen, 8, 16, QChar('0')).arg(m_nImgExifThumbLen);
    m_pLog->AddLine(strTmp);

    // Quick scan for DQT tables
    m_nPos = m_nImgExifThumbOffset;
    bDone = false;

    while(!bDone)
    {
      // For some reason, I have found files that have a nLength of 0
      if(m_nImgExifThumbLen != 0)
      {
        if((m_nPos - m_nImgExifThumbOffset) > m_nImgExifThumbLen)
        {
          strTmp = QString("ERROR: Read more than specified EXIF thumb nLength (%1 bytes) before EOI").arg(m_nImgExifThumbLen);
          m_pLog->AddLineErr(strTmp);
          bErrorAny = true;
          bDone = true;
        }
      }
      else
      {
        // Don't try to process if nLength is 0!
        // Seen this in a Canon 1ds file (processed by photoshop)
        bDone = true;
        bErrorAny = true;
        bErrorThumbLenZero = true;
      }

      if((!bDone) && (Buf(m_nPos++) != 0xFF))
      {
        strTmp =
          QString("ERROR: Expected marker 0xFF, got 0x%1 @ offset 0x%2")
            .arg(Buf(m_nPos - 1), 2, 16, QChar('0'))
            .arg(m_nPos - 1, 8, 16, QChar('0'));
        m_pLog->AddLineErr(strTmp);
        bErrorAny = true;
        bDone = true;
      }

      if(!bDone)
      {
        nCode = Buf(m_nPos++);

        m_pLog->AddLine("");

        switch (nCode)
        {
          case JFIF_SOI:       // SOI
            m_pLog->AddLine("  * Embedded Thumb Marker: SOI");
            break;

          case JFIF_DQT:       // Define quantization tables
            m_pLog->AddLine("  * Embedded Thumb Marker: DQT");

            nLength = Buf(m_nPos) * 256 + Buf(m_nPos + 1);
            nPosEnd = m_nPos + nLength;
            m_nPos += 2;
            strTmp = QString("    Length = %1").arg(nLength);
            m_pLog->AddLine(strTmp);

            while(nPosEnd > m_nPos)
            {
              strTmp = QString("    ----");
              m_pLog->AddLine(strTmp);

              nTmpVal = Buf(m_nPos++);
              nDqtPrecision_Pq = (nTmpVal & 0xF0) >> 4;
              nDqtQuantDestId_Tq = nTmpVal & 0x0F;
              QString strPrecision = "";

              if(nDqtPrecision_Pq == 0)
              {
                strPrecision = "8 bits";
              }
              else if(nDqtPrecision_Pq == 1)
              {
                strPrecision = "16 bits";
              }
              else
              {
                strPrecision = QString("??? unknown [value=%1]").arg(nDqtPrecision_Pq);
              }

              strTmp = QString("    Precision=%1").arg(strPrecision);
              m_pLog->AddLine(strTmp);
              strTmp = QString("    Destination ID=%1").arg(nDqtQuantDestId_Tq);

              // NOTE: The mapping between destination IDs and the actual
              // usage is defined in the SOF marker which is often later.
              // In nearly all images, the following is true. However, I have
              // seen some test images that set Tbl 3 = Lum, Tbl 0=Chr,
              // Tbl1=Chr, and Tbl2 undefined
              if(nDqtQuantDestId_Tq == 0)
              {
                strTmp += " (Luminance, typically)";
              }
              else if(nDqtQuantDestId_Tq == 1)
              {
                strTmp += " (Chrominance, typically)";
              }
              else if(nDqtQuantDestId_Tq == 2)
              {
                strTmp += " (Chrominance, typically)";
              }
              else
              {
                strTmp += " (???)";
              }

              m_pLog->AddLine(strTmp);

              if(nDqtQuantDestId_Tq >= 4)
              {
                strTmp = QString("ERROR: nDqtQuantDestId_Tq = %1, >= 4").arg(nDqtQuantDestId_Tq);
                m_pLog->AddLineErr(strTmp);
                bDone = true;
                bErrorAny = true;
                break;
              }

              for(uint32_t nInd = 0; nInd <= 63; nInd++)
              {
                nTmpVal = Buf(m_nPos++);
                m_anImgThumbDqt[nDqtQuantDestId_Tq][glb_anZigZag[nInd]] = nTmpVal;
              }

              m_abImgDqtThumbSet[nDqtQuantDestId_Tq] = true;

              // Now display the table
              for(uint32_t nY = 0; nY < 8; nY++)
              {
                strFull = QString("      DQT, Row #%1: ").arg(nY);

                for(uint32_t nX = 0; nX < 8; nX++)
                {
                  strTmp = QString("%1 ").arg(m_anImgThumbDqt[nDqtQuantDestId_Tq][nY * 8 + nX], 3);
                  strFull += strTmp;

                  // Store the DQT entry into the Image DenCoder
                  bRet = m_pImgDec->SetDqtEntry(nDqtQuantDestId_Tq, nY * 8 + nX,
                                                glb_anUnZigZag[nY * 8 + nX], m_anImgDqtTbl[nDqtQuantDestId_Tq][nY * 8 + nX]);
                  DecodeErrCheck(bRet);
                }

                m_pLog->AddLine(strFull);
              }
            }

            break;

          case JFIF_SOF0:
            m_pLog->AddLine("  * Embedded Thumb Marker: SOF");
            nLength = Buf(m_nPos) * 256 + Buf(m_nPos + 1);
            nPosSaved_sof = m_nPos;
            m_nPos += 2;
            strTmp = QString("    Frame header length = %1").arg(nLength);
            m_pLog->AddLine(strTmp);

            nImgPrecision = Buf(m_nPos++);
            strTmp = QString("    Precision = %1").arg(nImgPrecision);
            m_pLog->AddLine(strTmp);

            m_nImgThumbNumLines = Buf(m_nPos) * 256 + Buf(m_nPos + 1);
            m_nPos += 2;
            strTmp = QString("    Number of Lines = %1").arg(m_nImgThumbNumLines);
            m_pLog->AddLine(strTmp);

            m_nImgThumbSampsPerLine = Buf(m_nPos) * 256 + Buf(m_nPos + 1);
            m_nPos += 2;
            strTmp = QString("    Samples per Line = %1").arg(m_nImgThumbSampsPerLine);
            m_pLog->AddLine(strTmp);
            strTmp = QString("    Image Size = %1 x %2").arg(m_nImgThumbSampsPerLine).arg(m_nImgThumbNumLines);
            m_pLog->AddLine(strTmp);

            m_nPos = nPosSaved_sof + nLength;

            break;

          case JFIF_SOS:       // SOS
            m_pLog->AddLine("  * Embedded Thumb Marker: SOS");
            m_pLog->AddLine("    Skipping scan data");
            bScanSkipDone = false;
            nSkipCount = 0;

            while(!bScanSkipDone)
            {
              if((Buf(m_nPos) == 0xFF) && (Buf(m_nPos + 1) != 0x00))
              {
                // Was it a restart marker?
                if((Buf(m_nPos + 1) >= JFIF_RST0) && (Buf(m_nPos + 1) <= JFIF_RST7))
                {
                  m_nPos++;
                }
                else
                {
                  // No... it's a real marker
                  bScanSkipDone = true;
                }
              }
              else
              {
                m_nPos++;
                nSkipCount++;
              }
            }

            strTmp = QString("    Skipped %1 bytes").arg(nSkipCount);
            m_pLog->AddLine(strTmp);
            break;

          case JFIF_EOI:
            m_pLog->AddLine("  * Embedded Thumb Marker: EOI");
            bDone = true;
            break;

          case JFIF_RST0:
          case JFIF_RST1:
          case JFIF_RST2:
          case JFIF_RST3:
          case JFIF_RST4:
          case JFIF_RST5:
          case JFIF_RST6:
          case JFIF_RST7:
            break;

          default:
            GetMarkerName(nCode, strMarker);
            strTmp = QString("  * Embedded Thumb Marker: %1").arg(strMarker);
            m_pLog->AddLine(strTmp);
            nLength = Buf(m_nPos) * 256 + Buf(m_nPos + 1);
            strTmp = QString("    Length = %1").arg(nLength);
            m_pLog->AddLine(strTmp);
            m_nPos += nLength;
            break;
        }
      }                         // if !bDone
    }                           // while !bDone

    // Now calculate the signature
    if(!bErrorAny)
    {
      PrepareSignatureThumb();
      m_pLog->AddLine("");
      strTmp = QString("  * Embedded Thumb Signature: %1").arg(m_strHashThumb);
      m_pLog->AddLine(strTmp);
    }

    if(bErrorThumbLenZero)
    {
      m_strHashThumb = "ERR: Len=0";
      m_strHashThumbRot = "ERR: Len=0";
    }
  }                             // if JPEG compressed

  m_nPos = nPosSaved;
}

//-----------------------------------------------------------------------------
// Lookup the EXIF marker name from the code value
bool CjfifDecode::GetMarkerName(uint32_t nCode, QString & markerStr)
{
  bool bDone = false;
  bool bFound = false;

  uint32_t nInd = 0;

  while(!bDone)
  {
    if(m_pMarkerNames[nInd].nCode == 0)
    {
      bDone = true;
    }
    else if(m_pMarkerNames[nInd].nCode == nCode)
    {
      bDone = true;
      bFound = true;
      markerStr = m_pMarkerNames[nInd].strName;
      return true;
    }
    else
    {
      nInd++;
    }
  }

  if(!bFound)
  {
    markerStr = "";
    markerStr = QString("(0xFF%1)").arg(nCode, 2, 16, QChar('0'));
    return false;
  }

  return true;
}

//-----------------------------------------------------------------------------
// Determine if the file is an AVI MJPEG.
// If so, parse the headers.
// TODO: Expand this function to use sub-functions for each block type
bool CjfifDecode::DecodeAvi()
{
  if(DEBUG_EN)
    m_pAppConfig->DebugLogAdd("CfifDecode::DecodeAvi() Begin");

  QString strTmp;

  uint32_t nPosSaved;

  m_bAvi = false;
  m_bAviMjpeg = false;

  // Perhaps start from file position 0?
  nPosSaved = m_nPos;

  // Start from file position 0
  m_nPos = 0;

  bool bSwap = true;

  QString strRiff;

  uint32_t nRiffLen;

  QString strForm;


  if(DEBUG_EN)
    m_pAppConfig->DebugLogAdd("CfifDecode::DecodeAvi() Checkpoint 1");

  strRiff = m_pWBuf->BufReadStrn(m_nPos, 4);
  m_nPos += 4;
  nRiffLen = m_pWBuf->BufX(m_nPos, 4, bSwap);
  m_nPos += 4;
  strForm = m_pWBuf->BufReadStrn(m_nPos, 4);
  m_nPos += 4;

  if(DEBUG_EN)
    m_pAppConfig->DebugLogAdd("CfifDecode::DecodeAvi() Checkpoint 2");

  if((strRiff == "RIFF") && (strForm == "AVI "))
  {
    m_bAvi = true;
    m_pLog->AddLine("");
    m_pLog->AddLineHdr("*** AVI File Decoding ***");
    m_pLog->AddLine("Decoding RIFF AVI format...");
    m_pLog->AddLine("");
  }
  else
  {
    // Reset file position
    m_nPos = nPosSaved;
    return false;
  }

  QString strHeader;

  uint32_t nChunkSize;
  uint32_t nChunkDataStart;

  bool done = false;

  while(!done)
  {
    if(m_nPos >= m_pWBuf->GetPosEof())
    {
      done = true;
      break;
    }

    strHeader = m_pWBuf->BufReadStrn(m_nPos, 4);
    m_nPos += 4;
    strTmp = QString("  %1").arg(strHeader);
    m_pLog->AddLine(strTmp);

    nChunkSize = m_pWBuf->BufX(m_nPos, 4, bSwap);
    m_nPos += 4;
    nChunkDataStart = m_nPos;

    if(strHeader == "LIST")
    {
      // --- LIST ---
      QString strListType;

      strListType = m_pWBuf->BufReadStrn(m_nPos, 4);
      m_nPos += 4;

      strTmp = QString("    %1").arg(strListType);
      m_pLog->AddLine(strTmp);

      if(strListType == "hdrl")
      {
        // --- hdrl ---

        uint32_t nPosHdrlStart;

        QString strHdrlId;

        strHdrlId = m_pWBuf->BufReadStrn(m_nPos, 4);
        m_nPos += 4;
        uint32_t nHdrlLen;

        nHdrlLen = m_pWBuf->BufX(m_nPos, 4, bSwap);
        m_nPos += 4;
        nPosHdrlStart = m_nPos;

        // nHdrlLen should be 14*4 bytes

        m_nPos = nPosHdrlStart + nHdrlLen;

      }
      else if(strListType == "strl")
      {
        // --- strl ---

        // strhHEADER
        uint32_t nPosStrlStart;

        QString strStrlId;

        strStrlId = m_pWBuf->BufReadStrn(m_nPos, 4);
        m_nPos += 4;
        uint32_t nStrhLen;

        nStrhLen = m_pWBuf->BufX(m_nPos, 4, bSwap);
        m_nPos += 4;
        nPosStrlStart = m_nPos;

        QString fccType;

        QString fccHandler;

        uint32_t dwFlags, dwReserved1, dwInitialFrames, dwScale, dwRate;

        uint32_t dwStart, dwLength, dwSuggestedBufferSize, dwQuality;

        uint32_t dwSampleSize, xdwQuality, xdwSampleSize;

        fccType = m_pWBuf->BufReadStrn(m_nPos, 4);
        m_nPos += 4;
        fccHandler = m_pWBuf->BufReadStrn(m_nPos, 4);
        m_nPos += 4;
        dwFlags = m_pWBuf->BufX(m_nPos, 4, bSwap);
        m_nPos += 4;
        dwReserved1 = m_pWBuf->BufX(m_nPos, 4, bSwap);
        m_nPos += 4;
        dwInitialFrames = m_pWBuf->BufX(m_nPos, 4, bSwap);
        m_nPos += 4;
        dwScale = m_pWBuf->BufX(m_nPos, 4, bSwap);
        m_nPos += 4;
        dwRate = m_pWBuf->BufX(m_nPos, 4, bSwap);
        m_nPos += 4;
        dwStart = m_pWBuf->BufX(m_nPos, 4, bSwap);
        m_nPos += 4;
        dwLength = m_pWBuf->BufX(m_nPos, 4, bSwap);
        m_nPos += 4;
        dwSuggestedBufferSize = m_pWBuf->BufX(m_nPos, 4, bSwap);
        m_nPos += 4;
        dwQuality = m_pWBuf->BufX(m_nPos, 4, bSwap);
        m_nPos += 4;
        dwSampleSize = m_pWBuf->BufX(m_nPos, 4, bSwap);
        m_nPos += 4;
        xdwQuality = m_pWBuf->BufX(m_nPos, 4, bSwap);
        m_nPos += 4;
        xdwSampleSize = m_pWBuf->BufX(m_nPos, 4, bSwap);
        m_nPos += 4;

        QString fccTypeDecode = "";

        if(fccType == "vids")
        {
          fccTypeDecode = "[vids] Video";
        }
        else if(fccType == "auds")
        {
          fccTypeDecode = "[auds] Audio";
        }
        else if(fccType == "txts")
        {
          fccTypeDecode = "[txts] Subtitle";
        }
        else
        {
          fccTypeDecode = QString("[%1]").arg(fccType);
        }

        strTmp = QString("      -[FourCC Type]  = %1").arg(fccTypeDecode);
        m_pLog->AddLine(strTmp);

        strTmp = QString("      -[FourCC Codec] = [%1]").arg(fccHandler);
        m_pLog->AddLine(strTmp);

        double fSampleRate = 0;

        if(dwScale != 0)
        {
          fSampleRate = static_cast<double>(dwRate) / static_cast<double>(dwScale);
        }

        strTmp = QString("      -[Sample Rate]  = [%.2f]").arg(fSampleRate);

        if(fccType == "vids")
        {
          strTmp.append(" frames/sec");
        }
        else if(fccType == "auds")
        {
          strTmp.append(" samples/sec");
        }

        m_pLog->AddLine(strTmp);

        m_nPos = nPosStrlStart + nStrhLen;      // Skip

        strTmp = QString("      %1").arg(fccType);
        m_pLog->AddLine(strTmp);

        if(fccType == "vids")
        {
          // --- vids ---

          // Is it MJPEG?
          //strTmp = QString("      -[Video Stream FourCC]=[%s]"),fccHandler;
          //m_pLog->AddLine(strTmp);
          if(fccHandler == "mjpg")
          {
            m_bAviMjpeg = true;
          }

          if(fccHandler == "MJPG")
          {
            m_bAviMjpeg = true;
          }

          // strfHEADER_BIH
          QString strSkipId;

          uint32_t nSkipLen;
          uint32_t nSkipStart;

          strSkipId = m_pWBuf->BufReadStrn(m_nPos, 4);
          m_nPos += 4;
          nSkipLen = m_pWBuf->BufX(m_nPos, 4, bSwap);
          m_nPos += 4;
          nSkipStart = m_nPos;

          m_nPos = nSkipStart + nSkipLen;       // Skip
        }
        else if(fccType == "auds")
        {
          // --- auds ---

          // strfHEADER_WAVE

          QString strSkipId;

          uint32_t nSkipLen;
          uint32_t nSkipStart;

          strSkipId = m_pWBuf->BufReadStrn(m_nPos, 4);
          m_nPos += 4;
          nSkipLen = m_pWBuf->BufX(m_nPos, 4, bSwap);
          m_nPos += 4;
          nSkipStart = m_nPos;

          m_nPos = nSkipStart + nSkipLen;       // Skip
        }
        else
        {
          // strfHEADER

          QString strSkipId;

          uint32_t nSkipLen;

          uint32_t nSkipStart;

          strSkipId = m_pWBuf->BufReadStrn(m_nPos, 4);
          m_nPos += 4;
          nSkipLen = m_pWBuf->BufX(m_nPos, 4, bSwap);
          m_nPos += 4;
          nSkipStart = m_nPos;

          m_nPos = nSkipStart + nSkipLen;       // Skip
        }

        // strnHEADER
        uint32_t nPosStrnStart;

        QString strStrnId;

        strStrnId = m_pWBuf->BufReadStrn(m_nPos, 4);
        m_nPos += 4;
        uint32_t nStrnLen;

        nStrnLen = m_pWBuf->BufX(m_nPos, 4, bSwap);
        m_nPos += 4;
        nPosStrnStart = m_nPos;

        // FIXME: Can we rewrite in terms of ChunkSize and ChunkDataStart?
        //Q_ASSERT ((nPosStrnStart + nStrnLen + (nStrnLen%2)) == (nChunkDataStart + nChunkSize + (nChunkSize%2)));
        //m_nPos = nChunkDataStart + nChunkSize + (nChunkSize%2);
        m_nPos = nPosStrnStart + nStrnLen + (nStrnLen % 2);     // Skip
      }
      else if(strListType == "movi")
      {
        // movi

        m_nPos = nChunkDataStart + nChunkSize + (nChunkSize % 2);
      }
      else if(strListType == "INFO")
      {
        // INFO
        uint32_t nInfoStart;

        nInfoStart = m_nPos;

        QString strInfoId;

        uint32_t nInfoLen;

        strInfoId = m_pWBuf->BufReadStrn(m_nPos, 4);
        m_nPos += 4;
        nInfoLen = m_pWBuf->BufX(m_nPos, 4, bSwap);
        m_nPos += 4;

        if(strInfoId == "ISFT")
        {
          QString strIsft = "";

          strIsft = m_pWBuf->BufReadStrn(m_nPos, nChunkSize);
          strIsft = strIsft.trimmed();  //!! trim right
          strTmp = QString("      -[Software] = [%1]").arg(strIsft);
          m_pLog->AddLine(strTmp);
        }

        m_nPos = nChunkDataStart + nChunkSize + (nChunkSize % 2);
      }
      else
      {
        // ?
        m_nPos = nChunkDataStart + nChunkSize + (nChunkSize % 2);
      }
    }
    else if(strHeader == "JUNK")
    {
      // Junk

      m_nPos = nChunkDataStart + nChunkSize + (nChunkSize % 2);
    }
    else if(strHeader == "IDIT")
    {
      // Timestamp info (Canon, etc.)

      QString strIditTimestamp = "";

      strIditTimestamp = m_pWBuf->BufReadStrn(m_nPos, nChunkSize);
      strIditTimestamp = strIditTimestamp.trimmed();    //!!
      strTmp = QString("    -[Timestamp] = [1s]").arg(strIditTimestamp);
      m_pLog->AddLine(strTmp);

      m_nPos = nChunkDataStart + nChunkSize + (nChunkSize % 2);

    }
    else if(strHeader == "indx")
    {
      // Index

      m_nPos = nChunkDataStart + nChunkSize + (nChunkSize % 2);
    }
    else if(strHeader == "idx1")
    {
      // Index
      //uint32_t nIdx1Entries = nChunkSize / (4*4);

      m_nPos = nChunkDataStart + nChunkSize + (nChunkSize % 2);
    }
    else
    {
      // Unsupported
      m_nPos = nChunkDataStart + nChunkSize + (nChunkSize % 2);
    }
  }

  m_pLog->AddLine("");

  if(m_bAviMjpeg)
  {
    m_strImgExtras += "[AVI]:[mjpg],";
    m_pLog->AddLineGood("  AVI is MotionJPEG");
    m_pLog->AddLineWarn("  Use [Tools->Img Search Fwd] to locate next frame");
  }
  else
  {
    m_strImgExtras += "[AVI]:[????],";
    m_pLog->AddLineWarn("  AVI is not MotionJPEG. [Img Search Fwd/Rev] unlikely to find frames.");
  }

  m_pLog->AddLine("");

  // Reset file position
  m_nPos = nPosSaved;

  if(DEBUG_EN)
    m_pAppConfig->DebugLogAdd("CfifDecode::DecodeAvi() End");

  return m_bAviMjpeg;
}

//-----------------------------------------------------------------------------
// This is the primary JFIF parsing routine.
// The main loop steps through all of the JFIF markers and calls
// DecodeMarker() each time until we reach the end of file or an error.
// Finally, we invoke the compression signature search function.
//
// Processing starts at the file offset m_pAppConfig->startPos()
//
// INPUT:
// - inFile                                             = Input file pointer
//
// PRE:
// - m_pAppConfig->startPos()    = Starting file offset for decode
//
void CjfifDecode::ProcessFile(QFile *inFile)
{
  QString strTmp;

  if(DEBUG_EN)
    m_pAppConfig->DebugLogAdd("CfifDecode::ProcessFile() Begin");

  // Reset the JFIF decoder state as we may be redoing another file
  Reset();

  // Reset the IMG Decoder state
  if(m_pImgSrcDirty)
  {
    m_pImgDec->ResetState();
  }

  if(DEBUG_EN)
    m_pAppConfig->DebugLogAdd("CfifDecode::ProcessFile() Checkpoint 2");

  // Set the statusbar text to Processing...

  // Ensure the status bar has been allocated
  // NOTE: The stat bar is NULL if we drag & drop a file onto
  //       the JPEGsnoop app icon.
  if(m_pStatBar)
  {
    m_pStatBar->showMessage("Processing...");
  }

  // Note that we don't clear out the logger (with m_pLog->Reset())
  // as we want top-level caller to do this. This way we can
  // still insert extra lines from top level.

  // GetLength returns ULONGLONG. Abort on large files (>=4GB)
  qint64 nPosFileEnd;

  nPosFileEnd = inFile->size();

  if(nPosFileEnd > 0xFFFFFFFF)
  {
    strTmp = "File too large. Skipping.";
    m_pLog->AddLineErr(strTmp);

    if(m_pAppConfig->interactive())
      msgBox.setText(strTmp);

    return;
  }

  if(DEBUG_EN)
    m_pAppConfig->DebugLogAdd("CfifDecode::ProcessFile() Checkpoint 3");


  m_nPosFileEnd = static_cast < uint32_t >(nPosFileEnd);

  uint32_t nStartPos;

  nStartPos = m_pAppConfig->startPos();
  m_nPos = nStartPos;
  m_nPosEmbedStart = nStartPos; // Save the embedded file start position

  if(DEBUG_EN)
    m_pAppConfig->DebugLogAdd("CfifDecode::ProcessFile() Checkpoint 4");

  m_pLog->AddLine(QString("Start Offset: 0x%1").arg(nStartPos, 8, 16, QChar('0')));

  // ----------------------------------------------------------------
  // Test for AVI file
  // - Detect header
  // - start from beginning of file
  DecodeAvi();
  // TODO: Should we skip decode of file if not MJPEG?
  // ----------------------------------------------------------------

  // Test for PSD file
  // - Detect header
  // - FIXME: start from current offset?
  uint32_t nWidth = 0;
  uint32_t nHeight = 0;

#ifdef PS_IMG_DEC_EN
  // If PSD image decoding is enabled, associate the PSD parsing with
  // the current DIB. After decoding, flag the DIB as ready for display.

  // Attempt decode as PSD
  bool bDecPsdOk;

//@@  bDecPsdOk = m_pPsDec->DecodePsd(nStartPos, &m_pImgDec->m_pDibTemp, nWidth, nHeight);
  bDecPsdOk = false;

  if(bDecPsdOk)
  {
    // FIXME: The following is a bit of a hack
    m_pImgDec->m_bDibTempReady = true;
    m_pImgDec->m_bPreviewIsJpeg = false;        // MCU/Block info not available
    m_pImgDec->SetImageDimensions(nWidth, nHeight);

    // Clear the image information
    // The primary reason for this is to ensure we don't have stale information from a previous
    // JPEG image (eg. real image border inside MCU border which would be overlayed during draw).
    m_pImgDec->SetImageDetails(0, 0, 0, 0, false, 0);

    // No more processing of file
    // - Otherwise we'd continue to attempt to decode as JPEG
    return;
  }
#else
  // Don't attempt to display Photoshop image data
  if(m_pPsDec->DecodePsd(nStartPos, nullptr, nWidth, nHeight))
  {
    return;
  }
#endif

  // ----------------------------------------------------------------

// Disable DICOM for now until fully tested
#ifdef SUPPORT_DICOM
  // Test for DICOM
  // - Detect header
  // - start from beginning of file
  bool bDicom = false;

  uint32_t nPosJpeg = 0;   // File offset to embedded JPEG in DICOM

  bDicom = m_pDecDicom->DecodeDicom(0, m_nPosFileEnd, nPosJpeg);

  if(bDicom)
  {
    // Adjust start of JPEG decoding if we are currently without an offset
    if(nStartPos == 0)
    {
      m_pAppConfig->startPos() = nPosJpeg;

      nStartPos = m_pAppConfig->startPos();
      m_nPos = nStartPos;
      m_nPosEmbedStart = nStartPos;     // Save the embedded file start position

      strTmp = QString("Adjusting Start Offset to: 0x%08X"), nStartPos;
      m_pLog->AddLine(strTmp);
      m_pLog->AddLine("");
    }
  }
#endif

  // ----------------------------------------------------------------

  // Decode as JPEG JFIF file

  // If we are in a non-zero offset, add this to extras
  if(m_pAppConfig->startPos() != 0)
  {
    strTmp = QString("[Offset]=[%1],").arg(m_pAppConfig->startPos());
    m_strImgExtras += strTmp;
  }

  uint32_t nDataAfterEof = 0;

  bool bDone = false;

  while(!bDone)
  {
    // Allow some other threads to jump in

    // Return value 0 - OK
    //              1 - Error
    //              2 - EOI
    if(DecodeMarker() != DECMARK_OK)
    {
      bDone = true;

      if(m_nPosFileEnd >= m_nPosEoi)
      {
        nDataAfterEof = m_nPosFileEnd - m_nPosEoi;
      }
    }
    else
    {
      if(m_nPos > m_pWBuf->GetPosEof())
      {
        m_pLog->AddLineErr("ERROR: Early EOF - file may be missing EOI");
        bDone = true;
      }
    }
  }

  // -----------------------------------------------------------
  // Perform any other informational calculations that require all tables
  // to be present.

  // Determine the CSS Ratio
  // Save the subsampling string. Assume component 2 is representative of the overall chrominance.

  // NOTE: Ensure that we don't execute the following code if we haven't
  //       completed our read (ie. get bad marker earlier in processing).
  // TODO: What is the best way to determine all is OK?

  m_strImgQuantCss = "?x?";
  m_strHash = "NONE";
  m_strHashRot = "NONE";

  if(m_bImgOK)
  {
    Q_ASSERT(m_eImgLandscape != ENUM_LANDSCAPE_UNSET);

    if(m_nSofNumComps_Nf == NUM_CHAN_YCC)
    {
      // We only try to determine the chroma subsampling ratio if we have 3 components (assume YCC)
      // In general, we should be able to use the 2nd or 3rd component

      // NOTE: The following assumes m_anSofHorzSampFact_Hi and m_anSofVertSampFact_Vi
      // are non-zero as otherwise we'll have a divide-by-0 exception.
      uint32_t nCompIdent = m_anSofQuantCompId[SCAN_COMP_CB];
      uint32_t nCssFactH = m_nSofHorzSampFactMax_Hmax / m_anSofHorzSampFact_Hi[nCompIdent];
      uint32_t nCssFactV = m_nSofVertSampFactMax_Vmax / m_anSofVertSampFact_Vi[nCompIdent];

      if(m_eImgLandscape != ENUM_LANDSCAPE_NO)
      {
        // Landscape orientation
        m_strImgQuantCss = QString("%1x%2").arg(nCssFactH).arg(nCssFactV);
      }
      else
      {
        // Portrait orientation (flip subsampling ratio)
        m_strImgQuantCss = QString("%1x%1").arg(nCssFactV).arg(nCssFactH);
      }
    }
    else if(m_nSofNumComps_Nf == NUM_CHAN_GRAYSCALE)
    {
      m_strImgQuantCss = "Gray";
    }

    DecodeEmbeddedThumb();

    // Generate the signature
    PrepareSignature();

    // Compare compression signature
    if(m_pAppConfig->searchSig())
    {
      // In the case of lossless files, there won't be any DQT and
      // hence no compression signatures to compare. Therefore, skip this process.
      if(m_strHash == "NONE")
      {
        m_pLog->AddLineWarn("Skipping compression signature search as no DQT");
      }
      else
      {
        CompareSignature();
      }
    }

    if(nDataAfterEof > 0)
    {
      m_pLog->AddLine("");
      m_pLog->AddLineHdr("*** Additional Info ***");
      strTmp =
        QString("NOTE: Data exists after EOF, range: 0x%1-0x%2 (%3 bytes)")
          .arg(m_nPosEoi, 8, 16, QChar('0'))
          .arg(m_nPosFileEnd, 8, 16, QChar('0'))
          .arg(nDataAfterEof);
      m_pLog->AddLine(strTmp);
    }

    // Print out the special-purpose outputs
    OutputSpecial();
  }

  // Reset the status bar text
  if(m_pStatBar)
  {
    m_pStatBar->showMessage("Done");
  }

  // Mark the file as closed
  //m_pWBuf->BufFileUnset();

}

//-----------------------------------------------------------------------------
// Determine if the analyzed file is in a state ready for image
// extraction. Confirms that the important JFIF markers have been
// detected in the previous analysis.
//
// PRE:
// - m_nPosEmbedStart
// - m_nPosEmbedEnd
// - m_nPosFileEnd
//
// RETURN:
// - True if image is ready for extraction
//
bool CjfifDecode::ExportJpegPrepare(QString strFileIn, bool bForceSoi, bool bForceEoi, bool bIgnoreEoi)
{
  // Extract from current file
  //   [m_nPosEmbedStart ... m_nPosEmbedEnd]
  // If state is valid (i.e. file opened)

  QString strTmp = "";

  m_pLog->AddLine("");
  m_pLog->AddLineHdr("*** Exporting JPEG ***");

  strTmp = QString("  Exporting from: [%1]").arg(strFileIn);
  m_pLog->AddLine(strTmp);

  // Only bother to extract if all main sections are present
  bool bExtractWarn = false;

  QString strMissing = "";

  if(!m_bStateEoi)
  {
    if(!bForceEoi && !bIgnoreEoi)
    {
      strTmp = QString("  ERROR: Missing marker: %1").arg("EOI");
      m_pLog->AddLineErr(strTmp);
      m_pLog->AddLineErr("         Aborting export. Consider enabling [Force EOI] or [Ignore Missing EOI] option");
      return false;
    }
    else if(bIgnoreEoi)
    {
      // Ignore the EOI, so mark the end of file, but don't
      // set the flag where we force one.
      m_nPosEmbedEnd = m_nPosFileEnd;
    }
    else
    {
      // We're missing the EOI but the user has requested
      // that we force an EOI, so let's fix things up
      m_nPosEmbedEnd = m_nPosFileEnd;
    }
  }

  if((m_nPosEmbedStart == 0) && (m_nPosEmbedEnd == 0))
  {
    strTmp = QString("  No frame found at this position in file. Consider using [Img Search]");
    m_pLog->AddLineErr(strTmp);
    return false;
  }

  if(!m_bStateSoi)
  {
    if(!bForceSoi)
    {
      strTmp = QString("  ERROR: Missing marker: %1").arg("SOI");
      m_pLog->AddLineErr(strTmp);
      m_pLog->AddLineErr("         Aborting export. Consider enabling [Force SOI] option");
      return false;
    }
    else
    {
      // We're missing the SOI but the user has requested
      // that we force an SOI, so let's fix things up
    }
  }

  if(!m_bStateSos)
  {
    strTmp = QString("  ERROR: Missing marker: %1").arg("SOS");
    m_pLog->AddLineErr(strTmp);
    m_pLog->AddLineErr("         Aborting export");
    return false;
  }

  if(!m_bStateDqt)
  {
    bExtractWarn = true;
    strMissing += "DQT ";
  }

  if(!m_bStateDht)
  {
    bExtractWarn = true;
    strMissing += "DHT ";
  }

  if(!m_bStateSof)
  {
    bExtractWarn = true;
    strMissing += "SOF ";
  }

  if(bExtractWarn)
  {
    strTmp = QString("  NOTE: Missing marker: %1").arg(strMissing);
    m_pLog->AddLineWarn(strTmp);
    m_pLog->AddLineWarn("        Exported JPEG may not be valid");
  }

  if(m_nPosEmbedEnd < m_nPosEmbedStart)
  {
    strTmp = QString("ERROR: Invalid SOI-EOI order. Export aborted.");
    m_pLog->AddLineErr(strTmp);
    if(m_pAppConfig->interactive())
      msgBox.setText(strTmp);
    return false;
  }

  return true;
}

#define EXPORT_BUF_SIZE 131072

//-----------------------------------------------------------------------------
// Export the embedded JPEG image at the current position in the file (with overlays)
// (may be the primary image or even an embedded thumbnail).
bool CjfifDecode::ExportJpegDo(QString strFileIn, QString strFileOut,
                               uint32_t nFileLen, bool bOverlayEn, bool bDhtAviInsert, bool bForceSoi, bool bForceEoi)
{
  QFile *pFileOutput;

  QString strTmp = "";

  strTmp = QString("  Exporting to:   [%1]").arg(strFileOut);
  m_pLog->AddLine(strTmp);

  if(strFileIn == strFileOut)
  {
    strTmp = QString("ERROR: Can't overwrite source file. Aborting export.");
    m_pLog->AddLineErr(strTmp);

    if(m_pAppConfig->interactive())
      msgBox.setText(strTmp);

    return false;
  }

  Q_ASSERT(strFileIn != "");

  if(strFileIn == "")
  {
    strTmp = QString("ERROR: Export but source filename empty");
    m_pLog->AddLineErr(strTmp);

    if(m_pAppConfig->interactive())
      msgBox.setText(strTmp);

    return false;
  }

    // Open specified file
    // Added in shareDenyNone as this apparently helps resolve some people's troubles
    // with an error showing: Couldn't open file "Sharing Violation"
//              pFileOutput = new CFile(strFileOut, CFile::modeCreate| CFile::modeWrite | CFile::typeBinary | CFile::shareDenyNone);
  pFileOutput = new QFile(strFileOut);

  if((pFileOutput->open(QIODevice::WriteOnly)) == false)
  {
    QString strError;

    strError = QString("ERROR: Couldn't open file for write [%1]: [%2]").arg(strFileOut).arg(pFileOutput->errorString());
    m_pLog->AddLineErr(strError);

    if(m_pAppConfig->interactive())
    {
      msgBox.setText(strError);
      msgBox.exec();
    }

    pFileOutput = nullptr;

    return false;
  }

  // Don't attempt to load buffer with zero length file!
  if(nFileLen == 0)
  {
    strTmp = QString("ERROR: Source file length error. Please Reprocess first.");
    m_pLog->AddLineErr(strTmp);

    if(m_pAppConfig->interactive())
      msgBox.setText(strTmp);

    if(pFileOutput)
    {
      delete pFileOutput;

      pFileOutput = nullptr;
    }

    return false;
  }

  // Need to insert fake DHT. Assume we have enough buffer allocated.
  //
  // Step 1: Copy from SOI -> SOS (not incl)
  // Step 2: Insert Fake DHT
  // Step 3: Copy from SOS -> EOI
  uint32_t nCopyStart;
  uint32_t nCopyEnd;
  uint32_t nCopyLeft;
  uint32_t ind;
  uint8_t *pBuf;

  pBuf = new uint8_t[EXPORT_BUF_SIZE + 10];

  if(!pBuf)
  {
    if(pFileOutput)
    {
      delete pFileOutput;
      pFileOutput = nullptr;
    }

    return false;
  }

  // Step 1

  // If we need to force an SOI, do it now
  if(!m_bStateSoi && bForceSoi)
  {
    m_pLog->AddLine("    Forcing SOI Marker");
    uint8_t anBufSoi[2] = { 0xFF, JFIF_SOI };
    pFileOutput->write(reinterpret_cast<char *>(&anBufSoi), 2);
  }

  nCopyStart = m_nPosEmbedStart;
  nCopyEnd = (m_nPosSos - 1);
  ind = nCopyStart;

  while(ind < nCopyEnd)
  {
    nCopyLeft = nCopyEnd - ind + 1;

    if(nCopyLeft > EXPORT_BUF_SIZE)
    {
      nCopyLeft = EXPORT_BUF_SIZE;
    }

    for(uint32_t ind1 = 0; ind1 < nCopyLeft; ind1++)
    {
      pBuf[ind1] = Buf(ind + ind1, !bOverlayEn);
    }

    pFileOutput->write(reinterpret_cast<char *>(pBuf), nCopyLeft);
    ind += nCopyLeft;
    // NOTE: We ensure nFileLen != 0 earlier
    Q_ASSERT(nFileLen > 0);
    strTmp = QString("Exporting %1%...").arg(ind * 100 / nFileLen, 3);
    SetStatusText(strTmp);
  }

  if(bDhtAviInsert)
  {
    // Step 2. The following struct includes the JFIF marker too
    strTmp = "  Inserting standard AVI DHT huffman table";
    m_pLog->AddLine(strTmp);
//!!    pFileOutput->write(reinterpret_cast<char *>(&m_abMJPGDHTSeg), JFIF_DHT_FAKE_SZ);
  }

  // Step 3
  nCopyStart = m_nPosSos;
  nCopyEnd = m_nPosEmbedEnd - 1;
  ind = nCopyStart;

  while(ind < nCopyEnd)
  {
    nCopyLeft = nCopyEnd - ind + 1;

    if(nCopyLeft > EXPORT_BUF_SIZE)
    {
      nCopyLeft = EXPORT_BUF_SIZE;
    }

    for(uint32_t ind1 = 0; ind1 < nCopyLeft; ind1++)
    {
      pBuf[ind1] = Buf(ind + ind1, !bOverlayEn);
    }

    pFileOutput->write(reinterpret_cast<char *>(pBuf), nCopyLeft);
    ind += nCopyLeft;
    // NOTE: We ensure nFileLen != 0 earlier
    Q_ASSERT(nFileLen > 0);
    strTmp = QString("Exporting %1%%...").arg(ind * 100 / nFileLen, 3);
    SetStatusText(strTmp);
  }

  // Now optionally insert the EOI Marker
  if(bForceEoi)
  {
    m_pLog->AddLine("    Forcing EOI Marker");
    uint8_t anBufEoi[2] = { 0xFF, JFIF_EOI };
    pFileOutput->write(reinterpret_cast<char *>(&anBufEoi), 2);
  }

  // Free up space
  pFileOutput->close();

  if(pBuf)
  {
    delete[]pBuf;
    pBuf = nullptr;
  }

  if(pFileOutput)
  {
    delete pFileOutput;
    pFileOutput = nullptr;
  }

  SetStatusText("");
  strTmp = QString("  Export done");
  m_pLog->AddLine(strTmp);

  return true;
}

//-----------------------------------------------------------------------------
// Export a subset of the file with no overlays or mods
bool CjfifDecode::ExportJpegDoRange(QString strFileIn, QString strFileOut, uint32_t nStart, uint32_t nEnd)
{
  QFile *pFileOutput;

  QString strTmp = "";

  strTmp = QString("  Exporting range to:   [%1]").arg(strFileOut);
  m_pLog->AddLine(strTmp);

  if(strFileIn == strFileOut)
  {
    strTmp = "ERROR: Can't overwrite source file. Aborting export.";
    m_pLog->AddLineErr(strTmp);
    if(m_pAppConfig->interactive())
      msgBox.setText(strTmp);

    return false;
  }

  Q_ASSERT(strFileIn != "");

  if(strFileIn == "")
  {
    strTmp = QString("ERROR: Export but source filename empty");
    m_pLog->AddLineErr(strTmp);
    if(m_pAppConfig->interactive())
      msgBox.setText(strTmp);

    return false;
  }

  pFileOutput = new QFile(strFileOut);

  if((pFileOutput->open(QIODevice::WriteOnly)) == false)
  {
    QString strError;

    strError = QString("ERROR: Couldn't open file for write [%1]: [%2]").arg(strFileOut).arg(pFileOutput->errorString());
    m_pLog->AddLineErr(strError);

    if(m_pAppConfig->interactive())
    {
      msgBox.setText(strError);
      msgBox.exec();
    }

    pFileOutput = nullptr;

    return false;
  }

  uint32_t nCopyStart;
  uint32_t nCopyEnd;
  uint32_t nCopyLeft;
  uint32_t ind;

  uint8_t *pBuf;

  pBuf = new uint8_t [EXPORT_BUF_SIZE + 10];

  if(!pBuf)
  {
    if(pFileOutput)
    {
      delete pFileOutput;
      pFileOutput = nullptr;
    }

    return false;
  }

  // Step 1
  nCopyStart = nStart;
  nCopyEnd = nEnd;
  ind = nCopyStart;

  while(ind < nCopyEnd)
  {
    nCopyLeft = nCopyEnd - ind + 1;

    if(nCopyLeft > EXPORT_BUF_SIZE)
    {
      nCopyLeft = EXPORT_BUF_SIZE;
    }

    for(uint32_t ind1 = 0; ind1 < nCopyLeft; ind1++)
    {
      pBuf[ind1] = Buf(ind + ind1, false);
    }

    pFileOutput->write(reinterpret_cast<char *>(pBuf), nCopyLeft);
    ind += nCopyLeft;
    strTmp = QString("Exporting %1%%...").arg(ind * 100 / (nCopyEnd - nCopyStart), 3);
    SetStatusText(strTmp);
  }

  // Free up space
  pFileOutput->close();

  if(pBuf)
  {
    delete[]pBuf;
    pBuf = nullptr;
  }

  if(pFileOutput)
  {
    delete pFileOutput;
    pFileOutput = nullptr;
  }

  SetStatusText("");
  strTmp = "  Export range done";
  m_pLog->AddLine(strTmp);

  return true;
}

// ====================================================================================
// JFIF Decoder Constants
// ====================================================================================

// List of the JFIF markers
const MarkerNameTable CjfifDecode::m_pMarkerNames[] = {
  {JFIF_SOF0, "SOF0"},
  {JFIF_SOF1, "SOF1"},
  {JFIF_SOF2, "SOF2"},
  {JFIF_SOF3, "SOF3"},
  {JFIF_SOF5, "SOF5"},
  {JFIF_SOF6, "SOF6"},
  {JFIF_SOF7, "SOF7"},
  {JFIF_JPG, "JPG"},
  {JFIF_SOF9, "SOF9"},
  {JFIF_SOF10, "SOF10"},
  {JFIF_SOF11, "SOF11"},
  {JFIF_SOF13, "SOF13"},
  {JFIF_SOF14, "SOF14"},
  {JFIF_SOF15, "SOF15"},
  {JFIF_DHT, "DHT"},
  {JFIF_DAC, "DAC"},
  {JFIF_RST0, "RST0"},
  {JFIF_RST1, "RST1"},
  {JFIF_RST2, "RST2"},
  {JFIF_RST3, "RST3"},
  {JFIF_RST4, "RST4"},
  {JFIF_RST5, "RST5"},
  {JFIF_RST6, "RST6"},
  {JFIF_RST7, "RST7"},
  {JFIF_SOI, "SOI"},
  {JFIF_EOI, "EOI"},
  {JFIF_SOS, "SOS"},
  {JFIF_DQT, "DQT"},
  {JFIF_DNL, "DNL"},
  {JFIF_DRI, "DRI"},
  {JFIF_DHP, "DHP"},
  {JFIF_EXP, "EXP"},
  {JFIF_APP0, "APP0"},
  {JFIF_APP1, "APP1"},
  {JFIF_APP2, "APP2"},
  {JFIF_APP3, "APP3"},
  {JFIF_APP4, "APP4"},
  {JFIF_APP5, "APP5"},
  {JFIF_APP6, "APP6"},
  {JFIF_APP7, "APP7"},
  {JFIF_APP8, "APP8"},
  {JFIF_APP9, "APP9"},
  {JFIF_APP10, "APP10"},
  {JFIF_APP11, "APP11"},
  {JFIF_APP12, "APP12"},
  {JFIF_APP13, "APP13"},
  {JFIF_APP14, "APP14"},
  {JFIF_APP15, "APP15"},
  {JFIF_JPG0, "JPG0"},
  {JFIF_JPG1, "JPG1"},
  {JFIF_JPG2, "JPG2"},
  {JFIF_JPG3, "JPG3"},
  {JFIF_JPG4, "JPG4"},
  {JFIF_JPG5, "JPG5"},
  {JFIF_JPG6, "JPG6"},
  {JFIF_JPG7, "JPG7"},
  {JFIF_JPG8, "JPG8"},
  {JFIF_JPG9, "JPG9"},
  {JFIF_JPG10, "JPG10"},
  {JFIF_JPG11, "JPG11"},
  {JFIF_JPG12, "JPG12"},
  {JFIF_JPG13, "JPG13"},
  {JFIF_COM, "COM"},
  {JFIF_TEM, "TEM"},
  //{JFIF_RES*,"RES"},
  {0x00, "*"},
};

// For Motion JPEG, define the DHT tables that we use since they won't exist
// in each frame within the AVI. This table will be read in during
// DecodeDHT()'s call to Buf().
const quint8 CjfifDecode::m_abMJPGDHTSeg[JFIF_DHT_FAKE_SZ] = {
  /* JPEG DHT Segment for YCrCb omitted from MJPG data */
  0xFF, 0xC4, 0x01, 0xA2,
  0x00, 0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x01, 0x00, 0x03, 0x01, 0x01, 0x01, 0x01,
  0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0A, 0x0B, 0x10, 0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03, 0x05, 0x05, 0x04, 0x04, 0x00,
  0x00, 0x01, 0x7D, 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61,
  0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08, 0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0, 0x24,
  0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x34,
  0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56,
  0x57, 0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
  0x79, 0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
  0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9,
  0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9,
  0xDA, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
  0xF8, 0xF9, 0xFA, 0x11, 0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04, 0x07, 0x05, 0x04, 0x04, 0x00, 0x01,
  0x02, 0x77, 0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
  0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xA1, 0xB1, 0xC1, 0x09, 0x23, 0x33, 0x52, 0xF0, 0x15, 0x62,
  0x72, 0xD1, 0x0A, 0x16, 0x24, 0x34, 0xE1, 0x25, 0xF1, 0x17, 0x18, 0x19, 0x1A, 0x26, 0x27, 0x28, 0x29, 0x2A,
  0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56,
  0x57, 0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
  0x79, 0x7A, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
  0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8,
  0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8,
  0xD9, 0xDA, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
  0xF9, 0xFA
};

// TODO: Add ITU-T Example DQT & DHT
//       These will be useful for GeoRaster decode (ie. JPEG-B)

QString glb_strMsgStopDecode = "  Stopping decode. Use [Relaxed Parsing] to continue.";
