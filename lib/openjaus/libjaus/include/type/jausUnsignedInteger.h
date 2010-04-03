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
// File Name: jausUnsignedInteger.h
//
// Written By: Danny Kent (jaus AT dannykent DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: This file defines all the basic JausUnsignedInteger funtionality, this should be primarily used 
// through the JausType file and its methods

#ifndef JAUS_UNSIGNED_INTEGER_H
#define JAUS_UNSIGNED_INTEGER_H

#include "jaus.h"

#define JAUS_UNSIGNED_INTEGER_SIZE_BYTES	4

#define JAUS_UNSIGNED_INTEGER_RANGE 		4294967294.0 // NOTE: This range is incorrect, but is as defined by Jaus
#define JAUS_UNSIGNED_INTEGER_MAX_VALUE 	4294967295L
#define JAUS_UNSIGNED_INTEGER_MIN_VALUE 	0

#define JAUS_INTEGER_PRESENCE_VECTOR_ALL_ON 	4294967295L

typedef unsigned int JausUnsignedInteger;

JAUS_EXPORT JausUnsignedInteger newJausUnsignedInteger(unsigned int val);

JAUS_EXPORT JausBoolean jausUnsignedIntegerFromBuffer(JausUnsignedInteger *jUint, unsigned char *buf, unsigned int bufferSizeBytes);
JAUS_EXPORT JausBoolean jausUnsignedIntegerToBuffer(JausUnsignedInteger input, unsigned char *buf, unsigned int bufferSizeBytes);

JAUS_EXPORT double jausUnsignedIntegerToDouble(JausUnsignedInteger input, double min, double max);
JAUS_EXPORT JausUnsignedInteger jausUnsignedIntegerFromDouble(double value, double min, double max);

JAUS_EXPORT JausBoolean jausUnsignedIntegerIsBitSet(JausUnsignedInteger input, int bit);
JAUS_EXPORT JausBoolean jausUnsignedIntegerSetBit(JausUnsignedInteger *input, int bit);
JAUS_EXPORT JausBoolean jausUnsignedIntegerClearBit(JausUnsignedInteger *input, int bit);

#endif // JAUS_UNSIGNED_INTEGER_H
