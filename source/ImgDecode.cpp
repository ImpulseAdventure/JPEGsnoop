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

#include <math.h>

#include <QtDebug>
#include <QtWidgets>

#include "SnoopConfig.h"

#include "ImgDecode.h"


// ------------------------------------------------------
// Settings

// Flag: Use fixed point arithmetic for IDCT?
//#define IDCT_FIXEDPT

// Flag: Do we stop during scan decode if 0xFF (but not pad)?
// TODO: Make this a config option
//#define SCAN_BAD_MARKER_STOP

// ------------------------------------------------------
// Main code

// Constructor for the Image Decoder
// - This constructor is called only once by Document class
CimgDecode::CimgDecode(CDocLog *pLog, CwindowBuf *pWBuf, CSnoopConfig *pAppConfig, QObject *_parent) : QObject(_parent), m_pLog(pLog), m_pWBuf(pWBuf),
  m_pAppConfig(pAppConfig)
{
  m_bVerbose = false;

  m_pStatBar = 0;
  m_bDibTempReady = false;
  m_bPreviewIsJpeg = false;
  m_bDibHistRgbReady = false;
  m_bDibHistYReady = false;

  m_bHistEn = false;
  m_bStatClipEn = false;        // UNUSED

  m_pMcuFileMap = 0;
  m_pBlkDcValY = 0;
  m_pBlkDcValCb = 0;
  m_pBlkDcValCr = 0;
  m_pPixValY = 0;
  m_pPixValCb = 0;
  m_pPixValCr = 0;

  // Reset the image decoding state
  Reset();

  m_nImgSizeXPartMcu = 0;
  m_nImgSizeYPartMcu = 0;
  m_nImgSizeX = 0;
  m_nImgSizeY = 0;

  // FIXME: Temporary hack to avoid divide-by-0 when displaying PSD (instead of JPEG)
  m_nMcuWidth = 1;
  m_nMcuHeight = 1;

  // Detailed VLC Decode mode
  m_bDetailVlc = false;
  m_nDetailVlcX = 0;
  m_nDetailVlcY = 0;
  m_nDetailVlcLen = 1;

  m_sHisto.nClipYMin = 0;

  connect(this, SIGNAL(updateImage()), _parent, SLOT(updateImage()));

  // Set up the IDCT lookup tables
  PrecalcIdct();

  GenLookupHuffMask();

  // The following contain information that is set by
  // the JFIF Decoder. We can only reset them here during
  // the constructor and later by explicit call by JFIF Decoder.
  ResetState();

  // We don't call SetPreviewMode() here because it would
  // automatically try to recalculate the view (but nothing ready yet)
  m_nPreviewMode = PREVIEW_RGB;
  SetPreviewZoom(false, false, true, PRV_ZOOM_12);

  m_bDecodeScanAc = true;

  m_bViewOverlaysMcuGrid = false;

  // Start off with no YCC offsets for CalcChannelPreview()
  SetPreviewYccOffset(0, 0, 0, 0, 0);

  SetPreviewMcuInsert(0, 0, 0);

//  setStyleSheet("background-color: white");
}

// Reset decoding state for start of new decode
// Note that we don't touch the DQT or DHT entries as
// those are set at different times versus reset (sometimes
// before Reset() ).
void CimgDecode::Reset()
{
  qDebug() << "CimgDecode::Reset() Start";

  DecodeRestartScanBuf(0, false);
  DecodeRestartDcState();

  m_bRestartRead = false;       // No restarts seen yet
  m_nRestartRead = 0;

  m_nImgSizeXPartMcu = 0;
  m_nImgSizeYPartMcu = 0;
  m_nImgSizeX = 0;
  m_nImgSizeY = 0;
  m_nMcuXMax = 0;
  m_nMcuYMax = 0;
  m_nBlkXMax = 0;
  m_nBlkYMax = 0;

  m_bBrightValid = false;
  m_nBrightY = -32768;
  m_nBrightCb = -32768;
  m_nBrightCr = -32768;
  m_nBrightR = 0;
  m_nBrightG = 0;
  m_nBrightB = 0;
  m_ptBrightMcu.setX(0);
  m_ptBrightMcu.setY(0);

  m_bAvgYValid = false;
  m_nAvgY = 0;

  // If a DIB has been generated, release it!
  if(m_bDibTempReady)
  {
    m_bDibTempReady = false;
  }

  if(m_bDibHistRgbReady)
  {
    m_bDibHistRgbReady = false;
  }

  if(m_bDibHistYReady)
  {
    m_bDibHistYReady = false;
  }

  if(m_pMcuFileMap)
  {
    delete[]m_pMcuFileMap;
    m_pMcuFileMap = 0;
  }

  if(m_pBlkDcValY)
  {
    delete[]m_pBlkDcValY;
    m_pBlkDcValY = 0;
  }

  if(m_pBlkDcValCb)
  {
    delete[]m_pBlkDcValCb;
    m_pBlkDcValCb = 0;
  }

  if(m_pBlkDcValCr)
  {
    delete[]m_pBlkDcValCr;
    m_pBlkDcValCr = 0;
  }

  if(m_pPixValY)
  {
    delete[]m_pPixValY;
    m_pPixValY = 0;
  }
  if(m_pPixValCb)
  {
    delete[]m_pPixValCb;
    m_pPixValCb = 0;
  }
  if(m_pPixValCr)
  {
    delete[]m_pPixValCr;
    m_pPixValCr = 0;
  }

  // Haven't warned about anything yet
  if(!m_bScanErrorsDisable)
  {
    m_nWarnBadScanNum = 0;
  }
  m_nWarnYccClipNum = 0;

  // Reset the view
  m_nPreviewPosX = 0;
  m_nPreviewPosY = 0;
  m_nPreviewSizeX = 0;
  m_nPreviewSizeY = 0;
}

// Destructor for Image Decode class
// - Deallocate any image-related dynamic storage
CimgDecode::~CimgDecode()
{
  if(m_pMcuFileMap)
  {
    delete[]m_pMcuFileMap;
    m_pMcuFileMap = 0;
  }

  if(m_pBlkDcValY)
  {
    delete[]m_pBlkDcValY;
    m_pBlkDcValY = 0;
  }
  if(m_pBlkDcValCb)
  {
    delete[]m_pBlkDcValCb;
    m_pBlkDcValCb = 0;
  }

  if(m_pBlkDcValCr)
  {
    delete[]m_pBlkDcValCr;
    m_pBlkDcValCr = 0;
  }

  if(m_pPixValY)
  {
    delete[]m_pPixValY;
    m_pPixValY = 0;
  }

  if(m_pPixValCb)
  {
    delete[]m_pPixValCb;
    m_pPixValCb = 0;
  }

  if(m_pPixValCr)
  {
    delete[]m_pPixValCr;
    m_pPixValCr = 0;
  }
}

// Reset the major parameters
// - Called by JFIF Decoder when we begin a new file
// TODO: Consider merging with Reset()
// POST:
// - m_anSofSampFactH[]
// - m_anSofSampFactV[]
// - m_bImgDetailsSet
// - m_nNumSofComps
// - m_nPrecision
// - m_bScanErrorsDisable
// - m_nMarkersBlkNum
//
void CimgDecode::ResetState()
{
  ResetDhtLookup();
  ResetDqtTables();

  for(uint32_t nCompInd = 0; nCompInd < MAX_SOF_COMP_NF; nCompInd++)
  {
    m_anSofSampFactH[nCompInd] = 0;
    m_anSofSampFactV[nCompInd] = 0;
  }

  m_bImgDetailsSet = false;
  m_nNumSofComps = 0;

  m_nPrecision = 0;             // Default to "precision not set"

  m_bScanErrorsDisable = false;

  // Reset the markers
  m_nMarkersBlkNum = 0;
}

//void CimgDecode::paintEvent(QPaintEvent *)
//{
//  int32_t top = 0;

//  QRect bRect;

//  QStyleOption opt;
//  opt.init(this);
//  QPainter p(this);
//  style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
//  p.setFont(QFont("Courier New", 14));

//  p.fillRect(0, 0, width(), height(), QColor(Qt::white));

//  if(m_bDibTempReady)
//  {
//    if(!m_bPreviewIsJpeg)
//    {
//      // For all non-JPEG images, report with simple title
//      m_strTitle = "Image";
//    }
//    else
//    {
//      m_strTitle = "Image (";

//      switch (m_nPreviewMode)
//      {
//        case PREVIEW_RGB:
//          m_strTitle += "RGB";
//          break;

//        case PREVIEW_YCC:
//          m_strTitle += "YCC";
//          break;

//        case PREVIEW_R:
//          m_strTitle += "R";
//          break;

//        case PREVIEW_G:
//          m_strTitle += "G";
//          break;

//        case PREVIEW_B:
//          m_strTitle += "B";
//          break;

//        case PREVIEW_Y:
//          m_strTitle += "Y";
//          break;

//        case PREVIEW_CB:
//          m_strTitle += "Cb";
//          break;

//        case PREVIEW_CR:
//          m_strTitle += "Cr";
//          break;

//        default:
//          m_strTitle += "???";
//          break;
//      }

//      if(m_bDecodeScanAc)
//      {
//        m_strTitle += ", DC+AC)";
//      }
//      else
//      {
//        m_strTitle += ", DC)";
//      }
//    }

//    switch (m_nZoomMode)
//    {
//      case PRV_ZOOM_12:
//        m_strTitle += " @ 12.5% (1/8)";
//        break;

//      case PRV_ZOOM_25:
//        m_strTitle += " @ 25% (1/4)";
//        break;

//      case PRV_ZOOM_50:
//        m_strTitle += " @ 50% (1/2)";
//        break;

//      case PRV_ZOOM_100:
//        m_strTitle += " @ 100% (1:1)";
//        break;

//      case PRV_ZOOM_150:
//        m_strTitle += " @ 150% (3:2)";
//        break;

//      case PRV_ZOOM_200:
//        m_strTitle += " @ 200% (2:1)";
//        break;

//      case PRV_ZOOM_300:
//        m_strTitle += " @ 300% (3:1)";
//        break;

//      case PRV_ZOOM_400:
//        m_strTitle += " @ 400% (4:1)";
//        break;

//      case PRV_ZOOM_800:
//        m_strTitle += " @ 800% (8:1)";
//        break;

//      default:
//        m_strTitle += "";
//        break;
//    }

//    // Draw image title
//    bRect = p.boundingRect(QRect(nTitleIndent, top, width(), 100), Qt::AlignLeft | Qt::AlignTop, m_strTitle);
//    p.drawText(bRect, Qt::AlignLeft | Qt::AlignCenter, m_strTitle);

//    top = bRect.height() + nTitleLowGap;
////    p.drawImage(nTitleIndent, top, m_pDibTemp->scaled(m_pDibTemp->width() / 4, m_pDibTemp->height() / 4, Qt::KeepAspectRatio));

////    top += m_pDibTemp->height() + nTitleLowGap;
//    top += nTitleLowGap;
//  }

//  if(m_bHistEn)
//  {
//    if(m_bDibHistRgbReady)
//    {
//      bRect = p.boundingRect(QRect(nTitleIndent, top, width(), 100), Qt::AlignLeft | Qt::AlignTop, "Histogram (RGB)");
//      p.drawText(bRect, Qt::AlignLeft | Qt::AlignCenter, "Histogram (RGB)");
      
//      top += bRect.height() + nTitleLowGap;
//      p.drawImage(nTitleIndent, top, *m_pDibHistRgb);
//      top += m_pDibHistRgb->height() + nTitleLowGap;
//    }

//    if(m_bDibHistYReady)
//    {
//      bRect = p.boundingRect(QRect(nTitleIndent, top, width(), 100), Qt::AlignLeft | Qt::AlignTop, "Histogram (Y)");
//      p.drawText(bRect, Qt::AlignLeft | Qt::AlignCenter, "Histogram (Y)");

//      top += bRect.height() + nTitleLowGap;
//      p.drawImage(nTitleIndent, top, *m_pDibHistY);
//    }
//  }
//}

// Save a copy of the status bar control
//
// INPUT:
// - pStatBar                   = Pointer to status bar
// POST:
// - m_pStatBar
//
void CimgDecode::SetStatusBar(QStatusBar * pStatBar)
{
  m_pStatBar = pStatBar;
}

// Update the status bar text
//
// INPUT:
// - str                                = New text to display on status bar
// PRE:
// - m_pStatBar
//
void CimgDecode::SetStatusText(QString str)
{
  // Make sure that we have been connected to the status
  // bar of the main window first! Note that it is jpegsnoopDoc
  // that sets this variable.
  if(m_pStatBar)
  {
    m_pStatBar->showMessage(str);
  }
}

// Clears the DQT entries
// POST:
// - m_anDqtTblSel[]
// - m_anDqtCoeff[][]
// - m_anDqtCoeffZz[][]
void CimgDecode::ResetDqtTables()
{
  for(uint32_t nDqtComp = 0; nDqtComp < MAX_DQT_COMP; nDqtComp++)
  {
    // Force entries to an invalid value. This makes
    // sure that we have to get a valid SetDqtTables() call
    // from JfifDecode first.
    m_anDqtTblSel[nDqtComp] = -1;
  }

  for(uint32_t nDestId = 0; nDestId < MAX_DQT_DEST_ID; nDestId++)
  {
    for(uint32_t nCoeff = 0; nCoeff < MAX_DQT_COEFF; nCoeff++)
    {
      m_anDqtCoeff[nDestId][nCoeff] = 0;
      m_anDqtCoeffZz[nDestId][nCoeff] = 0;
    }
  }

  m_nNumSofComps = 0;
}

// Reset the DHT lookup tables
// - These tables are used to speed up VLC lookups
// - This should be called by the JFIF decoder any time we start a new file
// POST:
// - m_anDhtLookupSetMax[]
// - m_anDhtLookupSize[][]
// - m_anDhtLookup_bitlen[][][]
// - m_anDhtLookup_bits[][][]
// - m_anDhtLookup_code[][][]
// - m_anDhtLookupfast[][][]
//
void CimgDecode::ResetDhtLookup()
{
  memset(m_anDhtHisto, 0, sizeof(m_anDhtHisto));

  // Use explicit loop ranges instead of memset
  for(uint32_t nClass = DHT_CLASS_DC; nClass <= DHT_CLASS_AC; nClass++)
  {
    m_anDhtLookupSetMax[nClass] = 0;
    // DHT table destination ID is range 0..3
    for(uint32_t nDestId = 0; nDestId < MAX_DHT_DEST_ID; nDestId++)
    {
      m_anDhtLookupSize[nClass][nDestId] = 0;

      for(uint32_t nCodeInd = 0; nCodeInd < MAX_DHT_CODES; nCodeInd++)
      {
        m_anDhtLookup_bitlen[nClass][nDestId][nCodeInd] = 0;
        m_anDhtLookup_bits[nClass][nDestId][nCodeInd] = 0;
        m_anDhtLookup_mask[nClass][nDestId][nCodeInd] = 0;
        m_anDhtLookup_code[nClass][nDestId][nCodeInd] = 0;
      }

      for(uint32_t nElem = 0; nElem < (2 << DHT_FAST_SIZE); nElem++)
      {
        // Mark with invalid value
        m_anDhtLookupfast[nClass][nDestId][nElem] = DHT_CODE_UNUSED;
      }
    }

    for(uint32_t nCompInd = 0; nCompInd < 1 + MAX_SOS_COMP_NS; nCompInd++)
    {
      // Force entries to an invalid value. This makes
      // sure that we have to get a valid SetDhtTables() call
      // from JfifDecode first.
      // Even though nCompInd is supposed to be 1-based numbering,
      // we start at index 0 to ensure it is marked as invalid.
      m_anDhtTblSel[nClass][nCompInd] = -1;
    }
  }

  m_nNumSosComps = 0;
}

// Configure an entry in a quantization table
//
// INPUT:
// - nSet                       = Quant table dest ID (from DQT:Tq)
// - nInd                       = Coeff index (normal order)
// - nIndzz                     = Coeff index (zigzag order)
// - nCoeff                     = Coeff value
// POST:
// - m_anDqtCoeff[]
// - m_anDqtCoeffZz[]
// RETURN:
// - True if params in range, false otherwise
//
// NOTE: Asynchronously called by JFIF Decoder
//
bool CimgDecode::SetDqtEntry(uint32_t nTblDestId, uint32_t nCoeffInd, uint32_t nCoeffIndZz, uint16_t nCoeffVal)
{
  if((nTblDestId < MAX_DQT_DEST_ID) && (nCoeffInd < MAX_DQT_COEFF))
  {
    m_anDqtCoeff[nTblDestId][nCoeffInd] = nCoeffVal;

    // Save a copy that represents the original zigzag order
    // This is used by the IDCT logic
    m_anDqtCoeffZz[nTblDestId][nCoeffIndZz] = nCoeffVal;

  }
  else
  {
    // Should never get here!
    QString strTmp;

    strTmp = QString("ERROR: Attempt to set DQT entry out of range (nTblDestId = %1, nCoeffInd = %2, nCoeffVal = %3")
        .arg(nTblDestId)
        .arg(nCoeffInd)
        .arg(nCoeffVal);

#ifdef DEBUG_LOG
    QString strDebug;

    strDebug = QString("## File = %1 Block = %2 Error = %3")
        .arg(m_pAppConfig->strCurFname, -100)
        .arg("ImgDecode", -10)
        .arg(strTmp);
    qDebug() << strDebug;
#else
    Q_ASSERT(false);
#endif

    if(m_pAppConfig->interactive())
    {
      msgBox.setText(strTmp);
      msgBox.exec();
    }

    return false;
  }

  return true;
}

// Fetch a DQT table entry
//
// INPUT:
// - nTblDestId                         = DQT Table Destination ID
// - nCoeffInd                          = Coefficient index in 8x8 matrix
// PRE:
// - m_anDqtCoeff[][]
// RETURN:
// - Returns the indexed DQT matrix entry
//
uint32_t CimgDecode::GetDqtEntry(uint32_t nTblDestId, uint32_t nCoeffInd)
{
  if((nTblDestId < MAX_DQT_DEST_ID) && (nCoeffInd < MAX_DQT_COEFF))
  {
    return m_anDqtCoeff[nTblDestId][nCoeffInd];
  }
  else
  {
    // Should never get here!
    QString strTmp;

    strTmp = QString("ERROR: GetDqtEntry(nTblDestId = %1, nCoeffInd = %2").arg(nTblDestId).arg(nCoeffInd);
    m_pLog->AddLineErr(strTmp);

    if(m_pAppConfig->interactive())
    {
      msgBox.setText(strTmp);
      msgBox.exec();
    }

#ifdef DEBUG_LOG
    qDebug() << QString("## File = %1 Block = %2 Error = %3").arg(m_pAppConfig->strCurFname).arg("ImgDecode").arg(strTmp);
#else
    Q_ASSERT(false);
#endif

    return 0;
  }
}

// Set a DQT table for a frame image component identifier
//
// INPUT:
// - nCompInd                   = Component index. Based on m_nSofNumComps_Nf-1 (ie. 0..254)
// - nTbl                               = DQT Table number. Based on SOF:Tqi (ie. 0..3)
// POST:
// - m_anDqtTblSel[]
// RETURN:
// - Success if index and table are in range
// NOTE:
// - Asynchronously called by JFIF Decoder
//
bool CimgDecode::SetDqtTables(uint32_t nCompId, uint32_t nTbl)
{
  QString strTmp;

  if((nCompId < MAX_SOF_COMP_NF) && (nTbl < MAX_DQT_DEST_ID))
  {
    m_anDqtTblSel[nCompId] = static_cast<int32_t>(nTbl);
  }
  else
  {
    // Should never get here unless the JFIF SOF table has a bad entry!
    strTmp = QString("ERROR: SetDqtTables(Comp ID = %1, Table = %2")
        .arg(nCompId)
        .arg(nTbl);
    m_pLog->AddLineErr(strTmp);

    if(m_pAppConfig->interactive())
    {
      msgBox.setText(strTmp);
      msgBox.exec();
    }

    return false;
  }

  return true;
}

// Set a DHT table for a scan image component index
// - The DHT Table select array is stored as:
//   m_anDhtTblSel[0][1,2,3] for DC
//   m_anDhtTblSel[1][1,2,3] for AC
//
// INPUT:
// - nCompInd                   = Component index (1-based). Range 1..4
// - nTblDc                             = DHT table index for DC elements of component
// - nTblAc                             = DHT table index for AC elements of component
// POST:
// - m_anDhtTblSel[][]
// RETURN:
// - Success if indices are in range
//
bool CimgDecode::SetDhtTables(uint32_t nCompInd, uint32_t nTblDc, uint32_t nTblAc)
{
  QString strTmp;

  // Note use of (nCompInd < MAX_SOS_COMP_NS+1) as nCompInd is 1-based notation
  if((nCompInd >= 1) && (nCompInd < MAX_SOS_COMP_NS + 1) && (nTblDc < MAX_DHT_DEST_ID) && (nTblAc < MAX_DHT_DEST_ID))
  {
    m_anDhtTblSel[DHT_CLASS_DC][nCompInd] = static_cast<int32_t>(nTblDc);
    m_anDhtTblSel[DHT_CLASS_AC][nCompInd] = static_cast<int32_t>(nTblAc);
  }
  else
  {
    // Should never get here!
    strTmp = QString("ERROR: SetDhtTables(comp = %1, TblDC = %2 TblAC = %3) out of indexed range")
        .arg(nCompInd)
        .arg(nTblDc)
        .arg(nTblAc);
    m_pLog->AddLineErr(strTmp);

    if(m_pAppConfig->interactive())
    {
      msgBox.setText(strTmp);
      msgBox.exec();
    }

    return false;
  }

  return true;
}

// Get the precision field
//
// INPUT:
// - nPrecision                 = DCT sample precision (typically 8 or 12)
// POST:
// - m_nPrecision
//
void CimgDecode::SetPrecision(uint32_t nPrecision)
{
  m_nPrecision = nPrecision;
}

// Set the general image details for the image decoder
//
// INPUT:
// - nDimX                              = Image dimension (X)
// - nDimY                              = Image dimension (Y)
// - nCompsSOF                  = Number of components in Frame
// - nCompsSOS                  = Number of components in Scan
// - bRstEn                             = Restart markers present?
// - nRstInterval               = Restart marker interval
// POST:
// - m_bImgDetailsSet
// - m_nDimX
// - m_nDimY
// - m_nNumSofComps
// - m_nNumSosComps
// - m_bRestartEn
// - m_nRestartInterval
// NOTE:
// - Called asynchronously by the JFIF decoder
//
void CimgDecode::SetImageDetails(uint32_t nDimX, uint32_t nDimY, uint32_t nCompsSOF, uint32_t nCompsSOS, bool bRstEn,
                                 uint32_t nRstInterval)
{
  m_bImgDetailsSet = true;
  m_nDimX = nDimX;
  m_nDimY = nDimY;
  m_nNumSofComps = nCompsSOF;
  m_nNumSosComps = nCompsSOS;
  m_bRestartEn = bRstEn;
  m_nRestartInterval = nRstInterval;
}

// Reset the image content to prepare it for the upcoming scans
// - TODO: Migrate pixel bitmap allocation / clearing from DecodeScanImg() to here
void CimgDecode::ResetImageContent()
{
}

// Set the sampling factor for an image component
//
// INPUT:
// - nCompInd                   = Component index from Nf (ie. 1..255)
// - nSampFactH                 = Sampling factor in horizontal direction
// - nSampFactV                 = Sampling factor in vertical direction
// POST:
// - m_anSofSampFactH[]
// - m_anSofSampFactV[]
// NOTE:
// - Called asynchronously by the JFIF decoder in SOF
//
void CimgDecode::SetSofSampFactors(uint32_t nCompInd, uint32_t nSampFactH, uint32_t nSampFactV)
{
  // TODO: Check range
  m_anSofSampFactH[nCompInd] = nSampFactH;
  m_anSofSampFactV[nCompInd] = nSampFactV;
}

// Update the preview mode (affects channel display)
//
// INPUT:
// - nMode                              = Mode used in channel display (eg. NONE, RGB, YCC)
//                        See PREVIEW_* constants
//
void CimgDecode::setPreviewMode(QAction *action)
{
  // Need to check to see if mode has changed. If so, we need to recalculate the temporary preview.
  m_nPreviewMode = action->data().toInt();
  CalcChannelPreview();
  emit updateImage();
}

// Update any level shifts for the preview display
//
// INPUT:
// - nMcuX                              = MCU index in X direction
// - nMcuY                              = MCU index in Y direction
// - nY                                 = DC shift in Y component
// - nCb                                = DC shift in Cb component
// - nCr                                = DC shift in Cr component
//
void CimgDecode::SetPreviewYccOffset(uint32_t nMcuX, uint32_t nMcuY, int32_t nY, int32_t nCb, int32_t nCr)
{
  m_nPreviewShiftY = nY;
  m_nPreviewShiftCb = nCb;
  m_nPreviewShiftCr = nCr;
  m_nPreviewShiftMcuX = nMcuX;
  m_nPreviewShiftMcuY = nMcuY;

  CalcChannelPreview();
}

// Fetch the current level shift setting for the preview display
//
// OUTPUT:
// - nMcuX                              = MCU index in X direction
// - nMcuY                              = MCU index in Y direction
// - nY                                 = DC shift in Y component
// - nCb                                = DC shift in Cb component
// - nCr                                = DC shift in Cr component
//
void CimgDecode::GetPreviewYccOffset(uint32_t &nMcuX, uint32_t &nMcuY, int32_t &nY, int32_t &nCb, int32_t &nCr)
{
  nY = m_nPreviewShiftY;
  nCb = m_nPreviewShiftCb;
  nCr = m_nPreviewShiftCr;
  nMcuX = m_nPreviewShiftMcuX;
  nMcuY = m_nPreviewShiftMcuY;
}

// Set the Preview MCU insert
// UNUSED
void CimgDecode::SetPreviewMcuInsert(uint32_t nMcuX, uint32_t nMcuY, int32_t nLen)
{

  m_nPreviewInsMcuX = nMcuX;
  m_nPreviewInsMcuY = nMcuY;
  m_nPreviewInsMcuLen = nLen;

  CalcChannelPreview();
}

// Get the Preview MCU insert
// UNUSED
void CimgDecode::GetPreviewMcuInsert(uint32_t &nMcuX, uint32_t &nMcuY, uint32_t &nLen)
{
  nMcuX = m_nPreviewInsMcuX;
  nMcuY = m_nPreviewInsMcuY;
  nLen = m_nPreviewInsMcuLen;
}

// Fetch the coordinate of the top-left corner of the preview image
//
// OUTPUT:
// - nX                         = X coordinate of top-left corner
// - nY                         = Y coordinate of top-left corner
//
void CimgDecode::GetPreviewPos(uint32_t &nX, uint32_t &nY)
{
  nX = m_nPreviewPosX;
  nY = m_nPreviewPosY;
}

// Fetch the dimensions of the preview image
//
// OUTPUT:
// - nX                         = X dimension of image
// - nY                         = Y dimension of iamge
//
void CimgDecode::GetPreviewSize(uint32_t &nX, uint32_t &nY)
{
  nX = m_nPreviewSizeX;
  nY = m_nPreviewSizeY;
}

// Set a DHT table entry and associated lookup table
// - Configures the fast lookup table for short code bitstrings
//
// INPUT:
// - nDestId                    = DHT destination table ID (0..3)
// - nClass                             = Select between DC and AC tables (0=DC, 1=AC)
// - nInd                               = Index into table
// - nLen                               = Huffman code length
// - nBits                              = Huffman code bitstring (left justified)
// - nMask                              = Huffman code bit mask (left justified)
// - nCode                              = Huffman code value
// POST:
// - m_anDhtLookup_bitlen[][][]
// - m_anDhtLookup_bits[][][]
// - m_anDhtLookup_mask[][][]
// - m_anDhtLookup_code[][][]
// - m_anDhtLookupSetMax[]
// - m_anDhtLookupfast[][][]
// RETURN:
// - Success if indices are in range
// NOTE:
// - Asynchronously called by JFIF Decoder
//
bool CimgDecode::SetDhtEntry(uint32_t nDestId, uint32_t nClass, uint32_t nInd, uint32_t nLen,
                             uint32_t nBits, uint32_t nMask, uint32_t nCode)
{
  if((nDestId >= MAX_DHT_DEST_ID) || (nClass >= MAX_DHT_CLASS) || (nInd >= MAX_DHT_CODES))
  {
    QString strTmp = "ERROR: Attempt to set DHT entry out of range";

    m_pLog->AddLineErr(strTmp);

    if(m_pAppConfig->interactive())
    {
      msgBox.setText(strTmp);
      msgBox.exec();
    }
#ifdef DEBUG_LOG
    QString strDebug;

    strDebug = QString("## File = %1 Block = %2 Error = %3")
        .arg(m_pAppConfig->strCurFname, -100)
        .arg("ImgDecode", -10)
        .arg(strTmp);
    qDebug() << strDebug;
#else
    Q_ASSERT(false);
#endif
    return false;
  }

  m_anDhtLookup_bitlen[nClass][nDestId][nInd] = nLen;
  m_anDhtLookup_bits[nClass][nDestId][nInd] = nBits;
  m_anDhtLookup_mask[nClass][nDestId][nInd] = nMask;
  m_anDhtLookup_code[nClass][nDestId][nInd] = nCode;

  // Record the highest numbered DHT set.
  // TODO: Currently assuming that there are no missing tables in the sequence
  if(nDestId > m_anDhtLookupSetMax[nClass])
  {
    m_anDhtLookupSetMax[nClass] = nDestId;
  }

  uint32_t nBitsMsb;
  uint32_t nBitsExtraLen;
  uint32_t nBitsExtraVal;
  uint32_t nBitsMax;
  uint32_t nFastVal;

  // If the variable-length code is short enough, add it to the
  // fast lookup table.
  if(nLen <= DHT_FAST_SIZE)
  {
    // nBits is a left-justified number (assume right-most bits are zero)
    // nLen is number of leading bits to compare

    //   nLen     = 5
    //   nBits    = 32'b1011_1xxx_xxxx_xxxx_xxxx_xxxx_xxxx_xxxx  (0xB800_0000)
    //   nBitsMsb =  8'b1011_1xxx (0xB8)
    //   nBitsMsb =  8'b1011_1000 (0xB8)
    //   nBitsMax =  8'b1011_1111 (0xBF)

    //   nBitsExtraLen = 8-len = 8-5 = 3

    //   nBitsExtraVal = (1<<nBitsExtraLen) -1 = 1<<3 -1 = 8'b0000_1000 -1 = 8'b0000_0111
    //
    //   nBitsMax = nBitsMsb + nBitsExtraVal
    //   nBitsMax =  8'b1011_1111
    nBitsMsb = (nBits & nMask) >> (32 - DHT_FAST_SIZE);
    nBitsExtraLen = DHT_FAST_SIZE - nLen;
    nBitsExtraVal = (1 << nBitsExtraLen) - 1;
    nBitsMax = nBitsMsb + nBitsExtraVal;

    for(uint32_t ind1 = nBitsMsb; ind1 <= nBitsMax; ind1++)
    {
      // The arrangement of the dht_lookupfast[] values:
      //                         0xFFFF_FFFF = No entry, resort to slow search
      //    m_anDhtLookupfast[nClass][nDestId] [15: 8] = nLen
      //    m_anDhtLookupfast[nClass][nDestId] [ 7: 0] = nCode
      //
      //Q_ASSERT(code <= 0xFF);
      //Q_ASSERT(ind1 <= 0xFF);
      nFastVal = nCode + (nLen << 8);
      m_anDhtLookupfast[nClass][nDestId][ind1] = nFastVal;
    }
  }

  return true;
}

// Assign the size of the DHT table
//
// INPUT:
// - nDestId                    = Destination DHT table ID (From DHT:Th, range 0..3)
// - nClass                             = Select between DC and AC tables (0=DC, 1=AC) (From DHT:Tc, range 0..1)
// - nSize                              = Number of entries in the DHT table
// POST:
// - m_anDhtLookupSize[][]
// RETURN:
// - Success if indices are in range
//
bool CimgDecode::SetDhtSize(uint32_t nDestId, uint32_t nClass, uint32_t nSize)
{
  if((nDestId >= MAX_DHT_DEST_ID) || (nClass >= MAX_DHT_CLASS) || (nSize >= MAX_DHT_CODES))
  {
    QString strTmp = "ERROR: Attempt to set DHT table size out of range";

    m_pLog->AddLineErr(strTmp);

    if(m_pAppConfig->interactive())
    {
      msgBox.setText(strTmp);
      msgBox.exec();
    }

    return false;
  }
  else
  {
    m_anDhtLookupSize[nClass][nDestId] = nSize;
  }

  return true;
}

// Convert huffman code (DC) to signed value
// - Convert according to ITU-T.81 Table 5
//
// INPUT:
// - nVal                               = Huffman DC value (left justified)
// - nBits                              = Bitstring length of value
// RETURN:
// - Signed integer representing Huffman DC value
//
int32_t CimgDecode::HuffmanDc2Signed(uint32_t nVal, uint32_t nBits)
{
  if(nVal >= (1 << (nBits - 1)))
  {
    return static_cast<int32_t>(nVal);
  }
  else
  {
    return static_cast<int32_t>(nVal - ((1 << nBits) - 1));
  }
}

// Generate the Huffman code lookup table mask
//
// POST:
// - m_anHuffMaskLookup[]
//
void CimgDecode::GenLookupHuffMask()
{
  uint32_t nMask;

  for(uint32_t nLen = 0; nLen < 32; nLen++)
  {
    nMask = (1 << (nLen)) - 1;
    nMask <<= 32 - nLen;
    m_anHuffMaskLookup[nLen] = nMask;
  }
}

// Extract a specified number of bits from a 32-bit holding register
//
// INPUT:
// - nWord                              = The 32-bit holding register
// - nBits                              = Number of bits (leftmost) to extract from the holding register
// PRE:
// - m_anHuffMaskLookup[]
// RETURN:
// - The subset of bits extracted from the holding register
// NOTE:
// - This routine is inlined for speed
//
inline uint32_t CimgDecode::ExtractBits(uint32_t nWord, uint32_t nBits)
{
  uint32_t nVal;

  nVal = (nWord & m_anHuffMaskLookup[nBits]) >> (32 - nBits);
  return nVal;
}

// Removes bits from the holding buffer
// - Discard the leftmost "nNumBits" of m_nScanBuff
// - And then realign file position pointers
//
// INPUT:
// - nNumBits                           = Number of left-most bits to consume
// POST:
// - m_nScanBuff
// - m_nScanBuff_vacant
// - m_nScanBuffPtr_align
// - m_anScanBuffPtr_pos[]
// - m_anScanBuffPtr_err[]
// - m_nScanBuffLatchErr
// - m_nScanBuffPtr_num
//
inline void CimgDecode::ScanBuffConsume(uint32_t nNumBits)
{
  m_nScanBuff <<= nNumBits;
  m_nScanBuff_vacant += nNumBits;

  // Need to latch any errors during consumption of multi-bytes
  // otherwise we'll miss the error notification if we skip over
  // it before we exit this routine! e.g if num_bytes=2 and error
  // appears on first byte, then we want to retain it in pos[0]

  // Now realign the file position pointers if necessary
  uint32_t nNumBytes = (m_nScanBuffPtr_align + nNumBits) / 8;

  for(uint32_t nInd = 0; nInd < nNumBytes; nInd++)
  {
    m_anScanBuffPtr_pos[0] = m_anScanBuffPtr_pos[1];
    m_anScanBuffPtr_pos[1] = m_anScanBuffPtr_pos[2];
    m_anScanBuffPtr_pos[2] = m_anScanBuffPtr_pos[3];
    // Don't clear the last ptr position because during an overread
    // this will be the only place that the last position was preserved
    //m_anScanBuffPtr_pos[3] = 0;

    m_anScanBuffPtr_err[0] = m_anScanBuffPtr_err[1];
    m_anScanBuffPtr_err[1] = m_anScanBuffPtr_err[2];
    m_anScanBuffPtr_err[2] = m_anScanBuffPtr_err[3];
    m_anScanBuffPtr_err[3] = SCANBUF_OK;

    if(m_anScanBuffPtr_err[0] != SCANBUF_OK)
    {
      m_nScanBuffLatchErr = m_anScanBuffPtr_err[0];
    }

    m_nScanBuffPtr_num--;
  }
  
  m_nScanBuffPtr_align = (m_nScanBuffPtr_align + nNumBits) % 8;
}

// Augment the current scan buffer with another byte
// - Extra bits are added to right side of existing bitstring
//
// INPUT:
// - nNewByte                 = 8-bit byte to add to buffer
// - nPtr                               = UNUSED
// PRE:
// - m_nScanBuff
// - m_nScanBuff_vacant
// - m_nScanBuffPtr_num
// POST:
// - m_nScanBuff
// - m_nScanBuff_vacant
// - m_anScanBuffPtr_err[]
// - m_anScanBuffPtr_pos[]
//
inline void CimgDecode::ScanBuffAdd(uint32_t nNewByte, uint32_t nPtr)
{
  // Add the new byte to the buffer
  // Assume that m_nScanBuff has already been shifted to be
  // aligned to bit 31 as first bit.
  m_nScanBuff += (nNewByte << (m_nScanBuff_vacant - 8));
  m_nScanBuff_vacant -= 8;

  Q_ASSERT(m_nScanBuffPtr_num < 4);
  
  if(m_nScanBuffPtr_num >= 4)
  {
    return;
  }                             // Unexpected by design
  
  m_anScanBuffPtr_err[m_nScanBuffPtr_num] = SCANBUF_OK;
  m_anScanBuffPtr_pos[m_nScanBuffPtr_num++] = nPtr;

  // m_nScanBuffPtr_align stays the same as we add 8 bits
}

// Augment the current scan buffer with another byte (but mark as error)
//
// INPUT:
// - nNewByte                 = 8-bit byte to add to buffer
// - nPtr                               = UNUSED
// - nErr                               = Error code to associate with this buffer byte
// POST:
// - m_anScanBuffPtr_err[]
//
inline void CimgDecode::ScanBuffAddErr(uint32_t nNewByte, uint32_t nPtr, uint32_t nErr)
{
  ScanBuffAdd(nNewByte, nPtr);
  m_anScanBuffPtr_err[m_nScanBuffPtr_num - 1] = nErr;
}

// Disable any further reporting of scan errors
//
// PRE:
// - m_nScanErrMax
// POST:
// - m_nWarnBadScanNum
// - m_bScanErrorsDisable
//
void CimgDecode::ScanErrorsDisable()
{
  m_nWarnBadScanNum = m_nScanErrMax;
  m_bScanErrorsDisable = true;
}

// Enable reporting of scan errors
//
// POST:
// - m_nWarnBadScanNum
// - m_bScanErrorsDisable
//
void CimgDecode::ScanErrorsEnable()
{
  m_nWarnBadScanNum = 0;
  m_bScanErrorsDisable = false;
}

// Read in bits from the buffer and find matching huffman codes
// - Input buffer is m_nScanBuff
// - Perform shift in buffer afterwards
// - Abort if there aren't enough bits (note that the scan buffer
//   is almost empty when we reach the end of a scan segment)
//
// INPUT:
// - nClass                                     = DHT Table class (0..1)
// - nTbl                                       = DHT Destination ID (0..3)
// PRE:
// - Assume that dht_lookup_size[nTbl] != 0 (already checked)
// - m_bRestartRead
// - m_nScanBuff_vacant
// - m_nScanErrMax
// - m_nScanBuff
// - m_anDhtLookupFast[][][]
// - m_anDhtLookupSize[][]
// - m_anDhtLookup_mask[][][]
// - m_anDhtLookup_bits[][][]
// - m_anDhtLookup_bitlen[][][]
// - m_anDhtLookup_code[][][]
// - m_nPrecision
// POST:
// - m_nScanBitsUsed# is calculated
// - m_bScanEnd
// - m_bScanBad
// - m_nWarnBadScanNum
// - m_anDhtHisto[][][]
// OUTPUT:
// - rZrl                               = Zero run amount (if any)
// - rVal                               = Coefficient value
// RETURN:
// - Status from attempting to decode the current value
//
// PERFORMANCE:
// - Tried to unroll all function calls listed below,
//   but performance was same before & after (27sec).
// - Calls unrolled: BuffTopup(), ScanBuffConsume(),
//                   ExtractBits(), HuffmanDc2Signed()
teRsvRet CimgDecode::ReadScanVal(uint32_t nClass, uint32_t nTbl, uint32_t &rZrl, int32_t &rVal)
{
  bool bDone = false;

  uint32_t nInd = 0;

  uint32_t nCode = DHT_CODE_UNUSED;     // Not a valid nCode

  uint32_t nVal;

  Q_ASSERT(nClass < MAX_DHT_CLASS);
  Q_ASSERT(nTbl < MAX_DHT_DEST_ID);

  m_nScanBitsUsed1 = 0;         // bits consumed for nCode
  m_nScanBitsUsed2 = 0;

  rZrl = 0;
  rVal = 0;

  // First check to see if we've entered here with a completely empty
  // scan buffer with a restart marker already observed. In that case
  // we want to exit with condition 3 (restart terminated)
  if((m_nScanBuff_vacant == 32) && (m_bRestartRead))
  {
    return RSV_RST_TERM;
  }

  // Has the scan buffer been depleted?
  if(m_nScanBuff_vacant >= 32)
  {
    // Trying to overread end of scan segment

    if(m_nWarnBadScanNum < m_nScanErrMax)
    {
      QString strTmp;

      strTmp = QString("*** ERROR: Overread scan segment (before nCode)! @ Offset: %1").arg(GetScanBufPos());
      m_pLog->AddLineErr(strTmp);

      m_nWarnBadScanNum++;

      if(m_nWarnBadScanNum >= m_nScanErrMax)
      {
        strTmp = QString("    Only reported first %1 instances of this message...").arg(m_nScanErrMax);
        m_pLog->AddLineErr(strTmp);
      }
    }

    m_bScanEnd = true;
    m_bScanBad = true;
    return RSV_UNDERFLOW;
  }

  // Top up the buffer just in case
  BuffTopup();

  bDone = false;
  bool bFound = false;

  // Fast search for variable-length huffman nCode
  // Do direct lookup for any codes DHT_FAST_SIZE bits or shorter

  uint32_t nCodeMsb;

  uint32_t nCodeFastSearch;

  // Only enable this fast search if m_nScanBuff_vacant implies
  // that we have at least DHT_FAST_SIZE bits available in the buffer!
  if((32 - m_nScanBuff_vacant) >= DHT_FAST_SIZE)
  {
    nCodeMsb = m_nScanBuff >> (32 - DHT_FAST_SIZE);
    nCodeFastSearch = m_anDhtLookupfast[nClass][nTbl][nCodeMsb];
    
    if(nCodeFastSearch != DHT_CODE_UNUSED)
    {
      // We found the code!
      m_nScanBitsUsed1 += (nCodeFastSearch >> 8);
      nCode = (nCodeFastSearch & 0xFF);
      bDone = true;
      bFound = true;
    }
  }

  // Slow search for variable-length huffman nCode
  while(!bDone)
  {
    uint32_t nBitLen;

    if((m_nScanBuff & m_anDhtLookup_mask[nClass][nTbl][nInd]) == m_anDhtLookup_bits[nClass][nTbl][nInd])
    {

      nBitLen = m_anDhtLookup_bitlen[nClass][nTbl][nInd];
      
      // Just in case this VLC bit string is longer than the number of
      // bits we have left in the buffer (due to restart marker or end
      // of scan data), we need to double-check
      if(nBitLen <= 32 - m_nScanBuff_vacant)
      {
        nCode = m_anDhtLookup_code[nClass][nTbl][nInd];
        m_nScanBitsUsed1 += nBitLen;
        bDone = true;
        bFound = true;
      }
    }
    
    nInd++;
    
    if(nInd >= m_anDhtLookupSize[nClass][nTbl])
    {
      bDone = true;
    }
  }

  // Could not find huffman nCode in table!
  if(!bFound)
  {
    // If we didn't find the huffman nCode, it might be due to a
    // restart marker that prematurely stopped the scan buffer filling.
    // If so, return with an indication so that DecodeScanComp() can
    // handle the restart marker, refill the scan buffer and then
    // re-do ReadScanVal()
    if(m_bRestartRead)
    {
      return RSV_RST_TERM;
    }

    // FIXME:
    // What should we be consuming here? we need to make forward progress
    // in file. Options:
    // 1) Move forward to next byte in file
    // 2) Move forward to next bit in file
    // Currently moving 1 bit so that we have slightly more opportunities
    // to re-align earlier.
    m_nScanBitsUsed1 = 1;
    nCode = DHT_CODE_UNUSED;
  }

  // Log an entry into a histogram
  if(m_nScanBitsUsed1 < MAX_DHT_CODELEN + 1)
  {
    m_anDhtHisto[nClass][nTbl][m_nScanBitsUsed1]++;
  }
  else
  {
    Q_ASSERT(false);
    // Somehow we got out of range
  }

  ScanBuffConsume(m_nScanBitsUsed1);

  // Did we overread the scan buffer?
  if(m_nScanBuff_vacant > 32)
  {
    // The nCode consumed more bits than we had!
    QString strTmp;

    strTmp = QString("*** ERROR: Overread scan segment (after nCode)! @ Offset: %1").arg(GetScanBufPos());
    m_pLog->AddLineErr(strTmp);
    m_bScanEnd = true;
    m_bScanBad = true;
    return RSV_UNDERFLOW;
  }

  // Replenish buffer after nCode extraction and before variable extra bits
  // This is required because we may have a 12 bit nCode followed by a 16 bit var length bitstring
  BuffTopup();

  // Did we find the nCode?
  if(nCode != DHT_CODE_UNUSED)
  {
    rZrl = (nCode & 0xF0) >> 4;
    m_nScanBitsUsed2 = nCode & 0x0F;
    
    if((rZrl == 0) && (m_nScanBitsUsed2 == 0))
    {
      // EOB (was bits_extra=0)
      return RSV_EOB;
    }
    else if(m_nScanBitsUsed2 == 0)
    {
      // Zero rValue
      rVal = 0;
      return RSV_OK;

    }
    else
    {
      // Normal nCode
      nVal = ExtractBits(m_nScanBuff, m_nScanBitsUsed2);
      rVal = HuffmanDc2Signed(nVal, m_nScanBitsUsed2);

      // Now handle the different precision values
      // Treat 12-bit like 8-bit but scale values first (ie. drop precision down to 8-bit)
      int32_t nPrecisionDivider = 1;

      if(m_nPrecision >= 8)
      {
        nPrecisionDivider = 1 << (m_nPrecision - 8);
        rVal /= nPrecisionDivider;
      }
      else
      {
        // Precision value seems out of range!
      }

      ScanBuffConsume(m_nScanBitsUsed2);

      // Did we overread the scan buffer?
      if(m_nScanBuff_vacant > 32)
      {
        // The nCode consumed more bits than we had!
        QString strTmp;

        strTmp = QString("*** ERROR: Overread scan segment (after bitstring)! @ Offset: %1").arg(GetScanBufPos());
        m_pLog->AddLineErr(strTmp);
        m_bScanEnd = true;
        m_bScanBad = true;
        return RSV_UNDERFLOW;
      }

      return RSV_OK;
    }
  }
  else
  {
    // ERROR: Invalid huffman nCode!

    // FIXME: We may also enter here if we are about to encounter a
    // restart marker! Need to see if ScanBuf is terminated by
    // restart marker; if so, then we simply flush the ScanBuf,
    // consume the 2-byte RST marker, clear the ScanBuf terminate
    // reason, then indicate to caller that they need to call ReadScanVal
    // again.

    if(m_nWarnBadScanNum < m_nScanErrMax)
    {
      QString strTmp;

      strTmp =
        QString("*** ERROR: Can't find huffman bitstring @ %1, table %2, value 0x%3").arg(GetScanBufPos()).arg(nTbl).
        arg(m_nScanBuff, 8, 16, QChar('0'));
      m_pLog->AddLineErr(strTmp);

      m_nWarnBadScanNum++;

      if(m_nWarnBadScanNum >= m_nScanErrMax)
      {
        strTmp = QString("    Only reported first %1 instances of this message...").arg(m_nScanErrMax);
        m_pLog->AddLineErr(strTmp);
      }
    }

    // TODO: What rValue and ZRL should we return?
    m_bScanBad = true;
    return RSV_UNDERFLOW;
  }

  // NOTE: Can't reach here
  // return RSV_UNDERFLOW;
}

// Refill the scan buffer as needed
//
void CimgDecode::BuffTopup()
{
  uint32_t nRetVal;

  // Do we have space to load another byte?
  bool bDone = (m_nScanBuff_vacant < 8);

  // Have we already reached the end of the scan segment?
  if(m_bScanEnd)
  {
    bDone = true;
  }

  while(!bDone)
  {
    nRetVal = BuffAddByte();

    // NOTE: If we have read in a restart marker, the above call will not
    // read in any more bits into the scan buffer, so we should just simply
    // say that we've done the best we can for the top up.
    if(m_bRestartRead)
    {
      bDone = true;
    }

    if(m_nScanBuff_vacant < 8)
    {
      bDone = true;
    }
    
    // If the buffer read returned an error or end of scan segment
    // then stop filling buffer
    if(nRetVal != 0)
    {
      bDone = true;
    }
  }
}

// Check for restart marker and compare the index against expected
//
// PRE:
// - m_nScanBuffPtr
// RETURN:
// - Restart marker found at the current buffer position
// NOTE:
// - This routine expects that we did not remove restart markers
//   from the bytestream (in BuffAddByte).
//
bool CimgDecode::ExpectRestart()
{
  uint32_t nMarker;
  uint32_t nBuf0 = m_pWBuf->Buf(m_nScanBuffPtr);
  uint32_t nBuf1 = m_pWBuf->Buf(m_nScanBuffPtr + 1);

  // Check for restart marker first. Assume none back-to-back.
  if(nBuf0 == 0xFF)
  {
    nMarker = nBuf1;

    // FIXME: Later, check that we are on the right marker!
    if((nMarker >= JFIF_RST0) && (nMarker <= JFIF_RST7))
    {
      if(m_bVerbose)
      {
        QString strTmp;

        strTmp =
          QString("  RESTART marker: @ 0x%1.0 : RST%2")
            .arg(m_nScanBuffPtr, 8, 16, QChar('0'))
            .arg(nMarker - JFIF_RST0, 2, 10, QChar('0'));
        m_pLog->AddLine(strTmp);
      }

      m_nRestartRead++;

      // FIXME:
      //     Later on, we should be checking for RST out of
      //     sequence!

      // Now we need to skip to the next bytes
      m_nScanBuffPtr += 2;

      // Now refill in the buffer if we need to
      BuffAddByte();

      return true;
    }
  }

  return false;
}

// Add a byte to the scan buffer from the file
// - Handle stuff bytes
// - Handle restart markers
//
// PRE:
// - m_nScanBuffPtr
// POST:
// - m_nRestartRead
// - m_nRestartLastInd
//
uint32_t CimgDecode::BuffAddByte()
{
  uint32_t nMarker = 0x00;
  uint32_t nBuf0, nBuf1;

  // If we have already read in a restart marker but not
  // handled it yet, then simply return without reading any
  // more bytes
  if(m_bRestartRead)
  {
    return 0;
  }

  nBuf0 = m_pWBuf->Buf(m_nScanBuffPtr);
  nBuf1 = m_pWBuf->Buf(m_nScanBuffPtr + 1);

  // Check for restart marker first. Assume none back-to-back.
  if(nBuf0 == 0xFF)
  {
    nMarker = nBuf1;

    if((nMarker >= JFIF_RST0) && (nMarker <= JFIF_RST7))
    {
      if(m_bVerbose)
      {
        QString strTmp;

        strTmp =
          QString("  RESTART marker: @ 0x%1.0 : RST%2")
            .arg(m_nScanBuffPtr, 8, 16, QChar('0'))
            .arg(nMarker - JFIF_RST0, 2, 10, QChar('0'));
        m_pLog->AddLine(strTmp);
      }

      m_nRestartRead++;
      m_nRestartLastInd = nMarker - JFIF_RST0;

      if(m_nRestartLastInd != m_nRestartExpectInd)
      {
        if(!m_bScanErrorsDisable)
        {
          QString strTmp;

          strTmp =
            QString("  ERROR: Expected RST marker index RST%1 got RST%2 @ 0x%3.0")
              .arg(m_nRestartExpectInd)
              .arg(m_nRestartLastInd)
              .arg(m_nScanBuffPtr, 8, 16, QChar('0'));
          m_pLog->AddLineErr(strTmp);
        }
      }

      m_nRestartExpectInd = (m_nRestartLastInd + 1) % 8;

      // FIXME: Later on, we should be checking for RST out of sequence

      // END BUFFER READ HERE!
      // Indicate that a Restart marker has been seen, which
      // will prevent further reading of scan buffer until it
      // has been handled.
      m_bRestartRead = true;

      return 0;

/*
      // Now we need to skip to the next bytes
			m_nScanBuffPtr+=2;

			// Update local saved buffer vars
			nBuf0 = m_pWBuf->Buf(m_nScanBuffPtr);
			nBuf1 = m_pWBuf->Buf(m_nScanBuffPtr+1);

      // Use ScanBuffAddErr() to mark this byte as being after Restart marker!
			m_anScanBuffPtr_err[m_nScanBuffPtr_num] = SCANBUF_RST;

			// FIXME: We should probably discontinue filling the scan
			// buffer if we encounter a restart marker. This will stop us
			// from reading past the restart marker and missing the necessary
			// step in resetting the decoder accumulation state.

      // When we stop adding bytes to the buffer, we should also
			// mark this scan buffer with a flag to indicate that it was
			// ended by RST not by EOI or an Invalid Marker.

			// If the main decoder (in ReadScanVal) cannot find a VLC code
			// match with the few bits left in the scan buffer
			// (presumably padded with 1's), then it should check to see
			// if the buffer is terminated by RST. If so, then it
      // purges the scan buffer, advances to the next byte (after the
			// RST marker) and does a fill, then re-invokes the ReadScanVal
			// routine. At the level above, the Decoder that is calling
			// ReadScanVal should be counting MCU rows and expect this error
			// from ReadScanVal (no code match and buf terminated by RST).
			// If it happened out of place, we have corruption somehow!

			// See IJG Code:
			//   jdmarker.c:
			//     read_restart_marker()
			//     jpeg_resync_to_restart()
*/
    }
  }

  // Check for byte Stuff
  if((nBuf0 == 0xFF) && (nBuf1 == 0x00))
  {
    // Add byte to m_nScanBuff & record file position
    ScanBuffAdd(nBuf0, m_nScanBuffPtr);
    m_nScanBuffPtr += 2;
  }
  else if((nBuf0 == 0xFF) && (nBuf1 == 0xFF))
  {
    // NOTE:
    // We should be checking for a run of 0xFF before EOI marker.
    // It is possible that we could get marker padding on the end
    // of the scan segment, so we'd want to handle it here, otherwise
    // we'll report an error that we got a non-EOI Marker in scan
    // segment.

    // The downside of this is that we don't detect errors if we have
    // a run of 0xFF in the stream, until we leave the string of FF's.
    // If it were followed by an 0x00, then we may not notice it at all.
    // Probably OK.

    /*
       if (m_nWarnBadScanNum < m_nScanErrMax) {
       QString strTmp;
       strTmp = QString("  Scan Data encountered sequence 0xFFFF @ 0x%08X.0 - Assume start of marker pad at end of scan segment",
       m_nScanBuffPtr);
       m_pLog->AddLineWarn(strTmp);

       m_nWarnBadScanNum++;
       if (m_nWarnBadScanNum >= m_nScanErrMax) {
       strTmp = QString("    Only reported first %u instances of this message..."),m_nScanErrMax;
       m_pLog->AddLineErr(strTmp);
       }
       }

       // Treat as single byte of byte stuff for now, since we don't
       // know if FF bytes will arrive in pairs or not.
       m_nScanBuffPtr+=1;
     */

    // NOTE:
    // If I treat the 0xFFFF as a potential marker pad, we may not stop correctly
    // upon error if we see this inside the image somewhere (not at end).
    // Therefore, let's simply add these bytes to the buffer and let the DecodeScanImg()
    // routine figure out when we're at the end, etc.

    ScanBuffAdd(nBuf0, m_nScanBuffPtr);
    m_nScanBuffPtr += 1;
  }
  else if((nBuf0 == 0xFF) && (nMarker != 0x00))
  {
    // We have read a marker... don't assume that this is bad as it will
    // always happen at the end of the scan segment. Therefore, we will
    // assume this marker is valid (ie. not bit error in scan stream)
    // and mark the end of the scan segment.

    if(m_nWarnBadScanNum < m_nScanErrMax)
    {
      QString strTmp;

      strTmp = QString("  Scan Data encountered marker   0xFF%1 @ 0x%2.0")
          .arg(nMarker, 2, 16, QChar('0'))
          .arg(m_nScanBuffPtr, 8, 16, QChar('0'));
      m_pLog->AddLine(strTmp);

      if(nMarker != JFIF_EOI)
      {
        m_pLog->AddLineErr("  NOTE: Marker wasn't EOI (0xFFD9)");
      }

      m_nWarnBadScanNum++;

      if(m_nWarnBadScanNum >= m_nScanErrMax)
      {
        strTmp = QString("    Only reported first %1 instances of this message...").arg(m_nScanErrMax);
        m_pLog->AddLineErr(strTmp);
      }
    }

    // Optionally stop immediately upon a bad marker
#ifdef SCAN_BAD_MARKER_STOP
    m_bScanEnd = true;
    return 1;
#else
    // Add byte to m_nScanBuff & record file position
    ScanBuffAddErr(nBuf0, m_nScanBuffPtr, SCANBUF_BADMARK);

    m_nScanBuffPtr += 1;
#endif
  }
  else
  {
    // Normal byte
    // Add byte to m_nScanBuff & record file position
    ScanBuffAdd(nBuf0, m_nScanBuffPtr);

    m_nScanBuffPtr += 1;
  }

  return 0;
}

// Define minimum value before we include DCT entry in
// the IDCT calcs.
// NOTE: Currently disabled
#define IDCT_COEF_THRESH 4

// Decode a single component for one block of an MCU
// - Pull bits from the main buffer
// - Find matching huffman codes
// - Fill in the IDCT matrix (m_anDctBlock[])
// - Perform the IDCT to create spatial domain
//
// INPUT:
// - nTblDhtDc                                  = DHT table ID for DC component
// - nTblDhtAc                                  = DHT talbe ID for AC component
// - nMcuX                                              = UNUSED
// - nMcuY                                              = UNUSED
// RETURN:
// - Indicate if the decode was successful
// POST:
// - m_nScanCurDc = DC component value
// Performance:
// - This routine is called on every block so it must be
//   reasonably efficient. Only call string routines when we
//   are sure that we have an error.
//
// FIXME: Consider adding checks for DHT table like in ReadScanVal()
//
bool CimgDecode::DecodeScanComp(uint32_t nTblDhtDc, uint32_t nTblDhtAc, uint32_t nTblDqt, uint32_t, uint32_t)
{
  uint32_t nZrl;

  int32_t nVal;

  bool bDone = false;
  bool bDC = true;              // Start with DC coeff

  teRsvRet eRsvRet;             // Return value from ReadScanVal()

  uint32_t nNumCoeffs = 0;

  //uint32_t nDctMax = 0;                 // Maximum DCT coefficient to use for IDCT
  uint32_t nSavedBufPos = 0;

  uint32_t nSavedBufErr = SCANBUF_OK;

  uint32_t nSavedBufAlign = 0;

  // Profiling: No difference noted
  DecodeIdctClear();

  while(!bDone)
  {
    BuffTopup();

    // Note that once we perform ReadScanVal(), then GetScanBufPos() will be
    // after the decoded VLC
    // Save old file position info in case we want accurate error positioning
    nSavedBufPos = m_anScanBuffPtr_pos[0];
    nSavedBufErr = m_nScanBuffLatchErr;
    nSavedBufAlign = m_nScanBuffPtr_align;

    // ReadScanVal return values:
    // - RSV_OK                     OK
    // - RSV_EOB            End of block
    // - RSV_UNDERFLOW      Ran out of data in buffer
    // - RSV_RST_TERM       No huffman code found, but restart marker seen
    // Assume nTblDht just points to DC tables, adjust for AC
    // e.g. nTblDht = 0,2,4
    eRsvRet = ReadScanVal(bDC ? 0 : 1, bDC ? nTblDhtDc : nTblDhtAc, nZrl, nVal);

    // Handle Restart marker first.
    if(eRsvRet == RSV_RST_TERM)
    {
      // Assume that m_bRestartRead is TRUE
      // No huffman code found because either we ran out of bits
      // in the scan buffer or the bits padded with 1's didn't result
      // in a valid VLC code.

      // Steps:
      //   1) Reset the decoder state (DC values)
      //   2) Advance the buffer pointer (might need to handle the
      //      case of perfect alignment to byte boundary separately)
      //   3) Flush the Scan Buffer
      //   4) Clear m_bRestartRead
      //   5) Refill Scan Buffer with BuffTopUp()
      //   6) Re-invoke ReadScanVal()

      // Step 1:
      DecodeRestartDcState();

      // Step 2
      m_nScanBuffPtr += 2;

      // Step 3
      DecodeRestartScanBuf(m_nScanBuffPtr, true);

      // Step 4
      m_bRestartRead = false;

      // Step 5
      BuffTopup();

      // Step 6
      // Q_ASSERT is because we assume that we don't get 2 restart
      // markers in a row!
      eRsvRet = ReadScanVal(bDC ? 0 : 1, bDC ? nTblDhtDc : nTblDhtAc, nZrl, nVal);
      Q_ASSERT(eRsvRet != RSV_RST_TERM);
    }

    // In case we encountered a restart marker or bad scan marker
    if(nSavedBufErr == SCANBUF_BADMARK)
    {
      // Mark as scan error
      m_nScanCurErr = true;

      m_bScanBad = true;

      if(m_nWarnBadScanNum < m_nScanErrMax)
      {
        QString strPos = GetScanBufPos(nSavedBufPos, nSavedBufAlign);

        QString strTmp;

        strTmp = QString("*** ERROR: Bad marker @ %1").arg(strPos);
        m_pLog->AddLineErr(strTmp);

        m_nWarnBadScanNum++;

        if(m_nWarnBadScanNum >= m_nScanErrMax)
        {
          strTmp = QString("    Only reported first %1 instances of this message...").arg(m_nScanErrMax);
          m_pLog->AddLineErr(strTmp);
        }
      }

      // Reset the latched error now that we've dealt with it
      m_nScanBuffLatchErr = SCANBUF_OK;
    }

    int16_t nVal2;

    nVal2 = static_cast<int16_t>(nVal & 0xFFFF);

    if(eRsvRet == RSV_OK)
    {
      // DC entry is always one value only
      if(bDC)
      {
        DecodeIdctSet(nTblDqt, nNumCoeffs, nZrl, nVal2);        //CALZ
        bDC = false;            // Now we will be on AC comps
      }
      else
      {
        // We're on AC entry, so keep looping until
        // we have finished up to 63 entries
        // Set entry in table
        // PERFORMANCE:
        //   No noticeable difference if following is skipped
        if(m_bDecodeScanAc)
        {
          DecodeIdctSet(nTblDqt, nNumCoeffs, nZrl, nVal2);
        }
      }
    }
    else if(eRsvRet == RSV_EOB)
    {
      if(bDC)
      {
        DecodeIdctSet(nTblDqt, nNumCoeffs, nZrl, nVal2);        //CALZ
        // Now that we have finished the DC coefficient, start on AC coefficients
        bDC = false;
      }
      else
      {
        // Now that we have finished the AC coefficients, we are done
        bDone = true;
      }
    }
    else if(eRsvRet == RSV_UNDERFLOW)
    {
      // ERROR

      if(m_nWarnBadScanNum < m_nScanErrMax)
      {
        QString strPos = GetScanBufPos(nSavedBufPos, nSavedBufAlign);

        QString strTmp;

        strTmp = QString("*** ERROR: Bad huffman code @ %1").arg(strPos);
        m_pLog->AddLineErr(strTmp);

        m_nWarnBadScanNum++;

        if(m_nWarnBadScanNum >= m_nScanErrMax)
        {
          strTmp = QString("    Only reported first %1 instances of this message...").arg(m_nScanErrMax);
          m_pLog->AddLineErr(strTmp);
        }
      }

      m_nScanCurErr = true;
      bDone = true;

      return false;
    }

    // Increment the number of coefficients
    nNumCoeffs += 1 + nZrl;

    // If we have filled out an entire 64 entries, then we move to
    // the next block without an EOB
    // NOTE: This is only 63 entries because we assume that we
    //       are doing the AC (DC was already bDone in a different pass)

    // FIXME: Would like to combine DC & AC in one pass so that
    // we don't end up having to use 2 tables. The check below will
    // also need to be changed to == 64.
    //
    // Currently, we will have to correct AC nNumCoeffs entries (in IDCT) to
    // be +1 to get real index, as we are ignoring DC position 0.

    if(nNumCoeffs == 64)
    {
      bDone = true;
    }
    else if(nNumCoeffs > 64)
    {
      // ERROR

      if(m_nWarnBadScanNum < m_nScanErrMax)
      {
        QString strTmp;

        QString strPos = GetScanBufPos(nSavedBufPos, nSavedBufAlign);

        strTmp = QString("*** ERROR: @ %1, nNumCoeffs>64 [%2]").arg(strPos).arg(nNumCoeffs);
        m_pLog->AddLineErr(strTmp);

        m_nWarnBadScanNum++;

        if(m_nWarnBadScanNum >= m_nScanErrMax)
        {
          strTmp = QString("    Only reported first %1 instances of this message...").arg(m_nScanErrMax);
          m_pLog->AddLineErr(strTmp);
        }
      }

      m_nScanCurErr = true;
      m_bScanBad = true;
      bDone = true;

      nNumCoeffs = 64;          // Just to ensure we don't use an overrun value anywhere
    }
  }

  // We finished the MCU
  // Now calc the IDCT matrix

  // The following code needs to be very efficient.
  // A number of experiments have been carried out to determine
  // the magnitude of speed improvements through various settings
  // and IDCT methods:
  //
  // PERFORMANCE:
  //   Example file: canon_1dsmk2_
  //
  //   0:06       Turn off m_bDecodeScanAc (so no array memset, etc.)
  //   0:10   m_bDecodeScanAc=true, but DecodeIdctCalc() skipped
  //   0:26       m_bDecodeScanAc=true and DecodeIdctCalcFixedpt()
  //   0:27       m_bDecodeScanAc=true and DecodeIdctCalcFloat()

  if(m_bDecodeScanAc)
  {
#ifdef IDCT_FIXEDPT
    DecodeIdctCalcFixedpt();
#else

    // TODO: Select appropriate conversion routine based on performance
//              DecodeIdctCalcFloat(nDctMax);
//              DecodeIdctCalcFloat(nNumCoeffs);
//              DecodeIdctCalcFloat(m_nDctCoefMax);
    DecodeIdctCalcFloat(64);
//              DecodeIdctCalcFloat(32);
#endif
  }

  return true;
}

// Decode a single component for one block of an MCU with printing
// used for the Detailed Decode functionality
// - Same as DecodeScanComp() but adds reporting of variable length codes (VLC)
//
// INPUT:
// - nTblDhtDc                                  = DHT table ID for DC component
// - nTblDhtAc                                  = DHT talbe ID for AC component
// - nMcuX                                              = Current MCU X coordinate (for reporting only)
// - nMcuY                                              = Current MCU Y coordinate (for reporting only)
// RETURN:
// - Indicate if the decode was successful
// POST:
// - m_nScanCurDc = DC component value
// PERFORMANCE:
// - As this routine is called for every MCU, it is important
//   for it to be efficient. However, we are in print mode, so
//   we can afford to be slower.
//
// FIXME: need to fix like DecodeScanComp() (ordering of exit conditions, etc.)
// FIXME: Consider adding checks for DHT table like in ReadScanVal()
//
bool CimgDecode::DecodeScanCompPrint(uint32_t nTblDhtDc, uint32_t nTblDhtAc, uint32_t nTblDqt, uint32_t nMcuX, uint32_t nMcuY)
{
  bool bPrint = true;

  teRsvRet eRsvRet;

  QString strTmp;
  QString strTbl;
  QString strSpecial;
  QString strPos;

  uint32_t nZrl;
  int32_t nVal;

  bool bDone = false;
  bool bDC = true;              // Start with DC component

  if(bPrint)
  {
    switch (nTblDqt)
    {
      case 0:
        strTbl = "Lum";
        break;

      case 1:
        strTbl = "Chr(0)";      // Usually Cb
        break;

      case 2:
        strTbl = "Chr(1)";      // Usually Cr
        break;

      default:
        strTbl = "???";
        break;
    }

    strTmp = QString("    %1 (Tbl #%2), MCU=[%3,%4]").arg(strTbl).arg(nTblDqt).arg(nMcuX).arg(nMcuY);
    m_pLog->AddLine(strTmp);
  }

  uint32_t nNumCoeffs = 0;
  uint32_t nSavedBufPos = 0;
  uint32_t nSavedBufErr = SCANBUF_OK;
  uint32_t nSavedBufAlign = 0;

  DecodeIdctClear();

  while(!bDone)
  {
    BuffTopup();

    // Note that once we perform ReadScanVal(), then GetScanBufPos() will be
    // after the decoded VLC

    // Save old file position info in case we want accurate error positioning
    nSavedBufPos = m_anScanBuffPtr_pos[0];
    nSavedBufErr = m_nScanBuffLatchErr;
    nSavedBufAlign = m_nScanBuffPtr_align;

    // Return values:
    //      0 - OK
    //  1 - EOB
    //  2 - Overread error
    //  3 - No huffman code found, but restart marker seen
    eRsvRet = ReadScanVal(bDC ? 0 : 1, bDC ? nTblDhtDc : nTblDhtAc, nZrl, nVal);

    // Handle Restart marker first.
    if(eRsvRet == RSV_RST_TERM)
    {
      // Assume that m_bRestartRead is TRUE
      // No huffman code found because either we ran out of bits
      // in the scan buffer or the bits padded with 1's didn't result
      // in a valid VLC code.

      // Steps:
      //   1) Reset the decoder state (DC values)
      //   2) Advance the buffer pointer (might need to handle the
      //      case of perfect alignment to byte boundary separately)
      //   3) Flush the Scan Buffer
      //   4) Clear m_bRestartRead
      //   5) Refill Scan Buffer with BuffTopUp()
      //   6) Re-invoke ReadScanVal()

      // Step 1:
      DecodeRestartDcState();

      // Step 2
      m_nScanBuffPtr += 2;

      // Step 3
      DecodeRestartScanBuf(m_nScanBuffPtr, true);

      // Step 4
      m_bRestartRead = false;

      // Step 5
      BuffTopup();

      // Step 6
      // Q_ASSERT is because we assume that we don't get 2 restart
      // markers in a row!
      eRsvRet = ReadScanVal(bDC ? 0 : 1, bDC ? nTblDhtDc : nTblDhtAc, nZrl, nVal);
      Q_ASSERT(eRsvRet != RSV_RST_TERM);
    }

    // In case we encountered a restart marker or bad scan marker
    if(nSavedBufErr == SCANBUF_BADMARK)
    {
      // Mark as scan error
      m_nScanCurErr = true;

      m_bScanBad = true;

      if(m_nWarnBadScanNum < m_nScanErrMax)
      {
        strPos = GetScanBufPos(nSavedBufPos, nSavedBufAlign);
        strTmp = QString("*** ERROR: Bad marker @ %1").arg(strPos);
        m_pLog->AddLineErr(strTmp);

        m_nWarnBadScanNum++;

        if(m_nWarnBadScanNum >= m_nScanErrMax)
        {
          strTmp = QString("    Only reported first %1 instances of this message...").arg(m_nScanErrMax);
          m_pLog->AddLineErr(strTmp);
        }
      }

      // Reset the latched error now that we've dealt with it
      m_nScanBuffLatchErr = SCANBUF_OK;
    }

    // Should this be before or after restart checks?
    uint32_t nCoeffStart = nNumCoeffs;
    uint32_t nCoeffEnd = nNumCoeffs + nZrl;

    int16_t nVal2;

    nVal2 = static_cast<int16_t>(nVal & 0xFFFF);

    if(eRsvRet == RSV_OK)
    {
      strSpecial = "";
      // DC entry is always one value only
      // FIXME: Do I need nTblDqt == 4 as well?
      if(bDC)
      {
        DecodeIdctSet(nTblDqt, nNumCoeffs, nZrl, nVal2);
        bDC = false;            // Now we will be on AC comps
      }
      else
      {
        // We're on AC entry, so keep looping until
        // we have finished up to 63 entries
        // Set entry in table
        DecodeIdctSet(nTblDqt, nNumCoeffs, nZrl, nVal2);
      }
    }
    else if(eRsvRet == RSV_EOB)
    {
      if(bDC)
      {
        DecodeIdctSet(nTblDqt, nNumCoeffs, nZrl, nVal2);
        bDC = false;            // Now we will be on AC comps
      }
      else
      {
        bDone = true;
      }
      strSpecial = "EOB";
    }
    else if(eRsvRet == RSV_UNDERFLOW)
    {
      if(m_nWarnBadScanNum < m_nScanErrMax)
      {
        strSpecial = "ERROR";
        strPos = GetScanBufPos(nSavedBufPos, nSavedBufAlign);

        strTmp = QString("*** ERROR: Bad huffman code @ %1").arg(strPos);
        m_pLog->AddLineErr(strTmp);

        m_nWarnBadScanNum++;
        
        if(m_nWarnBadScanNum >= m_nScanErrMax)
        {
          strTmp = QString("    Only reported first %1 instances of this message...").arg(m_nScanErrMax);
          m_pLog->AddLineErr(strTmp);
        }
      }

      m_nScanCurErr = true;
      bDone = true;

      // Print out before we leave
      if(bPrint)
      {
        ReportVlc(nSavedBufPos, nSavedBufAlign, nZrl, nVal2, nCoeffStart, nCoeffEnd, strSpecial);
      }

      return false;
    }

    // Increment the number of coefficients
    nNumCoeffs += 1 + nZrl;
    // If we have filled out an entire 64 entries, then we move to
    // the next block without an EOB
    // NOTE: This is only 63 entries because we assume that we
    //       are doing the AC (DC was already done in a different pass)
    if(nNumCoeffs == 64)
    {
      strSpecial = "EOB64";
      bDone = true;
    }
    else if(nNumCoeffs > 64)
    {
      // ERROR

      if(m_nWarnBadScanNum < m_nScanErrMax)
      {
        strPos = GetScanBufPos(nSavedBufPos, nSavedBufAlign);
        strTmp = QString("*** ERROR: @ %1, nNumCoeffs>64 [%2]").arg(strPos).arg(nNumCoeffs);
        m_pLog->AddLineErr(strTmp);

        m_nWarnBadScanNum++;

        if(m_nWarnBadScanNum >= m_nScanErrMax)
        {
          strTmp = QString("    Only reported first %1 instances of this message...").arg(m_nScanErrMax);
          m_pLog->AddLineErr(strTmp);
        }
      }

      m_nScanCurErr = true;
      m_bScanBad = true;
      bDone = true;

      nNumCoeffs = 64;          // Just to ensure we don't use an overrun value anywhere
    }

    if(bPrint)
    {
      ReportVlc(nSavedBufPos, nSavedBufAlign, nZrl, nVal2, nCoeffStart, nCoeffEnd, strSpecial);
    }
  }

  // We finished the MCU component

  // Now calc the IDCT matrix
#ifdef IDCT_FIXEDPT
  DecodeIdctCalcFixedpt();
#else
//      DecodeIdctCalcFloat(nNumCoeffs);
  DecodeIdctCalcFloat(64);
#endif

  // Now report the coefficient matrix (after zigzag reordering)
  if(bPrint)
  {
    ReportDctMatrix();
  }

  return true;
}

// Print out the DCT matrix for a given block
//
// PRE:
// - m_anDctBlock[]
//
void CimgDecode::ReportDctMatrix()
{
  QString strTmp;
  QString strLine;

  int32_t nCoefVal;

  for(uint32_t nY = 0; nY < 8; nY++)
  {
    if(nY == 0)
    {
      strLine = "                      DCT Matrix=[";
    }
    else
    {
      strLine = "                                 [";
    }

    for(uint32_t nX = 0; nX < 8; nX++)
    {
      strTmp = "";
      nCoefVal = m_anDctBlock[nY * 8 + nX];
      strTmp = QString("%1").arg(nCoefVal, 5);
      strLine.append(strTmp);

      if(nX != 7)
      {
        strLine.append(" ");
      }
    }

    strLine.append("]");
    m_pLog->AddLine(strLine);
  }

  m_pLog->AddLine("");
}

// Report out the variable length codes (VLC)
//
// Need to consider impact of padding bytes... When pads get extracted, they
// will not appear in the binary display shown below. Might want the get buffer
// to do post-pad removal first.
//
// eg. file Dsc0019.jpg
// Overlay 0x4215 = 7FFF0000 len=4
//
// INPUT:
// - nVlcPos                            =
// - nVlcAlign                          =
// - nZrl                                       =
// - nVal                                       =
// - nCoeffStart                        =
// - nCoeffEnd                          =
// - specialStr                         =
//
void CimgDecode::ReportVlc(uint32_t nVlcPos, uint32_t nVlcAlign,
                           uint32_t nZrl, int32_t nVal, uint32_t nCoeffStart, uint32_t nCoeffEnd, QString specialStr)
{
  QString strPos;
  QString strTmp;

  uint32_t nBufByte[4];
  uint32_t nBufPosInd = nVlcPos;

  QString strData = "";
  QString strByte1 = "";
  QString strByte2 = "";
  QString strByte3 = "";
  QString strByte4 = "";
  QString strBytes = "";
  QString strBytesOrig = "";
  QString strBinMarked = "";

  strPos = GetScanBufPos(nVlcPos, nVlcAlign);

  // Read in the buffer bytes, but skip pad bytes (0xFF00 -> 0xFF)

  // We need to look at previous byte as it might have been
  // start of stuff byte! If so, we need to ignore the byte
  // and advance the pointers.
  uint8_t nBufBytePre = m_pWBuf->Buf(nBufPosInd - 1);

  nBufByte[0] = m_pWBuf->Buf(nBufPosInd++);

  if((nBufBytePre == 0xFF) && (nBufByte[0] == 0x00))
  {
    nBufByte[0] = m_pWBuf->Buf(nBufPosInd++);
  }

  nBufByte[1] = m_pWBuf->Buf(nBufPosInd++);

  if((nBufByte[0] == 0xFF) && (nBufByte[1] == 0x00))
  {
    nBufByte[1] = m_pWBuf->Buf(nBufPosInd++);
  }

  nBufByte[2] = m_pWBuf->Buf(nBufPosInd++);

  if((nBufByte[1] == 0xFF) && (nBufByte[2] == 0x00))
  {
    nBufByte[2] = m_pWBuf->Buf(nBufPosInd++);
  }

  nBufByte[3] = m_pWBuf->Buf(nBufPosInd++);

  if((nBufByte[2] == 0xFF) && (nBufByte[3] == 0x00))
  {
    nBufByte[3] = m_pWBuf->Buf(nBufPosInd++);
  }

  strByte1 = Dec2Bin(nBufByte[0], 8, true);
  strByte2 = Dec2Bin(nBufByte[1], 8, true);
  strByte3 = Dec2Bin(nBufByte[2], 8, true);
  strByte4 = Dec2Bin(nBufByte[3], 8, true);
  strBytesOrig = strByte1 + " " + strByte2 + " " + strByte3 + " " + strByte4;
  strBytes = strByte1 + strByte2 + strByte3 + strByte4;

  for(uint32_t ind = 0; ind < nVlcAlign; ind++)
  {
    strBinMarked += "-";
  }

  strBinMarked += strBytes.mid(nVlcAlign, m_nScanBitsUsed1 + m_nScanBitsUsed2);

  for(uint32_t ind = nVlcAlign + m_nScanBitsUsed1 + m_nScanBitsUsed2; ind < 32; ind++)
  {
    strBinMarked += "-";
  }

  strBinMarked.insert(24, " ");
  strBinMarked.insert(16, " ");
  strBinMarked.insert(8, " ");

  strData = QString("0x %1 %2 %3 %4 = 0b (%5)")
      .arg(nBufByte[0], 2, 16, QChar('0'))
      .arg(nBufByte[1], 2, 16, QChar('0'))
      .arg(nBufByte[2], 2, 16, QChar('0'))
      .arg(nBufByte[3], 2, 16, QChar('0'))
      .arg(strBinMarked);

  if((nCoeffStart == 0) && (nCoeffEnd == 0))
  {
    strTmp = QString("      [%1]: ZRL=[%2] Val=[%3] Coef=[%4= DC] Data=[%5] %6")
        .arg(strPos)
        .arg(nZrl, 2)
        .arg(nVal, 5)
        .arg(nCoeffStart, 2, 10, QChar('0'))
        .arg(strData)
        .arg(specialStr);
  }
  else
  {
    strTmp = QString("      [%1]: ZRL=[%2] Val=[%3] Coef=[%4..%5] Data=[%6] %7")
        .arg(strPos)
        .arg(nZrl, 2)
        .arg(nVal, 5)
        .arg(nCoeffStart, 2, 10, QChar('0'))
        .arg(nCoeffEnd, 2, 10, QChar('0'))
        .arg(strData)
        .arg(specialStr);
  }

  m_pLog->AddLine(strTmp);
}

// Clear input and output matrix
//
// POST:
// - m_anDctBlock[]
// - m_afIdctBlock[]
// - m_anIdctBlock[]
// - m_nDctCoefMax
//
void CimgDecode::DecodeIdctClear()
{
  memset(m_anDctBlock, 0, sizeof m_anDctBlock);
  memset(m_afIdctBlock, 0, sizeof m_afIdctBlock);
  memset(m_anIdctBlock, 0, sizeof m_anIdctBlock);

  m_nDctCoefMax = 0;
}

// Set the DCT matrix entry
// - Fills in m_anDctBlock[] with the unquantized coefficients
// - Reversing the quantization is done using m_anDqtCoeffZz[][]
//
// INPUT:
// - nDqtTbl                            =
// - num_coeffs                         =
// - zrl                                        =
// - val                                        =
// PRE:
// - glb_anZigZag[]
// - m_anDqtCoeffZz[][]
// - m_nDctCoefMax
// POST:
// - m_anDctBlock[]
// NOTE:
// - We need to convert between the zigzag order and the normal order
//
void CimgDecode::DecodeIdctSet(uint32_t nDqtTbl, uint32_t num_coeffs, uint32_t zrl, int16_t val)
{
  uint32_t ind = num_coeffs + zrl;

  if(ind >= 64)
  {
    // We have an error! Don't set the block. Skip this comp for now
    // After this call, we will likely trap the error.
  }
  else
  {
    uint32_t nDctInd = glb_anZigZag[ind];

    int16_t nValUnquant = val * m_anDqtCoeffZz[nDqtTbl][ind];

    /*
       // NOTE:
       //  To test steganography analysis, we can experiment with dropping
       //  specific components of the image.
       uint32_t nRow = nDctInd/8;
       uint32_t nCol = nDctInd - (nRow*8);
       if ((nRow == 0) && (nCol>=0 && nCol<=7)) {
       nValUnquant = 0;
       }
     */

    m_anDctBlock[nDctInd] = nValUnquant;

    // Update max DCT coef # (after unzigzag) so that we can save
    // some work when performing IDCT.
    // FIXME: The following doesn't seem to work when we later
    // restrict DecodeIdctCalc() to only m_nDctCoefMax coefs!

//              if ( (nDctInd > m_nDctCoefMax) && (abs(nValUnquant) >= IDCT_COEF_THRESH) ) {
    if(nDctInd > m_nDctCoefMax)
    {
      m_nDctCoefMax = nDctInd;
    }
  }
}

// Precalculate the IDCT lookup tables
//
// POST:
// - m_afIdctLookup[]
// - m_anIdctLookup[]
// NOTE:
// - This is 4k entries @ 4B each = 16KB
//
void CimgDecode::PrecalcIdct()
{
  uint32_t nX, nY, nU, nV;

  uint32_t nYX, nVU;

  double fCu, fCv;
  double fCosProd;
  double fInsideProd;
  double fPi = 3.141592654;
  double fSqrtHalf = 0.707106781;

  for(nY = 0; nY < DCT_SZ_Y; nY++)
  {
    for(nX = 0; nX < DCT_SZ_X; nX++)
    {
      nYX = nY * DCT_SZ_X + nX;

      for(nV = 0; nV < DCT_SZ_Y; nV++)
      {
        for(nU = 0; nU < DCT_SZ_X; nU++)
        {

          nVU = nV * DCT_SZ_X + nU;

          fCu = (nU == 0) ? fSqrtHalf : 1;
          fCv = (nV == 0) ? fSqrtHalf : 1;
          fCosProd = cos((2 * nX + 1) * nU * fPi / 16) * cos((2 * nY + 1) * nV * fPi / 16);
          // Note that the only part we are missing from
          // the "Inside Product" is the "m_afDctBlock[nV*8+nU]" term
          fInsideProd = fCu * fCv * fCosProd;

          // Store the Lookup result
          m_afIdctLookup[nYX][nVU] = fInsideProd;

          // Store a fixed point Lookup as well
          m_anIdctLookup[nYX][nVU] = static_cast<int32_t>(fInsideProd * (1 << 10));
        }
      }
    }
  }
}

// Perform IDCT
//
// Formula:
//  See itu-t81.pdf, section A.3.3
//
// s(yx) = 1/4*Sum(u=0..7)[ Sum(v=0..7)[ C(u) * C(v) * S(vu) *
//                     cos( (2x+1)*u*Pi/16 ) * cos( (2y+1)*v*Pi/16 ) ] ]
// Cu, Cv = 1/sqrt(2) for u,v=0
// Cu, Cv = 1 else
//
// INPUT:
// - nCoefMax                           = Maximum number of coefficients to calculate
// PRE:
// - m_afIdctLookup[][]
// - m_anDctBlock[]
// POST:
// - m_afIdctBlock[]
//
void CimgDecode::DecodeIdctCalcFloat(uint32_t nCoefMax)
{
  uint32_t nYX, nVU;

  double fSum;

  for(nYX = 0; nYX < DCT_SZ_ALL; nYX++)
  {
    fSum = 0;

    // Skip DC coefficient!
    for(nVU = 1; nVU < nCoefMax; nVU++)
    {
      fSum += m_afIdctLookup[nYX][nVU] * m_anDctBlock[nVU];
    }
    
    fSum *= 0.25;

    // Store the result
    // FIXME: Note that float->int is very slow!
    //   Should consider using fixed point instead!
    m_afIdctBlock[nYX] = fSum;
  }
}

// Fixed point version of DecodeIdctCalcFloat()
//
// PRE:
// - m_anIdctLookup[][]
// - m_anDctBlock[]
// POST:
// - m_anIdctBlock[]
//
void CimgDecode::DecodeIdctCalcFixedpt()
{
  uint32_t nYX, nVU;

  int32_t nSum;

  for(nYX = 0; nYX < DCT_SZ_ALL; nYX++)
  {
    nSum = 0;
    
    // Skip DC coefficient!
    for(nVU = 1; nVU < DCT_SZ_ALL; nVU++)
    {
      nSum += m_anIdctLookup[nYX][nVU] * m_anDctBlock[nVU];
    }

    nSum /= 4;

    // Store the result
    // FIXME: Note that float->int is very slow!
    //   Should consider using fixed point instead!
    m_anIdctBlock[nYX] = nSum >> 10;
  }
}

// Clear the entire pixel image arrays for all three components (YCC)
//
// INPUT:
// - nWidth                                     = Current allocated image width
// - nHeight                            = Current allocated image height
// POST:
// - m_pPixValY
// - m_pPixValCb
// - m_pPixValCr
//
void CimgDecode::ClrFullRes(int32_t nWidth, int32_t nHeight)
{
  Q_ASSERT(m_pPixValY);
  
  if(m_nNumSosComps == NUM_CHAN_YCC)
  {
    Q_ASSERT(m_pPixValCb);
    Q_ASSERT(m_pPixValCr);
  }
  
  // FIXME: Add in range checking here
  memset(m_pPixValY, 0, (nWidth * nHeight * sizeof(int16_t)));
  
  if(m_nNumSosComps == NUM_CHAN_YCC)
  {
    memset(m_pPixValCb, 0, (nWidth * nHeight * sizeof(int16_t)));
    memset(m_pPixValCr, 0, (nWidth * nHeight * sizeof(int16_t)));
  }
}

// Generate a single component's pixel content for one MCU
// - Fetch content from the 8x8 IDCT block (m_afIdctBlock[])
//   for the specified component (nComp)
// - Transfer the pixel content to the specified component's
//   pixel map (m_pPixValY[],m_pPixValCb[],m_pPixValCr[])
// - DC level shifting is performed (nDcOffset)
// - Replication of pixels according to Chroma Subsampling (sampling factors)
//
// INPUT:
// - nMcuX                                      =
// - nMcuY                                      =
// - nComp                                      = Component index (1,2,3)
// - nCssXInd                           =
// - nCssYInd                           =
// - nDcOffset                          =
// PRE:
// - DecodeIdctCalc() already called on Lum AC, and Lum DC already done
//
void CimgDecode::SetFullRes(int32_t nMcuX, int32_t nMcuY, int32_t nComp, uint32_t nCssXInd, uint32_t nCssYInd, int16_t nDcOffset)
{
  uint32_t nYX;

  double fVal;

  int16_t nVal;

  int32_t nChan;

  // Convert from Component index (1-based) to Channel index (0-based)
  // Component index is direct from SOF/SOS
  // Channel index is used for internal display representation
  if(nComp <= 0)
  {
#ifdef DEBUG_LOG
    QString strTmp;
    QString strDebug;

    strTmp = QString("SetFullRes() with nComp <= 0 [%1]").arg(nComp);
    strDebug = QString("## File=[%1] Block=[%2] Error=[%3]\n")
        .arg(m_pAppConfig->strCurFname, -100)
        .arg("ImgDecode", -10)
        .arg(strTmp);
    qDebug() << strDebug;
#else
    Q_ASSERT(false);
#endif
    return;
  }

  nChan = nComp - 1;

  int32_t nPixMapW = m_nBlkXMax * BLK_SZ_X;    // Width of pixel map
  int32_t nOffsetBlkCorner;    // Linear offset to top-left corner of block
  int32_t nOffsetPixCorner;    // Linear offset to top-left corner of pixel (start point for expansion)

  // Calculate the linear pixel offset for the top-left corner of the block in the MCU
  nOffsetBlkCorner = ((nMcuY * m_nMcuHeight) + nCssYInd * BLK_SZ_X) * nPixMapW + ((nMcuX * m_nMcuWidth) + nCssXInd * BLK_SZ_Y);

  // Use the expansion factor to determine how many bits to replicate
  // Typically for luminance (Y) this will be 1 & 1
  // The replication factor is available in m_anExpandBitsMcuH[] and m_anExpandBitsMcuV[]

  // Step through all pixels in the block
  for(uint32_t nY = 0; nY < BLK_SZ_Y; nY++)
  {
    for(uint32_t nX = 0; nX < BLK_SZ_X; nX++)
    {
      nYX = nY * BLK_SZ_X + nX;

      // Fetch the pixel value from the IDCT 8x8 block and perform DC level shift
#ifdef IDCT_FIXEDPT
      nVal = m_anIdctBlock[nYX];
      // TODO: Why do I need AC value x8 multiplier?
      nVal = (nVal * 8) + nDcOffset;
#else
      fVal = m_afIdctBlock[nYX];
      // TODO: Why do I need AC value x8 multiplier?
      nVal = (static_cast<int16_t>(fVal * 8) + nDcOffset);
#endif

      // NOTE: These range checks were already done in DecodeScanImg()
      Q_ASSERT(nCssXInd < MAX_SAMP_FACT_H);
      Q_ASSERT(nCssYInd < MAX_SAMP_FACT_V);
      Q_ASSERT(nY < BLK_SZ_Y);
      Q_ASSERT(nX < BLK_SZ_X);

      // Set the pixel value for the component

      // We start with the linear offset into the pixel map for the top-left
      // corner of the block. Then we adjust to determine the top-left corner
      // of the pixel that we may optionally expand in subsampling scenarios.

      // Calculate the top-left corner pixel linear offset after taking
      // into account any expansion in the X direction
      nOffsetPixCorner = nOffsetBlkCorner + nX * m_anExpandBitsMcuH[nComp];

      // Replication the pixels as specified in the sampling factor
      // This is typically done for the chrominance channels when
      // chroma subsamping is used.
      for(uint32_t nIndV = 0; nIndV < m_anExpandBitsMcuV[nComp]; nIndV++)
      {
        for(uint32_t nIndH = 0; nIndH < m_anExpandBitsMcuH[nComp]; nIndH++)
        {
          if(nChan == CHAN_Y)
          {
            m_pPixValY[nOffsetPixCorner + (nIndV * nPixMapW) + nIndH] = nVal;
          }
          else if(nChan == CHAN_CB)
          {
            m_pPixValCb[nOffsetPixCorner + (nIndV * nPixMapW) + nIndH] = nVal;
          }
          else if(nChan == CHAN_CR)
          {
            m_pPixValCr[nOffsetPixCorner + (nIndV * nPixMapW) + nIndH] = nVal;
          }
          else
          {
            Q_ASSERT(false);
          }
        }                       // nIndH
      }                         // nIndV
    }                           // nX

    nOffsetBlkCorner += (nPixMapW * m_anExpandBitsMcuV[nComp]);
  }                             // nY
}

// Calculates the actual byte offset (from start of file) for
// the current position in the m_nScanBuff.
//
// PRE:
// - m_anScanBuffPtr_pos[]
// - m_nScanBuffPtr_align
// RETURN:
// - File position
//
QString CimgDecode::GetScanBufPos()
{
  return GetScanBufPos(m_anScanBuffPtr_pos[0], m_nScanBuffPtr_align);
}

// Generate a file position string that also indicates bit alignment
//
// INPUT:
// - pos                        = File position (byte)
// - align                      = File position (bit)
// RETURN:
// - Formatted string
//
QString CimgDecode::GetScanBufPos(uint32_t pos, uint32_t align)
{
  return QString("0x%1.%2")
      .arg(pos, 8, 16, QChar('0'))
      .arg(align);;
}

// Test the scan error flag and, if set, report out the position
//
// INPUT:
// - nMcuX                              = MCU x coordinate
// - nMcuY                              = MCU y coordinate
// - nCssIndH                   = Chroma subsampling (horizontal)
// - nCssIndV                   = Chroma subsampling (vertical)
// - nComp                              = Image component
//
void CimgDecode::CheckScanErrors(uint32_t nMcuX, uint32_t nMcuY, uint32_t nCssIndH, uint32_t nCssIndV, uint32_t nComp)
{
  //uint32_t mcu_x_max = (m_nDimX/m_nMcuWidth);
  //uint32_t mcu_y_max = (m_nDimY/m_nMcuHeight);

  // Determine pixel position, taking into account sampling quadrant as well
  uint32_t err_pos_x = m_nMcuWidth * nMcuX + nCssIndH * BLK_SZ_X;

  uint32_t err_pos_y = m_nMcuHeight * nMcuY + nCssIndV * BLK_SZ_Y;

  if(m_nScanCurErr)
  {
    QString strTmp, errStr;

    // Report component and subsampling quadrant
    switch (nComp)
    {
      case SCAN_COMP_Y:
        strTmp = QString("Lum CSS(%1,%2)")
            .arg(nCssIndH)
            .arg(nCssIndV);
        break;

      case SCAN_COMP_CB:
        strTmp = QString("Chr(Cb) CSS(%1,%2)")
            .arg(nCssIndH)
            .arg(nCssIndV);
        break;

      case SCAN_COMP_CR:
        strTmp = QString("Chr(Cr) CSS(%1,%2)")
            .arg(nCssIndH)
            .arg(nCssIndV);
        break;

      default:
        // Unknown component
        strTmp = QString("??? CSS(%1,%2)")
            .arg(nCssIndH)
            .arg(nCssIndV);
        break;
    }

    if(m_nWarnBadScanNum < m_nScanErrMax)
    {
      errStr = QString("*** ERROR: Bad scan data in MCU(%1,%2): %3 @ Offset %4")
          .arg(nMcuX)
          .arg(nMcuY)
          .arg(strTmp)
          .arg(GetScanBufPos());
      m_pLog->AddLineErr(errStr);
      errStr = QString("           MCU located at pixel=(%1, %2)")
          .arg(err_pos_x)
          .arg(err_pos_y);
      m_pLog->AddLineErr(errStr);

      //errStr = QString("*** Resetting Error state to continue ***");
      //m_pLog->AddLineErr(errStr);

      m_nWarnBadScanNum++;

      if(m_nWarnBadScanNum >= m_nScanErrMax)
      {
        strTmp = QString("    Only reported first %1 instances of this message...").arg(m_nScanErrMax);
        m_pLog->AddLineErr(strTmp);
      }
    }

    // TODO: Should we reset m_nScanCurErr?
    m_nScanCurErr = false;

    //errStr = QString("*** Resetting Error state to continue ***");
    //m_pLog->AddLineErr(errStr);
  }                             // Error?
}

// Report the cumulative DC value
//
// INPUT:
// - nMcuX                              = MCU x coordinate
// - nMcuY                              = MCU y coordinate
// - nVal                               = DC value
//
void CimgDecode::PrintDcCumVal(uint32_t, uint32_t, int32_t nVal)
{
  QString strTmp;

/*  strTmp = QString("  MCU [%1,%2] DC Cumulative Val = [%3]")
      .arg(nMcuX, 4)
      .arg(nMcuY, 4)
      .arg(nVal, 5); */
  strTmp = QString("                 Cumulative DC Val=[%1]").arg(nVal, 5);
  m_pLog->AddLine(strTmp);
}

// Reset the DC values in the decoder (e.g. at start and
// after restart markers)
//
// POST:
// - m_nDcLum
// - m_nDcChrCb
// - m_nDcChrCr
// - m_anDcLumCss[]
// - m_anDcChrCbCss[]
// - m_anDcChrCrCss[]
//
void CimgDecode::DecodeRestartDcState()
{
  m_nDcLum = 0;
  m_nDcChrCb = 0;
  m_nDcChrCr = 0;
  
  for(uint32_t nInd = 0; nInd < MAX_SAMP_FACT_V * MAX_SAMP_FACT_H; nInd++)
  {
    m_anDcLumCss[nInd] = 0;
    m_anDcChrCbCss[nInd] = 0;
    m_anDcChrCrCss[nInd] = 0;
  }
}

//TODO
void CimgDecode::SetImageDimensions(uint32_t nWidth, uint32_t nHeight)
{
  m_rectImgBase = QRect(QPoint(0, 0), QSize(nWidth, nHeight));
}

// Process the entire scan segment and optionally render the image
// - Reset and clear the output structures
// - Loop through each MCU and read each component
// - Maintain running DC level accumulator
// - Call SetFullRes() to transfer IDCT output to YCC Pixel Map
//
// INPUT:
// - nStart                                     = File position at start of scan
// - bDisplay                           = Generate a preview image?
// - bQuiet                                     = Disable output of certain messages during decode?
//
void CimgDecode::DecodeScanImg(uint32_t nStart, bool bDisplay, bool bQuiet)
{
  qDebug() << "CimgDecode::DecodeScanImg Start";

  QString strTmp;

  bool bDieOnFirstErr = false;  // FIXME: do we want this? It makes it less useful for corrupt jpegs

  // Fetch configuration values locally
  bool bDumpHistoY = m_pAppConfig->displayYHistogram();
  bool bDecodeScanAc;

  int32_t nScanErrMax = m_pAppConfig->maxDecodeError();

  // Add some extra speed-up in hidden mode (we don't need AC)
  if(bDisplay)
  {
    bDecodeScanAc = m_pAppConfig->decodeAc();
  }
  else
  {
    bDecodeScanAc = false;
  }
  
  m_bHistEn = m_pAppConfig->displayRgbHistogram();
  m_bStatClipEn = m_pAppConfig->clipStats();

  int32_t nPixMapW = 0;
  int32_t nPixMapH = 0;

  // Reset the decoder state variables
  Reset();

  m_nScanErrMax = nScanErrMax;
  m_bDecodeScanAc = bDecodeScanAc;

  // Detect the scenario where the image component details haven't been set yet
  // The image details are set via SetImageDetails()
  if(!m_bImgDetailsSet)
  {
    m_pLog->AddLineErr("*** ERROR: Decoding image before Image components defined ***");
    return;
  }

  // Even though we support decoding of MAX_SOS_COMP_NS we limit the component flexibility further
  if((m_nNumSosComps != NUM_CHAN_GRAYSCALE) && (m_nNumSosComps != NUM_CHAN_YCC))
  {
    strTmp = QString("  NOTE: Number of SOS components not supported [%1]").arg(m_nNumSosComps);
    m_pLog->AddLineWarn(strTmp);
#ifndef DEBUG_YCCK
    return;
#endif
  }

  // Determine the maximum sampling factor and min sampling factor for this scan
  m_nSosSampFactHMax = 0;
  m_nSosSampFactVMax = 0;
  m_nSosSampFactHMin = 0xFF;
  m_nSosSampFactVMin = 0xFF;

  for(uint32_t nComp = 1; nComp <= m_nNumSosComps; nComp++)
  {
    m_nSosSampFactHMax = qMax(m_nSosSampFactHMax, m_anSofSampFactH[nComp]);
    m_nSosSampFactVMax = qMax(m_nSosSampFactVMax, m_anSofSampFactV[nComp]);
    m_nSosSampFactHMin = qMin(m_nSosSampFactHMin, m_anSofSampFactH[nComp]);
    m_nSosSampFactVMin = qMin(m_nSosSampFactVMin, m_anSofSampFactV[nComp]);
    Q_ASSERT(m_nSosSampFactHMin != 0);
    Q_ASSERT(m_nSosSampFactVMin != 0);
  }

  // ITU-T.81 clause A.2.2 "Non-interleaved order (Ns=1)"
  // - In some cases an image may have a single component in a scan but with sampling factors other than 1:
  //     Number of Img components = 1
  //       Component[1]: ID=0x01, Samp Fac=0x22 (Subsamp 1 x 1), Quant Tbl Sel=0x00 (Lum: Y)
  // - This could either be in a 3-component SOF with multiple 1-component SOS or a 1-component SOF (monochrome image)
  // - In general, grayscale images exhibit a sampling factor of 0x11
  // - Per ITU-T.81 A.2.2:
  //     When Ns = 1 (where Ns is the number of components in a scan), the order of data units
  //     within a scan shall be left-to-right and top-to-bottom, as shown in Figure A.2. This
  //     ordering applies whenever Ns = 1, regardless of the values of H1 and V1.
  // - Thus, instead of the usual decoding sequence for 0x22:
  //   [ 0 1 ] [ 4 5 ]
  //   [ 2 3 ] [ 6 7 ]
  // - The sequence for decode should be:
  //   [ 0 ] [ 1 ] [ 2 ] [ 3 ] [ 4 ] ...
  // - Which is equivalent to the non-subsampled ordering (ie. 0x11)
  // - Apply a correction for such images to remove the sampling factor
  if(m_nNumSosComps == 1)
  {
    // TODO: Need to confirm if component index needs to be looked up
    // in the case of multiple SOS or if [1] is the correct index
    if((m_anSofSampFactH[1] != 1) || (m_anSofSampFactV[1] != 1))
    {
      m_pLog->AddLineWarn("    Altering sampling factor for single component scan to 0x11");
    }

    m_anSofSampFactH[1] = 1;
    m_anSofSampFactV[1] = 1;
    m_nSosSampFactHMax = 1;
    m_nSosSampFactVMax = 1;
    m_nSosSampFactHMin = 1;
    m_nSosSampFactVMin = 1;
  }

  // Perform additional range checks
  if((m_nSosSampFactHMax == 0) || (m_nSosSampFactVMax == 0) || (m_nSosSampFactHMax > MAX_SAMP_FACT_H)
     || (m_nSosSampFactVMax > MAX_SAMP_FACT_V))
  {
    strTmp = QString("  NOTE: Degree of subsampling factor not supported [HMax=%1, VMax=%2]")
        .arg(m_nSosSampFactHMax)
        .arg(m_nSosSampFactVMax);
    m_pLog->AddLineWarn(strTmp);
    return;
  }

  // Calculate the MCU size for this scan. We do it here rather
  // than at the time of SOF (ie. SetImageDetails) for the reason
  // that under some circumstances we need to override the sampling
  // factor in single-component scans. This is done earlier.
  m_nMcuWidth = m_nSosSampFactHMax * BLK_SZ_X;
  m_nMcuHeight = m_nSosSampFactVMax * BLK_SZ_Y;

  // Calculate the number of bits to replicate when we generate the pixel map
  for(uint32_t nComp = 1; nComp <= m_nNumSosComps; nComp++)
  {
    m_anExpandBitsMcuH[nComp] = m_nSosSampFactHMax / m_anSofSampFactH[nComp];
    m_anExpandBitsMcuV[nComp] = m_nSosSampFactVMax / m_anSofSampFactV[nComp];
  }

  // Calculate the number of component samples per MCU
  for(uint32_t nComp = 1; nComp <= m_nNumSosComps; nComp++)
  {
    m_anSampPerMcuH[nComp] = m_anSofSampFactH[nComp];
    m_anSampPerMcuV[nComp] = m_anSofSampFactV[nComp];
  }

  // Determine the MCU ranges
  m_nMcuXMax = (m_nDimX / m_nMcuWidth);
  m_nMcuYMax = (m_nDimY / m_nMcuHeight);

  m_nImgSizeXPartMcu = m_nMcuXMax * m_nMcuWidth;
  m_nImgSizeYPartMcu = m_nMcuYMax * m_nMcuHeight;

  // Detect incomplete (partial) MCUs and round-up the MCU ranges if necessary.
  if((m_nDimX % m_nMcuWidth) != 0)
  {
    m_nMcuXMax++;
  }

  if((m_nDimY % m_nMcuHeight) != 0)
  {
    m_nMcuYMax++;
  }

  // Save the maximum 8x8 block dimensions
  m_nBlkXMax = m_nMcuXMax * m_nSosSampFactHMax;
  m_nBlkYMax = m_nMcuYMax * m_nSosSampFactVMax;

  // Ensure the image has a size
  if((m_nBlkXMax == 0) || (m_nBlkYMax == 0))
  {
    return;
  }

  // Set the decoded size and before scaling
  m_nImgSizeX = m_nMcuXMax * m_nMcuWidth;
  m_nImgSizeY = m_nMcuYMax * m_nMcuHeight;

  m_rectImgBase = QRect(QPoint(0, 0), QSize(m_nImgSizeX, m_nImgSizeY));

  // Determine decoding range
  int32_t nDecMcuRowStart;
  int32_t nDecMcuRowEnd;       // End to AC scan decoding
  int32_t nDecMcuRowEndFinal;  // End to general decoding

  nDecMcuRowStart = 0;
  nDecMcuRowEnd = m_nMcuYMax;
  nDecMcuRowEndFinal = m_nMcuYMax;

  // Limit the decoding range to valid range
  nDecMcuRowEnd = qMin(nDecMcuRowEnd, m_nMcuYMax);
  nDecMcuRowEndFinal = qMin(nDecMcuRowEndFinal, m_nMcuYMax);

  // Allocate the MCU File Map
  Q_ASSERT(m_pMcuFileMap == 0);
  m_pMcuFileMap = new uint32_t[m_nMcuYMax * m_nMcuXMax];

  if(!m_pMcuFileMap)
  {
    strTmp = "ERROR: Not enough memory for Image Decoder MCU File Pos Map";
    m_pLog->AddLineErr(strTmp);

    if(m_pAppConfig->interactive())
    {
      msgBox.setText(strTmp);
      msgBox.exec();
    }

    return;
  }
  
  memset(m_pMcuFileMap, 0, (m_nMcuYMax * m_nMcuXMax * sizeof(int32_t)));

  // Allocate the 8x8 Block DC Map
  m_pBlkDcValY = new int16_t[m_nBlkYMax * m_nBlkXMax];

  if((!m_pBlkDcValY))
  {
    strTmp = "ERROR: Not enough memory for Image Decoder Blk DC Value Map";
    m_pLog->AddLineErr(strTmp);

    if(m_pAppConfig->interactive())
    {
      msgBox.setText(strTmp);
      msgBox.exec();
    }

    return;
  }

  if(m_nNumSosComps == NUM_CHAN_YCC)
  {
    m_pBlkDcValCb = new int16_t[m_nBlkYMax * m_nBlkXMax];
    m_pBlkDcValCr = new int16_t[m_nBlkYMax * m_nBlkXMax];

    if((!m_pBlkDcValCb) || (!m_pBlkDcValCr))
    {
      strTmp = "ERROR: Not enough memory for Image Decoder Blk DC Value Map";
      m_pLog->AddLineErr(strTmp);

      if(m_pAppConfig->interactive())
      {
        msgBox.setText(strTmp);
        msgBox.exec();
      }

      return;
    }
  }

  memset(m_pBlkDcValY, 0, (m_nBlkYMax * m_nBlkXMax * sizeof(int16_t)));

  if(m_nNumSosComps == NUM_CHAN_YCC)
  {
    memset(m_pBlkDcValCb, 0, (m_nBlkYMax * m_nBlkXMax * sizeof(int16_t)));
    memset(m_pBlkDcValCr, 0, (m_nBlkYMax * m_nBlkXMax * sizeof(int16_t)));
  }

  // Allocate the real YCC pixel Map
  nPixMapH = m_nBlkYMax * BLK_SZ_Y;
  nPixMapW = m_nBlkXMax * BLK_SZ_X;

  // Ensure no image allocated yet
  Q_ASSERT(m_pPixValY == 0);

  if(m_nNumSosComps == NUM_CHAN_YCC)
  {
    Q_ASSERT(m_pPixValCb == 0);
    Q_ASSERT(m_pPixValCr == 0);
  }

  // Allocate image (YCC)
  m_pPixValY = new int16_t[nPixMapW * nPixMapH];

  if((!m_pPixValY))
  {
    strTmp = "ERROR: Not enough memory for Image Decoder Pixel YCC Value Map";
    m_pLog->AddLineErr(strTmp);

    if(m_pAppConfig->interactive())
    {
      msgBox.setText(strTmp);
      msgBox.exec();
    }

    return;
  }

  if(m_nNumSosComps == NUM_CHAN_YCC)
  {
    m_pPixValCb = new int16_t[nPixMapW * nPixMapH];
    m_pPixValCr = new int16_t[nPixMapW * nPixMapH];

    if((!m_pPixValCb) || (!m_pPixValCr))
    {
      strTmp = "ERROR: Not enough memory for Image Decoder Pixel YCC Value Map";
      m_pLog->AddLineErr(strTmp);

      if(m_pAppConfig->interactive())
      {
        msgBox.setText(strTmp);
        msgBox.exec();
      }

      return;
    }
  }

  // Reset pixel map
  if(bDisplay)
  {
    ClrFullRes(nPixMapW, nPixMapH);
  }

  // If a previous bitmap was created, deallocate it and start fresh
  m_bDibTempReady = false;
  m_bPreviewIsJpeg = false;
  
  // -------------------------------------

  QString picStr;

  // Reset the DC cumulative state
  DecodeRestartDcState();

  // Reset the scan buffer
  DecodeRestartScanBuf(nStart, false);

  // Load the buffer
  m_pWBuf->BufLoadWindow(nStart);

  // Expect that we start with RST0
  m_nRestartExpectInd = 0;
  m_nRestartLastInd = 0;

  // Load the first data into the scan buffer
  // This is done so that the MCU map will already
  // have access to the data.
  BuffTopup();

  if(!bQuiet)
  {
    m_pLog->AddLineHdr("*** Decoding SCAN Data ***");
    strTmp = QString("  OFFSET: 0x%1").arg(nStart, 8, 16, QChar('0'));
    m_pLog->AddLine(strTmp);
  }

  // TODO: Might be more appropriate to check against m_nNumSosComps instead?
  if((m_nNumSofComps != NUM_CHAN_GRAYSCALE) && (m_nNumSofComps != NUM_CHAN_YCC))
  {
    strTmp = QString("  NOTE: Number of Image Components not supported [%1]").arg(m_nNumSofComps);
    m_pLog->AddLineWarn(strTmp);
#ifndef DEBUG_YCCK
    return;
#endif
  }

  // Check DQT tables
  // We need to ensure that the DQT Table selection has already
  // been done (via a call from JfifDec to SetDqtTables() ).
  uint32_t nDqtTblY = 0;
  uint32_t nDqtTblCr = 0;
  uint32_t nDqtTblCb = 0;

#ifdef DEBUG_YCCK
  uint32_t nDqtTblK = 0;
#endif
  bool bDqtReady = true;

  for(uint32_t ind = 1; ind <= m_nNumSosComps; ind++)
  {
    if(m_anDqtTblSel[ind] < 0)
    {
      bDqtReady = false;
    }
  }

  if(!bDqtReady)
  {
    m_pLog->AddLineErr("*** ERROR: Decoding image before DQT Table Selection via JFIF_SOF ***");
    // TODO: Is more error handling required?
    return;
  }
  else
  {
    // FIXME: Not sure that we can always depend on the indices to appear in this order.
    // May need another layer of indirection to get at the frame image component index.
    nDqtTblY = m_anDqtTblSel[DQT_DEST_Y];
    nDqtTblCb = m_anDqtTblSel[DQT_DEST_CB];
    nDqtTblCr = m_anDqtTblSel[DQT_DEST_CR];
#ifdef DEBUG_YCCK
    if(m_nNumSosComps == 4)
    {
      nDqtTblK = m_anDqtTblSel[DQT_DEST_K];
    }
#endif
  }

  // Now check DHT tables
  bool bDhtReady = true;

  uint32_t nDhtTblDcY, nDhtTblDcCb, nDhtTblDcCr;
  uint32_t nDhtTblAcY, nDhtTblAcCb, nDhtTblAcCr;

#ifdef DEBUG_YCCK
  uint32_t nDhtTblDcK, nDhtTblAcK;
#endif
  for(uint32_t nClass = DHT_CLASS_DC; nClass <= DHT_CLASS_AC; nClass++)
  {
    for(uint32_t nCompInd = 1; nCompInd <= m_nNumSosComps; nCompInd++)
    {
      if(m_anDhtTblSel[nClass][nCompInd] < 0)
      {
        bDhtReady = false;
      }
    }
  }

  // Ensure that the table has been defined already!
  uint32_t nSel;

  for(uint32_t nCompInd = 1; nCompInd <= m_nNumSosComps; nCompInd++)
  {
    // Check for DC DHT table
    nSel = m_anDhtTblSel[DHT_CLASS_DC][nCompInd];

    if(m_anDhtLookupSize[DHT_CLASS_DC][nSel] == 0)
    {
      bDhtReady = false;
    }

    // Check for AC DHT table
    nSel = m_anDhtTblSel[DHT_CLASS_AC][nCompInd];

    if(m_anDhtLookupSize[DHT_CLASS_AC][nSel] == 0)
    {
      bDhtReady = false;
    }
  }

  if(!bDhtReady)
  {
    m_pLog->AddLineErr("*** ERROR: Decoding image before DHT Table Selection via JFIF_SOS ***");
    // TODO: Is more error handling required here?
    return;
  }
  else
  {
    // Define the huffman table indices that are selected for each
    // image component index and class (AC,DC).
    //
    // No need to check if a table is valid here since we have
    // previously checked to ensure that all required tables exist.
    // NOTE: If the table has not been defined, then the index
    // will be 0xFFFFFFFF. ReadScanVal() will trap this with Q_ASSERT
    // should it ever be used.
    nDhtTblDcY = m_anDhtTblSel[DHT_CLASS_DC][COMP_IND_YCC_Y];
    nDhtTblAcY = m_anDhtTblSel[DHT_CLASS_AC][COMP_IND_YCC_Y];
    nDhtTblDcCb = m_anDhtTblSel[DHT_CLASS_DC][COMP_IND_YCC_CB];
    nDhtTblAcCb = m_anDhtTblSel[DHT_CLASS_AC][COMP_IND_YCC_CB];
    nDhtTblDcCr = m_anDhtTblSel[DHT_CLASS_DC][COMP_IND_YCC_CR];
    nDhtTblAcCr = m_anDhtTblSel[DHT_CLASS_AC][COMP_IND_YCC_CR];
#ifdef DEBUG_YCCK
    nDhtTblDcK = m_anDhtTblSel[DHT_CLASS_DC][COMP_IND_YCC_K];
    nDhtTblAcK = m_anDhtTblSel[DHT_CLASS_AC][COMP_IND_YCC_K];
#endif
  }

  // Done checks

  // Inform if they are in AC+DC/DC mode
  if(!bQuiet)
  {
    if(m_bDecodeScanAc)
    {
      m_pLog->AddLine("  Scan Decode Mode: Full IDCT (AC + DC)");
    }
    else
    {
      m_pLog->AddLine("  Scan Decode Mode: No IDCT (DC only)");
      m_pLog-> AddLineWarn("    NOTE: Low-resolution DC component shown. Can decode full-res with [Options->Scan Segment->Full IDCT]");
    }

    m_pLog->AddLine("");
  }

  // Report any Buffer overlays
  m_pWBuf->ReportOverlays(m_pLog);

  m_nNumPixels = 0;

  // Clear the histogram and color correction clipping stats
  if(bDisplay)
  {
    memset(&m_sStatClip, 0, sizeof(m_sStatClip));
    memset(&m_sHisto, 0, sizeof(m_sHisto));

    // FIXME: Histo should now be done after color convert
    memset(&m_anCcHisto_r, 0, sizeof(m_anCcHisto_r));
    memset(&m_anCcHisto_g, 0, sizeof(m_anCcHisto_g));
    memset(&m_anCcHisto_b, 0, sizeof(m_anCcHisto_b));

    memset(&m_anHistoYFull, 0, sizeof(m_anHistoYFull));
    memset(&m_anHistoYSubset, 0, sizeof(m_anHistoYSubset));
  }

  // -----------------------------------------------------------------------
  // Process all scan MCUs
  // -----------------------------------------------------------------------

  for(uint32_t nMcuY = nDecMcuRowStart; nMcuY < nDecMcuRowEndFinal; nMcuY++)
  {
    // Set the statusbar text to Processing...
    strTmp = QString("Decoding Scan Data... Row %1 of %2 (%3%%)")
        .arg(nMcuY, 4, 10, QChar('0'))
        .arg(m_nMcuYMax, 4, 10, QChar('0'))
        .arg(nMcuY * 100.0 / m_nMcuYMax, 3, 'f', 0);
    SetStatusText(strTmp);

    // TODO: Trap escape keypress here (or run as thread)

    bool bDscRet;               // Return value for DecodeScanComp()
    bool bScanStop = false;

    for(uint32_t nMcuX = 0; (nMcuX < m_nMcuXMax) && (!bScanStop); nMcuX++)
    {
      // Check to see if we should expect a restart marker!
      // FIXME: Should actually check to ensure that we do in
      // fact get a restart marker, and that it was the right one!
      if((m_bRestartEn) && (m_nRestartMcusLeft == 0))
      {
        /*
           if (m_bVerbose) {
           strTmp = QString("  Expect Restart interval elapsed @ %s"),GetScanBufPos();
           m_pLog->AddLine(strTmp);
           }
         */

        if(m_bRestartRead)
        {
          /*
             // FIXME: Check for restart counter value match
             if (m_bVerbose) {
             strTmp = QString("  Restart marker matched");
             m_pLog->AddLine(strTmp);
             }
           */
        }
        else
        {
          strTmp = QString("  Expect Restart interval elapsed @ %1").arg(GetScanBufPos());
          m_pLog->AddLine(strTmp);
          strTmp = QString("    ERROR: Restart marker not detected");
          m_pLog->AddLineErr(strTmp);
        }
        /*
           if (ExpectRestart()) {
           if (m_bVerbose) {
           strTmp = QString("  Restart marker detected");
           m_pLog->AddLine(strTmp);
           }
           } else {
           strTmp = QString("  ERROR: Restart marker expected but not found @ %s"),GetScanBufPos();
           m_pLog->AddLineErr(strTmp);
           }
         */

      }

      // To support a fast decode mode, allow for a subset of the
      // image to have DC+AC decoding, while the remainder is only DC decoding
      if((nMcuY < nDecMcuRowStart) || (nMcuY > nDecMcuRowEnd))
      {
        m_bDecodeScanAc = false;
      }
      else
      {
        m_bDecodeScanAc = bDecodeScanAc;
      }

      // Precalculate MCU matrix index
      uint32_t nMcuXY = nMcuY * m_nMcuXMax + nMcuX;

      // Mark the start of the MCU in the file map
      m_pMcuFileMap[nMcuXY] = PackFileOffset(m_anScanBuffPtr_pos[0], m_nScanBuffPtr_align);

      // Is this an MCU that we want full printing of decode process?
      bool bVlcDump = false;

      uint32_t nRangeBase;
      uint32_t nRangeCur;

      if(m_bDetailVlc)
      {
        nRangeBase = (m_nDetailVlcY * m_nMcuXMax) + m_nDetailVlcX;
        nRangeCur = nMcuXY;

        if((nRangeCur >= nRangeBase) && (nRangeCur < nRangeBase + m_nDetailVlcLen))
        {
          bVlcDump = true;
        }
      }

      // Luminance
      // If there is chroma subsampling, then this block will have (css_x * css_y) luminance blocks to process
      // We store them all in an array m_anDcLumCss[]

      // Give separator line between MCUs
      if(bVlcDump)
      {
        m_pLog->AddLine("");
      }

      // CSS array indices
      uint32_t nCssIndH;
      uint32_t nCssIndV;
      uint32_t nComp;

      // No need to reset the IDCT output matrix for this MCU
      // since we are going to be generating it here. This help
      // maintain performance.

      // --------------------------------------------------------------
      nComp = SCAN_COMP_Y;

      // Step through the sampling factors per image component
      // TODO: Could rewrite this to use single loop across each image component
      for(nCssIndV = 0; nCssIndV < m_anSampPerMcuV[nComp]; nCssIndV++)
      {
        for(nCssIndH = 0; nCssIndH < m_anSampPerMcuH[nComp]; nCssIndH++)
        {
          if(!bVlcDump)
          {
            bDscRet = DecodeScanComp(nDhtTblDcY, nDhtTblAcY, nDqtTblY, nMcuX, nMcuY);   // Lum DC+AC
          }
          else
          {
            bDscRet = DecodeScanCompPrint(nDhtTblDcY, nDhtTblAcY, nDqtTblY, nMcuX, nMcuY);      // Lum DC+AC
          }
          
          if(m_nScanCurErr)
          {
            CheckScanErrors(nMcuX, nMcuY, nCssIndH, nCssIndV, nComp);
          }
          
          if(!bDscRet && bDieOnFirstErr)
          {
            return;
          }

          // The DCT Block matrix has already been dezigzagged
          // and multiplied against quantization table entry
          m_nDcLum += m_anDctBlock[DCT_COEFF_DC];

          if(bVlcDump)
          {
//                                              PrintDcCumVal(nMcuX,nMcuY,m_nDcLum);
          }

          // Now take a snapshot of the current cumulative DC value
          m_anDcLumCss[nCssIndV * MAX_SAMP_FACT_H + nCssIndH] = m_nDcLum;

          // At this point we have one of the luminance comps
          // fully decoded (with IDCT if enabled). The result is
          // currently in the array: m_afIdctBlock[]
          // The next step would be to move these elements into
          // the 3-channel MCU image map

          // Store the pixels associated with this channel into
          // the full-res pixel map. IDCT has already been computed
          // on the 8x8 (or larger) MCU block region.

#if 1
          if(bDisplay)
          {
            SetFullRes(nMcuX, nMcuY, nComp, nCssIndH, nCssIndV, m_nDcLum);
          }
#else
          // FIXME
          // Temporarily experiment with trying to handle multiple scans
          // by converting sampling factor of luminance scan back to 1x1
          uint32_t nNewMcuX, nNewMcuY, nNewCssX, nNewCssY;

          if(nCssIndV == 0)
          {
            if(nMcuX < m_nMcuXMax / 2)
            {
              nNewMcuX = nMcuX;
              nNewMcuY = nMcuY;
              nNewCssY = 0;
            }
            else
            {
              nNewMcuX = nMcuX - (m_nMcuXMax / 2);
              nNewMcuY = nMcuY;
              nNewCssY = 1;
            }
            nNewCssX = nCssIndH;
            SetFullRes(nNewMcuX, nNewMcuY, SCAN_COMP_Y, nNewCssX, nNewCssY, m_nDcLum);
          }
          else
          {
            nNewMcuX = (nMcuX / 2) + 1;
            nNewMcuY = (nMcuY / 2);
            nNewCssX = nCssIndH;
            nNewCssY = nMcuY % 2;
          }
#endif

          // ---------------

          // TODO: Counting pixels makes assumption that luminance is
          // not subsampled, so we increment by 64.
          m_nNumPixels += BLK_SZ_X * BLK_SZ_Y;
        }
      }

      // In a grayscale image, we don't do this part!
      //if (m_nNumSofComps == NUM_CHAN_YCC) {
      if(m_nNumSosComps == NUM_CHAN_YCC)
      {
        // --------------------------------------------------------------
        nComp = SCAN_COMP_CB;

        // Chrominance Cb
        for(nCssIndV = 0; nCssIndV < m_anSampPerMcuV[nComp]; nCssIndV++)
        {
          for(nCssIndH = 0; nCssIndH < m_anSampPerMcuH[nComp]; nCssIndH++)
          {
            if(!bVlcDump)
            {
              bDscRet = DecodeScanComp(nDhtTblDcCb, nDhtTblAcCb, nDqtTblCb, nMcuX, nMcuY);      // Chr Cb DC+AC
            }
            else
            {
              bDscRet = DecodeScanCompPrint(nDhtTblDcCb, nDhtTblAcCb, nDqtTblCb, nMcuX, nMcuY); // Chr Cb DC+AC
            }
            
            if(m_nScanCurErr)
            {
              CheckScanErrors(nMcuX, nMcuY, nCssIndH, nCssIndV, nComp);
            }
            
            if(!bDscRet && bDieOnFirstErr)
            {
              return;
            }

            m_nDcChrCb += m_anDctBlock[DCT_COEFF_DC];

            if(bVlcDump)
            {
              //PrintDcCumVal(nMcuX,nMcuY,m_nDcChrCb);
            }

            // Now take a snapshot of the current cumulative DC value
            m_anDcChrCbCss[nCssIndV * MAX_SAMP_FACT_H + nCssIndH] = m_nDcChrCb;

            // Store fullres value
            if(bDisplay)
            {
              SetFullRes(nMcuX, nMcuY, nComp, nCssIndH, nCssIndV, m_nDcChrCb);
            }
          }
        }

        // --------------------------------------------------------------
        nComp = SCAN_COMP_CR;

        // Chrominance Cr
        for(nCssIndV = 0; nCssIndV < m_anSampPerMcuV[nComp]; nCssIndV++)
        {
          for(nCssIndH = 0; nCssIndH < m_anSampPerMcuH[nComp]; nCssIndH++)
          {
            if(!bVlcDump)
            {
              bDscRet = DecodeScanComp(nDhtTblDcCr, nDhtTblAcCr, nDqtTblCr, nMcuX, nMcuY);      // Chr Cr DC+AC
            }
            else
            {
              bDscRet = DecodeScanCompPrint(nDhtTblDcCr, nDhtTblAcCr, nDqtTblCr, nMcuX, nMcuY); // Chr Cr DC+AC
            }
            
            if(m_nScanCurErr)
            {
              CheckScanErrors(nMcuX, nMcuY, nCssIndH, nCssIndV, nComp);
            }
            
            if(!bDscRet && bDieOnFirstErr)
            {
             return;
            }

            m_nDcChrCr += m_anDctBlock[DCT_COEFF_DC];

            if(bVlcDump)
            {
              //PrintDcCumVal(nMcuX,nMcuY,m_nDcChrCr);
            }

            // Now take a snapshot of the current cumulative DC value
            m_anDcChrCrCss[nCssIndV * MAX_SAMP_FACT_H + nCssIndH] = m_nDcChrCr;

            // Store fullres value
            if(bDisplay)
            {
              SetFullRes(nMcuX, nMcuY, nComp, nCssIndH, nCssIndV, m_nDcChrCr);
            }
          }
        }
      }

#ifdef DEBUG_YCCK
      else if(m_nNumSosComps == NUM_CHAN_YCCK)
      {
        // --------------------------------------------------------------
        nComp = SCAN_COMP_CB;

        // Chrominance Cb
        for(nCssIndV = 0; nCssIndV < m_anSampPerMcuV[nComp]; nCssIndV++)
        {
          for(nCssIndH = 0; nCssIndH < m_anSampPerMcuH[nComp]; nCssIndH++)
          {
            if(!bVlcDump)
            {
              bDscRet = DecodeScanComp(nDhtTblDcCb, nDhtTblAcCb, nDqtTblCb, nMcuX, nMcuY);      // Chr Cb DC+AC
            }
            else
            {
              bDscRet = DecodeScanCompPrint(nDhtTblDcCb, nDhtTblAcCb, nDqtTblCb, nMcuX, nMcuY); // Chr Cb DC+AC
            }
            
            if(m_nScanCurErr)
            {
              CheckScanErrors(nMcuX, nMcuY, nCssIndH, nCssIndV, nComp);
            }
            
            if(!bDscRet && bDieOnFirstErr)
            {
              return;
            }

            m_nDcChrCb += m_anDctBlock[DCT_COEFF_DC];

            if(bVlcDump)
            {
              //PrintDcCumVal(nMcuX,nMcuY,m_nDcChrCb);
            }

            // Now take a snapshot of the current cumulative DC value
            m_anDcChrCbCss[nCssIndV * MAX_SAMP_FACT_H + nCssIndH] = m_nDcChrCb;

            // Store fullres value
            if(bDisplay)
            {
              SetFullRes(nMcuX, nMcuY, nComp, 0, 0, m_nDcChrCb);
            }
          }
        }

        // --------------------------------------------------------------
        nComp = SCAN_COMP_CR;

        // Chrominance Cr
        for(nCssIndV = 0; nCssIndV < m_anSampPerMcuV[nComp]; nCssIndV++)
        {
          for(nCssIndH = 0; nCssIndH < m_anSampPerMcuH[nComp]; nCssIndH++)
          {
            if(!bVlcDump)
            {
              bDscRet = DecodeScanComp(nDhtTblDcCr, nDhtTblAcCr, nDqtTblCr, nMcuX, nMcuY);      // Chr Cr DC+AC
            }
            else
            {
              bDscRet = DecodeScanCompPrint(nDhtTblDcCr, nDhtTblAcCr, nDqtTblCr, nMcuX, nMcuY); // Chr Cr DC+AC
            }
            
            if(m_nScanCurErr)
              CheckScanErrors(nMcuX, nMcuY, nCssIndH, nCssIndV, nComp);
            
            if(!bDscRet && bDieOnFirstErr)
              return;

            m_nDcChrCr += m_anDctBlock[DCT_COEFF_DC];

            if(bVlcDump)
            {
              //PrintDcCumVal(nMcuX,nMcuY,m_nDcChrCr);
            }

            // Now take a snapshot of the current cumulative DC value
            m_anDcChrCrCss[nCssIndV * MAX_SAMP_FACT_H + nCssIndH] = m_nDcChrCr;

            // Store fullres value
            if(bDisplay)
              SetFullRes(nMcuX, nMcuY, nComp, 0, 0, m_nDcChrCr);
          }
        }

        // --------------------------------------------------------------
        // IGNORED
        nComp = SCAN_COMP_K;

        // Black K
        for(nCssIndV = 0; nCssIndV < m_anSampPerMcuV[nComp]; nCssIndV++)
        {
          for(nCssIndH = 0; nCssIndH < m_anSampPerMcuH[nComp]; nCssIndH++)
          {

            if(!bVlcDump)
            {
              bDscRet = DecodeScanComp(nDhtTblDcK, nDhtTblAcK, nDqtTblK, nMcuX, nMcuY); // K DC+AC
            }
            else
            {
              bDscRet = DecodeScanCompPrint(nDhtTblDcK, nDhtTblAcK, nDqtTblK, nMcuX, nMcuY);    // K DC+AC
            }
            
            if(m_nScanCurErr)
              CheckScanErrors(nMcuX, nMcuY, nCssIndH, nCssIndV, nComp);
            
            if(!bDscRet && bDieOnFirstErr)
              return;

/*
						m_nDcChrK += m_anDctBlock[DCT_COEFF_DC];

						if (bVlcDump) {
							//PrintDcCumVal(nMcuX,nMcuY,m_nDcChrCb);
						}

						// Now take a snapshot of the current cumulative DC value
						m_anDcChrKCss[nCssIndV*MAX_SAMP_FACT_H+nCssIndH] = m_nDcChrK;

						// Store fullres value
						if (bDisplay)
							SetFullRes(nMcuX,nMcuY,nComp,0,0,m_nDcChrK);
*/

          }
        }
      }
#endif
      // --------------------------------------------------------------------

      uint32_t nBlkXY;

      // Now save the DC YCC values (expanded per 8x8 block)
      // without ranging or translation into RGB.
      //
      // We enter this code once per MCU so we need to expand
      // out to cover all blocks in this MCU.

      // --------------------------------------------------------------
      nComp = SCAN_COMP_Y;

      // Calculate top-left corner of MCU in block map
      // and then linear offset into block map
      uint32_t nBlkCornerMcuX, nBlkCornerMcuY, nBlkCornerMcuLinear;

      nBlkCornerMcuX = nMcuX * m_anExpandBitsMcuH[nComp];
      nBlkCornerMcuY = nMcuY * m_anExpandBitsMcuV[nComp];
      nBlkCornerMcuLinear = (nBlkCornerMcuY * m_nBlkXMax) + nBlkCornerMcuX;

      // Now step through each block in the MCU per subsampling
      for(nCssIndV = 0; nCssIndV < m_anSampPerMcuV[nComp]; nCssIndV++)
      {
        for(nCssIndH = 0; nCssIndH < m_anSampPerMcuH[nComp]; nCssIndH++)
        {
          // Calculate upper-left Blk index
          // FIXME: According to code analysis the following write assignment
          // to m_pBlkDcValY[] can apparently exceed the buffer bounds (C6386).
          // I have not yet determined what scenario can lead to
          // this. So for now, add in specific clause to trap and avoid.
          nBlkXY = nBlkCornerMcuLinear + (nCssIndV * m_nBlkXMax) + nCssIndH;
          
          // FIXME: Temporarily catch any range issue
          if(nBlkXY >= m_nBlkXMax * m_nBlkYMax)
          {
#ifdef DEBUG_LOG
            QString strDebug;

            strTmp = QString("DecodeScanImg() with nBlkXY out of range. nBlkXY=[%1] m_nBlkXMax=[%2] m_nBlkYMax=[%3]")
                .arg(nBlkXY)
                .arg(m_nBlkXMax)
                .arg(m_nBlkYMax);
            strDebug = QString("## File=[%1] Block=[%2] Error=[%3]\n")
                .arg(m_pAppConfig->strCurFname, -100)
                .arg("ImgDecode", -10).arg(strTmp);
            qDebug() << strDebug;
#else
            Q_ASSERT(false);
#endif
          }
          else
          {
            m_pBlkDcValY[nBlkXY] = m_anDcLumCss[nCssIndV * MAX_SAMP_FACT_H + nCssIndH];
          }
        }
      }

      // Only process the chrominance if it is YCC
      if(m_nNumSosComps == NUM_CHAN_YCC)
      {
        // --------------------------------------------------------------
        nComp = SCAN_COMP_CB;

        for(nCssIndV = 0; nCssIndV < m_anSampPerMcuV[nComp]; nCssIndV++)
        {
          for(nCssIndH = 0; nCssIndH < m_anSampPerMcuH[nComp]; nCssIndH++)
          {
            // Calculate upper-left Blk index
            nBlkXY = (nMcuY * m_anExpandBitsMcuV[nComp] + nCssIndV) * m_nBlkXMax + (nMcuX * m_anExpandBitsMcuH[nComp] + nCssIndH);

            // FIXME: Temporarily catch any range issue
            if(nBlkXY >= m_nBlkXMax * m_nBlkYMax)
            {
#ifdef DEBUG_LOG
              QString strDebug;

              strTmp = QString("DecodeScanImg() with nBlkXY out of range. nBlkXY=[%1] m_nBlkXMax=[%2] m_nBlkYMax=[%3]").arg(nBlkXY).arg(m_nBlkXMax).arg(m_nBlkYMax);
              strDebug = QString("## File=[%1] Block=[%2] Error=[%3]\n").arg(m_pAppConfig->strCurFname, -100).arg("ImgDecode", -10).arg(strTmp);
              qDebug() << strDebug;
#else
              Q_ASSERT(false);
#endif
            }
            else
            {
              m_pBlkDcValCb[nBlkXY] = m_anDcChrCbCss[nCssIndV * MAX_SAMP_FACT_H + nCssIndH];
            }
          }
        }

        // --------------------------------------------------------------
        nComp = SCAN_COMP_CR;

        for(nCssIndV = 0; nCssIndV < m_anSampPerMcuV[nComp]; nCssIndV++)
        {
          for(nCssIndH = 0; nCssIndH < m_anSampPerMcuH[nComp]; nCssIndH++)
          {
            // Calculate upper-left Blk index
            nBlkXY = (nMcuY * m_anExpandBitsMcuV[nComp] + nCssIndV) * m_nBlkXMax + (nMcuX * m_anExpandBitsMcuH[nComp] + nCssIndH);

            // FIXME: Temporarily catch any range issue
            if(nBlkXY >= m_nBlkXMax * m_nBlkYMax)
            {
#ifdef DEBUG_LOG
              QString strDebug;

              strTmp = QString("DecodeScanImg() with nBlkXY out of range. nBlkXY=[%1] m_nBlkXMax=[%2] m_nBlkYMax=[%3]").arg(nBlkXY).arg(m_nBlkXMax).arg(m_nBlkYMax);
              strDebug = QString("## File=[%1] Block=[%2] Error=[%3]\n").arg(m_pAppConfig->strCurFname, -100).arg("ImgDecode", -10).arg(strTmp);
              qDebug() << strDebug;
#else
              Q_ASSERT(false);
#endif
            }
            else
            {
              m_pBlkDcValCr[nBlkXY] = m_anDcChrCrCss[nCssIndV * MAX_SAMP_FACT_H + nCssIndH];
            }
          }
        }
      }

      // Now that we finished an MCU, decrement the restart interval counter
      if(m_bRestartEn)
      {
        m_nRestartMcusLeft--;
      }

      // Check to see if we need to abort for some reason.
      // Note that only check m_bScanEnd if we have a failure.
      // m_bScanEnd is Q_ASSERTed during normal out-of-data when scan
      // segment ends with marker. We don't want to abort early
      // or else we'll not decode the last MCU or two!
      if(m_bScanEnd && m_bScanBad)
      {
        bScanStop = true;
      }
    }                           // nMcuX
  }                             // nMcuY

  if(!bQuiet)
  {
    m_pLog->AddLine("");
  }

  // ---------------------------------------------------------

  // Now we can create the final preview. Since we have just finished
  // decoding a new image, we need to ensure that we invalidate
  // the temporary preview (multi-channel option). Done earlier
  // with PREVIEW_NONE
  if(bDisplay)
  {
    CalcChannelPreview();
  }

  // DIB is ready for display now
  if(bDisplay)
  {
    m_bDibTempReady = true;
    m_bPreviewIsJpeg = true;
  }

  // ------------------------------------
  // Report statistics

  if(!bQuiet)
  {
    // Report Compression stats
    // TODO: Should we use m_nNumSofComps?
    strTmp = QString("  Compression stats:");
    m_pLog->AddLine(strTmp);
    double nCompressionRatio =
      static_cast<double>(m_nDimX * m_nDimY * m_nNumSosComps * 8) /
      static_cast<double>((m_anScanBuffPtr_pos[0] - m_nScanBuffPtr_first) * 8);
    strTmp = QString("    Compression Ratio: %1:1").arg(nCompressionRatio, 5, 'f', 2);
    m_pLog->AddLine(strTmp);

    double nBitsPerPixel =
        static_cast<double>((m_anScanBuffPtr_pos[0] - m_nScanBuffPtr_first) * 8) /
        static_cast<double>(m_nDimX * m_nDimY);

    strTmp = QString("    Bits per pixel:    %1:1").arg(nBitsPerPixel, 5, 'f', 2);
    m_pLog->AddLine(strTmp);
    m_pLog->AddLine("");

    // Report Huffman stats
    strTmp = QString("  Huffman code histogram stats:");
    m_pLog->AddLine(strTmp);

    uint32_t nDhtHistoTotal;

    for(uint32_t nClass = DHT_CLASS_DC; nClass <= DHT_CLASS_AC; nClass++)
    {
      for(uint32_t nDhtDestId = 0; nDhtDestId <= m_anDhtLookupSetMax[nClass]; nDhtDestId++)
      {
        nDhtHistoTotal = 0;

        for(uint32_t nBitLen = 1; nBitLen <= MAX_DHT_CODELEN; nBitLen++)
        {
          nDhtHistoTotal += m_anDhtHisto[nClass][nDhtDestId][nBitLen];
        }

        strTmp = QString("    Huffman Table: (Dest ID: %1, Class: %2)").arg(nDhtDestId).arg(nClass?"AC":"DC");
        m_pLog->AddLine(strTmp);

        for(uint32_t nBitLen = 1; nBitLen <= MAX_DHT_CODELEN; nBitLen++)
        {
          strTmp = QString("      # codes of length %1 bits: %2 (%3%)")
              .arg(nBitLen, 2, 10, QChar('0'))
              .arg(m_anDhtHisto[nClass][nDhtDestId][nBitLen], 8)
              .arg((m_anDhtHisto[nClass][nDhtDestId][nBitLen]*100.0)/nDhtHistoTotal, 3, 'f', 0);
          m_pLog->AddLine(strTmp);
        }

        m_pLog->AddLine("");
      }
    }

    // Report YCC stats
    ReportColorStats();
  }                             // !bQuiet

  // ------------------------------------

  // Display the image histogram if enabled
  if(bDisplay && m_bHistEn)
  {
    DrawHistogram(bQuiet, bDumpHistoY);
  }

  if(bDisplay && m_bAvgYValid)
  {
    m_pLog->AddLine("  Average Pixel Luminance (Y):");
    m_pLog->AddLine(QString("    Y=[%1] (range: 0..255)").arg(m_nAvgY, 3));
    m_pLog->AddLine("");
  }

  if(bDisplay && m_bBrightValid)
  {
    m_pLog->AddLine("  Brightest Pixel Search:");
    strTmp = QString("    YCC=[%1,%2,%3] RGB=[%4,%5,%6] @ MCU[%7,%8]")
        .arg(m_nBrightY, 5)
        .arg(m_nBrightCb, 5)
        .arg(m_nBrightCr, 5)
        .arg(m_nBrightR, 3)
        .arg(m_nBrightG, 3)
        .arg(m_nBrightB, 3)
        .arg(m_ptBrightMcu.x(), 3)
        .arg(m_ptBrightMcu.y(), 3);
    m_pLog->AddLine(strTmp);
    m_pLog->AddLine("");
  }

  // --------------------------------------

  if(!bQuiet)
  {
    m_pLog->AddLine("  Finished Decoding SCAN Data");
    strTmp = QString("    Number of RESTART markers decoded: %1").arg(m_nRestartRead);
    m_pLog->AddLine(strTmp);
    strTmp = QString("    Next position in scan buffer: Offset %1").arg(GetScanBufPos());
    m_pLog->AddLine(strTmp);
    m_pLog->AddLine("");
  }

  // --------------------------------------
  // Write out the full Y histogram if requested!

  QString strFull;

  if(bDisplay && m_bHistEn && bDumpHistoY)
  {
    ReportHistogramY();
  }
}

//
// Report if image preview is ready to display
//
// RETURN:
// - True if image preview is ready
//
bool CimgDecode::IsPreviewReady()
{
  return m_bPreviewIsJpeg;
}

// Report out the color conversion statistics
//
// PRE:
// - m_sStatClip
// - m_sHisto
//
void CimgDecode::ReportColorStats()
{
  QString strTmp;

  // Report YCC stats
  if(CC_CLIP_YCC_EN)
  {
    strTmp = QString("  YCC clipping in DC:");
    m_pLog->AddLine(strTmp);
    strTmp = QString("    Y  component: [<0=%1] [>255=%2]").arg(m_sStatClip.nClipYUnder, 5).arg( m_sStatClip.nClipYOver, 5);
    m_pLog->AddLine(strTmp);
    strTmp = QString("    Cb component: [<0=%1] [>255=%2]").arg(m_sStatClip.nClipCbUnder, 5).arg( m_sStatClip.nClipCbOver, 5);
    m_pLog->AddLine(strTmp);
    strTmp = QString("    Cr component: [<0=%1] [>255=%2]").arg(m_sStatClip.nClipCrUnder, 5).arg( m_sStatClip.nClipCrOver, 5);
    m_pLog->AddLine(strTmp);
    m_pLog->AddLine("");
  }

  if(m_bHistEn)
  {
    strTmp = QString("  YCC histogram in DC (DCT sums : pre-ranged:");
    m_pLog->AddLine(strTmp);
    strTmp = QString("    Y  component histo: [min=%1 max=%2 avg=%3]")
        .arg(m_sHisto.nPreclipYMin, 5)
        .arg(m_sHisto.nPreclipYMax, 5)
        .arg(static_cast<double>(m_sHisto.nPreclipYSum) / static_cast<double>(m_sHisto.nCount), 7, 'f', 1);
    m_pLog->AddLine(strTmp);
    strTmp = QString("    Cb component histo: [min=%1 max=%2 avg=%3]")
        .arg(m_sHisto.nPreclipCbMin, 5)
        .arg(m_sHisto.nPreclipCbMax, 5)
        .arg(static_cast<double>(m_sHisto.nPreclipCbSum) / static_cast<double>(m_sHisto.nCount), 7, 'f', 1);
    m_pLog->AddLine(strTmp);
    strTmp = QString("    Cr component histo: [min=%1 max=%2 avg=%3]")
        .arg(m_sHisto.nPreclipCrMin, 5)
        .arg(m_sHisto.nPreclipCrMax, 5)
        .arg(static_cast<double>(m_sHisto.nPreclipCrSum) / static_cast<double>(m_sHisto.nCount), 7, 'f', 1);
    m_pLog->AddLine(strTmp);
    m_pLog->AddLine("");

    strTmp = QString("  YCC histogram in DC:");
    m_pLog->AddLine(strTmp);
    strTmp = QString("    Y  component histo: [min=%1 max=%2 avg=%3]")
        .arg(m_sHisto.nClipYMin, 5)
        .arg(m_sHisto.nClipYMax, 5)
        .arg(static_cast<double>(m_sHisto.nClipYSum) / static_cast<double>(m_sHisto.nCount), 7, 'f', 1);
    m_pLog->AddLine(strTmp);
    strTmp = QString("    Cb component histo: [min=%1 max=%2 avg=%3]")
        .arg(m_sHisto.nClipCbMin, 5)
        .arg(m_sHisto.nClipCbMax, 5)
        .arg(static_cast<double>(m_sHisto.nClipCbSum) / static_cast<double>(m_sHisto.nCount), 7, 'f', 1);
    m_pLog->AddLine(strTmp);
    strTmp = QString("    Cr component histo: [min=%1 max=%2 avg=%3]")
        .arg(m_sHisto.nClipCrMin, 5)
        .arg(m_sHisto.nClipCrMax, 5)
        .arg(static_cast<double>(m_sHisto.nClipCrSum) / static_cast<double>(m_sHisto.nCount), 7, 'f', 1);
    m_pLog->AddLine(strTmp);
    m_pLog->AddLine("");

    strTmp = QString("  RGB histogram in DC (before clip):");
    m_pLog->AddLine(strTmp);
    strTmp = QString("    R  component histo: [min=%1 max=%2 avg=%3]")
        .arg(m_sHisto.nPreclipRMin, 5)
        .arg(m_sHisto.nPreclipRMax, 5)
        .arg(static_cast<double>(m_sHisto.nPreclipRSum) / static_cast<double>(m_sHisto.nCount), 7, 'f', 1);
    m_pLog->AddLine(strTmp);
    strTmp = QString("    G  component histo: [min=%1 max=%2 avg=%3]")
        .arg(m_sHisto.nPreclipGMin, 5)
        .arg(m_sHisto.nPreclipGMax, 5)
        .arg(static_cast<double>(m_sHisto.nPreclipGSum) / static_cast<double>(m_sHisto.nCount), 7, 'f', 1);
    m_pLog->AddLine(strTmp);
    strTmp = QString("    B  component histo: [min=%1 max=%2 avg=%3]")
        .arg(m_sHisto.nPreclipBMin, 5)
        .arg(m_sHisto.nPreclipBMax, 5)
        .arg(static_cast<double>(m_sHisto.nPreclipBSum) / static_cast<double>(m_sHisto.nCount), 7, 'f', 1);
    m_pLog->AddLine(strTmp);
    m_pLog->AddLine("");
  }

  strTmp = QString("  RGB clipping in DC:");
  m_pLog->AddLine(strTmp);
  strTmp = QString("    R  component: [<0=%1] [>255=%2]")
      .arg(m_sStatClip.nClipRUnder, 5)
      .arg(m_sStatClip.nClipROver, 5);
  m_pLog->AddLine(strTmp);
  strTmp = QString("    G  component: [<0=%1] [>255=%2]")
      .arg(m_sStatClip.nClipGUnder, 5)
      .arg(m_sStatClip.nClipGOver, 5);
  m_pLog->AddLine(strTmp);
  strTmp = QString("    B  component: [<0=%1] [>255=%2]")
      .arg(m_sStatClip.nClipBUnder, 5)
      .arg(m_sStatClip.nClipBOver, 5);
  m_pLog->AddLine(strTmp);
  /*
     strTmp = QString("    White Highlight:         [>255=%5u]")).arg(m_sStatClip.nClipWhiteOver;
     m_pLog->AddLine(strTmp);
   */
  m_pLog->AddLine("");
}

// Report the histogram stats from the Y component
//
// PRE:
// - m_anHistoYFull
//
void CimgDecode::ReportHistogramY()
{
  QString strFull;
  QString strTmp;

  m_pLog->AddLine("  Y Histogram in DC: (DCT sums) Full");

  for(uint32_t row = 0; row < 2048 / 8; row++)
  {
    strFull = QString("    Y=%1..%2: ")
        .arg(-1024 + (static_cast<int32_t>(row) * 8), 5)
        .arg(-1024 + (static_cast<int32_t>(row) * 8) + 7, 5);

    for(uint32_t col = 0; col < 8; col++)
    {
      strTmp = QString("0x%1, ").arg(m_anHistoYFull[col + row * 8], 6, 16, QChar('0'));
      strFull += strTmp;
    }

    m_pLog->AddLine(strFull);
  }
}

// Draw the histograms (RGB and/or Y)
//
// INPUT:
// - bQuiet                                     = Calculate stats without reporting to log?
// - bDumpHistoY                        = Generate the Y histogram?
// PRE:
// - m_sHisto
//
void CimgDecode::DrawHistogram(bool bQuiet, bool bDumpHistoY)
{
  QString strTmp;

  if(!bQuiet)
  {
    strTmp = QString("  RGB histogram in DC (after clip):");
    m_pLog->AddLine(strTmp);
    strTmp = QString("    R  component histo: [min=%1 max=%2 avg=%3]")
        .arg(m_sHisto.nClipRMin, 5)
        .arg(m_sHisto.nClipRMax, 5)
        .arg(static_cast<double>(m_sHisto.nClipRSum) / static_cast<double>(m_sHisto.nCount), 7, 'f', 1);
    m_pLog->AddLine(strTmp);
    strTmp = QString("    G  component histo: [min=%1 max=%2 avg=%3]")
        .arg(m_sHisto.nClipGMin, 5)
        .arg(m_sHisto.nClipGMax, 5)
        .arg(static_cast<double>(m_sHisto.nClipGSum) / static_cast<double>(m_sHisto.nCount), 7, 'f', 1);
    m_pLog->AddLine(strTmp);
    strTmp = QString("    B  component histo: [min=%1 max=%2 avg=%3]")
        .arg(m_sHisto.nClipBMin, 5)
        .arg(m_sHisto.nClipBMax, 5)
        .arg(static_cast<double>(m_sHisto.nClipBSum) / static_cast<double>(m_sHisto.nCount), 7, 'f', 1);
    m_pLog->AddLine(strTmp);
    m_pLog->AddLine("");
  }

  // --------------------------------------
  // Now draw the RGB histogram!

  int32_t nCoordY;
  int32_t rgbHistOffset;
  int32_t nHistoBinHeight;
  int32_t nHistoPeakVal;
  int32_t nHistoX;
  int32_t nHistoCurVal;

  m_bDibHistRgbReady = false;

  m_pDibHistRgb = new QImage(HISTO_BINS * HISTO_BIN_WIDTH, 3 * HISTO_BIN_HEIGHT_MAX, QImage::Format_RGB32);
  m_pDibHistRgb->fill(Qt::white);

  // Do peak detect first
  // Don't want to reset peak value to 0 as otherwise we might get
  // division by zero later when we calculate nHistoBinHeight
  nHistoPeakVal = 1;

  // Peak value is across all three channels!
  for(int32_t nHistChan = 0; nHistChan < 3; nHistChan++)
  {
    for(int32_t i = 0; i < HISTO_BINS; i++)
    {
      if(nHistChan == 0)
      {
        nHistoCurVal = m_anCcHisto_r[i];
      }
      else if(nHistChan == 1)
      {
        nHistoCurVal = m_anCcHisto_g[i];
      }
      else
      {
        nHistoCurVal = m_anCcHisto_b[i];
      }

      nHistoPeakVal = (nHistoCurVal > nHistoPeakVal) ? nHistoCurVal : nHistoPeakVal;
    }
  }

  for(int32_t nHistChan = 0; nHistChan < 3; nHistChan++)
  {
    if(nHistChan == CHAN_R)
    {
      rgbHistOffset = HISTO_BIN_HEIGHT_MAX - 1;
    }
    else if(nHistChan == CHAN_G)
    {
      rgbHistOffset = HISTO_BIN_HEIGHT_MAX * 2 - 1;
    }
    else
    {
      rgbHistOffset = HISTO_BIN_HEIGHT_MAX * 3 - 1;
    }

    for(int32_t i = 0; i < HISTO_BINS; i++)
    {
      // Calculate bin's height (max HISTO_BIN_HEIGHT_MAX)
      if(nHistChan == CHAN_R)
      {
        nHistoCurVal = m_anCcHisto_r[i];
      }
      else if(nHistChan == CHAN_G)
      {
        nHistoCurVal = m_anCcHisto_g[i];
      }
      else
      {
        nHistoCurVal = m_anCcHisto_b[i];
      }

      nHistoBinHeight = HISTO_BIN_HEIGHT_MAX * nHistoCurVal / nHistoPeakVal;

      for(int32_t y = 0; y < nHistoBinHeight; y++)
      {
        // Store the RGB triplet
        for(int32_t bin_width = 0; bin_width < HISTO_BIN_WIDTH; bin_width++)
        {
          nHistoX = (i * HISTO_BIN_WIDTH) + bin_width;
          //            nCoordY = ((2 - nHistChan) * HISTO_BIN_HEIGHT_MAX) + y;
          nCoordY = (rgbHistOffset - y) -1;
          m_pDibHistRgb->setPixelColor(nHistoX, nCoordY, QColor((nHistChan == CHAN_R) ? 255 : 0, (nHistChan == CHAN_G) ? 255 : 0, (nHistChan == CHAN_B) ? 255 : 0));
        }
      }
    }  // i: 0..HISTO_BINS-1

    m_bDibHistRgbReady = true;
  }

  // Only create the Y DC Histogram if requested
  m_bDibHistYReady = false;

  if(bDumpHistoY)
  {
    m_pDibHistY = new QImage(SUBSET_HISTO_BINS * HISTO_BIN_WIDTH, HISTO_BIN_HEIGHT_MAX, QImage::Format_RGB32);
    m_pDibHistY->fill(Qt::white);

    // Do peak detect first
    // Don't want to reset peak value to 0 as otherwise we might get
    // division by zero later when we calculate nHistoBinHeight
    nHistoPeakVal = 1;

    // TODO: Temporarily made quarter width - need to resample instead

    // Peak value
    for(uint32_t i = 0; i < SUBSET_HISTO_BINS; i++)
    {
      nHistoCurVal = m_anHistoYFull[i * 4 + 0];
      nHistoCurVal += m_anHistoYFull[i * 4 + 1];
      nHistoCurVal += m_anHistoYFull[i * 4 + 2];
      nHistoCurVal += m_anHistoYFull[i * 4 + 3];
      nHistoPeakVal = (nHistoCurVal > nHistoPeakVal) ? nHistoCurVal : nHistoPeakVal;
    }

    for(int32_t i = 0; i < SUBSET_HISTO_BINS; i++)
    {
      // Calculate bin's height (max HISTO_BIN_HEIGHT_MAX)
      nHistoCurVal = m_anHistoYFull[i * 4 + 0];
      nHistoCurVal += m_anHistoYFull[i * 4 + 1];
      nHistoCurVal += m_anHistoYFull[i * 4 + 2];
      nHistoCurVal += m_anHistoYFull[i * 4 + 3];
      nHistoBinHeight = HISTO_BIN_HEIGHT_MAX * nHistoCurVal / nHistoPeakVal;

      for(int32_t y = 0; y < nHistoBinHeight; y++)
      {
        // Store the RGB triplet
        for(int32_t bin_width = 0; bin_width < HISTO_BIN_WIDTH; bin_width++)
        {
          nHistoX = (i * HISTO_BIN_WIDTH) + bin_width;
          nCoordY = (HISTO_BIN_HEIGHT_MAX - y) - 1;
          m_pDibHistY->setPixelColor(nHistoX, nCoordY, Qt::gray);
        }
      }
    }  // i: 0..HISTO_BINS-1

    m_bDibHistYReady = true;
  }
}

// Reset the decoder Scan Buff (at start of scan and
// after any restart markers)
//
// INPUT:
// - nFilePos                                   = File position at start of scan
// - bRestart                                   = Is this a reset due to RSTn marker?
// PRE:
// - m_nRestartInterval
// POST:
// - m_bScanEnd
// - m_bScanBad
// - m_nScanBuff
// - m_nScanBuffPtr_first
// - m_nScanBuffPtr_start
// - m_nScanBuffPtr_align
// - m_anScanBuffPtr_pos[]
// - m_anScanBuffPtr_err[]
// - m_nScanBuffLatchErr
// - m_nScanBuffPtr_num
// - m_nScanBuff_vacant
// - m_nScanCurErr
// - m_bRestartRead
// - m_nRestartMcusLeft
//
void CimgDecode::DecodeRestartScanBuf(uint32_t nFilePos, bool bRestart)
{
  // Reset the state
  m_bScanEnd = false;
  m_bScanBad = false;
  m_nScanBuff = 0x00000000;
  m_nScanBuffPtr = nFilePos;
  
  if(!bRestart)
  {
    // Only reset the scan buffer pointer at the start of the file,
    // not after any RSTn markers. This is only used for the compression
    // ratio calculations.
    m_nScanBuffPtr_first = nFilePos;
  }

  m_nScanBuffPtr_start = nFilePos;
  m_nScanBuffPtr_align = 0;     // Start with byte alignment (0)
  m_anScanBuffPtr_pos[0] = 0;
  m_anScanBuffPtr_pos[1] = 0;
  m_anScanBuffPtr_pos[2] = 0;
  m_anScanBuffPtr_pos[3] = 0;
  m_anScanBuffPtr_err[0] = SCANBUF_OK;
  m_anScanBuffPtr_err[1] = SCANBUF_OK;
  m_anScanBuffPtr_err[2] = SCANBUF_OK;
  m_anScanBuffPtr_err[3] = SCANBUF_OK;
  m_nScanBuffLatchErr = SCANBUF_OK;

  m_nScanBuffPtr_num = 0;       // Empty m_nScanBuff
  m_nScanBuff_vacant = 32;

  m_nScanCurErr = false;

  //
  m_nScanBuffPtr = nFilePos;

  // Reset RST Interval checking
  m_bRestartRead = false;
  m_nRestartMcusLeft = m_nRestartInterval;
}

// Color conversion from YCC to RGB
//
// INPUT:
// - sPix                               = Structure for color conversion
// OUTPUT:
// - sPix                               = Structure for color conversion
//
void CimgDecode::ConvertYCCtoRGBFastFloat(PixelCc & sPix)
{
  int32_t nValY, nValCb, nValCr;

  double nValR, nValG, nValB;

  // Perform ranging to adjust from Huffman sums to reasonable range
  // -1024..+1024 -> -128..127
  sPix.nPreclipY = sPix.nPrerangeY >> 3;
  sPix.nPreclipCb = sPix.nPrerangeCb >> 3;
  sPix.nPreclipCr = sPix.nPrerangeCr >> 3;

  // Limit on YCC input
  // The y/cb/nPreclipCr values should already be 0..255 unless we have a
  // decode error where DC value gets out of range!
  //CapYccRange(nMcuX,nMcuY,sPix);
  nValY = (sPix.nPreclipY < -128) ? -128 : (sPix.nPreclipY > 127) ? 127 : sPix.nPreclipY;
  nValCb = (sPix.nPreclipCb < -128) ? -128 : (sPix.nPreclipCb > 127) ? 127 : sPix.nPreclipCb;
  nValCr = (sPix.nPreclipCr < -128) ? -128 : (sPix.nPreclipCr > 127) ? 127 : sPix.nPreclipCr;

  // Save the YCC values (0..255)
  sPix.nFinalY = static_cast < uint8_t > (nValY + 128);
  sPix.nFinalCb = static_cast < uint8_t > (nValCb + 128);
  sPix.nFinalCr = static_cast < uint8_t > (nValCr + 128);

  // Convert
  // Since the following seems to preserve the multiplies and subtractions
  // we could expand this out manually
  double fConstRed = 0.299f;
  double fConstGreen = 0.587f;
  double fConstBlue = 0.114f;

  // r = cr * 1.402 + y;
  // b = cb * 1.772 + y;
  // g = (y - 0.03409 * r) / 0.587;
  nValR = nValCr * (2 - 2 * fConstRed) + nValY;
  nValB = nValCb * (2 - 2 * fConstBlue) + nValY;
  nValG = (nValY - fConstBlue * nValB - fConstRed * nValR) / fConstGreen;

  // Level shift
  nValR += 128;
  nValB += 128;
  nValG += 128;

  // --------------- Finshed the color conversion

  // Limit
  //   r/g/nPreclipB -> r/g/b
  //CapRgbRange(nMcuX,nMcuY,sPix);
  sPix.nFinalR = (nValR < 0) ? 0 : (nValR > 255) ? 255 : static_cast<uint8_t>(nValR);
  sPix.nFinalG = (nValG < 0) ? 0 : (nValG > 255) ? 255 : static_cast<uint8_t>(nValG);
  sPix.nFinalB = (nValB < 0) ? 0 : (nValB > 255) ? 255 : static_cast<uint8_t>(nValB);
}

// Color conversion from YCC to RGB
//
// INPUT:
// - sPix                               = Structure for color conversion
// OUTPUT:
// - sPix                               = Structure for color conversion
//
void CimgDecode::ConvertYCCtoRGBFastFixed(PixelCc & sPix)
{
  int32_t nPreclipY, nPreclipCb, nPreclipCr;
  int32_t nValY, nValCb, nValCr;
  int32_t nValR, nValG, nValB;

  // Perform ranging to adjust from Huffman sums to reasonable range
  // -1024..+1024 -> -128..+127
  nPreclipY = sPix.nPrerangeY >> 3;
  nPreclipCb = sPix.nPrerangeCb >> 3;
  nPreclipCr = sPix.nPrerangeCr >> 3;

  // Limit on YCC input
  // The nPreclip* values should already be 0..255 unless we have a
  // decode error where DC value gets out of range!

  //CapYccRange(nMcuX,nMcuY,sPix);
  nValY = (nPreclipY < -128) ? -128 : (nPreclipY > 127) ? 127 : nPreclipY;
  nValCb = (nPreclipCb < -128) ? -128 : (nPreclipCb > 127) ? 127 : nPreclipCb;
  nValCr = (nPreclipCr < -128) ? -128 : (nPreclipCr > 127) ? 127 : nPreclipCr;

  // Save the YCC values (0..255)
  sPix.nFinalY = static_cast < uint8_t > (nValY + 128);
  sPix.nFinalCb = static_cast < uint8_t > (nValCb + 128);
  sPix.nFinalCr = static_cast < uint8_t > (nValCr + 128);

  // --------------

  // Convert
  // Fixed values is x 1024 (10 bits). Leaves 22 bits for integer
  //r2 = 1024*cr1*(2-2*fConstRed)+1024*y1;
  //b2 = 1024*cb1*(2-2*fConstBlue)+1024*y1;
  //g2 = 1024*(y1-fConstBlue*b2/1024-fConstRed*r2/1024)/fConstGreen;

  const int32_t CFIX_R = 306;
  const int32_t CFIX_G = 601;
  const int32_t CFIX_B = 116;
  const int32_t CFIX2_R = 1436;        // 2*(1024-cfix_red)
  const int32_t CFIX2_B = 1816;        // 2*(1024-cfix_blue)
  const int32_t CFIX2_G = 1048576;     // 1024*1024

  nValR = CFIX2_R * nValCr + 1024 * nValY;
  nValB = CFIX2_B * nValCb + 1024 * nValY;
  nValG = (CFIX2_G * nValY - CFIX_B * nValB - CFIX_R * nValR) / CFIX_G;

  nValR >>= 10;
  nValG >>= 10;
  nValB >>= 10;

  // Level shift
  nValR += 128;
  nValB += 128;
  nValG += 128;

  // Limit
  //   r/g/nPreclipB -> r/g/b
  sPix.nFinalR = (nValR < 0) ? 0 : (nValR > 255) ? 255 : static_cast < uint8_t > (nValR);
  sPix.nFinalG = (nValG < 0) ? 0 : (nValG > 255) ? 255 : static_cast < uint8_t > (nValG);
  sPix.nFinalB = (nValB < 0) ? 0 : (nValB > 255) ? 255 : static_cast < uint8_t > (nValB);
}

// Color conversion from YCC to RGB
// - CC: y/cb/cr -> r/g/b
//
// INPUT:
// - nMcuX                              = MCU x coordinate
// - nMcuY                              = MCU y coordinate
// - sPix                               = Structure for color conversion
// OUTPUT:
// - sPix                               = Structure for color conversion
// POST:
// - m_sHisto
// - m_anHistoYFull[]
// - m_anCcHisto_r[]
// - m_anCcHisto_g[]
// - m_anCcHisto_b[]
//
void CimgDecode::ConvertYCCtoRGB(uint32_t nMcuX, uint32_t nMcuY, PixelCc & sPix)
{  
  double fConstRed = 0.299;
  double fConstGreen = 0.587;
  double fConstBlue = 0.114;

  int32_t nByteY, nByteCb, nByteCr;
  int32_t nValY, nValCb, nValCr;

  double nValR, nValG, nValB;

  if(m_bHistEn)
  {
    // Calc stats on preranged YCC (direct from huffman DC sums)
    m_sHisto.nPreclipYMin = (sPix.nPrerangeY < m_sHisto.nPreclipYMin) ? sPix.nPrerangeY : m_sHisto.nPreclipYMin;
    m_sHisto.nPreclipYMax = (sPix.nPrerangeY > m_sHisto.nPreclipYMax) ? sPix.nPrerangeY : m_sHisto.nPreclipYMax;
    m_sHisto.nPreclipYSum += sPix.nPrerangeY;
    m_sHisto.nPreclipCbMin = (sPix.nPrerangeCb < m_sHisto.nPreclipCbMin) ? sPix.nPrerangeCb : m_sHisto.nPreclipCbMin;
    m_sHisto.nPreclipCbMax = (sPix.nPrerangeCb > m_sHisto.nPreclipCbMax) ? sPix.nPrerangeCb : m_sHisto.nPreclipCbMax;
    m_sHisto.nPreclipCbSum += sPix.nPrerangeCb;
    m_sHisto.nPreclipCrMin = (sPix.nPrerangeCr < m_sHisto.nPreclipCrMin) ? sPix.nPrerangeCr : m_sHisto.nPreclipCrMin;
    m_sHisto.nPreclipCrMax = (sPix.nPrerangeCr > m_sHisto.nPreclipCrMax) ? sPix.nPrerangeCr : m_sHisto.nPreclipCrMax;
    m_sHisto.nPreclipCrSum += sPix.nPrerangeCr;
  }

  if(m_bHistEn)
  {
    // Now generate the Y histogram, if requested
    // Add the Y value to the full histogram (for image similarity calcs)
    //if (bDumpHistoY) {
    int32_t histo_index = sPix.nPrerangeY;

    if(histo_index < -1024)
      histo_index = -1024;

    if(histo_index > 1023)
      histo_index = 1023;

    histo_index += 1024;
    m_anHistoYFull[histo_index]++;
    //}
  }

  // Perform ranging to adjust from Huffman sums to reasonable range
  // -1024..+1024 -> 0..255
  // Add 1024 then / 8
  sPix.nPreclipY = (sPix.nPrerangeY + 1024) / 8;
  sPix.nPreclipCb = (sPix.nPrerangeCb + 1024) / 8;
  sPix.nPreclipCr = (sPix.nPrerangeCr + 1024) / 8;

  // Limit on YCC input
  // The y/cb/nPreclipCr values should already be 0..255 unless we have a
  // decode error where DC value gets out of range!
  CapYccRange(nMcuX, nMcuY, sPix);

  // --------------- Perform the color conversion
  nByteY = sPix.nFinalY;
  nByteCb = sPix.nFinalCb;
  nByteCr = sPix.nFinalCr;

  // Level shift
  nValY = nByteY - 128;
  nValCb = nByteCb - 128;
  nValCr = nByteCr - 128;

  // Convert
  nValR = nValCr * (2.0 - 2.0 * fConstRed) + nValY;
  nValB = nValCb * (2.0 - 2.0 * fConstBlue) + nValY;
  nValG = (nValY - fConstBlue * nValB - fConstRed * nValR) / fConstGreen;

  // Level shift
  nValR += 128;
  nValB += 128;
  nValG += 128;

  sPix.nPreclipR = nValR;
  sPix.nPreclipG = nValG;
  sPix.nPreclipB = nValB;
  // --------------- Finshed the color conversion

  // Limit
  // - Preclip RGB to Final RGB
  CapRgbRange(nMcuX, nMcuY, sPix);

  // Display
  /*
     strTmp = QString("* (YCC->RGB) @ (%03u,%03u): YCC=(%4d,%4d,%4d) RGB=(%03u,%03u,%u)",
     nMcuX,nMcuY,y,cb,cr,r_limb,g_limb,b_limb);
     m_pLog->AddLine(strTmp);
   */

  if(m_bHistEn)
  {
    // Bin the result into a histogram!
    //   value = 0..255
    //   bin   = 0..7, 8..15, ..., 248..255
    // Channel: Red
    uint32_t bin_divider = 256 / HISTO_BINS;

    m_anCcHisto_r[sPix.nFinalR / bin_divider]++;
    m_anCcHisto_g[sPix.nFinalG / bin_divider]++;
    m_anCcHisto_b[sPix.nFinalB / bin_divider]++;
  }
}

// Color conversion clipping
// - Process the pre-clipped YCC values and ensure they
//   have been clipped into the valid region
//
// INPUT:
// - nMcuX                              = MCU x coordinate
// - nMcuY                              = MCU y coordinate
// - sPix                               = Structure for color conversion
// OUTPUT:
// - sPix                               = Structure for color conversion
// POST:
// - m_sHisto
//
void CimgDecode::CapYccRange(uint32_t nMcuX, uint32_t nMcuY, PixelCc & sPix)
{
  // Check the bounds on the YCC value. It should probably be 0..255 unless our DC
  // values got really messed up in a corrupt file. Perhaps it might be best to reset it to 0? Otherwise
  // it will continuously report an out-of-range value.
  int32_t nCurY, nCurCb, nCurCr;

  nCurY = sPix.nPreclipY;
  nCurCb = sPix.nPreclipCb;
  nCurCr = sPix.nPreclipCr;

  if(m_bHistEn)
  {
    m_sHisto.nClipYMin = (nCurY < m_sHisto.nClipYMin) ? nCurY : m_sHisto.nClipYMin;
    m_sHisto.nClipYMax = (nCurY > m_sHisto.nClipYMax) ? nCurY : m_sHisto.nClipYMax;
    m_sHisto.nClipYSum += nCurY;
    m_sHisto.nClipCbMin = (nCurCb < m_sHisto.nClipCbMin) ? nCurCb : m_sHisto.nClipCbMin;
    m_sHisto.nClipCbMax = (nCurCb > m_sHisto.nClipCbMax) ? nCurCb : m_sHisto.nClipCbMax;
    m_sHisto.nClipCbSum += nCurCb;
    m_sHisto.nClipCrMin = (nCurCr < m_sHisto.nClipCrMin) ? nCurCr : m_sHisto.nClipCrMin;
    m_sHisto.nClipCrMax = (nCurCr > m_sHisto.nClipCrMax) ? nCurCr : m_sHisto.nClipCrMax;
    m_sHisto.nClipCrSum += nCurCr;
    m_sHisto.nCount++;
  }

  if(CC_CLIP_YCC_EN)
  {
    if(nCurY > CC_CLIP_YCC_MAX)
    {
      if(YCC_CLIP_REPORT_ERR && (m_nWarnYccClipNum < YCC_CLIP_REPORT_MAX))
      {
        QString strTmp;

        strTmp = QString("*** NOTE: YCC Clipped. MCU=(%1,%2) YCC=(%3,%4,%5) Y Overflow @ Offset %6")
            .arg(nMcuX, 4)
            .arg(nMcuY, 4)
            .arg(nCurY, 5)
            .arg(nCurCb, 5)
            .arg(nCurCr, 5)
            .arg(GetScanBufPos());
        m_pLog->AddLineWarn(strTmp);
        m_nWarnYccClipNum++;
        m_sStatClip.nClipYOver++;

        if(m_nWarnYccClipNum == YCC_CLIP_REPORT_MAX)
        {
          strTmp = QString("    Only reported first %1 instances of this message...").arg(YCC_CLIP_REPORT_MAX);
          m_pLog->AddLineWarn(strTmp);
        }
      }

      sPix.nClip |= CC_CLIP_Y_OVER;
      nCurY = CC_CLIP_YCC_MAX;
    }

    if(nCurY < CC_CLIP_YCC_MIN)
    {
      if(YCC_CLIP_REPORT_ERR && (m_nWarnYccClipNum < YCC_CLIP_REPORT_MAX))
      {
        QString strTmp;

        strTmp = QString("*** NOTE: YCC Clipped. MCU=(%1,%2) YCC=(%3,%4,%5) Y Underflow @ Offset %6")
            .arg(nMcuX, 4)
            .arg(nMcuY, 4)
            .arg(nCurY, 5)
            .arg(nCurCb, 5)
            .arg(nCurCr, 5)
            .arg(GetScanBufPos());
        m_pLog->AddLineWarn(strTmp);
        m_nWarnYccClipNum++;
        m_sStatClip.nClipYUnder++;

        if(m_nWarnYccClipNum == YCC_CLIP_REPORT_MAX)
        {
          strTmp = QString("    Only reported first %1 instances of this message...").arg(YCC_CLIP_REPORT_MAX);
          m_pLog->AddLineWarn(strTmp);
        }
      }
      
      sPix.nClip |= CC_CLIP_Y_UNDER;
      nCurY = CC_CLIP_YCC_MIN;
    }

    if(nCurCb > CC_CLIP_YCC_MAX)
    {
      if(YCC_CLIP_REPORT_ERR && (m_nWarnYccClipNum < YCC_CLIP_REPORT_MAX))
      {
        QString strTmp;

        strTmp = QString("*** NOTE: YCC Clipped.  MCU=(%1,%2) YCC=(%3,%4,%5) Cb Overflow @ Offset %6")
            .arg(nMcuX, 4)
            .arg(nMcuY, 4)
            .arg(nCurY, 5)
            .arg(nCurCb, 5)
            .arg(nCurCr, 5)
            .arg(GetScanBufPos());
        m_pLog->AddLineWarn(strTmp);
        m_nWarnYccClipNum++;
        m_sStatClip.nClipCbOver++;

        if(m_nWarnYccClipNum == YCC_CLIP_REPORT_MAX)
        {
          strTmp = QString("    Only reported first %1 instances of this message...").arg(YCC_CLIP_REPORT_MAX);
          m_pLog->AddLineWarn(strTmp);
        }
      }

      sPix.nClip |= CC_CLIP_CB_OVER;
      nCurCb = CC_CLIP_YCC_MAX;
    }

    if(nCurCb < CC_CLIP_YCC_MIN)
    {
      if(YCC_CLIP_REPORT_ERR && (m_nWarnYccClipNum < YCC_CLIP_REPORT_MAX))
      {
        QString strTmp;

        strTmp = QString("*** NOTE: YCC Clipped.  MCU=(%1,%2) YCC=(%3,%4,%5) Cb Underflow @ Offset %6")
            .arg(nMcuX, 4)
            .arg(nMcuY, 4)
            .arg(nCurY, 5)
            .arg(nCurCb, 5)
            .arg(nCurCr, 5)
            .arg(GetScanBufPos());
        m_pLog->AddLineWarn(strTmp);
        m_nWarnYccClipNum++;
        m_sStatClip.nClipCbUnder++;

        if(m_nWarnYccClipNum == YCC_CLIP_REPORT_MAX)
        {
          strTmp = QString("    Only reported first %1 instances of this message...").arg(YCC_CLIP_REPORT_MAX);
          m_pLog->AddLineWarn(strTmp);
        }
      }

      sPix.nClip |= CC_CLIP_CB_UNDER;
      nCurCb = CC_CLIP_YCC_MIN;
    }

    if(nCurCr > CC_CLIP_YCC_MAX)
    {
      if(YCC_CLIP_REPORT_ERR && (m_nWarnYccClipNum < YCC_CLIP_REPORT_MAX))
      {
        QString strTmp;

        strTmp = QString("*** NOTE: YCC Clipped.  MCU=(%1,%2) YCC=(%3,%4,%5) Cr Overflow @ Offset %6")
            .arg(nMcuX, 4)
            .arg(nMcuY, 4)
            .arg(nCurY, 5)
            .arg(nCurCb, 5)
            .arg(nCurCr, 5)
            .arg(GetScanBufPos());
        m_pLog->AddLineWarn(strTmp);
        m_nWarnYccClipNum++;
        m_sStatClip.nClipCrOver++;

        if(m_nWarnYccClipNum == YCC_CLIP_REPORT_MAX)
        {
          strTmp = QString("    Only reported first %1 instances of this message...").arg(YCC_CLIP_REPORT_MAX);
          m_pLog->AddLineWarn(strTmp);
        }
      }

      sPix.nClip |= CC_CLIP_CR_OVER;
      nCurCr = CC_CLIP_YCC_MAX;
    }

    if(nCurCr < CC_CLIP_YCC_MIN)
    {
      if(YCC_CLIP_REPORT_ERR && (m_nWarnYccClipNum < YCC_CLIP_REPORT_MAX))
      {
        QString strTmp;

        strTmp = QString("*** NOTE: YCC Clipped.  MCU=(%1,%2) YCC=(%3,%4,%5) Cr Underflow @ Offset %6")
            .arg(nMcuX, 4)
            .arg(nMcuY, 4)
            .arg(nCurY, 5)
            .arg(nCurCb, 5)
            .arg(nCurCr, 5)
            .arg(GetScanBufPos());
        m_pLog->AddLineWarn(strTmp);
        m_nWarnYccClipNum++;
        m_sStatClip.nClipCrUnder++;

        if(m_nWarnYccClipNum == YCC_CLIP_REPORT_MAX)
        {
          strTmp = QString("    Only reported first %1 instances of this message...").arg(YCC_CLIP_REPORT_MAX);
          m_pLog->AddLineWarn(strTmp);
        }
      }

      sPix.nClip |= CC_CLIP_CR_UNDER;
      nCurCr = CC_CLIP_YCC_MIN;
    }
  }                             // YCC clip enabled?

  // Perform color conversion: YCC->RGB
  // The nCurY/cb/cr values should already be clipped to byte size
  sPix.nFinalY = static_cast < uint8_t > (nCurY);
  sPix.nFinalCb = static_cast < uint8_t > (nCurCb);
  sPix.nFinalCr = static_cast < uint8_t > (nCurCr);
}

// Color conversion clipping (RGB)
// - Input RGB triplet in floats
// - Expect range to be 0..255
// - Return RGB triplet in byteuis
// - Report if it is out of range
// - Converts from Preclip RGB to Final RGB
//
// INPUT:
// - nMcuX                              = MCU x coordinate
// - nMcuY                              = MCU y coordinate
// - sPix                               = Structure for color conversion
// OUTPUT:
// - sPix                               = Structure for color conversion
// POST:
// - m_sHisto
//
void CimgDecode::CapRgbRange(uint32_t nMcuX, uint32_t nMcuY, PixelCc & sPix)
{
  int32_t nLimitR, nLimitG, nLimitB;

  // Truncate
  nLimitR = static_cast<int32_t>(sPix.nPreclipR);
  nLimitG = static_cast<int32_t>(sPix.nPreclipG);
  nLimitB = static_cast<int32_t>(sPix.nPreclipB);

  if(m_bHistEn)
  {
    m_sHisto.nPreclipRMin = (nLimitR < m_sHisto.nPreclipRMin) ? nLimitR : m_sHisto.nPreclipRMin;
    m_sHisto.nPreclipRMax = (nLimitR > m_sHisto.nPreclipRMax) ? nLimitR : m_sHisto.nPreclipRMax;
    m_sHisto.nPreclipRSum += nLimitR;
    m_sHisto.nPreclipGMin = (nLimitG < m_sHisto.nPreclipGMin) ? nLimitG : m_sHisto.nPreclipGMin;
    m_sHisto.nPreclipGMax = (nLimitG > m_sHisto.nPreclipGMax) ? nLimitG : m_sHisto.nPreclipGMax;
    m_sHisto.nPreclipGSum += nLimitG;
    m_sHisto.nPreclipBMin = (nLimitB < m_sHisto.nPreclipBMin) ? nLimitB : m_sHisto.nPreclipBMin;
    m_sHisto.nPreclipBMax = (nLimitB > m_sHisto.nPreclipBMax) ? nLimitB : m_sHisto.nPreclipBMax;
    m_sHisto.nPreclipBSum += nLimitB;
  }

  if(nLimitR < 0)
  {
    if(m_bVerbose)
    {
      QString strTmp;

      strTmp = QString("  YCC->RGB Clipped. MCU=(%1,%2) RGB=(%3,%4,%5) Red Underflow")
          .arg(nMcuX, 4)
          .arg(nMcuY, 4)
          .arg(nLimitR, 5)
          .arg(nLimitG, 5)
          .arg(nLimitB, 5);
      m_pLog->AddLineWarn(strTmp);
    }

    sPix.nClip |= CC_CLIP_R_UNDER;
    m_sStatClip.nClipRUnder++;
    nLimitR = 0;
  }

  if(nLimitG < 0)
  {
    if(m_bVerbose)
    {
      QString strTmp;

      strTmp = QString("  YCC->RGB Clipped. MCU=(%1,%2) RGB=(%3,%4,%5) Green Underflow")
          .arg(nMcuX, 4)
          .arg(nMcuY, 4)
          .arg(nLimitR, 5)
          .arg(nLimitG, 5)
          .arg(nLimitB, 5);
      m_pLog->AddLineWarn(strTmp);
    }

    sPix.nClip |= CC_CLIP_G_UNDER;
    m_sStatClip.nClipGUnder++;
    nLimitG = 0;
  }

  if(nLimitB < 0)
  {
    if(m_bVerbose)
    {
      QString strTmp;

      strTmp = QString("  YCC->RGB Clipped. MCU=(%1,%2) RGB=(%3,%4,%5) Blue Underflow")
          .arg(nMcuX, 4)
          .arg(nMcuY, 4)
          .arg(nLimitR, 5)
          .arg(nLimitG, 5)
          .arg(nLimitB, 5);
      m_pLog->AddLineWarn(strTmp);
    }

    sPix.nClip |= CC_CLIP_B_UNDER;
    m_sStatClip.nClipBUnder++;
    nLimitB = 0;
  }

  if(nLimitR > 255)
  {
    if(m_bVerbose)
    {
      QString strTmp;

      strTmp = QString("  YCC->RGB Clipped. MCU=(%1,%2) RGB=(%3,%4,%5) Red Overflow")
          .arg(nMcuX, 4)
          .arg(nMcuY, 4)
          .arg(nLimitR, 5)
          .arg(nLimitG, 5)
          .arg(nLimitB, 5);
      m_pLog->AddLineWarn(strTmp);
    }

    sPix.nClip |= CC_CLIP_R_OVER;
    m_sStatClip.nClipROver++;
    nLimitR = 255;
  }

  if(nLimitG > 255)
  {
    if(m_bVerbose)
    {
      QString strTmp;

      strTmp = QString("  YCC->RGB Clipped. MCU=(%1,%2) RGB=(%3,%4,%5) Green Overflow")
          .arg(nMcuX, 4)
          .arg(nMcuY, 4)
          .arg(nLimitR, 5)
          .arg(nLimitG, 5)
          .arg(nLimitB, 5);
      m_pLog->AddLineWarn(strTmp);
    }

    sPix.nClip |= CC_CLIP_G_OVER;
    m_sStatClip.nClipGOver++;
    nLimitG = 255;
  }

  if(nLimitB > 255)
  {
    if(m_bVerbose)
    {
      QString strTmp;

      strTmp = QString("  YCC->RGB Clipped. MCU=(%1,%2) RGB=(%3,%4,%5) Blue Overflow")
          .arg(nMcuX, 4)
          .arg(nMcuY, 4)
          .arg(nLimitR, 5)
          .arg(nLimitG, 5)
          .arg(nLimitB, 5);
      m_pLog->AddLineWarn(strTmp);
    }

    sPix.nClip |= CC_CLIP_B_OVER;
    m_sStatClip.nClipBOver++;
    nLimitB = 255;
  }

  if(m_bHistEn)
  {
    m_sHisto.nClipRMin = (nLimitR < m_sHisto.nClipRMin) ? nLimitR : m_sHisto.nClipRMin;
    m_sHisto.nClipRMax = (nLimitR > m_sHisto.nClipRMax) ? nLimitR : m_sHisto.nClipRMax;
    m_sHisto.nClipRSum += nLimitR;
    m_sHisto.nClipGMin = (nLimitG < m_sHisto.nClipGMin) ? nLimitG : m_sHisto.nClipGMin;
    m_sHisto.nClipGMax = (nLimitG > m_sHisto.nClipGMax) ? nLimitG : m_sHisto.nClipGMax;
    m_sHisto.nClipGSum += nLimitG;
    m_sHisto.nClipBMin = (nLimitB < m_sHisto.nClipBMin) ? nLimitB : m_sHisto.nClipBMin;
    m_sHisto.nClipBMax = (nLimitB > m_sHisto.nClipBMax) ? nLimitB : m_sHisto.nClipBMax;
    m_sHisto.nClipBSum += nLimitB;
  }

  // Now convert to byteui
  sPix.nFinalR = static_cast<uint8_t>(nLimitR);
  sPix.nFinalG = static_cast<uint8_t>(nLimitG);
  sPix.nFinalB = static_cast<uint8_t>(nLimitB);
}

// Recalcs the full image based on the original YCC pixmap
// - Also locate the brightest pixel.
// - Note that we cannot do the brightest pixel search when we called SetFullRes()
//   because we need to have access to all of the channel components at once to do this.
//
// INPUT:
// - pRectView                          = UNUSED. Intended to limit updates to visible region
//                            (Range of real image that is visibile / cropped)
// PRE:
// - m_pPixValY[]
// - m_pPixValCb[]
// - m_pPixValCr[]
// OUTPUT:
// - pTmp                                       = RGB pixel map (32-bit per pixel, [0x00,R,G,B])
//
void CimgDecode::CalcChannelPreviewFull(QRect *, uint8_t *pTmp)
{
  PixelCc sPixSrc, sPixDst;

  QString strTmp;

  uint32_t nRowBytes;

  nRowBytes = m_nImgSizeX * sizeof(QRgb);

  // Color conversion process

  uint32_t nPixMapW = m_nBlkXMax * BLK_SZ_X;
  uint32_t nPixmapInd;
  uint32_t nRngX1, nRngX2, nRngY1, nRngY2;
  uint32_t nSumY;
  uint32_t nNumPixels = 0;

  // TODO: Update ranges to take into account the visible view region
  // The approach might include:
  //   nRngX1 = pRectView->left;
  //   nRngX2 = pRectView->right;
  //   nRngY1 = pRectView->top;
  //   nRngY2 = pRectView->bottom;
  //
  // These co-ords will define range in nPixX,nPixY that get drawn
  // This will help YCC Adjust display react much faster. Only recalc
  // visible portion of image, but then recalc entire one once Adjust
  // dialog is closed.
  //
  // NOTE: if we were to make these ranges a subset of the
  // full image dimensions, then we'd have to determine the best
  // way to handle the brightest pixel search & average luminance logic
  // since those appear in the nRngX/Y loops.

  nRngX1 = 0;
  nRngX2 = m_nImgSizeX;
  nRngY1 = 0;
  nRngY2 = m_nImgSizeY;

  // Brightest pixel values were already reset during Reset() call, but for
  // safety, do it again here.
  m_bBrightValid = false;
  m_nBrightY = -32768;
  m_nBrightCb = -32768;
  m_nBrightCr = -32768;

  // Average luminance calculation
  m_bAvgYValid = false;
  m_nAvgY = 0;
  nSumY = 0;

  // For IDCT RGB Printout:
  bool bRowStart = false;

  QString strLine;

  uint32_t nMcuShiftInd = m_nPreviewShiftMcuY * (m_nImgSizeX / m_nMcuWidth) + m_nPreviewShiftMcuX;

  SetStatusText("Color conversion...");

  if(m_bDetailVlc)
  {
    m_pLog->AddLine("  Detailed IDCT Dump (RGB):");
    strTmp = QString("    MCU [%1,%2]:")
        .arg(m_nDetailVlcX, 3)
        .arg(m_nDetailVlcY, 3);
    m_pLog->AddLine(strTmp);
  }

  // Determine pixel count
  nNumPixels = (nRngY2 - nRngY1 + 1) * (nRngX2 - nRngX1 + 1);

  m_pDibTemp = new QImage(m_nImgSizeX, m_nImgSizeY, QImage::Format_RGB32);

  // Step through the image
  for(int32_t nPixY = nRngY1; nPixY < nRngY2; nPixY++)
  {
    uint32_t nMcuY = nPixY / m_nMcuHeight;

    // DIBs appear to be stored up-side down, so correct Y
    uint32_t nCoordYInv = (m_nImgSizeY - 1) - nPixY;

    for(int32_t nPixX = nRngX1; nPixX < nRngX2; nPixX++)
    {
      nPixmapInd = nPixY * nPixMapW + nPixX;
      uint32_t nPixByte = nPixX * 4 + 0 + nCoordYInv * nRowBytes;
      
      uint32_t nMcuX = nPixX / m_nMcuWidth;
      uint32_t nMcuInd = nMcuY * (m_nImgSizeX / m_nMcuWidth) + nMcuX;

      int32_t nTmpY, nTmpCb, nTmpCr;

      nTmpY = m_pPixValY[nPixmapInd];

      if(m_nNumSosComps == NUM_CHAN_YCC)
      {
        nTmpCb = m_pPixValCb[nPixmapInd];
        nTmpCr = m_pPixValCr[nPixmapInd];
      }
      else
      {
        nTmpCb = 0;
        nTmpCr = 0;
      }

      // Load the YCC value
      sPixSrc.nPrerangeY = nTmpY;
      sPixSrc.nPrerangeCb = nTmpCb;
      sPixSrc.nPrerangeCr = nTmpCr;

      // Update brightest pixel search here
      int32_t nBrightness = nTmpY;

      if(nBrightness > m_nBrightY)
      {
        m_nBrightY = nTmpY;
        m_nBrightCb = nTmpCb;
        m_nBrightCr = nTmpCr;
        m_ptBrightMcu.setX(nMcuX);
        m_ptBrightMcu.setY(nMcuY);
      }

      // FIXME
      // Could speed this up by adding boolean check to see if we are truly needing to do any shifting!
      if(nMcuInd >= nMcuShiftInd)
      {
        sPixSrc.nPrerangeY += m_nPreviewShiftY;
        sPixSrc.nPrerangeCb += m_nPreviewShiftCb;
        sPixSrc.nPrerangeCr += m_nPreviewShiftCr;
      }

      // Invoke the appropriate color conversion routine
      if(m_bHistEn || m_bStatClipEn)
      {
        ConvertYCCtoRGB(nMcuX, nMcuY, sPixSrc);
      }
      else
      {
        ConvertYCCtoRGBFastFloat(sPixSrc);
        //ConvertYCCtoRGBFastFixed(sPixSrc);
      }

      // Accumulate the luminance value for this pixel
      // after we have converted it to the range 0..255
      nSumY += sPixSrc.nFinalY;

      // If we want a detailed decode of RGB, print it out
      // now if we are on the correct MCU.
      // NOTE: The level shift (adjust) will affect this report!
      if(m_bDetailVlc)
      {
        if(nMcuY == m_nDetailVlcY)
        {
          if(nMcuX == m_nDetailVlcX)
          {
            //if ((nMcuX >= m_nDetailVlcX) && (nMcuX < m_nDetailVlcX+m_nDetailVlcLen)) {
            if(!bRowStart)
            {
              bRowStart = true;
              strLine = QString("      [ ");
            }

            strTmp = QString("x%1%2%3 ")
                .arg(sPixSrc.nFinalR, 2, 16, QChar('0'))
                .arg(sPixSrc.nFinalG, 2, 16, QChar('0'))
                .arg(sPixSrc.nFinalB, 2, 16, QChar('0'));
            strLine.append(strTmp);
          }
          else
          {
            if(bRowStart)
            {
              // We had started a row, but we are now out of range, so we
              // need to close up!
              strTmp = QString(" ]");
              strLine.append(strTmp);
              m_pLog->AddLine(strLine);
              bRowStart = false;
            }
          }
        }
      }

      // Perform any channel filtering if enabled
      ChannelExtract(m_nPreviewMode, sPixSrc, sPixDst);

      // Assign the RGB pixel map
      m_pDibTemp->setPixelColor(nPixX, nPixY, QColor(sPixDst.nFinalR, sPixDst.nFinalG, sPixDst.nFinalB));
    }                           // x
  }                             // y

  SetStatusText("");
  // ---------------------------------------------------------

  if(m_bDetailVlc)
  {
    m_pLog->AddLine("");
  }

  // Assume that brightest pixel search was successful
  // Now compute the RGB value for this pixel!
  m_bBrightValid = true;
  sPixSrc.nPrerangeY = m_nBrightY;
  sPixSrc.nPrerangeCb = m_nBrightCb;
  sPixSrc.nPrerangeCr = m_nBrightCr;
  ConvertYCCtoRGBFastFloat(sPixSrc);
  m_nBrightR = sPixSrc.nFinalR;
  m_nBrightG = sPixSrc.nFinalG;
  m_nBrightB = sPixSrc.nFinalB;

  // Now perform average luminance calculation
  // NOTE: This will result in a value in the range 0..255
  Q_ASSERT(nNumPixels > 0);

  // Avoid divide by zero
  if(nNumPixels == 0)
  {
    nNumPixels = 1;
  }

  m_nAvgY = nSumY / nNumPixels;
  m_bAvgYValid = true;
}

// Extract the specified channel
//
// INPUT:
// - nMode                                      = Channel(s) to extract from
// - sSrc                                       = Color representations (YCC & RGB) for pixel
// OUTPUT:
// - sDst                                       = Resulting RGB output after filtering
//
void CimgDecode::ChannelExtract(uint32_t nMode, PixelCc & sSrc, PixelCc & sDst)
{
  if(nMode == PREVIEW_RGB)
  {
    sDst.nFinalR = sSrc.nFinalR;
    sDst.nFinalG = sSrc.nFinalG;
    sDst.nFinalB = sSrc.nFinalB;
  }
  else if(nMode == PREVIEW_YCC)
  {
    sDst.nFinalR = sSrc.nFinalCr;
    sDst.nFinalG = sSrc.nFinalY;
    sDst.nFinalB = sSrc.nFinalCb;
  }
  else if(nMode == PREVIEW_R)
  {
    sDst.nFinalR = sSrc.nFinalR;
    sDst.nFinalG = sSrc.nFinalR;
    sDst.nFinalB = sSrc.nFinalR;
  }
  else if(nMode == PREVIEW_G)
  {
    sDst.nFinalR = sSrc.nFinalG;
    sDst.nFinalG = sSrc.nFinalG;
    sDst.nFinalB = sSrc.nFinalG;
  }
  else if(nMode == PREVIEW_B)
  {
    sDst.nFinalR = sSrc.nFinalB;
    sDst.nFinalG = sSrc.nFinalB;
    sDst.nFinalB = sSrc.nFinalB;
  }
  else if(nMode == PREVIEW_Y)
  {
    sDst.nFinalR = sSrc.nFinalY;
    sDst.nFinalG = sSrc.nFinalY;
    sDst.nFinalB = sSrc.nFinalY;
  }
  else if(nMode == PREVIEW_CB)
  {
    sDst.nFinalR = sSrc.nFinalCb;
    sDst.nFinalG = sSrc.nFinalCb;
    sDst.nFinalB = sSrc.nFinalCb;
  }
  else if(nMode == PREVIEW_CR)
  {
    sDst.nFinalR = sSrc.nFinalCr;
    sDst.nFinalG = sSrc.nFinalCr;
    sDst.nFinalB = sSrc.nFinalCr;
  }
  else
  {
    sDst.nFinalR = sSrc.nFinalR;
    sDst.nFinalG = sSrc.nFinalG;
    sDst.nFinalB = sSrc.nFinalB;
  }
}

// Fetch the detailed decode settings (VLC)
//
// OUTPUT:
// - bDetail                    = Enable for detailed scan VLC reporting
// - nX                                 = Start of detailed scan decode MCU X coordinate
// - nY                                 = Start of detailed scan decode MCU Y coordinate
// - nLen                               = Number of MCUs to parse in detailed scan decode
//
void CimgDecode::GetDetailVlc(bool & bDetail, uint32_t &nX, uint32_t &nY, uint32_t &nLen)
{
  bDetail = m_bDetailVlc;
  nX = m_nDetailVlcX;
  nY = m_nDetailVlcY;
  nLen = m_nDetailVlcLen;
}

// Set the detailed scan decode settings (VLC)
//
// INPUT:
// - bDetail                    = Enable for detailed scan VLC reporting
// - nX                                 = Start of detailed scan decode MCU X coordinate
// - nY                                 = Start of detailed scan decode MCU Y coordinate
// - nLen                               = Number of MCUs to parse in detailed scan decode
//
void CimgDecode::SetDetailVlc(bool bDetail, uint32_t nX, uint32_t nY, uint32_t nLen)
{
  m_bDetailVlc = bDetail;
  m_nDetailVlcX = nX;
  m_nDetailVlcY = nY;
  m_nDetailVlcLen = nLen;
}

// Fetch the pointers for the pixel map
//
// OUTPUT:
// - pMayY                              = Pointer to pixel map for Y component
// - pMapCb                             = Pointer to pixel map for Cb component
// - pMapCr                             = Pointer to pixel map for Cr component
//
void CimgDecode::GetPixMapPtrs(int16_t *&pMapY, int16_t *&pMapCb, int16_t *&pMapCr)
{
  Q_ASSERT(m_pPixValY);
  Q_ASSERT(m_pPixValCb);
  Q_ASSERT(m_pPixValCr);
  pMapY = m_pPixValY;
  pMapCb = m_pPixValCb;
  pMapCr = m_pPixValCr;
}

// Get image pixel dimensions rounded up to nearest MCU
//
// OUTPUT:
// - nX                                 = X dimension of preview image
// - nY                                 = Y dimension of preview image
//
void CimgDecode::GetImageSize(uint32_t &nX, uint32_t &nY)
{
  nX = m_nImgSizeX;
  nY = m_nImgSizeY;
}

// Get the bitmap pointer
//
// OUTPUT:
// - pBitmap                    = Bitmap (DIB) of preview
//
void CimgDecode::GetBitmapPtr(uint8_t *&pBitmap)
{
  uint8_t *pDibImgTmpBits = 0;

//@@  pDibImgTmpBits = (uint8_t *) (m_pDibTemp.GetDIBBitArray());

  // Ensure that the pointers are available!
  if(!pDibImgTmpBits)
  {
    pBitmap = 0;
  }
  else
  {
    pBitmap = pDibImgTmpBits;
  }
}

// Calculate RGB pixel map from selected channels of YCC pixel map
//
// PRE:
// - m_pPixValY
// - m_pPixValCb
// - m_pPixValCr
// POST:
// - m_pDibTemp
// NOTE:
// - Channels are selected in CalcChannelPreviewFull()
//
void CimgDecode::CalcChannelPreview()
{
  uint8_t *pDibImgTmpBits = 0;

//@@  pDibImgTmpBits = (uint8_t *) (m_pDibTemp.GetDIBBitArray());

  // Ensure that the pointers are available!
//  if(!pDibImgTmpBits)
//  {
//    return;
//  }

  // If we need to do a YCC shift, then do full recalc into tmp array
  CalcChannelPreviewFull(0, pDibImgTmpBits);

  // Since this was a complex mod, we don't mark this channel as
  // being "done", so we will need to recalculate any time we change
  // the channel display.

  // Force an update of the view to be sure
  //m_pDoc->UpdateAllViews(0);

  return;
}

// Determine the file position from a pixel coordinate
//
// INPUT:
// - nPixX                                      = Pixel coordinate (x)
// - nPixY                                      = Pixel coordinate (y)
// OUTPUT:
// - nByte                                      = File offset (byte)
// - nBit                                       = File offset (bit)
//
void CimgDecode::LookupFilePosPix(QPoint p, uint32_t &nByte, uint32_t &nBit)
{
  uint32_t nMcuX, nMcuY;
  uint32_t nPacked;

  nMcuX = p.x() / m_nMcuWidth;
  nMcuY = p.y() / m_nMcuHeight;
  nPacked = m_pMcuFileMap[nMcuX + nMcuY * m_nMcuXMax];
  UnpackFileOffset(nPacked, nByte, nBit);
}

// Determine the file position from a MCU coordinate
//
// INPUT:
// - nMcuX                                      = MCU coordinate (x)
// - nMcuY                                      = MCU coordinate (y)
// OUTPUT:
// - nByte                                    = File offset (byte)
// - nBit                                       = File offset (bit)
//
void CimgDecode::LookupFilePosMcu(QPoint p, uint32_t &nByte, uint32_t &nBit)
{
  uint32_t nPacked;

  nPacked = m_pMcuFileMap[p.x() + p.y() * m_nMcuXMax];
  UnpackFileOffset(nPacked, nByte, nBit);
}

// Determine the YCC DC value of a specified block
//
// INPUT:
// - nBlkX                                      = 8x8 block coordinate (x)
// - nBlkY                                      = 8x8 block coordinate (y)
// OUTPUT:
// - nY                                         = Y channel value
// - nCb                                        = Cb channel value
// - nCr                                        = Cr channel value
//
void CimgDecode::LookupBlkYCC(QPoint p, int32_t &nY, int32_t &nCb, int32_t &nCr)
{
  nY = m_pBlkDcValY[p.x() + p.y() * m_nBlkXMax];

  if(m_nNumSosComps == NUM_CHAN_YCC)
  {
    nCb = m_pBlkDcValCb[p.x() + p.y() * m_nBlkXMax];
    nCr = m_pBlkDcValCr[p.x() + p.y() * m_nBlkXMax];
  }
  else
  {
    nCb = 0;                    // FIXME
    nCr = 0;                    // FIXME
  }
}

// Convert pixel coordinate to MCU coordinate
//
// INPUT:
// - ptPix                                      = Pixel coordinate
// RETURN:
// - MCU coordinate
//
QPoint CimgDecode::PixelToMcu(QPoint ptPix)
{
  QPoint ptMcu;

  ptMcu.setX(ptPix.x() / m_nMcuWidth);
  ptMcu.setY(ptPix.y() / m_nMcuHeight);
  return ptMcu;
}

// Convert pixel coordinate to block coordinate
//
// INPUT:
// - ptPix                                      = Pixel coordinate
// RETURN:
// - 8x8 block coordinate
//
QPoint CimgDecode::PixelToBlk(QPoint ptPix)
{
  QPoint ptBlk;

  ptBlk.setX(ptPix.x() / BLK_SZ_X);
  ptBlk.setY(ptPix.y() / BLK_SZ_Y);
  return ptBlk;
}

// Return the linear MCU offset from an MCU X,Y coord
//
// INPUT:
// - ptMcu                                      = MCU coordinate
// PRE:
// - m_nMcuXMax
// RETURN:
// - Index of MCU from start of MCUs
//
uint32_t CimgDecode::McuXyToLinear(QPoint ptMcu)
{
  uint32_t nLinear;

  nLinear = ((ptMcu.y() * m_nMcuXMax) + ptMcu.x());
  return nLinear;
}

// Create a file offset notation that represents bytes and bits
// - Essentially a fixed-point notation
//
// INPUT:
// - nByte                            = File byte position
// - nBit                               = File bit position
// RETURN:
// - Fixed-point file offset (29b for bytes, 3b for bits)
//
uint32_t CimgDecode::PackFileOffset(uint32_t nByte, uint32_t nBit)
{
  uint32_t nTmp;

  // Note that we only really need 3 bits, but I'll keep 4
  // so that the file offset is human readable. We will only
  // handle files up to 2^28 bytes (256MB), so this is probably
  // fine!
  nTmp = (nByte << 4) + nBit;
  return nTmp;
}

// Convert from file offset notation to bytes and bits
//
// INPUT:
// - nPacked                    = Fixed-point file offset (29b for bytes, 3b for bits)
// OUTPUT:
// - nByte                            = File byte position
// - nBit                               = File bit position
//
void CimgDecode::UnpackFileOffset(uint32_t nPacked, uint32_t &nByte, uint32_t &nBit)
{
  nBit = nPacked & 0x7;
  nByte = nPacked >> 4;
}

// Fetch the number of block markers assigned
//
// RETURN:
// - Number of marker blocks
//
uint32_t CimgDecode::GetMarkerCount()
{
  return m_nMarkersBlkNum;
}

// Fetch an indexed block marker
//
// INPUT:
// - nInd                                       = Index into marker block array
// RETURN:
// - Point (8x8 block) from marker array
//
QPoint CimgDecode::GetMarkerBlk(uint32_t nInd)
{
  QPoint myPt(0, 0);

  if(nInd < m_nMarkersBlkNum)
  {
    myPt = m_aptMarkersBlk[nInd];
  }
  else
  {
    Q_ASSERT(false);
  }
  return myPt;
}

// Add a block to the block marker list
// - Also report out the YCC DC value for the block
//
// INPUT:
// - nBlkX                                      = 8x8 block X coordinate
// - nBlkY                                      = 8x8 block Y coordinate
// POST:
// - m_nMarkersBlkNum
// - m_aptMarkersBlk[]
//
void CimgDecode::SetMarkerBlk(QPoint p)
{
  if(m_nMarkersBlkNum == MAX_BLOCK_MARKERS)
  {
    // Shift them down by 1. Last entry will be deleted next
    for(uint32_t ind = 1; ind < MAX_BLOCK_MARKERS; ind++)
    {
      m_aptMarkersBlk[ind - 1] = m_aptMarkersBlk[ind];
    }

    // Reflect new reduced count
    m_nMarkersBlkNum--;
  }

  QString strTmp;

  int32_t nY, nCb, nCr;
  int32_t nMcuX, nMcuY;
  int32_t nCssX, nCssY;

  // Determine the YCC of the block
  // Then calculate the MCU coordinate and the block coordinate within the MCU
  LookupBlkYCC(p, nY, nCb, nCr);
  nMcuX = p.x() / (m_nMcuWidth / BLK_SZ_X);
  nMcuY = p.y() / (m_nMcuHeight / BLK_SZ_Y);
  nCssX = p.x() % (m_nMcuWidth / BLK_SZ_X);
  nCssY = p.y() % (m_nMcuHeight / BLK_SZ_X);

  strTmp = QString("Position Marked @ MCU=[%1,%2](%3,%4) Block=[%5,%6] YCC=[%7,%8,%9]")
      .arg(nMcuX, 4)
      .arg(nMcuY, 4)
      .arg(nCssX)
      .arg(nCssY)
      .arg(p.x(), 4)
      .arg(p.y(), 4)
      .arg(nY, 5)
      .arg(nCb, 5)
      .arg(nCr, 5);
  m_pLog->AddLine(strTmp);
  m_pLog->AddLine("");

  m_aptMarkersBlk[m_nMarkersBlkNum] = p;
  m_nMarkersBlkNum++;
}

// Fetch the preview zoom mode
//
// RETURN:
// - The zoom mode enumeration
//
uint32_t CimgDecode::GetPreviewZoomMode()
{
  return m_nZoomMode;
}

// Fetch the preview mode
//
// RETURN:
// - The image preview mode enumeration
//
uint32_t CimgDecode::GetPreviewMode()
{
  return m_nPreviewMode;
}

// Fetch the preview zoom level
//
// RETURN:
// - The preview zoom level enumeration
//
double CimgDecode::GetPreviewZoom()
{
  return m_nZoom;
}

// Change the current zoom level
// - Supports either direct setting of the level
//   or an increment/decrement operation
//
// INPUT:
// - bInc                               = Flag to increment the zoom level
// - bDec                               = Flag to decrement the zoom level
// - bSet                               = Flag to set the zoom level
// - nVal                               = Zoom level for "set" operation
// POST:
// - m_nZoomMode
//
void CimgDecode::SetPreviewZoom(bool bInc, bool bDec, bool bSet, uint32_t nVal)
{
  if(bInc)
  {
    if(m_nZoomMode + 1 < PRV_ZOOMEND)
    {
      m_nZoomMode++;

    }
  }
  else if(bDec)
  {
    if(m_nZoomMode - 1 > PRV_ZOOMBEGIN)
    {
      m_nZoomMode--;
    }
  }
  else if(bSet)
  {
    m_nZoomMode = nVal;
  }
  
  switch (m_nZoomMode)
  {
    case PRV_ZOOM_12:
      m_nZoom = 0.125;
      break;

    case PRV_ZOOM_25:
      m_nZoom = 0.25;
      break;

    case PRV_ZOOM_50:
      m_nZoom = 0.5;
      break;

    case PRV_ZOOM_100:
      m_nZoom = 1.0;
      break;

    case PRV_ZOOM_150:
      m_nZoom = 1.5;
      break;

    case PRV_ZOOM_200:
      m_nZoom = 2.0;
      break;

    case PRV_ZOOM_300:
      m_nZoom = 3.0;
      break;

    case PRV_ZOOM_400:
      m_nZoom = 4.0;
      break;

    case PRV_ZOOM_800:
      m_nZoom = 8.0;
      break;

    default:
      m_nZoom = 1.0;
      break;
  }
}

// Main draw routine for the image
// - Draws the preview image with frame
// - Draws any histogram
// - Draws a title
// - Draws any MCU overlays / grid
//
// INPUT:
// - pDC                                        = The device context pointer
// - rectClient                         = From GetClientRect()
// - ptScrolledPos                      = From GetScrollPosition()
// - pFont                                      = Pointer to the font used for title/lables
// OUTPUT:
// - szNewScrollSize            = New dimension used for SetScrollSizes()
//
void CimgDecode::ViewOnDraw(QPainter *, QRect, QPoint, QFont *, QSize &)
//void CimgDecode::ViewOnDraw(QPainter * pDC, QRect rectClient, QPoint ptScrolledPos, QFont * pFont, QSize & szNewScrollSize)
{
//  m_nPageWidth = 600;
//  m_nPageHeight = 10;           // Start with some margin

//  QString strTmp;
//  QString strRender;

//  int32_t nHeight;

//  uint32_t nYPosImgTitle = 0;
//  uint32_t nYPosImg = 0;
//  uint32_t nYPosHistTitle = 0;
//  uint32_t nYPosHist = 0;
//  uint32_t nYPosHistYTitle = 0;
//  uint32_t nYPosHistY = 0;      // Y position of Img & Histogram

//  QRect rectTmp;

//  qDebug() << "CimgDecode::ViewOnDraw Start";

//  // If we have displayed an image, make sure to allow for
//  // the additional space!

//  bool bImgDrawn = false;

////  QBrush brGray(QColor(128, 128, 128));
////  QBrush brGrayLt1(QColor(210, 210, 210));
////  QBrush brGrayLt2(QColor(240, 240, 240));
////  QBrush brBlueLt(QColor(240, 240, 255));

//  QColor brGray = QColor(128, 128, 128);
//  QColor brGrayLt1 = QColor(210, 210, 210);
//  QColor brGrayLt2 = QColor(240, 240, 240);
//  QColor brBlueLt = QColor(240, 240, 255);

//  QPen penRed(Qt::red, 1, Qt::DotLine);

//  if(m_bDibTempReady)
//  {
//    qDebug() << "CimgDecode::ViewOnDraw CP1";

//    nYPosImgTitle = m_nPageHeight;
//    m_nPageHeight += nTitleHeight;      // Margin at top for title

//    m_rectImgReal = QRect(0, 0, static_cast<int32_t>(m_rectImgBase.right() * m_nZoom), static_cast<int32_t>(m_rectImgBase.bottom() * m_nZoom));

//    nYPosImg = m_nPageHeight;
//    m_nPageHeight += m_rectImgReal.height();
//    m_nPageHeight += nBorderBottom;     // Margin at bottom
//    bImgDrawn = true;

//    m_rectImgReal.adjust(nBorderLeft, nYPosImg, 0, 0);

//    // Now create the shadow of the main image
////!!    rectTmp = m_rectImgReal;
////!!    rectTmp.adjust(4, 4, 0, 0);
////!!    pDC->fillRect(rectTmp, QBrush(brGrayLt1));
////!!    rectTmp.adjust(-1, -1, 1, 1);
////!!    pDC->FrameRect(rectTmp, &brGrayLt2);
//  }

//  if(m_bHistEn)
//  {
//    if(m_bDibHistRgbReady)
//    {
//      nYPosHistTitle = m_nPageHeight;
//      m_nPageHeight += nTitleHeight;    // Margin at top for title

//      m_rectHistReal = m_rectHistBase;

//      nYPosHist = m_nPageHeight;
//      m_nPageHeight += m_rectHistReal.height();
//      m_nPageHeight += nBorderBottom;   // Margin at bottom
//      bImgDrawn = true;

//      m_rectHistReal.adjust(nBorderLeft, nYPosHist, 0, 0);

//      // Create the border
//      rectTmp = m_rectHistReal;
//      rectTmp.adjust(-1, -1, 1, 1);
//      pDC->setPen(QPen(brGray));
//      pDC->drawRect(rectTmp);
//    }

//    if(m_bDibHistYReady)
//    {
//      nYPosHistYTitle = m_nPageHeight;
//      m_nPageHeight += nTitleHeight;    // Margin at top for title

//      m_rectHistYReal = m_rectHistYBase;

//      nYPosHistY = m_nPageHeight;
//      m_nPageHeight += m_rectHistYReal.height();
//      m_nPageHeight += nBorderBottom;   // Margin at bottom
//      bImgDrawn = true;

//      m_rectHistYReal.adjust(nBorderLeft, nYPosHistY, 0, 0);

//      // Create the border
//      rectTmp = m_rectHistYReal;
//      rectTmp.adjust(-1, -1, 1, 1);
//      pDC->setPen(QPen(brGray));
//      pDC->drawRect(rectTmp);
//    }
//  }

//  // Find a starting line based on scrolling
//  // and current client size

//  QRect rectClientScrolled = rectClient;

//  rectClientScrolled.adjust(ptScrolledPos.x(), ptScrolledPos.y(), 0, 0);

//  // Change the font
//  QFont pOldFont;

//  pOldFont = pDC->font();

//  qDebug() << "Title 1";

//  // Draw the bitmap if ready
//  if(m_bDibTempReady)
//  {
//    // Print label
//    qDebug() << "Title 2 m_bDibTempReady";

//    if(!m_bPreviewIsJpeg)
//    {
//      // For all non-JPEG images, report with simple title
//      m_strTitle = "Image";
//    }
//    else
//    {
//      qDebug() << "Title 3 m_bPreviewIsJpeg";
//      m_strTitle = "Image (";

//      switch (m_nPreviewMode)
//      {
//        case PREVIEW_RGB:
//          m_strTitle += "RGB";
//          break;

//        case PREVIEW_YCC:
//          m_strTitle += "YCC";
//          break;

//        case PREVIEW_R:
//          m_strTitle += "R";
//          break;

//        case PREVIEW_G:
//          m_strTitle += "G";
//          break;

//        case PREVIEW_B:
//          m_strTitle += "B";
//          break;

//        case PREVIEW_Y:
//          m_strTitle += "Y";
//          break;

//        case PREVIEW_CB:
//          m_strTitle += "Cb";
//          break;

//        case PREVIEW_CR:
//          m_strTitle += "Cr";
//          break;

//        default:
//          m_strTitle += "???";
//          break;
//      }

//      if(m_bDecodeScanAc)
//      {
//        m_strTitle += ", DC+AC)";
//      }
//      else
//      {
//        m_strTitle += ", DC)";
//      }
//    }

//    switch (m_nZoomMode)
//    {
//      case PRV_ZOOM_12:
//        m_strTitle += " @ 12.5% (1/8)";
//        break;

//      case PRV_ZOOM_25:
//        m_strTitle += " @ 25% (1/4)";
//        break;

//      case PRV_ZOOM_50:
//        m_strTitle += " @ 50% (1/2)";
//        break;

//      case PRV_ZOOM_100:
//        m_strTitle += " @ 100% (1:1)";
//        break;

//      case PRV_ZOOM_150:
//        m_strTitle += " @ 150% (3:2)";
//        break;

//      case PRV_ZOOM_200:
//        m_strTitle += " @ 200% (2:1)";
//        break;

//      case PRV_ZOOM_300:
//        m_strTitle += " @ 300% (3:1)";
//        break;

//      case PRV_ZOOM_400:
//        m_strTitle += " @ 400% (4:1)";
//        break;

//      case PRV_ZOOM_800:
//        m_strTitle += " @ 800% (8:1)";
//        break;

//      default:
//        m_strTitle += "";
//        break;
//    }

//    // Calculate the title width
//    QRect rectCalc = QRect(0, 0, 0, 0);

////@@            nHeight = pDC->DrawText(m_strTitle,-1,&rectCalc,DT_CALQRect | DT_TOP | DT_NOPREFIX | DT_SINGLELINE);
//    int32_t nWidth = rectCalc.width() + 2 * nTitleIndent;

//    // Determine the title area (could be larger than the image
//    // if the image zoom is < 100% or sized small)

//    // Draw title background
//    rectTmp = QRect(m_rectImgReal.left(), nYPosImgTitle,
//                    qMax(m_rectImgReal.right(), m_rectImgReal.left() + nWidth), m_rectImgReal.top() - nTitleLowGap);
//    pDC->fillRect(rectTmp, brBlueLt);

//    // Draw the title
//    Qt::BGMode nBkMode = pDC->backgroundMode();

//    pDC->setBackgroundMode(Qt::TransparentMode);
//    rectTmp.adjust(nTitleIndent, 0, 0, 0);
////@@            nHeight = pDC->DrawText(m_strTitle,-1,&rectTmp,DT_TOP | DT_NOPREFIX | DT_SINGLELINE);
//    pDC->setBackgroundMode(nBkMode);

//    // Draw image

//    // Assume that the temp image has already been generated!
//    // For speed purposes, we use m_pDibTemp only when we are
//    // in a mode other than RGB or YCC. In the RGB/YCC modes,
//    // we skip the CalcChannelPreview() step.

//    // TODO: Improve redraw time by only redrawing the currently visible
//    // region. Requires a different CopyDIB routine with a subset region.
//    // Needs to take into account zoom setting (eg. intersection between
//    // rectClient and m_rectImgReal).

//    // Use a common DIB instead of creating/swapping tmp / ycc and rgb.
//    // This way we can also have more flexibility in modifying RGB & YCC displays.
//    // Time calling CalcChannelPreview() seems small, so no real impact.

//    // Image member usage:
//    // m_pDibTemp:

////@@            m_pDibTemp.CopyDIB(pDC,m_rectImgReal.left,m_rectImgReal.top,m_nZoom);

//    // Now create overlays

//    // Only draw overlay (eg. actual image boundary overlay) if the values
//    // have been set properly. Note that PSD decode currently sets these
//    // values to zero.
//    if((m_nDimX != 0) && (m_nDimY != 0))
//    {
//      pDC->setPen(penRed);

//      // Draw boundary for end of valid data (inside partial MCU)
//      int32_t nXZoomed = static_cast<int32_t>(m_nDimX * m_nZoom);

//      int32_t nYZoomed = static_cast<int32_t>(m_nDimY * m_nZoom);

//      pDC->drawLine(m_rectImgReal.left() + nXZoomed, m_rectImgReal.top(), m_rectImgReal.left() + nXZoomed, m_rectImgReal.top() + nYZoomed);
//      pDC->drawLine(m_rectImgReal.left(), m_rectImgReal.top() + nYZoomed, m_rectImgReal.left() + nXZoomed, m_rectImgReal.top() + nYZoomed);
////!!      pDC->setPen(pPen);
//    }

//    // Before we frame the region, let's draw any remaining overlays

//    // Only draw the MCU overlay if zoom is > 100%, otherwise we will have
//    // replaced entire image with the boundary lines!
//    if(m_bViewOverlaysMcuGrid && (m_nZoomMode >= PRV_ZOOM_25))
//    {
//      ViewMcuOverlay(pDC);
//    }

//    // Always draw the markers
//    ViewMcuMarkedOverlay(pDC);

//    // Final frame border
//    rectTmp = m_rectImgReal;
//    rectTmp.adjust(-1, -1, 1, 1);
////@@    pDC->setPen(brGray);
//    pDC->drawRect(rectTmp);

//    // Preserve the image placement (for other functions, such as click detect)
//    m_nPreviewPosX = m_rectImgReal.left();
//    m_nPreviewPosY = m_rectImgReal.top();
//    m_nPreviewSizeX = m_rectImgReal.width();
//    m_nPreviewSizeY = m_rectImgReal.height();

//    // m_nPageWidth is already hardcoded above
//    Q_ASSERT(m_nPageWidth >= 0);
//    m_nPageWidth = qMax(static_cast<uint32_t>(m_nPageWidth), m_nPreviewPosX + m_nPreviewSizeX);

//    update();
//  }

//  if(m_bHistEn)
//  {
//    if(m_bDibHistRgbReady)
//    {
//      // Draw title background
//      rectTmp = QRect(m_rectHistReal.left(), nYPosHistTitle, m_rectHistReal.right(), m_rectHistReal.top() - nTitleLowGap);
//      pDC->fillRect(rectTmp, brBlueLt);

//      // Draw the title
//      QRect boundingRect;
//      Qt::BGMode nBkMode = pDC->backgroundMode();
//      pDC->setBackgroundMode(Qt::TransparentMode);
//      rectTmp.adjust(nTitleIndent, 0, 0, 0);
//      pDC->drawText(rectTmp, Qt::AlignLeft | Qt::AlignTop, "Histogram (RGB)", &boundingRect);
//      pDC->setBackgroundMode(nBkMode);

//      // Draw image
////@@                    m_pDibHistRgb.CopyDIB(pDC,m_rectHistReal.left,m_rectHistReal.top);
//    }

//    if(m_bDibHistYReady)
//    {
//      // Draw title background
//      rectTmp = QRect(m_rectHistYReal.left(), nYPosHistYTitle, m_rectHistYReal.right(), m_rectHistYReal.top() - nTitleLowGap);
//      pDC->fillRect(rectTmp, brBlueLt);

//      // Draw the title
//      Qt::BGMode nBkMode = pDC->backgroundMode();
//      pDC->setBackgroundMode(Qt::TransparentMode);
//      rectTmp.adjust(nTitleIndent, 0, 0, 0);
////@@                    nHeight = pDC->DrawText("Histogram (Y)",-1,&rectTmp,DT_TOP | DT_NOPREFIX | DT_SINGLELINE);
//      pDC->setBackgroundMode(nBkMode);

//      // Draw image
////@@                    m_pDibHistY.CopyDIB(pDC,m_rectHistYReal.left,m_rectHistYReal.top);
//      //m_pDibHistY.CopyDIB(pDC,nBorderLeft,nYPosHistY);
//    }

//    update();
//  }

//  // If no image has been drawn, indicate to user why!
//  if(!bImgDrawn)
//  {
//    //ScrollRect.top = m_nPageHeight;       // FIXME:?

//    // Print label
//    //nHeight = pDC->DrawText("Image Decode disabled. Enable with [Options->Decode Scan Image]", -1,&ScrollRect,
//    //      DT_TOP | DT_NOPREFIX | DT_SINGLELINE);

//  }

//  // Restore the original font
//  pDC->setFont(pOldFont);

//  // Set scroll bars accordingly. We use the page dimensions here.
//  QSize sizeTotal(m_nPageWidth + nBorderLeft, m_nPageHeight);

//  szNewScrollSize = sizeTotal;
}

// Draw an overlay that shows the MCU grid
//
// INPUT:
// - pDC                        = The device context pointer
//
void CimgDecode::ViewMcuOverlay(QPainter * pDC)
{
  // Now create overlays
  uint32_t nXZoomed, nYZoomed;

  QPen penDot(QColor(32, 32, 32), 1, Qt::DotLine);

  Qt::BGMode nBkModeOld = pDC->backgroundMode();
  QPen pPenOld = pDC->pen();

  pDC->setBackgroundMode(Qt::TransparentMode);

  // Draw vertical lines
  for(uint32_t nMcuX = 0; nMcuX < m_nMcuXMax; nMcuX++)
  {
    nXZoomed = static_cast<int32_t>(nMcuX * m_nMcuWidth * m_nZoom);
    pDC->drawLine(m_rectImgReal.left() + nXZoomed, m_rectImgReal.top(), m_rectImgReal.left() + nXZoomed, m_rectImgReal.bottom());
  }

  for(uint32_t nMcuY = 0; nMcuY < m_nMcuYMax; nMcuY++)
  {
    nYZoomed = static_cast<int32_t>(nMcuY * m_nMcuHeight * m_nZoom);
    pDC->drawLine(m_rectImgReal.left(), m_rectImgReal.top() + nYZoomed, m_rectImgReal.right(), m_rectImgReal.top() + nYZoomed);
  }

  pDC->setPen(pPenOld);
  pDC->setBackgroundMode(nBkModeOld);
}

// Draw an overlay that highlights the marked MCUs
//
// INPUT:
// - pDC                        = The device context pointer
//
void CimgDecode::ViewMcuMarkedOverlay(QPainter * pDC)
{
  pDC;                          // Unreferenced param

  // Now draw a simple MCU Marker overlay
  QRect my_rect;

  QBrush my_brush(QColor(255, 0, 255));

  for(uint32_t nMcuY = 0; nMcuY < m_nMcuYMax; nMcuY++)
  {
    for(uint32_t nMcuX = 0; nMcuX < m_nMcuXMax; nMcuX++)
    {
      //uint32_t nXY = nMcuY*m_nMcuXMax + nMcuX;
      //uint32_t nXZoomed = (uint32_t)(nMcuX*m_nMcuWidth*m_nZoom);
      //uint32_t nYZoomed = (uint32_t)(nMcuY*m_nMcuHeight*m_nZoom);

      /*
         // TODO: Implement an overlay function
         if (m_bMarkedMcuMapEn && m_abMarkedMcuMap && m_abMarkedMcuMap[nXY]) {
         // Note that drawing is an overlay, so we are dealing with real
         // pixel coordinates, not preview image coords
         my_rect = QRect(nXZoomed,nYZoomed,nXZoomed+(uint32_t)(m_nMcuWidth*m_nZoom),
         nYZoomed+(uint32_t)(m_nMcuHeight*m_nZoom));
         my_rect.adjust(m_rectImgReal.left,m_rectImgReal.top);
         pDC->FrameRect(my_rect,&my_brush);
         }
       */

    }
  }

}

// Draw an overlay for the indexed block
//
// INPUT:
// - pDC                        = The device context pointer
// - nBlkX                      = 8x8 block X coordinate
// - nBlkY                      = 8x8 block Y coordinate
//
void CimgDecode::ViewMarkerOverlay(QPainter * pDC, uint32_t nBlkX, uint32_t nBlkY)
{
  QRect my_rect;

  // Note that drawing is an overlay, so we are dealing with real
  // pixel coordinates, not preview image coords
  my_rect = QRect(static_cast<int32_t>(m_nZoom * (nBlkX + 0)),
                  static_cast<int32_t>(m_nZoom * (nBlkY + 0)),
                  static_cast<int32_t>(m_nZoom * (nBlkX + 1)),
                  static_cast<int32_t>(m_nZoom * (nBlkY + 1)));
  my_rect.adjust(m_nPreviewPosX, m_nPreviewPosY, 0, 0);
  pDC->fillRect(my_rect, QColor(255, 0, 255));
}

// Get the preview MCU grid setting
//
// OUTPUT:
// - The MCU grid enabled flag
//
bool CimgDecode::GetPreviewOverlayMcuGrid()
{
  return m_bViewOverlaysMcuGrid;
}

// Toggle the preview MCU grid setting
//
// POST:
// - m_bViewOverlaysMcuGrid
//
void CimgDecode::SetPreviewOverlayMcuGridToggle()
{
  if(m_bViewOverlaysMcuGrid)
  {
    m_bViewOverlaysMcuGrid = false;
  }
  else
  {
    m_bViewOverlaysMcuGrid = true;
  }
}

// Report the DC levels
// - UNUSED
//
// INPUT:
// - nMcuX                              = MCU x coordinate
// - nMcuY                              = MCU y coordinate
// - nMcuLen                    = Number of MCUs to report
//
void CimgDecode::ReportDcRun(uint32_t nMcuX, uint32_t nMcuY, uint32_t nMcuLen)
{
  // FIXME: Should I be working on MCU level or block level?
  QString strTmp;

  m_pLog->AddLine("");
  m_pLog->AddLineHdr("*** Reporting DC Levels ***");
  strTmp = QString("  Starting MCU   = [%1,%2]")
      .arg(nMcuX)
      .arg(nMcuY);
  strTmp = QString("  Number of MCUs = %1").arg(nMcuLen);
  m_pLog->AddLine(strTmp);
  for(uint32_t ind = 0; ind < nMcuLen; ind++)
  {
    // TODO: Need some way of getting these values
    // For now, just rely on bDetailVlcEn & PrintDcCumVal() to report this...
  }
}

// Update the YCC status text
//
// INPUT:
// - strText                    = Status text to display
//
void CimgDecode::SetStatusYccText(QString strText)
{
  m_strStatusYcc = strText;
}

// Fetch the YCC status text
//
// RETURN:
// - Status text currently displayed
//
QString CimgDecode::GetStatusYccText()
{
  return m_strStatusYcc;
}

// Update the MCU status text
//
// INPUT:
// - strText                            = MCU indicator text
//
void CimgDecode::SetStatusMcuText(QString strText)
{
  m_strStatusMcu = strText;
}

// Fetch the MCU status text
//
// RETURN:
// - MCU indicator text
//
QString CimgDecode::GetStatusMcuText()
{
  return m_strStatusMcu;
}

// Update the file position text
//
// INPUT:
// - strText                            = File position text
//
void CimgDecode::SetStatusFilePosText(QString strText)
{
  m_strStatusFilePos = strText;
}

// Fetch the file position text
//
// RETURN:
// - File position text
//
QString CimgDecode::GetStatusFilePosText()
{
  return m_strStatusFilePos;
}
