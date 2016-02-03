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
// - This module performs all of the main decoding and processing functions
// - It contains the major context (m_pWBuf, m_pImgDec, m_pJfifDec)
//
// ==========================================================================

#pragma once

#include "JfifDecode.h"
#include "ImgDecode.h"
#include "WindowBuf.h"
#include "SnoopConfig.h"


class CJPEGsnoopCore
{
public:
	CJPEGsnoopCore(void);
	~CJPEGsnoopCore(void);

	void			Reset();

	void			SetStatusBar(CStatusBar* pStatBar);

	BOOL			AnalyzeFile(CString strFname);
	void			AnalyzeFileDo();
	BOOL			AnalyzeOpen();
	void			AnalyzeClose();
	BOOL			IsAnalyzed();
	BOOL			DoAnalyzeOffset(CString strFname);

	void			DoLogSave(CString strLogName);

	void			GenBatchFileList(CString strDirSrc,CString strDirDst,bool bRecSubdir,bool bExtractAll);
	unsigned		GetBatchFileCount();
	void			DoBatchFileProcess(unsigned nFileInd,bool bWriteLog,bool bExtractAll);
	CString			GetBatchFileInfo(unsigned nFileInd);

	void			BuildDirPath(CString strPath);

	void			DoExtractEmbeddedJPEG(CString strInputFname,CString strOutputFname,
							bool bOverlayEn,bool bForceSoi,bool bForceEoi,bool bIgnoreEoi,bool bExtractAllEn,bool bDhtAviInsert,
							CString strOutPath);


	// Accessor wrappers for CjfifDecode
	void			J_GetAviMode(bool &bIsAvi,bool &bIsMjpeg);
	void			J_SetAviMode(bool bIsAvi,bool bIsMjpeg);
	void			J_ImgSrcChanged();
	unsigned long	J_GetPosEmbedStart();
	unsigned long	J_GetPosEmbedEnd();
	void			J_GetDecodeSummary(CString &strHash,CString &strHashRot,CString &strImgExifMake,CString &strImgExifModel,
										CString &strImgQualExif,CString &strSoftware,teDbAdd &eDbReqSuggest);
	unsigned		J_GetDqtZigZagIndex(unsigned nInd,bool bZigZag);
	unsigned		J_GetDqtQuantStd(unsigned nInd);
	void			J_SetStatusBar(CStatusBar* pStatBar);
	void			J_ProcessFile(CFile* inFile);
	void			J_PrepareSendSubmit(CString strQual,teSource eUserSource,CString strUserSoftware,CString strUserNotes);
	
	// Accessor wrappers for CImgDec
	void			I_SetStatusBar(CStatusBar* pStatBar);
	unsigned		I_GetDqtEntry(unsigned nTblDestId, unsigned nCoeffInd);
	void			I_SetPreviewMode(unsigned nMode);
	unsigned		I_GetPreviewMode();
	void			I_SetPreviewYccOffset(unsigned nMcuX,unsigned nMcuY,int nY,int nCb,int nCr);
	void			I_GetPreviewYccOffset(unsigned &nMcuX,unsigned &nMcuY,int &nY,int &nCb,int &nCr);
	void			I_SetPreviewMcuInsert(unsigned nMcuX,unsigned nMcuY,int nLen);
	void			I_GetPreviewMcuInsert(unsigned &nMcuX,unsigned &nMcuY,unsigned &nLen);
	void			I_SetPreviewZoom(bool bInc,bool bDec,bool bSet,unsigned nVal);
	unsigned		I_GetPreviewZoomMode();
	float			I_GetPreviewZoom();
	bool			I_GetPreviewOverlayMcuGrid();
	void			I_SetPreviewOverlayMcuGridToggle();
	CPoint			I_PixelToMcu(CPoint ptPix);
	CPoint			I_PixelToBlk(CPoint ptPix);
	unsigned		I_McuXyToLinear(CPoint ptMcu);
	void			I_GetImageSize(unsigned &nX,unsigned &nY);
	void			I_GetPixMapPtrs(short* &pMapY,short* &pMapCb,short* &pMapCr);
	void			I_GetDetailVlc(bool &bDetail,unsigned &nX,unsigned &nY,unsigned &nLen);
	void			I_SetDetailVlc(bool bDetail,unsigned nX,unsigned nY,unsigned nLen);	
	unsigned		I_GetMarkerCount();
	void			I_SetMarkerBlk(unsigned nBlkX,unsigned nBlkY);
	CPoint			I_GetMarkerBlk(unsigned nInd);
	void			I_SetStatusText(CString strText);
	CString			I_GetStatusYccText();
	void			I_SetStatusYccText(CString strText);
	CString			I_GetStatusMcuText();
	void			I_SetStatusMcuText(CString strText);
	CString			I_GetStatusFilePosText();
	void			I_SetStatusFilePosText(CString strText);
	void			I_GetBitmapPtr(unsigned char* &pBitmap);
	void			I_LookupFilePosMcu(unsigned nMcuX,unsigned nMcuY, unsigned &nByte, unsigned &nBit);
	void			I_LookupFilePosPix(unsigned nPixX,unsigned nPixY, unsigned &nByte, unsigned &nBit);
	void			I_LookupBlkYCC(unsigned nBlkX,unsigned nBlkY,int &nY,int &nCb,int &nCr);
	void			I_ViewOnDraw(CDC* pDC,CRect rectClient,CPoint ptScrolledPos,CFont* pFont, CSize &szNewScrollSize);
	void			I_GetPreviewPos(unsigned &nX,unsigned &nY);
	void			I_GetPreviewSize(unsigned &nX,unsigned &nY);
	bool			I_IsPreviewReady();

	// Accessor wrappers for CwindowBuf
	void			B_SetStatusBar(CStatusBar* pStatBar);
	void			B_BufLoadWindow(unsigned long nPosition);
	void			B_BufFileSet(CFile* inFile);
	void			B_BufFileUnset();
	BYTE			B_Buf(unsigned long nOffset,bool bClean=false);
	bool			B_BufSearch(unsigned long nStartPos, unsigned nSearchVal, unsigned nSearchLen,
						   bool bDirFwd, unsigned long &nFoundPos);
	bool			B_OverlayInstall(unsigned nOvrInd, BYTE* pOverlay,unsigned nLen,unsigned nBegin,
							unsigned nMcuX,unsigned nMcuY,unsigned nMcuLen,unsigned nMcuLenIns,
							int nAdjY,int nAdjCb,int nAdjCr);
	void			B_OverlayRemoveAll();
	bool			B_OverlayGet(unsigned nOvrInd, BYTE* &pOverlay,unsigned &nLen,unsigned &nBegin);

private:

	// Batch processing
	void			GenBatchFileListRecurse(CString strSrcRootName,CString strDstRootName,CString strPathName,bool bSubdirs,bool bExtractAll);
	void			GenBatchFileListSingle(CString strSrcRootName,CString strDstRootName,CString strPathName,bool bExtractAll);


private:

	// Config
	CSnoopConfig*	m_pAppConfig;		// Pointer to application config

	// Input JPEG file
	CFile*			m_pFile;
	ULONGLONG		m_lFileSize;


	CString			m_strPathName;
	BOOL			m_bFileAnalyzed;		// Have we opened and analyzed a file?
	BOOL			m_bFileOpened;			// Is a file currently opened?
	
	// Decoders and Buffers
	CjfifDecode*	m_pJfifDec;
	CimgDecode*		m_pImgDec;
	CwindowBuf*		m_pWBuf;


	// Batch processing
	CStringArray	m_asBatchFiles;
	CStringArray	m_asBatchDest;
	CStringArray	m_asBatchOutputs;

};

