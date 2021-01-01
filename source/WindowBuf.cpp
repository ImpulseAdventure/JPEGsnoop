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

#include <QPlainTextEdit>
#include <QString>

#include "WindowBuf.h"

// Constructor for WindowBuf class
// - Allocates storage for the buffer
// - Resets state
// - Clears all buffer overlay bytes
//
CwindowBuf::CwindowBuf(CDocLog *pDocLog) : m_pDocLog(pDocLog)
{
  m_pBuffer = new unsigned char[MAX_BUF];

  if(!m_pBuffer)
  {
    msgBox.setText("ERROR: Not enough memory for File Buffer");
    msgBox.exec();
    exit(1);
  }

  m_pStatBar = nullptr;

  Reset();

  // Initialize all overlays as not defined.
  // Only create space for them as required
  m_nOverlayNum = 0;
  m_nOverlayMax = 0;

  for(uint32_t nInd = 0; nInd < NUM_OVERLAYS; nInd++)
  {
    m_psOverlay[nInd] = nullptr;
  }
}

// Destructor deallocates buffers and overlays
CwindowBuf::~CwindowBuf()
{
  if(m_pBuffer != nullptr)
  {
    delete m_pBuffer;

    m_pBuffer = nullptr;
    m_bBufOK = false;
  }

  // Clear up overlays
  for(uint32_t nInd = 0; nInd < NUM_OVERLAYS; nInd++)
  {
    if(m_psOverlay[nInd])
    {
      delete m_psOverlay[nInd];

      m_psOverlay[nInd] = nullptr;
    }
  }
}

// Reset the main state
//
void CwindowBuf::Reset()
{
  // File handling
  m_bBufOK = false;             // Initialize the buffer to not loaded yet
  m_pBufFile = nullptr;            // No file open yet
}

// Accessor for m_bBufOk
bool CwindowBuf::GetBufOk()
{
  return m_bBufOK;
}

// Accessor for m_nPosEof
uint32_t CwindowBuf::GetPosEof()
{
  return m_nPosEof;
}

// Retain a copy of the file pointer and fetch the file size
//
// POST:
// - m_pBufFile
// - m_nPosEof
//
void CwindowBuf::BufFileSet(QFile * inFile)
{
  Q_ASSERT(inFile);

  if(!inFile)
  {
    msgBox.setText("ERROR: BufFileSet() with NULL inFile");
    msgBox.exec();
    Q_ASSERT(false);
    return;
  }

  m_pBufFile = inFile;
  m_nPosEof = m_pBufFile->size();

  if(m_nPosEof == 0)
  {
    m_pBufFile = nullptr;
    msgBox.setText("ERROR: BufFileSet() File length zero");
    msgBox.exec();
  }
}

// Called to mark the buffer as closed
// - Typically done when we've finished processing a file
//   This avoids problems where we try to read a buffer
//   that has already been terminated.
//
// POST:
// - m_pBufFile
//
void CwindowBuf::BufFileUnset()
{
  if(m_pBufFile)
  {
    m_pBufFile = nullptr;
  }
}

// Search for a value in the buffer from a given starting position
// and direction, limited to a maximum search depth
// - Search value can be 8-bit, 16-bit or 32-bit
// - Update progress in lengthy searches
//
// INPUT:
// - nStartPos                  Starting byte offset for search
// - nSearchVal                 Value to search for (up to 32-bit unsigned)
// - nSearchLen                 Maximum number of bytes to search
// - bDirFwd                    TRUE for forward, FALSE for backwards
//
// PRE:
// - m_nPosEof
//
// OUTPUT:
// - nFoundPos                  Byte offset in buffer for start of search match
//
// RETURN:
// - Success in finding the value
//
bool CwindowBuf::BufSearch(uint32_t nStartPos, uint32_t nSearchVal, uint32_t nSearchLen,
                           bool bDirFwd, uint32_t &nFoundPos)
{
  // Save the current position
  uint32_t nCurPos;

  uint32_t nCurVal;

  QString strStatus;

  time_t tmLast = clock();

  nCurPos = nStartPos;
  bool bDone = false;

  bool bFound = false;

  while(!bDone)
  {
    // Update progress in status bar
    // - Note that we only check timer when step counter has
    //   reached a certain threshold. This limits the overhead
    //   associated with the timer comparison
    if(((nCurPos % (16 * 1024)) == 0) && (m_pStatBar))
    {
      time_t tmNow = clock();

      if((tmNow - tmLast) > (CLOCKS_PER_SEC / 8))
      {
        tmLast = tmNow;
        float fProgress = (nCurPos * 100.0f) / m_nPosEof;

        strStatus = QString("Searching %3.f%% (%lu of %lu)...").arg(fProgress).arg(nCurPos).arg(m_nPosEof);
        m_pStatBar->showMessage(strStatus);
      }
    }

    if(bDirFwd)
    {
      nCurPos++;

      if(nCurPos + (nSearchLen - 1) >= m_nPosEof)
      {
        bDone = true;
      }
    }
    else
    {
      if(nCurPos > 0)
      {
        nCurPos--;
      }
      else
      {
        bDone = true;
      }
    }

    if(nSearchLen == 4)
    {
      nCurVal = (Buf(nCurPos + 0) << 24) + (Buf(nCurPos + 1) << 16) + (Buf(nCurPos + 2) << 8) + Buf(nCurPos + 3);
    }
    else if(nSearchLen == 3)
    {
      nCurVal = (Buf(nCurPos + 0) << 16) + (Buf(nCurPos + 1) << 8) + Buf(nCurPos + 2);
    }
    else if(nSearchLen == 2)
    {
      nCurVal = (Buf(nCurPos + 0) << 8) + Buf(nCurPos + 1);
    }
    else if(nSearchLen == 1)
    {
      nCurVal = Buf(nCurPos + 0);
    }
    else
    {
      msgBox.setText("ERROR: Unexpected nSearchLen");
      msgBox.exec();
      nCurVal = 0x0000;
    }

    if(nCurVal == nSearchVal)
    {
      bFound = true;
      bDone = true;
    }
  }

  nFoundPos = nCurPos;
  return bFound;
}

//SetStatusText

// Establish local copy of status bar pointer
void CwindowBuf::SetStatusBar(QStatusBar * pStatBar)
{
  m_pStatBar = pStatBar;
}

// Search for a variable-length byte string in the buffer from a given starting position
// and direction, limited to a maximum search depth
// - Search string is array of unsigned bytes
// - Update progress in lengthy searches
//
// INPUT:
// - nStartPos                  Starting byte offset for search
// - anSearchVal                Byte array to search for
// - nSearchLen                 Maximum number of bytes to search
// - bDirFwd                    TRUE for forward, FALSE for backwards
//
// PRE:
// - m_nPosEof
// - m_pStatBar
//
// OUTPUT:
// - nFoundPos                  Byte offset in buffer for start of search match
//
// RETURN:
// - Success in finding the value
//
bool CwindowBuf::BufSearchX(uint32_t nStartPos, unsigned char *anSearchVal, uint32_t nSearchLen,
                            bool bDirFwd, uint32_t &nFoundPos)
{
  // Save the current position
  uint32_t nCurPos;

  uint32_t nByteCur;
  uint32_t nByteSearch;

  uint32_t nCurPosOffset;
  uint32_t nMatchStartPos = 0;

  //bool                  bMatchStart = false;
  bool bMatchOn = false;

  QString strStatus;

  time_t tmLast = clock();

  nCurPosOffset = 0;
  nCurPos = nStartPos;
  bool bDone = false;

  bool bFound = false;          // Matched entire search string

  while(!bDone)
  {

    if(bDirFwd)
    {
      nCurPos++;

      if(nCurPos + (nSearchLen - 1) >= m_nPosEof)
      {
        bDone = true;
      }
    }
    else
    {
      if(nCurPos > 0)
      {
        nCurPos--;
      }
      else
      {
        bDone = true;
      }
    }

    // Update progress in status bar
    // - Note that we only check timer when step counter has
    //   reached a certain threshold. This limits the overhead
    //   associated with the timer comparison
    if(((nCurPos % (16 * 1024)) == 0) && (m_pStatBar))
    {
      time_t tmNow = clock();

      if((tmNow - tmLast) > (CLOCKS_PER_SEC / 8))
      {
        tmLast = tmNow;
        float fProgress = (nCurPos * 100.0f) / m_nPosEof;

        strStatus = QString("Searching %3.f%% (%lu of %lu)...").arg(fProgress).arg(nCurPos).arg(m_nPosEof);
        m_pStatBar->showMessage(strStatus);
      }
    }

    nByteSearch = anSearchVal[nCurPosOffset];
    nByteCur = Buf(nCurPos);

    if(nByteSearch == nByteCur)
    {
      if(!bMatchOn)
      {
        // Since we aren't in match mode, we are beginning a new
        // sequence, so save the starting position in case we
        // have to rewind
        nMatchStartPos = nCurPos;
        bMatchOn = true;
      }

      nCurPosOffset++;
    }
    else
    {
      if(bMatchOn)
      {
        // Since we were in a sequence of matches, but ended early,
        // we now need to reset our position to just after the start
        // of the previous 1st match.
        nCurPos = nMatchStartPos;
        bMatchOn = false;
      }

      nCurPosOffset = 0;
    }

    if(nCurPosOffset >= nSearchLen)
    {
      // We matched the entire length of our search string!
      bFound = true;
      bDone = true;
    }
  }

  if(m_pStatBar)
  {
    m_pStatBar->showMessage("Done");
  }

  if(bFound)
  {
    nFoundPos = nMatchStartPos;
  }

  return bFound;
}

// Ensure that the file offset parameter is captured in the current
// buffer cache. If not, reload a new cache around this offset.
// - Provide some limited cache prior to the desired address
// - Provide majority of cache after the desired address
// - The cache prior to the address facilitates reverse search performance
//
// INPUT:
// - nPosition                          File offset to ensure is available in new window
// PRE:
// - m_pBufFile
// - m_nPosEof
//
// POST:
// - m_bBufOK
// - m_nBufWinSize
// - m_nBufWinStart
//
void CwindowBuf::BufLoadWindow(uint32_t nPosition)
{

  // We must not try to perform a seek command on a CFile that
  // has already been closed, so we must check first.

  if(m_pBufFile)
  {
    /*
       QString strTmp;
       strTmp = QString("** BufLoadWindow @ 0x%08X"),nPosition);
       log->AddLine(strTmp);
     */

    // Initialize to bad values
    m_bBufOK = false;
    m_nBufWinSize = 0;
    m_nBufWinStart = 0;

    // For now, just read 128KB starting at current position
    // Later on, we will need to do range checking and even start
    // at a nPosition some distance before the "nPosition" to allow
    // for some reverse seeking without refetching data.

    uint32_t nPositionAdj;

    if(nPosition >= MAX_BUF_WINDOW_REV)
    {
      nPositionAdj = nPosition - MAX_BUF_WINDOW_REV;
    }
    else
    {
      nPositionAdj = 0;
    }

    // NOTE:
    // Need to ensure that we don't try to read past the end of file.
    // I have encountered some JPEGs that have Canon makernote
    // fields with OffsetValue > 0xFFFF0000! Interpreting this as-is
    // would cuase BufLoadWindow() to read past the end of file.
    if(nPositionAdj >= m_nPosEof)
    {

      // ERROR! For now, just do this silently
      // We're not going to throw up any errors unless we can
      // limit the number that we'll display to the user!
      // The current code will not reset the nPosition of the
      // buffer, so every byte will try to reset it.
      // FIXME
      return;
    }

    qint64 nVal;

    nVal = m_pBufFile->seek(nPositionAdj);
    nVal = m_pBufFile->read(reinterpret_cast<char *>(m_pBuffer), MAX_BUF_WINDOW);

    if(nVal <= 0)
    {
      // Failed to read anything!
      // ERROR!
      return;
    }
    else
    {
      // Read OK
      // Recalculate bounds
      m_bBufOK = true;
      m_nBufWinStart = nPositionAdj;
      m_nBufWinSize = nVal;
    }
  }
  else
  {
    msgBox.setText("ERROR: BufLoadWindow() and no file open");
    msgBox.exec();
  }
}

// Allocate a new buffer overlay into the array
// over overlays. Limits the number of overlays
// to NUM_OVERLAYS.
// - If the indexed overlay already exists, no changes are made
// - TODO: Replace with vector
//
// INPUT:
// - nInd               Overlay index to allocate
//
// POST:
// - m_psOverlay[]
//
bool CwindowBuf::OverlayAlloc(uint32_t nInd)
{
  if(nInd >= NUM_OVERLAYS)
  {
    msgBox.setText("ERROR: Maximum number of overlays reached");
    msgBox.exec();
    return false;

  }
  else if(m_psOverlay[nInd])
  {
    // Already allocated, move on
    return true;

  }
  else
  {
    m_psOverlay[nInd] = new sOverlay();

    if(!m_psOverlay[nInd])
    {
      msgBox.setText("NOTE: Out of memory for extra file overlays");
      msgBox.exec();
      return false;
    }
    else
    {
      memset(m_psOverlay[nInd], 0, sizeof(sOverlay));
      // FIXME: may not be necessary
      m_psOverlay[nInd]->bEn = false;
      m_psOverlay[nInd]->nStart = 0;
      m_psOverlay[nInd]->nLen = 0;

      m_psOverlay[nInd]->nMcuX = 0;
      m_psOverlay[nInd]->nMcuY = 0;
      m_psOverlay[nInd]->nMcuLen = 0;
      m_psOverlay[nInd]->nMcuLenIns = 0;
      m_psOverlay[nInd]->nDcAdjustY = 0;
      m_psOverlay[nInd]->nDcAdjustCb = 0;
      m_psOverlay[nInd]->nDcAdjustCr = 0;

      // FIXME: Need to ensure that this is right
      if(nInd + 1 >= m_nOverlayMax)
      {
        m_nOverlayMax = nInd + 1;
      }
      //m_nOverlayMax++;

      return true;
    }
  }
}

// Report out the list of overlays thave have been allocated
//
// PRE:
// - m_nOverlayNum
// - m_psOverlay[]
//
void CwindowBuf::ReportOverlays(CDocLog *pLog)
{
  QString strTmp;

  if(m_nOverlayNum > 0)
  {
    strTmp = QString("  Buffer Overlays active: %1").arg(m_nOverlayNum);
    pLog->AddLine(strTmp);

    for(uint32_t ind = 0; ind < m_nOverlayNum; ind++)
    {
      if(m_psOverlay[ind])
      {
        strTmp =
          QString("    %03u: MCU[%4u,%4u] MCU DelLen=[%2u] InsLen=[%2u] DC Offset YCC=[%5d,%5d,%5d] Overlay Byte Len=[%4u]").
          arg(ind).arg(m_psOverlay[ind]->nMcuX).arg(m_psOverlay[ind]->nMcuY).arg(m_psOverlay[ind]->nMcuLen).arg(m_psOverlay[ind]->
                                                                                                                nMcuLenIns).
          arg(m_psOverlay[ind]->nDcAdjustY).arg(m_psOverlay[ind]->nDcAdjustCb).arg(m_psOverlay[ind]->nDcAdjustCr).
          arg(m_psOverlay[ind]->nLen);
        pLog->AddLine(strTmp);
      }
    }

    pLog->AddLine("");
  }
}

// Define the content of an overlay
//
// INPUT:
// - nOvrInd                    The overlay index to update/replace
// - pOverlay                   The byte array that defines the overlay content
// - nLen                               Byte length of the overlay
// - nBegin                             Starting byte offset for the overlay
// - nMcuX                              Additional info for this overlay
// - nMcuY                              Additional info for this overlay
// - nMcuLen                    Additional info for this overlay
// - nMcuLenIns                 Additional info for this overlay
// - nAdjY                              Additional info for this overlay
// - nAdjCb                             Additional info for this overlay
// - nAdjCr                             Additional info for this overlay
//
bool CwindowBuf::OverlayInstall(uint32_t nOvrInd, unsigned char *pOverlay, uint32_t nLen, uint32_t nBegin,
                                uint32_t nMcuX, uint32_t nMcuY, uint32_t nMcuLen, uint32_t nMcuLenIns,
                                int nAdjY, int nAdjCb, int nAdjCr)
{
  nOvrInd;                      // Unreferenced param

  // Ensure that the overlay is allocated, and allocate it
  // if required. Fail out if we can't add (or run out of space)
  if(!OverlayAlloc(m_nOverlayNum))
  {
    return false;
  }

  if(nLen < MAX_OVERLAY)
  {
    m_psOverlay[m_nOverlayNum]->bEn = true;
    m_psOverlay[m_nOverlayNum]->nLen = nLen;

    // Copy the overlay content, but clip to maximum size and pad if shorter
    for(uint32_t i = 0; i < MAX_OVERLAY; i++)
    {
      m_psOverlay[m_nOverlayNum]->anData[i] = (i < nLen) ? pOverlay[i] : 0x00;
    }

    m_psOverlay[m_nOverlayNum]->nStart = nBegin;

    // For reporting, save the extra data
    m_psOverlay[m_nOverlayNum]->nMcuX = nMcuX;
    m_psOverlay[m_nOverlayNum]->nMcuY = nMcuY;
    m_psOverlay[m_nOverlayNum]->nMcuLen = nMcuLen;
    m_psOverlay[m_nOverlayNum]->nMcuLenIns = nMcuLenIns;
    m_psOverlay[m_nOverlayNum]->nDcAdjustY = nAdjY;
    m_psOverlay[m_nOverlayNum]->nDcAdjustCb = nAdjCb;
    m_psOverlay[m_nOverlayNum]->nDcAdjustCr = nAdjCr;

    m_nOverlayNum++;
  }
  else
  {
    msgBox.setText("ERROR: CwindowBuf:OverlayInstall() overlay too large");
    msgBox.exec();
    return false;
  }

  return true;
}

// Remove latest overlay entry
//
// POST:
// - m_nOverlayNum
// - m_psOverlay[]
//
void CwindowBuf::OverlayRemove()
{
  if(m_nOverlayNum <= 0)
  {
    return;
  }

  m_nOverlayNum--;

  // Note that we've already decremented the m_nOverlayNum
  if(m_psOverlay[m_nOverlayNum])
  {
    // Don't need to delete the overlay struct as we might as well reuse it
    m_psOverlay[m_nOverlayNum]->bEn = false;
    //delete m_psOverlay[m_nOverlayNum];
    //m_psOverlay[m_nOverlayNum] = nullptr;
  }
}

// Disable all buffer overlays
//
// POST:
// - m_nOverlayNum
// - m_psOverlay[]
//
void CwindowBuf::OverlayRemoveAll()
{
  m_nOverlayNum = 0;

  for(uint32_t nInd = 0; nInd < m_nOverlayMax; nInd++)
  {
    if(m_psOverlay[nInd])
    {
      m_psOverlay[nInd]->bEn = false;
    }
  }
}

// Fetch the indexed buffer overlay
//
// INPUT:
// - nOvrInd            The overlay index
//
// OUTPUT:
// - pOverlay           A pointer to the indexed buffer
// - nLen                       Length of the overlay string
// - nBegin                     Starting file offset for the overlay
//
// RETURN:
// - Success if overlay index was allocated and enabled
//
bool CwindowBuf::OverlayGet(uint32_t nOvrInd, unsigned char *&pOverlay, uint32_t &nLen, uint32_t &nBegin)
{
  if((m_psOverlay[nOvrInd]) && (m_psOverlay[nOvrInd]->bEn))
  {
    pOverlay = m_psOverlay[nOvrInd]->anData;
    nLen = m_psOverlay[nOvrInd]->nLen;
    nBegin = m_psOverlay[nOvrInd]->nStart;
    return m_psOverlay[nOvrInd]->bEn;
  }
  else
  {
    return false;
  }
}

// Get the number of buffer overlays allocated
//
uint32_t CwindowBuf::OverlayGetNum()
{
  return m_nOverlayNum;
}

// Replaces the direct buffer access with a managed refillable window/cache.
// - Support for 1-byte access only
// - Support for overlays (optional)
//
// INPUT:
// - nOffset                    File offset to fetch from (via cache)
// - bClean                             Flag that indicates if overlays can be used
//                      If set to FALSE, then content from overlays that span
//                      the offset address will be returned instead of the file content
//
// RETURN:
// - Byte from the desired address
uint8_t CwindowBuf::Buf(uint32_t nOffset, bool bClean)
{
  // We are requesting address "nOffset"
  // Our current window runs from "m_nBufWinStart...buf_win_end" (m_nBufWinSize)
  // Therefore, our relative addr is nOffset-m_nBufWinStart

  long nWinRel;

  unsigned char nCurVal = 0;

  uint32_t bInOvrWindow = false;

  if(!m_pBufFile)
  {
    // FIXME: Open file or provide error
  }

  Q_ASSERT(m_pBufFile);

  // Allow for overlay buffer capability (if not in "clean" mode)
  if(!bClean)
  {
    // Now handle any overlays
    for(uint32_t nInd = 0; nInd < m_nOverlayNum; nInd++)
    {
      if(m_psOverlay[nInd])
      {
        if(m_psOverlay[nInd]->bEn)
        {
          if((nOffset >= m_psOverlay[nInd]->nStart) && (nOffset < m_psOverlay[nInd]->nStart + m_psOverlay[nInd]->nLen))
          {
            nCurVal = m_psOverlay[nInd]->anData[nOffset - m_psOverlay[nInd]->nStart];
            bInOvrWindow = true;
          }
        }
      }
    }

    if(bInOvrWindow)
    {

      // Before we return, make sure that the real buffer handles this region!
      nWinRel = nOffset - m_nBufWinStart;

      if((nWinRel >= 0) && (nWinRel < (long) m_nBufWinSize))
      {
      }
      else
      {
        // Address is outside of current window
        BufLoadWindow(nOffset);
      }

      return nCurVal;
    }
  }

  // Now that we've finished any overlays, proceed to actual cache content

  // Determine if the offset is within the current cache
  // If not, reload a new cache around the desired address
  nWinRel = nOffset - m_nBufWinStart;

  if((nWinRel >= 0) && (nWinRel < (long) m_nBufWinSize))
  {
    // Address is within current window
    return m_pBuffer[nWinRel];
  }
  else
  {
    // Address is outside of current window
    BufLoadWindow(nOffset);

    // Now we assume that the address is in range
    // m_nBufWinStart has now been updated
    // TODO: check this
    nWinRel = nOffset - m_nBufWinStart;

    // Now recheck the window
    // TODO: Rewrite the following in a cleaner manner
    if((nWinRel >= 0) && (nWinRel < (long) m_nBufWinSize))
    {
      return m_pBuffer[nWinRel];
    }
    else
    {
      // Still bad after refreshing window, so it must be bad addr
      m_bBufOK = false;
      // FIXME: Need to report error somehow
      //log->AddLine("ERROR: Overread buffer - file may be truncated"),9);
      return 0;
    }
  }
}

// Replaces the direct buffer access with a managed refillable window/cache.
// - Supports 1/2/4 byte fetch
// - No support for overlays
//
// INPUT:
// - nOffset                    File offset to fetch from (via cache)
// - nSz                                Size of word to fetch (1,2,4)
// - nByteSwap                  Flag to indicate if UINT16 or UINT32 should be byte-swapped
//
// RETURN:
// - 1/2/4 unsigned bytes from the desired address
//
uint32_t CwindowBuf::BufX(uint32_t nOffset, uint32_t nSz, bool nByteSwap)
{
  long nWinRel;

  Q_ASSERT(m_pBufFile);

  nWinRel = nOffset - m_nBufWinStart;
  if((nWinRel >= 0) && (nWinRel + nSz < m_nBufWinSize))
  {
    // Address is within current window
    if(!nByteSwap)
    {
      if(nSz == 4)
      {
        return ((m_pBuffer[nWinRel + 0] << 24) + (m_pBuffer[nWinRel + 1] << 16) + (m_pBuffer[nWinRel + 2] << 8) +
                (m_pBuffer[nWinRel + 3]));
      }
      else if(nSz == 2)
      {
        return ((m_pBuffer[nWinRel + 0] << 8) + (m_pBuffer[nWinRel + 1]));
      }
      else if(nSz == 1)
      {
        return (m_pBuffer[nWinRel + 0]);
      }
      else
      {
        msgBox.setText("ERROR: BufX() with bad size");
        msgBox.exec();
        return 0;
      }
    }
    else
    {
      if(nSz == 4)
      {
        return ((m_pBuffer[nWinRel + 3] << 24) + (m_pBuffer[nWinRel + 2] << 16) + (m_pBuffer[nWinRel + 1] << 8) +
                (m_pBuffer[nWinRel + 0]));
      }
      else if(nSz == 2)
      {
        return ((m_pBuffer[nWinRel + 1] << 8) + (m_pBuffer[nWinRel + 0]));
      }
      else if(nSz == 1)
      {
        return (m_pBuffer[nWinRel + 0]);
      }
      else
      {
        msgBox.setText("ERROR: BufX() with bad size");
        msgBox.exec();
        return 0;
      }
    }
  }
  else
  {
    // Address is outside of current window
    BufLoadWindow(nOffset);

    // Now we assume that the address is in range
    // m_nBufWinStart has now been updated
    // TODO: Check this
    nWinRel = nOffset - m_nBufWinStart;

    // Now recheck the window
    // TODO: Rewrite the following in a cleaner manner
    if((nWinRel >= 0) && (nWinRel + nSz < m_nBufWinSize))
    {
      if(!nByteSwap)
      {
        if(nSz == 4)
        {
          return ((m_pBuffer[nWinRel + 0] << 24) + (m_pBuffer[nWinRel + 1] << 16) + (m_pBuffer[nWinRel + 2] << 8) +
                  (m_pBuffer[nWinRel + 3]));
        }
        else if(nSz == 2)
        {
          return ((m_pBuffer[nWinRel + 0] << 8) + (m_pBuffer[nWinRel + 1]));
        }
        else if(nSz == 1)
        {
          return (m_pBuffer[nWinRel + 0]);
        }
        else
        {
          msgBox.setText("ERROR: BufX() with bad size");
          msgBox.exec();
          return 0;
        }
      }
      else
      {
        if(nSz == 4)
        {
          return ((m_pBuffer[nWinRel + 3] << 24) + (m_pBuffer[nWinRel + 2] << 16) + (m_pBuffer[nWinRel + 1] << 8) +
                  (m_pBuffer[nWinRel + 0]));
        }
        else if(nSz == 2)
        {
          return ((m_pBuffer[nWinRel + 1] << 8) + (m_pBuffer[nWinRel + 0]));
        }
        else if(nSz == 1)
        {
          return (m_pBuffer[nWinRel + 0]);
        }
        else
        {
          msgBox.setText("ERROR: BufX() with bad size");
          msgBox.exec();
          return 0;
        }
      }
    }
    else
    {
      // Still bad after refreshing window, so it must be bad addr
      m_bBufOK = false;
      // FIXME: Need to report error somehow
      //log->AddLine("ERROR: Overread buffer - file may be truncated"),9);
      return 0;
    }
  }
}

unsigned char CwindowBuf::BufRdAdv1(uint32_t &nOffset, bool bByteSwap)
{
  unsigned char nRet;

  nRet = static_cast < unsigned char >(BufX(nOffset, 1, bByteSwap));

  nOffset += 1;
  return nRet;
}

uint16_t CwindowBuf::BufRdAdv2(uint32_t &nOffset, bool bByteSwap)
{
  uint16_t nRet;

  nRet = static_cast < uint16_t >(BufX(nOffset, 2, bByteSwap));

  nOffset += 2;
  return nRet;
}

uint32_t CwindowBuf::BufRdAdv4(uint32_t &nOffset, bool bByteSwap)
{
  uint32_t nRet;

  nRet = BufX(nOffset, 4, bByteSwap);
  nOffset += 4;
  return nRet;
}

// Read a null-terminated string from the buffer/cache at the
// indicated file offset.
// - Does not affect the current file pointer nPosition
// - String length is limited by encountering either the NULL character
//   of exceeding the maximum length of MAX_BUF_READ_STR
//
// INPUT:
// - nPosition                  File offset to start string fetch
//
// RETURN:
// - String fetched from file
//
QString CwindowBuf::BufReadStr(uint32_t nPosition)
{
  // Try to read a NULL-terminated string from file offset "nPosition"
  // up to a maximum of MAX_BUF_READ_STR bytes. Result is max length MAX_BUF_READ_STR
  QString strRd = "";

  unsigned char cRd;

  bool bDone = false;

  uint32_t nIndex = 0;

  while(!bDone)
  {
    cRd = Buf(nPosition + nIndex);
    // Only add if printable
    if(isprint(cRd))
    {
      strRd += cRd;
    }
    nIndex++;
    if(cRd == 0)
    {
      bDone = true;
    }
    else if(nIndex >= MAX_BUF_READ_STR)
    {
      bDone = true;
      // No need to null-terminate the string since we are using QString
    }
  }
  return strRd;
}

// Read a null-terminated 16-bit unicode string from the buffer/cache at the
// indicated file offset.
// - FIXME: Replace faked out unicode-to-ASCII conversion with real implementation
// - Does not affect the current file pointer nPosition
// - String length is limited by encountering either the NULL character
//   of exceeding the maximum length of MAX_BUF_READ_STR
// - Reference: BUG: #1112
//
// INPUT:
// - nPosition                  File offset to start string fetch
//
// RETURN:
// - String fetched from file
//
QString CwindowBuf::BufReadUniStr(uint32_t nPosition)
{
  // Try to read a NULL-terminated string from file offset "nPosition"
  // up to a maximum of MAX_BUF_READ_STR bytes. Result is max length MAX_BUF_READ_STR
  QString strRd;

  unsigned char cRd;

  bool bDone = false;

  uint32_t nIndex = 0;

  while(!bDone)
  {
    cRd = Buf(nPosition + nIndex);

    // Make sure it is a printable char!
    // FIXME: No, we can't check for this as it will cause
    // _tcslen() call in the calling function to get the wrong
    // length as it isn't null-terminated. Skip for now.
//              if (isprint(cRd)) {
//                      strRd += cRd;
//              } else {
//                      strRd += ".");
//              }
    strRd += cRd;

    nIndex += 2;
    if(cRd == 0)
    {
      bDone = true;
    }
    else if(nIndex >= (MAX_BUF_READ_STR * 2))
    {
      bDone = true;
    }
  }
  return strRd;
}

// Wrapper for ByteStr2Unicode that uses local Window Buffer
#define MAX_UNICODE_STRLEN	255
QString CwindowBuf::BufReadUniStr2(uint32_t nPos, uint32_t nBufLen)
{
  // Convert byte array into unicode string
  // TODO: Replace with call to ByteStr2Unicode()

  bool bByteSwap = false;

  QString strVal;

  uint32_t nStrLenTrunc;

  unsigned char nChVal;

  unsigned char anStrBuf[(MAX_UNICODE_STRLEN + 1) * 2];

  wchar_t acStrBuf[(MAX_UNICODE_STRLEN + 1)];

  // Start with length before any truncation
  nStrLenTrunc = nBufLen;

  // Read unicode bytes into byte array
  // Truncate the string, leaving room for terminator
  if(nStrLenTrunc > MAX_UNICODE_STRLEN)
  {
    nStrLenTrunc = MAX_UNICODE_STRLEN;
  }

  for(uint32_t nInd = 0; nInd < nStrLenTrunc; nInd++)
  {
    if(bByteSwap)
    {
      // Reverse the order of the bytes
      nChVal = Buf(nPos + (nInd * 2) + 0);
      anStrBuf[(nInd * 2) + 1] = nChVal;
      nChVal = Buf(nPos + (nInd * 2) + 1);
      anStrBuf[(nInd * 2) + 0] = nChVal;
    }
    else
    {
      // No byte reversal
      nChVal = Buf(nPos + (nInd * 2) + 0);
      anStrBuf[(nInd * 2) + 0] = nChVal;
      nChVal = Buf(nPos + (nInd * 2) + 1);
      anStrBuf[(nInd * 2) + 1] = nChVal;
    }
  }

  // Ensure it is terminated
  anStrBuf[nStrLenTrunc * 2 + 0] = 0;
  anStrBuf[nStrLenTrunc * 2 + 1] = 0;
  // Copy into unicode string
  // Ensure that it is terminated first!
//  lstrcpyW(acStrBuf, (LPCWSTR) anStrBuf);
  // Copy into QString
  strVal = QString(reinterpret_cast<char *>(anStrBuf));

  return strVal;
}

// Read a string from the buffer/cache at the indicated file offset.
// - Does not affect the current file pointer nPosition
// - String length is limited by encountering either the NULL character
//   of exceeding the maximum length parameter
//
// INPUT:
// - nPosition                  File offset to start string fetch
// - nLen                               Maximum number of bytes to fetch
//
// RETURN:
// - String fetched from file
//
QString CwindowBuf::BufReadStrn(uint32_t nPosition, uint32_t nLen)
{
  // Try to read a fixed-length string from file offset "nPosition"
  // up to a maximum of "nLen" bytes. Result is length "nLen"
  QString strRd = "";

  unsigned char cRd;

  bool bDone = false;

  if(nLen > 0)
  {
    for(uint32_t nInd = 0; ((!bDone) && (nInd < nLen)); nInd++)
    {
      cRd = Buf(nPosition + nInd);
      if(isprint(cRd))
      {
        strRd += cRd;
      }
      if(cRd == char (0))
      {
        bDone = true;
      }
    }
    return strRd;
  }
  else
  {
    return "";
  }
}
