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
// File Name: queryVksObjectsMessage.h
//
// Written By: Danny Kent (jaus AT dannykent DOT com) 
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: This file defines the attributes of a QueryVksObjectsMessage

#ifndef QUERY_VKS_OBJECTS_MESSAGE_H
#define QUERY_VKS_OBJECTS_MESSAGE_H

#include "jaus.h"

// Note: The PV for this message is a bit over-defined
// If the VKS_PV_QUERY_OBJECTS_REGION_BIT is set, then the VKS_PV_QUERY_OBJECTS_POINT_COUNT_BIT is required
// The VKS_PV_QUERY_OBJECTS_BUFFER_BIT cannot be set without the VKS_PV_QUERY_OBJECTS_REGION_BIT
//
// If the VKS_PV_QUERY_OBJECTS_FEATURE_CLASS_BIT is set or the VKS_PV_QUERY_OBJECTS_ATTRIBUTE_BIT is set, 
// then the VKS_PV_QUERY_OBJECTS_FC_COUNT_BIT is required
#ifndef VKS_PV_QUERY_OBJECTS
#define VKS_PV_QUERY_OBJECTS
#define VKS_PV_QUERY_OBJECTS_ID_BIT					0
#define VKS_PV_QUERY_OBJECTS_REGION_BIT				1
#define VKS_PV_QUERY_OBJECTS_BUFFER_BIT				2
#define VKS_PV_QUERY_OBJECTS_FC_COUNT_BIT			3
#define VKS_PV_QUERY_OBJECTS_FEATURE_CLASS_BIT		4
#define VKS_PV_QUERY_OBJECTS_ATTRIBUTE_BIT			5
#define VKS_PV_QUERY_OBJECTS_POINT_COUNT_BIT		6
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

	// MESSAGE DATA MEMBERS GO HERE
	JausByte presenceVector;				// 1: Presence Vector
	JausByte responsePresenceVector;		// 2: Presence Vector to be used in response (defines the data requested)
	JausByte requestId;									// 3: Local Request Id
	JausUnsignedShort objectCount;						// 4: Count of specific object ids requested
	JausUnsignedInteger *objectIds;						// 5: Array of specific object ids
	JausWorldModelVectorObject queryRegion;				// 6: Query region with associated FCs and attributes
	
}QueryVksObjectsMessageStruct;

typedef QueryVksObjectsMessageStruct* QueryVksObjectsMessage;

JAUS_EXPORT QueryVksObjectsMessage queryVksObjectsMessageCreate(void);
JAUS_EXPORT void queryVksObjectsMessageDestroy(QueryVksObjectsMessage);

JAUS_EXPORT JausBoolean queryVksObjectsMessageFromBuffer(QueryVksObjectsMessage message, unsigned char* buffer, unsigned int bufferSizeBytes);
JAUS_EXPORT JausBoolean queryVksObjectsMessageToBuffer(QueryVksObjectsMessage message, unsigned char *buffer, unsigned int bufferSizeBytes);

JAUS_EXPORT QueryVksObjectsMessage queryVksObjectsMessageFromJausMessage(JausMessage jausMessage);
JAUS_EXPORT JausMessage queryVksObjectsMessageToJausMessage(QueryVksObjectsMessage message);

JAUS_EXPORT unsigned int queryVksObjectsMessageSize(QueryVksObjectsMessage message);

#endif // QUERY_VKS_OBJECTS_MESSAGE_H
