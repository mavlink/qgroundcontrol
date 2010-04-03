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
// File Name: reportEventsMessage.h
//
// Written By: Danny Kent (jaus AT dannykent DOT com), Tom Galluzzo 
//
// Version: 3.3.0a
//
// Date: 11/2/05
//
// This file defines the attributes of a ReportEventsMessage

#ifndef REPORT_EVENTS_MESSAGE_H
#define REPORT_EVENTS_MESSAGE_H

#include "jaus.h"

// PV as defined in v3.3
#ifndef REPORT_EVENTS_PV
#define REPORT_EVENTS_PV_BOUNDARY_BIT		0	// Field 5
#define REPORT_EVENTS_PV_DATA_FIELD_BIT		1	// Field 6
#define REPORT_EVENTS_PV_LOWER_LIMIT_BIT		2	// Field 7/8
#define REPORT_EVENTS_PV_UPPER_LIMIT_BIT		3	// Field 9/10
#define REPORT_EVENTS_PV_STATE_LIMIT_BIT		4	// Field 11/12
#define REPORT_EVENTS_PV_EVENT_ID_BIT			5	// Field 15/16
#define REPORT_EVENTS_PV_QUERY_MESSAGE_BIT	6	// Field 15/16

#endif

// Event Types
#ifndef EVENT_TYPES
#define EVENT_TYPES
#define EVENT_PERIODIC_TYPE						0
#define EVENT_EVERY_CHANGE_TYPE					1
#define EVENT_FIRST_CHANGE_TYPE					2
#define EVENT_FIRST_CHANGE_IN_AND_OUT_TYPE		3
#define EVENT_PERIODIC_NO_REPEAT_TYPE			4
#define EVENT_ONE_TIME_ON_DEMAND_TYPE			5
#endif

// Event Boundaries
#ifndef EVENT_BOUNDARIES
#define EVENT_BOUNDARIES
#define EQUAL_BOUNDARY							0
#define NOT_EQUAL_BOUNDARY						1
#define INSIDE_INCLUSIVE_BOUNDARY				2
#define INSIDE_EXCLUSIVE_BOUNDARY				3
#define OUTSIDE_INCLUSIVE_BOUNDARY				4
#define OUTSIDE_EXCLUSIVE_BOUNDARY				5
#define GREATER_THAN_OR_EQUAL_BOUNDARY			6
#define GREATER_THAN_BOUNDARY					7
#define LESS_THAN_OR_EQUAL_BOUNDARY				8
#define LESS_THAN_BOUNDARY						9
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
	JausByte count;						// Number of events reported
	JausByte *presenceVector;			// Presence Vector
	JausUnsignedShort *messageCode;			// Command Code of the resulting query
	JausByte *eventType;						// Enumeration of Event types
	JausByte *eventBoundary;					// Enumeration of Event Boundary Conditions
	JausByte *limitDataField;				// Field from Report for Limit Trigger
	JausEventLimit *lowerLimit;				// Lower Event Limit
	JausEventLimit *upperLimit;				// Upper Event Limit
	JausEventLimit *stateLimit;				// State Event Limit used for Equal Boundary
	JausByte *eventId;						// ID of event to be updated
	JausMessage *queryMessage;				// Array to Query Messages (including header) to use for response
}ReportEventsMessageStruct;

typedef ReportEventsMessageStruct* ReportEventsMessage;

JAUS_EXPORT ReportEventsMessage reportEventsMessageCreate(void);
JAUS_EXPORT void reportEventsMessageDestroy(ReportEventsMessage);

JAUS_EXPORT JausBoolean reportEventsMessageFromBuffer(ReportEventsMessage message, unsigned char* buffer, unsigned int bufferSizeBytes);
JAUS_EXPORT JausBoolean reportEventsMessageToBuffer(ReportEventsMessage message, unsigned char *buffer, unsigned int bufferSizeBytes);

JAUS_EXPORT ReportEventsMessage reportEventsMessageFromJausMessage(JausMessage jausMessage);
JAUS_EXPORT JausMessage reportEventsMessageToJausMessage(ReportEventsMessage message);

JAUS_EXPORT unsigned int reportEventsMessageSize(ReportEventsMessage message);

#endif // REPORT_EVENTS_MESSAGE_H
