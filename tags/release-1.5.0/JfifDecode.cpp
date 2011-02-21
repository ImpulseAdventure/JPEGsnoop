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

// This file is largely responsible for the decoding of the JPEG JFIF
// marker segments. It does not parse the scan segment as that is
// handled by the ImgDecode.cpp code.

#include "stdafx.h"

#include "JfifDecode.h"
#include "snoop.h"
#include "JPEGsnoop.h" // for m_pAppConfig get

#include "WindowBuf.h"

#include "Md5.h"

#include "afxinet.h"

#include "windows.h"
#include "UrlString.h"
#include "DbSigs.h"

#define MAX_naValues	64			// Max size of decoded value array

// ZigZag DQT coefficient reordering matrix
// as defined by the ITU-T T.81 standard.
const unsigned CjfifDecode::m_sZigZag[64] =
{
	 0, 1, 8,16, 9, 2, 3,10,
	17,24,32,25,18,11, 4, 5,
	12,19,26,33,40,48,41,34,
	27,20,13, 6, 7,14,21,28,
	35,42,49,56,57,50,43,36,
	29,22,15,23,30,37,44,51,
	58,59,52,45,38,31,39,46,
	53,60,61,54,47,55,62,63
};

// Reverse ZigZag reordering matrix
const unsigned CjfifDecode::m_sUnZigZag[64] =
{
	 0, 1, 5, 6,14,15,27,28,
	 2, 4, 7,13,16,26,29,42,
	 3, 8,12,17,25,30,41,43,
	 9,11,18,24,31,40,44,53,
	10,19,23,32,39,45,52,54,
	20,22,33,38,46,51,55,60,
	21,34,37,47,50,56,59,61,
	35,36,48,49,57,58,62,63
};


// This matrix is used to convert a DQT table into its
// rotated form (ie. by 90 degrees), used in the signature
// search functionality.
const unsigned CjfifDecode::m_sQuantRotate[] =
{
	0, 8,16,24,32,40,48,56,
	1, 9,17,25,33,41,49,57,
	2,10,18,26,34,42,50,58,
	3,11,19,27,35,43,51,59,
	4,12,20,28,36,44,52,60,
	5,13,21,29,37,45,53,61,
	6,14,22,30,38,46,54,62,
	7,15,23,31,39,47,55,63,
};

// The ITU-T standard provides some sample quantization
// tables (for luminance and chrominance) that are often
// the basis for many different quantization tables through
// a scaling function.
const unsigned CjfifDecode::m_sStdQuantLum[] =
{ 16, 11, 10, 16, 24, 40, 51, 61,
12, 12, 14, 19, 26, 58, 60, 55,
14, 13, 16, 24, 40, 57, 69, 56,
14, 17, 22, 29, 51, 87, 80, 62,
18, 22, 37, 56, 68,109,103, 77,
24, 35, 55, 64, 81,104,113, 92,
49, 64, 78, 87,103,121,120,101,
72, 92, 95, 98,112,100,103, 99};

const unsigned CjfifDecode::m_sStdQuantChr[] =
{ 17, 18, 24, 47, 99, 99, 99, 99,
18, 21, 26, 66, 99, 99, 99, 99,
24, 26, 56, 99, 99, 99, 99, 99,
47, 66, 99, 99, 99, 99, 99, 99,
99, 99, 99, 99, 99, 99, 99, 99,
99, 99, 99, 99, 99, 99, 99, 99,
99, 99, 99, 99, 99, 99, 99, 99,
99, 99, 99, 99, 99, 99, 99, 99};

// Structure used for each IPTC field
struct iptc_field_t {
	unsigned	id;
	unsigned	fld_type;
	CString		fld_name;
};

// Define all of the currently-known IPTC fields, used when we
// parse the APP13 marker
struct iptc_field_t iptc_fields[] =
{
	{  0,1,"Record Version"},
	{  3,2,"Object Type Ref"},
	{  4,2,"Object Attrib Ref"},
	{  5,2,"Object Name"},
	{  7,2,"Edit Status"},
	{ 10,2,"Urgency"},
	{ 12,2,"Subject Reference"},
	{ 15,2,"Category"},
	{ 20,2,"SuppCategory"},
	{ 22,2,"Fixture ID"},
	{ 25,2,"Keywords"},
	{ 26,2,"Content Location Code"},
	{ 27,2,"Content Location Name"},
	{ 30,2,"ReleaseDate"},
	{ 31,2,"ReleaseTime"},
	{ 37,2,"ExpirationDate"},
	{ 38,2,"ExpirationTime"},
	{ 40,2,"Special Instructions"},
	{ 42,2,"ActionAdvised"},
	{ 45,2,"ReferenceService"},
	{ 47,2,"ReferenceDate"},
	{ 50,2,"ReferenceNumber"},
	{ 55,2,"DateCreated"},
	{ 60,2,"TimeCreated"},
	{ 62,2,"DigitalCreationDate"},
	{ 63,2,"DigitalCreationTime"},
	{ 65,2,"OriginatingProgram"},
	{ 70,2,"ProgramVersion"},
	{ 75,2,"ObjectCycle"},
	{ 80,2,"By-line"},
	{ 85,2,"By-line Title"},
	{ 90,2,"City"},
	{ 92,2,"Sub-location"},
	{ 95,2,"Province-State"},
	{100,2,"CountryCode"},
	{101,2,"CountryName"},
	{103,2,"OriginalTransmissionRef"},
	{105,2,"Headline"},
	{110,2,"Credit"},
	{115,2,"Source"},
	{116,2,"CopyrightNotice"},
	{118,2,"Contact"},
	{120,2,"Caption-Abstract"},
	{130,2,"ImageType"},
	{131,2,"ImageOrientation"},
	{135,2,"LanguageID"},
	{999,9,"DONE"},
};


// Clear out internal members
void CjfifDecode::Reset()
{
	// File handling
	m_nPos = 0;
	m_nPosEoi = 0;

	m_bImgOK = false;		// Set during SOF to indicate further proc OK
	m_bAvi = false;
	m_bAviMjpeg = false;

	strcpy(m_strApp0Identifier,"");
	m_strSoftware = "";
	m_strHash = "NONE";
	m_strHashRot = "NONE";
	m_nImgEdited = EDITED_UNSET;
	m_nDbReqSuggest = DB_ADD_SUGGEST_UNSET;

	m_bSigExactInDB = false;

	m_bBufFakeDHT = false;			// Start in normal Buf mode

	m_bImgProgressive = false;		// Assume not m_bImgProgressive scan

	m_strComment = "";

	ClearDQT();

	// Photoshop
	m_nImgQualPhotoshopSfw = 0;
	m_nImgQualPhotoshopSa = 0;

	m_nImgRstEn = false;
	m_nImgRstInterval = 0;

	m_nImgNumLines = 0;
	m_nImgSampsPerLine = 0;

	m_nImgSofCompNum = 0;

	m_strImgExifMake = "???";
	m_nImgExifMakeSubtype = 0;
	m_strImgExifModel = "???";
	m_bImgExifMakernotes = false;
	m_strImgExtras = "";

	m_nImgLandscape = ENUM_LANDSCAPE_UNSET;
	m_strImgQualExif = "";



	// Embedded thumbnail
	m_nImgExifThumbComp = 0;
	m_nImgExifThumbOffset = 0;
	m_nImgExifThumbLen = 0;
	m_strHashThumb = "NONE";		// Will go into DB to say NONE!
	m_strHashThumbRot = "NONE";		// Will go into DB to say NONE!
	m_nImgThumbNumLines = 0;
	m_nImgThumbSampsPerLine = 0;

	// Now clear out any previously generated bitmaps
	// or image decoding parameters
	if (m_pImgDec) {
		if (m_pImgSrcDirty) {
			m_pImgDec->Reset();
		}
	}

	// Reset the decoding state checks
	// These are to help ensure we don't start decoding SOS
	// if we haven't seen other valid markers yet! Otherwise
	// we could run into very bad loops (e.g. .PSD files)
	// just because we saw FFD8FF first then JFIF_SOS
	m_bStateAbort	= false;
	m_bStateSoi		= false;
	m_bStateDht		= false;
	m_bStateDhtOk	= false;
	m_bStateDhtFake = false;
	m_bStateDqt		= false;
	m_bStateDqtOk	= false;
	m_bStateSof		= false;
	m_bStateSofOk	= false;
	m_bStateSos		= false;
	m_bStateSosOk	= false;
	m_bStateEoi		= false;

	m_nPosEmbedStart = 0;
	m_nPosEmbedEnd = 0;

	m_nPosFileEnd = 0;

	m_nPosSos = 0;

}


// Initialize the JFIF decoder. Several class pointers are provided
// as parameters, so that we can directly access the output log, the
// file buffer (CwindowBuf) and the Image scan decoder (CimgDecode).
//
// PRE: pLog, pWBuf, pImgDec are already initialized
CjfifDecode::CjfifDecode(CDocLog* pLog,CwindowBuf* pWBuf,CimgDecode* pImgDec)
{

	// Need to zero out all of the private members!
	bOutputWeb = FALSE;
	bOutputExcel = FALSE;
	bOutputDB = FALSE;		// mySQL output for web

	// Enable verbose reporting
	bVerbose = FALSE;

	m_pImgSrcDirty = TRUE;

	// Generate lookup tables for Huffman codes
	HuffMaskLookupGen();

	// Window status bar is not ready yet, wait for call to SetStatusBar()
	m_pStatBar = NULL;

	// Save copies of other class pointers
	m_pLog = pLog;
	m_pWBuf = pWBuf;
	m_pImgDec = pImgDec;

	// Ideally this would be passed by constructor, but simply access
	// directly for now.
	CJPEGsnoopApp*	pApp;
	pApp = (CJPEGsnoopApp*)AfxGetApp();
    m_pAppConfig = pApp->m_pAppConfig;

	// Reset decoding state
	Reset();

	// Load the local database (if it exists)
	theApp.m_pDbSigs->DatabaseExtraLoad();

}

CjfifDecode::~CjfifDecode()
{
}

// Asynchronously update a local pointer to the status bar once
// it becomes available. It was not ready at time of the CjfifDecode
// class constructor call.
void CjfifDecode::SetStatusBar(CStatusBar* pStatBar)
{
	m_pStatBar = pStatBar;
}


// Indicate that the source of the image scan data
// has been dirtied. Either the source has changed
// or some of the View2 options have changed.
void CjfifDecode::ImgSrcChanged()
{
	m_pImgSrcDirty = true;
}

// Reset the DQT tables in memory
void CjfifDecode::ClearDQT()
{
	for (unsigned set=0;set<4;set++)
	{
		for (unsigned ind=0;ind<64;ind++)
		{
			m_anImgDqt[set][ind] = 0;
			m_anImgThumbDqt[set][ind] = 0;
		}
		m_adImgDqtQual[set] = 0;
		m_bImgDqtSet[set] = false;
		m_bImgDqtThumbSet[set] = false;
	}
}

void CjfifDecode::SetDQTQuick(unsigned dqt0[64],unsigned dqt1[64])
{
	m_nImgLandscape = ENUM_LANDSCAPE_YES;
	for (unsigned ind=0;ind<64;ind++)
	{
		m_anImgDqt[0][ind] = dqt0[ind];
		m_anImgDqt[1][ind] = dqt1[ind];
	}
	m_bImgDqtSet[0] = true;
	m_bImgDqtSet[1] = true;
	m_strImgQuantCss = "NA";
}

// This routine builds a lookup table for the Huffman code masks
// The result is a simple bit sequence of zeros followed by
// an increasing number of 1 bits.
//   00000000...00000001
//   00000000...00000011
//   00000000...00000111
//   ...
//   01111111...11111111
//   11111111...11111111
void CjfifDecode::HuffMaskLookupGen()
{
	unsigned int mask;
	for (unsigned len=0;len<32;len++)
	{
		mask = (1 << (len))-1;
		mask <<= 32-len;
		m_anMaskLookup[len] = mask;
	}
}


// Throughout the code, we are using Buf() as the means of reading
// in the next byte from the file. This call normally
// calls the WindowBuf Buf() call directly, but it redirects the
// file reading to a local table in case we are faking out the DHT
// (eg. for MotionJPEG files). This routine simply provides a shorthand
// accessor function for the CwindowBuf.Buf()
//
// IN:     offset = file offset to read
// IN:     bClean = forcibly disables any redirection to Fake DHT table
// RETURN: byte from file (or local table)
BYTE CjfifDecode::Buf(unsigned long offset,bool bClean=false)
{
	// Buffer can be redirected to internal array for AVI DHT
	// tables, so check for it here.
	if (m_bBufFakeDHT) {
		return m_abMJPGDHTSeg[offset];
	} else {
		return m_pWBuf->Buf(offset,bClean);
	}
}

// Write out a line to the log buffer if we are in verbose mode
void CjfifDecode::DbgAddLine(const char* strLine)
{
	if (bVerbose)
	{
		m_pLog->AddLine(strLine);
	}
}

// Take 32-bit value and decompose into 4 bytes, but support
// both byte endian bytes
void CjfifDecode::UnByteSwap4(unsigned nVal,unsigned &nByte0,unsigned &nByte1,unsigned &nByte2,unsigned &nByte3)
{
	if (m_nImgExifEndian == 0) {
		// Little Endian
		nByte3 = (nVal & 0xFF000000) >> 24;
		nByte2 = (nVal & 0x00FF0000) >> 16;
		nByte1 = (nVal & 0x0000FF00) >> 8;
		nByte0 = (nVal & 0x000000FF);
	} else {
		// Big Endian
		nByte0 = (nVal & 0xFF000000) >> 24;
		nByte1 = (nVal & 0x00FF0000) >> 16;
		nByte2 = (nVal & 0x0000FF00) >> 8;
		nByte3 = (nVal & 0x000000FF);
	}
}

// Convert from 4 unsigned bytes into a 32-bit dword, with
// support for endian byte-swapping
unsigned CjfifDecode::ByteSwap4(unsigned b0,unsigned b1, unsigned b2, unsigned b3)
{
	unsigned val;

	//CAL! Note: FUJIFILM Maker notes apparently use
	// big endian format even though main section uses little.
	// Need to switch here if in maker section for FUJI
	if (m_nImgExifEndian == 0) {
		// Little endian, byte swap required
		val = (b3<<24) + (b2<<16) + (b1<<8) + b0;
	} else {
		// Big endian, no swap required
		val = (b0<<24) + (b1<<16) + (b2<<8) + b3;
	}
	return val;
}

// Perform conversion from 2 bytes into half-word with
// endian byte-swapping support
unsigned CjfifDecode::ByteSwap2(unsigned b0,unsigned b1)
{
	unsigned val;
	if (m_nImgExifEndian == 0) {
		// Little endian, byte swap required
		val = (b1<<8) + b0;
	} else {
		// Big endian, no swap required
		val = (b0<<8) + b1;
	}
	return val;
}

// Decode Canon Makernotes
// Only the most common makernotes are supported; there are a large
// number of makernotes that have not been documented anywhere.
CStr2 CjfifDecode::LookupMakerCanonTag(unsigned mainTag,unsigned subTag,unsigned val)
{
	CString tmpStr;
	CStr2 retval;

	retval.tag = "???";
	retval.bUnknown = false;		// Set to true in default clauses
	retval.val.Format(_T("%u"),val); // Provide default value

	unsigned valHi,valLo;
	valHi = (val & 0xff00) >> 8;
	valLo = (val & 0x00ff);

	switch(mainTag)
	{

	case 0x0001:

		switch(subTag)
		{
		case 0x0001: retval.tag = "Canon.Cs1.Macro";break; // Short Macro mode 
		case 0x0002: retval.tag = "Canon.Cs1.Selftimer";break; // Short Self timer 
		case 0x0003: retval.tag = "Canon.Cs1.Quality";
			if (val == 2) { retval.val = "norm"; }
			else if (val == 3) { retval.val = "fine"; }
			else if (val == 5) { retval.val = "superfine"; }
			else {
				retval.val = "?";
			}
			// Save the quality string for later
			m_strImgQualExif = retval.val;
			break; // Short Quality 
		case 0x0004: retval.tag = "Canon.Cs1.FlashMode";break; // Short Flash mode setting 
		case 0x0005: retval.tag = "Canon.Cs1.DriveMode";break; // Short Drive mode setting 
		case 0x0007: retval.tag = "Canon.Cs1.FocusMode"; // Short Focus mode setting 
			switch(val) {
				case 0 : retval.val = "One-shot";break;
				case 1 : retval.val = "AI Servo";break;
				case 2 : retval.val = "AI Focus";break;
				case 3 : retval.val = "Manual Focus";break;
				case 4 : retval.val = "Single";break;
				case 5 : retval.val = "Continuous";break;
				case 6 : retval.val = "Manual Focus";break;
				default : retval.val = "?";break;
			}
			break;
		case 0x000a: retval.tag = "Canon.Cs1.ImageSize"; // Short Image size 
			if (val == 0) { retval.val = "Large"; }
			else if (val == 1) { retval.val = "Medium"; }
			else if (val == 2) { retval.val = "Small"; }
			else {
				retval.val = "?";
			}
			break;
		case 0x000b: retval.tag = "Canon.Cs1.EasyMode";break; // Short Easy shooting mode 
		case 0x000c: retval.tag = "Canon.Cs1.DigitalZoom";break; // Short Digital zoom 
		case 0x000d: retval.tag = "Canon.Cs1.Contrast";break; // Short Contrast setting 
		case 0x000e: retval.tag = "Canon.Cs1.Saturation";break; // Short Saturation setting 
		case 0x000f: retval.tag = "Canon.Cs1.Sharpness";break; // Short Sharpness setting 
		case 0x0010: retval.tag = "Canon.Cs1.ISOSpeed";break; // Short ISO speed setting 
		case 0x0011: retval.tag = "Canon.Cs1.MeteringMode";break; // Short Metering mode setting 
		case 0x0012: retval.tag = "Canon.Cs1.FocusType";break; // Short Focus type setting 
		case 0x0013: retval.tag = "Canon.Cs1.AFPoint";break; // Short AF point selected 
		case 0x0014: retval.tag = "Canon.Cs1.ExposureProgram";break; // Short Exposure mode setting 
		case 0x0016: retval.tag = "Canon.Cs1.LensType";break; // 
		case 0x0017: retval.tag = "Canon.Cs1.Lens";break; // Short 'long' and 'short' focal length of lens (in 'focal m_nImgUnits') and 'focal m_nImgUnits' per mm 
		case 0x001a: retval.tag = "Canon.Cs1.MaxAperture";break; // 
		case 0x001b: retval.tag = "Canon.Cs1.MinAperture";break; // 
		case 0x001c: retval.tag = "Canon.Cs1.FlashActivity";break; // Short Flash activity 
		case 0x001d: retval.tag = "Canon.Cs1.FlashDetails";break; // Short Flash details 
		case 0x0020: retval.tag = "Canon.Cs1.FocusMode";break; // Short Focus mode setting 
		default:
			retval.tag.Format(_T("Canon.Cs1.x%04X"),subTag);
			retval.bUnknown = true;
			break;
		} // switch subTag
		break;

	case 0x0004:

		switch(subTag)
		{
		case 0x0002: retval.tag = "Canon.Cs2.ISOSpeed";break; // Short ISO speed used 
		case 0x0004: retval.tag = "Canon.Cs2.TargetAperture";break; // Short Target Aperture 
		case 0x0005: retval.tag = "Canon.Cs2.TargetShutterSpeed";break; // Short Target shutter speed 
		case 0x0007: retval.tag = "Canon.Cs2.WhiteBalance";break; // Short White balance setting 
		case 0x0009: retval.tag = "Canon.Cs2.Sequence";break; // Short Sequence number (if in a continuous burst) 
		case 0x000e: retval.tag = "Canon.Cs2.AFPointUsed";break; // Short AF point used 
		case 0x000f: retval.tag = "Canon.Cs2.FlashBias";break; // Short Flash bias 
		case 0x0013: retval.tag = "Canon.Cs2.SubjectDistance";break; // Short Subject distance (m_nImgUnits are not clear) 
		case 0x0015: retval.tag = "Canon.Cs2.ApertureValue";break; // Short Aperture 
		case 0x0016: retval.tag = "Canon.Cs2.ShutterSpeedValue";break; // Short Shutter speed 
		default:
			retval.tag.Format(_T("Canon.Cs2.x%04X"),subTag);
			retval.bUnknown = true;
			break;
		} // switch subTag
		break;

	case 0x000F:

		// CustomFunctions are different!! Tag given by high byte, value by low!
		// Index order (usually the subTag) is not used.
//		switch(subTag)
		retval.val.Format(_T("%u"),valLo); // Provide default value
		switch(valHi)
		{

		case 0x0001: retval.tag = "Canon.Cf.NoiseReduction";break; // Short Long exposure noise reduction 
		case 0x0002: retval.tag = "Canon.Cf.ShutterAeLock";break; // Short Shutter/AE lock buttons 
		case 0x0003: retval.tag = "Canon.Cf.MirrorLockup";break; // Short Mirror lockup 
		case 0x0004: retval.tag = "Canon.Cf.ExposureLevelIncrements";break; // Short Tv/Av and exposure level 
		case 0x0005: retval.tag = "Canon.Cf.AFAssist";break; // Short AF assist light 
		case 0x0006: retval.tag = "Canon.Cf.FlashSyncSpeedAv";break; // Short Shutter speed in Av mode 
		case 0x0007: retval.tag = "Canon.Cf.AEBSequence";break; // Short AEB sequence/auto cancellation 
		case 0x0008: retval.tag = "Canon.Cf.ShutterCurtainSync";break; // Short Shutter curtain sync 
		case 0x0009: retval.tag = "Canon.Cf.LensAFStopButton";break; // Short Lens AF stop button Fn. Switch 
		case 0x000a: retval.tag = "Canon.Cf.FillFlashAutoReduction";break; // Short Auto reduction of fill flash 
		case 0x000b: retval.tag = "Canon.Cf.MenuButtonReturn";break; // Short Menu button return position 
		case 0x000c: retval.tag = "Canon.Cf.SetButtonFunction";break; // Short SET button func. when shooting 
		case 0x000d: retval.tag = "Canon.Cf.SensorCleaning";break; // Short Sensor cleaning 
		case 0x000e: retval.tag = "Canon.Cf.SuperimposedDisplay";break; // Short Superimposed display 
		case 0x000f: retval.tag = "Canon.Cf.ShutterReleaseNoCFCard";break; // Short Shutter Release W/O CF Card 
		default:
			retval.tag.Format(_T("Canon.Cf.x%04X"),valHi);
			retval.bUnknown = true;
			break;
		} // switch subTag
		break;

/*
	// Other ones assumed to use high-byte/low-byte method:
	case 0x00C0:
		retval.val.Format(_T("%u"),valLo); // Provide default value
		switch(valHi)
		{
			//case 0x0001: retval.tag = "Canon.x00C0.???";break; //
			default:
				retval.tag.Format(_T("Canon.x00C0.x%04X"),valHi);
				break;
		}
		break;

	case 0x00C1:
		retval.val.Format(_T("%u"),valLo); // Provide default value
		switch(valHi)
		{
			//case 0x0001: retval.tag = "Canon.x00C1.???";break; //
			default:
				retval.tag.Format(_T("Canon.x00C1.x%04X"),valHi);
				break;
		}
		break;
*/

	case 0x0012:

		switch(subTag)
		{
		case 0x0002: retval.tag = "Canon.Pi.ImageWidth";break; //
		case 0x0003: retval.tag = "Canon.Pi.ImageHeight";break; //
		case 0x0004: retval.tag = "Canon.Pi.ImageWidthAsShot";break; //
		case 0x0005: retval.tag = "Canon.Pi.ImageHeightAsShot";break; //
		case 0x0016: retval.tag = "Canon.Pi.AFPointsUsed";break; //
		case 0x001a: retval.tag = "Canon.Pi.AFPointsUsed20D";break; //
		default:
			retval.tag.Format(_T("Canon.Pi.x%04X"),subTag);
			retval.bUnknown = true;
			break;
		} // switch subTag
		break;

	default:
		retval.tag.Format(_T("Canon.x%04X.x%04X"),mainTag,subTag);
		retval.bUnknown = true;
		break;

	} // switch mainTag

	return retval;
}


// Perform decode of EXIF IFD tags
CString CjfifDecode::LookupExifTag(CString sect,unsigned tag,bool &bUnknown)
{
	CString tmpStr;
	bUnknown = false;

	if (sect == "IFD0")
	{

		switch(tag)
		{

		case 0x010E: return _T("ImageDescription");break; // ascii string Describes image 
		case 0x010F: return _T("Make");break; // ascii string Shows manufacturer of digicam 
		case 0x0110: return _T("Model");break; // ascii string Shows model number of digicam
		case 0x0112: return _T("Orientation");break; // unsigned short 1  The orientation of the camera relative to the scene, when the image was captured. The start point of stored data is, '1' means upper left, '3' lower right, '6' upper right, '8' lower left, '9' undefined. 
		case 0x011A: return _T("XResolution");break; // unsigned rational 1  Display/Print resolution of image. Large number of digicam uses 1/72inch, but it has no mean because personal computer doesn't use this value to display/print out. 
		case 0x011B: return _T("YResolution");break; // unsigned rational 1  
		case 0x0128: return _T("ResolutionUnit");break; // unsigned short 1  Unit of XResolution(0x011a)/YResolution(0x011b). '1' means no-unit, '2' means inch, '3' means centimeter. 
		case 0x0131: return _T("Software");break; //  ascii string Shows firmware(internal software of digicam) version number. 
		case 0x0132: return _T("DateTime");break; // ascii string 20  Date/Time of image was last modified. Data format is "YYYY:MM:DD HH:MM:SS"+0x00, total 20bytes. In usual, it has the same value of DateTimeOriginal(0x9003) 
		case 0x013B: return _T("Artist");break; // Seems to be here and not only in SubIFD (maybe instead of SubIFD)
		case 0x013E: return _T("WhitePoint");break; // unsigned rational 2  Defines chromaticity of white point of the image. If the image uses CIE Standard Illumination D65(known as international standard of 'daylight'), the values are '3127/10000,3290/10000'. 
		case 0x013F: return _T("PrimChromaticities");break; // unsigned rational 6  Defines chromaticity of the primaries of the image. If the image uses CCIR Recommendation 709 primearies, values are '640/1000,330/1000,300/1000,600/1000,150/1000,0/1000'. 
		case 0x0211: return _T("YCbCrCoefficients");break; // unsigned rational 3  When image format is YCbCr, this value shows a constant to translate it to RGB format. In usual, values are '0.299/0.587/0.114'. 
		case 0x0213: return _T("YCbCrPositioning");break; // unsigned short 1  When image format is YCbCr and uses 'Subsampling'(cropping of chroma data, all the digicam do that), defines the chroma sample point of subsampling pixel array. '1' means the center of pixel array, '2' means the datum point. 
		case 0x0214: return _T("ReferenceBlackWhite");break; // unsigned rational 6  Shows reference value of black point/white point. In case of YCbCr format, first 2 show black/white of Y, next 2 are Cb, last 2 are Cr. In case of RGB format, first 2 show black/white of R, next 2 are G, last 2 are B.
		case 0x8298: return _T("Copyright");break; // ascii string Shows copyright information
		case 0x8769: return _T("ExifOffset");break; //unsigned long 1  Offset to Exif Sub IFD
		case 0x8825: return _T("GPSOffset");break; //unsigned long 1  Offset to Exif GPS IFD
//NEW:
		case 0x9C9B: return _T("XPTitle");break;
		case 0x9C9C: return _T("XPComment");break;
		case 0x9C9D: return _T("XPAuthor");break;
		case 0x9C9e: return _T("XPKeywords");break;
		case 0x9C9f: return _T("XPSubject");break;
//NEW: The following were found in IFD0 even though they should just be SubIFD???
		case 0xA401: return _T("CustomRendered");break;
		case 0xA402: return _T("ExposureMode");break;
		case 0xA403: return _T("WhiteBalance");break;
		case 0xA406: return _T("SceneCaptureType");break;

		default:
			tmpStr.Format(_T("IFD0.0x%04X"),tag);
			bUnknown = true;
			return tmpStr;
			break;
		}

	} else if (sect == "SubIFD") {

		switch(tag)
		{
		case 0x00fe: return _T("NewSubfileType");break; //  unsigned long 1  
		case 0x00ff: return _T("SubfileType");break; //  unsigned short 1   
		case 0x012d: return _T("TransferFunction");break; //  unsigned short 3  
		case 0x013b: return _T("Artist");break; //  ascii string 
		case 0x013d: return _T("Predictor");break; //  unsigned short 1  
		case 0x0142: return _T("TileWidth");break; //  unsigned short 1  
		case 0x0143: return _T("TileLength");break; //  unsigned short 1  
		case 0x0144: return _T("TileOffsets");break; //  unsigned long 
		case 0x0145: return _T("TileByteCounts");break; //  unsigned short 
		case 0x014a: return _T("SubIFDs");break; //  unsigned long 
		case 0x015b: return _T("JPEGTables");break; //  undefined 
		case 0x828d: return _T("CFARepeatPatternDim");break; //  unsigned short 2  
		case 0x828e: return _T("CFAPattern");break; //  unsigned byte 
		case 0x828f: return _T("BatteryLevel");break; //  unsigned rational 1  
		case 0x829A: return _T("ExposureTime");break;
		case 0x829D: return _T("FNumber");break;
		case 0x83bb: return _T("IPTC/NAA");break; //  unsigned long 
		case 0x8773: return _T("InterColorProfile");break; //  undefined 
		case 0x8822: return _T("ExposureProgram");break;
		case 0x8824: return _T("SpectralSensitivity");break; //  ascii string 
		case 0x8825: return _T("GPSInfo");break; //  unsigned long 1  
		case 0x8827: return _T("ISOSpeedRatings");break;
		case 0x8828: return _T("OECF");break; //  undefined 
		case 0x8829: return _T("Interlace");break; //  unsigned short 1  
		case 0x882a: return _T("TimeZoneOffset");break; //  signed short 1  
		case 0x882b: return _T("SelfTimerMode");break; //  unsigned short 1  
		case 0x9000: return _T("ExifVersion");break;
		case 0x9003: return _T("DateTimeOriginal");break;
		case 0x9004: return _T("DateTimeDigitized");break;
		case 0x9101: return _T("ComponentsConfiguration");break;
		case 0x9102: return _T("CompressedBitsPerPixel");break;
		case 0x9201: return _T("ShutterSpeedValue");break;
		case 0x9202: return _T("ApertureValue");break;
		case 0x9203: return _T("BrightnessValue");break;
		case 0x9204: return _T("ExposureBiasValue");break;
		case 0x9205: return _T("MaxApertureValue");break;
		case 0x9206: return _T("SubjectDistance");break;
		case 0x9207: return _T("MeteringMode");break;
		case 0x9208: return _T("LightSource");break;
		case 0x9209: return _T("Flash");break;
		case 0x920A: return _T("FocalLength");break;
		case 0x920b: return _T("FlashEnergy");break; //  unsigned rational 1  
		case 0x920c: return _T("SpatialFrequencyResponse");break; //  undefined 
		case 0x920d: return _T("Noise");break; //  undefined 
		case 0x9211: return _T("ImageNumber");break; //  unsigned long 1  
		case 0x9212: return _T("SecurityClassification");break; //  ascii string 1  
		case 0x9213: return _T("ImageHistory");break; //  ascii string 
		case 0x9214: return _T("SubjectLocation");break; //  unsigned short 4  
		case 0x9215: return _T("ExposureIndex");break; //  unsigned rational 1  
		case 0x9216: return _T("TIFF/EPStandardID");break; //  unsigned byte 4  
		case 0x927C: return _T("MakerNote");break;
		case 0x9286: return _T("UserComment");break;
		case 0x9290: return _T("SubSecTime");break; //  ascii string 
		case 0x9291: return _T("SubSecTimeOriginal");break; //  ascii string 
		case 0x9292: return _T("SubSecTimeDigitized");break; //  ascii string 
		case 0xA000: return _T("FlashPixVersion");break;
		case 0xA001: return _T("ColorSpace");break;
		case 0xA002: return _T("ExifImageWidth");break;
		case 0xA003: return _T("ExifImageHeight");break;
		case 0xA004: return _T("RelatedSoundFile");break;
		case 0xA005: return _T("ExifInteroperabilityOffset");break;
		case 0xa20b: return _T("FlashEnergy  unsigned");break; // rational 1  
		case 0xa20c: return _T("SpatialFrequencyResponse");break; //  unsigned short 1  
		case 0xA20E: return _T("FocalPlaneXResolution");break;
		case 0xA20F: return _T("FocalPlaneYResolution");break;
		case 0xA210: return _T("FocalPlaneResolutionUnit");break;
		case 0xa214: return _T("SubjectLocation");break; //  unsigned short 1  
		case 0xa215: return _T("ExposureIndex");break; //  unsigned rational 1 
		case 0xA217: return _T("SensingMethod");break;
		case 0xA300: return _T("FileSource");break;
		case 0xA301: return _T("SceneType");break;
		case 0xa302: return _T("CFAPattern");break; //  undefined 1  
		case 0xa401: return _T("CustomRendered");break; // Short Custom image processing 
		case 0xa402: return _T("ExposureMode");break; // Short Exposure mode 
		case 0xa403: return _T("WhiteBalance");break; // Short White balance 
		case 0xa404: return _T("DigitalZoomRatio");break; // Rational Digital zoom ratio 
		case 0xa405: return _T("FocalLengthIn35mmFilm");break; // Short Focal length in 35 mm film 
		case 0xa406: return _T("SceneCaptureType");break; // Short Scene capture type 
		case 0xa407: return _T("GainControl");break; // Rational Gain control 
		case 0xa408: return _T("Contrast");break; // Short Contrast 
		case 0xa409: return _T("Saturation");break; // Short Saturation 
		case 0xa40a: return _T("Sharpness");break; // Short Sharpness 
		case 0xa40b: return _T("DeviceSettingDescription");break; // Undefined Device settings description 
		case 0xa40c: return _T("SubjectDistanceRange");break; // Short Subject distance range 
		case 0xa420: return _T("ImageUniqueID");break; // Ascii Unique image ID 

		default:
			tmpStr.Format(_T("SubIFD.0x%04X"),tag);
			bUnknown = true;
			return tmpStr;
			break;

		}

	} else if (sect == "IFD1") {

		switch(tag)
		{
		case 0x0100: return _T("ImageWidth");break; //  unsigned short/long 1  Shows size of thumbnail image. 
		case 0x0101: return _T("ImageLength");break; //  unsigned short/long 1  
		case 0x0102: return _T("BitsPerSample");break; //  unsigned short 3  When image format is no compression, this value shows the number of bits per component for each pixel. Usually this value is '8,8,8' 
		case 0x0103: return _T("Compression");break; //  unsigned short 1  Shows compression method. '1' means no compression, '6' means JPEG compression. 
		case 0x0106: return _T("PhotometricInterpretation");break; //  unsigned short 1  Shows the color space of the image data components. '1' means monochrome, '2' means RGB, '6' means YCbCr. 
		case 0x0111: return _T("StripOffsets");break; //  unsigned short/long When image format is no compression, this value shows offset to image data. In some case image data is striped and this value is plural. 
		case 0x0115: return _T("SamplesPerPixel");break; //  unsigned short 1  When image format is no compression, this value shows the number of components stored for each pixel. At color image, this value is '3'. 
		case 0x0116: return _T("RowsPerStrip");break; //  unsigned short/long 1  When image format is no compression and image has stored as strip, this value shows how many rows stored to each strip. If image has not striped, this value is the same as ImageLength(0x0101). 
		case 0x0117: return _T("StripByteConunts");break; //  unsigned short/long  When image format is no compression and stored as strip, this value shows how many bytes used for each strip and this value is plural. If image has not stripped, this value is single and means whole data size of image. 
		case 0x011a: return _T("XResolution");break; //  unsigned rational 1  Display/Print resolution of image. Large number of digicam uses 1/72inch, but it has no mean because personal computer doesn't use this value to display/print out. 
		case 0x011b: return _T("YResolution");break; //  unsigned rational 1  
		case 0x011c: return _T("PlanarConfiguration");break; //  unsigned short 1  When image format is no compression YCbCr, this value shows byte aligns of YCbCr data. If value is '1', Y/Cb/Cr value is chunky format, contiguous for each subsampling pixel. If value is '2', Y/Cb/Cr value is separated and stored to Y plane/Cb plane/Cr plane format. 
		case 0x0128: return _T("ResolutionUnit");break; //  unsigned short 1  Unit of XResolution(0x011a)/YResolution(0x011b). '1' means inch, '2' means centimeter. 
		case 0x0201: return _T("JpegIFOffset");break; //  unsigned long 1  When image format is JPEG, this value show offset to JPEG data stored. 
		case 0x0202: return _T("JpegIFByteCount");break; //  unsigned long 1  When image format is JPEG, this value shows data size of JPEG image. 
		case 0x0211: return _T("YCbCrCoefficients");break; //  unsigned rational 3  When image format is YCbCr, this value shows constants to translate it to RGB format. In usual, '0.299/0.587/0.114' are used. 
		case 0x0212: return _T("YCbCrSubSampling");break; //  unsigned short 2  When image format is YCbCr and uses subsampling(cropping of chroma data, all the digicam do that), this value shows how many chroma data subsampled. First value shows horizontal, next value shows vertical subsample rate. 
		case 0x0213: return _T("YCbCrPositioning");break; //  unsigned short 1  When image format is YCbCr and uses 'Subsampling'(cropping of chroma data, all the digicam do that), this value defines the chroma sample point of subsampled pixel array. '1' means the center of pixel array, '2' means the datum point(0,0). 
		case 0x0214: return _T("ReferenceBlackWhite");break; //  unsigned rational 6  Shows reference value of black point/white point. In case of YCbCr format, first 2 show black/white of Y, next 2 are Cb, last 2 are Cr. In case of RGB format, first 2 show black/white of R, next 2 are G, last 2 are B. 

		default:
			tmpStr.Format(_T("IFD1.0x%04X"),tag);
			bUnknown = true;
			return tmpStr;
			break;

		}

	} else if (sect == "InteropIFD") {
		switch(tag) {
			case 0x0001: return _T("InteroperabilityIndex");break;
			case 0x0002: return _T("InteroperabilityVersion");break;
			case 0x1000: return _T("RelatedImageFileFormat");break;
			case 0x1001: return _T("RelatedImageWidth");break;
			case 0x1002: return _T("RelatedImageLength");break;

			default:
				tmpStr.Format(_T("Interop.0x%04X"),tag);
				bUnknown = true;
				return tmpStr;
				break;
		}
	} else if (sect == "GPSIFD") {
		switch(tag) {
			case 0x0000: return _T("GPSVersionID");break;
			case 0x0001: return _T("GPSLatitudeRef");break;
			case 0x0002: return _T("GPSLatitude");break;
			case 0x0003: return _T("GPSLongitudeRef");break;
			case 0x0004: return _T("GPSLongitude");break;
			case 0x0005: return _T("GPSAltitudeRef");break;
			case 0x0006: return _T("GPSTimeStamp");break;
			case 0x0007: return _T("GPSSatellites");break;
			case 0x0008: return _T("GPSAltitude");break;
			case 0x0009: return _T("GPSStatus");break;
			case 0x000A: return _T("GPSMeasureMode");break;
			case 0x000B: return _T("GPSDOP");break;
			case 0x000C: return _T("GPSSpeedRef");break;
			case 0x000D: return _T("GPSSpeed");break;
			case 0x000E: return _T("GPSTrackRef");break;
			case 0x000F: return _T("GPSTrack");break;
			case 0x0010: return _T("GPSImgDirectionRef");break;
			case 0x0011: return _T("GPSImgDirection");break;
			case 0x0012: return _T("GPSMapDatum");break;
			case 0x0013: return _T("GPSDestLatitudeRef");break;
			case 0x0014: return _T("GPSDestLatitude");break;
			case 0x0015: return _T("GPSDestLongitudeRef");break;
			case 0x0016: return _T("GPSDestLongitude");break;
			case 0x0017: return _T("GPSDestBearingRef");break;
			case 0x0018: return _T("GPSDestBearing");break;
			case 0x0019: return _T("GPSDestDistanceRef");break;
			case 0x001A: return _T("GPSDestDistance");break;
			case 0x001B: return _T("GPSProcessingMethod");break;
			case 0x001C: return _T("GPSAreaInformation");break;
			case 0x001D: return _T("GPSDateStamp");break;
			case 0x001E: return _T("GPSDifferential");break;

			default:
				tmpStr.Format(_T("GPS.0x%04X"),tag);
				bUnknown = true;
				return tmpStr;
				break;
		}
	} else if (sect == "MakerIFD") {

		// Makernotes need special handling
		// We only support a few different manufacturers for makernotes.
		
		// A few Canon tags are supported in this routine, the rest are
		// handled by the LookupMakerCanonTag() call.
		if (m_strImgExifMake == "Canon") {

			switch(tag)
			{
			case 0x0001: return _T("Canon.CameraSettings1");break;
			case 0x0004: return _T("Canon.CameraSettings2");break;
			case 0x0006: return _T("Canon.ImageType");break;
			case 0x0007: return _T("Canon.FirmwareVersion");break;
			case 0x0008: return _T("Canon.ImageNumber");break;
			case 0x0009: return _T("Canon.OwnerName");break;
			case 0x000C: return _T("Canon.SerialNumber");break;
			case 0x000F: return _T("Canon.CustomFunctions");break;
			case 0x0012: return _T("Canon.PictureInfo");break;
			case 0x00A9: return _T("Canon.WhiteBalanceTable");break;

			default:
				tmpStr.Format(_T("Canon.0x%04X"),tag);
				bUnknown = true;
				return tmpStr;
				break;
			}

		} // Canon

		else if (m_strImgExifMake == "SIGMA")
		{
			switch(tag)
			{
			case 0x0002: return _T("Sigma.SerialNumber");break; // Ascii Camera serial number 
			case 0x0003: return _T("Sigma.DriveMode");break; // Ascii Drive Mode 
			case 0x0004: return _T("Sigma.ResolutionMode");break; // Ascii Resolution Mode 
			case 0x0005: return _T("Sigma.AutofocusMode");break; // Ascii Autofocus mode 
			case 0x0006: return _T("Sigma.FocusSetting");break; // Ascii Focus setting 
			case 0x0007: return _T("Sigma.WhiteBalance");break; // Ascii White balance 
			case 0x0008: return _T("Sigma.ExposureMode");break; // Ascii Exposure mode 
			case 0x0009: return _T("Sigma.MeteringMode");break; // Ascii Metering mode 
			case 0x000a: return _T("Sigma.LensRange");break; // Ascii Lens focal length range 
			case 0x000b: return _T("Sigma.ColorSpace");break; // Ascii Color space 
			case 0x000c: return _T("Sigma.Exposure");break; // Ascii Exposure 
			case 0x000d: return _T("Sigma.Contrast");break; // Ascii Contrast 
			case 0x000e: return _T("Sigma.Shadow");break; // Ascii Shadow 
			case 0x000f: return _T("Sigma.Highlight");break; // Ascii Highlight 
			case 0x0010: return _T("Sigma.Saturation");break; // Ascii Saturation 
			case 0x0011: return _T("Sigma.Sharpness");break; // Ascii Sharpness 
			case 0x0012: return _T("Sigma.FillLight");break; // Ascii X3 Fill light 
			case 0x0014: return _T("Sigma.ColorAdjustment");break; // Ascii Color adjustment 
			case 0x0015: return _T("Sigma.AdjustmentMode");break; // Ascii Adjustment mode 
			case 0x0016: return _T("Sigma.Quality");break; // Ascii Quality 
			case 0x0017: return _T("Sigma.Firmware");break; // Ascii Firmware 
			case 0x0018: return _T("Sigma.Software");break; // Ascii Software 
			case 0x0019: return _T("Sigma.AutoBracket");break; // Ascii Auto bracket 
			default:
				tmpStr.Format(_T("Sigma.0x%04X"),tag);
				bUnknown = true;
				return tmpStr;
				break;
			}
		} // SIGMA

		else if (m_strImgExifMake == "SONY")
		{

			switch(tag)
			{
			case 0xb021: return _T("Sony.ColorTemperature");break;
			case 0xb023: return _T("Sony.SceneMode");break;
			case 0xb024: return _T("Sony.ZoneMatching");break;
			case 0xb025: return _T("Sony.DynamicRangeOptimizer");break;
			case 0xb026: return _T("Sony.ImageStabilization");break;
			case 0xb027: return _T("Sony.LensID");break;
			case 0xb029: return _T("Sony.ColorMode");break;
			case 0xb040: return _T("Sony.Macro");break;
			case 0xb041: return _T("Sony.ExposureMode");break;
			case 0xb047: return _T("Sony.Quality");break;
			case 0xb04e: return _T("Sony.LongExposureNoiseReduction");break;
			default:
				// No real info is known
				tmpStr.Format(_T("Sony.0x%04X"),tag);
				bUnknown = true;
				return tmpStr;
				break;
			}
		} // SONY

		else if (m_strImgExifMake == "FUJIFILM")
		{
			switch(tag)
			{
			case 0x0000: return _T("Fujifilm.Version");break; // Undefined Fujifilm Makernote version 
			case 0x1000: return _T("Fujifilm.Quality");break; // Ascii Image quality setting 
			case 0x1001: return _T("Fujifilm.Sharpness");break; // Short Sharpness setting 
			case 0x1002: return _T("Fujifilm.WhiteBalance");break; // Short White balance setting 
			case 0x1003: return _T("Fujifilm.Color");break; // Short Chroma saturation setting 
			case 0x1004: return _T("Fujifilm.Tone");break; // Short Contrast setting 
			case 0x1010: return _T("Fujifilm.FlashMode");break; // Short Flash firing mode setting 
			case 0x1011: return _T("Fujifilm.FlashStrength");break; // SRational Flash firing strength compensation setting 
			case 0x1020: return _T("Fujifilm.Macro");break; // Short Macro mode setting 
			case 0x1021: return _T("Fujifilm.FocusMode");break; // Short Focusing mode setting 
			case 0x1030: return _T("Fujifilm.SlowSync");break; // Short Slow synchro mode setting 
			case 0x1031: return _T("Fujifilm.PictureMode");break; // Short Picture mode setting 
			case 0x1100: return _T("Fujifilm.Continuous");break; // Short Continuous shooting or auto bracketing setting 
			case 0x1210: return _T("Fujifilm.FinePixColor");break; // Short Fuji FinePix Color setting 
			case 0x1300: return _T("Fujifilm.BlurWarning");break; // Short Blur warning status 
			case 0x1301: return _T("Fujifilm.FocusWarning");break; // Short Auto Focus warning status 
			case 0x1302: return _T("Fujifilm.AeWarning");break; // Short Auto Exposure warning status 
			default:
				tmpStr.Format(_T("Fujifilm.0x%04X"),tag);
				bUnknown = true;
				return tmpStr;
				break;
			}
		} // FUJIFILM

		else if (m_strImgExifMake == "NIKON")
		{
			if (m_nImgExifMakeSubtype == 1) {
				// Type 1
				switch(tag)
				{
				case 0x0001: return _T("Nikon1.Version");break; // Undefined Nikon Makernote version 
				case 0x0002: return _T("Nikon1.ISOSpeed");break; // Short ISO speed setting 
				case 0x0003: return _T("Nikon1.ColorMode");break; // Ascii Color mode 
				case 0x0004: return _T("Nikon1.Quality");break; // Ascii Image quality setting 
				case 0x0005: return _T("Nikon1.WhiteBalance");break; // Ascii White balance 
				case 0x0006: return _T("Nikon1.Sharpening");break; // Ascii Image sharpening setting 
				case 0x0007: return _T("Nikon1.Focus");break; // Ascii Focus mode 
				case 0x0008: return _T("Nikon1.Flash");break; // Ascii Flash mode 
				case 0x000f: return _T("Nikon1.ISOSelection");break; // Ascii ISO selection 
				case 0x0010: return _T("Nikon1.DataDump");break; // Undefined Data dump 
				case 0x0080: return _T("Nikon1.ImageAdjustment");break; // Ascii Image adjustment setting 
				case 0x0082: return _T("Nikon1.Adapter");break; // Ascii Adapter used 
				case 0x0085: return _T("Nikon1.FocusDistance");break; // Rational Manual focus distance 
				case 0x0086: return _T("Nikon1.DigitalZoom");break; // Rational Digital zoom setting 
				case 0x0088: return _T("Nikon1.AFFocusPos");break; // Undefined AF focus position 
				default:
					tmpStr.Format(_T("Nikon1.0x%04X"),tag);
					bUnknown = true;
					return tmpStr;
					break;
				}
			}
			else if (m_nImgExifMakeSubtype == 2)
			{
				// Type 2
				switch(tag)
				{
				case 0x0003: return _T("Nikon2.Quality");break; // Short Image quality setting 
				case 0x0004: return _T("Nikon2.ColorMode");break; // Short Color mode 
				case 0x0005: return _T("Nikon2.ImageAdjustment");break; // Short Image adjustment setting 
				case 0x0006: return _T("Nikon2.ISOSpeed");break; // Short ISO speed setting 
				case 0x0007: return _T("Nikon2.WhiteBalance");break; // Short White balance 
				case 0x0008: return _T("Nikon2.Focus");break; // Rational Focus mode 
				case 0x000a: return _T("Nikon2.DigitalZoom");break; // Rational Digital zoom setting 
				case 0x000b: return _T("Nikon2.Adapter");break; // Short Adapter used 
				default:
					tmpStr.Format(_T("Nikon2.0x%04X"),tag);
					bUnknown = true;
					return tmpStr;
					break;
				}
			}
			else if (m_nImgExifMakeSubtype == 3)
			{
				// Type 3
				switch(tag)
				{
				case 0x0001: return _T("Nikon3.Version");break; // Undefined Nikon Makernote version 
				case 0x0002: return _T("Nikon3.ISOSpeed");break; // Short ISO speed used 
				case 0x0003: return _T("Nikon3.ColorMode");break; // Ascii Color mode 
				case 0x0004: return _T("Nikon3.Quality");break; // Ascii Image quality setting 
				case 0x0005: return _T("Nikon3.WhiteBalance");break; // Ascii White balance 
				case 0x0006: return _T("Nikon3.Sharpening");break; // Ascii Image sharpening setting 
				case 0x0007: return _T("Nikon3.Focus");break; // Ascii Focus mode 
				case 0x0008: return _T("Nikon3.FlashSetting");break; // Ascii Flash setting 
				case 0x0009: return _T("Nikon3.FlashMode");break; // Ascii Flash mode 
				case 0x000b: return _T("Nikon3.WhiteBalanceBias");break; // SShort White balance bias 
				case 0x000e: return _T("Nikon3.ExposureDiff");break; // Undefined Exposure difference 
				case 0x000f: return _T("Nikon3.ISOSelection");break; // Ascii ISO selection 
				case 0x0010: return _T("Nikon3.DataDump");break; // Undefined Data dump 
				case 0x0011: return _T("Nikon3.ThumbOffset");break; // Long Thumbnail IFD offset 
				case 0x0012: return _T("Nikon3.FlashComp");break; // Undefined Flash compensation setting 
				case 0x0013: return _T("Nikon3.ISOSetting");break; // Short ISO speed setting 
				case 0x0016: return _T("Nikon3.ImageBoundary");break; // Short Image boundry 
				case 0x0018: return _T("Nikon3.FlashBracketComp");break; // Undefined Flash bracket compensation applied 
				case 0x0019: return _T("Nikon3.ExposureBracketComp");break; // SRational AE bracket compensation applied 
				case 0x0080: return _T("Nikon3.ImageAdjustment");break; // Ascii Image adjustment setting 
				case 0x0081: return _T("Nikon3.ToneComp");break; // Ascii Tone compensation setting (contrast) 
				case 0x0082: return _T("Nikon3.AuxiliaryLens");break; // Ascii Auxiliary lens (adapter) 
				case 0x0083: return _T("Nikon3.LensType");break; // Byte Lens type 
				case 0x0084: return _T("Nikon3.Lens");break; // Rational Lens 
				case 0x0085: return _T("Nikon3.FocusDistance");break; // Rational Manual focus distance 
				case 0x0086: return _T("Nikon3.DigitalZoom");break; // Rational Digital zoom setting 
				case 0x0087: return _T("Nikon3.FlashType");break; // Byte Type of flash used 
				case 0x0088: return _T("Nikon3.AFFocusPos");break; // Undefined AF focus position 
				case 0x0089: return _T("Nikon3.Bracketing");break; // Short Bracketing 
				case 0x008b: return _T("Nikon3.LensFStops");break; // Undefined Number of lens stops 
				case 0x008c: return _T("Nikon3.ToneCurve");break; // Undefined Tone curve 
				case 0x008d: return _T("Nikon3.ColorMode");break; // Ascii Color mode 
				case 0x008f: return _T("Nikon3.SceneMode");break; // Ascii Scene mode 
				case 0x0090: return _T("Nikon3.LightingType");break; // Ascii Lighting type 
				case 0x0092: return _T("Nikon3.HueAdjustment");break; // SShort Hue adjustment 
				case 0x0094: return _T("Nikon3.Saturation");break; // SShort Saturation adjustment 
				case 0x0095: return _T("Nikon3.NoiseReduction");break; // Ascii Noise reduction 
				case 0x0096: return _T("Nikon3.CompressionCurve");break; // Undefined Compression curve 
				case 0x0097: return _T("Nikon3.ColorBalance2");break; // Undefined Color balance 2 
				case 0x0098: return _T("Nikon3.LensData");break; // Undefined Lens data 
				case 0x0099: return _T("Nikon3.NEFThumbnailSize");break; // Short NEF thumbnail size 
				case 0x009a: return _T("Nikon3.SensorPixelSize");break; // Rational Sensor pixel size 
				case 0x00a0: return _T("Nikon3.SerialNumber");break; // Ascii Camera serial number 
				case 0x00a7: return _T("Nikon3.ShutterCount");break; // Long Number of shots taken by camera 
				case 0x00a9: return _T("Nikon3.ImageOptimization");break; // Ascii Image optimization 
				case 0x00aa: return _T("Nikon3.Saturation");break; // Ascii Saturation 
				case 0x00ab: return _T("Nikon3.VariProgram");break; // Ascii Vari program 

				default:
					tmpStr.Format(_T("Nikon3.0x%04X"),tag);
					bUnknown = true;
					return tmpStr;
					break;
				}

			}
		} // NIKON

	} // if sect

	bUnknown = true;
	return _T("???");
}


bool CjfifDecode::DecodeMakerSubType()
{
	CString tmpStr;

	m_nImgExifMakeSubtype = 0;

	if (m_strImgExifMake == "NIKON")
	{
		tmpStr = "";
		for (unsigned ind=0;ind<5;ind++) {
			tmpStr += (char)Buf(m_nPos+ind);
		}

		if (tmpStr == "Nikon") {
			if (Buf(m_nPos+6) == 1) {
				// Type 1
				m_pLog->AddLine(_T("    Nikon Makernote Type 1 detected"));
				m_nImgExifMakeSubtype = 1;
				m_nPos += 8;
			} else if (Buf(m_nPos+6) == 2) {
				// Type 3
				m_pLog->AddLine(_T("    Nikon Makernote Type 3 detected"));
				m_nImgExifMakeSubtype = 3;
				m_nPos += 18;
			} else {
				m_pLog->AddLineErr(_T("ERROR: Unknown Nikon Makernote Type"));
				AfxMessageBox(_T("ERROR: Unknown Nikon Makernote Type"));
				return FALSE;
			}
		} else {
			// Type 2
			m_pLog->AddLine(_T("    Nikon Makernote Type 2 detected"));
			//m_nImgExifMakeSubtype = 2;
			// tests on D1 seem to indicate that it uses Type 1 headers
			m_nImgExifMakeSubtype = 1;
			m_nPos += 0;
		}

	}
	else if (m_strImgExifMake == "SIGMA")
	{
		tmpStr = "";
		for (unsigned ind=0;ind<8;ind++) {
			if (Buf(m_nPos+ind) != 0)
				tmpStr += (char)Buf(m_nPos+ind);
		}
		if ( (tmpStr == "SIGMA") ||
			(tmpStr == "FOVEON")  )
		{
			// Valid marker
			// Now skip over the 8-chars and 2 unknown chars
			m_nPos += 10;
		} else {
			m_pLog->AddLineErr(_T("ERROR: Unknown SIGMA Makernote identifier"));
			AfxMessageBox(_T("ERROR: Unknown SIGMA Makernote identifier"));
			return FALSE;
		}

	} // SIGMA
	else if (m_strImgExifMake == "FUJIFILM")
	{
		tmpStr = "";
		for (unsigned ind=0;ind<8;ind++) {
			if (Buf(m_nPos+ind) != 0)
				tmpStr += (char)Buf(m_nPos+ind);
		}
		if (tmpStr == "FUJIFILM")
		{
			// Valid marker
			// Now skip over the 8-chars and 4 Pointer chars
			//CAL! Do I need to lookup this pointer???
			m_nPos += 12;
		} else {
			m_pLog->AddLineErr(_T("ERROR: Unknown FUJIFILM Makernote identifier"));
			AfxMessageBox(_T("ERROR: Unknown FUJIFILM Makernote identifier"));
			return FALSE;
		}

	} // FUJIFILM
	else if (m_strImgExifMake == "SONY")
	{
		tmpStr = "";
		for (unsigned ind=0;ind<12;ind++) {
			if (Buf(m_nPos+ind) != 0)
				tmpStr += (char)Buf(m_nPos+ind);
		}
		if (tmpStr == "SONY DSC ")
		{
			// Valid marker
			// Now skip over the 9-chars and 3 null chars
			m_nPos += 12;
		} else {
			m_pLog->AddLineErr(_T("ERROR: Unknown SONY Makernote identifier"));
			AfxMessageBox(_T("ERROR: Unknown SONY Makernote identifier"));
			return FALSE;
		}

	} // SONY


	return TRUE;

}

// Read the next two DWORDs (8B) to get a rational number
bool CjfifDecode::DecodeValRational(unsigned pos,float &val)
{
	int val_numer;
	int val_denom;
	val = 0;

	val_numer = ByteSwap4(Buf(pos+0),Buf(pos+1),Buf(pos+2),Buf(pos+3));
	val_denom = ByteSwap4(Buf(pos+4),Buf(pos+5),Buf(pos+6),Buf(pos+7));

	if (val_denom == 0) {
		// Divide by zero!
		return false;
	} else {
		val = (float)val_numer/(float)val_denom;
		return true;
	}
}

// Read the next two DWORDs (8B) to get a fraction
CString CjfifDecode::DecodeValFraction(unsigned pos)
{
	CString tmpStr;
	int val_numer = ReadSwap4(pos+0);
	int val_denom = ReadSwap4(pos+4);
	tmpStr.Format(_T("%d/%d"),val_numer,val_denom);
	return tmpStr;
}


bool CjfifDecode::PrintValGPS(unsigned nCount, float fCoord1, float fCoord2, float fCoord3,CString &strCoord)
{
	float		fTemp;
	unsigned	nCoordDeg;
	unsigned	nCoordMin;
	float		fCoordSec;

	//CAL! TODO: Support 1 & 2 coordinate GPS entries
	if (nCount == 3) {
		nCoordDeg = unsigned(fCoord1);
		nCoordMin = unsigned(fCoord2);
		fTemp = fCoord2 - (float)nCoordMin;
		fCoordSec = fTemp * (float)60.0;

		strCoord.Format(_T("%u deg %u' %.3f\""),nCoordDeg,nCoordMin,fCoordSec);
		return true;
	} else {
		strCoord.Format(_T("ERROR: Can't handle %u-comonent GPS coords"),nCount);
		return false;
	}

}

// Read in 3 rationals and format in standard notation
bool CjfifDecode::DecodeValGPS(unsigned pos,CString &strCoord)
{
	float		fCoord1,fCoord2,fCoord3;
	bool		bRet;

	bRet = true;
	if (bRet) { bRet = DecodeValRational(pos,fCoord1); pos += 8; }
	if (bRet) { bRet = DecodeValRational(pos,fCoord2); pos += 8; }
	if (bRet) { bRet = DecodeValRational(pos,fCoord3); pos += 8; }

	if (!bRet) {
		strCoord.Format(_T("???"));
		return false;
	} else {
		return PrintValGPS(3,fCoord1,fCoord2,fCoord3,strCoord);
	}
}

unsigned CjfifDecode::ReadSwap2(unsigned pos)
{
	return ByteSwap2(Buf(pos+0),Buf(pos+1));
}

unsigned CjfifDecode::ReadSwap4(unsigned pos)
{
	return ByteSwap4(Buf(pos),Buf(pos+1),Buf(pos+2),Buf(pos+3));
}

// Read Big Endian DWORD
unsigned CjfifDecode::ReadBe4(unsigned pos)
{
	// Big endian, no swap required
	return (Buf(pos)<<24) + (Buf(pos+1)<<16) + (Buf(pos+2)<<8) + Buf(pos+3);
}

// Print hex from array of uchars
CString CjfifDecode::PrintAsHexUC(unsigned char* naBytes,unsigned nCount)
{
	CString sVal;
	CString sFull;
	sFull = "0x[";
	unsigned nMaxDisplay = MAX_naValues;
	bool bExceedMaxDisplay;
	bExceedMaxDisplay = (nCount > nMaxDisplay);
	for (unsigned i=0;i<nCount;i++)
	{
		if (i < nMaxDisplay) {
			if ((i % 4) == 0) {
				if (i == 0) {
					// Don't do anything for first value!
				} else {
					// Every 16 add big spacer / break
					sFull += " ";
				}
			} else {
				//sFull += " ";
			}

			sVal.Format(_T("%02X"),naBytes[i]);
			sFull += sVal;
		}

		if ((i == nMaxDisplay) && (bExceedMaxDisplay)) {
			sFull += "...";
		}

	}
	sFull += "]";
	return sFull;
}


// Print hex from array of uint
CString CjfifDecode::PrintAsHex(unsigned* naBytes,unsigned nCount)
{
	CString sVal;
	CString sFull;
	sFull = "0x[";
	unsigned nMaxDisplay = MAX_naValues;
	bool bExceedMaxDisplay;
	bExceedMaxDisplay = (nCount > nMaxDisplay);
	for (unsigned i=0;i<nCount;i++)
	{
		if (i < nMaxDisplay) {
			if ((i % 4) == 0) {
				if (i == 0) {
					// Don't do anything for first value!
				} else {
					// Every 16 add big spacer / break
					sFull += " ";
				}
			} else {
				//sFull += " ";
			}

			sVal.Format(_T("%02X"),naBytes[i]);
			sFull += sVal;
		}

		if ((i == nMaxDisplay) && (bExceedMaxDisplay)) {
			sFull += "...";
		}

	}
	sFull += "]";
	return sFull;
}

// Process all of the entries within an EXIF IFD directory
// Param: ifdStr - The IFD section that we are in
//
// NOTE: IFD1 typically contains the thumbnail!
unsigned CjfifDecode::DecodeExifIfd(CString ifdStr,unsigned pos_exif_start,unsigned start_ifd_ptr)
{
	// Temp variables
	bool	bRet;
	CString tmpStr;
	CStr2   retval;
	CString valStr1;
	float val_real;
	CString makerStr;

	// Main printing variables
	CString fullStr;
	CString valStr;
	BOOL extraDecode;

	// Primary IFD variables
	char ifdValOffsetStr[5];
	unsigned ifdDirLen;
	unsigned ifdTagVal;
	unsigned ifdFormat;
	unsigned ifdNumComps;
	bool	ifdTagUnknown;


	unsigned naValues[MAX_naValues];	// Array of decoded values (Uint32)
	signed   naValuesS[MAX_naValues];	// Array of decoded values (Int32)
	float	 faValues[MAX_naValues];	// Array of decoded values (float)
	unsigned nIfdOffset;				// First DWORD decode, usually offset

	// Clear values array
	for (unsigned ind=0;ind<MAX_naValues;ind++) {
		naValues[ind] = 0;
		naValuesS[ind] = 0;
		faValues[ind] = 0;
	}

	// Move the file pointer to the start of the IFD
	m_nPos = pos_exif_start+start_ifd_ptr;

	tmpStr.Format(_T("  EXIF %s @ Absolute 0x%08X"),ifdStr,m_nPos);
	m_pLog->AddLine(tmpStr);

	////////////

	// NOTE: Nikon type 3 starts out with the ASCII string "Nikon\0"
	//       before the rest of the items.
	//CAL! need to process type1,type2,type3
	// see: http://www.gvsoft.homedns.org/exif/makernote-nikon.html

	tmpStr.Format(_T("ifdStr=[%s] m_strImgExifMake=[%s]"),ifdStr,m_strImgExifMake);
	DbgAddLine(tmpStr);


	// If this is the MakerNotes section, then we may want to skip
	// altogether. Check to see if we are configured to process this
	// section or if it is a supported manufacturer.

	if (ifdStr == "MakerIFD")
	{
		// Mark the image as containing Makernotes
		m_bImgExifMakernotes = true;

		if (!m_pAppConfig->bDecodeMaker) {
			tmpStr.Format(_T("    Makernote decode option not enabled."));
			m_pLog->AddLine(tmpStr);

			// If user didn't enable makernote decode, don't exit, just
			// hide output. We still want to get at some info (such as Quality setting).
			// At end, we'll need to re-enable it again.
			m_pLog->Disable();
		}

		// If this Make is not supported, we'll need to exit
		if (!m_bImgExifMakeSupported) {
			tmpStr.Format(_T("    Makernotes not yet supported for [%s]"),m_strImgExifMake);
			m_pLog->AddLine(tmpStr);

			m_pLog->Enable();
			return 2;
		}

		// Determine the sub-type of the Maker field (if applicable)
		// and advance the m_nPos pointer past the custom header.
		// This call uses the class members: Buf(),m_nPos
		if (!DecodeMakerSubType())
		{
			// If the subtype decode failed, skip the processing
			m_pLog->Enable();
			return 2;
		}

	}

	CString ifdTagStr;

	ifdDirLen = ReadSwap2(m_nPos);
	m_nPos+=2;
	tmpStr.Format(_T("    Dir Length = 0x%04X"),ifdDirLen);
	m_pLog->AddLine(tmpStr);

	// Start of IFD processing
	// Step through each IFD entry and determine the type and
	// decode accordingly.
	for (unsigned ifdEntryInd=0;ifdEntryInd<ifdDirLen;ifdEntryInd++)
	{

		// By default, show single-line value summary
		// extraDecode is used to indicate that additional special
		// parsing output is available for this entry
		extraDecode = FALSE;

		tmpStr.Format(_T("    Entry #%02u:"),ifdEntryInd);
		DbgAddLine(tmpStr);

		// Read Tag #
		ifdTagVal = ReadSwap2(m_nPos);
		m_nPos+=2;
		ifdTagUnknown = false;
		ifdTagStr = LookupExifTag(ifdStr,ifdTagVal,ifdTagUnknown);
		tmpStr.Format(_T("      Tag # = 0x%04X = [%s]"),ifdTagVal,ifdTagStr);
		DbgAddLine(tmpStr);

		// Read Format
		ifdFormat = ReadSwap2(m_nPos);
		m_nPos+=2;
		tmpStr.Format(_T("      Format # = 0x%04X"),ifdFormat);
		DbgAddLine(tmpStr);

		// Read number of Components
		ifdNumComps = ReadSwap4(m_nPos);
		m_nPos+=4;
		tmpStr.Format(_T("      # Comps = 0x%08X"),ifdNumComps);
		DbgAddLine(tmpStr);

		// Check to see how many components have been listed.
		// This helps trap errors in corrupted IFD segments, otherwise
		// we will hang trying to decode millions of entries!
		// See issue & testcase #1148
		if (ifdNumComps > 4000) {
			tmpStr.Format("      Excessive # components (%u). Limiting to first 4000.",ifdNumComps);
			m_pLog->AddLineWarn(tmpStr);
			ifdNumComps = 4000;
		}

		// Read Component Value / Offset
		// We first treat it as a string and then re-interpret it as an integer

		// ... first as a string (just in case length <=4)
		for (unsigned i=0;i<4;i++) {
			ifdValOffsetStr[i] = (char)Buf(m_nPos+i);
		}
		ifdValOffsetStr[4] = 0;

		// ... now as an unsigned value
		// This assignment is general-purpose, typically used when
		// we know that the IFD Value/Offset is just an offset
		nIfdOffset = ReadSwap4(m_nPos);
		tmpStr.Format(_T("      # Val/Offset = 0x%08X"),nIfdOffset);
		DbgAddLine(tmpStr);

/*
		// FIXME
		// Important:
		//   I have encountered a file that contains very large (negative?) numbers in the "value offset"
		//   field (for makernotes). This will cause an exception if I try to read past
		//   the end of the file, so to safeguard it, I will skip it here:
		NOTE: Somehow I don't want this code to trigger in the normal case
		  where numbers are greater than this when used as a value, not offset!
		  Instead, I am simply going to fix up the Buf() code to be more robust
		  and let these display routines error out.
		if ( (nIfdOffset > 0xFF000000) ||
			(pos_exif_start+nIfdOffset+1024 > m_nPosFileEnd) )
		{
			// Looks like a dangerous pointer!
			valStr = "";
			fullStr = "        Unsigned Short=[";
			valStr.Format("*** INVALID Offset 0x%08X ***",nIfdOffset);
			fullStr += valStr;
			fullStr += "]";
			DbgAddLine(fullStr);
			die?
		}
*/

		// The EXIF IFD entries can appear in a wide range of
		// formats / data types. The formats that have been
		// publicly documented include:
		//   EXIF_FORMAT_BYTE       =  1,
		//   EXIF_FORMAT_ASCII      =  2,
		//   EXIF_FORMAT_SHORT      =  3,
		//   EXIF_FORMAT_LONG       =  4,
		//   EXIF_FORMAT_RATIONAL   =  5,
		//   EXIF_FORMAT_SBYTE      =  6,
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

		switch(ifdFormat)
		{

		// ----------------------------------------
		// --- IFD Entry Type: Unsigned Byte
		// ----------------------------------------
		case 1:
			fullStr = "        Unsigned Byte=[";
			valStr = "";
			// Assume this is a byte (always in m_nPos[0])

			// If only a single value, use decimal, else use hex
			if (ifdNumComps == 1) {
					naValues[0] = Buf(m_nPos+0);
					tmpStr.Format(_T("%u"),naValues[0]);
					valStr += tmpStr;
			} else {

				for (unsigned i=0;i<ifdNumComps;i++)
				{
					if (i<MAX_naValues) {
						naValues[i] = Buf(m_nPos+i);
					}
				}
				valStr = PrintAsHex(naValues,ifdNumComps);
			}
			fullStr += valStr;
			fullStr += "]";
			DbgAddLine(fullStr);
			break;


		// ----------------------------------------
		// --- IFD Entry Type: ASCII string
		// ----------------------------------------
		case 2:
			fullStr = "        String=";
			valStr = "";
			char cVal;
			BYTE nVal;
			for (unsigned i=0;i<ifdNumComps;i++)
			{
				if (ifdNumComps<=4) {
					nVal = ifdValOffsetStr[i];
					//valStr += ifdValOffsetStr[i];
				}
				else
				{
					//CAL! How can I move this to the later "custom decode" section?
					// I would prefer not to decode the makernotes here, but unfortunately
					// some of the Nikon makernotes use a non-standard offset value
					// that would prevent us from 
					if ( (ifdStr == "MakerIFD") && (m_strImgExifMake == "NIKON") && (m_nImgExifMakeSubtype == 3) )
					{
						// It seems that pointers in the Nikon Makernotes are
						// done relative to the start of Maker IFD
						// But why 10? Is this 10 = 18-8?
						nVal = Buf(pos_exif_start+m_nImgExifMakerPtr+nIfdOffset+10+i);
						//valStr += (char)Buf(pos_exif_start+m_nImgExifMakerPtr+nIfdOffset+10+i);
					} else if ( (ifdStr == "MakerIFD") && (m_strImgExifMake == "NIKON") )
					{
						// It seems that pointers in the Nikon Makernotes are
						// done relative to the start of Maker IFD
						nVal = Buf(pos_exif_start+nIfdOffset+0+i);
						//valStr += (char)Buf(pos_exif_start+nIfdOffset+0+i);
					} else {
						// Canon Makernotes seem to be relative to the start
						// of the EXIF IFD
						nVal = Buf(pos_exif_start+nIfdOffset+i);
						//valStr += (char)Buf(pos_exif_start+nIfdOffset+i);
					}
				}

				// Add the character!
				//if (cVal != 0) {
					//valStr += cVal;
				//}
				// Just in case the string has been null-terminated early
				// or we have garbage, replace with .
				//CAL! Need to clean this up!
				if (nVal != 0) {
					cVal = (char)nVal;
					if (!isprint(nVal)) {
						cVal = '.';
					}
					valStr += cVal;
					//valStr += (isprint((int)nVal) ? cVal : ".");
				}

			}
			fullStr += valStr;
			DbgAddLine(fullStr);

			//CAL! TODO: Would be nice to have a separate string that is used
			// only for display purposes. "valStr" is used in other areas (eg.
			// in assignment to EXIF Make/Model/Software, etc. and so we wouldn't
			// want to mess that up. But for display purposes it would be nice
			// to show quotes around strings.

			break;


		// ----------------------------------------
		// --- IFD Entry Type: Unsigned Short (2 bytes)
		// ----------------------------------------
		case 3:
			// Unsigned Short (2 bytes)
			if (ifdNumComps == 1) {
				fullStr = "        Unsigned Short=[";
				//CAL! BUG! Refer to pg 14 of Exif2-2 spec. Need to use endianness!
				// What do I do about using only 2 bytes (short) out of 1 word??
				//CAL! for now assume 2byte conversion from [1:0] out of [3:0]
				naValues[0] = ReadSwap2(m_nPos);
				valStr.Format(_T("%u"),naValues[0]);
				fullStr += valStr;
				fullStr += "]";
				DbgAddLine(fullStr);
			} else if (ifdNumComps == 2) {
				fullStr = "        Unsigned Short=[";
				// 2 unsigned shorts in 1 word
				naValues[0] = ReadSwap2(m_nPos+0);
				naValues[1] = ReadSwap2(m_nPos+2);
				valStr.Format(_T("%u, %u"),naValues[0],naValues[1]);
				fullStr += valStr;
				fullStr += "]";
				DbgAddLine(fullStr);
			} else if (ifdNumComps > MAX_COMPS) {
				valStr1.Format(_T("    Unsigned Short=[Too many entries (%u) to display]"),ifdNumComps);
				DbgAddLine(valStr1);
				valStr.Format(_T("[Too many entries (%u) to display]"),ifdNumComps);
			} else {
				// Try to handle multiple entries... note that this
				// is used by the Maker notes IFD decode



/*
				// Important:
				//   I have encountered a file (Submitted by ?)
				//   that contains very large (negative?) numbers in the "value offset"
				//   field (for makernotes). This will cause an exception if I try to read past
				//   the end of the file, so to safeguard it, I will skip it here:
				if ( (nIfdOffset > 0xFF000000) ||
					(pos_exif_start+nIfdOffset+1024 > m_nPosFileEnd) )
				{
					// Looks like a dangerous pointer!
					valStr = "";
					fullStr = "        Unsigned Short=[";
					valStr.Format("*** INVALID Offset 0x%08X ***",nIfdOffset);
					fullStr += valStr;
					fullStr += "]";
					DbgAddLine(fullStr);
				} else
*/

					valStr = "";
					fullStr = "        Unsigned Short=[";
					for (unsigned ind=0;ind<ifdNumComps;ind++) {
						if (ind!=0)	{ valStr += ", "; }
						if (ind<MAX_naValues) {
							naValues[ind] = ReadSwap2(pos_exif_start+nIfdOffset+(2*ind));
							valStr1.Format(_T("%u"),naValues[ind]);
							valStr += valStr1;
						}
					}
					fullStr += valStr;
					fullStr += "]";
					DbgAddLine(fullStr);
//				} // not a maker

				// ****************************


			}
			break;


		// ----------------------------------------
		// --- IFD Entry Type: Unsigned Long (4 bytes)
		// ----------------------------------------
		case 4:
			fullStr = "        Unsigned Long=[";
			valStr = "";
			for (unsigned ind=0;ind<ifdNumComps;ind++)
			{
				if (ind<MAX_naValues) {
					naValues[ind] = ReadSwap4(m_nPos+(ind*4));
					if (ind!=0)	{ valStr += ", "; }
					valStr1.Format(_T("%u"),naValues[ind]);
					valStr += valStr1;
				}
			}
			fullStr += valStr;
			fullStr += "]";
			DbgAddLine(fullStr);
			break;


		// ----------------------------------------
		// --- IFD Entry Type: Unsigned Rational (8 bytes)
		// ----------------------------------------
		case 5:
			// Unsigned Rational
			fullStr = "        Unsigned Rational=[";
			valStr = "";

			for (unsigned ind=0;ind<ifdNumComps;ind++)
			{
				if (ind!=0)	{ valStr += ", "; }
				if (ind<MAX_naValues) {
					valStr1 = DecodeValFraction(pos_exif_start+nIfdOffset+(ind*8));
					bRet = DecodeValRational(pos_exif_start+nIfdOffset+(ind*8),val_real);
					faValues[ind] = val_real;
					valStr += valStr1;
				}
			}
			fullStr += valStr;
			fullStr += "]";
			DbgAddLine(fullStr);
			break;


		// ----------------------------------------
		// --- IFD Entry Type: Undefined (?)
		// ----------------------------------------
		case 7:
			// Undefined -- assume 1 word long
			// This is supposed to be a series of 8-bit bytes
			// It is usually used for 32-bit pointers (in case of offsets), but could
			// also represent ExifVersion, etc.
			fullStr = "        Undefined=[";
			valStr = "";
			
			if (ifdNumComps <= 4) {
				//CAL! What to do here???
				for (unsigned ind=0;ind<ifdNumComps;ind++) {
					naValues[ind] = Buf(m_nPos+ind);
				}
				valStr = PrintAsHex(naValues,ifdNumComps);
				fullStr += valStr;

			} else {

				for (unsigned ind=0;ind<ifdNumComps;ind++) {
					if (ind<MAX_naValues) {
						naValues[ind] = Buf(pos_exif_start+nIfdOffset+ind);
					}
				}
				valStr = PrintAsHex(naValues,ifdNumComps);
				fullStr += valStr;
			}
			fullStr += "]";
			DbgAddLine(fullStr);

			break;


		// ----------------------------------------
		// --- IFD Entry Type: Signed Short (2 bytes)
		// ----------------------------------------
		case 8:
			// Signed Short (2 bytes)
			if (ifdNumComps == 1) {
				fullStr = "        Signed Short=[";
				// What do I do about using only 2 bytes (short) out of 1 word??
				//CAL! for now assume 2byte conversion from [1:0] out of [3:0]

				//CAL! BUG: *** Need to make sure ReadSwap2 does signed!!!
				naValuesS[0] = ReadSwap2(m_nPos);
				valStr.Format(_T("%d"),naValuesS[0]);
				fullStr += valStr;
				fullStr += "]";
				DbgAddLine(fullStr);
			} else if (ifdNumComps == 2) {
				fullStr = "        Signed Short=[";
				// 2 signed shorts in 1 word

				//CAL! BUG: *** Need to make sure ReadSwap2 does signed!!!
				naValuesS[0] = ReadSwap2(m_nPos+0);
				naValuesS[1] = ReadSwap2(m_nPos+2);
				valStr.Format(_T("%d, %d"),naValuesS[0],naValuesS[0]);
				fullStr += valStr;
				fullStr += "]";
				DbgAddLine(fullStr);
			} else if (ifdNumComps > MAX_COMPS) {
				// Only print it out if it has less than MAX_COMPS entries
				valStr1.Format(_T("    Signed Short=[Too many entries (%u) to display]"),ifdNumComps);
				DbgAddLine(valStr1);
				valStr.Format(_T("[Too many entries (%u) to display]"),ifdNumComps);
			} else {
				// Try to handle multiple entries... note that this
				// is used by the Maker notes IFD decode

				// Note that we don't call LookupMakerCanonTag() here
				// as that is only needed for the "unsigned short", not
				// "signed short".
				valStr = "";
				fullStr = "        Signed Short=[";
				for (unsigned ind=0;ind<ifdNumComps;ind++) {
					if (ind!=0)	{ valStr += ", "; }
					if (ind<MAX_naValues) {
						naValuesS[ind] = ReadSwap2(pos_exif_start+nIfdOffset+(2*ind));
						valStr1.Format(_T("%d"),naValuesS[ind]);
						valStr += valStr1;
					}
				}
				fullStr += valStr;
				fullStr += "]";
				DbgAddLine(fullStr);

			}
			break;



		// ----------------------------------------
		// --- IFD Entry Type: Signed Rational (8 bytes)
		// ----------------------------------------
		case 10:
			// Signed Rational
			fullStr = "        Signed Rational=[";
			valStr = "";
			for (unsigned ind=0;ind<ifdNumComps;ind++)
			{
				if (ind!=0)	{ valStr += ", "; }

				valStr1 = DecodeValFraction(pos_exif_start+nIfdOffset+(ind*8));
				bRet = DecodeValRational(pos_exif_start+nIfdOffset+(ind*8),val_real);
				faValues[ind] = val_real;
				valStr += valStr1;
			}
			fullStr += valStr;
			fullStr += "]";
			DbgAddLine(fullStr);
			break;

		default:
			tmpStr.Format(_T("ERROR: Unsupported format [%d]"),ifdFormat);
			//AfxMessageBox(tmpStr);
			naValues[0] = ReadSwap4(m_nPos);
			valStr.Format(_T("0x%08X???"),naValues[0]);
			m_pLog->Enable();
			return 2;
			break;
		} // switch ifdTagVal




		// ======================================================
		// Custom Value String decodes
		// ======================================================
		// At this point we might re-format the values, thereby
		// overriding the default valStr. We have access to the
		//   naValues[]  (array of unsigned int)
		//   naValuesS[] (array of signed int)
		//   faValues[]  (array of float)
		// ======================================================


		// ----------------------------------------
		// Re-format special output items
		//   This will override "valStr" that may have previously been defined
		// ----------------------------------------

		if ((ifdTagStr == "GPSLatitude") ||
			(ifdTagStr == "GPSLongitude")) {
			bRet = PrintValGPS(ifdNumComps,faValues[0],faValues[1],faValues[2],valStr);
		} else if (ifdTagStr == "GPSVersionID") {
			valStr.Format(_T("%u.%u.%u.%u"),naValues[0],naValues[1],naValues[2],naValues[3]);
		} else if (ifdTagStr == "GPSAltitudeRef") {
			switch (naValues[0]) {
				case 0 : valStr = "Above Sea Level"; break;
				case 1 : valStr = "Below Sea Level"; break;
			}
		} else if (ifdTagStr == "GPSStatus") {
			switch (ifdValOffsetStr[0]) {
				case 'A' : valStr = "Measurement in progress"; break;
				case 'V' : valStr = "Measurement Interoperability"; break;
			}
		} else if (ifdTagStr == "GPSMeasureMode") {
			switch (ifdValOffsetStr[0]) {
				case '2' : valStr = "2-dimensional"; break;
				case '3' : valStr = "3-dimensional"; break;
			}
		} else if ((ifdTagStr == "GPSSpeedRef") ||
					(ifdTagStr == "GPSDestDistanceRef")) {
			switch (ifdValOffsetStr[0]) {
				case 'K' : valStr = "km/h"; break;
				case 'M' : valStr = "mph"; break;
				case 'N' : valStr = "knots"; break;
			}
		} else if ((ifdTagStr == "GPSTrackRef") ||
					(ifdTagStr == "GPSImgDirectionRef") ||
					(ifdTagStr == "GPSDestBearingRef")) {
			switch (ifdValOffsetStr[0]) {
				case 'T' : valStr = "True direction"; break;
				case 'M' : valStr = "Magnetic direction"; break;
			}
		} else if (ifdTagStr == "GPSDifferential") {
			switch (naValues[0]) {
				case 0 : valStr = "Measurement without differential correction"; break;
				case 1 : valStr = "Differential correction applied"; break;
			}
		}


		if (ifdTagStr == "Compression") {
			switch (naValues[0]) {
				case 1 : valStr = "None"; break;
				case 6 : valStr = "JPEG"; break;
			}
		} else if (ifdTagStr == "ExposureTime") {
			// Assume only one
			valStr1 = valStr;
			valStr.Format(_T("%s s"),valStr1);
		} else if (ifdTagStr == "FNumber") {
			// Assume only one
			valStr.Format(_T("F%.1f"),faValues[0]);
		} else if (ifdTagStr == "FocalLength") {
			// Assume only one
			valStr.Format(_T("%.0f mm"),faValues[0]);
		} else if (ifdTagStr == "ExposureBiasValue") {
			// Assume only one
			//CAL! BUG! Need to test negative numbers!!!
			valStr.Format(_T("%0.2f eV"),faValues[0]);
		} else if (ifdTagStr == "ExifVersion") {
			// Assume only one
			valStr.Format(_T("%c%c.%c%c"),naValues[0],naValues[1],naValues[2],naValues[3]);
		} else if (ifdTagStr == "FlashPixVersion") {
			// Assume only one
			valStr.Format(_T("%c%c.%c%c"),naValues[0],naValues[1],naValues[2],naValues[3]);
		} else if (ifdTagStr == "PhotometricInterpretation") {
			switch (naValues[0]) {
				case 1 : valStr = "Monochrome"; break;
				case 2 : valStr = "RGB"; break;
				case 6 : valStr = "YCbCr"; break;
			}
		} else if (ifdTagStr == "Orientation") {
			switch (naValues[0]) {
				case 1 : valStr = "Row 0: top, Col 0: left"; break;
				case 2 : valStr = "Row 0: top, Col 0: right"; break;
				case 3 : valStr = "Row 0: bottom, Col 0: right"; break;
				case 4 : valStr = "Row 0: bottom, Col 0: left"; break;
				case 5 : valStr = "Row 0: left, Col 0: top"; break;
				case 6 : valStr = "Row 0: right, Col 0 top"; break;
				case 7 : valStr = "Row 0: right, Col 0: bottom"; break;
				case 8 : valStr = "Row 0: left, Col 0: bottom"; break;
			}
		} else if (ifdTagStr == "PlanarConfiguration") {
			switch (naValues[0]) {
				case 1 : valStr = "Chunky format"; break;
				case 2 : valStr = "Planar format"; break;
			}
		} else if (ifdTagStr == "YCbCrSubSampling") {
			switch (naValues[0]*65536 + naValues[1]) {
				case 0x00020001 : valStr = "4:2:2"; break;
				case 0x00020002 : valStr = "4:2:0"; break;
			}
		} else if (ifdTagStr == "YCbCrPositioning") {
			switch (naValues[0]) {
				case 1 : valStr = "Centered"; break;
				case 2 : valStr = "Co-sited"; break;
			}
		} else if (ifdTagStr == "ResolutionUnit") {
			switch (naValues[0]) {
				case 1 : valStr = "None"; break;
				case 2 : valStr = "Inch"; break;
				case 3 : valStr = "Centimeter"; break;
			}
		} else if (ifdTagStr == "FocalPlaneResolutionUnit") {
			switch (naValues[0]) {
				case 1 : valStr = "None"; break;
				case 2 : valStr = "Inch"; break;
				case 3 : valStr = "Centimeter"; break;
			}
		} else if (ifdTagStr == "ColorSpace") {
			switch (naValues[0]) {
				case 1 : valStr = "sRGB"; break;
				case 0xFFFF : valStr = "Uncalibrated"; break;
			}
		} else if (ifdTagStr == "ComponentsConfiguration") {
			// Undefined type, assume 4 bytes
			valStr = "[";
			for (unsigned vind=0;vind<4;vind++) {
				if (vind != 0) { valStr += " "; }
				switch (naValues[vind]) {
					case 0 : valStr += "."; break;
					case 1 : valStr += "Y"; break;
					case 2 : valStr += "Cb"; break;
					case 3 : valStr += "Cr"; break;
					case 4 : valStr += "R"; break;
					case 5 : valStr += "G"; break;
					case 6 : valStr += "B"; break;
					default : valStr += "?"; break;
				}
			}
			valStr += "]";

		} else if ( (ifdTagStr == "XPTitle") ||
					(ifdTagStr == "XPComment") ||
					(ifdTagStr == "XPAuthor") ||
					(ifdTagStr == "XPKeywords") ||
					(ifdTagStr == "XPSubject") ) {
			//CAL! Bug! Need to do proper UNICODE decode!
			valStr = "\"";
			char cTmp1,cTmp2;
			for (unsigned vInd=0;vInd*2<ifdNumComps;vInd++) {
				// UNICODE hack!
				cTmp1 = (char)Buf(pos_exif_start+nIfdOffset+vInd*2+0);
				cTmp2 = (char)Buf(pos_exif_start+nIfdOffset+vInd*2+1);
				if (cTmp1 != 0) { valStr += cTmp1; }
			}
			valStr += "\"";
		} else if (ifdTagStr == "UserComment") {
			// Character code
			unsigned naCharCode[8];
			for (unsigned vInd=0;vInd<8;vInd++) {
				naCharCode[vInd] = Buf(pos_exif_start+nIfdOffset+0+vInd);
			}
			// Actual string
			valStr = "\"";
			bool bDone = false;
			char cTmp;

			for (unsigned vInd=0;(vInd<ifdNumComps-8)&&(!bDone);vInd++) {
				cTmp = (char)Buf(pos_exif_start+nIfdOffset+8+vInd);
				if (cTmp == 0) { bDone = true; } else {	valStr += cTmp;	}
			}
			valStr += "\"";
		} else if (ifdTagStr == "MeteringMode") {
			switch (naValues[0]) {
				case 0 : valStr = "Unknown"; break;
				case 1 : valStr = "Average"; break;
				case 2 : valStr = "CenterWeightedAverage"; break;
				case 3 : valStr = "Spot"; break;
				case 4 : valStr = "MultiSpot"; break;
				case 5 : valStr = "Pattern"; break;
				case 6 : valStr = "Partial"; break;
				case 255 : valStr = "Other"; break;
			}
		} else if (ifdTagStr == "ExposureProgram") {
			switch (naValues[0]) {
				case 0 : valStr = "Not defined"; break;
				case 1 : valStr = "Manual"; break;
				case 2 : valStr = "Normal program"; break;
				case 3 : valStr = "Aperture priority"; break;
				case 4 : valStr = "Shutter priority"; break;
				case 5 : valStr = "Creative program (depth of field)"; break;
				case 6 : valStr = "Action program (fast shutter speed)"; break;
				case 7 : valStr = "Portrait mode"; break;
				case 8 : valStr = "Landscape mode"; break;
			}
		} else if (ifdTagStr == "Flash") {
			switch (naValues[0] & 1) {
				case 0 : valStr = "Flash did not fire"; break;
				case 1 : valStr = "Flash fired"; break;
			}
			//CAL! TODO: Add other bitfields?
		} else if (ifdTagStr == "SensingMethod") {
			switch (naValues[0]) {
				case 1 : valStr = "Not defined"; break;
				case 2 : valStr = "One-chip color area sensor"; break;
				case 3 : valStr = "Two-chip color area sensor"; break;
				case 4 : valStr = "Three-chip color area sensor"; break;
				case 5 : valStr = "Color sequential area sensor"; break;
				case 7 : valStr = "Trilinear sensor"; break;
				case 8 : valStr = "Color sequential linear sensor"; break;
			}
		} else if (ifdTagStr == "FileSource") {
			switch (naValues[0]) {
				case 3 : valStr = "DSC"; break;
			}
		} else if (ifdTagStr == "CustomRendered") {
			switch (naValues[0]) {
				case 0 : valStr = "Normal process"; break;
				case 1 : valStr = "Custom process"; break;
			}
		} else if (ifdTagStr == "ExposureMode") {
			switch (naValues[0]) {
				case 0 : valStr = "Auto exposure"; break;
				case 1 : valStr = "Manual exposure"; break;
				case 2 : valStr = "Auto bracket"; break;
			}
		} else if (ifdTagStr == "WhiteBalance") {
			switch (naValues[0]) {
				case 0 : valStr = "Auto white balance"; break;
				case 1 : valStr = "Manual white balance"; break;
			}
		} else if (ifdTagStr == "SceneCaptureType") {
			switch (naValues[0]) {
				case 0 : valStr = "Standard"; break;
				case 1 : valStr = "Landscape"; break;
				case 2 : valStr = "Portrait"; break;
				case 3 : valStr = "Night scene"; break;
			}
		} else if (ifdTagStr == "SceneType") {
			switch (naValues[0]) {
				case 1 : valStr = "A directly photographed image"; break;
			}
		} else if (ifdTagStr == "LightSource") {
			switch (naValues[0]) {
				case 0 : valStr = "unknown"; break;
				case 1 : valStr = "Daylight"; break;
				case 2 : valStr = "Fluorescent"; break;
				case 3 : valStr = "Tungsten (incandescent light)"; break;
				case 4 : valStr = "Flash"; break;
				case 9 : valStr = "Fine weather"; break;
				case 10 : valStr = "Cloudy weather"; break;
				case 11 : valStr = "Shade"; break;
				case 12 : valStr = "Daylight fluorescent (D 5700 – 7100K)"; break;
				case 13 : valStr = "Day white fluorescent (N 4600 – 5400K)"; break;
				case 14 : valStr = "Cool white fluorescent (W 3900 – 4500K)"; break;
				case 15 : valStr = "White fluorescent (WW 3200 – 3700K)"; break;
				case 17 : valStr = "Standard light A"; break;
				case 18 : valStr = "Standard light B"; break;
				case 19 : valStr = "Standard light C"; break;
				case 20 : valStr = "D55"; break;
				case 21 : valStr = "D65"; break;
				case 22 : valStr = "D75"; break;
				case 23 : valStr = "D50"; break;
				case 24 : valStr = "ISO studio tungsten"; break;
				case 255 : valStr = "other light source"; break;
			}
		} else if (ifdTagStr == "SubjectArea") {
			switch (ifdNumComps) {
				case 2 :
					// coords
					valStr.Format(_T("Coords: Center=[%u,%u]"),
						naValues[0],naValues[1]);
					break;
				case 3 :
					// circle
					valStr.Format(_T("Coords (Circle): Center=[%u,%u] Diameter=%u"),
						naValues[0],naValues[1],naValues[2]);
					break;
				case 4 :
					// rectangle
					valStr.Format(_T("Coords (Rect): Center=[%u,%u] Width=%u Height=%u"),
						naValues[0],naValues[1],naValues[2],naValues[3]);
					break;
				default:
					// Leave default decode, unexpected value
					break;
			}
		}

		if (ifdTagStr == "CFAPattern") {
			unsigned nHorzRepeat,nVertRepeat;
			unsigned nCfaVal[16][16];
			unsigned nInd=0;
			unsigned nVal;
			CString	 sLine,sCol;
			nHorzRepeat = naValues[nInd+0]*256+naValues[nInd+1];
			nVertRepeat = naValues[nInd+2]*256+naValues[nInd+3];
			nInd+=4;
			if ((nHorzRepeat < 16) && (nVertRepeat < 16)) {
				extraDecode = TRUE;
				tmpStr.Format(_T("    [%-36s] ="),ifdTagStr);
				m_pLog->AddLine(tmpStr);
				for (unsigned nY=0;nY<nVertRepeat;nY++) {
					sLine.Format(_T("     %-36s  = [ "  ),"");
					for (unsigned nX=0;nX<nHorzRepeat;nX++) {
						nVal = naValues[nInd++];
						nCfaVal[nY][nX] = nVal;
						switch(nVal) {
							case 0: sCol = "Red";break;
							case 1: sCol = "Grn";break;
							case 2: sCol = "Blu";break;
							case 3: sCol = "Cya";break;
							case 4: sCol = "Mgn";break;
							case 5: sCol = "Yel";break;
							case 6: sCol = "Wht";break;
							default: sCol.Format(_T("x%02X"),nVal);break;
						}
						sLine.AppendFormat(_T("%s "),sCol);
					}
					sLine.Append(_T("]"));
					m_pLog->AddLine(sLine);
				}
			}

		}


		if ((ifdStr == "InteropIFD") && (ifdTagStr == "InteroperabilityVersion")) {
			// Assume only one
			valStr.Format(_T("%c%c.%c%c"),naValues[0],naValues[1],naValues[2],naValues[3]);
		}


		// ----------------------------------------
		// Handle certain MakerNotes
		//   For Canon, we have a special parser routine to handle these
		// ----------------------------------------
		if (ifdStr == "MakerIFD") {

			if ((m_strImgExifMake == "Canon") && (ifdFormat == 3) && (ifdNumComps > 4)) {
				// Print summary line now, before sub details
				// Disable later summary line
				extraDecode = TRUE;
				if ((!m_pAppConfig->bExifHideUnknown) || (!ifdTagUnknown)) {
					tmpStr.Format(_T("    [%-36s]"),ifdTagStr);
					m_pLog->AddLine(tmpStr);

					// Assume it is a maker field with subentries!

					for (unsigned ind=0;ind<ifdNumComps;ind++) {
						//CAL! Limit the number of entries!
						if (ind < 300) {
							valStr.Format(_T("#%u=%u "),ind,naValues[ind]);
							retval = LookupMakerCanonTag(ifdTagVal,ind,naValues[ind]);
							makerStr = retval.tag;
							valStr1.Format(_T("      [%-34s] = %s"),makerStr,retval.val);
							if ((!m_pAppConfig->bExifHideUnknown) || (!retval.bUnknown)) {
								m_pLog->AddLine(valStr1);
							}
						} else if (ind == 300) {
							m_pLog->AddLine(_T("      [... etc ...]"));
						} else {
							// Don't print!
						}
					}
				}


				valStr = "...";
			}

			// For Nikon & Sigma, we simply support the quality field
			if ( (ifdTagStr=="Nikon1.Quality") || 
				(ifdTagStr=="Nikon2.Quality") ||
				(ifdTagStr=="Nikon3.Quality") ||
				(ifdTagStr=="Sigma.Quality") )
			{
				m_strImgQualExif = valStr;

				// Collect extra details (for later DB submission)
				tmpStr = "";
				tmpStr.Format("[%s]:[%s],",ifdTagStr,valStr);
				m_strImgExtras += tmpStr;
			}

			// Collect extra details (for later DB submission)
			if (ifdTagStr=="Canon.ImageType") {
				tmpStr = "";
				tmpStr.Format("[%s]:[%s],",ifdTagStr,valStr);
				m_strImgExtras += tmpStr;
			}
		}

		// Now extract some of the important offsets / pointers
		if ((ifdStr == "IFD0") && (ifdTagStr == "ExifOffset")) {
			// EXIF SubIFD - Pointer
			m_nImgExifSubIfdPtr = nIfdOffset;
			valStr.Format(_T("@ 0x%04X"),nIfdOffset);
		}

		if ((ifdStr == "IFD0") && (ifdTagStr == "GPSOffset")) {
			// GPS SubIFD - Pointer
			m_nImgExifGpsIfdPtr = nIfdOffset;
			valStr.Format(_T("@ 0x%04X"),nIfdOffset);
		}

		//CAL! Add Interoperability IFD (0xA005)???
		if ((ifdStr == "SubIFD") && (ifdTagStr == "ExifInteroperabilityOffset")) {
			m_nImgExifInteropIfdPtr = nIfdOffset;
			valStr.Format(_T("@ 0x%04X"),nIfdOffset);
		}

		// Extract software field!
		if ((ifdStr == "IFD0") && (ifdTagStr == "Software")) {
			m_strSoftware = valStr;
		}

		// -------------------------
		// IFD0 - ExifMake
		// -------------------------
		if ((ifdStr == "IFD0") && (ifdTagStr == "Make")) {
			m_strImgExifMake = valStr;
			m_strImgExifMake.Trim(); // Trim whitespace (e.g. Pentax)

		}

		// -------------------------
		// IFD0 - ExifModel
		// -------------------------
		if ((ifdStr == "IFD0") && (ifdTagStr == "Model")) {
			m_strImgExifModel= valStr;
			m_strImgExifModel.Trim();
		}


		if ((ifdStr == "SubIFD") && (ifdTagStr == "MakerNote")) {
			// Maker IFD - Pointer
			m_nImgExifMakerPtr = nIfdOffset;
			valStr.Format(_T("@ 0x%04X"),nIfdOffset);
		}


		// -------------------------
		// IFD1 - Embedded Thumbnail
		// -------------------------
		if ((ifdStr == "IFD1") && (ifdTagStr == "Compression")) { //(ifdTagVal == 0x0103)) {
			// Embedded thumbnail, compression format
			m_nImgExifThumbComp = ReadSwap4(m_nPos);
		}
		if ((ifdStr == "IFD1") && (ifdTagStr == "JpegIFOffset")) { //(ifdTagVal == 0x0201)) {
			// Embedded thumbnail, offset
			m_nImgExifThumbOffset = nIfdOffset + pos_exif_start;
			valStr.Format(_T("@ +0x%04X = @ 0x%04X"),nIfdOffset,m_nImgExifThumbOffset);
		}
		if ((ifdStr == "IFD1") && (ifdTagStr == "JpegIFByteCount")) { //(ifdTagVal == 0x0202)) {
			// Embedded thumbnail, length
			m_nImgExifThumbLen = ReadSwap4(m_nPos);
		}

		// -------------------------
		// Determine MakerNote support
		// -------------------------

		if (m_strImgExifMake != "") {
			// 1) Identify the supported MakerNotes
			// 2) Remap variations of the Maker field (e.g. Nikon)
			//    as some manufacturers have been inconsistent in their
			//    use of the Make field

			m_bImgExifMakeSupported = FALSE;
			if (m_strImgExifMake == "Canon") {
				m_bImgExifMakeSupported = TRUE;
			}
			else if (m_strImgExifMake == "PENTAX Corporation") {
				m_strImgExifMake = "PENTAX";
			}
			else if (m_strImgExifMake == "NIKON CORPORATION") {
				m_strImgExifMake = "NIKON";
				m_bImgExifMakeSupported = TRUE;
			}
			else if (m_strImgExifMake == "NIKON") {
				m_strImgExifMake = "NIKON";
				m_bImgExifMakeSupported = TRUE;
			}
			else if (m_strImgExifMake == "SIGMA") {
				m_bImgExifMakeSupported = TRUE;
			}
			else if (m_strImgExifMake == "SONY") {
				m_bImgExifMakeSupported = TRUE;
			}
			else if (m_strImgExifMake == "FUJIFILM") {
				//m_bImgExifMakeSupported = TRUE;
				m_bImgExifMakeSupported = FALSE;
			}
		}


		// Now advance the m_nPos ptr as we have finished with valoffset
		m_nPos+=4;


		// SUMMARY


		// Only print single-line summary if we didn't already
		// print multi-line value summary
		if (!extraDecode)
		{
			if ((!m_pAppConfig->bExifHideUnknown) || (!ifdTagUnknown)) {

				// Now print out the final formatted entry

				// If the tag is an ASCII string, we want to display
				// quote marks around it
				if (ifdFormat == 2) {
					tmpStr.Format(_T("    [%-36s] = \"%s\""),ifdTagStr,valStr);
				} else {
					tmpStr.Format(_T("    [%-36s] = %s"),ifdTagStr,valStr);
				}

				m_pLog->AddLine(tmpStr);

			}
		}

		DbgAddLine(_T(""));

	} // for ifdEntryInd

	m_pLog->Enable();
	return 0;
}


// Handle APP13 marker
// This includes:
// - Photoshop "Save As" and "Save for Web" quality settings
// - IPTC entries
unsigned CjfifDecode::DecodeApp13Ps()
{
	// Photoshop APP13 marker definition doesn't appear to have a 
	// well-defined length, so I will read until I encounter a 
	// non-"8BIM" entry, then we reset the position counter
	// and move on to the next marker.

	CString tmpStr;
	CString bim_name;
	unsigned int	bim_sig,bim_id,bim_name_len,bim_len;
	bool done = false;

	unsigned val = 0x8000;
	unsigned save_format = 0;

	CString iptcStr;
	CString iptc_type_name;
	iptc_field_t iptc_fld;

	while (!done)
	{
		bim_sig = Buf(m_nPos)*0x01000000 + Buf(m_nPos+1)*0x00010000 +
			Buf(m_nPos+2)*0x00000100 + Buf(m_nPos+3);
		// Check for signature "8BIM"
		if (bim_sig == '8BIM') { // 0x3842494D
			m_nPos += 4;
			bim_id = Buf(m_nPos)*256+Buf(m_nPos+1);
			m_nPos+=2;
			bim_name_len = Buf(m_nPos);
			m_nPos+=1;
			bim_name = "";
			if (bim_name_len != 0) {
				bim_name = m_pWBuf->BufReadStrn(m_nPos,bim_name_len);
				m_nPos+=bim_name_len;
				if ((bim_name_len % 2) == 0) m_nPos++;
			} else {
				m_nPos++;
			}
			bim_len = Buf(m_nPos)*0x01000000 + Buf(m_nPos+1)*0x00010000 +
				Buf(m_nPos+2)*0x00000100 + Buf(m_nPos+3);
			m_nPos+=4;

			tmpStr.Format("    8BIM: [0x%04X] Name=[%s] Len=[0x%04X]",bim_id,bim_name,bim_len);
			m_pLog->AddLine(tmpStr);

			switch (bim_id) {
				case 0x0406: // Save As Quality
					// Index 0: Quality level
					val = Buf(m_nPos+0)*256 + Buf(m_nPos+1);
					switch(val) {
						case 0xFFFD: m_nImgQualPhotoshopSa = 1; break;
						case 0xFFFE: m_nImgQualPhotoshopSa = 2; break;
						case 0xFFFF: m_nImgQualPhotoshopSa = 3; break;
						case 0x0000: m_nImgQualPhotoshopSa = 4; break;
						case 0x0001: m_nImgQualPhotoshopSa = 5; break;
						case 0x0002: m_nImgQualPhotoshopSa = 6; break;
						case 0x0003: m_nImgQualPhotoshopSa = 7; break;
						case 0x0004: m_nImgQualPhotoshopSa = 8; break;
						case 0x0005: m_nImgQualPhotoshopSa = 9; break;
						case 0x0006: m_nImgQualPhotoshopSa = 10; break;
						case 0x0007: m_nImgQualPhotoshopSa = 11; break;
						case 0x0008: m_nImgQualPhotoshopSa = 12; break;
						default: m_nImgQualPhotoshopSa = 0; break;
					}
					if (m_nImgQualPhotoshopSa != 0) {
						tmpStr.Format("  Photoshop Save As Quality        = [%d]",m_nImgQualPhotoshopSa);
						m_pLog->AddLine(tmpStr);
					}
					// Index 1: Format
					save_format = Buf(m_nPos+2)*256 + Buf(m_nPos+3);
					switch(save_format) {
						case 0x0000: m_pLog->AddLine(_T("  Photoshop Save Format            = [Standard]")); break;
						case 0x0001: m_pLog->AddLine(_T("  Photoshop Save Format            = [Optimized]")); break;
						case 0x0101: m_pLog->AddLine(_T("  Photoshop Save Format            = [Progressive]")); break;
						default: break;
					}
					// Index 2: Progressive Scans
					// Only print out m_bImgProgressive scan count if that's the format!
					if (save_format == 0x0101) {
						val = Buf(m_nPos+4)*256 + Buf(m_nPos+5);
						switch(val) {
							case 0x0001: m_pLog->AddLine(_T("  Photoshop Save Progressive Scans = [3 Scans]")); break;
							case 0x0002: m_pLog->AddLine(_T("  Photoshop Save Progressive Scans = [4 Scans]")); break;
							case 0x0003: m_pLog->AddLine(_T("  Photoshop Save Progressive Scans = [5 Scans]")); break;
							default: break;
						}
					}
					break;
				case 0x0404:
					// IPTC
					unsigned pos_iptc_saved;
					unsigned seg_type1,seg_type2,seg_sz;
					unsigned iptc_rec_type;
					bool iptc_done,iptc_fld_done,iptc_fld_found;
					unsigned iptc_fld_ind;

					pos_iptc_saved = m_nPos;
					iptc_done = false;

					while (!iptc_done) {
						seg_type1 = Buf(m_nPos++)*256 + Buf(m_nPos++);
						seg_type2 = Buf(m_nPos++);
						seg_sz = Buf(m_nPos++)*256 + Buf(m_nPos++);

						// Now search the IPTC value list to find the
						// specific ID to lookup the format type and
						// field name. The IPTC list is defined in: iptc_fields[]
						iptc_rec_type = 0;
						iptc_fld_found = false;
						iptc_fld_done = false;
						iptc_fld_ind = 0;
						while (!iptc_fld_done) {
							iptc_fld = iptc_fields[iptc_fld_ind];
							if (iptc_fld.id == seg_type2) {
								iptc_fld_done = true;
								iptc_fld_found = true;
							}
							if (iptc_fld.id == 999) {
								// 999 is the ID used to demarcate the end-of-list
								iptc_fld_done = true;
							}
							iptc_fld_ind++;
						}
						if (iptc_fld_found) {
							iptc_rec_type = iptc_fld.fld_type;
							iptc_type_name = iptc_fld.fld_name;
						} else {
							iptc_rec_type = 0;
							iptc_type_name = "???";
						}

						// Now decode the IPTC field format type
						if (iptc_rec_type == 1) {
							// IPTC Format: Half-word
							iptcStr = m_pWBuf->BufReadStrn(m_nPos,seg_sz);
							tmpStr.Format("      IPTC [0x%04X:%03u] %s = [0x%02X%02X]",seg_type1,seg_type2,iptc_type_name,Buf(m_nPos),Buf(m_nPos+1));
							m_pLog->AddLine(tmpStr);
						} else if (iptc_rec_type == 2) {
							// IPTC Format: String
							iptcStr = m_pWBuf->BufReadStrn(m_nPos,seg_sz);
							tmpStr.Format("      IPTC [0x%04X:%03u] %s = [%s]",seg_type1,seg_type2,iptc_type_name,iptcStr);
							m_pLog->AddLine(tmpStr);
						} else {
							// IPTC Format: Unknown
							tmpStr.Format("      IPTC [0x%04X:%03u] ? size=%d",seg_type1,seg_type2,seg_sz);
							m_pLog->AddLine(tmpStr);

						}

						m_nPos += seg_sz;

						// Continue on processing entries until we find an entry that
						// does not begin with the IPTC marker 0x1C02
						if ( (Buf(m_nPos)*256 + Buf(m_nPos+1)) != 0x1C02 ) {
							iptc_done = true;
						}
					}

					// Reset the file marker
					m_nPos = pos_iptc_saved;

					break;
				default:
					break;
			}

			// Skip rest of 8BIM
			m_nPos += bim_len;

			//CAL! correct to make for even parity??
			if ((bim_len % 2) != 0) m_nPos++;

		} else {
			// Not 8BIM???
			done = true;
		}
	}
	return 0;
}

// Start decoding a single ICC header segment @ nPos
unsigned CjfifDecode::DecodeIccHeader(unsigned nPos)
{
	CString tmpStr,tmpStr1;

	// Profile header
	unsigned nProfSz;
	unsigned nPrefCmmType;
	unsigned nProfVer;
	unsigned nProfDevClass;
	unsigned nDataColorSpace;
	unsigned nPcs;
	unsigned nDateTimeCreated[3];
	unsigned nProfFileSig;
	unsigned nPrimPlatSig;
	unsigned nProfFlags;
	unsigned nDevManuf;
	unsigned nDevModel;
	unsigned nDevAttrib[2];
	unsigned nRenderIntent;
	unsigned nIllumPcsXyz[3];
	unsigned nProfCreatorSig;
	unsigned nProfId[4];
	unsigned nRsvd[7];

	// Read in all of the ICC header bytes
	nProfSz = ReadBe4(nPos);nPos+=4;
	nPrefCmmType = ReadBe4(nPos);nPos+=4;
	nProfVer = ReadBe4(nPos);nPos+=4;
	nProfDevClass = ReadBe4(nPos);nPos+=4;
	nDataColorSpace = ReadBe4(nPos);nPos+=4;
	nPcs = ReadBe4(nPos);nPos+=4;
	nDateTimeCreated[2] = ReadBe4(nPos);nPos+=4;
	nDateTimeCreated[1] = ReadBe4(nPos);nPos+=4;
	nDateTimeCreated[0] = ReadBe4(nPos);nPos+=4;
	nProfFileSig = ReadBe4(nPos);nPos+=4;
	nPrimPlatSig = ReadBe4(nPos);nPos+=4;
	nProfFlags = ReadBe4(nPos);nPos+=4;
	nDevManuf = ReadBe4(nPos);nPos+=4;
	nDevModel = ReadBe4(nPos);nPos+=4;
	nDevAttrib[1] = ReadBe4(nPos);nPos+=4;
	nDevAttrib[0] = ReadBe4(nPos);nPos+=4;
	nRenderIntent = ReadBe4(nPos);nPos+=4;
	nIllumPcsXyz[2] = ReadBe4(nPos);nPos+=4;
	nIllumPcsXyz[1] = ReadBe4(nPos);nPos+=4;
	nIllumPcsXyz[0] = ReadBe4(nPos);nPos+=4;
	nProfCreatorSig = ReadBe4(nPos);nPos+=4;
	nProfId[3] = ReadBe4(nPos);nPos+=4;
	nProfId[2] = ReadBe4(nPos);nPos+=4;
	nProfId[1] = ReadBe4(nPos);nPos+=4;
	nProfId[0] = ReadBe4(nPos);nPos+=4;
	nRsvd[6] = ReadBe4(nPos);nPos+=4;
	nRsvd[5] = ReadBe4(nPos);nPos+=4;
	nRsvd[4] = ReadBe4(nPos);nPos+=4;
	nRsvd[3] = ReadBe4(nPos);nPos+=4;
	nRsvd[2] = ReadBe4(nPos);nPos+=4;
	nRsvd[1] = ReadBe4(nPos);nPos+=4;
	nRsvd[0] = ReadBe4(nPos);nPos+=4;

	// Now output the formatted version of the above data structures
	tmpStr.Format(_T("        %-33s : %u bytes"),"Profile Size",nProfSz);
	m_pLog->AddLine(tmpStr);
	
	tmpStr.Format(_T("        %-33s : %s"),"Preferred CMM Type",Uint2Chars(nPrefCmmType));
	m_pLog->AddLine(tmpStr);

	tmpStr.Format(_T("        %-33s : %u.%u.%u.%u (0x%08X)"),"Profile Version",
		((nProfVer & 0xF0000000)>>28),
		((nProfVer & 0x0F000000)>>24),
		((nProfVer & 0x00F00000)>>20),
		((nProfVer & 0x000F0000)>>16),
		nProfVer);
	m_pLog->AddLine(tmpStr);

	switch (nProfDevClass) {
		case 'scnr':
			tmpStr1.Format(_T("Input Device profile"));break;
		case 'mntr':
			tmpStr1.Format(_T("Display Device profile"));break;
		case 'prtr':
			tmpStr1.Format(_T("Output Device profile"));break;
		case 'link':
			tmpStr1.Format(_T("DeviceLink Device profile"));break;
		case 'spac':
			tmpStr1.Format(_T("ColorSpace Conversion profile"));break;
		case 'abst':
			tmpStr1.Format(_T("Abstract profile"));break;
		case 'nmcl':
			tmpStr1.Format(_T("Named colour profile"));break;
		default:
			tmpStr1.Format(_T("? (0x%08X)"),nProfDevClass);
			break;
	}
	tmpStr.Format(_T("        %-33s : %s (%s)"),"Profile Device/Class",tmpStr1,Uint2Chars(nProfDevClass));
	m_pLog->AddLine(tmpStr);

	switch (nDataColorSpace) {
		case 'XYZ ':
			tmpStr1.Format(_T("XYZData"));break;
		case 'Lab ':
			tmpStr1.Format(_T("labData"));break;
		case 'Luv ':
			tmpStr1.Format(_T("lubData"));break;
		case 'YCbr':
			tmpStr1.Format(_T("YCbCrData"));break;
		case 'Yxy ':
			tmpStr1.Format(_T("YxyData"));break;
		case 'RGB ':
			tmpStr1.Format(_T("rgbData"));break;
		case 'GRAY':
			tmpStr1.Format(_T("grayData"));break;
		case 'HSV ':
			tmpStr1.Format(_T("hsvData"));break;
		case 'HLS ':
			tmpStr1.Format(_T("hlsData"));break;
		case 'CMYK':
			tmpStr1.Format(_T("cmykData"));break;
		case 'CMY ':
			tmpStr1.Format(_T("cmyData"));break;
		case '2CLR':
			tmpStr1.Format(_T("2colourData"));break;
		case '3CLR':
			tmpStr1.Format(_T("3colourData"));break;
		case '4CLR':
			tmpStr1.Format(_T("4colourData"));break;
		case '5CLR':
			tmpStr1.Format(_T("5colourData"));break;
		case '6CLR':
			tmpStr1.Format(_T("6colourData"));break;
		case '7CLR':
			tmpStr1.Format(_T("7colourData"));break;
		case '8CLR':
			tmpStr1.Format(_T("8colourData"));break;
		case '9CLR':
			tmpStr1.Format(_T("9colourData"));break;
		case 'ACLR':
			tmpStr1.Format(_T("10colourData"));break;
		case 'BCLR':
			tmpStr1.Format(_T("11colourData"));break;
		case 'CCLR':
			tmpStr1.Format(_T("12colourData"));break;
		case 'DCLR':
			tmpStr1.Format(_T("13colourData"));break;
		case 'ECLR':
			tmpStr1.Format(_T("14colourData"));break;
		case 'FCLR':
			tmpStr1.Format(_T("15colourData"));break;
		default:
			tmpStr1.Format(_T("? (0x%08X)"),nDataColorSpace);
			break;
	}
	tmpStr.Format(_T("        %-33s : %s (%s)"),"Data Colour Space",tmpStr1,Uint2Chars(nDataColorSpace));
	m_pLog->AddLine(tmpStr);

	tmpStr.Format(_T("        %-33s : %s"),"Profile connection space (PCS)",Uint2Chars(nPcs));
	m_pLog->AddLine(tmpStr);

	tmpStr.Format(_T("        %-33s : %s"),"Profile creation date",DecodeIccDateTime(nDateTimeCreated));
	m_pLog->AddLine(tmpStr);

	tmpStr.Format(_T("        %-33s : %s"),"Profile file signature",Uint2Chars(nProfFileSig));
	m_pLog->AddLine(tmpStr);

	switch (nPrimPlatSig) {
		case 'APPL':
			tmpStr1.Format(_T("Apple Computer, Inc."));break;
		case 'MSFT':
			tmpStr1.Format(_T("Microsoft Corporation"));break;
		case 'SGI ':
			tmpStr1.Format(_T("Silicon Graphics, Inc."));break;
		case 'SUNW':
			tmpStr1.Format(_T("Sun Microsystems, Inc."));break;
		default:
			tmpStr1.Format(_T("? (0x%08X)"),nPrimPlatSig);
			break;
	}
	tmpStr.Format(_T("        %-33s : %s (%s)"),"Primary platform",tmpStr1,Uint2Chars(nPrimPlatSig));
	m_pLog->AddLine(tmpStr);

	tmpStr.Format(_T("        %-33s : 0x%08X"),"Profile flags",nProfFlags);
	m_pLog->AddLine(tmpStr);
	tmpStr1 = (TestBit(nProfFlags,0))?"Embedded profile":"Profile not embedded";
	tmpStr.Format(_T("        %-35s > %s"),"Profile flags",tmpStr1);
	m_pLog->AddLine(tmpStr);
	tmpStr1 = (TestBit(nProfFlags,1))?"Profile can be used independently of embedded":"Profile can't be used independently of embedded";
	tmpStr.Format(_T("        %-35s > %s"),"Profile flags",tmpStr1);
	m_pLog->AddLine(tmpStr);

	tmpStr.Format(_T("        %-33s : %s"),"Device Manufacturer",Uint2Chars(nDevManuf));
	m_pLog->AddLine(tmpStr);

	tmpStr.Format(_T("        %-33s : %s"),"Device Model",Uint2Chars(nDevModel));
	m_pLog->AddLine(tmpStr);

	tmpStr.Format(_T("        %-33s : 0x%08X_%08X"),"Device attributes",nDevAttrib[1],nDevAttrib[0]);
	m_pLog->AddLine(tmpStr);
	tmpStr1 = (TestBit(nDevAttrib[0],0))?"Transparency":"Reflective";
	tmpStr.Format(_T("        %-35s > %s"),"Device attributes",tmpStr1);
	m_pLog->AddLine(tmpStr);
	tmpStr1 = (TestBit(nDevAttrib[0],1))?"Matte":"Glossy";
	tmpStr.Format(_T("        %-35s > %s"),"Device attributes",tmpStr1);
	m_pLog->AddLine(tmpStr);
	tmpStr1 = (TestBit(nDevAttrib[0],2))?"Media polarity = positive":"Media polarity = negative";
	tmpStr.Format(_T("        %-35s > %s"),"Device attributes",tmpStr1);
	m_pLog->AddLine(tmpStr);
	tmpStr1 = (TestBit(nDevAttrib[0],3))?"Colour media":"Black & white media";
	tmpStr.Format(_T("        %-35s > %s"),"Device attributes",tmpStr1);
	m_pLog->AddLine(tmpStr);

	switch(nRenderIntent) {
		case 0x00000000:	tmpStr1.Format(_T("Perceptual"));break;
		case 0x00000001:	tmpStr1.Format(_T("Media-Relative Colorimetric"));break;
		case 0x00000002:	tmpStr1.Format(_T("Saturation"));break;
		case 0x00000003:	tmpStr1.Format(_T("ICC-Absolute Colorimetric"));break;
		default:
			tmpStr1.Format(_T("0x%08X"),nRenderIntent);
			break;
	}
	tmpStr.Format(_T("        %-33s : %s"),"Rendering intent",tmpStr1);
	m_pLog->AddLine(tmpStr);

	// PCS illuminant

	tmpStr.Format(_T("        %-33s : %s"),"Profile creator",Uint2Chars(nProfCreatorSig));
	m_pLog->AddLine(tmpStr);

	tmpStr.Format(_T("        %-33s : 0x%08X_%08X_%08X"),"Profile ID",
		nProfId[3],nProfId[2],nProfId[1],nProfId[0]);
	m_pLog->AddLine(tmpStr);

	return 0;
}

// Provide special output formatter for ICC Date/Time
// NOTE: It appears that the nParts had to be decoded in the
//       reverse order from what I had expected, so one should
//       confirm that the byte order / endianness is appropriate.
CString CjfifDecode::DecodeIccDateTime(unsigned nVal[3])
{
	CString dateStr;
	unsigned short nParts[6];
	nParts[0] = (nVal[2] & 0xFFFF0000) >> 16;	// Year
	nParts[1] = (nVal[2] & 0x0000FFFF);			// Mon
	nParts[2] = (nVal[1] & 0xFFFF0000) >> 16;	// Day
	nParts[3] = (nVal[1] & 0x0000FFFF);			// Hour
	nParts[4] = (nVal[0] & 0xFFFF0000) >> 16;	// Min
	nParts[5] = (nVal[0] & 0x0000FFFF);			// Sec
	dateStr.Format(_T("%04u-%02u-%02u %02u:%02u:%02u"),
		nParts[0],nParts[1],nParts[2],nParts[3],nParts[4],nParts[5]);
	return dateStr;
}

// Simple helper routine to test whether an indexed bit is set
bool CjfifDecode::TestBit(unsigned nVal,unsigned nBit)
{
	unsigned nTmp;
	nTmp = (nVal & (1<<nBit));
	if (nTmp != 0) {
		return true;
	} else {
		return false;
	}
}

// Convert between unsigned integer to a 4-byte character string
// (also known as FourCC codes). This field type is used often in
// ICC profile entries.
CString CjfifDecode::Uint2Chars(unsigned nVal)
{
	CString tmpStr;
	char c3,c2,c1,c0;
	c3 = char((nVal & 0xFF000000)>>24);
	c2 = char((nVal & 0x00FF0000)>>16);
	c1 = char((nVal & 0x0000FF00)>>8);
	c0 = char(nVal & 0x000000FF);
	c3 = (c3 == 0)?'.':c3;
	c2 = (c2 == 0)?'.':c2;
	c1 = (c1 == 0)?'.':c1;
	c0 = (c0 == 0)?'.':c0;
	tmpStr.Format(_T("'%c%c%c%c' (0x%08X)"),
		c3,c2,c1,c0,nVal);
	return tmpStr;
}

// Convert between unsigned int and a dotted-byte notation
CString CjfifDecode::Uint2DotByte(unsigned nVal)
{
	CString tmpStr;
	tmpStr.Format(_T("'%u.%u.%u.%u' (0x%08X)"),
		((nVal & 0xFF000000)>>24),
		((nVal & 0x00FF0000)>>16),
		((nVal & 0x0000FF00)>>8),
		(nVal & 0x000000FF),
		nVal);
	return tmpStr;
}

// Parser for APP2 ICC profile marker
unsigned CjfifDecode::DecodeApp2IccProfile(unsigned nLen)
{
	CString tmpStr;
	bool done = false;
	unsigned	nMarkerSeqNum;	// Byte
	unsigned	nNumMarkers;	// Byte
	unsigned	nPayloadLen;	// Len of this ICC marker payload

	unsigned	nMarkerPosStart;

	nMarkerSeqNum = Buf(m_nPos++);
	nNumMarkers = Buf(m_nPos++);
	nPayloadLen = nLen - 2 - 12 - 2; //CAL! check?

	tmpStr.Format(_T("      Marker Number = %u of %u"),nMarkerSeqNum,nNumMarkers);
	m_pLog->AddLine(tmpStr);

	if (nMarkerSeqNum == 1) {
		nMarkerPosStart = m_nPos;
		DecodeIccHeader(nMarkerPosStart);
	} else {
		m_pLog->AddLineWarn(_T("      Only support decode of 1st ICC Marker"));
	}

	return 0;
}

// Parser for APP2 FlashPix marker
unsigned CjfifDecode::DecodeApp2Flashpix()
{

	CString tmpStr;
	bool done = false;

	unsigned	fpx_ver;
	unsigned	fpx_seg_type;
	unsigned	fpx_interop_cnt;
	unsigned	fpx_entity_sz;
	unsigned	fpx_default;

	bool		fpx_storage;
	CString		fpx_storage_clsStr;
	unsigned	fpx_st_index_cont;
	unsigned	fpx_st_offset;

	unsigned	fpx_st_wByteOrder;
	unsigned	fpx_st_wFormat;
	CString		fpx_st_clsidStr;
	unsigned	fpx_st_dwOSVer;
	unsigned	fpx_st_rsvd;

	CString		streamStr;

	fpx_ver = Buf(m_nPos++);
	fpx_seg_type = Buf(m_nPos++);

	// FlashPix segments: Contents List or Stream Data

	if (fpx_seg_type == 1) {
		// Contents List
		tmpStr.Format("    Segment: CONTENTS LIST");
		m_pLog->AddLine(tmpStr);

		fpx_interop_cnt = (Buf(m_nPos++)<<8) + Buf(m_nPos++);
		tmpStr.Format("      Interoperability Count = %u",fpx_interop_cnt);
		m_pLog->AddLine(tmpStr);

		for (unsigned ind=0;ind<fpx_interop_cnt;ind++) {
			tmpStr.Format("      Entity Index #%u",ind); 
			m_pLog->AddLine(tmpStr);
			fpx_entity_sz = (Buf(m_nPos++)<<24) + (Buf(m_nPos++)<<16) + (Buf(m_nPos++)<<8) + Buf(m_nPos++);

			// If the "entity size" field is 0xFFFFFFFF, then it should be treated as
			// a "storage". It looks like we should probably be using this to determine
			// that we have a "storage"
			fpx_storage = false;
			if (fpx_entity_sz == 0xFFFFFFFF) {
				fpx_storage = true;
			}

			if (!fpx_storage) {
				tmpStr.Format("        Entity Size = %u",fpx_entity_sz);
				m_pLog->AddLine(tmpStr);
			} else {
				tmpStr.Format("        Entity is Storage");
				m_pLog->AddLine(tmpStr);
			}

			fpx_default = Buf(m_nPos++);


			//CAL! BUG #1112
			streamStr = m_pWBuf->BufReadUniStr(m_nPos);
			m_nPos += 2*((unsigned)strlen(streamStr)+1); // 2x because unicode

			tmpStr.Format("        Stream Name = [%s]",streamStr);
			m_pLog->AddLine(tmpStr);


			// In the case of "storage", we decode the next 16 bytes as the class
			if (fpx_storage) {

				//CAL! NOTE: *** Very strange reordering required here. Doesn't seem consistent!
				// This means that other fields are probably wrong as well (endian)
				fpx_storage_clsStr.Format("%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
					Buf(m_nPos+3),Buf(m_nPos+2),Buf(m_nPos+1),Buf(m_nPos+0),
					Buf(m_nPos+5),Buf(m_nPos+4),
					Buf(m_nPos+7),Buf(m_nPos+6),
					Buf(m_nPos+8),Buf(m_nPos+9),
					Buf(m_nPos+10),Buf(m_nPos+11),Buf(m_nPos+12),Buf(m_nPos+13),Buf(m_nPos+14),Buf(m_nPos+15) );
				m_nPos+= 16;
				tmpStr.Format("        Storage Class = [%s]",fpx_storage_clsStr);
				m_pLog->AddLine(tmpStr);


			}

		}

		return 0;

	} else if (fpx_seg_type == 2) {
		// Stream Data
		tmpStr.Format("    Segment: STREAM DATA");
		m_pLog->AddLine(tmpStr);

		fpx_st_index_cont = (Buf(m_nPos++)<<8) + Buf(m_nPos++);
		tmpStr.Format("      Index in Contents List = %u",fpx_st_index_cont);
		m_pLog->AddLine(tmpStr);

		fpx_st_offset = (Buf(m_nPos++)<<24) + (Buf(m_nPos++)<<16) + (Buf(m_nPos++)<<8) + Buf(m_nPos++);
		tmpStr.Format("      Offset in stream = %u (0x%08X)",fpx_st_offset,fpx_st_offset);
		m_pLog->AddLine(tmpStr);

		// Now decode the Property Set Header

		//CAL! Should only decode this if we are doing first part of stream
		// *** How do we know this? First reference to index #?

		fpx_st_wByteOrder = (Buf(m_nPos++)<<8) + Buf(m_nPos++);
		fpx_st_wFormat = (Buf(m_nPos++)<<8) + Buf(m_nPos++);
		fpx_st_dwOSVer = (Buf(m_nPos++)<<24) + (Buf(m_nPos++)<<16) + (Buf(m_nPos++)<<8) + Buf(m_nPos++);

		//CAL! NOTE: *** Very strange reordering required here. Doesn't seem consistent!
		// This means that other fields are probably wrong as well (endian)
		fpx_st_clsidStr.Format("%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
			Buf(m_nPos+3),Buf(m_nPos+2),Buf(m_nPos+1),Buf(m_nPos+0),
			Buf(m_nPos+5),Buf(m_nPos+4),
			Buf(m_nPos+7),Buf(m_nPos+6),
			Buf(m_nPos+8),Buf(m_nPos+9),
			Buf(m_nPos+10),Buf(m_nPos+11),Buf(m_nPos+12),Buf(m_nPos+13),Buf(m_nPos+14),Buf(m_nPos+15) );
		m_nPos+= 16;
		fpx_st_rsvd = (Buf(m_nPos++)<<8) + Buf(m_nPos++);

		tmpStr.Format("      ByteOrder = 0x%04X",fpx_st_wByteOrder);
		m_pLog->AddLine(tmpStr);

		tmpStr.Format("      Format = 0x%04X",fpx_st_wFormat);
		m_pLog->AddLine(tmpStr);

		tmpStr.Format("      OSVer = 0x%08X",fpx_st_dwOSVer);
		m_pLog->AddLine(tmpStr);

		tmpStr.Format("      clsid = %s",fpx_st_clsidStr);
		m_pLog->AddLine(tmpStr);

		tmpStr.Format("      reserved = 0x%08X",fpx_st_rsvd);
		m_pLog->AddLine(tmpStr);

		// ....

		return 2;

	} else {
		tmpStr.Format("      Reserved Segment. Stopping.");
		m_pLog->AddLineErr(tmpStr);
		return 1;
	}

}


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
	unsigned	length;
	unsigned	tmp;
	CString		tmpStr,fullStr;
	unsigned	nPosEnd;
	unsigned	nPosSaved;

	bool		bRet;


	if (bInject) {
		// Redirect Buf() to DHT table in MJPGDHTSeg[]
		// ... so change mode that Buf() call uses
		m_bBufFakeDHT = true;

		// Preserve the "m_nPos" pointer, at end we undo it
		// And we also start at 2 which is just after FFC4 in array
		nPosSaved = m_nPos;
		m_nPos = 2;
	}

	length = Buf(m_nPos)*256 + Buf(m_nPos+1);
	nPosEnd = m_nPos+length;
	m_nPos+=2;
	tmpStr.Format(_T("  Huffman table length = %u"),length); 
	m_pLog->AddLine(tmpStr);

	unsigned dht_class,dht_dest_id;

	// In various places, added m_bStateAbort check to allow us
	// to escape in case we get in excessive number of DHT entries
	// See BUG FIX #1003

	while ((!m_bStateAbort)&&(nPosEnd > m_nPos))
	{
		m_pLog->AddLine(_T("  ----"));

		tmp = Buf(m_nPos++);
		dht_class = (tmp & 0xF0) >> 4;
		dht_dest_id = tmp & 0x0F;
		tmpStr.Format(_T("  Destination ID = %u"),dht_dest_id);
		m_pLog->AddLine(tmpStr);
		tmpStr.Format(_T("  Class = %u (%s)"),dht_class,(dht_class?"AC Table":"DC / Lossless Table"));
		m_pLog->AddLine(tmpStr);

		// Add in some error checking to prevent 
		if (dht_class >= 2) {
			tmpStr.Format("ERROR: Invalid DHT Class (%u). Aborting DHT Load.",dht_class);
			m_pLog->AddLineErr(tmpStr);
			m_nPos = nPosEnd;
			//m_bStateAbort = true;	// Stop decoding
			break;
		}
		if (dht_dest_id >= 4) {
			tmpStr.Format("ERROR: Invalid DHT Dest ID (%u). Aborting DHT Load.",dht_dest_id);
			m_pLog->AddLineErr(tmpStr);
			m_nPos = nPosEnd;
			//m_bStateAbort = true;	// Stop decoding
			break;
		}

		// Read in the array of DHT code lengths
		for (unsigned int i=1;i<=16;i++)
		{
			m_anImgDhtCodesLen[i] = Buf(m_nPos++);
		}

		#define DECODE_DHT_MAX_DHT 256

		unsigned int dht_code_list[DECODE_DHT_MAX_DHT+1]; // Should only need max 162 codes
		unsigned int dht_ind;
		unsigned int dht_codes_total;

		// Clear out the code list
		for (dht_ind = 0;dht_ind <DECODE_DHT_MAX_DHT;dht_ind++)
		{
			dht_code_list[dht_ind] = 0xFFFF; // Dummy value
		}

		// Now read in all of the DHT codes according to the lengths
		// read in earlier
		dht_codes_total = 0;
		dht_ind = 0;
		for (unsigned int ind_len=1;((!m_bStateAbort)&&(ind_len<=16));ind_len++)
		{
			// Keep a total count of the number of DHT codes read
			dht_codes_total += m_anImgDhtCodesLen[ind_len];

			fullStr.Format(_T("    Codes of length %02u bits (%03u total): "),ind_len,m_anImgDhtCodesLen[ind_len]);
			for (unsigned int ind_code=0;((!m_bStateAbort)&&(ind_code<m_anImgDhtCodesLen[ind_len]));ind_code++)
			{
				// Start a new line for every 16 codes
				if ( (ind_code != 0) && ((ind_code % 16) == 0) ) {
					fullStr = "                                         ";
				}
				tmp = Buf(m_nPos++);
				tmpStr.Format(_T("%02X "),tmp);
				fullStr += tmpStr;

				// Only write 16 codes per line
				if ((ind_code % 16) == 15) {
					m_pLog->AddLine(fullStr);
					fullStr = "";
				}

				// Save the huffman code
				// Just in case we have more DHT codes than we expect, trap
				// the range check here, otherwise we'll have buffer overrun!
				if (dht_ind < DECODE_DHT_MAX_DHT) {
					dht_code_list[dht_ind++] = tmp;
				} else {
					dht_ind++;
					tmpStr.Format("Excessive DHT entries (%u)... skipping",dht_ind);
					m_pLog->AddLineErr(tmpStr);
					if (!m_bStateAbort) { DecodeErrCheck(true); }
				}

			}
			m_pLog->AddLine(fullStr);
		}
		tmpStr.Format(_T("    Total number of codes: %03u"),dht_codes_total);
		m_pLog->AddLine(tmpStr);

		unsigned int dht_lookup_ind = 0;

		// Now print out the actual binary strings!
		unsigned long bit_val = 0;
		unsigned int code_val = 0;
		dht_ind = 0;
		if (m_pAppConfig->bOutputDHTexpand) {
			m_pLog->AddLine(_T(""));
			m_pLog->AddLine(_T("  Expanded Form of Codes:"));
		}
		for (unsigned int bit_len=1;((!m_bStateAbort)&&(bit_len<=16));bit_len++)
		{
			if (m_anImgDhtCodesLen[bit_len] != 0)
			{
				if (m_pAppConfig->bOutputDHTexpand) {
					tmpStr.Format(_T("    Codes of length %02u bits:"),bit_len);
					m_pLog->AddLine(tmpStr);
				}
				// Codes exist for this bit-length
				// Walk through and generate the bitvalues
				for (unsigned int bit_ind=1;((!m_bStateAbort)&&(bit_ind<=m_anImgDhtCodesLen[bit_len]));bit_ind++)
				{
					unsigned int decval = code_val;
					unsigned int bin_bit;
					char binstr[17] = "";
					unsigned int binstr_len = 0;

					// If the user has enabled output of DHT expanded tables,
					// report the bit-string sequences.
					if (m_pAppConfig->bOutputDHTexpand) {
						for (unsigned int bin_ind=bit_len;bin_ind>=1;bin_ind--)
						{
							bin_bit = (decval >> (bin_ind-1)) & 1;
							binstr[binstr_len++] = (bin_bit)?'1':'0';
						}
						binstr[binstr_len] = '\0';
						fullStr.Format(_T("      %s = %02X"),binstr,dht_code_list[dht_ind]);
						if (dht_code_list[dht_ind] == 0x00) { fullStr += " (EOB)"; }
						if (dht_code_list[dht_ind] == 0xF0) { fullStr += " (ZRL)"; }

						tmpStr.Format("%-40s (Total Len = %2u)",fullStr,bit_len + (dht_code_list[dht_ind] & 0xF));

						m_pLog->AddLine(tmpStr);
					}

					// Store the lookup value
					// Shift left to MSB of 32-bit
					unsigned tmp_mask = m_anMaskLookup[bit_len];

					unsigned tmp_bits = decval << (32-bit_len);
					unsigned tmp_code = dht_code_list[dht_ind];
					bRet = m_pImgDec->SetDhtEntry(dht_dest_id,dht_class,dht_lookup_ind,bit_len,
						tmp_bits,tmp_mask,tmp_code);

					DecodeErrCheck(bRet);

					dht_lookup_ind++;

					// Move to the next code
					code_val++;
					dht_ind++;
				}
			}
			// For each loop iteration (on bit length), we shift the code value
			code_val <<= 1;
		}


		// Now store the dht_lookup_size
		unsigned tmp_size = dht_lookup_ind;
		bRet = m_pImgDec->SetDhtSize(dht_dest_id,dht_class,tmp_size);
		if (!m_bStateAbort) { DecodeErrCheck(bRet); }

		m_pLog->AddLine(_T(""));

	}

	if (bInject) {
		// Restore position (as if we didn't move)
		m_nPos = nPosSaved;
		m_bBufFakeDHT = false;
	}
}


// Check return value of previous call. If failed, then ask
// user if they wish to continue decoding. If no, then flag to
// the decoder that we're done (avoids continuous failures)
void CjfifDecode::DecodeErrCheck(bool bRet)
{
	if (!bRet) {
		if (AfxMessageBox("Do you want continue decoding?",MB_YESNO|MB_ICONQUESTION)== IDNO) {
			m_bStateAbort = true;
		}
	}
}

// This is the primary JFIF marker parser. It reads the
// marker value at the current file position and launches the
// specific parser routine. This routine exits when 
#define DECMARK_OK 0
#define DECMARK_ERR 1
#define DECMARK_EOI 2
unsigned CjfifDecode::DecodeMarker()
{
	char			identifier[256];
	CString			tmpStr;
	CString			fullStr;			// Used for concatenation
	CString			valStr;
	unsigned		length;				// General purpose
	unsigned		tmp;
	unsigned		code;
	unsigned long	nPosEnd;
	unsigned long	nPosSaved;		// General-purpose saved position in file
	unsigned long	pos_exif_start;
	unsigned		nRet;			// General purpose return value
	bool			bRet;


	if (Buf(m_nPos) != 0xFF) {
		if (m_nPos == 0) {
			// Don't give error message if we've already alerted them of AVI
			if (!m_bAvi) {
				tmpStr.Format(_T("NOTE: File did not start with JPEG marker. Consider using [Tools->Img Search Fwd] to locate embedded JPEG."));
				m_pLog->AddLineErr(tmpStr);
			}
		} else {
			tmpStr.Format(_T("ERROR: Expected marker 0xFF, got 0x%02X @ offset 0x%08X. Consider using [Tools->Img Search Fwd/Rev]."),Buf(m_nPos),m_nPos);
			m_pLog->AddLineErr(tmpStr);
		}
		m_nPos++;
		return DECMARK_ERR;
	}
	m_nPos++;

	bool bSkipMarkerPad = true;
	unsigned nSkipMarkerPad = 0;
	while (bSkipMarkerPad) {
		code = Buf(m_nPos++);
		if (code == 0xFF) {
			// This is an extra pad!
			nSkipMarkerPad++;
		} else {
			bSkipMarkerPad = false;
		}
	}

	if (nSkipMarkerPad>0) {
		tmpStr.Format("*** Skipped %u marker pad bytes ***",nSkipMarkerPad);
		m_pLog->AddLineHdr(tmpStr);
	}

	AddHeader(code);

	switch (code)
	{
	case JFIF_SOI: // SOI
		m_bStateSoi = true;
		break;


	case JFIF_APP12:
		// Photoshop DUCKY (Save For Web)
		length = Buf(m_nPos)*256 + Buf(m_nPos+1);
		//length = m_pWBuf->BufX(m_nPos,2,!m_nImgExifEndian);
		tmpStr.Format(_T("  length          = %u"),length);
		m_pLog->AddLine(tmpStr);

		nPosSaved = m_nPos;

		m_nPos += 2; // Move past length now that we've used it

		strncpy(identifier,m_pWBuf->BufReadStr(m_nPos),20);
		identifier[19] = 0; // Null terminate just in case
		tmpStr.Format(_T("  Identifier      = [%s]"),identifier);
		m_pLog->AddLine(tmpStr);
		m_nPos += (unsigned)strlen(identifier)+1;
		if (strcmp(identifier,"Ducky") != 0)
		{
			m_pLog->AddLine(_T("    Not Photoshop DUCKY. Skipping remainder."));
		}
		else // Photoshop
		{
			// Please see reference on http://cpan.uwinnipeg.ca/htdocs/Image-ExifTool/Image/ExifTool/APP12.pm.html
			// A direct indexed approach should be safe
			m_nImgQualPhotoshopSfw = Buf(m_nPos+6);
			tmpStr.Format("  Photoshop Save For Web Quality = [%d]",m_nImgQualPhotoshopSfw);
			m_pLog->AddLine(tmpStr);
		}
		// Restore original position in file to a point
		// after the section
		m_nPos = nPosSaved+length;
		break;

	case JFIF_APP13:
		// Photoshop (Save As)
		length = Buf(m_nPos)*256 + Buf(m_nPos+1);
		//length = m_pWBuf->BufX(m_nPos,2,!m_nImgExifEndian);
		tmpStr.Format(_T("  length          = %u"),length);
		m_pLog->AddLine(tmpStr);

		nPosSaved = m_nPos;

		m_nPos += 2; // Move past length now that we've used it

		strncpy(identifier,m_pWBuf->BufReadStr(m_nPos),20);
		identifier[19] = 0; // Null terminate just in case
		tmpStr.Format(_T("  Identifier      = [%s]"),identifier);
		m_pLog->AddLine(tmpStr);
		m_nPos += (unsigned)strlen(identifier)+1;
		if (strcmp(identifier,"Photoshop 3.0") != 0)
		{
			m_pLog->AddLine(_T("    Not Photoshop. Skipping remainder."));
		}
		else // Photoshop
		{
			DecodeApp13Ps();
		}
		// Restore original position in file to a point
		// after the section
		m_nPos = nPosSaved+length;
		break;

	case JFIF_APP1:
		length = Buf(m_nPos)*256 + Buf(m_nPos+1);
		//length = m_pWBuf->BufX(m_nPos,2,!m_nImgExifEndian);
		tmpStr.Format(_T("  length          = %u"),length);
		m_pLog->AddLine(tmpStr);

		nPosSaved = m_nPos;

		m_nPos += 2; // Move past length now that we've used it



		strncpy(identifier,m_pWBuf->BufReadStr(m_nPos),255);
		identifier[254] = 0; // Null terminate just in case
		tmpStr.Format(_T("  Identifier      = [%s]"),identifier);
		m_pLog->AddLine(tmpStr);
		m_nPos += (unsigned)strlen(identifier);


		if (!strncmp(identifier,"http://ns.adobe.com/xap/1.0/\x00",29) != 0) {
			// XMP

			m_pLog->AddLine("    XMP = ");

			m_nPos++;

			unsigned nPosMarkerEnd = nPosSaved+length-1;
			unsigned sXmpLen = nPosMarkerEnd-m_nPos;
			char cXmpChar;
			bool bNonSpace;
			CString sLine;

			// Reset state
			sLine = "          |";
			bNonSpace = false;

			for (unsigned nInd=0;nInd<sXmpLen;nInd++) {

				// Get the next char
				cXmpChar = (char)m_pWBuf->Buf(m_nPos+nInd);

				// Detect a non-space in line
				if ((cXmpChar != 0x20) && (cXmpChar != 0x0A)) {
					bNonSpace = true;
				}

				// Detect Linefeed, print out line
				if (cXmpChar == 0x0A) {
					// Only print line if some non-space elements!
					if (bNonSpace) {
						m_pLog->AddLine(sLine);
					}
					// Reset state
					sLine = "          |";
					bNonSpace = false;
				} else {
					// Add the char
					sLine.AppendChar(cXmpChar);
				}
			}
		}
		else if (!strcmp(identifier,"Exif") != 0)
		{
			// Only decode it further if it is EXIF format

			m_nPos += 2; // Skip two 00 bytes


			pos_exif_start = m_nPos; // Save m_nPos @ start of EXIF used for all IFD offsets

			unsigned char identifier_tiff[9];
			fullStr = "";
			tmpStr = "";

			fullStr = "  Identifier TIFF = ";
			for (unsigned int i=0;i<8;i++) {
				identifier_tiff[i] = (unsigned char)Buf(m_nPos++);
			}
			tmpStr = PrintAsHexUC(identifier_tiff,8);
			fullStr += tmpStr;
			m_pLog->AddLine(fullStr);


			switch (identifier_tiff[0]*256+identifier_tiff[1])
			{
			case 0x4949: // "II"
				// Intel alignment
				m_nImgExifEndian = 0;
				m_pLog->AddLine(_T("  Endian          = Intel (little)"));
				break;
			case 0x4D4D: // "MM"
				// Motorola alignment
				m_nImgExifEndian = 1;
				m_pLog->AddLine(_T("  Endian          = Motorola (big)"));
				break;
			}

			// We expect the TAG mark of 0x002A (depending on endian mode)
			unsigned test_002a;
			test_002a = ByteSwap2(identifier_tiff[2],identifier_tiff[3]);
			tmpStr.Format(_T("  TAG Mark x002A  = 0x%04X"),test_002a);
			m_pLog->AddLine(tmpStr);

			unsigned ifd_count;     // Current IFD #
			unsigned offset_ifd1;

			// Mark pointer to EXIF Sub IFD as 0 so that we can
			// detect if the tag never showed up.
			m_nImgExifSubIfdPtr = 0;
			m_nImgExifMakerPtr = 0;
			m_nImgExifGpsIfdPtr = 0;
			m_nImgExifInteropIfdPtr = 0;

			bool exif_done = FALSE;

			offset_ifd1 = ByteSwap4(identifier_tiff[4],identifier_tiff[5],
				identifier_tiff[6],identifier_tiff[7]);

			ifd_count = 0;
			while (!exif_done) {

				m_pLog->AddLine(_T(""));

				tmpStr.Format(_T("IFD%u"),ifd_count);

				// Process the IFD
				nRet = DecodeExifIfd(tmpStr,pos_exif_start,offset_ifd1);

				// Now that we have gone through all entries in the IFD directory,
				// we read the offset to the next IFD
				offset_ifd1 = ByteSwap4(Buf(m_nPos+0),Buf(m_nPos+1),Buf(m_nPos+2),Buf(m_nPos+3));
				m_nPos += 4;


				tmpStr.Format(_T("    Offset to Next IFD = 0x%08X"),offset_ifd1);
				m_pLog->AddLine(tmpStr);


				if (nRet != 0) {
					// Error condition (DecodeExifIfd returned error)
					offset_ifd1 = 0x00000000;
				}


				if (offset_ifd1 == 0x00000000) {
					// Either error condition or truly end of IFDs
					exif_done = TRUE;
				} else {
					ifd_count++;
				}

			} // while ! exif_done

			// If EXIF SubIFD was defined, then handle it now
			if (m_nImgExifSubIfdPtr != 0)
			{
				m_pLog->AddLine(_T(""));
				DecodeExifIfd(_T("SubIFD"),pos_exif_start,m_nImgExifSubIfdPtr);
			}
			if (m_nImgExifMakerPtr != 0)
			{
				m_pLog->AddLine(_T(""));
				DecodeExifIfd(_T("MakerIFD"),pos_exif_start,m_nImgExifMakerPtr);
			}
			if (m_nImgExifGpsIfdPtr != 0)
			{
				m_pLog->AddLine(_T(""));
				DecodeExifIfd(_T("GPSIFD"),pos_exif_start,m_nImgExifGpsIfdPtr);
			}
			if (m_nImgExifInteropIfdPtr != 0)
			{
				m_pLog->AddLine(_T(""));
				DecodeExifIfd(_T("InteropIFD"),pos_exif_start,m_nImgExifInteropIfdPtr);
			}

		} else {
			tmpStr.Format(_T("Identifier [%s] not supported. Skipping remainder."));
			m_pLog->AddLine(tmpStr);
		}

		//////////


		// Dump out Makernote area
#if 1
		unsigned ptr_base;

		if (bVerbose)
		{
			if (m_nImgExifMakerPtr != 0)
			{
				//CAL! BUG! Seems that pos_exif_start is not initialized in VERBOSE mode
				ptr_base = pos_exif_start+m_nImgExifMakerPtr;

				m_pLog->AddLine(_T("Exif Maker IFD DUMP"));
				fullStr.Format(_T("  MarkerOffset @ 0x%08X"),ptr_base);
				m_pLog->AddLine(fullStr);
			}
		}
#endif

		// End of dump out makernote area



		// Restore file position
		m_nPos = nPosSaved;


		// Restore original position in file to a point
		// after the section
		m_nPos = nPosSaved+length;

		break;

	case JFIF_APP2:
		// Typically used for Flashpix and possibly ICC profiles
		// Photoshop (Save As)
		length = Buf(m_nPos)*256 + Buf(m_nPos+1);
		//length = m_pWBuf->BufX(m_nPos,2,!m_nImgExifEndian);
		tmpStr.Format(_T("  length          = %u"),length);
		m_pLog->AddLine(tmpStr);

		nPosSaved = m_nPos;

		m_nPos += 2; // Move past length now that we've used it

		strncpy(identifier,m_pWBuf->BufReadStr(m_nPos),20);
		identifier[19] = 0; // Null terminate just in case
		tmpStr.Format(_T("  Identifier      = [%s]"),identifier);
		m_pLog->AddLine(tmpStr);
		m_nPos += (unsigned)strlen(identifier)+1;
		if (strcmp(identifier,"FPXR") == 0) {
			// Photoshop
			m_pLog->AddLine(_T("    FlashPix:"));
			DecodeApp2Flashpix();
		} else if (strcmp(identifier,"ICC_PROFILE") == 0) {
			// ICC Profile
			m_pLog->AddLine(_T("    ICC Profile:"));
			DecodeApp2IccProfile(length);
		} else {
			m_pLog->AddLine(_T("    Not supported. Skipping remainder."));
		}
		// Restore original position in file to a point
		// after the section
		m_nPos = nPosSaved+length;
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
	case JFIF_APP14:
	case JFIF_APP15:
		length = Buf(m_nPos)*256 + Buf(m_nPos+1);
		//length = m_pWBuf->BufX(m_nPos,2,!m_nImgExifEndian);
		tmpStr.Format(_T("  length     = %u"),length);
		m_pLog->AddLine(tmpStr);

		if (bVerbose)
		{

			fullStr = "";
			for (unsigned int i=0;i<length;i++)
			{
				// Start a new line for every 16 codes
				if ((i % 16) == 0) {
					fullStr.Format(_T("  MarkerOffset [%04X]: "),i);
				} else if ((i % 8) == 0) {
					fullStr += "  ";
				}
				tmp = Buf(m_nPos+i);
				tmpStr.Format(_T("%02X "),tmp);
				fullStr += tmpStr;
				if ((i%16) == 15) {
					m_pLog->AddLine(fullStr);
					fullStr = "";
				}
			}
			m_pLog->AddLine(fullStr);

			fullStr = "";
			for (unsigned int i=0;i<length;i++)
			{
				// Start a new line for every 16 codes
				if ((i % 32) == 0) {
					fullStr.Format(_T("  MarkerOffset [%04X]: "),i);
				} else if ((i % 8) == 0) {
					fullStr += " ";
				}
				tmp = Buf(m_nPos+i);
				if (_istprint(tmp)) {
					tmpStr.Format(_T("%c"),tmp);
					fullStr += tmpStr;
				} else {
					fullStr += ".";
				}
				if ((i%32)==31) {
					m_pLog->AddLine(fullStr);
				}
			}
			m_pLog->AddLine(fullStr);

		} // nVerbose

		m_nPos += length;
		break;

	case JFIF_APP0: // APP0

		length = Buf(m_nPos)*256 + Buf(m_nPos+1);
		//length = m_pWBuf->BufX(m_nPos,2,!m_nImgExifEndian);
		m_nPos+=2;
		tmpStr.Format(_T("  length     = %u"),length);
		m_pLog->AddLine(tmpStr);

		strncpy(m_strApp0Identifier,m_pWBuf->BufReadStr(m_nPos),20);
		identifier[19] = 0; // Null terminate just in case
		tmpStr.Format(_T("  identifier = [%s]"),m_strApp0Identifier);
		m_pLog->AddLine(tmpStr);

		if (!strcmp(m_strApp0Identifier,"JFIF"))
		{
			// Only process remainder if it is JFIF. This marker
			// is also used for application-specific functions.

			m_nPos += (unsigned)(strlen(m_strApp0Identifier)+1);

			m_nImgVersionMajor = Buf(m_nPos++);
			m_nImgVersionMinor = Buf(m_nPos++);
			tmpStr.Format(_T("  version    = [%u.%u]"),m_nImgVersionMajor,m_nImgVersionMinor);
			m_pLog->AddLine(tmpStr);

			m_nImgUnits = Buf(m_nPos++);

			m_nImgDensityX = Buf(m_nPos)*256 + Buf(m_nPos+1);
			//m_nImgDensityX = m_pWBuf->BufX(m_nPos,2,!m_nImgExifEndian);
			m_nPos+=2;
			m_nImgDensityY = Buf(m_nPos)*256 + Buf(m_nPos+1);
			//m_nImgDensityY = m_pWBuf->BufX(m_nPos,2,!m_nImgExifEndian);
			m_nPos+=2;
			tmpStr.Format(_T("  density    = %u x %u "),m_nImgDensityX,m_nImgDensityY);
			fullStr = tmpStr;

			switch (m_nImgUnits)
			{
			case 0:
				fullStr += "(aspect ratio)";
				m_pLog->AddLine(fullStr);
				break;
			case 1:
				fullStr += "DPI (dots per inch)";
				m_pLog->AddLine(fullStr);
				break;
			case 2:
				fullStr += "DPcm (dots per cm)";
				m_pLog->AddLine(fullStr);
				break;
			default:
				tmpStr.Format(_T("ERROR: Unknown m_nImgUnits parameter [%u]"),m_nImgUnits);
				fullStr += tmpStr;
				m_pLog->AddLineWarn(fullStr);
				//return DECMARK_ERR;
				break;
			}


			m_nImgThumbSizeX = Buf(m_nPos++);
			m_nImgThumbSizeY = Buf(m_nPos++);
			tmpStr.Format(_T("  thumbnail  = %u x %u"),m_nImgThumbSizeX,m_nImgThumbSizeY);
			m_pLog->AddLine(tmpStr);

			// Unpack the thumbnail:
			unsigned thumbnail_r,thumbnail_g,thumbnail_b;
			if (m_nImgThumbSizeX && m_nImgThumbSizeY) {
				for (unsigned y=0;y<m_nImgThumbSizeY;y++) {
					fullStr.Format(_T("   Thumb[%03u] = "),y);
					for (unsigned x=0;x<m_nImgThumbSizeX;x++) {
						thumbnail_r = Buf(m_nPos++);
						thumbnail_g = Buf(m_nPos++);
						thumbnail_b = Buf(m_nPos++);
						tmpStr.Format(_T("(0x%02X,0x%02X,0x%02X) "),thumbnail_r,thumbnail_g,thumbnail_b);
						fullStr += tmpStr;
						m_pLog->AddLine(fullStr);
					}
				}
			}

			//CAL! TODO:
			// - In JPEG-B mode (GeoRaster), we will need to fake out
			//   the DHT & DQT tables here. Unfortunately, we'll have to
			//   rely on the user to put us into this mode as there is nothing
			//   in the file that specifies this mode.

			/*
			//CAL! Need to ensure that Faked DHT is correct table

			AddHeader(JFIF_DHT_FAKE);
			DecodeDHT(true);
			// Need to mark DHT tables as OK
			m_bStateDht = true;
			m_bStateDhtFake = true;
			m_bStateDhtOk = true;

			//CAL! ... same for DQT
			*/



		} else if (!strncmp(m_strApp0Identifier,"AVI1",4))
		{
			// AVI MJPEG type

			// Need to fill in predefined DHT table from spec:
			//   OpenDML file format for AVI, section "Proposed Data Chunk Format"
			//   Described in MMREG.H
			m_pLog->AddLine(_T("  Detected MotionJPEG"));
			m_pLog->AddLine(_T("  Importing standard Huffman table..."));
			m_pLog->AddLine(_T(""));

			AddHeader(JFIF_DHT_FAKE);

			DecodeDHT(true);
			// Need to mark DHT tables as OK
			m_bStateDht = true;
			m_bStateDhtFake = true;
			m_bStateDhtOk = true;

			m_nPos += length-2; // Skip over, and undo length short read


		} else {
			// Not JFIF or AVI1
			m_pLog->AddLine(_T("    Not known APP0 type. Skipping remainder."));
			m_nPos += length-2;
		}

		break;

	case JFIF_DQT:  // Define quantization tables
		m_bStateDqt = true;
		unsigned dqt_precision;
		unsigned dqt_dest_id;
		length = Buf(m_nPos)*256 + Buf(m_nPos+1);
		nPosEnd = m_nPos+length;
		m_nPos+=2;
		tmpStr.Format(_T("  Table length = %u"),length); 
		m_pLog->AddLine(tmpStr);

		while (nPosEnd > m_nPos)
		{
			tmpStr.Format(_T("  ----"));
			m_pLog->AddLine(tmpStr);

			tmp = Buf(m_nPos++);
			dqt_precision = (tmp & 0xF0) >> 4;
			dqt_dest_id = tmp & 0x0F;
			tmpStr.Format(_T("  Precision=%u bits"),(dqt_precision)?16:8);
			m_pLog->AddLine(tmpStr);
			tmpStr.Format(_T("  Destination ID=%u"),dqt_dest_id);
			if (dqt_dest_id == 0) {
				tmpStr += " (Luminance)";
			}
			else if (dqt_dest_id == 1) {
				tmpStr += " (Chrominance)";
			}
			else if (dqt_dest_id == 2) {
				tmpStr += " (Chrominance)";
			}
			else {
				tmpStr += " (???)";
			}
			m_pLog->AddLine(tmpStr);


			if (dqt_dest_id >= 4) {
				tmpStr.Format(_T("ERROR: dqt_dest_id = %u, >= 4"),dqt_dest_id);
				m_pLog->AddLineErr(tmpStr);
				return DECMARK_ERR;
			}

			unsigned quant_allones = 1;
			double comparePcnt;
			double cum_sum=0;
			double cum_sum2=0;


			for (unsigned ind=0;ind<=63;ind++)
			{
				tmp = Buf(m_nPos++);
				if (dqt_precision) {
					// 16-bit DQT entries!
					tmp <<= 8;
					tmp += Buf(m_nPos++);
				}
				m_anImgDqt[dqt_dest_id][m_sZigZag[ind]] = tmp;


				/* scaling factor in percent */

				// Now calculate the comparison with the Annex sample

				//CAL! *** Should probably use check for landscape orientation and
				//         rotate comparison matrix accordingly!!!

				if (dqt_dest_id == 0) {
					if (m_anImgDqt[dqt_dest_id][m_sZigZag[ind]] != 0) {
						m_afStdQuantLumCompare[m_sZigZag[ind]] =
							(float)(m_sStdQuantLum[m_sZigZag[ind]]) /
							(float)(m_anImgDqt[dqt_dest_id][m_sZigZag[ind]]);
						comparePcnt = 100.0 *
							(double)(m_anImgDqt[dqt_dest_id][m_sZigZag[ind]]) /
							(double)(m_sStdQuantLum[m_sZigZag[ind]]);
					} else {
						m_afStdQuantLumCompare[m_sZigZag[ind]] = (float)999.99;
						comparePcnt = 999.99;
					}
				} else {
					if (m_anImgDqt[dqt_dest_id][m_sZigZag[ind]] != 0) {
						m_afStdQuantChrCompare[m_sZigZag[ind]] =
							(float)(m_sStdQuantChr[m_sZigZag[ind]]) /
							(float)(m_anImgDqt[dqt_dest_id][m_sZigZag[ind]]);
						comparePcnt = 100.0 *
							(double)(m_anImgDqt[dqt_dest_id][m_sZigZag[ind]]) /
							(double)(m_sStdQuantChr[m_sZigZag[ind]]);

					} else {
						m_afStdQuantChrCompare[m_sZigZag[ind]] = (float)999.99;
					}
				}

				cum_sum += comparePcnt;
				cum_sum2 += comparePcnt * comparePcnt;

				// Check just in case entire table are ones (Quality 100)
				if (m_anImgDqt[dqt_dest_id][m_sZigZag[ind]] != 1) quant_allones = 0;


			} // 0..63

			// Note that the DQT table that we are saving is already
			// after doing zigzag reordering:
			// From high freq -> low freq
			// To X,Y, left-to-right, top-to-bottom

			// Flag this DQT table as being set!
			m_bImgDqtSet[dqt_dest_id] = true;

			// Now display the table
			for (unsigned y=0;y<8;y++) {
				fullStr.Format(_T("    DQT, Row #%u: "),y);
				for (unsigned x=0;x<8;x++) {
					tmpStr.Format(_T("%3u "),m_anImgDqt[dqt_dest_id][y*8+x]);
					fullStr += tmpStr;

					// Store the DQT entry into the Image Decoder
					bRet = m_pImgDec->SetDqtEntry(dqt_dest_id,y*8+x,m_sUnZigZag[y*8+x],m_anImgDqt[dqt_dest_id][y*8+x]);
					DecodeErrCheck(bRet);
				}

				// Now add the compare with Annex K
				// Decided to disable this as it was confusing users
				/*
				fullStr += "   AnnexRatio: <";
				for (unsigned x=0;x<8;x++) {
					if (dqt_dest_id == 0) {
						tmpStr.Format(_T("%5.1f "),m_afStdQuantLumCompare[y*8+x]);
					} else {
						tmpStr.Format(_T("%5.1f "),m_afStdQuantChrCompare[y*8+x]);
					}
					fullStr += tmpStr;
				}
				fullStr += ">";
				*/

				m_pLog->AddLine(fullStr);

			}



			// Perform some statistical analysis of the quality factor
			// to determine the likelihood of the current quantization
			// table being a scaled version of the "standard" tables.
			// If the variance is high, it is unlikely to be the case.
			double qual, var;
			cum_sum /= 64.0;	/* mean scale factor */
			cum_sum2 /= 64.0;
			var = cum_sum2 - (cum_sum * cum_sum); /* variance */

			// Generate the equivalent IJQ "quality" factor
			if (quant_allones)		/* special case for all-ones table */
				qual = 100.0;
			else if (cum_sum <= 100.0)
				qual = (200.0 - cum_sum) / 2.0;
			else
				qual = 5000.0 / cum_sum;

			// Save the quality rating for later
			m_adImgDqtQual[dqt_dest_id] = qual;

			tmpStr.Format(_T("    Approx quality factor = %.2f (scaling=%.2f variance=%.2f)"),
				qual,cum_sum,var);
			m_pLog->AddLine(tmpStr);


		}
		m_bStateDqtOk = true;

		break;

	case JFIF_SOF0: // SOF0 (Baseline)
	case JFIF_SOF1: // SOF1 (Extended sequential)
	case JFIF_SOF2: // SOF2 (Progressive)

		m_bStateSof = true;

		// If it is a progressive scan, then we note this so that we
		// don't try to do an img decode -- not supported yet.
		if (code == JFIF_SOF2) {
			m_bImgProgressive = true;
		}


		length = Buf(m_nPos)*256 + Buf(m_nPos+1);
		m_nPos+=2;
		tmpStr.Format(_T("  Frame header length = %u"),length); 
		m_pLog->AddLine(tmpStr);

		m_nImgPrecision = Buf(m_nPos++);
		tmpStr.Format(_T("  Precision = %u"),m_nImgPrecision);
		m_pLog->AddLine(tmpStr);

		m_nImgNumLines = Buf(m_nPos)*256 + Buf(m_nPos+1);
		m_nPos += 2;
		tmpStr.Format(_T("  Number of Lines = %u"),m_nImgNumLines);
		m_pLog->AddLine(tmpStr);

		m_nImgSampsPerLine = Buf(m_nPos)*256 + Buf(m_nPos+1);
		m_nPos += 2; 
		tmpStr.Format(_T("  Samples per Line = %u"),m_nImgSampsPerLine);
		m_pLog->AddLine(tmpStr);
		tmpStr.Format(_T("  Image Size = %u x %u"),m_nImgSampsPerLine,m_nImgNumLines);
		m_pLog->AddLine(tmpStr);

		// Determine orientation
		//   m_nImgSampsPerLine = X
		//   m_nImgNumLines = Y
		m_nImgLandscape = ENUM_LANDSCAPE_YES;
		if (m_nImgNumLines > m_nImgSampsPerLine)
			m_nImgLandscape = ENUM_LANDSCAPE_NO;
		tmpStr.Format(_T("  Raw Image Orientation = %s"),(m_nImgLandscape==ENUM_LANDSCAPE_YES)?"Landscape":"Portrait");
		m_pLog->AddLine(tmpStr);

		m_nImgSofCompNum = Buf(m_nPos++);
		tmpStr.Format(_T("  Number of Img components = %u"),m_nImgSofCompNum);
		m_pLog->AddLine(tmpStr);

		m_nImgSampFactorXMax = 0;
		m_nImgSampFactorYMax = 0;
		unsigned f_samp_factor[4];

		for (unsigned i=1;((!m_bStateAbort)&&(i<=m_nImgSofCompNum));i++)
		{
			m_anImgQuantCompId[i-1] = Buf(m_nPos++);
			f_samp_factor[i-1] = Buf(m_nPos++);
			m_anImgQuantTblSel[i-1] = Buf(m_nPos++);

			m_anImgSampFactorX[i-1] = (f_samp_factor[i-1] & 0xF0) >> 4;
			m_anImgSampFactorY[i-1] = (f_samp_factor[i-1] & 0x0F);

			m_nImgSampFactorXMax = max(m_nImgSampFactorXMax,m_anImgSampFactorX[i-1]);
			m_nImgSampFactorYMax = max(m_nImgSampFactorYMax,m_anImgSampFactorY[i-1]);

			// Store the DQT Table selection for the Image Decoder
			bRet = m_pImgDec->SetDqtTables(i-1,m_anImgQuantTblSel[i-1]);
			DecodeErrCheck(bRet);

			// Store the Precision (to handle 12-bit decode)
			m_pImgDec->SetPrecision(m_nImgPrecision);

		}




		for (unsigned i=1;((!m_bStateAbort)&&(i<=m_nImgSofCompNum));i++)
		{
			fullStr.Format(_T("    Component[%u]: "),i);
			tmpStr.Format(_T("ID=0x%02X, Samp Fac=0x%02X (Subsamp %u x %u), Quant Tbl Sel=0x%02X"),
				m_anImgQuantCompId[i-1],f_samp_factor[i-1],
				m_nImgSampFactorXMax/m_anImgSampFactorX[i-1],m_nImgSampFactorYMax/m_anImgSampFactorY[i-1],m_anImgQuantTblSel[i-1]);
			fullStr += tmpStr;

			if (m_anImgQuantCompId[i-1] == 1) {
				fullStr += " (Lum: Y)";
			}
			else if (m_anImgQuantCompId[i-1] == 2) {
				fullStr += " (Chrom: Cb)";
			}
			else if (m_anImgQuantCompId[i-1] == 3) {
				fullStr += " (Chrom: Cr)";
			}
			else {
				fullStr += " (???)";
			}
			m_pLog->AddLine(fullStr);

		}

		if (!m_bStateAbort) {
			// For some strange reason, the monochrome images in my Blade_CP_Pro_Manual.pdf
			// have the following sampling setup:
			//  Number of Img components = 1
			//    Component[1]: ID=0x01, Samp Fac=0x22 (Subsamp 1 x 1), Quant Tbl Sel=0x00 (Lum: Y)
			// Most normal Grayscale images show a sampling factor of 0x11!
			// The above seems to mess up my decoder as it uses CssX=CssY=2
			// Let's try to correct for this here
			if (m_nImgSofCompNum == 1) {
				if (m_anImgSampFactorX[0] == m_anImgSampFactorY[0]) {
					if (m_anImgSampFactorX[0] != 1) {
						m_anImgSampFactorX[0] = 1;
						m_anImgSampFactorY[0] = 1;

						m_nImgSampFactorXMax = 1;
						m_nImgSampFactorYMax = 1;
						m_pLog->AddLineWarn("    Altering odd sampling factor for Monochrome.");
					}
				}
			}


			// Now mark the image as been somewhat OK (ie. should
			// also be suitable for EmbeddedThumb() and PrepareSignature()
			m_bImgOK = true;

			m_bStateSofOk = true;

		}
		break;

		//case JFIF_SOF1:
		//case JFIF_SOF2:
	case JFIF_SOF3:
	case JFIF_SOF5:
	case JFIF_SOF6:
	case JFIF_SOF7:
		//case JFIF_JPG:
	case JFIF_SOF9:
	case JFIF_SOF10:
	case JFIF_SOF11:
	case JFIF_SOF12:
	case JFIF_SOF13:
	case JFIF_SOF14:
	case JFIF_SOF15:
		m_bStateSof = true;
		length = Buf(m_nPos)*256 + Buf(m_nPos+1);
		tmpStr.Format(_T("  Frame header length = %u"),length); 
		m_pLog->AddLine(tmpStr);
		m_pLog->AddLineWarn(_T("  *** WARNING: Only handle Baseline, Extended & Progressive DCT currently *** skipping..."));
		m_nPos += length;
		m_bStateSofOk = false;
		break;

	case JFIF_COM: // COM
		length = Buf(m_nPos)*256 + Buf(m_nPos+1);
		m_nPos+=2;
		tmpStr.Format(_T("  Comment length = %u"),length); 
		m_pLog->AddLine(tmpStr);

		// Check for JPEG COM vulnerability
		//   http://marc.info/?l=bugtraq&m=109524346729948
		// Note that the recovery is not very graceful. It will assume that the
		// field is actually zero-length, which will make the next byte trigger the
		// "Expected marker 0xFF" error message and probably abort. There is no
		// obvious way to 

		if ( (length == 0) || (length == 1) ) {
			tmpStr.Format("    JPEG Comment Field Vulnerability detected!");
			m_pLog->AddLineErr(tmpStr);
			tmpStr.Format("    Skipping data until next marker...");
			m_pLog->AddLineErr(tmpStr);
			length = 2;

			bool bDoneSearch = false;
			unsigned nSkipStart = m_nPos;
			while (!bDoneSearch) {
                if (Buf(m_nPos) != 0xFF) {
					m_nPos++;
				} else {
					bDoneSearch = true;
				}
				if (m_nPos >= m_pWBuf->pos_eof) {
					bDoneSearch = true;
				}
			}
			tmpStr.Format("    Skipped %u bytes",m_nPos - nSkipStart);
			m_pLog->AddLineErr(tmpStr);

			// Break out of case statement
			break;
		}

		// Assume COM field valid length (ie. >= 2)
		fullStr = "    Comment=";
		m_strComment = "";
		for (unsigned ind=0;ind<length-2;ind++)
		{
			tmp = Buf(m_nPos++);
			if (_istprint(tmp)) {
				tmpStr.Format(_T("%c"),tmp);
				m_strComment += tmpStr;
			} else {
				m_strComment += ".";
			}
		}
		fullStr += m_strComment;
		m_pLog->AddLine(fullStr);

		break;

	case JFIF_DHT: // DHT    
		m_bStateDht = true;
		DecodeDHT(false);
		m_bStateDhtOk = true;
		break;

	case JFIF_SOS: // SOS
		unsigned long pos_scan_start;	// Byte count at start of scan data segment

		m_bStateSos = true;

		// NOTE: Only want to capture position of first SOS
		//       This should make other function such as AVI frame extract
		//       more robust in case we get multiple SOS segments.
		// We assume that this value is reset when we start a new decode
		if (m_nPosSos == 0) {
			m_nPosSos = m_nPos-2;	// Used for Extract. Want to include actual marker
		}

		length = Buf(m_nPos)*256 + Buf(m_nPos+1);
		m_nPos+=2;

		// Ensure that we have seen proper markers before we try this one!
		if (!m_bStateDhtOk) {
			tmpStr.Format(_T("  ERROR: SOS before valid DHT defined"));
			m_pLog->AddLineErr(tmpStr);
			return DECMARK_ERR;
		}
		if (!m_bStateDqtOk) {
			tmpStr.Format(_T("  ERROR: SOS before valid DQT defined"));
			m_pLog->AddLineErr(tmpStr);
			return DECMARK_ERR;
		}
		if (!m_bStateSofOk) {
			tmpStr.Format(_T("  ERROR: SOS before valid SOF defined"));
			m_pLog->AddLineErr(tmpStr);
			return DECMARK_ERR;
		}

		tmpStr.Format(_T("  Scan header length = %u"),length); 
		m_pLog->AddLine(tmpStr);

		m_nImgSosCompNum = Buf(m_nPos++);
		tmpStr.Format(_T("  Number of img components = %u"),m_nImgSosCompNum);
		m_pLog->AddLine(tmpStr);

		// Just in case something got corrupted, don't want to get out
		// of range here. Note that this will be a hard abort, and
		// will not resume decoding.
		if (m_nImgSosCompNum > 4) {
			tmpStr.Format(_T("  ERROR: Scan decode does not support > 4 components"));
			m_pLog->AddLineErr(tmpStr);
			return DECMARK_ERR;
		}

		unsigned comp_sel,tbl_sel;
		for (unsigned int i=1;((i<=m_nImgSosCompNum) && (!m_bStateAbort));i++)
		{
			fullStr.Format(_T("    Component[%u]: "),i);
			comp_sel = Buf(m_nPos++);
			tbl_sel = Buf(m_nPos++);
			tmpStr.Format(_T("selector=0x%02X, table=0x%02X"),comp_sel,tbl_sel);
			fullStr += tmpStr;
			m_pLog->AddLine(fullStr);
			bRet = m_pImgDec->SetDhtTables(i-1,(tbl_sel & 0xf0)>>4,(tbl_sel & 0x0f));
			DecodeErrCheck(bRet);
		}

		m_nImgSpectralStart = Buf(m_nPos++);
		m_nImgSpectralEnd = Buf(m_nPos++);
		m_nImgSuccApprox = Buf(m_nPos++);

		tmpStr.Format(_T("  Spectral selection = %u .. %u"),m_nImgSpectralStart,m_nImgSpectralEnd);
		m_pLog->AddLine(tmpStr);
		tmpStr.Format(_T("  Successive approximation = 0x%02X"),m_nImgSuccApprox);
		m_pLog->AddLine(tmpStr);

		if (m_pAppConfig->bOutputScanDump) {
			m_pLog->AddLine(_T(""));
			m_pLog->AddLine(_T("  Scan Data: (after bitstuff removed)"));
		}

		// Save the scan data segment position
		pos_scan_start = m_nPos;

		// Skip over the Scan Data segment
		//   Pass 1) Quick, allowing for bOutputScanDump to dump first 640B.
		//   Pass 2) If bDecodeScanImg, we redo the process but in detail decoding.

		// FIXME: Not sure why, but if I skip over Pass 1 (eg if I leave in the
		// following line uncommented), then I get an error at the end of the
		// pass 2 decode (indicating that EOI marker not seen, and expecting
		// marker).
//		if (m_pAppConfig->bOutputScanDump) {

			// --- PASS 1 ---
			bool		m_bSkipDone;
			unsigned	m_nSkipCount;
			unsigned	m_nSkipData;
			unsigned	skip_pos;
			bool		scan_dump_trunc;

			m_bSkipDone = false;
			m_nSkipCount = 0;
			skip_pos = 0;
			scan_dump_trunc = FALSE;

			fullStr = "";
			while (!m_bSkipDone)
			{
				m_nSkipCount++;
				skip_pos++;
				m_nSkipData = Buf(m_nPos++);

				if (m_nSkipData == 0xFF) {
					// this could either be a marker or a byte stuff
					m_nSkipData = Buf(m_nPos++);
					m_nSkipCount++;
					if (m_nSkipData == 0x00) {
						// Byte stuff
						m_nSkipData = 0xFF;
					} else if ((m_nSkipData >= JFIF_RST0) && (m_nSkipData <= JFIF_RST7)) {
						// Skip over
					} else {
						// Marker
						m_bSkipDone = true;
						m_nPos -= 2;
					}
				}

				if (m_pAppConfig->bOutputScanDump && (!m_bSkipDone) ) {
					// Only display 20 lines of scan data
					if (skip_pos > 640) {
						if (!scan_dump_trunc) {
							m_pLog->AddLineWarn(_T("    WARNING: Dump truncated."));
							scan_dump_trunc = TRUE;
						}
					} else {
						if ( ((skip_pos-1) == 0) || (((skip_pos-1) % 32) == 0) ) {
							fullStr = "    ";
						}

						tmpStr.Format(_T("%02x "),m_nSkipData);
						fullStr += tmpStr;

						if (((skip_pos-1) % 32) == 31) {
							m_pLog->AddLine(fullStr);
							fullStr = "";
						}
					}
				}

				// Did we run out of bytes?

				// NOTE: This line here doesn't allow us to attempt to 
				// decode images that are missing EOI. Maybe this is
				// not the best solution here? Instead, we should be
				// checking m_nPos against file length? .. and not 
				// return but "break". FIXME
				if (!m_pWBuf->BufOK) {
					tmpStr.Format("ERROR: Ran out of buffer before EOI during phase 1 of Scan decode @ 0x08X",m_nPos);
					m_pLog->AddLineErr(tmpStr);
					break;
//CAL!					return DECMARK_ERR;
				}

			}
			m_pLog->AddLine(fullStr);

//		}

		// --- PASS 2 ---
		// If the option is set, start parsing!
		if (m_pAppConfig->bDecodeScanImg && m_bImgProgressive) {
			m_pLog->AddLine(_T("  NOTE: Scan parsing doesn't support Progressive scan files yet."));
		} else if (m_pAppConfig->bDecodeScanImg && (m_nImgSofCompNum == 4)) {
			m_pLog->AddLine(_T("  NOTE: Scan parsing doesn't support CMYK files yet."));
		} else if (m_pAppConfig->bDecodeScanImg && !m_bImgProgressive) {

			if (!m_bStateSofOk) {
				m_pLog->AddLineWarn(_T("  NOTE: Scan decode disabled as SOF not decoded."));

			} else {
				m_pLog->AddLine(_T(""));
				m_pImgDec->SetImageDetails(m_anImgSampFactorX[0],m_anImgSampFactorY[0],
					m_nImgSampsPerLine,m_nImgNumLines,m_nImgSofCompNum,m_nImgSosCompNum,m_nImgRstEn,m_nImgRstInterval);

				// Only recalculate the scan decoding if we need to (i.e. file
				// changed, offset changed, scan option changed)

				if (m_pImgSrcDirty) {

					m_pImgDec->DecodeScanImg(pos_scan_start,true,false,m_pAppConfig);

					m_pImgSrcDirty = false;
				}
			}

		}

		m_bStateSosOk = true;

		break;

	case JFIF_DRI:
		unsigned	len;
		unsigned	nVal;
		len = Buf(m_nPos)*256 + Buf(m_nPos+1);
		tmpStr.Format(_T("  length     = %u"),len);
		m_pLog->AddLine(tmpStr);
		nVal = Buf(m_nPos+2)*256 + Buf(m_nPos+3);

		// According to ITU-T spec B.2.4.4, we only expect
		// restart markers if DRI value is non-zero!
		m_nImgRstInterval = nVal;
		if (nVal != 0) {
			m_nImgRstEn = true;
		} else {
			m_nImgRstEn = false;
		}
		tmpStr.Format(_T("  interval   = %u"),m_nImgRstInterval);
		m_pLog->AddLine(tmpStr);
		m_nPos += 4;
		break;

	case JFIF_EOI: // EOI
		m_pLog->AddLine(_T(""));

		// Save the EOI file position
		// NOTE: If the file is missing the EOI, then this variable will be
		//       set to mark the end of file.
		m_nPosEmbedEnd = m_nPos;
		m_nPosEoi = m_nPos;
		m_bStateEoi = true;

		return DECMARK_EOI;

		break;

	default:
		tmpStr.Format(_T("WARNING: Unknown marker [0xFF%02X], stopping decode"),code);
		m_pLog->AddLineWarn(tmpStr);
		m_pLog->AddLine("  Use [Img Search Fwd/Rev] to locate other valid embedded JPEGs");
		return DECMARK_ERR;
		break;
	}

	// Add white-space between each marker
	m_pLog->AddLine(_T(" "));

	// If we decided to abort for any reason, make sure we trap it now.
	// This will stop the process() while loop. We can set m_bStateAbort
	// if user says that they want to stop.
	if (m_bStateAbort) {
		return DECMARK_ERR;
	}

	return DECMARK_OK;
}


// Print out a header for the current JFIF marker code
void CjfifDecode::AddHeader(unsigned code)
{
	CString tmpStr;

	switch(code)
	{
	case JFIF_SOI: m_pLog->AddLineHdr(_T("*** Marker: SOI (xFFD8) ***")); break;

	case JFIF_APP0: m_pLog->AddLineHdr(_T("*** Marker: APP0 (xFFE0) ***")); break;
	case JFIF_APP1: m_pLog->AddLineHdr(_T("*** Marker: APP1 (xFFE1) ***")); break;
	case JFIF_APP2: m_pLog->AddLineHdr(_T("*** Marker: APP2 (xFFE2) ***")); break;
	case JFIF_APP3: m_pLog->AddLineHdr(_T("*** Marker: APP3 (xFFE3) ***")); break;
	case JFIF_APP4: m_pLog->AddLineHdr(_T("*** Marker: APP4 (xFFE4) ***")); break;
	case JFIF_APP5: m_pLog->AddLineHdr(_T("*** Marker: APP5 (xFFE5) ***")); break;
	case JFIF_APP6: m_pLog->AddLineHdr(_T("*** Marker: APP6 (xFFE6) ***")); break;
	case JFIF_APP7: m_pLog->AddLineHdr(_T("*** Marker: APP7 (xFFE7) ***")); break;
	case JFIF_APP8: m_pLog->AddLineHdr(_T("*** Marker: APP8 (xFFE8) ***")); break;
	case JFIF_APP9: m_pLog->AddLineHdr(_T("*** Marker: APP9 (xFFE9) ***")); break;
	case JFIF_APP10: m_pLog->AddLineHdr(_T("*** Marker: APP10 (xFFEA) ***")); break;
	case JFIF_APP11: m_pLog->AddLineHdr(_T("*** Marker: APP11 (xFFEB) ***")); break;
	case JFIF_APP12: m_pLog->AddLineHdr(_T("*** Marker: APP12 (xFFEC) ***")); break;
	case JFIF_APP13: m_pLog->AddLineHdr(_T("*** Marker: APP13 (xFFED) ***")); break;
	case JFIF_APP14: m_pLog->AddLineHdr(_T("*** Marker: APP14 (xFFEE) ***")); break;
	case JFIF_APP15: m_pLog->AddLineHdr(_T("*** Marker: APP15 (xFFEF) ***")); break;

	case JFIF_SOF0: // SOF0
		m_pLog->AddLineHdr(_T("*** Marker: SOF0 (Baseline DCT) (xFFC0) ***"));
		break;
	case JFIF_SOF1: m_pLog->AddLineHdr(_T("*** Marker: SOF1 (Extended Sequential DCT) (xFFC1) ***")); break;
	case JFIF_SOF2: // SOF2
		m_pLog->AddLineHdr(_T("*** Marker: SOF2 (Progressive DCT) (xFFC2) ***"));
		break;
	case JFIF_SOF3: m_pLog->AddLineHdr(_T("*** Marker: SOF3 (xFFC3) ***")); break;
	case JFIF_SOF5: m_pLog->AddLineHdr(_T("*** Marker: SOF5 (xFFC4) ***")); break;
	case JFIF_SOF6: m_pLog->AddLineHdr(_T("*** Marker: SOF6 (xFFC5) ***")); break;
	case JFIF_SOF7: m_pLog->AddLineHdr(_T("*** Marker: SOF7 (xFFC6) ***")); break;
	case JFIF_JPG: m_pLog->AddLineHdr(_T("*** Marker: JPG (xFFC8) ***")); break;
	case JFIF_SOF9: m_pLog->AddLineHdr(_T("*** Marker: SOF9 (xFFC9) ***")); break;
	case JFIF_SOF10: m_pLog->AddLineHdr(_T("*** Marker: SOF10 (xFFCA) ***")); break;
	case JFIF_SOF11: m_pLog->AddLineHdr(_T("*** Marker: SOF11 (xFFCB) ***")); break;
	case JFIF_SOF12: m_pLog->AddLineHdr(_T("*** Marker: SOF12 (xFFCC) ***")); break;
	case JFIF_SOF13: m_pLog->AddLineHdr(_T("*** Marker: SOF13 (xFFCD) ***")); break;
	case JFIF_SOF14: m_pLog->AddLineHdr(_T("*** Marker: SOF14 (xFFCE) ***")); break;
	case JFIF_SOF15: m_pLog->AddLineHdr(_T("*** Marker: SOF15 (xFFCF) ***")); break;

	case JFIF_RST0:
	case JFIF_RST1:
	case JFIF_RST2:
	case JFIF_RST3:
	case JFIF_RST4:
	case JFIF_RST5:
	case JFIF_RST6:
	case JFIF_RST7:
		m_pLog->AddLineHdr(_T("*** Marker: RST# ***"));
		break;

	case JFIF_DQT:  // Define quantization tables
		m_pLog->AddLineHdr(_T("*** Marker: DQT (xFFDB) ***"));
		m_pLog->AddLineHdrDesc(_T("  Define a Quantization Table."));
		break;


	case JFIF_COM: // COM
		m_pLog->AddLineHdr(_T("*** Marker: COM (Comment) (xFFFE) ***"));
		break;

	case JFIF_DHT: // DHT
		m_pLog->AddLineHdr(_T("*** Marker: DHT (Define Huffman Table) (xFFC4) ***"));
		break;
	case JFIF_DHT_FAKE: // DHT from standard table (MotionJPEG)
		m_pLog->AddLineHdr(_T("*** Marker: DHT from MotionJPEG standard (Define Huffman Table) ***"));
		break;

	case JFIF_SOS: // SOS
		m_pLog->AddLineHdr(_T("*** Marker: SOS (Start of Scan) (xFFDA) ***"));
		break;

	case JFIF_DRI: // DRI
		m_pLog->AddLineHdr(_T("*** Marker: DRI (Restart Interval) (xFFDD) ***"));
		break;

	case JFIF_EOI: // EOI
		m_pLog->AddLineHdr(_T("*** Marker: EOI (End of Image) (xFFD9) ***"));
		break;
	default:
		break;
	}
	// Adjust position to account for the word used in decoding the marker!
	tmpStr.Format("  OFFSET: 0x%08X",m_nPos-2);
	m_pLog->AddLine(tmpStr);
}


// Update the status bar with a message
void CjfifDecode::SetStatusText(CString str)
{
	// Make sure that we have been connected to the status
	// bar of the main window first! Note that it is jpegsnoopDoc
	// that sets this variable.
	if (m_pStatBar) {
		m_pStatBar->SetPaneText(0,str);
	}
}

// Generate a special output form of the current image's
// compression signature and other characteristics. This is only
// used during development and batch import to build the online
// web repository.
void CjfifDecode::OutputSpecial()
{
	CString tmpStr;
	CString fullStr;

	ASSERT(m_nImgLandscape!=ENUM_LANDSCAPE_UNSET);

	// This mode of operation is currently only supported & used by me
	// to import the local signature database into a MySQL database
	// backend on the website. It simply reports the MySQL commands
	// which can then be imported into a MySQL client application.
	if (bOutputDB)
	{
		m_pLog->AddLine(_T("*** DB OUTPUT START ***"));
		m_pLog->AddLine(_T("INSERT INTO `quant` (`key`, `make`, `model`, "));
		m_pLog->AddLine(_T("`qual`, `subsamp`, `lum_00`, `lum_01`, `lum_02`, `lum_03`, `lum_04`, "));
		m_pLog->AddLine(_T("`lum_05`, `lum_06`, `lum_07`, `chr_00`, `chr_01`, `chr_02`, "));
		m_pLog->AddLine(_T("`chr_03`, `chr_04`, `chr_05`, `chr_06`, `chr_07`, `qual_lum`, `qual_chr`) VALUES ("));

		fullStr = "'*KEY*', "; // key -- need to override

		// Might need to change m_strImgExifMake to be lowercase
		tmpStr.Format(_T("'%s', "),m_strImgExifMake);
		fullStr += tmpStr; // make

		tmpStr.Format(_T("'%s', "),m_strImgExifModel);
		fullStr += tmpStr; // model

		tmpStr.Format(_T("'%s', "),m_strImgQualExif);
		fullStr += tmpStr; // quality

		tmpStr.Format(_T("'%s', "),m_strImgQuantCss);
		fullStr += tmpStr; // subsampling

		m_pLog->AddLine(fullStr);

		// Step through both quantization tables (0=lum,1=chr)
		unsigned matrix_ind;

		for (unsigned dqt_ind=0;dqt_ind<2;dqt_ind++) {

			fullStr = "";
			for (unsigned y=0;y<8;y++) {
				fullStr += "'";
				for (unsigned x=0;x<8;x++) {
					// Rotate the matrix if necessary!
					matrix_ind = (m_nImgLandscape!=ENUM_LANDSCAPE_NO)?(y*8+x):(x*8+y);
					tmpStr.Format(_T("%u"),m_anImgDqt[dqt_ind][matrix_ind]);
					fullStr += tmpStr;
					if (x!=7) { fullStr += ","; }
				}
				fullStr += "', ";
				if (y==3) {
					m_pLog->AddLine(fullStr);
					fullStr = "";
				}
			}
			m_pLog->AddLine(fullStr);

		}

		fullStr = "";
		// Output quality ratings
		tmpStr.Format(_T("'%f', "),m_adImgDqtQual[0]);
		fullStr += tmpStr;
		// Don't put out comma separator on last line!
		tmpStr.Format(_T("'%f'"),m_adImgDqtQual[1]);
		fullStr += tmpStr;
		fullStr += ");";
		m_pLog->AddLine(fullStr);

		m_pLog->AddLine(_T("*** DB OUTPUT END ***"));
	}

}

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
void CjfifDecode::PrepareSignatureSingle(bool bRotate)
{
	CString tmpStr;
	CString strSet;

	CString strHashIn;
	unsigned char pHashIn[2000];
	CString strDqt;
	MD5_CTX m_md5;
	unsigned uLenHashIn;
	unsigned ind;

	ASSERT(m_nImgLandscape!=ENUM_LANDSCAPE_UNSET);

	// -----------------------------------------------------------
	// Calculate the MD5 hash for online/internal database!
	// signature "00" : DQT0,DQT1,CSS
	// signature "01" : salt,DQT0,DQT1,..DQTx(if defined)


	// Build the source string
	// NOTE: For the purposes of the hash, we need to rotate the DQT tables
	// if we detect that the photo is in portrait orientation! This keeps everything
	// consistent.


	// If no DQT tables have been defined (e.g. could have loaded text file!)
	// then override the sig generation!
	bool dqt_defined = false;
	for (unsigned set=0;set<4;set++) {
		if (m_bImgDqtSet[set]) {
			dqt_defined = true;
		}
	}
	if (!dqt_defined) {
		m_strHash = "NONE";
		m_strHashRot = "NONE";
		return;
	}


	if (DB_SIG_VER == 0x00) {
		strHashIn = "";
	} else {
		strHashIn = "JPEGsnoop";
	}

	// Need to duplicate DQT0 if we only have one DQT table!!
	//   Example file: /editors/imatch_qual100_20070715_6366.jpg

	for (unsigned set=0;set<4;set++) {
		if (m_bImgDqtSet[set]) {
			strSet = "";
			strSet.Format("*DQT%u,",set);
			strHashIn += strSet;
			for (unsigned i=0;i<64;i++) {
//				ind = (m_nImgLandscape!=ENUM_LANDSCAPE_NO)?i:m_sQuantRotate[i];
				ind = (!bRotate)?i:m_sQuantRotate[i];
				tmpStr.Format("%03u,",m_anImgDqt[set][ind]);
				strHashIn += tmpStr;
			}
		} // if DQTx defined
	} // loop through sets (DQT0..DQT3)

	// Removed CSS from signature after version 0x00
	if (DB_SIG_VER == 0x00) {
		strHashIn += "*CSS,";
		strHashIn += m_strImgQuantCss;
		strHashIn += ",";
	}
	strHashIn += "*END";
	uLenHashIn = strlen(strHashIn);

	// Display hash input
	for (unsigned i=0;i<uLenHashIn;i+=80) {
		tmpStr = "";
		tmpStr.Format("In%u: [",i/80);
		tmpStr += strHashIn.Mid(i,80);
		tmpStr += "]";
#ifdef DEBUG_SIG
		m_pLog->AddLine(tmpStr);
#endif
	}

	// Copy into buffer
	ASSERT(uLenHashIn < 2000);
	for (unsigned i=0;i<uLenHashIn;i++) {
		pHashIn[i] = strHashIn.GetAt(i);
	}

	// Calculate the hash
	MD5Init(&m_md5, 0);
	MD5Update(&m_md5, pHashIn, uLenHashIn);
	MD5Final(&m_md5);

	// Overwrite top 8 bits for signature version number
	m_md5.digest32[0] = (m_md5.digest32[0] & 0x00FFFFFF) + (DB_SIG_VER << 24);

	// Convert hash to string format
	if (!bRotate) {
		m_strHash.Format("%08X%08X%08X%08X",m_md5.digest32[0],m_md5.digest32[1],m_md5.digest32[2],m_md5.digest32[3]);
	} else {
		m_strHashRot.Format("%08X%08X%08X%08X",m_md5.digest32[0],m_md5.digest32[1],m_md5.digest32[2],m_md5.digest32[3]);
	}

}

// Generate the compression signatures for the thumbnails
void CjfifDecode::PrepareSignatureThumb()
{
	// Generate m_strHashThumb
	PrepareSignatureThumbSingle(false);
	// Generate m_strHashThumbRot
	PrepareSignatureThumbSingle(true);
}

// Prepare the image signature for later submission
void CjfifDecode::PrepareSignatureThumbSingle(bool bRotate)
{
	CString tmpStr;
	CString	strSet;

	CString strHashIn;
	unsigned char pHashIn[2000];
	CString strDqt;
	MD5_CTX m_md5;
	unsigned uLenHashIn;
	unsigned ind;

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
	bool dqt_defined = false;
	for (unsigned set=0;set<4;set++) {
		if (m_bImgDqtThumbSet[set]) {
			dqt_defined = true;
		}
	}
	if (!dqt_defined) {
		m_strHashThumb = "NONE";
		m_strHashThumbRot = "NONE";
		return;
	}

	if (DB_SIG_VER == 0x00) {
		strHashIn = "";
	} else {
		strHashIn = "JPEGsnoop";
	}

	//tblSelY = m_anImgQuantTblSel[0]; // Y
	//tblSelC = m_anImgQuantTblSel[1]; // Cb (should be same as for Cr)

	// Need to duplicate DQT0 if we only have one DQT table!!
	//   Example file: /editors/imatch_qual100_20070715_6366.jpg

	for (unsigned set=0;set<4;set++) {
		if (m_bImgDqtThumbSet[set]) {
			strSet = "";
			strSet.Format("*DQT%u,",set);
			strHashIn += strSet;
			for (unsigned i=0;i<64;i++) {
				//ind = (m_nImgLandscape!=ENUM_LANDSCAPE_NO)?i:m_sQuantRotate[i];
				ind = (!bRotate)?i:m_sQuantRotate[i];
				tmpStr.Format("%03u,",m_anImgThumbDqt[set][ind]);
				strHashIn += tmpStr;
			}
		} // if DQTx defined
	} // loop through sets (DQT0..DQT3)

	// Removed CSS from signature after version 0x00
	if (DB_SIG_VER == 0x00) {
		strHashIn += "*CSS,";
		strHashIn += m_strImgQuantCss;
		strHashIn += ",";
	}
	strHashIn += "*END";
	uLenHashIn = strlen(strHashIn);

	// Display hash input
	for (unsigned i=0;i<uLenHashIn;i+=80) {
		tmpStr = "";
		tmpStr.Format("In%u: [",i/80);
		tmpStr += strHashIn.Mid(i,80);
		tmpStr += "]";
#ifdef DEBUG_SIG
		m_pLog->AddLine(tmpStr);
#endif
	}

	// Copy into buffer
	ASSERT(uLenHashIn < 2000);
	for (unsigned i=0;i<uLenHashIn;i++) {
		pHashIn[i] = strHashIn.GetAt(i);
	}

	// Calculate the hash
	MD5Init(&m_md5, 0);
	MD5Update(&m_md5, pHashIn, uLenHashIn);
	MD5Final(&m_md5);

	// Overwrite top 8 bits for signature version number
	m_md5.digest32[0] = (m_md5.digest32[0] & 0x00FFFFFF) + (DB_SIG_VER << 24);

	// Convert hash to string format
	if (!bRotate) {
		m_strHashThumb.Format("%08X%08X%08X%08X",m_md5.digest32[0],m_md5.digest32[1],m_md5.digest32[2],m_md5.digest32[3]);
	} else {
		m_strHashThumbRot.Format("%08X%08X%08X%08X",m_md5.digest32[0],m_md5.digest32[1],m_md5.digest32[2],m_md5.digest32[3]);
	}

}


// Compare the image compression signature & metadata against the database.
// This is the routine that is also responsible for creating an
// "Image Assessment" -- ie. whether the image may have been edited or not.
//
// PRE: m_strHash signature has already been calculated by PrepareSignature()
bool CjfifDecode::CompareSignature(bool bQuiet=false)
{
	CString tmpStr;
	CString strHashOut;
	CString locationStr;

	unsigned ind;

	bool	bCurXsw = false;
	bool	bCurXmm = false;
	bool	bCurXmkr = false;
	bool	bCurXextrasw = false;	// EXIF Extra fields match software indicator
	bool	bCurXcomsw = false;		// EXIF COM field match software indicator
	bool	bCurXps = false;		// EXIF photoshop IRB present
	bool	bSrchXsw = false;
	bool	bSrchXswUsig = false;
	bool	bSrchXmmUsig = false;
	bool	bSrchUsig = false;
	bool	bMatchIjg = false;
	CString	sMatchIjgQual = "";

	ASSERT(m_strHash != "NONE");
	ASSERT(m_strHashRot != "NONE");

	if (bQuiet) { m_pLog->Disable(); }


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

	tmpStr.Format("  File Offset:         %lu bytes",m_pAppConfig->nPosStart);
	m_pLog->AddLine(tmpStr);

	// Output the CSS
	tmpStr.Format("  Chroma subsampling:  %s",m_strImgQuantCss);
	m_pLog->AddLine(tmpStr);


	// Calculate the final fields
	// Add Photoshop IRB entries
	// Note that we always add an entry to the m_strImgExtras even if
	// there are no photoshop tags detected. It will appear as "[PS]:[0/0]"
	tmpStr = "";
	tmpStr.Format("[PS]:[%u/%u],",m_nImgQualPhotoshopSa,m_nImgQualPhotoshopSfw);
	m_strImgExtras += tmpStr;


	// --------------------------------------
	// Determine current entry fields

	// Note that some cameras/phones have an empty Make, but use the Model! (eg. Palm Treo)
	if ((m_strImgExifMake == "???") && (m_strImgExifModel == "???")) {
		m_pLog->AddLine("  EXIF Make/Model:     NONE");
		bCurXmm = false;
	} else {
		tmpStr.Format("  EXIF Make/Model:     OK   [%s] [%s]",m_strImgExifMake,m_strImgExifModel);
		m_pLog->AddLine(tmpStr);
		bCurXmm = true;
	}

	if (m_bImgExifMakernotes) {
		m_pLog->AddLine("  EXIF Makernotes:     OK  ");
		bCurXmkr = true;
	} else {
		m_pLog->AddLine("  EXIF Makernotes:     NONE");
		bCurXmkr = false;
	}

	if (strlen(m_strSoftware) == 0) {
		m_pLog->AddLine("  EXIF Software:       NONE");
		bCurXsw = false;
	} else {
		tmpStr.Format("  EXIF Software:       OK   [%s]",m_strSoftware);
		m_pLog->AddLine(tmpStr);

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

	unsigned nSigsInternal = theApp.m_pDbSigs->GetNumSigsInternal();
	unsigned nSigsExtra = theApp.m_pDbSigs->GetNumSigsExtra();

	tmpStr.Format("  Searching Compression Signatures: (%u built-in, %u user(*) )",nSigsInternal,nSigsExtra);
	m_pLog->AddLine(tmpStr);


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
	//  e.g. "[Canon.ImageType]:[CRW:EOS 300D DIGITAL CMOS RAW],"
	if (m_strImgExtras.Find("[Canon.ImageType]:[CRW:") != -1) {
		bCurXextrasw = true;
	}
	if (m_strImgExtras.Find("[Nikon1.Quality]:[RAW") != -1) {
		bCurXextrasw = true;
	}
	if (m_strImgExtras.Find("[Nikon2.Quality]:[RAW") != -1) {
		bCurXextrasw = true;
	}
	if (m_strImgExtras.Find("[Nikon3.Quality]:[RAW") != -1) {
		bCurXextrasw = true;
	}
	if ((m_nImgQualPhotoshopSa != 0) || (m_nImgQualPhotoshopSfw != 0)) {
		bCurXps = true;
	}

	// Search for known COMment field indicators
	if (theApp.m_pDbSigs->SearchCom(m_strComment)) {
		bCurXcomsw = true;
	}

	m_pLog->AddLine("");
	m_pLog->AddLine("          EXIF.Make / Software        EXIF.Model                            Quality           Subsamp Match?");
	m_pLog->AddLine("          -------------------------   -----------------------------------   ----------------  --------------");

	CompSig pEntry;
	unsigned ind_max = theApp.m_pDbSigs->GetDBNumEntries();
	for (ind=0;ind<ind_max;ind++) {
		theApp.m_pDbSigs->GetDBEntry(ind,&pEntry);

		// Reset current entry state
		curMatchMm = false;
		curMatchSw = false;
		curMatchSig = false;
		curMatchSigCss = false;

		// Compare make/model (only for digicams)
		if ((pEntry.m_editor == ENUM_EDITOR_CAM) &&
			(bCurXmm == true) &&
			(pEntry.x_make  == m_strImgExifMake) &&
			(pEntry.x_model == m_strImgExifModel) )
		{
			curMatchMm = true;
		}

		// For software entries, do a loose search
		if ((pEntry.m_editor == ENUM_EDITOR_SW) &&
			(bCurXsw == true) &&
			(pEntry.m_swtrim != "") &&
			(m_strSoftware.Find(pEntry.m_swtrim) != -1) )
		{
			// Software field matches known software string
			bSrchXsw = true;
			curMatchSw = true;
		}


		// Compare signature (and CSS for digicams)
		if ( (pEntry.c_sig == m_strHash) || (pEntry.c_sigrot == m_strHash) ||
			(pEntry.c_sig == m_strHashRot) || (pEntry.c_sigrot == m_strHashRot) )
		{
			curMatchSig = true;

			// If Database entry is for an editor, sig matches irrespective of CSS
			if (pEntry.m_editor == ENUM_EDITOR_SW) {
				bSrchUsig = true;
				curMatchSigCss = true; //CAL! ??? do I need this?

				// For special case of IJG
				if (pEntry.m_swdisp == "IJG Library") {
					bMatchIjg = true;
					sMatchIjgQual = pEntry.um_qual;
				}

			} else {
				// Database entry is for a digicam, sig match only if CSS matches too
				if (pEntry.x_subsamp == m_strImgQuantCss) {
					bSrchUsig = true;
					curMatchSigCss = true;
				} else {
					curMatchSigCss = false;
				}
				
			} // editor

		} else {
			// sig doesn't match
			curMatchSig = false;
			curMatchSigCss = false;
		}


		// For digicams:
		if (curMatchMm && curMatchSigCss) {
			bSrchXmmUsig = true;
		}
		// For software:
		if (curMatchSw && curMatchSig) {
			bSrchXswUsig = true;
		}

		if (theApp.m_pDbSigs->IsDBEntryUser(ind)) {
			locationStr = "*";
		} else {
			locationStr = " ";
		}

		// Display entry if it is a good match
		if (curMatchSig) {
			if (pEntry.m_editor==ENUM_EDITOR_CAM) {
				tmpStr.Format("    %s%4s[%-25s] [%-35s] [%-16s] %-5s %-5s %-5s",locationStr,"CAM:",
					pEntry.x_make.Left(25),pEntry.x_model.Left(35),pEntry.um_qual.Left(16),
					(curMatchSigCss?"Yes":"No"),"","");
			} else if (pEntry.m_editor==ENUM_EDITOR_SW) {
				tmpStr.Format("    %s%4s[%-25s]  %-35s  [%-16s] %-5s %-5s %-5s",locationStr,"SW :",
					pEntry.m_swdisp.Left(25),"",pEntry.um_qual.Left(16),
					"","","");
			} else {
				tmpStr.Format("    %s%4s[%-25s] [%-35s] [%-16s] %-5s %-5s %-5s",locationStr,"?? :",
					pEntry.x_make.Left(25),pEntry.x_model.Left(35),pEntry.um_qual.Left(16),
					"","","");
			}
			if (curMatchMm || curMatchSw) {
				m_pLog->AddLineGood(tmpStr);
			} else {
				m_pLog->AddLine(tmpStr);
			}

		}


	} // loop through DB

	CString strSw;
	// If it matches an IJG signature, report other possible sources:
	if (bMatchIjg) {
		m_pLog->AddLine("");
		m_pLog->AddLine("    The following IJG-based editors also match this signature:");
		unsigned nIjgNum;
		CString strIjgSw;
		nIjgNum = theApp.m_pDbSigs->GetIjgNum();
		for (ind=0;ind<nIjgNum;ind++)
		{
			strIjgSw = theApp.m_pDbSigs->GetIjgEntry(ind);
			tmpStr.Format("     %4s[%-25s]  %-35s  [%-16s] %-5s %-5s %-5s","SW :",
				strIjgSw.Left(25),"",sMatchIjgQual.Left(16),
				"","","");
			m_pLog->AddLine(tmpStr);
		}
	}

	//m_pLog->AddLine("          --------------------   -----------------------------------   ----------------  --------------");
	m_pLog->AddLine("");

	if (bCurXps) {
		m_pLog->AddLine("  NOTE: Photoshop IRB detected");
	}
	if (bCurXextrasw) {
		m_pLog->AddLine("  NOTE: Additional EXIF fields indicate software processing");
	}
	if (bSrchXsw) {
		m_pLog->AddLine("  NOTE: EXIF Software field recognized as from editor");
	}
	if (bCurXcomsw) {
		m_pLog->AddLine("  NOTE: JFIF COMMENT field is known software");
	}



	// ============================================
	// Image Assessment Algorithm
	// ============================================

	bool bEditDefinite = false;
	bool bEditLikely = false;
	bool bEditNot = false;
	bool bEditNotUnknownSw = false;

	if (bCurXps) {
		bEditDefinite = true;
	}
	if (!bCurXmm) {
		bEditDefinite = true;
	}
	if (bCurXextrasw) {
		bEditDefinite = true;
	}
	if (bCurXcomsw) {
		bEditDefinite = true;
	}
	if (bSrchXsw) {
		// Software field matches known software string
		bEditDefinite = true;
	}
	if (theApp.m_pDbSigs->LookupExcMmIsEdit(m_strImgExifMake,m_strImgExifModel)) {
		// Make/Model is in exception list of ones that mark known software
		bEditDefinite = true;
	}

	if (!bCurXmkr) {
		// If we are missing maker notes, we are almost always dealing with
		// edited images. There are some known exceptions, so far:
		//  - Very old digicams
		//  - Certain camera phones
		//  - Blackberry 
		// Perhaps we can make an exception for particular digicams (based on
		// make/model) that this determination will not apply. This means that
		// we open up the doors for these files being edited and not caught.

		if (theApp.m_pDbSigs->LookupExcMmNoMkr(m_strImgExifMake,m_strImgExifModel)) {
			// This is a known exception!
		} else {
			bEditLikely = true;
		}
	}


	// Filter down remaining scenarios
	if (!bEditDefinite && !bEditLikely) {

		if (bSrchXmmUsig) {
			// DB cam signature matches DQT & make/model
			if (!bCurXsw) {
				// EXIF software field is empty
				//
				// We can now be pretty confident that this file has not
				// been edited by all the means that we are checking
				bEditNot = true;
			} else {
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
		} else {
			// No DB cam signature matches DQT & make/model
			// According to EXIF data, this file does not appear to be edited,
			// but no compression signatures in the database match this
			// particular make/model. Therefore, result is UNSURE.
		}

	}


	// Now make final assessment call



	// Determine if image has been processed/edited
	m_nImgEdited = EDITED_UNSET;
	if (bEditDefinite) {
		m_nImgEdited = EDITED_YES;
	} else if (bEditLikely) {
		m_nImgEdited = EDITED_YESPROB;
	} else if (bEditNot) {
		// Images that fall into this category will have:
		//  - No Photoshop tags
		//  - Make/Model is present
		//  - Makernotes present
		//  - No extra software tags (eg. IFD)
		//  - No comment field with known software
		//  - No software field or it does not match known software
		//  - Signature matches DB for this make/model
		m_nImgEdited = EDITED_NO;
	} else {
		// Images that fall into this category will have:
		//  - Same as EDITED_NO but:
		//  - Signature does not match DB for this make/model
		// In all likelihood, this image will in fact be original
		m_nImgEdited = EDITED_UNSURE;
	}




	// If the file offset is non-zero, then don't ask for submit or show assessment
	if (m_pAppConfig->nPosStart != 0) {
		m_pLog->AddLine("  ASSESSMENT not done as file offset non-zero");
		if (bQuiet) { m_pLog->Enable(); }
		return false;
	}

	// ============================================
	// Display final assessment
	// ============================================

	m_pLog->AddLine("  Based on the analysis of compression characteristics and EXIF metadata:");
	m_pLog->AddLine("");
	if (m_nImgEdited == EDITED_YES) {
		m_pLog->AddLine("  ASSESSMENT: Class 1 - Image is processed/edited");
	} else if (m_nImgEdited == EDITED_YESPROB) {
		m_pLog->AddLine("  ASSESSMENT: Class 2 - Image has high probability of being processed/edited");
	} else if (m_nImgEdited == EDITED_NO) {
		m_pLog->AddLine("  ASSESSMENT: Class 3 - Image has high probability of being original");
		// In case the EXIF Software field was detected, 
		if (bEditNotUnknownSw) {
			m_pLog->AddLine("              Note that EXIF Software field is set (typically contains Firmware version)");
		}
	} else if (m_nImgEdited == EDITED_UNSURE) {
		m_pLog->AddLine("  ASSESSMENT: Class 4 - Uncertain if processed or original");
		m_pLog->AddLine("              While the EXIF fields indicate original, no compression signatures ");
		m_pLog->AddLine("              in the current database were found matching this make/model");
	} else {
		m_pLog->AddLineErr("  ASSESSMENT: *** Failed to complete ***");
	}
	m_pLog->AddLine("");



	// Determine if user should add entry to DB
	bool bDbReqAdd = false;		// Ask user to add
	bool bDbReqAddAuto = false;	// Automatically add (in batch operation)


	//CAL!
	// This section should be rewritten! It appears to be overly complex

	m_nDbReqSuggest = DB_ADD_SUGGEST_UNSET;
	if (m_nImgEdited == EDITED_NO) {
		bDbReqAdd = false;
		m_nDbReqSuggest = DB_ADD_SUGGEST_CAM;
	} else if (m_nImgEdited == EDITED_UNSURE) {
		bDbReqAdd = true;
		bDbReqAddAuto = true;
		m_nDbReqSuggest = DB_ADD_SUGGEST_CAM;
		m_pLog->AddLine("  Appears to be new signature for known camera.");
		m_pLog->AddLine("  If the camera/software doesn't appear in list above,");
		m_pLog->AddLineWarn("  PLEASE ADD TO DATABASE with [Tools->Add Camera to DB]");
	} else if (bCurXps && bSrchUsig) {
		// Photoshop and we already have sig
		bDbReqAdd = false;
		m_nDbReqSuggest = DB_ADD_SUGGEST_SW;
	} else if (bCurXps && !bSrchUsig) {
		// Photoshop and we don't already have sig
		bDbReqAdd = true;
		bDbReqAddAuto = true;
		m_nDbReqSuggest = DB_ADD_SUGGEST_SW;
		m_pLog->AddLine("  Appears to be new signature for Photoshop.");
		m_pLog->AddLine("  If it doesn't appear in list above,");
		m_pLog->AddLineWarn("  PLEASE ADD TO DATABASE with [Tools->Add Camera to DB]");
	} else if (bCurXsw && bSrchXsw && bSrchXswUsig) {
		bDbReqAdd = false;
		m_nDbReqSuggest = DB_ADD_SUGGEST_SW;
	} else if (bCurXextrasw) {
		bDbReqAdd = false;
		m_nDbReqSuggest = DB_ADD_SUGGEST_SW;
	} else if (bCurXsw && bSrchXsw && !bSrchXswUsig) {
		bDbReqAdd = true;
		//bDbReqAddAuto = true;
		m_nDbReqSuggest = DB_ADD_SUGGEST_SW;
		m_pLog->AddLine("  Appears to be new signature for known software.");
		m_pLog->AddLine("  If the camera/software doesn't appear in list above,");
		m_pLog->AddLineWarn("  PLEASE ADD TO DATABASE with [Tools->Add Camera to DB]");
	} else if (bCurXmm && bCurXmkr && !bSrchXsw && !bSrchXmmUsig) {
		// unsure if cam, so ask user
		bDbReqAdd = true;
		bDbReqAddAuto = true; //CAL! ****
		m_nDbReqSuggest = DB_ADD_SUGGEST_CAM;
		m_pLog->AddLine("  This may be a new camera for the database.");
		m_pLog->AddLine("  If this file is original, and camera doesn't appear in list above,");
		m_pLog->AddLineWarn("  PLEASE ADD TO DATABASE with [Tools->Add Camera to DB]");
	} else if (!bCurXmm && !bCurXmkr && !bSrchXsw) {
		// unsure if SW, so ask user
		bDbReqAdd = true;
		m_nDbReqSuggest = DB_ADD_SUGGEST_SW;
		m_pLog->AddLine("  This may be a new software editor for the database.");
		m_pLog->AddLine("  If this file is processed, and editor doesn't appear in list above,");
		m_pLog->AddLineWarn("  PLEASE ADD TO DATABASE with [Tools->Add Camera to DB]");
	}


	m_pLog->AddLine("");

	// -----------------------------------------------------------

	if (bQuiet) { m_pLog->Enable(); }

#ifdef BATCH_DO_DBSUBMIT_ALL
	bDbReqAddAuto = true;
	if (m_strImgQualExif == "") {
		// FIXME m_strImgQualExif = m_strFileName;
	}
#endif

	// Return a value that indicates whether or not we should add this
	// entry to the database
	return bDbReqAddAuto;

}


// Build the image data string that will be sent to the database repository
// This data string contains the compression siganture and a few special
// fields such as image dimensions, etc.
//
// If Portrait, Rotates DQT table, Width/Height.
//   m_strImgQuantCss is already rotated by process()
// PRE: m_strHash already defined
void CjfifDecode::PrepareSendSubmit(CString strQual,unsigned nUserSource,CString strUserSoftware,CString strUserNotes)
{
	// Generate the DQT arrays suitable for posting
	CString tmpStr1;
	CString s_dqt[4];
	unsigned matrix_ind;

	ASSERT(m_strHash != "NONE");
	ASSERT(m_nImgLandscape!=ENUM_LANDSCAPE_UNSET);

	for (unsigned set=0;set<4;set++) {
		s_dqt[set] = "";

		if (m_bImgDqtSet[set]) {
			for (unsigned ind=0;ind<64;ind++) {
				// FIXME Still consider rotating DQT table even though we
				// don't know for sure if m_nImgLandscape is accurate
				// Not a big deal if we get it wrong as we still add
				// both pre- and post-rotated sigs.
				matrix_ind = (m_nImgLandscape!=ENUM_LANDSCAPE_NO)?ind:m_sQuantRotate[ind];
				if ((ind%8 == 0) && (ind != 0)) {
					s_dqt[set].Append("!");
				}

				s_dqt[set].AppendFormat("%u",m_anImgDqt[set][matrix_ind]);
				if (ind%8 != 7) {
					s_dqt[set].Append(",");
				}
			}
		} // set defined?
	} // up to 4 sets

	unsigned orig_w,orig_h;
	orig_w = (m_nImgLandscape!=ENUM_LANDSCAPE_NO)?m_nImgSampsPerLine:m_nImgNumLines;
	orig_h = (m_nImgLandscape!=ENUM_LANDSCAPE_NO)?m_nImgNumLines:m_nImgSampsPerLine;

	unsigned orig_thumb_w,orig_thumb_h;
	orig_thumb_w = (m_nImgLandscape!=ENUM_LANDSCAPE_NO)?m_nImgThumbSampsPerLine:m_nImgThumbNumLines;
	orig_thumb_h = (m_nImgLandscape!=ENUM_LANDSCAPE_NO)?m_nImgThumbNumLines:m_nImgThumbSampsPerLine;

	unsigned maker;
	maker = (m_bImgExifMakernotes)?ENUM_MAKER_PRESENT:ENUM_MAKER_NONE;

	// Sort sig additions
	// To create some determinism in the database, arrange the sigs
	// to be in numerical order
	CString strSig0,strSig1,strSigThm0,strSigThm1;
	if (m_strHash <= m_strHashRot) {
		strSig0 = m_strHash;
		strSig1 = m_strHashRot;
	} else {
		strSig0 = m_strHashRot;
		strSig1 = m_strHash;
	}
	if (m_strHashThumb <= m_strHashThumbRot) {
		strSigThm0 = m_strHashThumb;
		strSigThm1 = m_strHashThumbRot;
	} else {
		strSigThm0 = m_strHashThumbRot;
		strSigThm1 = m_strHashThumb;
	}

	SendSubmit(m_strImgExifMake,m_strImgExifModel,strQual,s_dqt[0],s_dqt[1],s_dqt[2],s_dqt[3],m_strImgQuantCss,
		strSig0,strSig1,strSigThm0,strSigThm1,(float)m_adImgDqtQual[0],(float)m_adImgDqtQual[1],orig_w,orig_h,
		m_strSoftware,m_strComment,maker,nUserSource,strUserSoftware,m_strImgExtras,
		strUserNotes,m_nImgLandscape,orig_thumb_w,orig_thumb_h);

}


// Send the compression signature string to the local database file
// in addition to the web repository if the user has enabled it.
void CjfifDecode::SendSubmit(CString exif_make, CString exif_model, CString qual,
							 CString dqt0, CString dqt1, CString dqt2, CString dqt3,
							 CString css,
								CString sig, CString sig_rot ,CString sigthumb,CString sigthumb_rot, float qfact0, float qfact1, unsigned img_w, unsigned img_h, 
								CString exif_software, CString comment, unsigned maker,
								unsigned user_source, CString user_software, 
								CString extra, CString user_notes, unsigned exif_landscape,
								unsigned thumb_x,unsigned thumb_y)
{
	// NOTE: This assumes that we've already run PrepareSignature()
	// which usually happens when we process a file.
	ASSERT(sig != "");
	ASSERT(sig_rot != "");

	CUrlString curls;

	CString DB_SUBMIT_WWW_VER = "02";

#ifndef BATCH_DO
	if (m_bSigExactInDB) {
		AfxMessageBox("Compression signature already in database");
	} else {

		// Now append it to the local database and resave
		theApp.m_pDbSigs->DatabaseExtraAdd(exif_make,exif_model,
			qual,sig,sig_rot,css,user_source,user_software);

		AfxMessageBox("Added Compression signature to database");
	}
#endif


	// Is automatic internet update enabled?
	if (!theApp.m_pAppConfig->bDbSubmitNet) {
		return;
	}

	CString tmpStr;

	CString strFormat;
	CString strFormDataPre,strFormData;
	unsigned strFormDataLen;

	unsigned checksum=32;
	CString submit_host;
	CString submit_page;
	submit_host = IA_HOST;
	submit_page = IA_DB_SUBMIT_PAGE;
	for (unsigned i=0;i<strlen(IA_HOST);i++) {
		checksum += submit_host.GetAt(i);
		checksum += 3*submit_page.GetAt(i);
	}

	//if ( (m_pAppConfig->bIsWindowsNTorLater) && (checksum == 9678) ) {
	if (checksum == 9678) {

		// Submit to online database
		CString strHeaders =
			_T("Content-Type: application/x-www-form-urlencoded");
		// URL-encoded form variables -
		strFormat  = "ver=%s&x_make=%s&x_model=%s&um_qual=%s&x_dqt0=%s&x_dqt1=%s&x_dqt2=%s&x_dqt3=%s";
		strFormat += "&x_subsamp=%s&c_sig=%s&c_sigrot=%s&c_qfact0=%f&c_qfact1=%f&x_img_w=%u&x_img_h=%u";
		strFormat += "&x_sw=%s&x_com=%s&x_maker=%u&u_source=%d&u_sw=%s";
		strFormat += "&x_extra=%s&u_notes=%s&c_sigthumb=%s&c_sigthumbrot=%s&x_landscape=%u";
		strFormat += "&x_thumbx=%u&x_thumby=%u";


		strFormDataPre.Format(strFormat,
			DB_SUBMIT_WWW_VER,exif_make,exif_model,
			qual,dqt0,dqt1,dqt2,dqt3,css,sig,sig_rot,qfact0,qfact1,img_w,img_h,
			exif_software,comment,
			maker,user_source,user_software,
			extra,user_notes,
			sigthumb,sigthumb_rot,exif_landscape,thumb_x,thumb_y);

		//*** Need to sanitize data for URL submission!
		// Search for "&", "?", "="
		strFormData.Format(strFormat,
			DB_SUBMIT_WWW_VER,curls.Encode(exif_make),curls.Encode(exif_model),
			qual,dqt0,dqt1,dqt2,dqt3,css,sig,sig_rot,qfact0,qfact1,img_w,img_h,
			curls.Encode(exif_software),curls.Encode(comment),
			maker,user_source,curls.Encode(user_software),
			curls.Encode(extra),curls.Encode(user_notes),
			sigthumb,sigthumb_rot,exif_landscape,thumb_x,thumb_y);
		strFormDataLen = strFormData.GetLength();


#ifdef DEBUG_SIG
		AfxMessageBox(strFormDataPre);
		AfxMessageBox(strFormData);
#endif

#ifdef WWW_WININET
		static LPSTR acceptTypes[2]={"*/*", NULL};
		HINTERNET hINet, hConnection, hData;

		//unsigned len;
		//CHAR buffer[2048] ;
		//DWORD dwRead, dwFlags, dwStatus ;
		CString m_strContents ;
		hINet = InternetOpen("JPEGsnoop/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
		if ( !hINet )
		{
			AfxMessageBox("InternetOpen Failed");
			return;
		}
		try
		{
			//hConnection = InternetConnect( hINet, submit_host, 80, " "," ", INTERNET_SERVICE_HTTP, 0, 0 );
			hConnection = InternetConnect( hINet, (LPCTSTR)submit_host, 80, NULL,NULL, INTERNET_SERVICE_HTTP, 0, 1 );
			if ( !hConnection )
			{
				InternetCloseHandle(hINet);
				return;
			}
			//hData = HttpOpenRequestW( hConnection, "POST", submit_page, NULL, NULL, NULL, INTERNET_FLAG_KEEP_CONNECTION, 0 );
			hData = HttpOpenRequest( hConnection, "POST", (LPCTSTR)submit_page, NULL, NULL, NULL, 0, 1 );
			if ( !hData )
			{
				InternetCloseHandle(hConnection);
				InternetCloseHandle(hINet);
				return;
			}
			// GET HttpSendRequest( hData, NULL, 0, NULL, 0);

			//HttpSendRequestW( hData, NULL, 0, strPostData, strFormDataLen);
			HttpSendRequest( hData, (LPCTSTR)strHeaders, strHeaders.GetLength(), strFormData.GetBuffer(), strFormData.GetLength());

			/*
			
			while( InternetReadFile( hData, buffer, 255, &dwRead ) )
			{
				if ( dwRead == 0 )
					break;
				buffer[dwRead] = 0;
				m_strContents += buffer;
			}

			len = m_strContents.GetLength();
			// Display hash input
			for (unsigned i=0;i<len;i+=80) {
				tmpStr = "";
				tmpStr.Format("Data%u: [",i/80);
				tmpStr += m_strContents.Mid(i,80);
				tmpStr += "]";
				//AfxMessageBox(tmpStr);
				m_pLog->AddLine(tmpStr);
			}
			*/

		}
		catch( CInternetException* e)
		{
			e->ReportError();
			e->Delete();
			//AfxMessageBox("EXCEPTION!");
		}
		InternetCloseHandle(hConnection);
		InternetCloseHandle(hINet);
		InternetCloseHandle(hData);
#endif

#ifdef WWW_WINHTTP

		CInternetSession session;
		CHttpConnection* pConnection;
		CHttpFile* pFile;
		BOOL result;
		DWORD dwRet;

		// *** NOTE: Will not work on Windows 95/98!!
		// This section is avoided in early OSes otherwise we get an Illegal Op
		try {		
			pConnection = session.GetHttpConnection(submit_host);
			ASSERT (pConnection);
			pFile = pConnection->OpenRequest(CHttpConnection::HTTP_VERB_POST,_T(submit_page));
			ASSERT (pFile);
			result = pFile->SendRequest(
				strHeaders,(LPVOID)(LPCTSTR)strFormData, strFormData.GetLength());
			ASSERT (result != 0);
			pFile->QueryInfoStatusCode(dwRet);
			ASSERT (dwRet == HTTP_STATUS_OK);

			// Clean up!
			if (pFile) {
				pFile->Close();
				delete pFile;
				pFile = NULL;
			}
			if (pConnection) {
				pConnection->Close();
				delete pConnection;
				pConnection = NULL;
			}
			session.Close();

		}

		catch (CInternetException* pEx) 
		{
		// catch any exceptions from WinINet      
			TCHAR szErr[1024];
			szErr[0] = '\0';
			if(!pEx->GetErrorMessage(szErr, 1024))
				strcpy(szErr,"Unknown error");
			TRACE("Submit Failed! - %s",szErr);   
			AfxMessageBox(szErr);	//CAL!
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


// Parse the embedded JPEG thumbnail. This routine is a much-reduced
// version of the main JFIF parser, in that it focuses primarily on the
// DQT tables.
void CjfifDecode::DecodeEmbeddedThumb()
{
	CString		tmpStr;
	CString		markerStr;
	unsigned	nPosSaved;
	unsigned	nPosSaved_sof;
	unsigned	nPosEnd;
	bool		done;
	unsigned	code;
	bool		bRet;

	CString fullStr;
	unsigned dqt_precision;
	unsigned dqt_dest_id;
	unsigned nImgPrecision;
	unsigned length;
	unsigned tmp;
	bool scan_m_bSkipDone;
	bool error = false;
	bool error_thumb_len_zero = false;
	unsigned m_nSkipCount;

	nPosSaved = m_nPos;

	// Examine the EXIF embedded thumbnail (if it exists)
	if (m_nImgExifThumbComp == 6) {
		m_pLog->AddLine("");
		m_pLog->AddLineHdr("*** Embedded JPEG Thumbnail ***");
		tmpStr.Format("  Offset: 0x%08X",m_nImgExifThumbOffset);
		m_pLog->AddLine(tmpStr);
		tmpStr.Format("  Length: 0x%08X (%u)",m_nImgExifThumbLen,m_nImgExifThumbLen);
		m_pLog->AddLine(tmpStr);

		// Quick scan for DQT tables
		m_nPos = m_nImgExifThumbOffset;
		done = false;
		while (!done) {

			// For some reason, I have found files that have a length of 0
			if (m_nImgExifThumbLen != 0) {
				if ((m_nPos-m_nImgExifThumbOffset) > m_nImgExifThumbLen) {
					tmpStr.Format(_T("ERROR: Read more than specified EXIF thumb length (%u bytes) before EOI"),m_nImgExifThumbLen);
					m_pLog->AddLineErr(tmpStr);
					error = true;
					done = true;
				}
			} else {
				// Don't try to process if length is 0!
				// Seen this in a Canon 1ds file (processed by photoshop)
				done = true;
				error = true;
				error_thumb_len_zero = true;
			}
			if ((!done) && (Buf(m_nPos++) != 0xFF)) {
				tmpStr.Format(_T("ERROR: Expected marker 0xFF, got 0x%02X @ offset 0x%08X"),Buf(m_nPos-1),(m_nPos-1));
				m_pLog->AddLineErr(tmpStr);
				error = true;
				done = true;

			}



			if (!done) {
				code = Buf(m_nPos++);

 				m_pLog->AddLine("");
				switch (code) {
					case JFIF_SOI: // SOI
						m_pLog->AddLine("  * Embedded Thumb Marker: SOI"); 
						break;

					case JFIF_DQT:  // Define quantization tables
						m_pLog->AddLine("  * Embedded Thumb Marker: DQT"); 

						length = Buf(m_nPos)*256 + Buf(m_nPos+1);
						nPosEnd = m_nPos+length;
						m_nPos+=2;
						tmpStr.Format(_T("    Length = %u"),length); 
						m_pLog->AddLine(tmpStr);

						while (nPosEnd > m_nPos)
						{
							tmpStr.Format(_T("    ----"));
							m_pLog->AddLine(tmpStr);

							tmp = Buf(m_nPos++);
							dqt_precision = (tmp & 0xF0) >> 4;
							dqt_dest_id = tmp & 0x0F;
							tmpStr.Format(_T("    Precision=%u bits"),dqt_precision);
							m_pLog->AddLine(tmpStr);
							tmpStr.Format(_T("    Destination ID=%u"),dqt_dest_id);
							if (dqt_dest_id == 0) {
								tmpStr += " (Luminance)";
							}
							else if (dqt_dest_id == 1) {
								tmpStr += " (Chrominance)";
							}
							else if (dqt_dest_id == 2) {
								tmpStr += " (Chrominance)";
							}
							else {
								tmpStr += " (???)";
							}
							m_pLog->AddLine(tmpStr);


							if (dqt_dest_id >= 4) {
								tmpStr.Format(_T("ERROR: dqt_dest_id = %u, >= 4"),dqt_dest_id);
								m_pLog->AddLineErr(tmpStr);
								done = true;
								error = true;
								break;
							}

							for (unsigned ind=0;ind<=63;ind++)
							{
								tmp = Buf(m_nPos++);
								m_anImgThumbDqt[dqt_dest_id][m_sZigZag[ind]] = tmp;
							}
							m_bImgDqtThumbSet[dqt_dest_id] = true;
							
							// Now display the table
							for (unsigned y=0;y<8;y++) {
								fullStr.Format(_T("      DQT, Row #%u: "),y);
								for (unsigned x=0;x<8;x++) {
									tmpStr.Format(_T("%3u "),m_anImgThumbDqt[dqt_dest_id][y*8+x]);
									fullStr += tmpStr;

									// Store the DQT entry into the Image Decoder
									bRet = m_pImgDec->SetDqtEntry(dqt_dest_id,y*8+x,m_sUnZigZag[y*8+x],m_anImgDqt[dqt_dest_id][y*8+x]);
									DecodeErrCheck(bRet);

								}

								m_pLog->AddLine(fullStr);

							}
						

						}

						break;

					case JFIF_SOF0:
						m_pLog->AddLine("  * Embedded Thumb Marker: SOF");
						length = Buf(m_nPos)*256 + Buf(m_nPos+1);
						nPosSaved_sof = m_nPos;
						m_nPos+=2;
						tmpStr.Format(_T("    Frame header length = %u"),length); 
						m_pLog->AddLine(tmpStr);


						nImgPrecision = Buf(m_nPos++);
						tmpStr.Format(_T("    Precision = %u"),nImgPrecision);
						m_pLog->AddLine(tmpStr);

						m_nImgThumbNumLines = Buf(m_nPos)*256 + Buf(m_nPos+1);
						m_nPos += 2;
						tmpStr.Format(_T("    Number of Lines = %u"),m_nImgThumbNumLines);
						m_pLog->AddLine(tmpStr);

						m_nImgThumbSampsPerLine = Buf(m_nPos)*256 + Buf(m_nPos+1);
						m_nPos += 2; 
						tmpStr.Format(_T("    Samples per Line = %u"),m_nImgThumbSampsPerLine);
						m_pLog->AddLine(tmpStr);
						tmpStr.Format(_T("    Image Size = %u x %u"),m_nImgThumbSampsPerLine,m_nImgThumbNumLines);
						m_pLog->AddLine(tmpStr);

						m_nPos = nPosSaved_sof+length;

						break;

					case JFIF_SOS: // SOS
						m_pLog->AddLine("  * Embedded Thumb Marker: SOS");
						m_pLog->AddLine("    Skipping scan data");
						scan_m_bSkipDone = false;
						m_nSkipCount = 0;
						while (!scan_m_bSkipDone) {
							if ((Buf(m_nPos) == 0xFF) && (Buf(m_nPos+1) != 0x00)) {
								// Was it a restart marker?
								if ((Buf(m_nPos+1) >= JFIF_RST0) && (Buf(m_nPos+1) <= JFIF_RST7)) {
									m_nPos++;
								} else {
									// No... it's a real marker
									scan_m_bSkipDone = true;
								}
							} else {
								m_nPos++;
								m_nSkipCount++;
							}
						}
						tmpStr.Format("    Skipped %u bytes",m_nSkipCount);
						m_pLog->AddLine(tmpStr);
						break;
					case JFIF_EOI:
						m_pLog->AddLine("  * Embedded Thumb Marker: EOI"); 
						done = true;
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
						GetMarkerName(code,markerStr);
						tmpStr.Format(_T("  * Embedded Thumb Marker: %s"),markerStr); 
						m_pLog->AddLine(tmpStr);
						length = Buf(m_nPos)*256 + Buf(m_nPos+1);
						tmpStr.Format(_T("    Length = %u"),length); 
						m_pLog->AddLine(tmpStr);
						m_nPos += length;
						break;

				}
			} // if !done
		} // while !done

		// Now calculate the signature
		if (!error) {
			PrepareSignatureThumb();
			m_pLog->AddLine("");
			tmpStr.Format("  * Embedded Thumb Signature: %s",m_strHashThumb);
			m_pLog->AddLine(tmpStr);
		}

		if (error_thumb_len_zero) {
			m_strHashThumb = "ERR: Len=0";
			m_strHashThumbRot = "ERR: Len=0";
		}

	} // if JPEG compressed

	m_nPos = nPosSaved;
}


// Lookup the EXIF marker name from the code value
bool CjfifDecode::GetMarkerName(unsigned code,CString &markerStr)
{
	bool done = false;
	bool found = false;
	unsigned ind=0;
	while (!done)
	{
		if (m_pMarkerNames[ind].code==0) {
			done = true;
		} else if (m_pMarkerNames[ind].code==code) {
			done = true;
			found = true;
			markerStr = m_pMarkerNames[ind].str;
			return true;
		} else {
			ind++;
		}
	}
	if (!found) {
		markerStr = "";
		markerStr.Format("(0xFF%02X)",code);
		return false;
	}
	return true;

}

// Determine if the file is an AVI MJPEG.
// If so, parse the headers.
bool CjfifDecode::DecodeAvi()
{
	CString		tmpStr;
	unsigned	nPosSaved;

	m_bAvi = false;
	m_bAviMjpeg = false;

	// Perhaps start from file position 0?
	nPosSaved = m_nPos;

	// Start from file position 0
	m_nPos = 0;

	bool		bSwap = true;

	CString		strRiff;
	unsigned	nRiffLen;
	CString		strForm;

	strRiff = m_pWBuf->BufReadStrn(m_nPos,4); m_nPos+=4;
	nRiffLen = m_pWBuf->BufX(m_nPos,4,bSwap); m_nPos+=4;
	strForm = m_pWBuf->BufReadStrn(m_nPos,4); m_nPos+=4;
	if ((strRiff == "RIFF") && (strForm == "AVI ")) {
		m_bAvi = true;
		m_pLog->AddLine("");
		m_pLog->AddLineHdr("*** AVI File Decoding ***");
		m_pLog->AddLine("Decoding RIFF AVI format...");
		m_pLog->AddLine("");
	} else {
		// Reset file position
		m_nPos = nPosSaved;
		return false;
	}

	CString		strHeader;
	unsigned	nChunkSize;
	unsigned	nChunkDataStart;

	bool	done = false;
	while (!done) {
		if (m_nPos >= m_pWBuf->pos_eof) {
			done = true;
			break;
		}

		strHeader = m_pWBuf->BufReadStrn(m_nPos,4); m_nPos+=4;
        tmpStr.Format("  %s",strHeader);
		m_pLog->AddLine(tmpStr);

		nChunkSize = m_pWBuf->BufX(m_nPos,4,bSwap); m_nPos+=4;
		nChunkDataStart = m_nPos;


		if (strHeader == "LIST") {

			// --- LIST ---

			CString strListType;
			strListType = m_pWBuf->BufReadStrn(m_nPos,4); m_nPos+=4;

			tmpStr.Format("    %s",strListType);
			m_pLog->AddLine(tmpStr);

			if (strListType == "hdrl") {

				// --- hdrl ---

				unsigned nPosHdrlStart;
				CString strHdrlId;
				strHdrlId = m_pWBuf->BufReadStrn(m_nPos,4); m_nPos+=4;
				unsigned nHdrlLen;
				nHdrlLen = m_pWBuf->BufX(m_nPos,4,bSwap); m_nPos+=4;
				nPosHdrlStart = m_nPos;

				// nHdrlLen should be 14*4 bytes

				m_nPos = nPosHdrlStart + nHdrlLen;

			} else if (strListType == "strl") {

				// --- strl ---

				// strhHEADER
				unsigned nPosStrlStart;
				CString strStrlId;
				strStrlId = m_pWBuf->BufReadStrn(m_nPos,4); m_nPos+=4;
				unsigned nStrhLen;
				nStrhLen = m_pWBuf->BufX(m_nPos,4,bSwap); m_nPos+=4;
				nPosStrlStart = m_nPos;

				CString fccType;
				CString fccHandler;
				unsigned dwFlags,dwReserved1,dwInitialFrames,dwScale,dwRate;
				unsigned dwStart,dwLength,dwSuggestedBufferSize,dwQuality;
				unsigned dwSampleSize,xdwQuality,xdwSampleSize;
				fccType = m_pWBuf->BufReadStrn(m_nPos,4); m_nPos+=4;
				fccHandler = m_pWBuf->BufReadStrn(m_nPos,4); m_nPos+=4;
				dwFlags = m_pWBuf->BufX(m_nPos,4,bSwap); m_nPos+=4;
				dwReserved1 = m_pWBuf->BufX(m_nPos,4,bSwap); m_nPos+=4;
				dwInitialFrames = m_pWBuf->BufX(m_nPos,4,bSwap); m_nPos+=4;
				dwScale = m_pWBuf->BufX(m_nPos,4,bSwap); m_nPos+=4;
				dwRate = m_pWBuf->BufX(m_nPos,4,bSwap); m_nPos+=4;
				dwStart = m_pWBuf->BufX(m_nPos,4,bSwap); m_nPos+=4;
				dwLength = m_pWBuf->BufX(m_nPos,4,bSwap); m_nPos+=4;
				dwSuggestedBufferSize = m_pWBuf->BufX(m_nPos,4,bSwap); m_nPos+=4;
				dwQuality = m_pWBuf->BufX(m_nPos,4,bSwap); m_nPos+=4;
				dwSampleSize = m_pWBuf->BufX(m_nPos,4,bSwap); m_nPos+=4;
				xdwQuality = m_pWBuf->BufX(m_nPos,4,bSwap); m_nPos+=4;
				xdwSampleSize = m_pWBuf->BufX(m_nPos,4,bSwap); m_nPos+=4;

				CString fccTypeDecode = "";
				if (fccType == "vids") { fccTypeDecode = "[vids] Video"; }
				else if (fccType == "auds") { fccTypeDecode = "[auds] Audio"; }
				else if (fccType == "txts") { fccTypeDecode = "[txts] Subtitle"; }
				else { fccTypeDecode.Format("[%s]",fccType); } 
				tmpStr.Format("      -[FourCC Type]  = %s",fccTypeDecode);
				m_pLog->AddLine(tmpStr);

				tmpStr.Format("      -[FourCC Codec] = [%s]",fccHandler);
				m_pLog->AddLine(tmpStr);

				float fSampleRate = 0;
				if (dwScale != 0) {
					fSampleRate = (float)dwRate / (float)dwScale;
				}
				tmpStr.Format("      -[Sample Rate]  = [%.2f]",fSampleRate);
				if (fccType == "vids") { tmpStr.Append(" frames/sec"); }
				else if (fccType == "auds") { tmpStr.Append(" samples/sec"); }
				m_pLog->AddLine(tmpStr);

				m_nPos = nPosStrlStart + nStrhLen;	// Skip

				tmpStr.Format("      %s",fccType);
				m_pLog->AddLine(tmpStr);

				if (fccType == "vids") {
					// --- vids ---

					// Is it MJPEG?
					//tmpStr.Format("      -[Video Stream FourCC]=[%s]",fccHandler);
					//m_pLog->AddLine(tmpStr);
					if (fccHandler == "mjpg") {
						m_bAviMjpeg = true;
					}
					if (fccHandler == "MJPG") {
						m_bAviMjpeg = true;
					}

					// strfHEADER_BIH
					CString strSkipId;
					unsigned nSkipLen;
					unsigned nSkipStart;
					strSkipId = m_pWBuf->BufReadStrn(m_nPos,4); m_nPos+=4;
					nSkipLen = m_pWBuf->BufX(m_nPos,4,bSwap); m_nPos+=4;
					nSkipStart = m_nPos;
					
					m_nPos = nSkipStart + nSkipLen; // Skip

				} else if (fccType == "auds") {
					// --- auds ---

					// strfHEADER_WAVE

					CString strSkipId;
					unsigned nSkipLen;
					unsigned nSkipStart;
					strSkipId = m_pWBuf->BufReadStrn(m_nPos,4); m_nPos+=4;
					nSkipLen = m_pWBuf->BufX(m_nPos,4,bSwap); m_nPos+=4;
					nSkipStart = m_nPos;
					
					m_nPos = nSkipStart + nSkipLen; // Skip
				} else {
					// strfHEADER

					CString strSkipId;
					unsigned nSkipLen;
					unsigned nSkipStart;
					strSkipId = m_pWBuf->BufReadStrn(m_nPos,4); m_nPos+=4;
					nSkipLen = m_pWBuf->BufX(m_nPos,4,bSwap); m_nPos+=4;
					nSkipStart = m_nPos;
					
					m_nPos = nSkipStart + nSkipLen; // Skip
				}

				// strnHEADER
				unsigned nPosStrnStart;
				CString strStrnId;
				strStrnId = m_pWBuf->BufReadStrn(m_nPos,4); m_nPos+=4;
				unsigned nStrnLen;
				nStrnLen = m_pWBuf->BufX(m_nPos,4,bSwap); m_nPos+=4;
				nPosStrnStart = m_nPos;

				//CAL! FIXME
				// Can we rewrite in terms of ChunkSize and ChunkDataStart?
				//ASSERT ((nPosStrnStart + nStrnLen + (nStrnLen%2)) == (nChunkDataStart + nChunkSize + (nChunkSize%2)));
				//m_nPos = nChunkDataStart + nChunkSize + (nChunkSize%2);
				m_nPos = nPosStrnStart + nStrnLen + (nStrnLen%2); // Skip

			} else if (strListType == "movi") {
				// movi

				m_nPos = nChunkDataStart + nChunkSize + (nChunkSize%2);
			} else if (strListType == "INFO") {
				// INFO
				unsigned nInfoStart;
				nInfoStart = m_nPos;

				CString strInfoId;
				unsigned nInfoLen;
				strInfoId = m_pWBuf->BufReadStrn(m_nPos,4); m_nPos+=4;
				nInfoLen = m_pWBuf->BufX(m_nPos,4,bSwap); m_nPos+=4;

				if (strInfoId == "ISFT") {
					CString strIsft="";
					strIsft = m_pWBuf->BufReadStrn(m_nPos,nChunkSize);
					strIsft.TrimRight();
					tmpStr.Format("      -[Software] = [%s]",strIsft);
					m_pLog->AddLine(tmpStr);
				}

				m_nPos = nChunkDataStart + nChunkSize + (nChunkSize%2);
			} else {
				// ?
				m_nPos = nChunkDataStart + nChunkSize + (nChunkSize%2);
			}
			
		} else if (strHeader == "JUNK") {
			// Junk

			m_nPos = nChunkDataStart + nChunkSize + (nChunkSize%2);
		} else if (strHeader == "IDIT") {
			// Timestamp info (Canon, etc.)

			CString strIditTimestamp="";
			strIditTimestamp = m_pWBuf->BufReadStrn(m_nPos,nChunkSize);
			strIditTimestamp.TrimRight();
			tmpStr.Format("    -[Timestamp] = [%s]",strIditTimestamp);
			m_pLog->AddLine(tmpStr);
		
			m_nPos = nChunkDataStart + nChunkSize + (nChunkSize%2);

		} else if (strHeader == "indx") {
			// Index

			m_nPos = nChunkDataStart + nChunkSize + (nChunkSize%2);
		} else if (strHeader == "idx1") {
			// Index
			unsigned nIdx1Entries = nChunkSize / (4*4);

			m_nPos = nChunkDataStart + nChunkSize + (nChunkSize%2);

		} else {
			// ???

			m_nPos = nChunkDataStart + nChunkSize + (nChunkSize%2);

		}

	}

	m_pLog->AddLine("");
	if (m_bAviMjpeg) {
		m_strImgExtras += "[AVI]:[mjpg],";
		m_pLog->AddLineGood("  AVI is MotionJPEG");
		m_pLog->AddLineWarn("  Use [Tools->Img Search Fwd] to locate next frame");
	} else {
		m_strImgExtras += "[AVI]:[????],";
		m_pLog->AddLineWarn("  AVI is not MotionJPEG. [Img Search Fwd/Rev] unlikely to find frames.");
	}
	m_pLog->AddLine("");

	// Reset file position
	m_nPos = nPosSaved;

	return m_bAviMjpeg;
}



// This is the primary JFIF parsing routine.
// The main loop steps through all of the JFIF markers and calls
// DecodeMarker() each time until we reach the end of file or an error.
// Finally, we invoke the compression signature search function.
void CjfifDecode::process(CFile* inFile)
{

	CString tmpStr;

	// Reset the JFIF decoder state as we may be redoing another file
	Reset();

	// Reset the IMG Decoder state
	if (m_pImgSrcDirty) {
		m_pImgDec->FreshStart();
	}

	// Set the statusbar text to Processing...
	//CAL! For some reason, the stat bar is NULL if we drag & drop
	//     a file onto the JPEGsnoop app icon! So, we need to
	//     first check for its existence.
	if (m_pStatBar) {
		m_pStatBar->SetPaneText(0,"Processing...");
	}


	// Note that we don't clear out the logger (with m_pLog->Reset())
	// as we want top-level caller to do this. This way we can
	// still insert extra lines from top level.

	unsigned start_pos;
	start_pos = m_pAppConfig->nPosStart;
	m_nPos = start_pos;
	m_nPosEmbedStart = start_pos;	// Save the embedded file star position

	// TODO: GetLength returns ULONGLON. How to handle > 31-bit lengths (2GB)?
	m_nPosFileEnd = inFile->GetLength();

	tmpStr.Format("Start Offset: 0x%08X",start_pos);
	m_pLog->AddLine(tmpStr);


	CString strCharsIn;

	// Just in case it is an AVI file, detect the marker
	DecodeAvi();

	// FIXME Should we skip decode of file if not MJPEG?

	// If we are in a non-zero offset, add this to extras
	if (m_pAppConfig->nPosStart!=0) {
		tmpStr.Format("[Offset]=[%lu],",m_pAppConfig->nPosStart);
        m_strImgExtras += tmpStr;
	}

	unsigned nDataAfterEof = 0;

	BOOL done = FALSE;
	while (!done)
	{
		// Allow some other threads to jump in

		// Return value 0 - OK
		//              1 - Error
		//              2 - EOI
		if (DecodeMarker() != DECMARK_OK) {
			done = TRUE;
			if (m_nPosFileEnd >= m_nPosEoi) {
				nDataAfterEof = m_nPosFileEnd - m_nPosEoi;
			}
		} else {
			if (m_nPos > m_pWBuf->pos_eof) {
				m_pLog->AddLineErr(_T("ERROR: Early EOF - file may be missing EOI"));
				done = TRUE;
			}
		}
	}

	// -----------------------------------------------------------
	// Perform any other informational calculations that require all tables
	// to be present.

	// Determine the CSS Ratio
	// Save the subsampling string. Assume component 2 is
	// representative of the overall chrominance.

	//CAL! Need to make sure that I don't do any of the following
	// code if we don't complete our read (e.g. if get bad marker at start)
	// How do we determine if all is OK? Maybe presence of some specific marker?

	m_strImgQuantCss = "?x?";
	m_strHash = "NONE";
	m_strHashRot = "NONE";

	if (m_bImgOK) {
		ASSERT(m_nImgLandscape!=ENUM_LANDSCAPE_UNSET);

		//CAL! Don't use m_nImgSosCompNum as it gets reset multiple times in a progressive
		//     scan file (in each SOS segment). The first SOS = 3, but all others = 1.
		//     Using m_nImgSofCompNum, it seems that the SOF is consistent (one time).
		if (m_nImgSofCompNum == 3) {
			// Only try to determine CSS ratio if we have YCC!
			// Otherwise we will have a div/0 for m_anImgSampFactor?[1]
			if (m_nImgLandscape!=ENUM_LANDSCAPE_NO) {
				m_strImgQuantCss.Format(_T("%ux%u"),m_nImgSampFactorXMax/m_anImgSampFactorX[1],m_nImgSampFactorYMax/m_anImgSampFactorY[1]);
				//DbgAddLine(_T("CSS: Landscape"));
			}
			else {
				m_strImgQuantCss.Format(_T("%ux%u"),m_nImgSampFactorYMax/m_anImgSampFactorY[1],m_nImgSampFactorXMax/m_anImgSampFactorX[1]);
				//DbgAddLine(_T("CSS: Portrait"));
			}
		} else if (m_nImgSofCompNum == 1) {
			m_strImgQuantCss = "Gray";
		}

		DecodeEmbeddedThumb();

		// Generate the signature
		PrepareSignature();

		// Compare compression signature
		if (m_pAppConfig->bSigSearch) {
			CompareSignature();
		}

		if (nDataAfterEof > 0) {
			m_pLog->AddLine(_T(""));
			m_pLog->AddLineHdr(_T("*** Additional Info ***"));
			tmpStr.Format("NOTE: Data exists after EOF, range: 0x%08X-0x%08X (%u bytes)",
				m_nPosEoi,m_nPosFileEnd,nDataAfterEof);
			m_pLog->AddLine(tmpStr);
		}

		// Print out the special-purpose outputs
		OutputSpecial();
	}


	// Reset the status bar text
	if (m_pStatBar) {
		m_pStatBar->SetPaneText(0,"Done");
	}

	// Mark the file as closed
	//m_pWBuf->BufFileUnset();

}


// Export the embedded JPEG image at the current position in the file
// (may be the primary image or even an embedded thumbnail).
// Start be determining if the current state is sufficient for an
// image extraction.
bool CjfifDecode::ExportJpegPrepare(CString strFileIn,bool bForceSoi,bool bForceEoi,bool bIgnoreEoi)
{
	// Extract from current file
	//   [m_nPosEmbedStart ... m_nPosEmbedEnd]
	// If state is valid (i.e. file opened)

	CString		tmpStr = "";

	m_pLog->AddLine("");
	m_pLog->AddLineHdr("*** Exporting JPEG ***");

	tmpStr.Format("  Exporting from: [%s]",strFileIn);
	m_pLog->AddLine(tmpStr);

	// Only bother to extract if all main sections are present
	bool		bExtractWarn = false;
	CString		strMissing = "";

	if (!m_bStateEoi) {
		if (!bForceEoi && !bIgnoreEoi) {
			tmpStr.Format("  ERROR: Missing marker: %s","EOI");
			m_pLog->AddLineErr(tmpStr);
			m_pLog->AddLineErr("         Aborting export. Consider enabling [Force EOI] or [Ignore Missing EOI] option");
			return false;
		} else if (bIgnoreEoi) {
			// Ignore the EOI, so mark the end of file, but don't
			// set the flag where we force one.
			m_nPosEmbedEnd = m_nPosFileEnd;
		} else {
			// We're missing the EOI but the user has requested
			// that we force an EOI, so let's fix things up
			m_nPosEmbedEnd = m_nPosFileEnd;
		}
	}


	if ((m_nPosEmbedStart == 0) && (m_nPosEmbedEnd == 0)) {
		tmpStr.Format("  No frame found at this position in file. Consider using [Img Search]");
		m_pLog->AddLineErr(tmpStr);
		return false;
	}

	if (!m_bStateSoi) {
		if (!bForceSoi) {
			tmpStr.Format("  ERROR: Missing marker: %s","SOI");
			m_pLog->AddLineErr(tmpStr);
			m_pLog->AddLineErr("         Aborting export. Consider enabling [Force SOI] option");
			return false;
		} else {
			// We're missing the SOI but the user has requested
			// that we force an SOI, so let's fix things up
		}
		
	}
	if (!m_bStateSos) {
		tmpStr.Format("  ERROR: Missing marker: %s","SOS");
		m_pLog->AddLineErr(tmpStr);
		m_pLog->AddLineErr("         Aborting export");
		return false;
	}

	if (!m_bStateDqt) { bExtractWarn = true; strMissing += "DQT "; }
	if (!m_bStateDht) { bExtractWarn = true; strMissing += "DHT "; }
	if (!m_bStateSof) { bExtractWarn = true; strMissing += "SOF "; }
	
	if (bExtractWarn) {
		tmpStr.Format("  NOTE: Missing marker: %s",strMissing);
		m_pLog->AddLineWarn(tmpStr);
		m_pLog->AddLineWarn("        Exported JPEG may not be valid");
	}

	if (m_nPosEmbedEnd < m_nPosEmbedStart) {
		tmpStr.Format("ERROR: Invalid SOI-EOI order. Export aborted.");
		AfxMessageBox(tmpStr);
		m_pLog->AddLineErr(tmpStr);
		return false;
	}

	return true;
}



#define EXPORT_BUF_SIZE 131072

// Export the entire image file, using the current Buffer (with overlays)
bool CjfifDecode::ExportJpegDo(CString strFileIn, CString strFileOut, 
			unsigned long nFileLen, bool bOverlayEn,bool bDhtAviInsert,bool bForceSoi,bool bForceEoi)
{
	CFile*		pFileOutput;
	CString		tmpStr = "";

	tmpStr.Format("  Exporting to:   [%s]",strFileOut);
	m_pLog->AddLine(tmpStr);

	if (strFileIn == strFileOut) {
		tmpStr.Format("ERROR: Can't overwrite source file. Aborting export.");
		AfxMessageBox(tmpStr);
		m_pLog->AddLineErr(tmpStr);

		return false;
	}

	ASSERT(strFileIn != "");
	if (strFileIn == "") {
		tmpStr.Format("ERROR: Export but source filename empty");
		AfxMessageBox(tmpStr);
		m_pLog->AddLineErr(tmpStr);

		return false;
	}



	try
	{
		// Open specified file
		// Added in shareDenyNone as this apparently helps resolve some people's troubles
		// with an error showing: Couldn't open file "Sharing Violation"
		pFileOutput = new CFile(strFileOut, CFile::modeCreate| CFile::modeWrite | CFile::typeBinary | CFile::shareDenyNone);
	}
	catch (CFileException* e)
	{
		char msg[512];
		CString strError;
		e->GetErrorMessage(msg,sizeof(msg));
		strError.Format(_T("ERROR: Couldn't open file for write [%s]: [%s]"),
			strFileOut, msg);
		AfxMessageBox(strError);
		m_pLog->AddLineErr(strError);
		pFileOutput = NULL;

		return false;

	}

	// Don't attempt to load buffer with zero length file!
	if (nFileLen==0) {
		tmpStr.Format("ERROR: Source file length error. Please Reprocess first.");
		AfxMessageBox(tmpStr);
		m_pLog->AddLineErr(tmpStr);

		if (pFileOutput) { delete pFileOutput; pFileOutput = NULL; }
		return false;
	}


	// Need to insert fake DHT. Assume we have enough buffer allocated.
	//
	// Step 1: Copy from SOI -> SOS (not incl)
	// Step 2: Insert Fake DHT
	// Step 3: Copy from SOS -> EOI
	unsigned		nCopyStart;
	unsigned		nCopyEnd;
	unsigned		nCopyLeft;
	unsigned		ind;

	BYTE*			pBuf;

	pBuf = new BYTE[EXPORT_BUF_SIZE+10];
	if (!pBuf) {
		if (pFileOutput) { delete pFileOutput; pFileOutput = NULL; }
		return false;
	}



	// Step 1

	// If we need to force an SOI, do it now
	if (!m_bStateSoi && bForceSoi) {
		m_pLog->AddLine("    Forcing SOI Marker");
		BYTE	anBufSoi[2] = {0xFF,JFIF_SOI};
		pFileOutput->Write(&anBufSoi,2);
	}

	nCopyStart = m_nPosEmbedStart;
	nCopyEnd   = (m_nPosSos-1);
	ind = nCopyStart;
	while (ind<nCopyEnd) {
		nCopyLeft = nCopyEnd-ind+1;
		if (nCopyLeft>EXPORT_BUF_SIZE) { nCopyLeft = EXPORT_BUF_SIZE; }
		for (unsigned ind1=0;ind1<nCopyLeft;ind1++) {
			pBuf[ind1] = Buf(ind+ind1,!bOverlayEn);
		}
		pFileOutput->Write(pBuf,nCopyLeft);
		ind += nCopyLeft;
		tmpStr.Format("Exporting %3u%%...",ind*100/nFileLen);
		SetStatusText(tmpStr);
	}



	if (bDhtAviInsert) {
		// Step 2. The following struct includes the JFIF marker too
		tmpStr.Format("  Inserting standard AVI DHT huffman table");
		m_pLog->AddLine(tmpStr);
		pFileOutput->Write(m_abMJPGDHTSeg,JFIF_DHT_FAKE_SZ);
	}

	// Step 3
	nCopyStart = m_nPosSos;
	nCopyEnd   = m_nPosEmbedEnd-1;
	ind = nCopyStart;
	while (ind<nCopyEnd) {
		nCopyLeft = nCopyEnd-ind+1;
		if (nCopyLeft>EXPORT_BUF_SIZE) { nCopyLeft = EXPORT_BUF_SIZE; }
		for (unsigned ind1=0;ind1<nCopyLeft;ind1++) {
			pBuf[ind1] = Buf(ind+ind1,!bOverlayEn);
		}
		pFileOutput->Write(pBuf,nCopyLeft);
		ind += nCopyLeft;
		tmpStr.Format("Exporting %3u%%...",ind*100/nFileLen);
		SetStatusText(tmpStr);
	}

	// Now optionally insert the EOI Marker
	if (bForceEoi) {
		m_pLog->AddLine("    Forcing EOI Marker");
		BYTE	anBufEoi[2] = {0xFF,JFIF_EOI};
		pFileOutput->Write(&anBufEoi,2);
	}


	// Free up space
	pFileOutput->Close();

	if (pBuf) {
		delete [] pBuf;
		pBuf = NULL;
	}

	if (pFileOutput) {
		delete pFileOutput;
		pFileOutput = NULL;
	}

	SetStatusText("");
	tmpStr.Format("  Export done");
	m_pLog->AddLine(tmpStr);

	return true;
}


// Export a subset of the file with no overlays or mods
bool CjfifDecode::ExportJpegDoRange(CString strFileIn, CString strFileOut, 
			unsigned long nStart, unsigned long nEnd)
{
	CFile*		pFileOutput;
	CString		tmpStr = "";

	tmpStr.Format("  Exporting range to:   [%s]",strFileOut);
	m_pLog->AddLine(tmpStr);

	if (strFileIn == strFileOut) {
		tmpStr.Format("ERROR: Can't overwrite source file. Aborting export.");
		AfxMessageBox(tmpStr);
		m_pLog->AddLineErr(tmpStr);

		return false;
	}

	ASSERT(strFileIn != "");
	if (strFileIn == "") {
		tmpStr.Format("ERROR: Export but source filename empty");
		AfxMessageBox(tmpStr);
		m_pLog->AddLineErr(tmpStr);

		return false;
	}



	try
	{
		// Open specified file
		// Added in shareDenyNone as this apparently helps resolve some people's troubles
		// with an error showing: Couldn't open file "Sharing Violation"
		pFileOutput = new CFile(strFileOut, CFile::modeCreate| CFile::modeWrite | CFile::typeBinary | CFile::shareDenyNone);
	}
	catch (CFileException* e)
	{
		char msg[512];
		CString strError;
		e->GetErrorMessage(msg,sizeof(msg));
		strError.Format(_T("ERROR: Couldn't open file for write [%s]: [%s]"),
			strFileOut, msg);
		AfxMessageBox(strError);
		m_pLog->AddLineErr(strError);
		pFileOutput = NULL;

		return false;

	}


	unsigned		nCopyStart;
	unsigned		nCopyEnd;
	unsigned		nCopyLeft;
	unsigned		ind;

	BYTE*			pBuf;

	pBuf = new BYTE[EXPORT_BUF_SIZE+10];
	if (!pBuf) {
		if (pFileOutput) { delete pFileOutput; pFileOutput = NULL; }
		return false;
	}



	// Step 1
	nCopyStart = nStart;
	nCopyEnd   = nEnd;
	ind = nCopyStart;
	while (ind<nCopyEnd) {
		nCopyLeft = nCopyEnd-ind+1;
		if (nCopyLeft>EXPORT_BUF_SIZE) { nCopyLeft = EXPORT_BUF_SIZE; }
		for (unsigned ind1=0;ind1<nCopyLeft;ind1++) {
			pBuf[ind1] = Buf(ind+ind1,false);
		}
		pFileOutput->Write(pBuf,nCopyLeft);
		ind += nCopyLeft;
		tmpStr.Format("Exporting %3u%%...",ind*100/(nCopyEnd-nCopyStart));
		SetStatusText(tmpStr);
	}


	// Free up space
	pFileOutput->Close();

	if (pBuf) {
		delete [] pBuf;
		pBuf = NULL;
	}

	if (pFileOutput) {
		delete pFileOutput;
		pFileOutput = NULL;
	}

	SetStatusText("");
	tmpStr.Format("  Export range done");
	m_pLog->AddLine(tmpStr);

	return true;
}



// List of the JFIF markers
const MarkerNameTable CjfifDecode::m_pMarkerNames[] = {
	{JFIF_SOI,"SOI"},
	{JFIF_APP0,"APP0"},
	{JFIF_APP1,"APP1"},
	{JFIF_APP2,"APP2"},
	{JFIF_APP3,"APP3"},
	{JFIF_APP4,"APP4"},
	{JFIF_APP5,"APP5"},
	{JFIF_APP6,"APP6"},
	{JFIF_APP7,"APP7"},
	{JFIF_APP8,"APP8"},
	{JFIF_APP9,"APP9"},
	{JFIF_APP10,"APP10"},
	{JFIF_APP11,"APP11"},
	{JFIF_APP12,"APP12"},
	{JFIF_APP13,"APP13"},
	{JFIF_APP14,"APP14"},
	{JFIF_APP15,"APP15"},
	{JFIF_SOI,"SOI"},
	{JFIF_DQT,"DQT"},
	{JFIF_DHT,"DHT"},
	{JFIF_SOF0,"SOF0"},
	{JFIF_SOF1,"SOF1"},
	{JFIF_SOF2,"SOF2"},
	{JFIF_SOF3,"SOF3"},
	{JFIF_SOF5,"SOF5"},
	{JFIF_SOF6,"SOF6"},
	{JFIF_SOF7,"SOF7"},
	{JFIF_SOF9,"SOF9"},
	{JFIF_SOF10,"SOF10"},
	{JFIF_SOF11,"SOF11"},
	{JFIF_SOF12,"SOF12"},
	{JFIF_SOF13,"SOF13"},
	{JFIF_SOF14,"SOF14"},
	{JFIF_SOF15,"SOF15"},
	{JFIF_JPG,"JPG"},
	{JFIF_COM,"COM"},
	{JFIF_DHT,"DHT"},
	{JFIF_SOS,"SOS"},
	{JFIF_DRI,"DRI"},
	{JFIF_RST0,"RST0"},
	{JFIF_RST1,"RST1"},
	{JFIF_RST2,"RST2"},
	{JFIF_RST3,"RST3"},
	{JFIF_RST4,"RST4"},
	{JFIF_RST5,"RST5"},
	{JFIF_RST6,"RST6"},
	{JFIF_RST7,"RST7"},
	{JFIF_EOI,"EOI"},
	{JFIF_RST1,"DQT"},
	{0x00,"*"},
};


// For Motion JPEG, define the DHT tables that we use since they won't exist
// in each frame within the AVI. This table will be read in during
// DecodeDHT()'s call to Buf().
const BYTE CjfifDecode::m_abMJPGDHTSeg[JFIF_DHT_FAKE_SZ] = {
	/* JPEG DHT Segment for YCrCb omitted from MJPG data */
	0xFF,0xC4,0x01,0xA2,
		0x00,0x00,0x01,0x05,0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x01,0x00,0x03,0x01,0x01,0x01,0x01,
		0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
		0x08,0x09,0x0A,0x0B,0x10,0x00,0x02,0x01,0x03,0x03,0x02,0x04,0x03,0x05,0x05,0x04,0x04,0x00,
		0x00,0x01,0x7D,0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,0x61,
		0x07,0x22,0x71,0x14,0x32,0x81,0x91,0xA1,0x08,0x23,0x42,0xB1,0xC1,0x15,0x52,0xD1,0xF0,0x24,
		0x33,0x62,0x72,0x82,0x09,0x0A,0x16,0x17,0x18,0x19,0x1A,0x25,0x26,0x27,0x28,0x29,0x2A,0x34,
		0x35,0x36,0x37,0x38,0x39,0x3A,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x53,0x54,0x55,0x56,
		0x57,0x58,0x59,0x5A,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x73,0x74,0x75,0x76,0x77,0x78,
		0x79,0x7A,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,
		0x9A,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,
		0xBA,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,
		0xDA,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,
		0xF8,0xF9,0xFA,0x11,0x00,0x02,0x01,0x02,0x04,0x04,0x03,0x04,0x07,0x05,0x04,0x04,0x00,0x01,
		0x02,0x77,0x00,0x01,0x02,0x03,0x11,0x04,0x05,0x21,0x31,0x06,0x12,0x41,0x51,0x07,0x61,0x71,
		0x13,0x22,0x32,0x81,0x08,0x14,0x42,0x91,0xA1,0xB1,0xC1,0x09,0x23,0x33,0x52,0xF0,0x15,0x62,
		0x72,0xD1,0x0A,0x16,0x24,0x34,0xE1,0x25,0xF1,0x17,0x18,0x19,0x1A,0x26,0x27,0x28,0x29,0x2A,
		0x35,0x36,0x37,0x38,0x39,0x3A,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x53,0x54,0x55,0x56,
		0x57,0x58,0x59,0x5A,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x73,0x74,0x75,0x76,0x77,0x78,
		0x79,0x7A,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x92,0x93,0x94,0x95,0x96,0x97,0x98,
		0x99,0x9A,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,
		0xB9,0xBA,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,
		0xD9,0xDA,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,
		0xF9,0xFA
};


//CAL! TODO:
// Add ITU-T Example DQT & DHT
// These will be useful for GeoRaster decode (ie. JPEG-B)
