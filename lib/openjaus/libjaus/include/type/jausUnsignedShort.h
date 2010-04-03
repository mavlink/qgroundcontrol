/*****************************************************************************
 *  Copyright (c) 2008, University of Florida
 *  All rights reserved.
 *  
 *  This file is part of OpenJAUS.  OpenJAUS is distributed under the BSD 
 *  license.  See the LICENSE file for details.
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of the University of Florida nor the names of its 
 *       contributors may be used to endorse or promote products derived from 
 *       this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ****************************************************************************/
// File Name: jausUnsignedShort.h
//
// Written By: Danny Kent (jaus AT dannykent DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: This file defines all the basic JausUnsignedShort funtionality, this should be primarily used 
// through the JausType file and its methods

#ifndef JAUS_UNSIGNED_SHORT_H
#define JAUS_UNSIGNED_SHORT_H

#include "jaus.h"

#define JAUS_UNSIGNED_SHORT_SIZE_BYTES	2

#define JAUS_UNSIGNED_SHORT_RANGE 		65535.0 // NOTE: This range is incorrect, but is as defined by Jaus
#define JAUS_UNSIGNED_SHORT_MAX_VALUE 	65535
#define JAUS_UNSIGNED_SHORT_MIN_VALUE 	0

#define JAUS_SHORT_PRESENCE_VECTOR_ALL_ON 	65535

typedef unsigned short JausUnsignedShort;

JAUS_EXPORT JausUnsignedShort newJausUnsignedShort(unsigned short val);

JAUS_EXPORT JausBoolean jausUnsignedShortFromBuffer(JausUnsignedShort *jUShort, unsigned char *buf, unsigned int bufferSizeBytes);
JAUS_EXPORT JausBoolean jausUnsignedShortToBuffer(JausUnsignedShort input, unsigned char *buf, unsigned int bufferSizeBytes);

JAUS_EXPORT double jausUnsignedShortToDouble(JausUnsignedShort input, double min, double max);
JAUS_EXPORT JausUnsignedShort jausUnsignedShortFromDouble(double value, double min, double max);

JAUS_EXPORT JausBoolean jausUnsignedShortIsBitSet(JausUnsignedShort input, int bit);
JAUS_EXPORT JausBoolean jausUnsignedShortSetBit(JausUnsignedShort *input, int bit);
JAUS_EXPORT JausBoolean jausUnsignedShortClearBit(JausUnsignedShort *input, int bit);

#endif // JAUS_UNSIGNED_SHORT_H
