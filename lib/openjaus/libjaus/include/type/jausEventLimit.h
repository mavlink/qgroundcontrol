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
// File Name: JausEventLimit.h
//
// Written By: Danny Kent (jaus AT dannykent DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: This file describes the stucture and functionality associated with an 
// EventLimit for use with the Jaus Events Message Set
#ifndef EVENT_LIMITS_H
#define EVENT_LIMITS_H

typedef struct
{
	enum
	{
		EVENT_LIMIT_UNDEFINED_TYPE = -1,
		EVENT_LIMIT_BYTE_TYPE = 0,
		EVENT_LIMIT_SHORT_TYPE = 1,
		EVENT_LIMIT_INTEGER_TYPE = 2,
		EVENT_LIMIT_LONG_TYPE = 3,
		EVENT_LIMIT_UNSIGNED_SHORT_TYPE = 4,
		EVENT_LIMIT_UNSIGNED_INTEGER_TYPE = 5,
		EVENT_LIMIT_UNSIGNED_LONG_TYPE = 6,
		EVENT_LIMIT_FLOAT_TYPE = 7,
		EVENT_LIMIT_DOUBLE_TYPE = 8,
		EVENT_LIMIT_RGB_TYPE = 9		
	}dataType;
	
	union
	{
		JausByte byteValue;
		JausShort shortValue;
		JausInteger integerValue;
		JausLong longValue;
		JausUnsignedShort unsignedShortValue;
		JausUnsignedInteger unsignedIntegerValue;
		JausUnsignedLong unsignedLongValue;
		JausFloat floatValue;
		JausDouble doubleValue;
		struct
		{
			JausByte redValue;
			JausByte greenValue;
			JausByte blueValue;
		}rgb;
	}value;
}JausEventLimitStruct;

typedef JausEventLimitStruct *JausEventLimit;

typedef struct JausEventListStruct
{
	void *eventMessage;
	JausByte eventId;
	JausEventLimit previousLimitValue;
}JausEventStruct;

typedef JausEventStruct *JausEvent;

// JausEventLimit Constructor
JAUS_EXPORT JausEventLimit jausEventLimitCreate();

// JausEventLimit Destructor
JAUS_EXPORT void jausEventLimitDestroy(JausEventLimit limit);

// JausEventLimit Constructor (from Buffer)
JAUS_EXPORT JausBoolean jausEventLimitFromBuffer(JausEventLimit *limit, unsigned char *buffer, unsigned int bufferSizeBytes);

// JausEventLimit To Buffer
JAUS_EXPORT JausBoolean jausEventLimitToBuffer(JausEventLimit limit, unsigned char *buffer, unsigned int bufferSizeBytes);

JAUS_EXPORT unsigned int jausEventLimitSize(JausEventLimit limit);

JAUS_EXPORT JausEvent jausEventCreate();
JAUS_EXPORT void jausEventDestroy(JausEvent event);

#endif
