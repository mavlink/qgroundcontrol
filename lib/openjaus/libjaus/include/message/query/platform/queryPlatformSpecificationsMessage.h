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
// File Name: queryPlatformSpecificationsMessage.h
//
// Written By: Danny Kent (jaus AT dannykent DOT com), Tom Galluzzo 
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: This file defines the attributes of a QueryPlatformSpecificationsMessage

#ifndef QUERY_PLATFORM_SPECIFICATIONS_MESSAGE_H
#define QUERY_PLATFORM_SPECIFICATIONS_MESSAGE_H

#include "jaus.h"

#ifndef JAUS_PLATFORM_PV
#define JAUS_PLATFORM_PV
#define JAUS_PLATFORM_PV_PLATFORM_NAME_BIT		0
#define JAUS_PLATFORM_PV_FRONT_BIT				1
#define JAUS_PLATFORM_PV_BACK_BIT				2
#define JAUS_PLATFORM_PV_RIGHT_BIT				3
#define JAUS_PLATFORM_PV_LEFT_BIT				4
#define JAUS_PLATFORM_PV_BOTTOM_BIT				5
#define JAUS_PLATFORM_PV_TOP_BIT				6
#define JAUS_PLATFORM_PV_X_CG_BIT				7
#define JAUS_PLATFORM_PV_Y_CG_BIT				8
#define JAUS_PLATFORM_PV_Z_CG_BIT				9
#define JAUS_PLATFORM_PV_TURNING_RADIUS_BIT		10
#define JAUS_PLATFORM_PV_WHEEL_BASE_BIT			11
#define JAUS_PLATFORM_PV_TRACK_WIDTH_BIT		12
#define JAUS_PLATFORM_PV_PITCH_OVER_BIT			13
#define JAUS_PLATFORM_PV_ROLL_OVER_BIT			14
#define JAUS_PLATFORM_PV_MAX_VELOCITY_X_BIT		15
#define JAUS_PLATFORM_PV_MAX_VELOCITY_Y_BIT		16
#define JAUS_PLATFORM_PV_MAX_VELOCITY_Z_BIT		17
#define JAUS_PLATFORM_PV_MAX_ROLL_RATE_BIT		18
#define JAUS_PLATFORM_PV_MAX_PITCH_RATE_BIT		19
#define JAUS_PLATFORM_PV_MAX_YAW_RATE_BIT		20
#endif

typedef struct
{
	// Include all parameters from a JausMessage structure:
	// Header Properties
	struct
	{
		// Properties by bit fields
		#ifdef JAUS_BIG_ENDIAN
			JausUnsignedShort reserved:2;
			JausUnsignedShort version:6;
			JausUnsignedShort expFlag:1;
			JausUnsignedShort scFlag:1;
			JausUnsignedShort ackNak:2;
			JausUnsignedShort priority:4; 
		#elif JAUS_LITTLE_ENDIAN
			JausUnsignedShort priority:4; 
			JausUnsignedShort ackNak:2;
			JausUnsignedShort scFlag:1; 
			JausUnsignedShort expFlag:1;
			JausUnsignedShort version:6; 
			JausUnsignedShort reserved:2;
		#else
			#error "Please define system endianess (see jaus.h)"
		#endif
	}properties;

	JausUnsignedShort commandCode; 

	JausAddress destination;

	JausAddress source;

	JausUnsignedInteger dataSize;

	JausUnsignedInteger dataFlag;
	
	JausUnsignedShort sequenceNumber;

	JausUnsignedInteger presenceVector;
	
}QueryPlatformSpecificationsMessageStruct;

typedef QueryPlatformSpecificationsMessageStruct* QueryPlatformSpecificationsMessage;

JAUS_EXPORT QueryPlatformSpecificationsMessage queryPlatformSpecificationsMessageCreate(void);
JAUS_EXPORT void queryPlatformSpecificationsMessageDestroy(QueryPlatformSpecificationsMessage);

JAUS_EXPORT JausBoolean queryPlatformSpecificationsMessageFromBuffer(QueryPlatformSpecificationsMessage message, unsigned char* buffer, unsigned int bufferSizeBytes);
JAUS_EXPORT JausBoolean queryPlatformSpecificationsMessageToBuffer(QueryPlatformSpecificationsMessage message, unsigned char *buffer, unsigned int bufferSizeBytes);

JAUS_EXPORT QueryPlatformSpecificationsMessage queryPlatformSpecificationsMessageFromJausMessage(JausMessage jausMessage);
JAUS_EXPORT JausMessage queryPlatformSpecificationsMessageToJausMessage(QueryPlatformSpecificationsMessage message);

JAUS_EXPORT unsigned int queryPlatformSpecificationsMessageSize(QueryPlatformSpecificationsMessage message);

#endif // QUERY_PLATFORM_SPECIFICATIONS_MESSAGE_H
