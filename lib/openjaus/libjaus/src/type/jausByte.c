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
// File Name: jausByte.c
//
// Written By: Danny Kent (jaus AT dannykent DOT com), Tom Galluzzo (galluzzo AT gmail DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: This file defines all the basic JausByte funtionality, this should be primarily used through the 
// JausType file and its methods

#include <stdlib.h>
#include "jaus.h"

JausByte newJausByte(unsigned char value)
{
	return value;
}

JausBoolean jausByteFromBuffer(JausByte *jByte, unsigned char *buf, unsigned int bufferSizeBytes)
{
	if(bufferSizeBytes < JAUS_BYTE_SIZE_BYTES)
	{
		return JAUS_FALSE; // not enough data in buffer
	}
	else
	{
		*jByte = newJausByte((unsigned char)buf[0]);
		return JAUS_TRUE;
	}	
}

JausBoolean jausByteToBuffer(JausByte input, unsigned char *buf, unsigned int bufferSizeBytes)
{
	if(bufferSizeBytes < JAUS_BYTE_SIZE_BYTES) 
	{
		return JAUS_FALSE; // not enough room
	}
	else
	{
		buf[0] = input;
		return JAUS_TRUE;
	}
}

double jausByteToDouble(JausByte input, double min, double max)
{
	// BUG: What to do when max < min
	double scaleFactor = (max-min)/JAUS_BYTE_RANGE;
	double bias = min;
  
	return input*scaleFactor + bias;
}

JausByte jausByteFromDouble(double value, double min, double max)
{
	// BUG: What to do when max < min
	double scaleFactor = (max-min)/JAUS_BYTE_RANGE;
	double bias = min;
	
	// limit real number between min and max
	if(value < min) value = min;
	if(value > max) value = max;
		
	// return rounded integer value
	return newJausByte((unsigned char)((value - bias)/scaleFactor + 0.5));
}

JausBoolean jausByteIsBitSet(JausByte input, int bit )
{
        return (input & (0x01 << bit)) > 0 ? JAUS_TRUE : JAUS_FALSE;
}

JausBoolean jausByteSetBit(JausByte *input, int bit)
{
        if(JAUS_BYTE_SIZE_BYTES*8 < bit) // 8 bits per byte
        {
                return JAUS_FALSE;
        }
        else
        {
                *input |= 0x01 << bit;
                return JAUS_TRUE;
        }
}

JausBoolean jausByteClearBit(JausByte *input, int bit)
{
        if(JAUS_BYTE_SIZE_BYTES*8 < bit) // 8 bits per byte
        {
                return JAUS_FALSE;
        }
        else
        {
                *input &= ~(0x01 << bit);
                return JAUS_TRUE;
        }
}

