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
// File Name: jausByte.h
//
// Written By: Danny Kent (jaus AT dannykent DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: This file defines all the basic JausByte funtionality, this should be primarily used 
// through the JausType file and its methods

#ifndef JAUS_BYTE_H
#define JAUS_BYTE_H

#include "jaus.h"

#define JAUS_BYTE_SIZE_BYTES	1 // size

#define JAUS_BYTE_RANGE 	255.0
#define JAUS_BYTE_MAX_VALUE 255
#define JAUS_BYTE_MIN_VALUE 0

#define JAUS_BYTE_PRESENCE_VECTOR_ALL_ON	255

typedef unsigned char JausByte;

JAUS_EXPORT JausByte newJausByte(unsigned char val);

JAUS_EXPORT JausBoolean jausByteFromBuffer(JausByte *jByte, unsigned char *buf, unsigned int bufferSizeBytes);
JAUS_EXPORT JausBoolean jausByteToBuffer(JausByte input, unsigned char *buf, unsigned int bufferSizeBytes);

JAUS_EXPORT JausByte jausByteFromDouble(double val, double min, double max);
JAUS_EXPORT double jausByteToDouble(JausByte input, double min, double max);

JAUS_EXPORT JausBoolean jausByteIsBitSet(JausByte byte, int bit);
JAUS_EXPORT JausBoolean jausByteSetBit(JausByte *byte, int bit);
JAUS_EXPORT JausBoolean jausByteClearBit(JausByte *byte, int bit);

#endif //JAUS_BYTE_H
