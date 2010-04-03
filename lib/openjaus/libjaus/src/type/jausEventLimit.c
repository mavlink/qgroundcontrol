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
// File Name: jausEventLimit.c
//
// Written By: Danny Kent (jaus AT dannykent DOT com), Tom Galluzzo (galluzzo AT gmail DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description:

#include <stdlib.h>
#include <string.h>
#include "jaus.h"

// JausEventLimit Constructor
JausEventLimit jausEventLimitCreate()
{
	JausEventLimit limit;
	limit = (JausEventLimit) malloc(sizeof(JausEventLimitStruct));
	if(limit)
	{
		memset(limit, 0, sizeof(JausEventLimitStruct));
		limit->dataType = EVENT_LIMIT_UNDEFINED_TYPE;
		return limit;
	}
	else
	{
		return NULL;
	}	
}

// JausEventLimit Destructor
void jausEventLimitDestroy(JausEventLimit limit)
{
	if(limit)
	{
		free(limit);
		limit = NULL;
	}		
}

// JausEventLimit Constructor (from Buffer)
JausBoolean jausEventLimitFromBuffer(JausEventLimit *limitPointer, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	JausEventLimit limit;
	JausByte tempByte;
	int index = 0;

	limit = (JausEventLimit) malloc(sizeof(JausEventLimitStruct));
	*limitPointer = limit; 
	
	if(limit)
	{
		memset(limit, 0, sizeof(JausEventLimitStruct));

		if(!jausByteFromBuffer(&tempByte, buffer+index, bufferSizeBytes-index))
		{
			*limitPointer = NULL;
			return JAUS_FALSE;
		}
		limit->dataType = tempByte;
		index += JAUS_BYTE_SIZE_BYTES;

		switch(limit->dataType)
		{
			case EVENT_LIMIT_BYTE_TYPE:
				if(!jausByteFromBuffer(&limit->value.byteValue, buffer+index, bufferSizeBytes-index))
				{
					*limitPointer = NULL;
				}
				break;
				
			case EVENT_LIMIT_SHORT_TYPE:
				if(!jausShortFromBuffer(&limit->value.shortValue, buffer+index, bufferSizeBytes-index))
				{
					*limitPointer = NULL;
				}
				break;
	
			case EVENT_LIMIT_INTEGER_TYPE:
				if(!jausIntegerFromBuffer(&limit->value.integerValue, buffer+index, bufferSizeBytes-index))
				{
					*limitPointer = NULL;
				}
				break;
	
			case EVENT_LIMIT_LONG_TYPE:
				if(!jausLongFromBuffer(&limit->value.longValue, buffer+index, bufferSizeBytes-index))
				{
					*limitPointer = NULL;
				}
				break;
	
			case EVENT_LIMIT_UNSIGNED_SHORT_TYPE:
				if(!jausUnsignedShortFromBuffer(&limit->value.unsignedShortValue, buffer+index, bufferSizeBytes-index))
				{
					*limitPointer = NULL;
				}
				break;
	
			case EVENT_LIMIT_UNSIGNED_INTEGER_TYPE:
				if(!jausUnsignedIntegerFromBuffer(&limit->value.unsignedIntegerValue, buffer+index, bufferSizeBytes-index))
				{
					*limitPointer = NULL;
				}
				break;
	
			case EVENT_LIMIT_UNSIGNED_LONG_TYPE:
				if(!jausUnsignedLongFromBuffer(&limit->value.unsignedLongValue, buffer+index, bufferSizeBytes-index))
				{
					*limitPointer = NULL;
				}
				break;
	
			case EVENT_LIMIT_FLOAT_TYPE:
				if(!jausFloatFromBuffer(&limit->value.floatValue, buffer+index, bufferSizeBytes-index))
				{
					*limitPointer = NULL;
				}
				break;
	
			case EVENT_LIMIT_DOUBLE_TYPE:
				if(!jausDoubleFromBuffer(&limit->value.doubleValue, buffer+index, bufferSizeBytes-index))
				{
					*limitPointer = NULL;
				}
				break;
		
			case EVENT_LIMIT_RGB_TYPE:
				if(!jausByteFromBuffer(&limit->value.rgb.redValue, buffer+index, bufferSizeBytes-index))
				{
					*limitPointer = NULL;
				}
				index += JAUS_BYTE_SIZE_BYTES;
	
				if(!jausByteFromBuffer(&limit->value.rgb.greenValue, buffer+index, bufferSizeBytes-index))
				{
					*limitPointer = NULL;
				}
				index += JAUS_BYTE_SIZE_BYTES;
	
				if(!jausByteFromBuffer(&limit->value.rgb.blueValue, buffer+index, bufferSizeBytes-index))
				{
					*limitPointer = NULL;
				}
				index += JAUS_BYTE_SIZE_BYTES;
				break;
	
			default:
				*limitPointer = NULL;
				break;
		}	
	}

	if(*limitPointer)
	{
		return JAUS_TRUE;
	}
	
	return JAUS_FALSE;
}

// JausEventLimit To Buffer
JausBoolean jausEventLimitToBuffer(JausEventLimit limit, unsigned char *buffer, unsigned int bufferSizeBytes)
{
	unsigned int index = 0;
	if(limit)
	{
		if(limit->dataType < EVENT_LIMIT_BYTE_TYPE) return JAUS_FALSE;
	
		if(!jausByteToBuffer(limit->dataType, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
		index += JAUS_BYTE_SIZE_BYTES;

		// Value
		switch(limit->dataType)
		{
			case EVENT_LIMIT_BYTE_TYPE:
				if(!jausByteToBuffer(limit->value.byteValue, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
				return JAUS_TRUE;
								
			case EVENT_LIMIT_SHORT_TYPE:
				if(!jausShortToBuffer(limit->value.shortValue, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
				return JAUS_TRUE;

			case EVENT_LIMIT_INTEGER_TYPE:
				if(!jausIntegerToBuffer(limit->value.integerValue, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
				return JAUS_TRUE;

			case EVENT_LIMIT_LONG_TYPE:
				if(!jausLongToBuffer(limit->value.longValue, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
				return JAUS_TRUE;

			case EVENT_LIMIT_UNSIGNED_SHORT_TYPE:
				if(!jausUnsignedShortToBuffer(limit->value.unsignedShortValue, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
				return JAUS_TRUE;

			case EVENT_LIMIT_UNSIGNED_INTEGER_TYPE:
				if(!jausUnsignedIntegerToBuffer(limit->value.unsignedIntegerValue, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
				return JAUS_TRUE;

			case EVENT_LIMIT_UNSIGNED_LONG_TYPE:
				if(!jausUnsignedLongToBuffer(limit->value.unsignedLongValue, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
				return JAUS_TRUE;

			case EVENT_LIMIT_FLOAT_TYPE:
				if(!jausFloatToBuffer(limit->value.floatValue, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
				return JAUS_TRUE;

			case EVENT_LIMIT_DOUBLE_TYPE:
				if(!jausDoubleToBuffer(limit->value.doubleValue, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
				return JAUS_TRUE;

			case EVENT_LIMIT_RGB_TYPE:
				if(!jausByteToBuffer(limit->value.rgb.redValue, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
				index += JAUS_BYTE_SIZE_BYTES;

				if(!jausByteToBuffer(limit->value.rgb.greenValue, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
				index += JAUS_BYTE_SIZE_BYTES;

				if(!jausByteToBuffer(limit->value.rgb.blueValue, buffer+index, bufferSizeBytes-index)) return JAUS_FALSE;
				return JAUS_TRUE;

			default:
				return JAUS_FALSE;
		}
	}
	return JAUS_FALSE;
}

unsigned int jausEventLimitSize(JausEventLimit limit)
{
	unsigned int size = 0;
	
	size += JAUS_BYTE_SIZE_BYTES;

	switch(limit->dataType)
	{
		case EVENT_LIMIT_BYTE_TYPE:
			size += JAUS_BYTE_SIZE_BYTES;
			break;
			
		case EVENT_LIMIT_SHORT_TYPE:
			size += JAUS_SHORT_SIZE_BYTES;
			break;

		case EVENT_LIMIT_INTEGER_TYPE:
			size += JAUS_INTEGER_SIZE_BYTES;
			break;

		case EVENT_LIMIT_LONG_TYPE:
			size += JAUS_LONG_SIZE_BYTES;
			break;

		case EVENT_LIMIT_UNSIGNED_SHORT_TYPE:
			size += JAUS_UNSIGNED_SHORT_SIZE_BYTES;
			break;

		case EVENT_LIMIT_UNSIGNED_INTEGER_TYPE:
			size += JAUS_UNSIGNED_INTEGER_SIZE_BYTES;
			break;

		case EVENT_LIMIT_UNSIGNED_LONG_TYPE:
			size += JAUS_UNSIGNED_LONG_SIZE_BYTES;
			break;

		case EVENT_LIMIT_FLOAT_TYPE:
			size += JAUS_FLOAT_SIZE_BYTES;
			break;

		case EVENT_LIMIT_DOUBLE_TYPE:
			size += JAUS_DOUBLE_SIZE_BYTES;
			break;

		case EVENT_LIMIT_RGB_TYPE:
			size += JAUS_BYTE_SIZE_BYTES;	// Red
			size += JAUS_BYTE_SIZE_BYTES;	// Green
			size += JAUS_BYTE_SIZE_BYTES;	// Blue
			break;

		default:
			return 0;			
	}
	return size;
}

JausEvent jausEventCreate()
{
	JausEvent event;
	event = (JausEvent) malloc(sizeof(JausEventStruct));
	
	event->eventMessage = NULL;
	event->eventId = 0;
	event->previousLimitValue = jausEventLimitCreate();
	
	return event;
}

void jausEventDestroy(JausEvent event)
{
	// Note event->eventMessage needs to be freed
	/*if(event->eventMessage)*/ createEventMessageDestroy(event->eventMessage);
	
	jausEventLimitDestroy(event->previousLimitValue);	
	free(event);	
}
