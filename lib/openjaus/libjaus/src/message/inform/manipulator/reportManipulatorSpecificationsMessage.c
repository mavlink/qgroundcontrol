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
// File Name: reportManipulatorSpecificationsMessage.c
//
// Written By: Danny Kent (jaus AT dannykent DOT com), Tom Galluzzo (galluzzo AT gmail DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: This file defines the functionality of a ReportManipulatorSpecificationsMessage

#include <stdlib.h>
#include <string.h>
#include "jaus.h"

static const int commandCode = JAUS_REPORT_MANIPULATOR_SPECIFICATIONS;
static const int maxDataSizeBytes = 0;

static JausBoolean headerFromBuffer(ReportManipulatorSpecificationsMessage message, unsigned char *buffer, unsigned int bufferSizeBytes);
static JausBoolean headerToBuffer(ReportManipulatorSpecificationsMessage message, unsigned char *buffer, unsigned int bufferSizeBytes);

static JausBoolean dataFromBuffer(ReportManipulatorSpecificationsMessage message, unsigned char *buffer, unsigned int bufferSizeBytes);
static int dataToBuffer(ReportManipulatorSpecificationsMessage message, unsigned char *buffer, unsigned int bufferSizeBytes);
static void dataInitialize(ReportManipulatorSpecificationsMessage message);
static void dataDestroy(ReportManipulatorSpecificationsMessage message);
static unsigned int dataSize(ReportManipulatorSpecificationsMessage message);

// ************************************************************************************************************** //
//                                    USER CONFIGURED FUNCTIONS
// ************************************************************************************************************** //

// Initializes the message-specific fields
static void dataInitialize(ReportManipulatorSpecificationsMessage message)
{
	// Set initial values of message fields
	message->numJoints = newJausByte(0);
	message->jointType = NULL;	
	message->jointOffset = NULL;	
	message->linkLength = NULL;	
	message->twistAngle = NULL;	
	message->jointMinValue = NULL;	
	message->jointMaxValue = NULL;	
	message->jointMaxVelocity = NULL;	
	message->coordinateSysX = newJausDouble(0);
	message->coordinateSysY = newJausDouble(0);
	message->coordinateSysZ = newJausDouble(0);
	message->coordinateSysA = newJausDouble(0);
	message->coordinateSysB = newJausDouble(0);
	message->coordinateSysC = newJausDouble(0);
	message->coordinateSysD = newJausDouble(0);
}

// Destructs the message-specific fields
static void dataDestroy(ReportManipulatorSpecificationsMessage message)
{
	// Free message fields
	if(message->jointType != NULL)
	{
		free(message->jointType);
	}

	if(message->jointOffset != NULL)
	{
		free(message->jointOffset);
	}

	if(message->linkLength != NULL)
	{
		free(message->linkLength);
	}

	if(message->twistAngle != NULL)
	{
		free(message->twistAngle);
	}

	if(message->jointMinValue != NULL)
	{
		free(message->jointMinValue);
	}

	if(message->jointMaxValue != NULL)
	{
		free(message->jointMaxValue);
	}

	if(message->jointMaxVelocity != NULL)
	{
		free(message->jointMaxVelocity);
	}
	
}

// Return boolean of success
static JausBoolean dataFromBuffer(ReportManipulatorSpecificationsMessage message, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	int index = 0;
	int i;
	JausUnsignedShort tempUShort;
	JausUnsignedInteger tempUInt;
	
	
	if(bufferSizeBytes == message->dataSize)
	{
		// Unpack Message Fields from Buffer
		if(!jausByteFromBuffer(&message->numJoints, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_BYTE_SIZE_BYTES;

		if(message->numJoints == 0)
		{
			message->jointType = NULL;
			message->jointOffset = NULL;
			message->linkLength = NULL;
			message->twistAngle = NULL;
			message->jointMinValue = NULL;
			message->jointMaxValue = NULL;
			message->jointMaxVelocity = NULL;
			return JAUS_TRUE;
		}

		message->jointType = (JausByte *)malloc(sizeof(JausByte)*message->numJoints);
		message->jointOffset = (JausDouble *)malloc(sizeof(JausDouble)*message->numJoints);
		message->linkLength = (JausDouble *)malloc(sizeof(JausDouble)*message->numJoints - 1);
		message->twistAngle = (JausDouble *)malloc(sizeof(JausDouble)*message->numJoints - 1);
		message->jointMinValue = (JausDouble *)malloc(sizeof(JausDouble)*message->numJoints);
		message->jointMaxValue = (JausDouble *)malloc(sizeof(JausDouble)*message->numJoints);
		message->jointMaxVelocity = (JausDouble *)malloc(sizeof(JausDouble)*message->numJoints);
		
		if(!jausByteFromBuffer(&message->jointType[message->numJoints - 1], buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_BYTE_SIZE_BYTES;

		if(!jausUnsignedShortFromBuffer(&tempUShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
		message->jointOffset[message->numJoints - 1] = (JausDouble)tempUShort;
		if(message->jointType[message->numJoints - 1] == 1) // Handle special scaling for revolute joints
		{
			message->jointOffset[message->numJoints - 1] *= 0.001;		
		}

		if(!jausUnsignedShortFromBuffer(&tempUShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
		message->jointMinValue[message->numJoints - 1] = (JausDouble)tempUShort;
		if(message->jointType[message->numJoints - 1] == 1) // Handle special scaling for revolute joints
		{
			message->jointMinValue[message->numJoints - 1] *= 0.001;		
		}

		if(!jausUnsignedShortFromBuffer(&tempUShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
		message->jointMaxValue[message->numJoints - 1] = (JausDouble)tempUShort;
		if(message->jointType[message->numJoints - 1] == 1) // Handle special scaling for revolute joints
		{
			message->jointMaxValue[message->numJoints - 1] *= 0.001;		
		}

		if(!jausUnsignedShortFromBuffer(&tempUShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
		message->jointMaxVelocity[message->numJoints - 1] = (JausDouble)tempUShort;
		if(message->jointType[message->numJoints - 1] == 1) // Handle special scaling for revolute joints
		{
			message->jointMaxVelocity[message->numJoints - 1] *= 0.001;		
		}

		if(!jausUnsignedIntegerFromBuffer(&tempUInt, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
		message->coordinateSysX = jausUnsignedIntegerToDouble(tempUInt, -30, 30); // Scaled Short (-30, 30)

		if(!jausUnsignedIntegerFromBuffer(&tempUInt, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
		message->coordinateSysY = jausUnsignedIntegerToDouble(tempUInt, -30, 30); // Scaled Short (-30, 30)

		if(!jausUnsignedIntegerFromBuffer(&tempUInt, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
		message->coordinateSysZ = jausUnsignedIntegerToDouble(tempUInt, -30, 30); // Scaled Short (-30, 30)			

		if(!jausUnsignedIntegerFromBuffer(&tempUInt, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
		message->coordinateSysD = jausUnsignedIntegerToDouble(tempUInt, -1, 1); // Scaled Short (-30, 30)				

		if(!jausUnsignedIntegerFromBuffer(&tempUInt, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
		message->coordinateSysA = jausUnsignedIntegerToDouble(tempUInt, -1, 1); // Scaled Short (-1, 1)

		if(!jausUnsignedIntegerFromBuffer(&tempUInt, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
		message->coordinateSysB = jausUnsignedIntegerToDouble(tempUInt, -1, 1); // Scaled Short (-1, 1)

		if(!jausUnsignedIntegerFromBuffer(&tempUInt, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
		message->coordinateSysC = jausUnsignedIntegerToDouble(tempUInt, -1, 1); // Scaled Short (-1, 1)


		for(i=0; i<message->numJoints - 1; i++)
		{
			if(!jausByteFromBuffer(&message->jointType[i], buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_BYTE_SIZE_BYTES;

			if(!jausUnsignedShortFromBuffer(&tempUShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			message->linkLength[i] = (JausDouble)tempUShort;

			if(!jausUnsignedShortFromBuffer(&tempUShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			message->twistAngle[i] = (JausDouble)tempUShort * 0.001;
	
			if(!jausUnsignedShortFromBuffer(&tempUShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			message->jointOffset[i] = (JausDouble)tempUShort;
			if(message->jointType[i] == 1) // Handle special scaling for revolute joints
			{
				message->jointOffset[i] *= 0.001;		
			}
	
			if(!jausUnsignedShortFromBuffer(&tempUShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			message->jointMinValue[i] = (JausDouble)tempUShort;
			if(message->jointType[i] == 1) // Handle special scaling for revolute joints
			{
				message->jointMinValue[i] *= 0.001;		
			}
	
			if(!jausUnsignedShortFromBuffer(&tempUShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			message->jointMaxValue[i] = (JausDouble)tempUShort;
			if(message->jointType[i] == 1) // Handle special scaling for revolute joints
			{
				message->jointMaxValue[i] *= 0.001;		
			}
	
			if(!jausUnsignedShortFromBuffer(&tempUShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			message->jointMaxVelocity[i] = (JausDouble)tempUShort;
			if(message->jointType[i] == 1) // Handle special scaling for revolute joints
			{
				message->jointMaxVelocity[i] *= 0.001;		
			}
		}

		return JAUS_TRUE;
	}
	else
	{
		return JAUS_FALSE;
	}
}

// Returns number of bytes put into the buffer
static int dataToBuffer(ReportManipulatorSpecificationsMessage message, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	int index = 0;
	int i;
	JausUnsignedShort tempUShort;
	JausUnsignedInteger tempUInt;

	if(bufferSizeBytes >= dataSize(message))
	{
		// Pack Message Fields to Buffer
		// Unpack Message Fields from Buffer
		if(!jausByteToBuffer(message->numJoints, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_BYTE_SIZE_BYTES;

		if(message->numJoints == 0)
		{
			return index;
		}

		if(!jausByteToBuffer(message->jointType[message->numJoints - 1], buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_BYTE_SIZE_BYTES;

		if(message->jointType[message->numJoints - 1] == 1) // Handle special scaling for revolute joints
		{
			message->jointOffset[message->numJoints - 1] *= 1000.0;		
		}
		tempUShort = (JausUnsignedShort)message->jointOffset[message->numJoints - 1];
		if(!jausUnsignedShortToBuffer(tempUShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;

		if(message->jointType[message->numJoints - 1] == 1) // Handle special scaling for revolute joints
		{
			message->jointMinValue[message->numJoints - 1] *= 1000.0;		
		}
		tempUShort = (JausUnsignedShort)message->jointMinValue[message->numJoints - 1];
		if(!jausUnsignedShortToBuffer(tempUShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;

		if(message->jointType[message->numJoints - 1] == 1) // Handle special scaling for revolute joints
		{
			message->jointMaxValue[message->numJoints - 1] *= 1000.0;		
		}
		tempUShort = (JausUnsignedShort)message->jointMaxValue[message->numJoints - 1];
		if(!jausUnsignedShortToBuffer(tempUShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;

		if(message->jointType[message->numJoints - 1] == 1) // Handle special scaling for revolute joints
		{
			message->jointMaxVelocity[message->numJoints - 1] *= 1000.0;		
		}
		tempUShort = (JausUnsignedShort)message->jointMaxVelocity[message->numJoints - 1];
		if(!jausUnsignedShortToBuffer(tempUShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
		
		tempUInt = jausUnsignedIntegerFromDouble(message->coordinateSysX, -30, 30); // Scaled Short (-30, 30)
		if(!jausUnsignedIntegerToBuffer(tempUInt, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_UNSIGNED_INTEGER_SIZE_BYTES;

		tempUInt = jausUnsignedIntegerFromDouble(message->coordinateSysY, -30, 30); // Scaled Short (-30, 30)
		if(!jausUnsignedIntegerToBuffer(tempUInt, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_UNSIGNED_INTEGER_SIZE_BYTES;

		tempUInt = jausUnsignedIntegerFromDouble(message->coordinateSysZ, -30, 30); // Scaled Short (-30, 30)			
		if(!jausUnsignedIntegerToBuffer(tempUInt, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_UNSIGNED_INTEGER_SIZE_BYTES;

		tempUInt = jausUnsignedIntegerFromDouble(message->coordinateSysD, -1, 1); // Scaled Short (-30, 30)				
		if(!jausUnsignedIntegerToBuffer(tempUInt, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_UNSIGNED_INTEGER_SIZE_BYTES;

		tempUInt = jausUnsignedIntegerFromDouble(message->coordinateSysA, -1, 1); // Scaled Short (-1, 1)
		if(!jausUnsignedIntegerToBuffer(tempUInt, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_UNSIGNED_INTEGER_SIZE_BYTES;

		tempUInt = jausUnsignedIntegerFromDouble(message->coordinateSysB, -1, 1); // Scaled Short (-1, 1)
		if(!jausUnsignedIntegerToBuffer(tempUInt, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_UNSIGNED_INTEGER_SIZE_BYTES;

		tempUInt = jausUnsignedIntegerFromDouble(message->coordinateSysC, -1, 1); // Scaled Short (-1, 1)
		if(!jausUnsignedIntegerToBuffer(tempUInt, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_UNSIGNED_INTEGER_SIZE_BYTES;


		for(i=0; i<message->numJoints - 1; i++)
		{
			if(!jausByteToBuffer(message->jointType[i], buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_BYTE_SIZE_BYTES;

			tempUShort = (JausUnsignedShort)message->linkLength[i];
			if(!jausUnsignedShortToBuffer(tempUShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;

			tempUShort = (JausUnsignedShort) (message->twistAngle[i] * 1000.0);
			if(!jausUnsignedShortToBuffer(tempUShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
	
			if(message->jointType[i] == 1) // Handle special scaling for revolute joints
			{
				message->jointOffset[i] *= 1000.0;		
			}
			tempUShort = (JausUnsignedShort)message->jointOffset[i];
			if(!jausUnsignedShortToBuffer(tempUShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
	
			if(message->jointType[i] == 1) // Handle special scaling for revolute joints
			{
				message->jointMinValue[i] *= 1000.0;		
			}
			tempUShort = (JausUnsignedShort)message->jointMinValue[i];
			if(!jausUnsignedShortToBuffer(tempUShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
	
			if(message->jointType[i] == 1) // Handle special scaling for revolute joints
			{
				message->jointMaxValue[i] *= 1000.0;		
			}
			tempUShort = (JausUnsignedShort)message->jointMaxValue[i];
			if(!jausUnsignedShortToBuffer(tempUShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
	
			if(message->jointType[i] == 1) // Handle special scaling for revolute joints
			{
				message->jointMaxVelocity[i] *= 1000.0;		
			}
			tempUShort = (JausUnsignedShort)message->jointMaxVelocity[i];
			if(!jausUnsignedShortToBuffer(tempUShort, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
		}
		
	}

	return index;
}

// Returns number of bytes put into the buffer
static unsigned int dataSize(ReportManipulatorSpecificationsMessage message)
{
	int index = 0;

	// Num joints
	index += JAUS_BYTE_SIZE_BYTES;

	if(message->numJoints == 0)
	{
		return index;
	}
	
	// Joint Types
	index += message->numJoints * JAUS_BYTE_SIZE_BYTES;

	// Joint offset, max, min, & max velocity
	index += message->numJoints * 4 * JAUS_UNSIGNED_SHORT_SIZE_BYTES;

	// link lengths & twist angles
	index += (message->numJoints - 1) * 2 * JAUS_UNSIGNED_SHORT_SIZE_BYTES;

	// Coordinate system
	index += 7 * JAUS_UNSIGNED_INTEGER_SIZE_BYTES;

	return index;
}

// ************************************************************************************************************** //
//                                    NON-USER CONFIGURED FUNCTIONS
// ************************************************************************************************************** //

ReportManipulatorSpecificationsMessage reportManipulatorSpecificationsMessageCreate(void)
{
	ReportManipulatorSpecificationsMessage message;

	message = (ReportManipulatorSpecificationsMessage)malloc( sizeof(ReportManipulatorSpecificationsMessageStruct) );
	if(message == NULL)
	{
		return NULL;
	}
	
	// Initialize Values
	message->properties.priority = JAUS_DEFAULT_PRIORITY;
	message->properties.ackNak = JAUS_ACK_NAK_NOT_REQUIRED;
	message->properties.scFlag = JAUS_NOT_SERVICE_CONNECTION_MESSAGE;
	message->properties.expFlag = JAUS_NOT_EXPERIMENTAL_MESSAGE;
	message->properties.version = JAUS_VERSION_3_3;
	message->properties.reserved = 0;
	message->commandCode = commandCode;
	message->destination = jausAddressCreate();
	message->source = jausAddressCreate();
	message->dataFlag = JAUS_SINGLE_DATA_PACKET;
	message->dataSize = maxDataSizeBytes;
	message->sequenceNumber = 0;
	
	dataInitialize(message);
	message->dataSize = dataSize(message);
	
	return message;	
}

void reportManipulatorSpecificationsMessageDestroy(ReportManipulatorSpecificationsMessage message)
{
	dataDestroy(message);
	jausAddressDestroy(message->source);
	jausAddressDestroy(message->destination);
	free(message);
}

JausBoolean reportManipulatorSpecificationsMessageFromBuffer(ReportManipulatorSpecificationsMessage message, unsigned char* buffer, unsigned int bufferSizeBytes)
{
	int index = 0;
	
	if(headerFromBuffer(message, buffer+index, bufferSizeBytes-index))
	{
		index += JAUS_HEADER_SIZE_BYTES;
		if(dataFromBuffer(message, buffer+index, bufferSizeBytes-index))
		{
			return JAUS_TRUE;
		}
		else
		{
			return JAUS_FALSE;
		}
	}
	else
	{
		return JAUS_FALSE;
	}
}

JausBoolean reportManipulatorSpecificationsMessageToBuffer(ReportManipulatorSpecificationsMessage message, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	if(bufferSizeBytes < reportManipulatorSpecificationsMessageSize(message))
	{
		return JAUS_FALSE; //improper size	
	}
	else
	{	
		message->dataSize = dataToBuffer(message, buffer+JAUS_HEADER_SIZE_BYTES, bufferSizeBytes - JAUS_HEADER_SIZE_BYTES);
		if(headerToBuffer(message, buffer, bufferSizeBytes))
		{
			return JAUS_TRUE;
		}
		else
		{
			return JAUS_FALSE; // headerToReportManipulatorSpecificationsBuffer failed
		}
	}
}

ReportManipulatorSpecificationsMessage reportManipulatorSpecificationsMessageFromJausMessage(JausMessage jausMessage)
{
	ReportManipulatorSpecificationsMessage message;
	
	if(jausMessage->commandCode != commandCode)
	{
		return NULL; // Wrong message type
	}
	else
	{
		message = (ReportManipulatorSpecificationsMessage)malloc( sizeof(ReportManipulatorSpecificationsMessageStruct) );
		if(message == NULL)
		{
			return NULL;
		}
		
		message->properties.priority = jausMessage->properties.priority;
		message->properties.ackNak = jausMessage->properties.ackNak;
		message->properties.scFlag = jausMessage->properties.scFlag;
		message->properties.expFlag = jausMessage->properties.expFlag;
		message->properties.version = jausMessage->properties.version;
		message->properties.reserved = jausMessage->properties.reserved;
		message->commandCode = jausMessage->commandCode;
		message->destination = jausAddressCreate();
		*message->destination = *jausMessage->destination;
		message->source = jausAddressCreate();
		*message->source = *jausMessage->source;
		message->dataSize = jausMessage->dataSize;
		message->dataFlag = jausMessage->dataFlag;
		message->sequenceNumber = jausMessage->sequenceNumber;
		
		// Unpack jausMessage->data
		if(dataFromBuffer(message, jausMessage->data, jausMessage->dataSize))
		{
			return message;
		}
		else
		{
			return NULL;
		}
	}
}

JausMessage reportManipulatorSpecificationsMessageToJausMessage(ReportManipulatorSpecificationsMessage message)
{
	JausMessage jausMessage;
	
	jausMessage = (JausMessage)malloc( sizeof(struct JausMessageStruct) );
	if(jausMessage == NULL)
	{
		return NULL;
	}	
	
	jausMessage->properties.priority = message->properties.priority;
	jausMessage->properties.ackNak = message->properties.ackNak;
	jausMessage->properties.scFlag = message->properties.scFlag;
	jausMessage->properties.expFlag = message->properties.expFlag;
	jausMessage->properties.version = message->properties.version;
	jausMessage->properties.reserved = message->properties.reserved;
	jausMessage->commandCode = message->commandCode;
	jausMessage->destination = jausAddressCreate();
	*jausMessage->destination = *message->destination;
	jausMessage->source = jausAddressCreate();
	*jausMessage->source = *message->source;
	jausMessage->dataSize = dataSize(message);
	jausMessage->dataFlag = message->dataFlag;
	jausMessage->sequenceNumber = message->sequenceNumber;
	
	jausMessage->data = (unsigned char *)malloc(jausMessage->dataSize);
	jausMessage->dataSize = dataToBuffer(message, jausMessage->data, jausMessage->dataSize);
	
	return jausMessage;
}

unsigned int reportManipulatorSpecificationsMessageSize(ReportManipulatorSpecificationsMessage message)
{
	return (unsigned int)(dataSize(message) + JAUS_HEADER_SIZE_BYTES);
}

//********************* PRIVATE HEADER FUNCTIONS **********************//

static JausBoolean headerFromBuffer(ReportManipulatorSpecificationsMessage message, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	if(bufferSizeBytes < JAUS_HEADER_SIZE_BYTES)
	{
		return JAUS_FALSE;
	}
	else
	{
		// unpack header
		message->properties.priority = (buffer[0] & 0x0F);
		message->properties.ackNak	 = ((buffer[0] >> 4) & 0x03);
		message->properties.scFlag	 = ((buffer[0] >> 6) & 0x01);
		message->properties.expFlag	 = ((buffer[0] >> 7) & 0x01);
		message->properties.version	 = (buffer[1] & 0x3F);
		message->properties.reserved = ((buffer[1] >> 6) & 0x03);
		
		message->commandCode = buffer[2] + (buffer[3] << 8);
	
		message->destination->instance = buffer[4];
		message->destination->component = buffer[5];
		message->destination->node = buffer[6];
		message->destination->subsystem = buffer[7];
	
		message->source->instance = buffer[8];
		message->source->component = buffer[9];
		message->source->node = buffer[10];
		message->source->subsystem = buffer[11];
		
		message->dataSize = buffer[12] + ((buffer[13] & 0x0F) << 8);

		message->dataFlag = ((buffer[13] >> 4) & 0x0F);

		message->sequenceNumber = buffer[14] + (buffer[15] << 8);
		
		return JAUS_TRUE;
	}
}

static JausBoolean headerToBuffer(ReportManipulatorSpecificationsMessage message, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	JausUnsignedShort *propertiesPtr = (JausUnsignedShort*)&message->properties;
	
	if(bufferSizeBytes < JAUS_HEADER_SIZE_BYTES)
	{
		return JAUS_FALSE;
	}
	else
	{	
		buffer[0] = (unsigned char)(*propertiesPtr & 0xFF);
		buffer[1] = (unsigned char)((*propertiesPtr & 0xFF00) >> 8);

		buffer[2] = (unsigned char)(message->commandCode & 0xFF);
		buffer[3] = (unsigned char)((message->commandCode & 0xFF00) >> 8);

		buffer[4] = (unsigned char)(message->destination->instance & 0xFF);
		buffer[5] = (unsigned char)(message->destination->component & 0xFF);
		buffer[6] = (unsigned char)(message->destination->node & 0xFF);
		buffer[7] = (unsigned char)(message->destination->subsystem & 0xFF);

		buffer[8] = (unsigned char)(message->source->instance & 0xFF);
		buffer[9] = (unsigned char)(message->source->component & 0xFF);
		buffer[10] = (unsigned char)(message->source->node & 0xFF);
		buffer[11] = (unsigned char)(message->source->subsystem & 0xFF);
		
		buffer[12] = (unsigned char)(message->dataSize & 0xFF);
		buffer[13] = (unsigned char)((message->dataFlag & 0xFF) << 4) | (unsigned char)((message->dataSize & 0x0F00) >> 8);

		buffer[14] = (unsigned char)(message->sequenceNumber & 0xFF);
		buffer[15] = (unsigned char)((message->sequenceNumber & 0xFF00) >> 8);
		
		return JAUS_TRUE;
	}
}

