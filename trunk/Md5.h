// JPEGsnoop - JPEG Image Decoder & Analysis Utility
// Copyright (C) 2010 - Calvin Hass
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

/*
 **********************************************************************
 ** MD5.h                                                            **
 **                                                                  **
 ** - Style modified by Tony Ray, January 2001                       **
 **   Added support for randomizing initialization constants         **
 ** - Style modified by Dominik Reichl, September 2002               **
 **   Optimized code                                                 **
 **                                                                  **
 **********************************************************************
 */

/*
 **********************************************************************
 ** MD5.h -- Header file for implementation of MD5                   **
 ** RSA Data Security, Inc. MD5 Message Digest Algorithm             **
 ** Created: 2/17/90 RLR                                             **
 ** Revised: 12/27/90 SRD,AJ,BSK,JT Reference C version              **
 ** Revised (for MD5): RLR 4/27/91                                   **
 **   -- G modified to have y&~z instead of y&z                      **
 **   -- FF, GG, HH modified to add in last register done            **
 **   -- Access pattern: round 2 works mod 5, round 3 works mod 3    **
 **   -- distinct additive constant for each step                    **
 **   -- round 4 added, working mod 7                                **
 **********************************************************************
 */

/*
 **********************************************************************
 ** Copyright (C) 1990, RSA Data Security, Inc. All rights reserved. **
 **                                                                  **
 ** License to copy and use this software is granted provided that   **
 ** it is identified as the "RSA Data Security, Inc. MD5 Message     **
 ** Digest Algorithm" in all material mentioning or referencing this **
 ** software or this function.                                       **
 **                                                                  **
 ** License is also granted to make and use derivative works         **
 ** provided that such works are identified as "derived from the RSA **
 ** Data Security, Inc. MD5 Message Digest Algorithm" in all         **
 ** material mentioning or referencing the derived work.             **
 **                                                                  **
 ** RSA Data Security, Inc. makes no representations concerning      **
 ** either the merchantability of this software or the suitability   **
 ** of this software for any particular purpose.  It is provided "as **
 ** is" without express or implied warranty of any kind.             **
 **                                                                  **
 ** These notices must be retained in any copies of any part of this **
 ** documentation and/or software.                                   **
 **********************************************************************
 */

#ifndef ___MD5_H___
#define ___MD5_H___

/* Typedef a 32 bit type */
#ifndef UINT4
typedef unsigned long int UINT4;
#endif

/* Data structure for MD5 (Message Digest) computation */
typedef struct {
	UINT4 i[2];                   /* Number of _bits_ handled mod 2^64 */
	UINT4 buf[4];                                    /* Scratch buffer */
	unsigned char in[64];                              /* Input buffer */
	unsigned char digest[16];     /* Actual digest after MD5Final call */

	unsigned int digest32[4]; //CAL!
} MD5_CTX;

static void MD5_Transform (UINT4 *buf, UINT4 *in);

void MD5Init(MD5_CTX *mdContext, unsigned long pseudoRandomNumber = 0);
void MD5Update(MD5_CTX *mdContext, unsigned char *inBuf, unsigned int inLen);
void MD5Final(MD5_CTX *mdContext);

#endif /* ___MD5_H___ included */
