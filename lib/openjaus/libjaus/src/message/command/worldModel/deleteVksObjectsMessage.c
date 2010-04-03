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
// File Name: deleteVksObjectsMessage.c
//
// Written By: Danny Kent (jaus AT dannykent DOT com), Tom Galluzzo (galluzzo AT gmail DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: This file defines the functionality of a DeleteVksObjectsMessage


#include <stdlib.h>
#include <string.h>
#include "jaus.h"

static const int commandCode = JAUS_DELETE_VKS_OBJECTS;
static const int maxDataSizeBytes = 0;

static JausBoolean headerFromBuffer(DeleteVksObjectsMessage message, unsigned char *buffer, unsigned int bufferSizeBytes);
static JausBoolean headerToBuffer(DeleteVksObjectsMessage message, unsigned char *buffer, unsigned int bufferSizeBytes);

static JausBoolean dataFromBuffer(DeleteVksObjectsMessage message, unsigned char *buffer, unsigned int bufferSizeBytes);
static int dataToBuffer(DeleteVksObjectsMessage message, unsigned char *buffer, unsigned int bufferSizeBytes);
static void dataInitialize(DeleteVksObjectsMessage message);
static void dataDestroy(DeleteVksObjectsMessage message);
static unsigned int dataSize(DeleteVksObjectsMessage message);

// ************************************************************************************************************** //
//                                    USER CONFIGURED FUNCTIONS
// ************************************************************************************************************** //

// Initializes the message-specific fields
static void dataInitialize(DeleteVksObjectsMessage message)
{
	// Set initial values of message fields
	message->presenceVector = newJausByte(JAUS_BYTE_PRESENCE_VECTOR_ALL_ON);
	message->requestId = newJausByte(0);
	message->objectCount = newJausUnsignedShort(0);
	message->objectIds = NULL;
	message->deletionRegion = vectorObjectCreate();	
}

// Destructs the message-specific fields
static void dataDestroy(DeleteVksObjectsMessage message)
{
	// Free message fields
	if(message->objectIds) 
		free(message->objectIds);
	
	vectorObjectDestroy(message->deletionRegion);	
}

// Return boolean of success
static JausBoolean dataFromBuffer(DeleteVksObjectsMessage message, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	int index = 0;
	int i = 0;
	JausByte featureClassCount;
	JausUnsignedShort dataPointCount;
	JausInteger tempInt;
	JausWorldModelFeatureClass fcClass = NULL;
	JausGeometryPointLLA point = NULL;

	if(bufferSizeBytes == message->dataSize)
	{
		// Unpack Message Fields from Buffer
		// Presence Vector
		if(!jausByteFromBuffer(&message->presenceVector, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_BYTE_SIZE_BYTES;
		
		// Request Id
		if(!jausByteFromBuffer(&message->requestId, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_BYTE_SIZE_BYTES;
		
		// Test if objectCount is specified
		if(jausByteIsBitSet(message->presenceVector, VKS_PV_DELETE_OBJECTS_ID_BIT))
		{
			// Request Id
			if(!jausUnsignedShortFromBuffer(&message->objectCount, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_BYTE_SIZE_BYTES;
			
			// Allocate memory for objectIds
			message->objectIds = (JausUnsignedInteger *)malloc(message->objectCount * JAUS_UNSIGNED_INTEGER_SIZE_BYTES);
			
			// Unpack Object Ids
			for(i = 0; i < message->objectCount; i++)
			{
				if(!jausUnsignedIntegerFromBuffer(&message->objectIds[i], buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
				index += JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
			}
		}
		
		// Create the DeletionRegion object
		message->deletionRegion = vectorObjectCreate();
		if(!message->deletionRegion) return JAUS_FALSE;
					
		// Test if a deletionRegion is specified
		if(jausByteIsBitSet(message->presenceVector, VKS_PV_DELETE_OBJECTS_REGION_BIT))
		{
			// Unpack Region Type
			if(!jausByteFromBuffer(&message->deletionRegion->type, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_BYTE_SIZE_BYTES;
			
			// Buffer is optional
			if(jausByteIsBitSet(message->presenceVector, VKS_PV_DELETE_OBJECTS_REGION_BIT))
			{
				// Unpack Buffer
				if(!jausFloatFromBuffer(&message->deletionRegion->bufferMeters, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
				index += JAUS_FLOAT_SIZE_BYTES;
			}
		}
	
		// Test if Feature Class provided
		if(jausByteIsBitSet(message->presenceVector, VKS_PV_DELETE_OBJECTS_FC_COUNT_BIT))
		{
			// FC Count
			if(!jausByteFromBuffer(&featureClassCount, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_BYTE_SIZE_BYTES;
		
			for(i = 0; i < featureClassCount; i++)
			{
				// Create JausWorldModelFeatureClass Object
				fcClass = featureClassCreate();
				if(!fcClass) return JAUS_FALSE;
	
				// This should ALWAYS be true, because VKS_PV_DELETE_OBJECTS_FC_COUNT_BIT is set
				if(jausByteIsBitSet(message->presenceVector, VKS_PV_DELETE_OBJECTS_FEATURE_CLASS_BIT))
				{
					// Unpack FC Id
					if(!jausUnsignedShortFromBuffer(&fcClass->id, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
					index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
				}
					
				if(jausByteIsBitSet(message->presenceVector, VKS_PV_DELETE_OBJECTS_ATTRIBUTE_BIT))
				{
					//Destroy the Attribute created by featureClassCreate
					featureClassAttributeDestroy(fcClass->attribute);
					
					// Unpack Attribute Data Type & Value
					fcClass->attribute = featureClassAttributeFromBuffer(buffer+index, bufferSizeBytes-index);
					if(!fcClass->attribute) return JAUS_FALSE;
					index += featureClassAttributeSizeBytes(fcClass->attribute);
				}
					
				// Add FC to array
				jausArrayAdd(message->deletionRegion->featureClasses, fcClass);
			}
		}
		
		// Test if region points specified
		// This should be true if the VKS_PV_DELETE_OBJECTS_REGION_BIT is set
		if(jausByteIsBitSet(message->presenceVector, VKS_PV_DELETE_OBJECTS_POINT_COUNT_BIT))
		{
			// Unpack Point Count
			if(!jausUnsignedShortFromBuffer(&dataPointCount, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			
			for(i = 0; i < dataPointCount; i++)
			{
				point = jausGeometryPointLLACreate();
				if(!point) return JAUS_FALSE;
			
				// unpack Latitude
				if(!jausIntegerFromBuffer(&tempInt, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
				index += JAUS_INTEGER_SIZE_BYTES;
				
				// Scaled Int (-90, 90)
				point->latitudeRadians = jausIntegerToDouble(tempInt, -90, 90) * JAUS_RAD_PER_DEG;
				
				// unpack Latitude
				if(!jausIntegerFromBuffer(&tempInt, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
				index += JAUS_INTEGER_SIZE_BYTES;
				
				// Scaled Int (-180, 180)
				point->longitudeRadians = jausIntegerToDouble(tempInt, -180, 180) * JAUS_RAD_PER_DEG;
				
				jausArrayAdd(message->deletionRegion->dataPoints, point);
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
static int dataToBuffer(DeleteVksObjectsMessage message, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	unsigned int index = 0;
	int i = 0;
	JausWorldModelFeatureClass fcClass = NULL;
	JausGeometryPointLLA point = NULL;
	JausInteger tempInt;

	if(bufferSizeBytes >= dataSize(message))
	{
		// Ensure the PV rules are met
		// If the VKS_PV_DELETE_OBJECTS_REGION_BIT is set, then the VKS_PV_DELETE_OBJECTS_POINT_COUNT_BIT is required
		// The VKS_PV_DELETE_OBJECTS_BUFFER_BIT cannot be set without the VKS_PV_DELETE_OBJECTS_REGION_BIT
		if(jausByteIsBitSet(message->presenceVector, VKS_PV_DELETE_OBJECTS_REGION_BIT))
		{
			jausByteSetBit(&message->presenceVector, VKS_PV_DELETE_OBJECTS_POINT_COUNT_BIT);
		}
		else
		{
			jausByteClearBit(&message->presenceVector, VKS_PV_DELETE_OBJECTS_POINT_COUNT_BIT);
			jausByteClearBit(&message->presenceVector, VKS_PV_DELETE_OBJECTS_BUFFER_BIT);
		}

		// If the VKS_PV_DELETE_OBJECTS_FEATURE_CLASS_BIT is set or the VKS_PV_DELETE_OBJECTS_ATTRIBUTE_BIT is set, 
		// then the VKS_PV_DELETE_OBJECTS_FC_COUNT_BIT is required
		if(	jausByteIsBitSet(message->presenceVector, VKS_PV_DELETE_OBJECTS_ATTRIBUTE_BIT) ||
			jausByteIsBitSet(message->presenceVector, VKS_PV_DELETE_OBJECTS_FEATURE_CLASS_BIT))
		{
			jausByteSetBit(&message->presenceVector, VKS_PV_DELETE_OBJECTS_FC_COUNT_BIT);
		}
		else
		{
			jausByteClearBit(&message->presenceVector, VKS_PV_DELETE_OBJECTS_FC_COUNT_BIT);
		}

		// Pack Message Fields to Buffer
		// Presence Vector
		if(!jausByteToBuffer(message->presenceVector, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_BYTE_SIZE_BYTES;
	
		// Request Id
		if(!jausByteToBuffer(message->requestId, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_BYTE_SIZE_BYTES;

		// objectCount is optional
		if(jausByteIsBitSet(message->presenceVector, VKS_PV_DELETE_OBJECTS_ID_BIT))
		{
			// Object Count
			if(!jausUnsignedShortToBuffer(message->objectCount, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
					
			// Pack Object Ids
			for(i = 0; i < message->objectCount; i++)
			{
				if(!jausUnsignedIntegerToBuffer(message->objectIds[i], buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
				index += JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
			}
		}

		// Region is Optional
		if(jausByteIsBitSet(message->presenceVector, VKS_PV_DELETE_OBJECTS_REGION_BIT))
		{
			// Region Type
			if(!jausByteToBuffer(message->deletionRegion->type, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_BYTE_SIZE_BYTES;
			
			// Buffer is Optional
			if(jausByteIsBitSet(message->presenceVector, VKS_PV_DELETE_OBJECTS_REGION_BIT))
			{
				// Buffer
				if(!jausFloatToBuffer(message->deletionRegion->bufferMeters, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
				index += JAUS_FLOAT_SIZE_BYTES;
			}
		}
		
		// Feature Class Information is optional
		if(jausByteIsBitSet(message->presenceVector, VKS_PV_DELETE_OBJECTS_FC_COUNT_BIT))
		{
			// Feature Class Count
			if(!jausByteToBuffer((JausByte)message->deletionRegion->featureClasses->elementCount, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_BYTE_SIZE_BYTES;
		}			
		
		if(jausByteIsBitSet(message->presenceVector, VKS_PV_DELETE_OBJECTS_FEATURE_CLASS_BIT))
		{
			for(i = 0; i < message->deletionRegion->featureClasses->elementCount; i++)
			{
				fcClass = (JausWorldModelFeatureClass) message->deletionRegion->featureClasses->elementData[i];

				// Feature Class Id
				if(!jausUnsignedShortToBuffer(fcClass->id, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
				index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
				
				// Attribute is Optional
				if(jausByteIsBitSet(message->presenceVector, VKS_PV_DELETE_OBJECTS_ATTRIBUTE_BIT))
				{
					if(!featureClassAttributeToBuffer(fcClass->attribute, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
					index += featureClassAttributeSizeBytes(fcClass->attribute);
				}
			}
		}
		else if(jausByteIsBitSet(message->presenceVector, VKS_PV_DELETE_OBJECTS_ATTRIBUTE_BIT))
		{
			for(i = 0; i < message->deletionRegion->featureClasses->elementCount; i++)
			{
				fcClass = (JausWorldModelFeatureClass) message->deletionRegion->featureClasses->elementData[i];
	
				if(!featureClassAttributeToBuffer(fcClass->attribute, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
				index += featureClassAttributeSizeBytes(fcClass->attribute);	
			}
		}
		
		// Region is optional, if VKS_PV_DELETE_OBJECTS_REGION_BIT set, points required
		if(jausByteIsBitSet(message->presenceVector, VKS_PV_DELETE_OBJECTS_POINT_COUNT_BIT))
		{
			// Point Count
			if(!jausUnsignedShortToBuffer((JausUnsignedShort)message->deletionRegion->dataPoints->elementCount, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
			index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;

			for(i = 0; i < message->deletionRegion->dataPoints->elementCount; i++)
			{
				point = (JausGeometryPointLLA) message->deletionRegion->dataPoints->elementData[i];
	
				// Scaled Int (-90, 90)
				tempInt = jausIntegerFromDouble((point->latitudeRadians * JAUS_DEG_PER_RAD), -90, 90);
	
				//pack Latitude
				if(!jausIntegerToBuffer(tempInt, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
				index += JAUS_INTEGER_SIZE_BYTES;
	
				// Scaled Int (-180, 180)
				tempInt = jausIntegerFromDouble((point->longitudeRadians * JAUS_DEG_PER_RAD), -180, 180);
	
				//pack Longitude
				if(!jausIntegerToBuffer(tempInt, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
				index += JAUS_INTEGER_SIZE_BYTES;
			}
		}
	}

	return index;
}

static unsigned int dataSize(DeleteVksObjectsMessage message)
{
	unsigned int index = 0;
	int i = 0;
	JausWorldModelFeatureClass fcClass = NULL;

	// Ensure the PV rules are met
	// If the VKS_PV_DELETE_OBJECTS_REGION_BIT is set, then the VKS_PV_DELETE_OBJECTS_POINT_COUNT_BIT is required
	// The VKS_PV_DELETE_OBJECTS_BUFFER_BIT cannot be set without the VKS_PV_DELETE_OBJECTS_REGION_BIT
	if(jausByteIsBitSet(message->presenceVector, VKS_PV_DELETE_OBJECTS_REGION_BIT))
	{
		jausByteSetBit(&message->presenceVector, VKS_PV_DELETE_OBJECTS_POINT_COUNT_BIT);
	}
	else
	{
		jausByteClearBit(&message->presenceVector, VKS_PV_DELETE_OBJECTS_POINT_COUNT_BIT);
		jausByteClearBit(&message->presenceVector, VKS_PV_DELETE_OBJECTS_BUFFER_BIT);
	}

	// If the VKS_PV_DELETE_OBJECTS_FEATURE_CLASS_BIT is set or the VKS_PV_DELETE_OBJECTS_ATTRIBUTE_BIT is set, 
	// then the VKS_PV_DELETE_OBJECTS_FC_COUNT_BIT is required
	if(	jausByteIsBitSet(message->presenceVector, VKS_PV_DELETE_OBJECTS_ATTRIBUTE_BIT) ||
		jausByteIsBitSet(message->presenceVector, VKS_PV_DELETE_OBJECTS_FEATURE_CLASS_BIT))
	{
		jausByteSetBit(&message->presenceVector, VKS_PV_DELETE_OBJECTS_FC_COUNT_BIT);
	}
	else
	{
		jausByteClearBit(&message->presenceVector, VKS_PV_DELETE_OBJECTS_FC_COUNT_BIT);
	}

	// Pack Message Fields to Buffer
	// Presence Vector
	index += JAUS_BYTE_SIZE_BYTES;

	// Request Id
	index += JAUS_BYTE_SIZE_BYTES;

	// objectCount is optional
	if(jausByteIsBitSet(message->presenceVector, VKS_PV_DELETE_OBJECTS_ID_BIT))
	{
		// Object Count
		index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
				
		// Pack Object Ids
		for(i = 0; i < message->objectCount; i++)
		{
			index += JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
		}
	}

	// Region is Optional
	if(jausByteIsBitSet(message->presenceVector, VKS_PV_DELETE_OBJECTS_REGION_BIT))
	{
		// Region Type
		index += JAUS_BYTE_SIZE_BYTES;
		
		// Buffer is Optional
		if(jausByteIsBitSet(message->presenceVector, VKS_PV_DELETE_OBJECTS_REGION_BIT))
		{
			// Buffer
			index += JAUS_FLOAT_SIZE_BYTES;
		}
	}
	
	// Feature Class Information is optional
	if(jausByteIsBitSet(message->presenceVector, VKS_PV_DELETE_OBJECTS_FC_COUNT_BIT))
	{
		// Feature Class Count
		index += JAUS_BYTE_SIZE_BYTES;
	}			
	
	if(jausByteIsBitSet(message->presenceVector, VKS_PV_DELETE_OBJECTS_FEATURE_CLASS_BIT))
	{
		for(i = 0; i < message->deletionRegion->featureClasses->elementCount; i++)
		{
			fcClass = (JausWorldModelFeatureClass) message->deletionRegion->featureClasses->elementData[i];

			// Feature Class Id
			index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			
			// Attribute is Optional
			if(jausByteIsBitSet(message->presenceVector, VKS_PV_DELETE_OBJECTS_ATTRIBUTE_BIT))
			{
				index += featureClassAttributeSizeBytes(fcClass->attribute);
			}
		}
	}
	else if(jausByteIsBitSet(message->presenceVector, VKS_PV_DELETE_OBJECTS_ATTRIBUTE_BIT))
	{
		for(i = 0; i < message->deletionRegion->featureClasses->elementCount; i++)
		{
			fcClass = (JausWorldModelFeatureClass) message->deletionRegion->featureClasses->elementData[i];

			index += featureClassAttributeSizeBytes(fcClass->attribute);	
		}
	}
	
	// Region is optional, if VKS_PV_DELETE_OBJECTS_REGION_BIT set, points required
	if(jausByteIsBitSet(message->presenceVector, VKS_PV_DELETE_OBJECTS_POINT_COUNT_BIT))
	{
		// Point Count
		index += JAUS_UNSIGNED_SHORT_SIZE_BYTES;

		for(i = 0; i < message->deletionRegion->dataPoints->elementCount; i++)
		{		
			//Latitude
			index += JAUS_INTEGER_SIZE_BYTES;

			//Longitude
			index += JAUS_INTEGER_SIZE_BYTES;
		}
	}

	return index;
}

// ************************************************************************************************************** //
//                                    NON-USER CONFIGURED FUNCTIONS
// ************************************************************************************************************** //

DeleteVksObjectsMessage deleteVksObjectsMessageCreate(void)
{
	DeleteVksObjectsMessage message;

	message = (DeleteVksObjectsMessage)malloc( sizeof(DeleteVksObjectsMessageStruct) );
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
	
	return message;	
}

void deleteVksObjectsMessageDestroy(DeleteVksObjectsMessage message)
{
	dataDestroy(message);
	jausAddressDestroy(message->source);
	jausAddressDestroy(message->destination);
	free(message);
}

JausBoolean deleteVksObjectsMessageFromBuffer(DeleteVksObjectsMessage message, unsigned char* buffer, unsigned int bufferSizeBytes)
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

JausBoolean deleteVksObjectsMessageToBuffer(DeleteVksObjectsMessage message, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	if(bufferSizeBytes < deleteVksObjectsMessageSize(message))
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
			return JAUS_FALSE; // headerToDeleteVksObjectsBuffer failed
		}
	}
}

DeleteVksObjectsMessage deleteVksObjectsMessageFromJausMessage(JausMessage jausMessage)
{
	DeleteVksObjectsMessage message;
	
	if(jausMessage->commandCode != commandCode)
	{
		return NULL; // Wrong message type
	}
	else
	{
		message = (DeleteVksObjectsMessage)malloc( sizeof(DeleteVksObjectsMessageStruct) );
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

JausMessage deleteVksObjectsMessageToJausMessage(DeleteVksObjectsMessage message)
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


unsigned int deleteVksObjectsMessageSize(DeleteVksObjectsMessage message)
{
	return (unsigned int)(dataSize(message) + JAUS_HEADER_SIZE_BYTES);
}

//********************* PRIVATE HEADER FUNCTIONS **********************//

static JausBoolean headerFromBuffer(DeleteVksObjectsMessage message, unsigned char *buffer, unsigned int bufferSizeBytes)
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

static JausBoolean headerToBuffer(DeleteVksObjectsMessage message, unsigned char *buffer, unsigned int bufferSizeBytes)
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

